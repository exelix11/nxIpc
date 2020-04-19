#pragma once
#include <switch.h>
#include <array>
#include <algorithm>

#include "../../../nxIpc/include/Types.hpp"
#include "../../../nxIpc/include/Exceptions.hpp"

using namespace nxIpc;

class TestServer
{
public:
	using CallHandler = void (TestServer::*)(IPCRequest & req);

	const std::array<CallHandler, 10> handlers {
		&TestServer::SetValue,
		&TestServer::GetValue,
		&TestServer::EchoSend,
		&TestServer::EchoReceive,
		&TestServer::GetEvent,
		&TestServer::FireEvent,
	};

	bool ReceivedCommand(IPCRequest& req)
	{
		if (req.cmdId >= handlers.size())
		{
			IPCResponse(R_UNKNOWN_CMDID).PrepareResponse();
			return true;
		}

		auto handler = handlers[req.cmdId];

		if (!handler)
		{
			IPCResponse(R_UNIMPLEMENTED_CMDID).PrepareResponse();
			return true;
		}

		(this->*handler)(req);
		return false;
	}

	~TestServer() 
	{
		if (eventActive(&evt))
			eventClose(&evt);

		if (data)
			delete[] data;
	}

protected:
	void SetValue(IPCRequest& req)
	{
		valueStore = *req.Payload<MySettableValue>();
		IPCResponse(0).PrepareResponse();
	}

	void GetValue(IPCRequest& req)
	{	
		IPCResponse(0).Payload(valueStore).PrepareResponse();
	}

	void EchoSend(IPCRequest& req)
	{
		auto buf = req.ReadBuffer(0);

		if (data)
			delete[] data;

		dataLen = buf.length;
		data = new u8[dataLen];
		buf.CopyTo_s(data, dataLen);

		IPCResponse(0).Payload((u32)dataLen).PrepareResponse();	
	}

	void EchoReceive(IPCRequest& req)
	{
		if (data)
		{
			req.WriteBuffer(0).AssignFrom_s(data, dataLen);
			IPCResponse(0).Payload((u32)dataLen).PrepareResponse();
		}
		else
			IPCResponse(666).PrepareResponse();
	}

	void GetEvent(IPCRequest& req)
	{
		if (!eventActive(&evt))
			R_THROW(eventCreate(&evt, true));

		IPCResponse().CopyHandle(evt.revent).PrepareResponse();
	}

	void FireEvent(IPCRequest& req)
	{
		eventFire(&evt);
		IPCResponse().PrepareResponse();
	}
private:
	struct PACKED MySettableValue
	{
		int num;
		char name[0x10];
	};

	size_t dataLen;
	u8* data = nullptr;
	
	MySettableValue valueStore = {0};

	Event evt = { 0 };
};