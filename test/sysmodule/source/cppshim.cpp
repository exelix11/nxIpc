#include "../../../nxIpc/include/Server.hpp"
#include "IPCInterface.hpp"

#include <cstdio>
#include <stdexcept>

extern "C" {
	
	void CPPThread()
	{
		try {
			nxIpc::Server<5, TestServer> test("test");
			test.RunServerInThread();
		}
		catch (const std::exception& ex){
			printf(ex.what());
		}
	
	}
	
	
}