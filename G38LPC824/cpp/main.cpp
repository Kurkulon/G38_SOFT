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

	InitHardware();


//	OpenValve(1000, -1);

	SetDutyPWMDir(230);

	static byte i = 0;

	static i32 pwm = 1200;

	static i32 dest = 12;

	SetDestShaftPos(0);

	while (1)
	{
		HW::GPIO->NOT0 = 1<<12;

		UpdateHardware();

//		SetDutyPWMDir(sin(GetMilliseconds()*3.14/9000)*1200);

		switch (i)
		{
			case 0: 

				if (c)
				{
					OpenValve(100, 1000, 1000);
				}
				else
				{
					CloseValve(200, 1000, 500);
				};

				c = !c;

				i++;

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
//					SetDestShaftPos(dest = -dest);
					i = 0;
				};

				break;
		}; // switch (i)

	}; // while (1)
}
