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

static u16 cal[129];

static Rsp30 *rsp_30 = 0;

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
	static u16 rsp[7];

	if (wb == 0 || len != 1) return false;

	rsp[0] = manReqWord|0x20;	//	1.Ответное слово (принятая команда)
	rsp[1] = GetAP();			//	2.Давление (у.е)
	rsp[2] = 100;				//	3.Температура манометра (у.е)
	rsp[3] = GetAP() / 2;		//	4.Давление (0.01 МПа)
	rsp[4] = 25;				//	5.Температура в приборе(гр)(short)
	rsp[5] = GetShaftPos();		//	6.6.Положение вала двигателя (short у.е)
	rsp[6] = GetCurrent();		//	7.7.Ток двигателя (мА)

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

	cal[0] = manReqWord|0x70;	// 	1. ответное слово

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

	for (byte i = 1; i < 129; i++)
	{
		cal[i] = data[i];
	};

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

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

int main()
{
	static bool c = true;

	TM32 tm;
	Dbt db(100);

//	__breakpoint(0);

	InitHardware();

	com.Connect(0, 100000, 2);

//	OpenValve(1000, -1);

//	SetDutyPWMDir(100);

	static byte i = 0;

//	static i32 pwm = 100;

//	static i32 dest =20;

	tm.Reset();
	tm.Reset();

	while (!tm.Check(1000))
	{
		UpdateHardware();
	};

	CalibrateShaftPos();

	while (1)
	{
//		HW::GPIO->NOT0 = 1<<12;

		UpdateHardware();

		UpdateMan();

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
//				if (tm.Check((!c)?1000:1000))
//				{
////					SetDutyPWMDir(pwm = -pwm);
//					i = 0;
//				};
//
//				break;
//		}; // switch (i)

	}; // while (1)
}
