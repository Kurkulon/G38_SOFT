#include "types.h"
#include "core.h"
#include "time.h"

#include "hardware.h"

#include "ComPort.h"

//#define OPEN_VALVE_CUR 600
//#define CLOSE_VALVE_CUR 600

//#define LOCK_CLOSE_POSITION 0
//#define INIT_CLOSE_POSITION 55
//#define OPEN_POSITION		60

#define PIN_EN		4
#define PIN_RTS		27
#define PIN_ISP		12
#define PIN_HIN1	17
#define PIN_UTX		16
#define PIN_URX		26
#define PIN_PULSE	15

#define EN		(1<<PIN_EN)
#define RTS		(1<<PIN_RTS)
#define ISP		(1<<PIN_ISP)
#define HIN1	(1<<PIN_HIN1)


//#define CUR_CAL_MAXON		600
//#define CUR_CAL_FAULHABER	300
//
//#define CUR_LIM_MAXON		600
//#define CUR_LIM_FAULHABER	600

//u16 curHV = 0;
//u16 reqHV = 800;
u16 curADC = 0; // 1 mA
u16 avrCurADC = 0;
i16 dCurADC = 0;
u32 fcurADC = 0;
u16 vAP = 0;
u32 fvAP = 0;
u16 curLow = 0; // 1 mA
u16 vFlow = 0;
u16 v80 = 0; // 0.1V
u16 dest_V80 = 700; // 0.1V
u16 minDestV80 = 450; // 0.1V
u16 maxDestV80 = 750; // 0.1V
u32 mmsec = 0; // 0.1 ms
u32 sumCur = 0;
u32 sumOpen = 0;
u16 startOpenVoltage = 0;
u16 cap = 0; // uF
bool startRsp30 = false;
u16 solenoidActiveTime = 0; // 0.1 ms
u16 minActiveTime = 60; // 0.1 ms
u16 delayRetention = 5;	// 0.1 ms
u16 delayMoveDetection = 75;	// 0.1 ms
u16 dCurMinMoveDetection = 200; // 1 mA

byte solenoidState = 0;


Rsp30 buf_rsp30[4] = {0};

static byte wrInd_rsp30 = 0;
static byte rdInd_rsp30 = 0;


//struct LogData
//{
//	u16 cur;
//	u16	ap;
//	i32 shaftPos;
//};

//static LogData log1[20];
//static LogData log2[20];

//static LogData *curLog = log1;
//static LogData *txLog = log2;

// static ComPort com;

static void InitRsp30();

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

//static i32 curDutyOut = 0;
//static i32 pidOut = 0;
//static i32 curPidOut = 0;
//static u16 curLim = CUR_LIM_MAXON;
//static u16 curCal = CUR_CAL_MAXON;

static i32 maxOut = 0;
static i32 limOut = 0;

//const u16 _minDuty = 100;//400;
//const u16 _maxDuty = 350;//400;
const u16 maxDuty = 4875;
//u16 duty = 0, curd = 0;

//static i32 Kp = 1000000/*2000000*/, Ki = 2000/*4000*/, Kd = 500000;
//static i32 iKp = 2000, iKi = 1000, iKd = 0;
//
static u32 startOpenTime = 0;
//static u32 startCloseTime = 0;
static u32 openValveTime = 0;
//static u32 closeValveTime = 0;
//static i8 tachoDir = 1;

//static void UpdateRsp30();




//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

u16 GetMinDestV80() 			{ return minDestV80; 			}
u16 GetMaxDestV80() 			{ return maxDestV80; 			}
u16 GetSolenoidActiveTime()		{ return solenoidActiveTime;	}
u16 GetMinActiveTime()			{ return minActiveTime;			}
u16 GetDelayRetention()			{ return delayRetention;		}
u16 GetDelayMoveDetection()		{ return delayMoveDetection;	}
u16 GetDifCurMinMoveDetection()	{ return dCurMinMoveDetection;	}

void SetMinDestV80(u16 v)				{ minDestV80 = v; 			}
void SetMaxDestV80(u16 v)				{ maxDestV80 = v; 			}
void SetSolenoidActiveTime(u16 v)		{ solenoidActiveTime = v;	}
void SetMinActiveTime(u16 v)			{ minActiveTime = v;		}
void SetDelayRetention(u16 v)			{ delayRetention = v;		}
void SetDelayMoveDetection(u16 v)		{ delayMoveDetection = v;	}
void SetDifCurMinMoveDetection(u16 v)	{ dCurMinMoveDetection = v;	}


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

	GPIO->DIRSET0 = RTS|ISP|EN|HIN1;
	GPIO->CLR0 = RTS|EN|HIN1;
	GPIO->SET0 = 0;

	IOCON->PIO0_1.B.MODE = 0;

	HW::GPIO->NOT0 = ISP;

	SWM->PINENABLE0.B.CLKIN = 0;

	for (i = 0; i < 200; i++) __nop();

	SYSCON->SYSPLLCLKSEL  = 3;					/* Select PLL Input         */
	SYSCON->SYSPLLCLKUEN  = 0;					/* Update Clock Source      */
	SYSCON->SYSPLLCLKUEN  = 1;					/* Update Clock Source      */
	while (!(SYSCON->SYSPLLCLKUEN & 1));		/* Wait Until Updated       */

	HW::GPIO->NOT0 = ISP;

	SYSCON->MAINCLKSEL    = 1;					/* Select PLL Clock Output  */
	SYSCON->MAINCLKUEN    = 0;					/* Update MCLK Clock Source */
	SYSCON->MAINCLKUEN    = 1;					/* Update MCLK Clock Source */
	while (!(SYSCON->MAINCLKUEN & 1));			/* Wait Until Updated       */

	HW::GPIO->NOT0 = ISP;

//	SYSCON->SYSAHBCLKDIV  = SYSAHBCLKDIV_Val;

	SYSCON->UARTCLKDIV = 1;
	SWM->U0_RXD = PIN_URX;
	SWM->U0_TXD = PIN_UTX;

	DMA->SRAMBASE = DmaTable;
	DMA->CTRL = 1;

	HW::GPIO->NOT0 = ISP;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

inline void EnableDriver()
{
	HW::SWM->CTOUT_0 = PIN_HIN1;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

inline void DisableDriver()
{
	HW::SWM->CTOUT_0 = ~0;
	HW::GPIO->CLR0 = HIN1;
	SetDutyPWM(0);
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

void OpenValve(bool forced)
{
	if (solenoidState < 1 || solenoidState > 3)
	{
		startOpenTime = GetMilliseconds();

		solenoidState = 1;
	};
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

void CloseValve(bool forced)
{
	if (solenoidState != 4 && solenoidState != 5)
	{
		DisableDriver();

		solenoidState = 4;
	};
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

static void UpdateSolenoid()
{
	//u32 tacho, t;
	static MTM32 tm, tm2;//, tm3;
//	static i32 prevshaftPos = 0;
//	static u32 t = 500;
//	static i32 cnt = 0;

	static bool moveDetected = false;
	static u32 moveStartTime = 0;
	static u32 moveDeadTime = 0;
	static i16 dCurMoveMax = 0;
	static i16 dCurMoveThr = 0;
	static u32 timeout = 0;

	switch (solenoidState)
	{
		case 0:		// Idle;

			//DisableDriver();

			//prevshaftPos = shaftPos;

			tm.Reset();

			break;

		case 1: // Старт открытия

			tm.Reset();

			SetDutyPWM(maxDuty);
			EnableDriver();

			sumCur = 0;
			startOpenVoltage = v80;
			moveDetected = false;
			moveStartTime = 0;
			moveDeadTime = (v80 > 400) ? (delayMoveDetection*400/v80) : delayMoveDetection;
			timeout = moveDeadTime * 2; if (timeout < minActiveTime) timeout = minActiveTime;
			dCurMoveMax = 0;
			dCurMoveThr = 0;

			startRsp30 = true;


			solenoidState++;

			break;

		case 2: // Открытие

			if (!moveDetected && dCurADC > dCurMoveMax)
			{
				dCurMoveMax = dCurADC;
				dCurMoveThr = dCurMoveMax / 4;
			};

			if (!moveDetected && tm.Timeout(moveDeadTime) && (dCurMoveMax >= dCurMinMoveDetection) && (dCurADC < dCurMoveThr)) 
			{
				moveDetected = true;
				moveStartTime = tm.GetTime();
			};

			bool c = tm.Timeout(timeout);

			if (c || (moveDetected && (tm.Timeout(moveStartTime+delayRetention) && dCurADC > (dCurMoveThr*2))))
			{
				SetDutyPWM(0);

				u32 sum = sumCur;
				u32 eV = v80;

				if (startOpenVoltage > eV)
				{
					cap = (sum * 3 / (startOpenVoltage - eV)) / 4; // q = I / 1000 / 10000
				};

				if (!c)
				{
					if (dest_V80 > minDestV80) dest_V80 -= 10;
				}
				else
				{
					if (dest_V80 <= maxDestV80) dest_V80 += 50;
				};

				solenoidState++;

				solenoidActiveTime = tm.GetTime();

				tm.Reset();
			};

			break;

		case 3: // Открыт

			if (tm.Check(10))
			{
				u16 t = (v80 < 750) ? (300 + (750 - v80)*1) : 300;

				SetDutyPWM(t);
			};

			break;

		case 4: // Старт закрытия

			tm.Reset();

			DisableDriver();
			
			solenoidState++;

			break;

		case 5: // Закрыт
 
			tm.Reset();

			break;
	};
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

static void UpdateV80()
{
	if (v80 >= (dest_V80+10))
	{
		HW::GPIO->SET0 = EN;
	}
	else if (v80 <= dest_V80)
	{
		HW::GPIO->CLR0 = EN;
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
		CALL( fvAP +=	(((ADC->DAT1&0xFFF0) * 3300) >> 16) - vAP; vAP = fvAP >> 3;	);
		CALL( vFlow =	((ADC->DAT2&0xFFF0) * 1800 ) >> 16;  						);
		CALL( curLow =	((ADC->DAT3&0xFFF0) * 2640 ) >> 16;  						);
		CALL( v80 =		((ADC->DAT10&0xFFF0) * 1033 ) >> 16;  	UpdateV80();		);
	};

	i = (i > (__LINE__-S-3)) ? 0 : i;
//	i &= 1;

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
	SCT->MATCHREL_L[1] = 4950;
	SCT->MATCHREL_L[2] = 5000; 
	SCT->MATCH_L[3] = 0; 
	SCT->MATCH_L[4] = 0;

	SCT->OUT[0].SET = (1<<2);
	SCT->OUT[0].CLR = (1<<0)|(1<<1);

	//SCT->OUT[1].SET = (1<<0)|(1<<1);
	//SCT->OUT[1].CLR = (1<<2);

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

	SWM->CTOUT_0 = 17;
	//SWM->CTOUT_1 = 17;

	SCT->CTRL_L = (1<<3);

	SetDutyPWM(0);
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

void SetDutyPWM(u16 v)
{
	HW::SCT->MATCHREL_L[0] = (v < maxDuty) ? v : maxDuty;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

static void InitADC()
{
	using namespace HW;

	//SWM->PINASSIGN[3] = (SWM->PINASSIGN[3] & 0x00FFFFFF) | 0x09000000;
	//SWM->PINASSIGN[4] = (SWM->PINASSIGN[4] & 0xFF000000) | 0x00100FFF;

	IOCON->PIO0_7.B.MODE = 0;
	IOCON->PIO0_6.B.MODE = 0;
	IOCON->PIO0_14.B.MODE = 0;
	IOCON->PIO0_23.B.MODE = 0;
	IOCON->PIO0_13.B.MODE = 0;

	SWM->PINENABLE0.B.ADC_0 = 0;
	SWM->PINENABLE0.B.ADC_1 = 0;
	SWM->PINENABLE0.B.ADC_2 = 0;
	SWM->PINENABLE0.B.ADC_3 = 0;
	SWM->PINENABLE0.B.ADC_10 = 0;

	SYSCON->PDRUNCFG &= ~(1<<4);
	SYSCON->SYSAHBCLKCTRL |= CLK::ADC_M;

	ADC->CTRL = (1<<30)|49;

	while(ADC->CTRL & (1<<30));

	ADC->CTRL = 11;
	ADC->SEQA_CTRL = 15|(1<<10)|(1UL<<31)|(1<<27);
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

#pragma push
#pragma O3
#pragma Otime

static void UpdateRsp30()
{
//	static MTM32 tm;
	static byte prMtrSt = 0;

	static byte prState = 0;

	static Rsp30 *rsp1 = 0;
	static Rsp30 *rsp2 = 0;
	static u16 n = 0;
	static u16 i = 0;

//	static ComPort::WriteBuffer wb;

	if (prState == 0 && startRsp30)
	{
		startRsp30 = false;

		rsp1 = &buf_rsp30[wrInd_rsp30];

		rsp2 = &buf_rsp30[(wrInd_rsp30 + 1) & 3];

		n = ArraySize(rsp1->data);
		i = 0;

		prState = (rsp1->rw == 0 && rsp2->rw == 0) ? solenoidState : 0;
	};

	if (prState)
	{
//		if (tm.Check(1))
		{
			rsp1->data[i] = curADC;//avrCurADC;
			rsp2->data[i++] = dCurADC;

			n -= 1;

			if (n == 0/* || prState != solenoidState*/)
			{
				rsp1->rw = 0x0030;
				rsp1->dir = 1;
				rsp1->st = 1;
				rsp1->sl = i;

				rsp2->rw = 0x0030;
				rsp2->dir = 0;
				rsp2->st = 1;
				rsp2->sl = i;

				wrInd_rsp30 = (wrInd_rsp30 + 2) & 3;

				prState = 0;
			};
		};
	};

	prMtrSt = solenoidState;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

__irq void MRT_Handler()
{
	if (HW::MRT->IRQ_FLAG & 8)
	{
		mmsec++;

		curADC = ((HW::ADC->DAT0&0xFFF0) * 9768 ) >> 16;  
		fcurADC += curADC - avrCurADC; 
		avrCurADC = fcurADC >> 2;
		dCurADC = curADC - avrCurADC;
		sumCur += curADC;
		
		UpdateRsp30();
	};

	HW::MRT->IRQ_FLAG = 8;
}

#pragma pop

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

void InitHardware()
{
	using namespace HW;

//	InitVectorTable();
	Init_time();
	InitADC();
	InitPWM();
//	InitTaho();
	InitRsp30();

//	StopMotor();

//	com.Connect(0, 921600, 0);


	VectorTableExt[MRT_IRQ] = MRT_Handler;
	CM0::NVIC->ICPR[0] = 1 << MRT_IRQ;
	CM0::NVIC->ISER[0] = 1 << MRT_IRQ;
	HW::MRT->Channel[3].CTRL = 1;
	HW::MRT->Channel[3].INTVAL = (MCK/10000)|(1UL<<31);


	SYSCON->SYSAHBCLKCTRL |= HW::CLK::WWDT_M;
	SYSCON->PDRUNCFG &= ~(1<<6); // WDTOSC_PD = 0
	SYSCON->WDTOSCCTRL = (1<<5)|59; // 600kHz/60 = 10kHz = 0.1ms

#ifndef _DEBUG

	WDT->TC = 250; // * 0.4ms
	WDT->MOD = 0x3;
	ResetWDT();

#else

	ResetWDT();
	WDT->MOD = 0;
	ResetWDT();

#endif
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

static void InitRsp30()
{
	for (u16 i = 0; i < ArraySize(buf_rsp30); i++)
	{
		buf_rsp30[i].rw = 0;
	};
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Rsp30* GetRsp30()
{
	Rsp30 *rsp = &buf_rsp30[rdInd_rsp30];

	if (rsp->rw == 0)
	{
		rsp = 0;
	}
	else
	{
		rdInd_rsp30 = (rdInd_rsp30 + 1) & 3;
	};

	return rsp;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

static byte *twi_wrPtr = 0;
static byte *twi_rdPtr = 0;
static u16 twi_wrCount = 0;
static u16 twi_rdCount = 0;
static byte *twi_wrPtr2 = 0;
static u16 twi_wrCount2 = 0;
static byte twi_adr = 0;
static DSCTWI* twi_dsc = 0;


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

__irq void Handler_TWI()
{
	using namespace HW;

	if(I2C1->INTSTAT & MSTPENDING)
	{
		u32 state = I2C1->STAT & MSTSTATE;

		if(state == MSTST_IDLE) // Address plus R/W received
		{
			if (twi_rdCount == 0 && twi_wrCount == 0)
			{
				I2C1->INTENCLR = MSTPENDING;
				I2C1->CFG = 0;
			};
		}
		else if(state == MSTST_RX) // Received data is available
		{
			*twi_rdPtr++ = I2C1->MSTDAT; // receive data

			twi_rdCount--;

			I2C1->MSTCTL = (twi_rdCount > 0) ? MSTCONTINUE : MSTSTOP; 
		}
		else if(state == MSTST_TX) // Data can be transmitted 
		{
			if (twi_wrCount > 0)
			{
				I2C1->MSTDAT = *twi_wrPtr++;
				I2C1->MSTCTL = MSTCONTINUE;
				twi_wrCount--;

				if(twi_wrCount == 0 && twi_wrCount2 != 0)
				{
					twi_wrPtr = twi_wrPtr2;
					twi_wrCount = twi_wrCount2;
					twi_wrCount2 = 0;
				};
			}
			else if (twi_rdCount > 0)
			{
				I2C1->MSTDAT = (twi_adr << 1) | 1;
				I2C1->MSTCTL = MSTSTART;
			}
			else
			{
				I2C1->MSTCTL = MSTSTOP;
			};
		}
		else
		{
			twi_rdCount = 0;
			twi_wrCount = 0;

			I2C1->MSTCTL = MSTSTOP;
		};
		
		I2C1->STAT = MSTPENDING;
	};
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

bool Init_TWI()
{
	using namespace HW;

	SYSCON->SYSAHBCLKCTRL |= CLK::I2C1_M;

	SWM->I2C1_SCL = 25;
	SWM->I2C1_SDA = 24;

	HW::IOCON->PIO0_25.B.OD = 1;
	HW::IOCON->PIO0_24.B.OD = 1;

	//SWM->PINENABLE0.B.I2C0_SCL = 0;
	//SWM->PINENABLE0.B.I2C0_SDA = 0;

	VectorTableExt[I2C1_IRQ] = Handler_TWI;
	CM0::NVIC->ICPR[0] = 1 << I2C1_IRQ;
	CM0::NVIC->ISER[0] = 1 << I2C1_IRQ;

//	I2C1->CLKDIV = 3;
//	I2C1->INTENSET = MSTPENDING;
//	I2C1->CFG = MSTEN;

	return true;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

bool Write_TWI(DSCTWI *d)
{
	using namespace HW;

	if (twi_dsc != 0 || d == 0) { return false; };
	if ((d->wdata == 0 || d->wlen == 0) && (d->rdata == 0 || d->rlen == 0)) { return false; }

//	smask = 1<<13;
	twi_dsc = d;

	VectorTableExt[I2C1_IRQ] = Handler_TWI;
	CM0::NVIC->ICPR[0] = 1 << I2C1_IRQ;
	CM0::NVIC->ISER[0] = 1 << I2C1_IRQ;

	twi_dsc->ready = false;

	twi_wrPtr = (byte*)twi_dsc->wdata;	
	twi_rdPtr = (byte*)twi_dsc->rdata;	
	twi_wrPtr2 = (byte*)twi_dsc->wdata2;	
	twi_wrCount = twi_dsc->wlen;
	twi_wrCount2 = twi_dsc->wlen2;
	twi_rdCount = twi_dsc->rlen;
	twi_adr = twi_dsc->adr;

	if (twi_wrPtr2 == 0) twi_wrCount2 = 0;

	__disable_irq();

	I2C1->CLKDIV = 3;
	I2C1->INTENSET = MSTPENDING;
	I2C1->CFG = MSTEN;

	I2C1->MSTDAT = (twi_dsc->adr << 1) | ((twi_wrCount == 0) ? 1 : 0);
	I2C1->MSTCTL = MSTSTART;

	__enable_irq();

	return true;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

bool Check_TWI_ready()
{
	if (twi_dsc == 0)
	{ 
		return true; 
	}
	else if ((HW::I2C1->CFG & MSTEN) == 0)
	{
		twi_dsc->ready = true;
		twi_dsc = 0;

		return true;
	}
	else
	{
		return false;
	};
}


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

bool CkeckPulsePin()
{
	static Deb db(false, 20);

	return db.Check(HW::GPIO->B0[PIN_PULSE] != 0);
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

void UpdateHardware()
{
	static byte i = 0;


	#define CALL(p) case (__LINE__-S): p; break;

	enum C { S = (__LINE__+3) };
	switch(i++)
	{
		CALL( UpdateSolenoid() );
		CALL( UpdateADC(); );
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

u16 GetCRC_DMA(const void *data, u32 len)
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
