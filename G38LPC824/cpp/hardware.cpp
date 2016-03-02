#include "types.h"
#include "core.h"
#include "time.h"

#include "hardware.h"

#include "ComPort.h"

u16 curHV = 0;
u16 reqHV = 800;
u16 curADC = 0;
u32 fcurADC = 0;
u16 vAP = 0;
u32 fvAP = 0;
u32 tachoCount = 0;
i32 shaftPos = 0;

byte motorState = 0;
static u32 reqTacho = 0;
static u32 reqTime = 0;
static u16 limCur = 500;
static u16 maxCur = 0;
static u16 minCur = 0;
//static u16 holdCur = 0;
static u32 startTacho = 0;
u32 startTime = 0;
u32 stopTime = 0;
static u32 prevTacho = 0;
//static u32 brakeTime = 160;
static Dbt lockTacho(1000);
static Dbt lockCur(5);
//static Dbt maxCur(10);
static Dbt stopTacho(160);

struct LogData
{
	u16 cur;
	u16	ap;
	i32 shaftPos;
};

static LogData log1[20];
static LogData log2[20];

static LogData *curLog = log1;
static LogData *txLog = log2;

static ComPort com;

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


#define UL 20 
#define VL 21
#define WL 22
#define UH 17
#define VH 18
#define WH 19

// D	HHH	WVUWVU  
// R	WVU	LLLHHH 

// 1	000	111111  
// 1	001 P10P11  1  
// 1	010	10P11P  5
// 1	011	P01P11  6
// 1	100	0P11P1  3
// 1	101	1P01P1  2
// 1	110	01P11P  4
// 1	111	111111  

// 0	000	111111  
// 0	001	01P11P  5
// 0	010	1P01P1  1
// 0	011	0P11P1  6
// 0	100	P01P11  3 
// 0	101	10P11P  4
// 0	110	P10P11  2
// 0	111	111111  

byte states[16] = {0x3F, 0x1F, 0x37, 0x1F, 0x2F, 0x2F, 0x37, 0x3F,		0x3F, 0x37, 0x2F, 0x2F, 0x1F, 0x37, 0x1F, 0x3F };

//byte states[16] = {0x3F, 0x1F, 0x37, 0x1F, 0x2F, 0x2F, 0x37, 0x3F,			0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x37, 0x3F, 0x3F };


byte t = 0;
byte s = 0;

bool dir = true;

byte LG_pin[16] =		{ 0xFF, UL, VL, VL, WL, UL, WL, 0XFF,		0xFF, WL, UL, WL, VL, VL, UL, 0xFF };
byte HG_pin[16] =		{ 0xFF, UH, VH, VH, WH, UH, WH, 0xFF,		0xFF, WH, UH, WH, VH, VH, UH, 0xFF };

//byte LG_pin[16] =		{ 0xFF, UL, VL, VL, WL, UL, WL, 0XFF,		0xFF, 0xFF, 0xFF, 0xFF, 0xFF, VL, 0xFF, 0xFF };
//byte HG_pin[16] =		{ 0xFF, UH, VH, VH, WH, UH, WH, 0xFF,		0xFF, 0xFF, 0xFF, 0xFF, 0xFF, VH, 0xFF, 0xFF };


i32 destShaftPos = 0;

static i32 pidOut = 0;

static i32 maxOut = 0;

const u16 maxDuty = 200;
u16 duty = 0, curd = 0;









//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

// HHH 
// WVU  UV W
//     00001111 EW
//     00110011 EV
//     01010101 EU
// 000 00000000
// 001 00+0-000         
// 010 0-00+000        
// 011 0+-00000          
// 100 0+-00000             
// 101 0-00+000             
// 110 00+0-000            
// 111 00000000


static i8 tachoEncoder[8][8] = {
	{0,0,0,0,0,0,0,0},
	{0,0,1,0,-1,0,0,0},
	{0,-1,0,0,1,0,0,0},
	{0,1,-1,0,0,0,0,0},
	{0,1,-1,0,0,0,0,0},
	{0,-1,0,0,1,0,0,0},
	{0,0,1,0,-1,0,0,0},
	{0,0,0,0,0,0,0,0}
};
	
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

	SYSCON->SYSAHBCLKCTRL |= CLK::SWM_M | CLK::IOCON_M | CLK::GPIO_M | HW::CLK::MRT_M | HW::CLK::UART0_M | HW::CLK::CRC_M | HW::CLK::DMA_M;

	GPIO->DIRSET0 = (1<<27)|(1<<14)|(1<<17)|(1<<18)|(1<<19)|(1<<20)|(1<<21)|(1<<22)|(1<<12);
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

	DMA->SRAMBASE = DmaTable;
	DMA->CTRL = 1;

}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

inline void LockShaftPos()
{
	pidOut = 0;
	destShaftPos = shaftPos-1;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

static u16 SetDutyCurrent(u16 cur)
{
	//static dword pt = GetMilliseconds();//, pt2 = GetMilliseconds();
	//dword t = GetMilliseconds();
	//dword dt = t - pt;

	static i32 e1 = 0, e2 = 0;
	static i32 duty = 0;

	const i32 Kp = 1.0 * 65536, Ki = 0.02 * 65536, Kd = 10.0 * 65536;

	i32 e;

	if (cur > 0)
	{
//		pt = t;

		e = (i32)cur - (i32)curADC;

		duty += Kp * (e - e1) + Ki * e + Kd * (e - e1 * 2  + e2);

		u32 t = maxDuty * 65536;

		if (duty < 0) 
		{
			duty = 0;
		}
		else if (duty > t)
		{
			duty = t;
		};

		e2 = e1; e1 = e;

		return duty / 65536;
	}
	else
	{
		e2 = e1 = e = 0;

		return 0;
	};

}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

static void PID_Update()
{
	//static dword pt = GetMilliseconds();//, pt2 = GetMilliseconds();
	//dword t = GetMilliseconds();
	//dword dt = t - pt;

	static i32 e1 = 0, e2 = 0;
	static i32 dst = 0;

	const i32 Kp = 16.0 * 65536, Ki = 0.05 * 65536, Kd = 10.0 * 65536;

	i32 e;


	if (HW::MRT->Channel[3].STAT & 1)
	{
		HW::MRT->Channel[3].STAT = 1;

		HW::GPIO->NOT0 = 1<<12;

//		pt = t;

//		if (destShaftPos > dst) { dst += 1; } else if (destShaftPos < dst) { dst -= 1; };


		e = destShaftPos - GetShaftPos();
		
		//float kp = Kp;
		//float kdd = Kd * 1000 / dt;
		//float kid = Ki * 1000 / dt;

		pidOut += (Kp * (e - e1) + Ki * e + Kd * (e - e1 * 2  + e2));

//		maxOut = (i32)maxDuty * 65536;
		maxOut = (i32)SetDutyCurrent(500) * 65536;

	
		if (pidOut < -maxOut) 
		{
			pidOut = -maxOut;
		}
		else if (pidOut > maxOut)
		{
			pidOut = maxOut;
		};

		SetDutyPWMDir(pidOut/65536);

		e2 = e1; e1 = e;
	};
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

//inline void StartMotor()
//{
//	HW::GPIO->CLR0 = (1<<22)|(1<<14);
//}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

//inline void StopMotor()
//{
//	HW::GPIO->SET0 = (1<<22)|(1<<14);
//}

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
	maxCur = 0;
	minCur = 30000;

	::dir = dir;

//	SetDutyPWM(3000);

//	StartMotor();
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

static void UpdateMotor()
{
	u32 tacho, t;

	switch (motorState)
	{
		case 0:		// Idle;

//			SetDutyPWM(SetDutyCurrent(0));
			PID_Update();
			break;

		case 1:

			t = GetMilliseconds() - startTime;
			tacho = tachoCount - startTacho;
 
			if (t >= reqTime 
				|| tacho >= reqTacho 
				|| lockTacho.Check(((tacho - prevTacho) == 0) && (t >= 20)) 
				/*|| lockCur.Check(((curADC > (limCur+minCur)) || (curADC > maxCur)) && (t >= 50))*/)
			{
				SetDutyPWM(0);
				motorState = 2;
				stopTime = GetMilliseconds();
				stopTacho.Check(false);
				prevTacho = tachoCount;
			}
			else
			{
				prevTacho = tacho;

				

				//duty = maxDuty;

				//if (t <= 10)
				//{ 
				//	duty = t * maxDuty / 10; 
				//}
				//else if ((t >= 60) && (t < 100))
				//{
				//	duty = maxDuty;// - (t - 60) * maxDuty / 80; 
				//};

				//if (curADC > limCur)
				//{
				//	t = (curADC - limCur) >> 4;

				//	duty -= (t < duty) ? t : duty;
				//};

				//if (t < 20)
				//{
				//	if (curADC > maxCur)
				//	{
				//		maxCur = curADC;
				//	};
				//}
				//else
				//{
				//	if (curADC < minCur)
				//	{
				//		minCur = curADC;
				//	};
				//};

				//if (curADC > 200)
				//{
				//	curd = (curADC-200) * 1;

				//	if (curd > duty) curd = duty;

				//	duty -= curd;
				//}

				SetDutyPWM(SetDutyCurrent(limCur));
			};

			break;

		case 2:

			LockShaftPos();
			motorState = 0;

			break;
	};
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

static void UpdateADC()
{
	using namespace HW;

	static byte i = 0;

	#define CALL(p) case (__LINE__-S): p; break;

	enum C { S = (__LINE__+3) };
	switch(i++)
	{
		CALL( fcurADC += (((ADC->DAT0&0xFFF0) * 1800 ) >> 16) - curADC;	curADC = fcurADC >> 3;	);
		CALL( fvAP += (((ADC->DAT1&0xFFF0) * 3300) >> 16) - vAP; vAP = fvAP >> 3;	);
	};

//	i = (i > (__LINE__-S-3)) ? 0 : i;
	i &= 1;

	#undef CALL
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

static void InitPWM()
{
	using namespace HW;

	SYSCON->SYSAHBCLKCTRL |= CLK::SCT_M;

	SCT->STATE_L = 0;
	SCT->REGMODE_L = 0;

	SCT->MATCHREL_L[0] = maxDuty; 
	SCT->MATCHREL_L[1] = 230;
	SCT->MATCHREL_L[2] = 250; 
	SCT->MATCH_L[3] = 0; 
	SCT->MATCH_L[4] = 0;

	SCT->OUT[0].SET = (1<<2);
	SCT->OUT[0].CLR = (1<<1);

	SCT->OUT[1].SET = (1<<0)|(1<<1);
	SCT->OUT[1].CLR = (1<<2);

	SCT->EVENT[0].STATE = 1;
	SCT->EVENT[0].CTRL = (1<<5)|(0<<6)|(1<<12)|0;

	SCT->EVENT[1].STATE = 1;
	SCT->EVENT[1].CTRL = (1<<5)|(0<<6)|(1<<12)|1;

	SCT->EVENT[2].STATE = 1;
	SCT->EVENT[2].CTRL = (1<<5)|(0<<6)|(1<<12)|2;

	SCT->EVENT[3].STATE = 0;
	SCT->EVENT[3].CTRL = 0;

	SCT->EVENT[4].STATE = 0;
	SCT->EVENT[4].CTRL = 0;

	SCT->EVENT[5].STATE = 0;
	SCT->EVENT[5].CTRL = 0;

	SCT->START_L = 0;
	SCT->STOP_L = 0;
	SCT->HALT_L = 0;
	SCT->LIMIT_L = (1<<2);

	SCT->CONFIG = 0; 

	//SWM->CTOUT_0 = 20;
	//SWM->CTOUT_1 = 17;

	SCT->CTRL_L = (1<<3);
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

void SetDutyPWM(u16 v)
{
	HW::SCT->MATCHREL_L[0] = (v < maxDuty) ? v : maxDuty;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

void SetDutyPWMDir(i32 v)
{
	if (v < 0)
	{
		v = -v; dir = false;
	}
	else
	{
		dir = true;
	};

	HW::SCT->MATCHREL_L[0] = (v < maxDuty) ? v : maxDuty;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

static void InitADC()
{
	using namespace HW;

	//SWM->PINASSIGN[3] = (SWM->PINASSIGN[3] & 0x00FFFFFF) | 0x09000000;
	//SWM->PINASSIGN[4] = (SWM->PINASSIGN[4] & 0xFF000000) | 0x00100FFF;

	SWM->PINENABLE0.B.ADC_0 = 0;
	SWM->PINENABLE0.B.ADC_1 = 0;


	SYSCON->PDRUNCFG &= ~(1<<4);
	SYSCON->SYSAHBCLKCTRL |= CLK::ADC_M;

	ADC->CTRL = (1<<30)|49;

	while(ADC->CTRL & (1<<30));

	ADC->CTRL = 24;
	ADC->SEQA_CTRL = 3|(1UL<<31)|(1<<27);
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

static __irq void TahoHandler()
{
	tachoCount++;

	t = ((HW::GPIO->PIN0 >> 8) & 7 | (dir<<3)) & 0xF;

	s = states[t];

	HW::GPIO->MASK0 = ~(0x3F << 17);
	HW::GPIO->MPIN0 = (u32)s << 17;

	HW::SWM->CTOUT_0 = LG_pin[t];
	HW::SWM->CTOUT_1 = HG_pin[t];

	shaftPos += tachoEncoder[(HW::GPIO->PIN0 >> 8) & 7][HW::PIN_INT->IST&7];

	HW::PIN_INT->IST = 7;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

static void TahoSync()
{
	static u16 pt = 0;

	if (GetMillisecondsLow() != pt)
	{
		pt = GetMillisecondsLow();

		__disable_irq();
		
		byte t = ((HW::GPIO->PIN0 >> 8) & 7 | (dir<<3)) & 0xF;

		byte s = states[t];

		HW::GPIO->MASK0 = ~(0x3F << 17);
		HW::GPIO->MPIN0 = (u32)s << 17;

		HW::SWM->CTOUT_0 = LG_pin[t];
		HW::SWM->CTOUT_1 = HG_pin[t];

		__enable_irq();
	};
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

static void InitTaho()
{
	using namespace HW;


	IOCON->PIO0_8.B.HYS = 1;
	IOCON->PIO0_9.B.HYS = 1;

	SYSCON->PINTSEL[0] = 8;
	SYSCON->PINTSEL[1] = 9;
	SYSCON->PINTSEL[2] = 10;
	PIN_INT->ISEL = 0;
	PIN_INT->SIENF = 7;
	PIN_INT->SIENR = 7;
	PIN_INT->FALL = 7;
	PIN_INT->RISE = 7;
	PIN_INT->IST = 7;

	VectorTableExt[PININT0_IRQ] = TahoHandler;
	VectorTableExt[PININT1_IRQ] = TahoHandler;
	VectorTableExt[PININT2_IRQ] = TahoHandler;
	CM0::NVIC->ISER[0] = 7<<PININT0_IRQ;

	GPIO->SET0 = (1<<14);

	GPIO->SET0 = (0x3F<<17);

	GPIO->MASK0 = ~(7 << 20);

	GPIO->MPIN0 = 0xFF;

}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

void InitHardware()
{
	InitVectorTable();
	Init_time();
	InitADC();
	InitPWM();
	InitTaho();
//	StopMotor();

	com.Connect(0, 921600, 0);

	HW::MRT->Channel[3].CTRL = 0;
	HW::MRT->Channel[3].INTVAL = (MCK/10000)|(1UL<<31);
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

static void UpdateLog()
{
	static u16 pt = 0;

	static LogData *plog = curLog;
	static u16 n = ArraySize(log1);

	static ComPort::WriteBuffer wb;

	if (GetMillisecondsLow() != pt)
	{
		pt = GetMillisecondsLow();

		plog->ap = vAP;
		plog->cur = curADC;
		plog->shaftPos = shaftPos;
		
		plog++;

		n -= 1;

		if (n == 0)
		{
			plog = txLog;
			txLog = curLog;
			curLog = plog;
			n = ArraySize(log1);

			wb.data = txLog;
			wb.len = sizeof(log1);

			com.Write(&wb);
		};

		com.Update();
	};

}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

void UpdateHardware()
{
	static byte i = 0;

	static TM32 tm;


	#define CALL(p) case (__LINE__-S): p; break;

	enum C { S = (__LINE__+3) };
	switch(i++)
	{
		CALL( UpdateADC() );
		CALL( TahoSync() );
		CALL( UpdateMotor(); );
		CALL( UpdateLog() );
	};

	i = (i > (__LINE__-S-3)) ? 0 : i;
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
