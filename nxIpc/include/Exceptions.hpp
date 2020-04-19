#pragma once
#include <switch.h>
#include <stdexcept>

namespace nxIpc 
{
	class RFailedException : public std::runtime_error
	{
	public:
		RFailedException(Result rc, const char* message) : runtime_error(message)
		{
			ResultCode = rc;
		}

		Result ResultCode;
	};
}

#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)
#define AT __FILE__ ":" TOSTRING(__LINE__)

#define LogFunction(...) do { \
	printf(__VA_ARGS__); \
	fflush(stdout); \
} while (0)

#define R_THROW(x) do { \
	Result ___code = x; \
	if (R_FAILED(___code)) \
		throw nxIpc::RFailedException(___code, "R_FAILED " AT); \
}while (0)

#define R_THROW_ACTION(x, action) do { \
	Result ___code = x; \
	if (R_FAILED(___code)) { \
		action; \
		throw nxIpc::RFailedException(___code, "R_FAILED " AT); \
	} \
}while (0)


#define LibIPC_Module 222
#define R_EXCEPTION_IN_HANDLER MAKERESULT(LibIPC_Module,1)
#define R_UNKNOWN_CMDID MAKERESULT(LibIPC_Module,2)
#define R_UNIMPLEMENTED_CMDID MAKERESULT(LibIPC_Module,3)
