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
	using CallHandler = void (TestServer::*)(Request & req);

	const std::array<CallHandler, 10> handlers {
		&TestServer::SetValue,
		&TestServer::GetValue,
		&TestServer::EchoSend,
		&TestServer::EchoReceive,
		&TestServer::GetEvent,
		&TestServer::FireEvent,
	};

	bool ReceivedCommand(Request& req)
	{
		if (req.cmdId >= handlers.size())
		{
			Response(R_UNKNOWN_CMDID).Finalize();
			return true;
		}

		auto handler = handlers[req.cmdId];

		if (!handler)
		{
			Response(R_UNIMPLEMENTED_CMDID).Finalize();
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
	void SetValue(Request& req)
	{
		valueStore = *req.Payload<MySettableValue>();
		Response().Finalize();
	}

	void GetValue(Request& req)
	{	
		Response().Payload(valueStore).Finalize();
	}

	void EchoSend(Request& req)
	{
		auto buf = req.ReadBuffer(0);

		if (data)
			delete[] data;

		dataLen = buf.length;
		data = new u8[dataLen];
		buf.CopyTo_s(data, dataLen);

		Response().Payload((u32)dataLen).Finalize();	
	}

	void EchoReceive(Request& req)
	{
		if (data)
		{
			req.WriteBuffer(0).AssignFrom_s(data, dataLen);
			Response().Payload((u32)dataLen).Finalize();
		}
		else
			Response(666).Finalize();
	}

	void GetEvent(Request& req)
	{
		if (!eventActive(&evt))
			R_THROW(eventCreate(&evt, true));

		Response().CopyHandle(evt.revent).Finalize();
	}

	void FireEvent(Request& req)
	{
		eventFire(&evt);
		Response().Finalize();
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