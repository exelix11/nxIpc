#pragma once
#include <switch.h>
#include <array>
#include <algorithm>

#include "../../../nxIpc/include/Server.hpp"

using namespace nxIpc;

class TestServer : public nxIpc::IInterface
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

	bool ReceivedCommand(Request& req) override
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
		LogFunction("SetValue got: %d %s\n", valueStore.num, valueStore.name);
		Response().Finalize();
	}

	void GetValue(Request& req)
	{	
		LogFunction("GetValue returning: %d %s\n", valueStore.num, valueStore.name);
		Response().Payload(valueStore).Finalize();
	}

	void EchoSend(Request& req)
	{
		auto buf = req.ReadBuffer(0);

		LogFunction("EchoSend got: %c%c%c%c\n", ((const char*)buf.data)[0], ((const char*)buf.data)[1], ((const char*)buf.data)[2], ((const char*)buf.data)[3]);

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
			LogFunction("EchoReceive send: %c%c%c%c\n", ((const char*)data)[0], ((const char*)data)[1], ((const char*)data)[2], ((const char*)data)[3]);
			Response().Payload((u32)dataLen).Finalize();
		}
		else
		{
			LogFunction("EchoReceive: no data\n");
			Response(666).Finalize();
		}
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