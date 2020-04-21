//#pragma once
//#include "Server.hpp"
//
//namespace nxIpc 
//{
//	template <u32 MaxClients, typename ServiceT>
//	class ThreadedServer : public Server<MaxClients, ServiceT>
//	{
//	public:
//		ThreadedServer(const char* ServerName) : Server<MaxClients, ServiceT>(ServerName)
//		{
//
//		}
//
//		void LaunchThread() 
//		{
//			R_THROW(threadCreate(&thread, (ThreadFunc)&ThreadedServer<MaxClients, ServiceT>::ServerThread, this, NULL, 0x2000, 0x3F, 3));
//		}
//
//		void StopThread() 
//		{
//			this->ShouldTerminate = true;
//			svcCancelSynchronization(this->ServerHandle());
//			threadWaitForExit(&thread);
//		}
//	public:
//
//		void ServerThread() 
//		{
//			try {
//				this->RunServerInThread();
//			}
//			catch (const std::exception & ex)
//			{
//				LogFunction("Server thread exception: %s", ex.what());
//			}
//		}
//
//		Thread thread = {0};
//	};
//}