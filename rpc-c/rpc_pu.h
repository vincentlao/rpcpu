#pragma once

#include "uv.h"
#include "protobuf-c\protobuf-c.h"
#include "ss.pb-c.h"

#define		REQUEST_MAGIC		"\x90\x90"
#define		RESPONSE_MAGIC		"\x91\x91"
#define		MAGIC_SIZE			2

#define		TYPE_UNKNOWN		0
#define		TYPE_REQUEST		1
#define		TYPE_RESPONSE		2

struct rpcpu_server;
struct rpcpu_client;
struct rpcpu_call_stub;
struct rpcpu_sync_context;
typedef struct rpcpu_server rpcpu_server;
typedef struct rpcpu_client rpcpu_client;
typedef struct rpcpu_call_stub rpcpu_call_stub;
typedef struct rpcpu_send_request_para rpcpu_send_request_para;
typedef struct rpcpu_sync_context rpcpu_sync_context;
typedef void(*request_return)(ProtobufCMessageDescriptor* message, void* context);

struct rpcpu_call_stub
{
	uint32_t id;
	request_return cb;
	ProtobufCMessageDescriptor* output;
	void* context;
};

//代表一个与客户端的链接
struct rpcpu_server
{
	uv_tcp_t srv_client;
	uv_tcp_s* srv_server;
	uint8_t status;
};

//代表一个与服务器端的链接
struct rpcpu_client
{
	uv_tcp_t cli_client;
	rpcpu_call_stub stub[100];
	uint8_t status;
};

//组包回调函数
struct rpcpu_closure_data
{
	rpcpu_server* srv;
	RpcRequest* req;
};

//同步调用参数
struct rpcpu_sync_context {
	ProtobufCMessage* msg;
	void* event;
};

struct rpcpu_send_request_para
{
	rpcpu_client* connection;
	ProtobufCService* rpc;
	char* name;
	ProtobufCMessage* message;
	request_return req_cb;
	void* context;
};

void rpcpu_reg_service(ProtobufCService* service);

void rpcpu_send_response(rpcpu_server* connection,
	RpcRequest* request,
	ProtobufCMessage* message);

void rpcpu_send_request(rpcpu_client* connection,
	ProtobufCService* rpc,
	char* name,
	ProtobufCMessage* message,
	request_return req_cb,
	void* context);

void rpcpu_receive_message(uv_stream_t* stream,
	ssize_t nread,
	const uv_buf_t* buf);

void rpcpu_create_server(char* ip, uint16_t port);

rpcpu_client* rpcpu_create_client(char* server_ip, uint16_t server_port);

void rpcpu_run();