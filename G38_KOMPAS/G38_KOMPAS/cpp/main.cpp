#include "hardware.h"
#include <time.h>
#include <ComPort.h>
#include <crc16.h>
#include <math.h>
#include <SEGGER_RTT.h>
#include <i2c.h>

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

//const char build_date[] __attribute__((used)) = "\n" __DATE__ "\n" __TIME__ "\n";


u32 fps = 0;

//static byte sec = 0;

static ComPort	com(USART0_USIC_NUM, ~0, PIN_UTX0, PIN_URX0, PIN_RTS0);
static S_I2C	i2c(I2C2_USIC_NUM, PIN_SCL, PIN_SDA, MCK);

static u16 manReqWord = 0x0000;
static u16 manReqMask = 0xFF00;

static u16 numDevice = 1;
static u16 verDevice = 0x103;

//static u32 manCounter = 0;

__packed struct S_cal
{
	u16 rw;
	float Tau1, Tau2;
	float P0, kP, T0, kT;
	float dP0, dkP, dT0, dkT;
	u16 crc;
};

static S_cal cal;
static bool loadCal = true;
static bool saveCal = false;

static u16 nvParams[32];

static bool loadParam = true;
static bool saveParam = false;

static Rsp30 *rsp_30 = 0;

static u16 temp = 0;

static void* eepromData = 0;
static u16 eepromWriteLen = 0;
static u16 eepromReadLen = 0;
static u16 eepromStartAdr = 0;

static float avrAP1 = 0;
static float avrAP2 = 0;
static u16 curAP = 0;
static u16 dAP = 0;

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

static void ResetParams()
{
	nvParams[0] = numDevice = 11111;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

static void InitParams()
{
	numDevice = nvParams[0];
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

static void SaveParams()
{
	nvParams[0] = numDevice;

	saveParam = true;
}

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

static void UpdateAP()
{
	static TM32 tm;

	if (tm.Check(50))
	{
		u16 p = GetAP();
		i16 t = temp;

		float ap = (cal.P0 + cal.kP*p + (cal.T0+cal.kT*t) * (cal.dP0+cal.dkP*p)) / (1-(cal.dT0+cal.dkT*t)*(cal.dP0+cal.dkP*p));

		avrAP1 += (ap - avrAP1) * (0.05/cal.Tau1);
		avrAP2 += (ap - avrAP2) * (0.05/cal.Tau2);

		curAP = avrAP1;
		dAP = avrAP1 - avrAP2;
		dAP += 32768;
	};
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

static void UpdateTemp()
{
	static byte i = 0;

	static DSCI2C dsc;
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


				if (i2c.AddRequest(&dsc))
				{
					i++;
				};
			};

			break;

		case 1:

			if (dsc.ready)
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

			if (i2c.AddRequest(&dsc))
			{
				tm.Reset();

				i++;
			};

			break;

		case 3:

			if (dsc.ready)
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

			if (i2c.AddRequest(&dsc))
			{
				tm.Reset();

				i++;
			};

			break;

		case 6:

			if (dsc.ready)
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

	rsp[0] = manReqWord|0x10;	// 	1. ответное слово

	wb->data = rsp;			 
	wb->len = sizeof(rsp);	 

	return true;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

static bool RequestMan_20(u16 *data, u16 len, ComPort::WriteBuffer *wb)
{
	static u16 rsp[9];

	if (wb == 0 || len != 1) return false;

	rsp[0] = manReqWord|0x20;	//	1.Ответное слово (принятая команда)
	rsp[1] = GetAP();			//	2.Давление (у.е)
	rsp[2] = temp;				//	3.Температура манометра (у.е)
	rsp[3] = curAP;				//	4.Давление (0.01 МПа)
	rsp[4] = temp;				//	5.Температура в приборе(гр)(short)
	rsp[5] = GetShaftPos();		//	6.Положение вала двигателя (short у.е)
	rsp[6] = closeShaftPos;		//	7.Положение вала при закрытии(у.е)(short)
	rsp[7] = GetMaxCurrent();	//	8.Ток двигателя (мА)
	rsp[8] = motorState;		//	9.Состояние двигателя

	wb->data = rsp;
	wb->len = sizeof(rsp);

	return true;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

static bool RequestMan_30(u16 *data, u16 len, ComPort::WriteBuffer *wb)
{
	static u16 rsp0[4];

	if (wb == 0 || len != 1) return false;

	if (rsp_30 != 0) FreeRsp30(rsp_30);

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
		wb->data = &rsp_30->rsp;
		wb->len = sizeof(rsp_30->rsp) - sizeof(rsp_30->rsp.data) + rsp_30->rsp.sl*2;
	};

	return true;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

static bool RequestMan_40(u16 *data, u16 len, ComPort::WriteBuffer *wb)
{
//	static u16 rsp[1];

	if (wb == 0 || len != 1) return false;

	CalibrateShaftPos();

	data[0] = manReqWord|0x40;	// 	1. ответное слово

	wb->data = data;			 
	wb->len = 2;	 

	return true;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

static bool RequestMan_70(u16 *data, u16 len, ComPort::WriteBuffer *wb)
{
	if (wb == 0 || len != 1) return false;

	cal.rw = manReqWord|0x70;	// 	1. ответное слово

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

	u16 *p = (u16*)&cal.rw;

	for (byte i = 0; i < (sizeof(cal)/2); i++)
	{
		*(p++) = data[i];
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

	SaveParams();

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
		case 4: 	r = RequestMan_40(p, len, wb); break;
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
			com.Read(&rb, ~0, US2COM(400));
			i++;

			break;

		case 1:

			if (!com.Update())
			{
				if (rb.recieved && rb.len > 0)
				{
					if (RequestMan(&wb, &rb))
					{
						com.Write(&wb, US2COM(20));
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
					rsp_30->rsp.rw = 0;

					FreeRsp30(rsp_30);

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
		case 0: //+++++++++++++++++++++++++++++++++++++++++++++++

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

				cal.crc = GetCRC(&cal, sizeof(cal) - 2);

				i = 3;
			}
			else if (loadParam)
			{
				adr = 8192;
				count = 4;

				i = 5;
			}
			else if (saveParam)
			{
				adr = 8192;
				count = 4;

				u16 n = ArraySize(nvParams) - 1;

				nvParams[n] = GetCRC(nvParams, sizeof(nvParams) - 2);

				i = 7;
			};

			break;

		case 1: //+++++++++++++++++++++++++++++++++++++++++++++++

			if (ReadEEPROM(&cal, sizeof(cal), adr))
			{
				i++;
			};

			break;

		case 2: //+++++++++++++++++++++++++++++++++++++++++++++++

			if (Check_EEPROM_Ready())
			{
				if (GetCRC(&cal, sizeof(cal)) != 0)
				{
					adr += sizeof(cal);
					count -= 1;

					if (count > 0)
					{
						i = 1;	
					}
					else
					{
						cal.Tau1 = 1;
						cal.Tau2 = 200;
						cal.P0 = 0;
						cal.kP = 1;
						cal.T0 = 0;
						cal.kT = 0;
						cal.dP0 = 0;
						cal.dkP = 0;
						cal.dT0 = 0;
						cal.dkT = 0;	
						
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

		case 3: //+++++++++++++++++++++++++++++++++++++++++++++++

			if (WriteEEPROM(&cal, sizeof(cal), adr))
			{
				i++;
			};

			break;

		case 4: //+++++++++++++++++++++++++++++++++++++++++++++++

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

		case 5: //+++++++++++++++++++++++++++++++++++++++++++++++

			if (ReadEEPROM(nvParams, sizeof(nvParams), adr))
			{
				i++;
			};

			break;

		case 6: //+++++++++++++++++++++++++++++++++++++++++++++++

			if (Check_EEPROM_Ready())
			{
				if (GetCRC(nvParams, sizeof(nvParams)) != 0)
				{
					adr += sizeof(nvParams);
					count -= 1;

					if (count > 0)
					{
						i = 5;	
					}
					else
					{
						ResetParams();

						loadParam = false;
						saveParam = true;

						i = 0;
					};
				}
				else
				{
					InitParams();

					loadParam = false;

					i = 0;
				};
			};

			break;

		case 7: //+++++++++++++++++++++++++++++++++++++++++++++++

			if (WriteEEPROM(nvParams, sizeof(nvParams), adr))
			{
				i++;
			};

			break;

		case 8: //+++++++++++++++++++++++++++++++++++++++++++++++

			if (Check_EEPROM_Ready())
			{
				adr += sizeof(nvParams);
				count -= 1;

				if (count > 0)
				{
					i = 7;	
				}
				else
				{
					saveParam = false;

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
		CALL( UpdateAP()		);
	};

	i = (i > (__LINE__-S-3)) ? 0 : i;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

int main()
{
	SEGGER_RTT_WriteString(0, RTT_CTRL_TEXT_WHITE "main() start ...\n");

//	__breakpoint(0);

	InitHardware();

	//Init_TWI();

	i2c.Connect(400000);

	com.Connect(ComPort::ASYNC, 100000, 2, 1);

	InitShaftPos();

	while (1)
	{
		HW::GPIO->SET0 = 1<<12;

		UpdateMisc();

		HW::GPIO->CLR0 = 1<<12;
	}; // while (1)
}
