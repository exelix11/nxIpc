#pragma once
#include "Server.hpp"

namespace nxIpc 
{
	template <u32 MaxClients, typename ServiceT>
	class ThreadedServer : public Server<MaxClients, ServiceT>
	{
	public:
		ThreadedServer(const char* ServerName) : Server(ServerName)
		{

		}

		void LaunchThread() 
		{
			u32 priority;
			R_THROW(svcGetThreadPriority(&priority, CUR_THREAD_HANDLE));
			R_THROW(threadCreate(&thread, &ThreadedServer::ServerThread, this, NULL, 0x2000, priority, -2));
		}

		void StopThread() 
		{
			ShouldTerminate = true;
			svcCancelSynchronization(this->ServerHandle());
			threadWaitForExit(thread);
		}
	protected:

		void ServerThread() 
		{
			try {
				RunServerInThread();
			}
			catch (const std::exception & ex)
			{
				LogFunction("Server thread exception: %s", ex.what());
			}
		}

		Thread thread = {0};
	};
}