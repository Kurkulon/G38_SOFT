#include "hardware.h"
#include "time.h"
#include "ComPort.h"
#include "crc16.h"
#include <math.h>

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++



u32 fps = 0;

//static byte sec = 0;

static ComPort com;

static u16 manReqWord = 0x0000;
static u16 manReqMask = 0xFF00;

static u16 numDevice = 1;
static u16 verDevice = 0x101;

//static u32 manCounter = 0;

static u16 cal[128+32];
static bool loadCal = true;
static bool saveCal = false;

static Rsp30 *rsp_30 = 0;

static u16 temp = 0;

inline u16 ReverseWord(u16 v) { __asm	{ rev16 v, v };	return v; }

static void* eepromData = 0;
static u16 eepromWriteLen = 0;
static u16 eepromReadLen = 0;
static u16 eepromStartAdr = 0;


//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

static bool WriteEEPROM(void* data, u16 len, u16 adr)
{
	if (data == 0 || len == 0 || eepromWriteLen != 0)
	{
		return false;
	};

	eepromData = data;
	eepromWriteLen = len;
	eepromStartAdr = adr;

	return true;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

static bool ReadEEPROM(void* data, u16 len, u16 adr)
{
	if (data == 0 || len == 0 || eepromReadLen != 0)
	{
		return false;
	};

	eepromData = data;
	eepromReadLen = len;
	eepromStartAdr = adr;

	return true;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

static bool Check_EEPROM_Ready()
{
	return eepromReadLen == 0 && eepromWriteLen == 0;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

static void UpdateTemp()
{
	static byte i = 0;

	static DSCTWI dsc;
	static byte reg = 0;
	static u16 rbuf = 0;
	static byte *romData = 0;
	static u16 romAdr = 0;
	static u16 revRomAdr = 0;
	static u16 romWrLen = 0;
	static u16 romRdLen = 0;
	static u16 pageLen = 0;

	static TM32 tm;

//	HW::GPIO->SET0 = 1<<12;

	switch (i)
	{
		case 0:

			if (eepromWriteLen != 0)
			{
				if (eepromData == 0)
				{
					eepromWriteLen = 0;
				}
				else
				{
					romData = (byte*)eepromData;
					romWrLen = eepromWriteLen;
					romAdr = eepromStartAdr;
					revRomAdr = ReverseWord(romAdr);

					i = 2;
				};
			}
			else if (eepromReadLen != 0)
			{
				if (eepromData == 0)
				{
					eepromReadLen = 0;
				}
				else
				{
					romData = (byte*)eepromData;
					romRdLen = eepromReadLen;
					romAdr = eepromStartAdr;
					revRomAdr = ReverseWord(romAdr);

					i = 5;
				};
			}
			else if (tm.Check(20))
			{
				dsc.wdata = &reg;
				dsc.wlen = 1;
				dsc.rdata = &rbuf;
				dsc.rlen = 2;
				dsc.adr = 0x49;
				dsc.wdata2 = 0;
				dsc.wlen2 = 0;


				if (Write_TWI(&dsc))
				{
					i++;
				};
			};

			break;

		case 1:

			if (Check_TWI_ready())
			{
				temp = ((i16)ReverseWord(rbuf) + 64) / 128;

				i = 0;
			};

			break;

		case 2:		// Write at24c128

			pageLen = (romWrLen > 64) ? 64 : romWrLen;

			dsc.wdata = &revRomAdr;
			dsc.wlen = sizeof(revRomAdr);
			dsc.wdata2 = romData;
			dsc.wlen2 = pageLen;
			dsc.rdata = 0;
			dsc.rlen = 0;
			dsc.adr = 0x50;

			if (Write_TWI(&dsc))
			{
				tm.Reset();

				i++;
			};

			break;

		case 3:

			if (Check_TWI_ready())
			{
				tm.Reset();

				i++;
			};

			break;

		case 4:

			if (tm.Check(10))
			{
				romWrLen -= pageLen;
				romData += pageLen;
				revRomAdr = ReverseWord(romAdr += pageLen);

				if (romWrLen > 0)
				{
					i = 2;
				}
				else
				{
					eepromWriteLen = 0;

					i = 0;
				};

			};

			break;

		case 5:		// Read at24c128

			dsc.wdata = &revRomAdr;
			dsc.wlen = sizeof(revRomAdr);
			dsc.wdata2 = 0;
			dsc.wlen2 = 0;
			dsc.rdata = romData;
			dsc.rlen = romRdLen;
			dsc.adr = 0x50;

			if (Read_TWI(&dsc))
			{
				tm.Reset();

				i++;
			};

			break;

		case 6:

			if (Check_TWI_ready())
			{
				eepromReadLen = 0;

				i = 0;
			};

			break;
	};

//	HW::GPIO->CLR0 = 1<<12;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

static bool RequestMan_00(u16 *data, u16 len, ComPort::WriteBuffer *wb)
{
	static u16 rsp[3];

	if (wb == 0 || len != 1) return false;

	rsp[0] = manReqWord;
	rsp[1] = numDevice;
	rsp[2] = verDevice;

	wb->data = rsp;
	wb->len = sizeof(rsp);

	return true;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

static bool RequestMan_10(u16 *data, u16 len, ComPort::WriteBuffer *wb)
{
	static u16 rsp[1];

	if (wb == 0 || len != 1) return false;

	rsp[0] = manReqWord|0x10;	// 	1. �������� �����

	wb->data = rsp;			 
	wb->len = sizeof(rsp);	 

	return true;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

static bool RequestMan_20(u16 *data, u16 len, ComPort::WriteBuffer *wb)
{
	static u16 rsp[7];

	if (wb == 0 || len != 1) return false;

	rsp[0] = manReqWord|0x20;	//	1.�������� ����� (�������� �������)
	rsp[1] = GetAP();			//	2.�������� (�.�)
	rsp[2] = 100;				//	3.����������� ��������� (�.�)
	rsp[3] = GetAP() / 2;		//	4.�������� (0.01 ���)
	rsp[4] = temp;				//	5.����������� � �������(��)(short)
	rsp[5] = GetShaftPos();		//	6.6.��������� ���� ��������� (short �.�)
	rsp[6] = GetCurrent();		//	7.7.��� ��������� (��)

	wb->data = rsp;
	wb->len = sizeof(rsp);

	return true;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

static bool RequestMan_30(u16 *data, u16 len, ComPort::WriteBuffer *wb)
{
	static u16 rsp0[4];

	if (wb == 0 || len != 1) return false;

	rsp_30 = GetRsp30();

	if (rsp_30 == 0)
	{
		rsp0[0] = data[0];
		rsp0[1] = 0;
		rsp0[2] = 0;
		rsp0[3] = 0;
		wb->data = rsp0;
		wb->len = sizeof(rsp0);
	}
	else
	{
		wb->data = rsp_30;
		wb->len = sizeof(*rsp_30) - sizeof(rsp_30->data) + rsp_30->sl*2;
	};

	return true;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

static bool RequestMan_70(u16 *data, u16 len, ComPort::WriteBuffer *wb)
{
	if (wb == 0 || len != 1) return false;

	cal[0] = manReqWord|0x70;	// 	1. �������� �����

	wb->data = &cal;
	wb->len = sizeof(cal);

	return true;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

static bool RequestMan_80(u16 *data, u16 len, ComPort::WriteBuffer *wb)
{
	static u16 rsp[1];

	if (wb == 0 || len != 3) return false;

	switch (data[1])
	{
		case 1:

			numDevice = data[2];

			break;
	};

	rsp[0] = manReqWord|0x80;
 
	wb->data = rsp;
	wb->len = sizeof(rsp);

	return true;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

static bool RequestMan_90(u16 *data, u16 len, ComPort::WriteBuffer *wb)
{
	static u16 rsp[1];

	if (wb == 0 || len != 3) return false;

	//switch (data[1])
	//{
	//};

	rsp[0] = manReqWord|0x90;
 
	wb->data = rsp;
	wb->len = sizeof(rsp);

	return true;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

static bool RequestMan_E0(u16 *data, u16 len, ComPort::WriteBuffer *wb)
{
	static u16 rsp[1];

	if (wb == 0 || len < 129) return false;

	for (byte i = 0; i < 128; i++)
	{
		cal[i] = data[i];
	};

	saveCal = true;

	rsp[0] = manReqWord|0xE0;
 
	wb->data = rsp;
	wb->len = sizeof(rsp);

	return true;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

static bool RequestMan_F0(u16 *data, u16 len, ComPort::WriteBuffer *wb)
{
	static u16 rsp[1];

	if (wb == 0 || len != 1) return false;

//	SaveParams();

	rsp[0] = manReqWord|0xF0;
 
	wb->data = rsp;
	wb->len = sizeof(rsp);

	return true;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

static bool RequestMan(ComPort::WriteBuffer *wb, ComPort::ReadBuffer *rb)
{
	u16 *p = (u16*)rb->data;
	bool r = false;

	u16 t = p[0];

	if ((t & manReqMask) != manReqWord || rb->len < 2)
	{
//		bfERC++; 
		return false;
	};

	u16 len = (rb->len)>>1;

	t = (t>>4) & 0xF;

	switch (t)
	{
		case 0: 	r = RequestMan_00(p, len, wb); break;
		case 1: 	r = RequestMan_10(p, len, wb); break;
		case 2: 	r = RequestMan_20(p, len, wb); break;
		case 3: 	r = RequestMan_30(p, len, wb); break;
		case 7: 	r = RequestMan_70(p, len, wb); break;
		case 8: 	r = RequestMan_80(p, len, wb); break;
		case 9:		r = RequestMan_90(p, len, wb); break;
		case 0xE:	r = RequestMan_E0(p, len, wb); break;
		case 0xF:	r = RequestMan_F0(p, len, wb); break;
		
//		default:	bfURC++; 
	};

	return r;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

static void UpdateMan()
{
	static byte i = 0;
	static ComPort::WriteBuffer wb;
	static ComPort::ReadBuffer rb;
	static byte buf[1024];

	switch(i)
	{
		case 0:

			rb.data = buf;
			rb.maxLen = sizeof(buf);
			com.Read(&rb, (u32)-1, 10000);
			i++;

			break;

		case 1:

			if (!com.Update())
			{
				if (rb.recieved && rb.len > 0)
				{
					if (RequestMan(&wb, &rb))
					{
						com.Write(&wb);
						i++;
					}
					else
					{
						i = 0;
					};
				}
				else
				{
					i = 0;
				};
			};

			break;

		case 2:

			if (!com.Update())
			{
				if (rsp_30 != 0)
				{
					rsp_30->rw = 0;
					rsp_30 = 0;
				};

				i = 0;
			};

			break;
	};
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

static void UpdateLoadSave()
{
	static byte i = 0;
	static u16 adr = 0;
	static u16 count = 0;

	switch(i)
	{
		case 0:

			if (loadCal)
			{
				adr = 0;
				count = 4;

				i = 1;
			}
			else if (saveCal)
			{
				adr = 0;
				count = 4;

				u16 n = ArraySize(cal) - 1;

				cal[n] = GetCRC(cal, sizeof(cal) - 2);

				i = 3;
			};

			break;

		case 1:

			if (ReadEEPROM(cal, sizeof(cal), adr))
			{
				i++;
			};

			break;

		case 2:

			if (Check_EEPROM_Ready())
			{
				if (GetCRC(cal, sizeof(cal)) != 0)
				{
					adr += sizeof(cal);
					count -= 1;

					if (count > 0)
					{
						i = 1;	
					}
					else
					{
						for (u16 n = 0; n < ArraySize(cal); n++)
						{
							cal[n] = 0x55AA;
						};

						loadCal = false;
						saveCal = true;

						i = 0;
					};
				}
				else
				{
					loadCal = false;

					i = 0;
				};
			};

			break;

		case 3:

			if (WriteEEPROM(cal, sizeof(cal), adr))
			{
				i++;
			};

			break;

		case 4:

			if (Check_EEPROM_Ready())
			{
				adr += sizeof(cal);
				count -= 1;

				if (count > 0)
				{
					i = 3;	
				}
				else
				{
					saveCal = false;

					i = 0;
				};
			};

			break;
	};

}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

static void UpdateMisc()
{
	static byte i = 0;

	#define CALL(p) case (__LINE__-S): p; break;

	enum C { S = (__LINE__+3) };
	switch(i++)
	{
		CALL( UpdateHardware();	);
		CALL( UpdateMan();		);
		CALL( UpdateTemp()		);
		CALL( UpdateLoadSave()	);
	};

	i = (i > (__LINE__-S-3)) ? 0 : i;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

int main()
{
	static bool c = true;

	TM32 tm;
//	Dbt db(100);

//	__breakpoint(0);

	InitHardware();

	Init_TWI();

	com.Connect(0, 100000, 2);

//	OpenValve(1000, -1);

//	SetDutyPWMDir(100);

	static byte i = 0;

//	static i32 pwm = 100;

//	static i32 dest =20;

	tm.Reset();
	tm.Reset();

	//while (!tm.Check(5000))
	//{
	//	UpdateHardware();
	//};

	CalibrateShaftPos();

	while (1)
	{
		HW::GPIO->SET0 = 1<<12;

//		UpdateMan();


		UpdateMisc();

		HW::GPIO->CLR0 = 1<<12;


//		SetDutyPWMDir(sin(GetMilliseconds()*3.14/9000)*1200);

//		if (tm.Check(5000))
		{
//			SetDutyPWMDir(pwm = -pwm);
//			SetDestShaftPos(dest = -dest);
		};


//		switch (i)
//		{
//			case 0: 
//
//				if (IsMotorIdle())
//				{
//					if (c)
//					{
//						OpenValve();
//					}
//					else
//					{
//						CloseValve();
//					};
//
//					c = !c;
//
//					i++;
//				};
//
//				break;
//
//			case 1:
//
//				if (IsMotorIdle())
//				{
//					tm.Reset();
//					i++;
//				};
//
//				break;
//
//			case 2:
//
//				if (tm.Check((!c)?500:500))
//				{
////					SetDutyPWMDir(pwm = -pwm);
//					i = 0;
//				};
//
//				break;
//		}; // switch (i)

	}; // while (1)
}
