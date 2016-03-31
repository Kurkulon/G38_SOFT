#include <stdio.h>
#include <conio.h>
#include <stdlib.h>


#include "hardware.h"
#include "time.h"
#include "comport.h"
//#include "req.h"

bool run = true;

static ComPort com;
static ComPort::ReadBuffer rb;


static byte comState = 0;

static byte sampleTime[3] = { 19, 19, 9};
static byte gain[3] = { 7, 7, 7 };


static LogData log1[20];
static LogData log2[20];

static LogData *rxLog = log1;
static LogData *curLog = log2;

static bool readyCurLog = false;

static FILE *logFile;


//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

void InitLogFile()
{
	char buf[1000];

    SYSTEMTIME t;
    GetLocalTime(&t);

    _snprintf(buf, sizeof(buf),	"G38_LOG__%04hu_%02hu_%02hu__%02hu_%02hu_%02hu_%03hu.csv", t.wYear, t.wMonth, t.wDay, t.wHour, t.wMinute, t.wSecond, t.wMilliseconds);

 	logFile = fopen(buf, "w");

    fprintf(logFile, "Time, Current (mA),Pressure,Shaft Pos\n");
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

void UpdateLog()
{
	static u32 tm = 0;
	static u32 prevt = 0;
	static TM32 t;
	static u16 prevCur = 0;
	static i32 prevSh = 0;
	static u16 cur_1 = 0;
	static i32 sh_1 = 0;
	static u16 ap_1 = 0;

	LogData *p = curLog;

	if (readyCurLog)
	{
		readyCurLog = false;

//		bool c = false;

		u16 cur = p->cur;
		i32 sh = p->shaftPos;

		for (u32 i = 1; i < ArraySize(log1); i++)
		{
			if (p[i].cur < (cur-2) || p[i].cur > (cur+2) || p[i].shaftPos < (sh-2) || p[i].shaftPos > (sh+2))
			{
				DrawWave(ArraySize(log1), curLog);
				break;
			};
		};

		for (u32 i = 0; i < ArraySize(log1); i++)
		{
			if (p->cur < (prevCur-2) || p->cur > (prevCur+2) || p->shaftPos < (prevSh-2) || p->shaftPos > (prevSh+2))
			{
				if ((tm - prevt) > 1)
				{
					fprintf(logFile, "%u,%hu,%hu,%i\n", tm-1, cur_1, ap_1, sh_1);
				};

				fprintf(logFile, "%u,%hu,%hu,%i\n", tm, p->cur, p->ap, p->shaftPos);

				prevt = tm; prevCur = p->cur; prevSh = p->shaftPos;
			};
			
			cur_1 = p->cur; sh_1 = p->shaftPos; ap_1 = p->ap;

			tm++;
			p++;
		};
	}
	else if (t.Check(1000))
	{
		fflush(logFile);
	};
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

void UpdateCom()
{
	LogData* t;

	switch(comState)
	{
		case 0:

			rb.data = rxLog;
			rb.maxLen = sizeof(log1);
			
			com.Read(&rb, 100, 1);

			comState++;

			break;

		case 1:

			if (!com.Update())
			{
				if (rb.recieved && rb.len == rb.maxLen)
				{
					t = rxLog;
					rxLog = curLog;
					curLog = t;

					readyCurLog = true;
				};

				comState = 0;
			};

			break;
	};
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

static void Init()
{
	//for (u32 i = 0; i < ArraySize(req02); i++)
	//{
	//	q02[i].wb.data = &req02[i];
	//	q02[i].rb.data = &rsp02[i];
	//	qfreq.Add(&q02[i]);
	//};
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

static void UpdateKeys(char k)
{
	if (k == '-' || k == '=')
	{
		//if (gain[0] > 0 && k == '-') gain[0] -= 1;
		//if (gain[0] < 7 && k == '=') gain[0] += 1;
		//gain[1] = gain[2] = gain[0];
		//qcom.Add(CreateReq03(sampleTime, gain));
	};

	if (k == '[' || k == ']')
	{
		//if (sampleTime[0] > 1 && k == '[') sampleTime[0] -= 1;
		//if (sampleTime[0] < 19 && k == ']') sampleTime[0] += 1;
		//sampleTime[1] = sampleTime[0];
		//qcom.Add(CreateReq03(sampleTime, gain));
	};
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

int main()
{
	InitHardware();

	Init();

	InitLogFile();

	com.Connect("COM2", 921600, 0);
	
	while(1)
	{
		static byte i = 0;

		#define CALL(p) case (__LINE__-S): p; break;

		enum C { S = (__LINE__+3) };
		switch(i++)
		{
			CALL( UpdateHardware();		);
			CALL( UpdateCom();			);
			CALL( UpdateLog();			);
		};

		i = (i > (__LINE__-S-3)) ? 0 : i;

		#undef CALL

		if (_kbhit())
		{
			UpdateKeys(_getch());
		};
	};
}
