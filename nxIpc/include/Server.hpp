#pragma once
#include <switch.h>
#include <map>
#include <atomic>
#include <vector>
#include <memory>

#include "Types.hpp"
#include "Exceptions.hpp"

namespace nxIpc 
{
	template <u32 MaxClients, typename ServiceT>
	class Server
	{
		static_assert(MaxClients > 0 && MaxClients < MAX_WAIT_OBJECTS, "MaxClients is out of range");
	public:
		Server(const char* ServerName)
		{
			std::strncpy(this->ServerName.name, ServerName, sizeof(this->ServerName));
			Handle ServerHandle;
			R_THROW(smRegisterService(&ServerHandle, this->ServerName, false, MaxClients));
			Handles.reserve(MaxClients + 1);
			Handles.push_back(ServerHandle);
		}

		void RunServerInThread()
		{
			IsRunning = true;
			ShouldTerminate = false;
			while (IsRunning)
			{
				if (ShouldTerminate)
					TerminateServer();

				s32 handleIndex = -1;

				Result rc = svcWaitSynchronization(&handleIndex, Handles.data(), Handles.size(), UINT64_MAX);
				if (R_FAILED(rc))
				{
					if (rc == KERNELRESULT(Cancelled))
					{
						TerminateServer();
						break;
					}
					if (rc != KERNELRESULT(ConnectionClosed))
						throw RFailedException(rc, "R_FAILED" AT);
				}

				if (handleIndex < 0 || (size_t)handleIndex >= Handles.size())
				{
					TerminateServer();
					throw RFailedException(MAKERESULT(Module_Libnx, LibnxError_NotFound), "R_FAILED" AT);
				}

				try
				{
					if (handleIndex)
						ProcessSession(handleIndex);
					else
						AcceptNewSession();
				}
				catch (const std::exception & ex)
				{
					LogFunction("%s\n", ex.what());
				}
			}
		}

	protected:
		std::atomic<bool> ShouldTerminate = false;

		Handle& ServerHandle() 
		{
			return Handles[0];
		}

	private:
		void TerminateServer()
		{
			IsRunning = false;

			for (Handle h : Handles)
				svcCloseHandle(h);

			Sessions.clear();
			Handles.clear();
		}

		void AcceptNewSession()
		{
			Handle session = 0;
			R_THROW(svcAcceptSession(&session, Handles[0]));

			if (Handles.size() >= MaxClients)
			{
				svcCloseHandle(session);
				LogFunction("Max number of clients reached !");
				return;
			}
			else
			{
				Handles.push_back(session);
				Sessions[session] = std::make_unique<ServiceT>();
			}
		}

		void DeleteSession(u32 index)
		{
			if (!index || index >= Handles.size())
				return;

			Handle h = Handles[index];
			Sessions.erase(h);

			svcCloseHandle(Handles[index]);
			Handles.erase(Handles.begin() + index);
		}

		void ProcessSession(u32 handleIndex)
		{
			Handle handle = Handles[handleIndex];

			s32 _idx;
			bool ShouldClose = false;

			R_THROW(svcReplyAndReceive(&_idx, &handle, 1, 0, UINT64_MAX));
			IPCRequest r = IPCRequest::ParseFromTLS();

			switch (r.hipc.meta.type)
			{
			case CmifCommandType_Request:
				try {
					ShouldClose = Sessions[handle]->ReceivedCommand(r);
				}
				catch (const std::exception & ex)
				{
					LogFunction("%s", ex.what());
					ShouldClose = true;
					IPCResponse(R_EXCEPTION_IN_HANDLER).PrepareResponse();
				}
				break;
			case CmifCommandType_Close:
				IPCResponse(0).PrepareResponse();
				ShouldClose = true;
				break;
			default:
				IPCResponse(MAKERESULT(11, 403)).PrepareResponse();
				break;
			}

			Result rc = svcReplyAndReceive(&_idx, &handle, 0, handle, 0);

			if (rc == KERNELRESULT(TimedOut))
				rc = 0;
			else R_THROW(rc);

			if (ShouldClose)
				DeleteSession(handleIndex);
		}

		std::vector<Handle> Handles;

		std::atomic<bool> IsRunning;
		std::map<Handle, std::unique_ptr<ServiceT>> Sessions;
		SmServiceName ServerName = { 0 };
	};
}