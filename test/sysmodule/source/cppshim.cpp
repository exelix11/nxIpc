#include "../../../nxIpc/include/ServerHost.hpp"
#include "IPCInterface.hpp"

#include <cstdio>
#include <stdexcept>

extern "C" {
	
	void CPPThread()
	{
		try {
			nxIpc::Server<TestServer> test(5, "test");
			nxIpc::ServerHost host;
			host.AddServer(&test);
			host.StartServer();
		}
		catch (const std::exception& ex){
			printf(ex.what());
		}
	
	}
	
	
}