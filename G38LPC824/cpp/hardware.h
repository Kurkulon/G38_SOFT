#ifndef HARDWARE_H__23_12_2013__11_37
#define HARDWARE_H__23_12_2013__11_37

#include "types.h"
#include "core.h"

#ifdef WIN32
#include <windows.h>
#endif

extern void InitHardware();
extern void UpdateHardware();
extern u16 GetCRC(const void *data, u32 len);
extern void StartValve(bool dir, u32 tacho, u32 time, u16 lim);

inline i32 GetShaftPos() { extern i32 shaftPos; return shaftPos; }

inline void SetDestShaftPos(i32 v) { extern i32 destShaftPos; destShaftPos = v; }

inline u32 GetTachoCount() { extern u32 tachoCount; return tachoCount; }

extern void SetDutyPWMDir(i32 v);

extern void SetDutyPWM(u16 v);

extern void OpenValve();
extern void CloseValve();

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

inline bool IsMotorIdle()
{
	extern byte motorState;
	return motorState == 0 || motorState == 2 || motorState == 4;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

inline u32 GetMotorStopTime()
{
	extern u32 stopTime;
	return stopTime;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

inline void OpenValve(u32 tacho, u32 time, u16 lim)
{
	StartValve(true, tacho, time, lim);
};

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

inline void CloseValve(u32 tacho, u32 time, u16 lim)
{
	StartValve(false, tacho, time, lim);
};

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

inline bool CalibrateShaftPos()
{
	extern byte motorState;

	if (IsMotorIdle())
	{
		motorState = 5;
		return true;
	};

	return false;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


#ifndef WIN32


#else


#endif

#endif // HARDWARE_H__23_12_2013__11_37
