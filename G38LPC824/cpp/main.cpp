#include "hardware.h"
#include "time.h"
#include "ComPort.h"
#include "crc16.h"

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


u32 fps = 0;

static ComPort com;
static byte sec = 0;




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

	SetDutyPWM(1200);

	static byte i = 0;

	while (1)
	{
		UpdateHardware();



//		PID_Update();


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


		if (tm.Check(1000))
		{
//			HW::GPIO->NOT0 = (1<<14);

			//t = ((HW::GPIO->PIN0 >> 8) & 7 | (dir<<3)) & 0xF;

			//HW::SWM->CTOUT_0 = LG_pin[t];
			//HW::SWM->CTOUT_1 = HG_pin[t];

			//s = states[t];

			//HW::GPIO->MPIN0 = (u32)s << 17;

//			com.TransmitByte(sec++);

//			destShaftPos = -destShaftPos;

		//	destShaftPos += 1;

			//dir = !dir;
		};

	}; // while (1)
}
