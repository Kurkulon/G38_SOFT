#ifndef HARDWARE_H__23_12_2013__11_37
#define HARDWARE_H__23_12_2013__11_37

#include "hw_conf.h"

struct SHAFTPOS
{
	i32 pos;

	SHAFTPOS() : pos(0) {}
//	SHAFTPOS(i32 v) : pos(v) {}

	operator i32() { return pos >> 8; }
	i32 operator=(i32 v) { pos = v<<8; return v; }
	i32 operator+=(i32 v) { return (pos += v<<8)>>8; }
	i32 operator-=(i32 v) { return (pos -= v<<8)>>8; }

};

struct Rsp30 { Rsp30 *next; struct Rsp { u16 rw; u16 dir; u16 st; u16 sl; u16 data[200]; } rsp; };

extern void InitHardware();
extern void UpdateHardware();
extern u16 GetCRC(const void *data, u32 len);
//extern void StartValve(bool dir, u32 tacho, u32 time, u16 lim);

inline i32 GetShaftPos() { extern i32 shaftPos; return shaftPos; }

inline void SetDestShaftPos(i32 v) { extern i32 destShaftPos; destShaftPos = v; }

inline u32 GetTachoCount() { extern u32 tachoCount; return tachoCount; }

extern void SetDutyPWMDir(i32 v);

extern void SetDutyPWM(u16 v);

extern void OpenValve(bool forced = false);
extern void CloseValve(bool forced = false);
extern Rsp30* GetRsp30();
extern void FreeRsp30(Rsp30* r);

enum MOTOSTATE { IDLE = 0, CLOSING, CLOSE_OK, CLOSE_ERR, OPENING, OPEN_OK, OPEN_ERR, CAL_1, CAL_2, CAL_3, CAL_4, CAL_START, CAL_6 };

extern MOTOSTATE motorState;
extern SHAFTPOS closeShaftPos;

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


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

//bool Init_TWI();
//extern bool Write_TWI(DSCTWI *d);
//inline bool Read_TWI(DSCTWI *d) { return Write_TWI(d); }
//extern bool Check_TWI_ready();

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
	//extern MOTOSTATE motorState;
	return motorState == IDLE || motorState == OPEN_OK || motorState == OPEN_ERR || motorState == CLOSE_OK || motorState == CLOSE_ERR;
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
	//extern MOTOSTATE motorState;

	if (IsMotorIdle())
	{
		motorState = CAL_1;
		return true;
	};

	return false;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

inline bool InitShaftPos()
{
	//extern MOTOSTATE motorState;

	motorState = CAL_START;
	return true;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


#ifndef WIN32


#else


#endif

#endif // HARDWARE_H__23_12_2013__11_37
