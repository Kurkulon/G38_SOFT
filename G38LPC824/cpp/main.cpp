#include "hardware.h"
#include "time.h"
#include "ComPort.h"
#include "crc16.h"
#include <math.h>

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


u32 fps = 0;

static ComPort com;
static byte sec = 0;




//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

int main()
{
	static bool c = true;

	TM32 tm;
	Dbt db(100);

	InitHardware();

	com.Connect(0, 115200, 0);

//	OpenValve(1000, -1);

	SetDutyPWMDir(230);

	static byte i = 0;

	static i32 pwm = 1200;

	static i32 dest = 1200;

//	SetDestShaftPos(dest);

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
					OpenValve(1000, 3000, 2000);
				}
				else
				{
					CloseValve(1000, 3000, 1000);
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
					i = 0;
				};

				break;
		}; // switch (i)

	}; // while (1)
}
