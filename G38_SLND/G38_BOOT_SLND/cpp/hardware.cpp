#include "types.h"
#include "core.h"
#include "time.h"

#include "hardware.h"

#include "ComPort.h"


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

void InitHardware()
{
	using namespace HW;

//	InitVectorTable();
	Init_time();

	SYSCON->SYSAHBCLKCTRL |= HW::CLK::WWDT_M;
	SYSCON->PDRUNCFG &= ~(1<<6); // WDTOSC_PD = 0
	SYSCON->WDTOSCCTRL = (1<<5)|59; // 600kHz/60 = 10kHz = 0.1ms

#ifndef _DEBUG

	WDT->TC = 2500; // * 0.4ms
	WDT->MOD = 0x3;
	ResetWDT();

#endif
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

//void UpdateHardware()
//{
//	static byte i = 0;
//
//	static Deb db(false, 20);
//
//	#define CALL(p) case (__LINE__-S): p; break;
//
//	enum C { S = (__LINE__+3) };
//	switch(i++)
//	{
//		CALL(	);
//		CALL(	);
//		CALL(	);
//		CALL(	);
//		CALL( 	);
//	};
//
//	i = (i > (__LINE__-S-3)) ? 0 : i;
//}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

//u16 GetCRC(const void *data, u32 len)
//{
//	union { u32 *pd; u16 *pw; u8 *pb; };
//
//	pb = (byte*)data;
//
////	byte * const p = (byte*)HW::CRC->B;
//
//	HW::CRC->MODE = 0x15;
//	HW::CRC->SEED = 0xFFFF;
//
//	u32 dl = len>>2;
//	u32 wl = (len&3)>>1;
//	u32 bl = (len&1);
//
//	for ( ; dl > 0; dl--) 
//	{
//		HW::CRC->D = *(pd++);
//	};
//
//	for ( ; wl > 0; wl--) 
//	{
//		HW::CRC->W = *(pw++);
//	};
//
//	for ( ; bl > 0; bl--) 
//	{
//		HW::CRC->B = *(pb++);
//	};
//
//	//for ( ; len > 0; len--) 
//	//{
//	//	HW::CRC->B = *(pb++);
//	//};
//
//	return HW::CRC->SUM;
//}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
