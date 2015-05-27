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

inline u32 GetTachoCount() { extern u32 tachoCount; return tachoCount; }



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

inline void SetDutyPWM(u16 v)
{
	HW::SCT->MATCHREL_L[0] = (v < HW::SCT->MATCH_L[1]) ? v : HW::SCT->MATCH_L[1];
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

inline bool IsMotorIdle()
{
	extern byte motorState;
	return motorState == 0;
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

#ifndef WIN32


#else


#endif

#endif // HARDWARE_H__23_12_2013__11_37
