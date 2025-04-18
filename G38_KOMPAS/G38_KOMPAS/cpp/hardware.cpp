#include <time.h>
#include <list.h>

#include "hardware.h"

#include "ComPort.h"

//#define OPEN_VALVE_CUR 600
//#define CLOSE_VALVE_CUR 600

#define LOCK_CLOSE_POSITION 0
#define INIT_CLOSE_POSITION 0
#define OPEN_POSITION		40
#define OPEN_RAND_MASK		15
#define CUR_CAL_MAXON		600
//#define CUR_CAL_FAULHABER	300

#define CUR_LIM_MAXON		1200
#define CUR_LIM_MAXON_ERR	1200
//#define CUR_LIM_FAULHABER	600
//#define MAXCNT 50				// ����������� �������� �����
//#define CLOSECURRENT 200		// ����������� ��� � closeShaftPos
//#define CLOSEDELTA 2			// 
#define CFK 256					// 
const u16 CSD = 4;					// 
const u16 DCL = CSD+3;				// ��������� � �������� ���������

//u16 curHV = 0;
//u16 reqHV = 800;
u16 curADC = 0;
u16 avrCurADC = 0;
u16 maxCurADC = 0;
u32 fcurADC = 0;
u16 vAP = 0;
u32 fvAP = 0;
u32 tachoCount = 0;
i32 shaftPos = 0;
u16 closeCurADC = 0;
u16 errCloseCount = 0;
u16 errOpenCount = 0;

//static u16 tachoTimeStamp = 0;
//static u16 tachoDT = 0;
//static const u16 dutyRPM[16] = { 100, 400, 400, 350, 200, 250, 200, 150, 100, 100, 100, 100, 100, 100, 100, 100 };
//static u16 speed = 0;

SHAFTPOS closeShaftPos;// = 0;
static i32 openShaftPos = 0;
static i32 deltaShaftPos = 0;
static i32 maxCloseShaftPos = 0;
//static i32 maxOpenShaftPos = 0;

static u32 cntHU = 0;
static u32 cntHV = 0;
static u32 cntHW = 0;

static u32 hallDisMask = 0;
//static u32 hallForced = 0;


MOTOSTATE motorState = IDLE;

//static u32 reqTacho = 0;
//static u32 reqTime = 0;
//static u16 limCur = 500;
//static u16 maxCur = 0;
//static u16 minCur = 0;
//static u16 holdCur = 0;
//static u32 startTacho = 0;
//u32 startTime = 0;
//u32 stopTime = 0;
//static u32 prevTacho = 0;
//static u32 brakeTime = 160;
//static Dbt lockTacho(1000);
//static Dbt lockCur(5);
//static Dbt maxCur(10);
//static Dbt stopTacho(160);

#define RSP30_ST 2

List<Rsp30>		freeRsp30;
List<Rsp30>		readyRsp30;

static Rsp30	*curRsp30 = 0;

static Rsp30	buf_rsp30[4] = {0};

//static byte wrInd_rsp30 = 0;
//static byte rdInd_rsp30 = 0;


struct LogData
{
	u16 cur;
	u16	ap;
	i32 shaftPos;
};

//static LogData log1[20];
//static LogData log2[20];

//static LogData *curLog = log1;
//static LogData *txLog = log2;

// static ComPort com;

static void InitRsp30();

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


#define UL 20 
#define VL 21
#define WL 22
#define UH 17
#define VH 18
#define WH 19
#define NN 0xFF

#define UU 0x37
#define VV 0x2F
#define WW 0x1F
#define NC 0x3F

#define UT 0x3E
#define VT 0x3D
#define WT 0x3B

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



byte t = 0;
byte s = 0;

bool dir = true;

// dir 0
// 1 UH WW
// 3 VH WW
// 2 VH UU
// 6 WH UU
// 4 WH VV
// 5 UH VV

// dir 1
// 1 WH UU
// 3 WH VV
// 2 UH VV
// 6 UH WW
// 4 VH WW
// 5 VH UU

// F/R 1


//                             1   2   3   4   5   6                1   2   3   4   5   6
const byte states[16] =		{ WW, WW, UU, WW, VV, VV, UU, UU,		WW, UU, VV, VV, WW, UU, WW, VV };
//const byte statesH[16] =	{ UT, UT, VT, VT, WT, UT, WT, VT,		VT, WT, UT, WT, VT, VT, UT, WT };
const byte LG_pin[16] =		{ UL, UL, VL, VL, WL, UL, WL, VL,		VL, WL, UL, WL, VL, VL, UL, WL };
const byte HG_pin[16] =		{ UH, UH, VH, VH, WH, UH, WH, VH,		VH, WH, UH, WH, VH, VH, UH, WH };
//byte states[16] =		{ NC, WW, UU, NC, VV, NC, NC, NC,		NC, NC, NC, NC, NC, NC, NC, NC }; 
//byte LG_pin[16] =		{ NN, UL, VL, NN, WL, NN, NN, NN,		NN, NN, NN, NN, NN, NN, NN, NN };
//byte HG_pin[16] =		{ NN, UH, VH, NN, WH, NN, NN, NN,		NN, NN, NN, NN, NN, NN, NN, NN };

const byte aaa[6] = { 1,3,2,6,4,5 }; 
//const byte qqq[16] = {5, 3, 6, 2, 5, 1, 4, 3, 14, 13, 11, 9, 14, 12, 10, 9};

i32 destShaftPos = 0;
//u16 maxCurrent = 600;

//static i32 fltDestShaftPos = 0;



static i32 curDutyOut = 0;
static i32 pidOut = 0;
//static i32 curPidOut = 0;
static u16 curLim = CUR_LIM_MAXON;
//static u16 curCal = CUR_CAL_MAXON;

//static i32 maxOut = 0;
//static i32 limOut = 0;

//const u16 _minDuty = 100;//400;
//const u16 _maxDuty = 350;//400;
const u16 maxDuty = 400;
//u16 duty = 0, curd = 0;

static i32 Kp = 1000000/*2000000*/, Ki = 2000/*4000*/, Kd = 500000;
static i32 iKp = 2000, iKi = 1000, iKd = 0;

static u32 startOpenTime = 0;
//static u32 startCloseTime = 0;
//static u32 openValveTime = 0;
//static u32 closeValveTime = 0;
static i8 tachoDir = -1;





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

static void EnableDriver() 
{ 
	HW::GPIO->SET((1<<14)|(1<<4)); 
	HW::MRT->Channel[3].CTRL = 1;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

static void DisableDriver() 
{ 
	HW::GPIO->BCLR(14);
	HW::MRT->Channel[3].CTRL = 0; 
	pidOut = 0; 
	//curDutyOut = 0;
}
	
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

static bool CheckDriverOn() 
{ 
	return HW::GPIO->B0[14] != 0; 
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

static bool CheckDriverOff() 
{ 
	return HW::GPIO->B0[14] == 0; 
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

inline void EnableCapSwitch() 
{ 
	HW::GPIO->BSET(4); 
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

inline void DisableCapSwitch() 
{ 
	HW::GPIO->BCLR(4); 
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

inline void LockShaftPos()
{
	pidOut = 0;
	destShaftPos = shaftPos-1;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

static i32 SetDutyCurrent(u16 cur)
{
	//static dword pt = GetMilliseconds();//, pt2 = GetMilliseconds();
	//dword t = GetMilliseconds();
	//dword dt = t - pt;

	static i32 e1 = 0, e2 = 0;

//	const i32 iKp = 1.0 * 65536, iKi = 0.02 * 65536, iKd = 10.0 * 65536;

	i32 e;

	if (cur > 0)
	{
//		pt = t;

		e = (i32)cur - (i32)curADC;

		curDutyOut += iKp * (e - e1) + iKi * e + iKd * (e - e1 * 2  + e2);

		i32 max = 65535*256;
		i32 min = 16384*256;

		if (curDutyOut < min) 
		{
			curDutyOut = min;
		}
		else if (curDutyOut > max)
		{
			curDutyOut = max;
		};

		e2 = e1; e1 = e;

		return curDutyOut/256;
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
	static i32 e1 = 0, e2 = 0;

	i32 e;// = fltDestShaftPos/32;

	e = destShaftPos - shaftPos;
	
	pidOut += Kp * (e - e1) + Ki * e + Kd * (e - e1 * 2  + e2);

	i32	maxOut = (i32)maxDuty * 65536;
	
	if (pidOut < -maxOut) 
	{
		pidOut = -maxOut;
	}
	else if (pidOut > maxOut)
	{
		pidOut = maxOut;
	};

	i32 po = pidOut/65536;
	
	po *= SetDutyCurrent(curLim);
	po /= 65536;

	SetDutyPWMDir(po);

	e2 = e1; e1 = e;
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

//void StartValve(bool dir, u32 tacho, u32 time, u16 lim)
//{
//	startTime = GetMilliseconds();
//	startTacho = tachoCount;
//	reqTime = time;
//	reqTacho = tacho;
//	motorState = 1;
//	lockCur.Check(false);
//	lockTacho.Check(false);
//	limCur = lim;
//	maxCur = 0;
//	minCur = 30000;
//
//	::dir = dir;
//
////	SetDutyPWM(3000);
//
////	StartMotor();
//}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

void OpenValve(bool forced)
{
	static u32 rand = 0;

	if (motorState == IDLE || motorState == CLOSE_OK || motorState == CLOSE_ERR || forced)
	{
		closeShaftPos.pos += (((shaftPos - closeShaftPos) - CSD) * CFK) >> 3; 

		openShaftPos = closeShaftPos + deltaShaftPos + (rand & OPEN_RAND_MASK);

		startOpenTime = GetMilliseconds();

		rand = (rand+1019+startOpenTime)*9871;

		EnableDriver();

		if (motorState == CLOSE_ERR)
		{
			SetDestShaftPos(shaftPos + 2000);
		}
		else
		{
			curLim = CUR_LIM_MAXON;
			SetDestShaftPos(openShaftPos);
		};

		if (curRsp30 != 0) readyRsp30.Add(curRsp30);

		curRsp30 = freeRsp30.Get();

		if (curRsp30 != 0)
		{
			curRsp30->rsp.rw = 0x0030;
			curRsp30->rsp.dir = 1;
			curRsp30->rsp.sl = 0;
			curRsp30->rsp.st = RSP30_ST;
		};

		motorState = OPENING;
	};
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

void CloseValve(bool forced)
{
	if (motorState == IDLE || motorState == OPEN_OK || motorState == OPEN_ERR || forced)
	{
		EnableDriver();

		if (motorState == OPEN_ERR)
		{
			SetDestShaftPos(shaftPos-2000);
		}
		else
		{
			curLim = CUR_LIM_MAXON;

			SetDestShaftPos(closeShaftPos-1);
		};

		//startCloseTime = GetMilliseconds();

		if (curRsp30 != 0) readyRsp30.Add(curRsp30);

		curRsp30 = freeRsp30.Get();

		if (curRsp30 != 0)
		{
			curRsp30->rsp.rw = 0x0030;
			curRsp30->rsp.dir = 0;
			curRsp30->rsp.sl = 0;
			curRsp30->rsp.st = RSP30_ST;
		};

		motorState = CLOSING;
	};
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

static void UpdateMotor()
{
	//u32 tacho, t;
	static TM32 tm, tm2;//, tm3;
	static i32 prevshaftPos = 0;
//	static u32 t = 500;
//	static i32 cnt = 0;

	switch (motorState)
	{
		case IDLE:		// Idle;

			DisableDriver();

			prevshaftPos = shaftPos;

			tm.Reset();

			break;

		case CLOSING: // ��������	//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

			if (tm.Check(100))
			{
				//closeShaftPos = shaftPos; //closeShaftPos.pos += 128;

				errCloseCount += 1;

				DisableDriver();

				curLim = CUR_LIM_MAXON_ERR;

				motorState = CLOSE_ERR;
			}
			else if ((shaftPos - closeShaftPos) <= CSD && tm.Timeout(20) || shaftPos <= closeShaftPos)
			{
				tm2.Reset();

				DisableDriver();

				//closeValveTime = GetMilliseconds() - startCloseTime;

				motorState = CLOSE_OK;
			};
			//else if (shaftPos <= ((openShaftPos - closeShaftPos) / 2))
			//{
			//	DisableDriver();
			//};

			if ((prevshaftPos - shaftPos) > 0/* || (curADC < 400)*/)
			{
				prevshaftPos = shaftPos;

				tm.Reset();
			};

			break;

		case CLOSE_OK: // ������	//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

			if (CheckDriverOff())
			{
				if (shaftPos > (closeShaftPos+DCL)) 
				{
				//	closeShaftPos++;
					SetDestShaftPos(closeShaftPos+DCL-2);
					EnableDriver();
				};

				tm.Reset();
			}
			else
			{
				if (shaftPos <= (closeShaftPos+DCL-1))
				{
					DisableDriver();

					tm.Reset();
				};
			};

			prevshaftPos = shaftPos;

			tm.Reset();

			break;

		case CLOSE_ERR: //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

			DisableDriver();

			prevshaftPos = shaftPos;

			tm.Reset();

			break;

		case OPENING: // ��������	//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

			if (tm.Check(100))
			{
				closeShaftPos.pos -= 16;

				errOpenCount += 1;

				DisableDriver();

				curLim = CUR_LIM_MAXON_ERR;

				motorState = OPEN_ERR;
			}
			else if ((openShaftPos - shaftPos) <= 5/* && tm.Timeout(50)*/)
			{
				//SetDestShaftPos(openShaftPos);

				tm2.Reset();
				t = 100;

				//DisableDriver();

				//openValveTime = GetMilliseconds() - startOpenTime;

				motorState = OPEN_OK;
			}
			else if ((shaftPos - prevshaftPos) > 2)
			{
				prevshaftPos = shaftPos;

				tm.Reset();
			};

			break;

		case OPEN_OK: // ������	//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

			if (CheckDriverOff())
			{
				if (shaftPos < (openShaftPos-10) || shaftPos > (openShaftPos+10)) 
				{
					SetDestShaftPos(openShaftPos);
					EnableDriver();
				};

				tm.Reset();
			}
			else
			{
				if (shaftPos >= (openShaftPos-5) && shaftPos <= (openShaftPos+5))
				{
					DisableDriver();

					tm.Reset();
				};
			};

			prevshaftPos = shaftPos;

			tm.Reset();

			break;

		case OPEN_ERR: //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

			DisableDriver();

			prevshaftPos = shaftPos;

			tm.Reset();

			break;

		case CAL_1:	//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

			curLim = CUR_CAL_MAXON;

			EnableDriver();

			maxCloseShaftPos = shaftPos;

			SetDestShaftPos(shaftPos-2000);

			tm.Reset();

			motorState = CAL_2;

			break;

		case CAL_2:	//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 
			if (tm.Check(50))
			{
				maxCloseShaftPos = shaftPos = 0;

//				DisableDriver();

				motorState = CAL_3;
			}
			else if (shaftPos < maxCloseShaftPos)
			{
				maxCloseShaftPos = shaftPos;
	
				tm.Reset();
			}
			else if ((prevshaftPos - shaftPos) > 2)
			{
				prevshaftPos = shaftPos;
				tm.Reset();
			}
			else if (tm.Timeout(50) && ((shaftPos - prevshaftPos) > 10))
			{
				tachoDir = -tachoDir; // ��������� FAULHABER 2250 024 BX4
			//	CSD *= 2;
			//	DCL *= 2;
			//	Kp /= 2;
			//	Ki /= 2;
			//	curCal = curLim = CUR_CAL_FAULHABER;
				tm.Reset();
			};

			break;

		case CAL_3:	//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

			if (tm.Check(10))
			{
				closeShaftPos = shaftPos + INIT_CLOSE_POSITION; // maxCloseShaftPos+15;

				openShaftPos = closeShaftPos + OPEN_POSITION;

				deltaShaftPos = openShaftPos - closeShaftPos;
							
				SetDestShaftPos(shaftPos);

				DisableDriver();

				curLim = CUR_LIM_MAXON;

				motorState = CAL_4;
			};

			break;

		case CAL_4:	//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

			if (tm.Check(100))
			{
				//if (cntHU < 10) { hallDisMask |= 1; };
				//if (cntHV < 10) { hallDisMask |= 2; };
				//if (cntHW < 10) { hallDisMask |= 4; };

				if (hallDisMask != 0) { closeShaftPos -= 100; };

				CloseValve(true);
			}
			else 
			{
				SetDestShaftPos(shaftPos);
			};

			break;

		case CAL_START:	//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

			tm.Reset();

			motorState = CAL_6;

			break;

		case CAL_6:	//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

			if (tm.Check(5000))
			{
				motorState = CAL_1;
			};

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
		CALL( curADC = ((ADC->DAT0&0xFFF0) * 1800 ) >> 16;  fcurADC += curADC - avrCurADC; avrCurADC = fcurADC >> 6; if (avrCurADC > maxCurADC) maxCurADC = avrCurADC;	);
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
	SCT->MATCHREL_L[1] = 480;
	SCT->MATCHREL_L[2] = 500; 
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

	SetDutyPWM(0);
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
		v = -v; dir = true;
	}
	else
	{
		dir = false;
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

	byte ist = HW::PIN_INT->IST & 7;

	HW::PIN_INT->IST = ist;

	t = ((HW::GPIO->PIN0 >> 8) & 7 | (dir<<3)) & 0xF;

//	if (hallDisMask == 0)
	{
		s = states[t];

		HW::GPIO->MASK0 = ~(0x3F << 17);
		HW::GPIO->MPIN0 = (u32)s << 17;

		HW::SWM->CTOUT_0 = LG_pin[t];
		HW::SWM->CTOUT_1 = HG_pin[t];
	};

	shaftPos += tachoEncoder[t & 7][ist] * tachoDir;

	HW::PIN_INT->IENF = (~t) & 7;

	cntHU += ist & 1;
	cntHV += (ist>>1) & 1;
	cntHW += ist>>2;

	//u16 tm = GetMillisecondsLow();
	//tachoDT = tm - tachoTimeStamp;
	//tachoTimeStamp = tm;

	//speed += 10;

	//if (speed > 50000)	{ speed = 50000; };

	//curADC = ((HW::ADC->DAT0&0xFFF0) * 1800) >> 16;

	//PID_Update();
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

static void TahoSync()
{
	static u16 pt = 0;
	static u16 pt2 = 0;
//	static u32 prtacho = 0;
//	static u16 dt = 100;

	static byte n = 0;

	if (GetMillisecondsLow() != pt)
	{
		pt = GetMillisecondsLow();

		//if (speed > 0) { speed -= 1; };

		//u16 x = tachoDT;
		//x >>= 1;

		//if ((pt - tachoTimeStamp) > 100 || x >= ArraySize(dutyRPM)) { x = ArraySize(dutyRPM)-1; };

		//maxDuty = dutyRPM[x];

		__disable_irq();

		HW::PIN_INT->IENF = (~(HW::GPIO->PIN0 >> 8)) & 7;
		
		__enable_irq();

		HW::ResetWDT();

		if (hallDisMask != 0 && CheckDriverOn())
		{
			u16 ms = GetMillisecondsLow();

			if (/*tachoCount != prtacho ||*/ (u16)(ms - pt2) >= 25)
			{
				//prtacho = tachoCount;

				//dt = ms - pt2;
				pt2 = ms;

				//if (dt > 100) dt = 100;

				byte xt = (aaa[n] | (dir<<3)) & 0xF;
				//byte xt = (qqq[t] | (dir<<3)) & 0xF;

				s = states[xt];

				HW::GPIO->MASK0 = ~(0x3F << 17);
				HW::GPIO->MPIN0 = (u32)s << 17;

				HW::SWM->CTOUT_0 = LG_pin[xt];
				HW::SWM->CTOUT_1 = HG_pin[xt];


				if (dir)
				{
					if (n > 0) { n -= 1; } else { n = ArraySize(aaa) -1; };

//					shaftPos += 1;

				}
				else
				{
					n += 1; if (n >= ArraySize(aaa)) { n = 0; };

//					shaftPos -= 1;
				}
			};
		};
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
	PIN_INT->ISEL = 1;
//	PIN_INT->SIENF = 7;
	PIN_INT->IENR = 7;
//	PIN_INT->FALL = 7;
//	PIN_INT->RISE = 7;
//	PIN_INT->IST = 7;
	PIN_INT->IENF = ((HW::GPIO->PIN0 >> 8)) & 7;

	VectorTableExt[PIN_INT0_IRQ] = TahoHandler;
	VectorTableExt[PIN_INT1_IRQ] = TahoHandler;
	VectorTableExt[PIN_INT2_IRQ] = TahoHandler;
	CM0::NVIC->ISER[0] = 7<<PIN_INT0_IRQ;

	GPIO->SET0 = (0x3F<<17);

	GPIO->MASK0 = ~(7 << 20);

	GPIO->MPIN0 = 0xFF;

}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

__irq void MRT_Handler()
{
	if (HW::MRT->IRQ_FLAG & 8)
	{
//		HW::GPIO->SET0 = 1<<12;

		curADC = ((HW::ADC->DAT0&0xFFF0) * 1800) >> 16;

//		fcurADC += (((HW::ADC->DAT0&0xFFF0) * 1800 ) >> 16) - curADC; curADC = fcurADC >> 3;

		PID_Update();

//		HW::GPIO->CLR0 = 1<<12;
	};

	HW::MRT->IRQ_FLAG = 8;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

void InitHardware()
{
	using namespace HW;

	Init_time(MCK);
	InitADC();
	InitPWM();
	InitTaho();
	InitRsp30();

//	StopMotor();

//	com.Connect(0, 921600, 0);


	VectorTableExt[MRT_IRQ] = MRT_Handler;
	CM0::NVIC->ICPR[0] = 1 << MRT_IRQ;
	CM0::NVIC->ISER[0] = 1 << MRT_IRQ;
	HW::MRT->Channel[3].CTRL = 0;
	HW::MRT->Channel[3].INTVAL = (MCK/10000)|(1UL<<31);


	SYSCON->SYSAHBCLKCTRL |= HW::CLK::WWDT_M;
	SYSCON->PDRUNCFG &= ~(1<<6); // WDTOSC_PD = 0
	SYSCON->WDTOSCCTRL = (1<<5)|59; // 600kHz/60 = 10kHz = 0.1ms

#ifndef _DEBUG

	WDT->TC = 250; // * 0.4ms
	WDT->MOD = 0x3;
	ResetWDT();

#endif
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

static void InitRsp30()
{
	for (u16 i = 0; i < ArraySize(buf_rsp30); i++)
	{
		Rsp30 *r = buf_rsp30+i;
		r->rsp.rw = 0;
		freeRsp30.Add(r);
	};
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Rsp30* GetRsp30()
{
	return readyRsp30.Get();
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

void FreeRsp30(Rsp30* r)
{
	freeRsp30.Add(r);
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

static void UpdateRsp30()
{
	static TM32 tm;

	if (curRsp30 != 0)
	{
		if (tm.Check(2))
		{
			curRsp30->rsp.data[curRsp30->rsp.sl++] = avrCurADC;

			if (curRsp30->rsp.sl >= ArraySize(curRsp30->rsp.data))
			{
				curRsp30->rsp.rw = 0x0030;
				curRsp30->rsp.st = RSP30_ST;

				readyRsp30.Add(curRsp30);

				curRsp30 = 0;
			};
		};
	};
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

/*static void UpdateLog()
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

}*/

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

//byte *twiData = (byte*)&twiReq;
//u16 twiCount = 0;
//u16 twiMaxCount = sizeof(req);
//u32 twiReqCount = 0;
//bool twiWrite = false;
//bool twiRead = false;
//
//byte twiWrBuf[4] = {0,0,0,0};

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

	//static byte *pr = (byte*)&req;
	//static byte *pw = twiWrBuf;
	//static u16 cr = 0;
	//static u16 cw = 0;

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


	//if(I2C1->INTSTAT & SLVDESELEN)
	//{
	//	I2C1->STAT = SLVDESELEN;
	//	twiCount = count;
	//	twiReqCount++;
	//	twiWrite = write;
	//	twiRead = read;

	//	if (read)
	//	{
	//		req.busy = true;
	//		req.ready = false;
	//	};
	//};
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

//static void UpdateTWI()
//{
//	static byte i = 0;
//
//	switch (i)
//	{
//		case 0:
//
//			if (twiRead)
//			{
//				HW::SCT->CTRL_L = 1<<2;
//				HW::SCT->OUTPUT = 0;
//
//				//HW::SCT->OUTPUT = 1;
//
//				i++;
//			};
//
//			break;
//
//		case 1:
//
//			HW::GPIO->MASK0 = ~(0xF<<17);
//			HW::GPIO->MPIN0 = req.chnl<<17;
//
//			i++;
//
//			break;
//
//		case 2:
//
//			HW::SCT->OUTPUT = 1;
//			HW::SCT->CTRL_L = 1<<1;
//
//			twiRead = false;
//			req.busy = false;
//			req.ready = true;
//
//			i = 0;
//
//			break;
//	};
//
//}

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

void UpdateHardware()
{
	static byte i = 0;

	static Deb db(false, 20);

	#define CALL(p) case (__LINE__-S): p; break;

	enum C { S = (__LINE__+3) };
	switch(i++)
	{
		CALL( TahoSync()	);
		CALL( UpdateMotor() );
		CALL( if (db.Check(HW::GPIO->B0[15] != 0)) OpenValve(); else CloseValve(); );
		CALL( UpdateRsp30()	);
	};

	i = (i > (__LINE__-S-3)) ? 0 : i;

	UpdateADC();
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
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

#include <system_imp.h>

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
