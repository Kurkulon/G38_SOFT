#ifndef REQ_H__09_10_2014__10_31
#define REQ_H__09_10_2014__10_31

#include "ComPort.h"


struct Request
{
	byte adr;
	byte func;
	
	union
	{
		struct  { byte n; word crc; } f1;  // старт оцифровки
		struct  { byte n; byte chnl; word crc; } f2;  // чтение вектора
		struct  { byte dt[3]; byte ka[3]; word crc; } f3;  // установка периода дискретизации вектора и коэффициента усиления
	};
};

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

__packed struct Req01	// старт оцифровки
{
	byte func;
	byte n; 
	word crc;  
};

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

__packed struct Rsp01	// старт оцифровки
{
	byte func;
	word crc;  
};

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

__packed struct Req02	// чтение вектора
{
	byte func;
	byte adr;
	byte n; 
	byte chnl; 
	word crc; 
};  

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

__packed struct Rsp02	// чтение вектора
{
	byte adr;
	byte func;
	byte n; 
	byte chnl; 
	u16 data1[500]; 
	u16 data2[500]; 
	word crc; 
};  

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

__packed struct  Req03	// установка периода дискретизации вектора и коэффициента усиления
{ 
	byte func;
	byte dt[3]; 
	byte ka[3]; 
	word crc; 
};  

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

__packed struct  Rsp03	// установка периода дискретизации вектора и коэффициента усиления
{ 
	byte func;
	word crc; 
};  

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

struct Response
{
	byte adr;
	byte func;
	
	union
	{
		struct  { word crc; } f1;  // старт оцифровки
		struct  { byte n; byte chnl; u16 data[500]; word crc; } f2;  // чтение вектора
		struct  { word crc; } f3;  // установка периода дискретизации вектора и коэффициента усиления
	};
};

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

struct REQ
{
	bool	crcOK;
	typedef void tRsp(REQ*);
	
	REQ *next;

	tRsp*	CallBack;

	ComPort::WriteBuffer wb;
	ComPort::ReadBuffer rb;

	u32		preTimeOut, postTimeOut;

	REQ() : next(0) {}
};

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

class RequestQueue
{
	REQ* _first;
	REQ* _last;
	REQ* _req;
	
	byte _state;


	ComPort *com;

	bool _run;

public:

	RequestQueue(ComPort *p) : _first(0), _last(0), _run(true), _state(0), com(p) {}
	void Add(REQ* req);
	REQ* Get();
	bool Empty() { return _first == 0; }
	bool Idle() { return (_first == 0) && (_req == 0); }
	bool Stoped() { return _req == 0; }
	void Update();
	void Stop() { _run = false; }
	void Start() { _run = true; }
};

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

struct R02
{
	ComPort::WriteBuffer	wb;
	ComPort::ReadBuffer		rb;
	REQ		q;
	Req02	req;
	Rsp02	rsp;
};

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

template <class T> struct QItem
{
	QItem	*next;
	T		*item;
};

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

template <class T> class Queue
{
	QItem<T>	* _first;
	QItem<T>	* _last;

public:

	Queue() : _first(0), _last(0) {}
	void Add(QItem<T> *i);
	QItem<T>* Get();
	bool Empty() { return _first == 0; }
};

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

template <class T> void Queue<T>::Add(QItem<T> *i)
{
	if (i != 0)
	{
		if (_first == 0)
		{
			_first = _last = i;
		}
		else
		{
			_last->next = i;
			_last = i;
		};

		i->next = 0;
	};
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

template <class T> QItem<T>* Queue<T>::Get()
{
	QItem<T>* r = _first;

	if (_first != 0)
	{
		_first = _first->next;
		r->next = 0;
	};

	return r;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++





#endif //REQ_H__09_10_2014__10_31
