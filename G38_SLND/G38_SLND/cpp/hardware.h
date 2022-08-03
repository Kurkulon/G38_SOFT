#ifndef HARDWARE_H__23_12_2013__11_37
#define HARDWARE_H__23_12_2013__11_37

#include "types.h"
#include "core.h"

#ifdef WIN32
#include <windows.h>
#endif

//struct SHAFTPOS
//{
//	i32 pos;
//
//	SHAFTPOS() : pos(0) {}
////	SHAFTPOS(i32 v) : pos(v) {}
//
//	operator i32() { return pos >> 8; }
//	i32 operator=(i32 v) { pos = v<<8; return v; }
//	i32 operator+=(i32 v) { return (pos += v<<8)>>8; }
//	i32 operator-=(i32 v) { return (pos -= v<<8)>>8; }
//
//};

struct Rsp30 { u16 rw; u16 dir; u16 st; u16 sl; u16 data[200]; };

extern void InitHardware();
extern void UpdateHardware();
extern u16 GetCRC(const void *data, u32 len);
extern u16 GetCRC_DMA(const void *data, u32 len);
extern bool CkeckPulsePin();

//extern void StartValve(bool dir, u32 tacho, u32 time, u16 lim);

//inline i32 GetShaftPos() { extern i32 shaftPos; return shaftPos; }

//inline void SetDestShaftPos(i32 v) { extern i32 destShaftPos; destShaftPos = v; }

//inline u32 GetTachoCount() { extern u32 tachoCount; return tachoCount; }

//extern void SetDutyPWMDir(i32 v);

extern void SetDutyPWM(u16 v);

extern void OpenValve(bool forced = false);
extern void CloseValve(bool forced = false);
extern Rsp30* GetRsp30();

extern byte solenoidState;
//extern SHAFTPOS closeShaftPos;

inline u32 GetMMSec() {	extern u32 mmsec; return mmsec; }

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

struct MTM32
{
	u32 pt;

	bool Check(u32 v) { u32 t = GetMMSec(); if ((t - pt) >= v) { pt = t; return true; } else { return false; }; }
	bool Timeout(u32 v) { return (GetMMSec() - pt) >= v; }
	void Reset() { pt = GetMMSec(); }
	u32 GetTime() { return (GetMMSec() - pt); }
};

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

#define MSTEN			(0x1)
#define SLVEN			(0x2)
#define MSTPENDING		(0x1)
#define MSTSTATE		(0xe)
#define MSTST_IDLE		(0x0)
#define MSTST_RX 		(0x2)
#define MSTST_TX 		(0x4)
#define MSTST_NACK_ADDR (0x6)
#define MSTST_NACK_TX	(0x8)
#define SLVPENDING		(0x100)
#define SLVSTATE		(0x600)
#define SLVST_ADDR		(0x000)
#define SLVST_RX 		(0x200)
#define SLVST_TX 		(0x400)
#define MSTCONTINUE		(0x1)
#define MSTSTART		(0x2)
#define MSTSTOP			(0x4)
#define SLVCONTINUE		(0x1)
#define SLVNACK			(0x2)
#define SLVDMA			8
#define SLVPENDINGEN	(0x100)
#define SLVDESELEN		(1<<15)

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

struct DSCTWI
{
	void*	wdata;
	void*	rdata;
	void*	wdata2;
	u16		wlen;
	u16		wlen2;
	u16		rlen;
	byte	adr;
	bool	ready;
};

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

bool Init_TWI();
extern bool Write_TWI(DSCTWI *d);
inline bool Read_TWI(DSCTWI *d) { return Write_TWI(d); }
extern bool Check_TWI_ready();

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

//struct TWI
//{
//	DSCTWI* dsc;
//
//	static byte *wrPtr;
//	static byte *rdPtr;
//	static u16 wrCount;
//	static u16 rdCount;
//	static byte adr;
//
//	TWI() : dsc(0) {}
//
//	bool Init(byte num);
//
//	bool Write(DSCTWI *d);
//	bool Read(DSCTWI *d) { return Write(d); }
//	bool Update();
//
//	static __irq void Handler_TWI();
//};

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

inline bool IsMotorIdle()
{
	extern byte solenoidState;
	return solenoidState == 0 || solenoidState == 2 || solenoidState == 4;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

inline u32 GetMotorStopTime()
{
	extern u32 stopTime;
	return stopTime;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

inline u16 GetAP()
{
	extern u16 vAP;
	return vAP;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

inline u16 GetCurrent()
{
	extern u16 curADC;
	return curADC;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

inline u16 GetAvrCurrent()
{
	extern u16 avrCurADC;
	return avrCurADC;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

inline u16 GetMaxCurrent()
{
	extern u16 maxCurADC;
	u16 t = maxCurADC; maxCurADC = 0;
	return t;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

inline u16 GetCap()
{
	extern u16 cap;
	return cap;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

inline u16 GetEnergy()
{
	extern u16 energy;
	return energy;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

inline u16 GetV80()
{
	extern u16 v80;
	return v80;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

extern u16 GetMinDestV80();
extern u16 GetMaxDestV80();
extern u16 GetSolenoidActiveTime();
extern u16 GetMinActiveTime();
extern u16 GetDelayRetention();
extern u16 GetDelayMoveDetection();
extern u16 GetDifCurMinMoveDetection();

extern void SetMinDestV80(u16);
extern void SetMaxDestV80(u16);
extern void SetMinActiveTime(u16);
extern void SetDelayRetention(u16);
extern void SetDelayMoveDetection(u16);
extern void SetDifCurMinMoveDetection(u16);

//inline void OpenValve(u32 tacho, u32 time, u16 lim)
//{
//	StartValve(true, tacho, time, lim);
//};

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

//inline void CloseValve(u32 tacho, u32 time, u16 lim)
//{
//	StartValve(false, tacho, time, lim);
//};

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

inline bool CalibrateShaftPos()
{
	extern byte solenoidState;

	if (IsMotorIdle())
	{
		solenoidState = 5;
		return true;
	};

	return false;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

inline bool InitShaftPos()
{
	extern byte solenoidState;

	solenoidState = 9;
	return true;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


#ifndef WIN32


#else


#endif

#endif // HARDWARE_H__23_12_2013__11_37
