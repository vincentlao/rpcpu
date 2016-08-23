// rpc-c.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include "rpc_pu.h"
//#include "windows.h"
//#include <iostream>
//
//#include <thrift/protocol/TBinaryProtocol.h>
//#include <thrift/transport/TSocket.h>
//#include <thrift/transport/TTransportUtils.h>
//
//#include "SrvTest.h"
void get_test(RpcTest_Service *service,
	const Test1 *input,
	Test2_Closure closure,
	void *closure_data)
{
	int a = 1;
	Test2 t = TEST2__INIT;
	t.id = 0;
	t.value = 0;

	closure(&t, closure_data);
}

void rr(ProtobufCMessageDescriptor* message, void* context)
{
	int a = 3;
}

int main()
{

	//uv_tcp_s server;
	//uv_tcp_init(uv_default_loop(), &server);

	Test1 t = TEST1__INIT;
	t.id = 1;
	t.value = "1234";

	char buf[100];
	RpcTest_Service rs = RPC_TEST__INIT();

	//rpc_test__get_test((ProtobufCService*)&rs, &t, tc, buf);
	rpcpu_reg_service((ProtobufCService*)&rs);
	rpcpu_create_server("127.0.0.1", 10000);
	rpcpu_run();

	//rpcpu_client* cli=rpcpu_create_client("127.0.0.1", 10000);
	//rpcpu_send_request(cli, (ProtobufCService*)&rs, "GetTest", (ProtobufCMessage*)&t, rr, NULL);


	//rpcpu_send_request()

    return 0;
}

