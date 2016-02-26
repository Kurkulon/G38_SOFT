#include "req.h"

#include "CRC16.h"


//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

void RequestQueue::Add(REQ* req)
{
	if (req != 0)
	{
		if (_first == 0)
		{
			_first = _last = req;
		}
		else
		{
			_last->next = req;
			_last = req;
		};

		req->next = 0;
	};
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

REQ* RequestQueue::Get()
{
	REQ* r = _first;

	if (_first != 0)
	{
		_first = _first->next;
		r->next = 0;
	};

	return r;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

void RequestQueue::Update()
{
		DataPointer p(0);

		switch(_state)
		{
			case 0:

				_req = (_run) ? Get() : 0;

				if (_req != 0) _state++;

				break;

			case 1:

				p = _req->wb.data;
				p.b += _req->wb.len;

				*p.w = GetCRC16(_req->wb.data, _req->wb.len);
				_req->wb.len += 2;
				com->Write(&_req->wb);
				_state++;

				break;

			case 2:

				if (!com->Update())
				{
					com->Read(&_req->rb, _req->preTimeOut, _req->postTimeOut); 
					_state++;
				};

				break;

			case 3:

				if (!com->Update())
				{
					_req->crcOK = GetCRC16(_req->rb.data, _req->rb.len) == 0;
					_state++;
				};

				break;

			case 4:

				if (_req->CallBack != 0)
				{
					_req->CallBack(_req);
				};

				_state = 0;

				break;

			default:

				_state = 0;
		};
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
