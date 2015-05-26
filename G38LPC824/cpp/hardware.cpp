#include "types.h"
#include "core.h"
#include "time.h"


u16 curHV = 0;
u16 reqHV = 800;
u16 curADC = 0;
u32 tachoCount = 0;

byte motorState = 0;
static u32 reqTacho = 0;
static u32 reqTime = 0;
static u16 limCur = 500;
//static u16 holdCur = 0;
static u32 startTacho = 0;
u32 startTime = 0;
u32 stopTime = 0;
static u32 prevTacho = 0;
//static u32 brakeTime = 160;
static Dbt lockTacho(1000);
static Dbt lockCur(5);
static Dbt maxCur(10);
static Dbt stopTacho(160);

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

static __irq void IntDummyHandler()
{
	__breakpoint(0);
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

static __irq void HardFaultHandler()
{
	__breakpoint(0);
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

static __irq void ExtDummyHandler()
{
	__breakpoint(0);
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

static void InitVectorTable()
{
	for (u32 i = 0; i < ArraySize(VectorTableInt); i++)
	{
		VectorTableInt[i] = IntDummyHandler;
	};

	for (u32 i = 0; i < ArraySize(VectorTableExt); i++)
	{
		VectorTableExt[i] = ExtDummyHandler;
	};

	VectorTableInt[3] = HardFaultHandler;

	CM0::SCB->VTOR = (u32)VectorTableInt;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

/*----------------------------------------------------------------------------
  Initialize the system
 *----------------------------------------------------------------------------*/
extern "C" void SystemInit()
{
	u32 i;
	using namespace CM0;
	using namespace HW;

	SYSCON->SYSAHBCLKCTRL |= CLK::SWM_M | CLK::IOCON_M | CLK::GPIO_M | HW::CLK::MRT_M | HW::CLK::UART0_M | HW::CLK::CRC_M;

	GPIO->DIRSET0 = (1<<27)|(1<<14)|(1<<17)|(1<<18)|(1<<19)|(1<<20)|(1<<21)|(1<<22);
	GPIO->CLR0 = (1<<27)|(1<<14)|(1<<20)|(1<<21)|(1<<22);
	GPIO->SET0 = (1<<17)|(1<<18)|(1<<19);

	IOCON->PIO0_1.B.MODE = 0;

	SWM->PINENABLE0.B.CLKIN = 0;

	for (i = 0; i < 200; i++) __nop();

	SYSCON->SYSPLLCLKSEL  = 3;					/* Select PLL Input         */
	SYSCON->SYSPLLCLKUEN  = 0;					/* Update Clock Source      */
	SYSCON->SYSPLLCLKUEN  = 1;					/* Update Clock Source      */
	while (!(SYSCON->SYSPLLCLKUEN & 1));		/* Wait Until Updated       */

	SYSCON->MAINCLKSEL    = 1;					/* Select PLL Clock Output  */
	SYSCON->MAINCLKUEN    = 0;					/* Update MCLK Clock Source */
	SYSCON->MAINCLKUEN    = 1;					/* Update MCLK Clock Source */
	while (!(SYSCON->MAINCLKUEN & 1));			/* Wait Until Updated       */

//	SYSCON->SYSAHBCLKDIV  = SYSAHBCLKDIV_Val;

	SYSCON->UARTCLKDIV = 1;
	SWM->U0_RXD = 26;
	SWM->U0_TXD = 16;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
inline void SetDutyPWM(u16 v)
{
	HW::SCT->MATCHREL_L[0] = v;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

inline void StartMotor()
{
	HW::GPIO->CLR0 = (1<<22)|(1<<14);
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

inline void StopMotor()
{
	HW::GPIO->SET0 = (1<<22)|(1<<14);
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

void StartValve(bool dir, u32 tacho, u32 time, u16 lim)
{
	startTime = GetMilliseconds();
	startTacho = tachoCount;
	reqTime = time;
	reqTacho = tacho;
	motorState = 1;
	lockCur.Check(false);
	lockTacho.Check(false);
	limCur = lim;

	if (dir) { HW::GPIO->SET0 = 1<<23; } else { HW::GPIO->CLR0 = 1<<23; };

	SetDutyPWM(3000);

	StartMotor();
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

static void UpdateMotor()
{
	u32 tacho, t;

	switch (motorState)
	{
		case 0:		// Idle;

			break;

		case 1:

			t = GetMilliseconds() - startTime;
			tacho = tachoCount - startTacho;
 
			if (t >= reqTime || tacho >= reqTacho || lockTacho.Check(((tacho - prevTacho) == 0) && (t >= 20)) || lockCur.Check((curADC > limCur) && (t >= 100)) || maxCur.Check((curADC > 1500)))
			{
				StopMotor();
				motorState = 2;
				stopTime = GetMilliseconds();
				stopTacho.Check(false);
				prevTacho = tachoCount;
			}
			else
			{
				prevTacho = tacho;

				if (t < 50)
				{ 
					SetDutyPWM(t * 25); 
				}
				else if ((t >= 60) && (t < 100))
				{
					SetDutyPWM(1250 - (t - 60) * 5); 
				};

			};

			break;

		case 2:

			if (stopTacho.Check((tachoCount - prevTacho) == 0))
			{
				motorState = 0;
			};

			prevTacho = tachoCount;

			break;
	};
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

static void UpdateADC()
{
	using namespace HW;

	curADC = ((ADC->DAT0&0xFFF0) * 4800 ) >> 16;
	ADC->SEQA_CTRL = 1|(1<<18)|(1UL<<31)|(1<<26);
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

static void InitPWM()
{
	using namespace HW;

	SYSCON->SYSAHBCLKCTRL |= CLK::SCT_M;

	SCT->STATE_L = 0;
	SCT->REGMODE_L = 0;

	SCT->MATCHREL_L[0] = 1251; 
	SCT->MATCHREL_L[1] = 1250;
	SCT->MATCH_L[2] = 0; 
	SCT->MATCH_L[3] = 0; 
	SCT->MATCH_L[4] = 0;

	SCT->OUT[0].SET = (1<<1);
	SCT->OUT[0].CLR = (1<<0);


	SCT->EVENT[0].STATE = 1;
	SCT->EVENT[0].CTRL = (1<<5)|(0<<6)|(1<<12)|0;

	SCT->EVENT[1].STATE = 1;
	SCT->EVENT[1].CTRL = (1<<5)|(0<<6)|(1<<12)|1;

	SCT->EVENT[2].STATE = 0;
	SCT->EVENT[2].CTRL = 0;

	SCT->EVENT[3].STATE = 0;
	SCT->EVENT[3].CTRL = 0;

	SCT->EVENT[4].STATE = 0;
	SCT->EVENT[4].CTRL = 0;

	SCT->EVENT[5].STATE = 0;
	SCT->EVENT[5].CTRL = 0;

	SCT->START_L = 0;
	SCT->STOP_L = 0;
	SCT->HALT_L = 0;
	SCT->LIMIT_L = (1<<1);

	SCT->CONFIG = 0; 

	SWM->CTOUT_0 = 21;

	SCT->CTRL_L = (1<<3);
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

static void InitADC()
{
	using namespace HW;

	//SWM->PINASSIGN[3] = (SWM->PINASSIGN[3] & 0x00FFFFFF) | 0x09000000;
	//SWM->PINASSIGN[4] = (SWM->PINASSIGN[4] & 0xFF000000) | 0x00100FFF;

	SWM->PINENABLE0.B.ADC_0 = 0;


	SYSCON->PDRUNCFG &= ~(1<<4);
	SYSCON->SYSAHBCLKCTRL |= CLK::ADC_M;

	ADC->CTRL = (1<<30)|49;

	while(ADC->CTRL & (1<<30));

	ADC->CTRL = 0;
	ADC->SEQA_CTRL = 1|(1<<18)|(1UL<<31)|(1<<26);
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

static __irq void TahoHandler()
{
	tachoCount++;

	HW::PIN_INT->IST = 1;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

static void InitTaho()
{
	using namespace HW;

	SYSCON->PINTSEL[0] = 18;
	PIN_INT->ISEL = 0;
	PIN_INT->SIENF = 1;
	PIN_INT->FALL = 1;
	PIN_INT->IST = 1;

	VectorTableExt[PININT0_IRQ] = TahoHandler;
	CM0::NVIC->ISER[0] = 1<<PININT0_IRQ;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

void InitHardware()
{
	InitVectorTable();
	Init_time();
	InitADC();
//	InitPWM();
//	InitTaho();
//	StopMotor();
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

void UpdateHardware()
{
	UpdateADC();
//	UpdateMotor();
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

u16 GetCRC(const void *data, u32 len)
{
	union { u32 *pd; u16 *pw; u8 *pb; };

	pb = (byte*)data;

//	byte * const p = (byte*)HW::CRC->B;

	HW::CRC->MODE = 0x15;
	HW::CRC->SEED = 0xFFFF;

	u32 dl = len>>2;
	u32 wl = (len&3)>>1;
	u32 bl = (len&1);

	for ( ; dl > 0; dl--) 
	{
		HW::CRC->D = *(pd++);
	};

	for ( ; wl > 0; wl--) 
	{
		HW::CRC->W = *(pw++);
	};

	for ( ; bl > 0; bl--) 
	{
		HW::CRC->B = *(pb++);
	};

	//for ( ; len > 0; len--) 
	//{
	//	HW::CRC->B = *(pb++);
	//};

	return HW::CRC->SUM;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
