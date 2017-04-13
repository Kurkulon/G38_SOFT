#include "hardware.h"
#include "time.h"
#include "ComPort.h"
#include "crc16.h"
#include <math.h>

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


u32 fps = 0;

static byte sec = 0;

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

int main()
{
	static bool c = true;

	TM32 tm;
	Dbt db(100);

//	__breakpoint(0);

	InitHardware();

//	OpenValve(1000, -1);

	SetDutyPWMDir(100);

	static byte i = 0;

	static i32 pwm = 75;

	static i32 dest =20;

	CalibrateShaftPos();

	while (1)
	{
//		HW::GPIO->NOT0 = 1<<12;

		UpdateHardware();

//		SetDutyPWMDir(sin(GetMilliseconds()*3.14/9000)*1200);

//		if (tm.Check(2000))
//		{
//			SetDutyPWMDir(pwm = -pwm);
////			SetDestShaftPos(dest = -dest);
//		};

		switch (i)
		{
			case 0: 

				if (IsMotorIdle())
				{
					if (c)
					{
						OpenValve();
					}
					else
					{
						CloseValve();
					};

					c = !c;

					i++;
				};

				break;

			case 1:

				if (IsMotorIdle())
				{
					tm.Reset();
					i++;
				};

				break;

			case 2:

				if (tm.Check(1000))
				{
//					SetDutyPWMDir(pwm = -pwm);
					i = 0;
				};

				break;
		}; // switch (i)

	}; // while (1)
}
