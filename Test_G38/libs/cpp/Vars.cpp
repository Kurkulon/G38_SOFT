//#include "hardware.h"
#include "vars.h"
#include "CRC16.h"

#ifdef _VARSDEBUG_
//	static const bool _debug = true;
#else
//	static const bool _debug = false;
#endif

//=====================================================================================================================
//=====================================================================================================================
//=====================================================================================================================

//word Var::count = 0;

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

bool Var::WriteEvent(byte acs, word startAdr, word count, DataPointer data)
{
//	if (_debug) printf("Var::WriteEvent: 0x%X, %i - OK!\n", startAdr, count);
	return true;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

bool Var::ReadEvent(word startAdr, word count, DataPointer data) const
{
//	if (_debug) printf("Var::ReadEvent: 0x%X, %i - OK!\n", startAdr, count);
	return true;
}
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

//void Var::Save(PointerCRC &p) const
//{
//}
//
////+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//void Var::Load(PointerCRC &p)
//{
//}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

#ifdef _TEST_

bool Var::SelfTest()
{
	return true;
}
#endif

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

//=====================================================================================================================
//=====================================================================================================================
//=====================================================================================================================

Vars::Vars(Var** p, word count)
{

	maxCount = count;

	vars = p;

	curAddress = 0;
	curVar = 0;
	varsCount = 0;

	if (vars == 0) 
	{
		while(1) {};
	};
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
/*
Vars::~Vars()
{
	if (vars != 0) 
	{
		delete vars;
	};
}
*/
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

void Vars::AddVar(Var &var)
{
	if (varsCount < maxCount)
	{
		var.address = curAddress;
		vars[curVar++] = &var;
		varsCount = curVar;
		curAddress += var.Size();

	};
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


