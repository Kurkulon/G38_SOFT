#include "hardware.h"
#include "time.h"
#include "ComPort.h"
#include "crc16.h"

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

#define UL 20 
#define VL 21
#define WL 22
#define UH 17
#define VH 18
#define WH 19

u32 fps = 0;

static ComPort com;
static byte sec = 0;

// D	HHH	WVUWVU  
// R	WVU	LLLHHH  
// 1	000	111111  
// 1	001 P10P11  
// 1	010	10P11P  
// 1	011	P01P11  
// 1	100	0P11P1  
// 1	101	1P01P1  
// 1	110	01P11P  
// 1	111	111111  

// 0	000	111111  
// 0	001	01P11P  
// 0	010	1P01P1  
// 0	011	0P11P1  
// 0	100	P01P11  
// 0	101	10P11P  
// 0	110	P10P11  
// 0	111	111111  

byte states[16] = {0x3F, 0x1F, 0x37, 0x1F, 0x2F, 0x2F, 0x37, 0x3F, 0x3F, 0x37, 0x2F, 0x2F, 0x1F, 0x37,  0x1F, 0x3F };


byte t = 0;
byte s = 0;

bool dir = false;

byte LG_pin[16] = { 0xFF, UL, VL, VL, WL, UL, WL, 0XFF, 0xFF, WL, UL, WL, VL, VL, UL, 0xFF };
byte HG_pin[16] = { 0xFF, UH, VH, VH, WH, UH, WH, 0xFF, 0xFF, WH, UH, WH, VH, VH, UH, 0xFF };


i32 destShaftPos = 500;


//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

static void PID_Update()
{
	static dword pt = GetMilliseconds();//, pt2 = GetMilliseconds();
	dword t = GetMilliseconds();
	dword dt = t - pt;

	static float e1 = 0, e2 = 0;
	static float out = 0;

	const float Kp = 10, Ki = 0.00014, Kd = 0.01;

	float e;

	if (dt > 0)
	{
		pt = t;

		e = destShaftPos - GetShaftPos();
		
		float kp = Kp;
		float kdd = Kd * 1000 / dt;
		float kid = Ki * 1000 / dt;

		out += (kp * (e - e1) + kid * e + kdd * (e - e1 * 2  + e2));

		
		if (out < -100) 
		{
			out = -100;
		}
		else if (out > 100)
		{
			out = 100;
		};

		dir = (out > 0);

		SetDutyPWM(ABS(out)*12);

		e2 = e1; e1 = e;
	};
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

int main()
{
	static bool c = false;

	TM32 tm;
	Dbt db(100);

	InitHardware();

	com.Connect(0, 115200, 0);

//	OpenValve(1000, -1);

	HW::GPIO->SET0 = (1<<14);

	HW::GPIO->SET0 = (0x3F<<17);

	HW::GPIO->MASK0 = ~(7 << 20);

	HW::GPIO->MPIN0 = 0xFF;

	static byte i = 0;

	while (1)
	{
		UpdateHardware();

		t = ((HW::GPIO->PIN0 >> 8) | (dir<<3)) & 0xF;

		HW::SWM->CTOUT_0 = LG_pin[t];
		HW::SWM->CTOUT_1 = HG_pin[t];

		s = states[t];

		HW::GPIO->MPIN0 = (u32)s << 17;


		PID_Update();


		//if (HW::GPIO->PIN0 & 0x100)
		//{
		//	HW::GPIO->SET0 = (1<<20);
		//	HW::GPIO->CLR0 = (1<<17);
		//}
		//else
		//{
		//	HW::GPIO->SET0 = (1<<17);
		//	HW::GPIO->CLR0 = (1<<20);
		//};


		//HW::SCT->MATCHREL_L[0] = 1250;
		//HW::GPIO->CLR0 = (1<<22)|(1<<14);
		//HW::GPIO->CLR0 = (1<<23);
   
		//switch (i)
		//{
		//	case 0: 

		//		if (c)
		//		{
		//			OpenValve(12, 200, 2000);
		//		}
		//		else
		//		{
		//			CloseValve(100, 1000, 1000);
		//		};

		//		c = !c;

		//		i++;

		//		break;

		//	case 1:

		//		if (IsMotorIdle())
		//		{
		//			i++;
		//		};

		//		break;

		//	case 2:

		//		if ((GetMilliseconds()- GetMotorStopTime()) >= 500)
		//		{
		//			i = 0;
		//		};

		//		break;
		//}; // switch (i)

		HW::GPIO->NOT0 = 1<<12;

		if (tm.Check(2000))
		{
//			com.TransmitByte(sec++);

//			destShaftPos = -destShaftPos;

			//dir = !dir;
		};

	}; // while (1)
}
