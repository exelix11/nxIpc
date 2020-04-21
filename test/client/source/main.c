#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <switch.h>

typedef struct
{
	int num;
	char name[0x10];
} PACKED MySettableValue;

MySettableValue val = 
{
	69420, "testTESTtest"
};

static Service svc;

Result svcInitialize()
{
    Result rc = smGetService(&svc, "test");
	return rc;
}

void svcExit()
{
    serviceClose(&svc);
}

Result SendValue()
{
	return serviceDispatchIn(&svc, 0, val);
}

Result GetValue()
{
	MySettableValue v = {0};
	Result rc = serviceDispatchOut(&svc, 1, v);
	if (R_FAILED(rc))
		return rc;
	return memcmp(&v, &val, sizeof(val));
}

Result EchoMessage()
{
	const char In[] = "TestTestTest";
	char Out[sizeof(In)] = "Wrong";
	
	Result rc =	serviceDispatch(&svc, 2,
		.buffer_attrs = { SfBufferAttr_HipcMapAlias | SfBufferAttr_In },
		.buffers = { { In, sizeof(In) } },
	);
	
	if (R_SUCCEEDED(rc))
	{
		rc = serviceDispatch(&svc, 3,
			.buffer_attrs = { SfBufferAttr_HipcMapAlias | SfBufferAttr_Out },
			.buffers = { {Out, sizeof(Out)} },
		);
		return strcmp(In, Out);
	}
	else printf("fail");
	
	return rc;
}

Event evt;
Result GetEvent()
{
	Handle event = INVALID_HANDLE;
	Result rc = serviceDispatch(&svc, 4,
		.out_handle_attrs = { SfOutHandleAttr_HipcCopy },
		.out_handles = &event
	);

	if (R_SUCCEEDED(rc))
		eventLoadRemote(&evt, event, true);

	return rc;
}

Result Evtcheck() 
{
	return eventWait(&evt, 1000);
}

Result FireEvent()
{
	if (R_SUCCEEDED(Evtcheck()))
		return 1;
	
	Result rc = serviceDispatch(&svc, 5);
	if (R_FAILED(rc))
		return rc;

	return Evtcheck();
}

int main(int argc, char* argv[])
{
    consoleInit(NULL);

	printf("Performing tests:\n\n");

#define TEST(name, fn) do { \
		Result rc = fn; \
		printf("%s : %x\n", name,rc); \
		if (R_FAILED(rc)) goto end_tests; \
	} while (0)

	TEST("Connect", svcInitialize());
	TEST("SendValue", SendValue());
	TEST("GetValue", GetValue());
	TEST("GetEvent", GetEvent());
	TEST("FireEvent", FireEvent());
	TEST("EchoMessage", EchoMessage());

	printf("\nALL GOOD !");

	end_tests:
    while (appletMainLoop())
    {
        hidScanInput();

        u64 kDown = hidKeysDown(CONTROLLER_P1_AUTO);

        if (kDown & KEY_PLUS)
            break; 

        consoleUpdate(NULL);
    }

    consoleExit(NULL);
    return 0;
}
