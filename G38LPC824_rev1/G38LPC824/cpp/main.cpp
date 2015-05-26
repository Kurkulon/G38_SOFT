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

	static byte i = 0;

	while (1)
	{
		UpdateHardware();

		//HW::SCT->MATCHREL_L[0] = 1250;
		//HW::GPIO->CLR0 = (1<<22)|(1<<14);
		//HW::GPIO->CLR0 = (1<<23);
   
		switch (i)
		{
			case 0: 

				if (c)
				{
					OpenValve(12, 200, 2000);
				}
				else
				{
					CloseValve(100, 1000, 1000);
				};

				c = !c;

				i++;

				break;

			case 1:

				if (IsMotorIdle())
				{
					i++;
				};

				break;

			case 2:

				if ((GetMilliseconds()- GetMotorStopTime()) >= 500)
				{
					i = 0;
				};

				break;
		}; // switch (i)

		if (tm.Check(1000))
		{
			com.TransmitByte(sec++);
		};

	}; // while (1)
}
