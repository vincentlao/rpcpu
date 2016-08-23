
#include "rpc_pu.h"

uv_async_t send_async;

ProtobufCService* services[100];
uint8_t srv_iter = 0;

void rpcpu_alloc_cb(uv_handle_t* handle,
	size_t suggested_size,
	uv_buf_t* uv_buf);

void rpcpu_receive_message(uv_stream_t* stream,
	ssize_t nread,
	const uv_buf_t* buf);

void rpcpu_send_request_internal(rpcpu_client* connection,
	ProtobufCService* rpc,
	char* name,
	ProtobufCMessage* message,
	request_return req_cb,
	void* context);

void rpcpu_common_request_return(ProtobufCMessageDescriptor* message,
	void* context);

void write_cb(uv_write_t* req, int status)
{
	free(req->data);
	free(req);
}

void rpcpu_common_closure(const ProtobufCMessage *msg, void *closure_data)
{

	rpcpu_closure_data* closure = (rpcpu_closure_data*)closure_data;
	rpcpu_send_response(closure->srv, closure->req, (ProtobufCMessage*)msg);

}


void rpcpu_reg_service(ProtobufCService* service)
{
	services[srv_iter++] = service;
}

void rpcpu_send_response(rpcpu_server* connection,
	RpcRequest* request,
	ProtobufCMessage* message)
{

	//create response object
	RpcResponse response = RPC_RESPONSE__INIT;
	response.id = request->id;

	//get the total send buffer size
	size_t res_size = rpc_response__get_packed_size(&response);
	size_t msg_size = protobuf_c_message_get_packed_size((const ProtobufCMessage*)(message));

	//group the packet
	char* send_buf = (char*)malloc(res_size + msg_size + MAGIC_SIZE + 4);
	char* iter = send_buf;

	//packet format:
	//RESPONSE_MAGIC|res_size|packed response|msg_size|packed message

	memcpy(send_buf, RESPONSE_MAGIC, MAGIC_SIZE);
	iter += MAGIC_SIZE;
	*(uint16_t*)iter = res_size;
	iter += 2;
	rpc_response__pack(&response, (uint8_t*)iter);
	iter += res_size;
	*(uint16_t*)iter = msg_size;
	iter += 2;
	protobuf_c_message_pack((const ProtobufCMessage*)message, (uint8_t*)iter);

	//send it
	uv_buf_t uv_buf = uv_buf_init(send_buf, res_size + msg_size + MAGIC_SIZE + 4);
	uv_write_t* writer = (uv_write_t*)malloc(sizeof(uv_write_t));
	writer->data = send_buf;

	uv_write(writer, (uv_stream_t*)&connection->srv_client, &uv_buf, 1, write_cb);

}

void rpcpu_send_request_cb(uv_async_t* handle)
{

	rpcpu_send_request_para* para = (rpcpu_send_request_para*)handle->data;
	if (para == 0)
		return;

	rpcpu_send_request_internal(para->connection, para->rpc, para->name, para->message, para->req_cb, para->context);
	free(para);

}

void rpcpu_send_request(rpcpu_client* connection,
	ProtobufCService* rpc,
	char* name,
	ProtobufCMessage* message,
	request_return req_cb,
	void* context)
{

	rpcpu_send_request_para* para = (rpcpu_send_request_para*)malloc(sizeof(rpcpu_send_request_para));
	para->connection = connection;
	para->rpc = rpc;
	para->name = name;
	para->message = message;
	para->req_cb = req_cb;
	para->context = context;

	send_async.data = para;
	uv_async_send(&send_async);

}

void rpcpu_common_request_return(ProtobufCMessageDescriptor* message, void* context)
{
	rpcpu_sync_context* sync_ctx = (rpcpu_sync_context*)context;
	sync_ctx->msg = (ProtobufCMessage*)message;
	SetEvent((HANDLE)sync_ctx->event);
}

ProtobufCMessage* rpcpu_send_request_sync(rpcpu_client* connection,
	ProtobufCService* rpc,
	char* name,
	ProtobufCMessage* message)
{

	rpcpu_send_request_para* para = (rpcpu_send_request_para*)malloc(sizeof(rpcpu_send_request_para));
	para->connection = connection;
	para->rpc = rpc;
	para->name = name;
	para->message = message;
	para->req_cb = rpcpu_common_request_return;

	rpcpu_sync_context sync_ctx;
	sync_ctx.event = CreateEvent(NULL, FALSE, FALSE, NULL);
	sync_ctx.msg = 0;
	para->context = &sync_ctx;

	send_async.data = para;
	uv_async_send(&send_async);

	WaitForSingleObject(para->context, INFINITE);
	CloseHandle((HANDLE)sync_ctx.event);

	return sync_ctx.msg;

}

void rpcpu_send_request_internal(rpcpu_client* connection,
	ProtobufCService* rpc,
	char* name,
	ProtobufCMessage* message,
	request_return req_cb,
	void* context)
{

	//create req_request object
	RpcRequest req = RPC_REQUEST__INIT;
	for (int i = 0;i < rpc->descriptor->n_methods;i++)
	{
		if (!strcmp(rpc->descriptor->methods[i].name, name))
		{
			req.request = i;
			break;
		}
	}
	req.service = (char*)malloc(strlen(rpc->descriptor->name) + 1);
	strcpy(req.service, rpc->descriptor->name);

	//save the request stub
	for (int i = 0;i < 100;i++)
	{
		if (connection->stub[i].id == 0xFFFFFFFF)
		{
			InterlockedExchange(&connection->stub[i].id, i);
			req.id = i;
			connection->stub[i].output = (ProtobufCMessageDescriptor*)rpc->descriptor->methods[req.request].output;
			connection->stub[i].context = context;
			connection->stub[i].cb = req_cb;
			break;
		}
	}

	//get the total send buffer size
	size_t req_size = rpc_request__get_packed_size(&req);
	size_t msg_size = protobuf_c_message_get_packed_size((const ProtobufCMessage*)(message));

	//group the packet
	char* send_buf = (char*)malloc(req_size + msg_size + MAGIC_SIZE + 4);
	char* iter = send_buf;

	//packet format:
	//REQUEST_MAGIC|req_size|packed request|msg_size|packed message

	memcpy(iter, REQUEST_MAGIC, MAGIC_SIZE);
	iter += MAGIC_SIZE;
	*(uint16_t*)iter = req_size;
	iter += 2;
	rpc_request__pack(&req, (uint8_t*)iter);
	iter += req_size;
	*(uint16_t*)iter = msg_size;
	iter += 2;
	protobuf_c_message_pack((const ProtobufCMessage*)message, (uint8_t*)iter);

	ProtobufCMessage* ss = protobuf_c_message_unpack((const ProtobufCMessageDescriptor*)message, NULL, msg_size, (uint8_t*)iter);

	//send it
	uv_buf_t uv_buf = uv_buf_init(send_buf, req_size + msg_size + MAGIC_SIZE);
	uv_write_t* writer = (uv_write_t*)malloc(sizeof(uv_write_t));
	writer->data = send_buf;

	uv_write(writer, (uv_stream_t*)&connection->cli_client, &uv_buf, 1, write_cb);

	//free the resource
	free(req.service);

}

uint32_t rpcpu_get_receive_type(uint8_t* buf)
{

	//adjust the packet type
	assert(buf != 0);
	if (!memcmp(buf, REQUEST_MAGIC, 2))
	{
		return TYPE_REQUEST;
	}
	if (!memcmp(buf, RESPONSE_MAGIC, 2))
	{
		return TYPE_RESPONSE;
	}
	return TYPE_UNKNOWN;

}

//得到数据后，以回调的方式将返回值通知caller

void rpcpu_worker_return_cb(uv_work_t* req)
{

	rpcpu_call_stub* stub = (rpcpu_call_stub*)req->data;
	stub->cb(stub->output, stub->context);
	//may be we shouldn't free the packed,let caller to do it
	if (stub->cb != rpcpu_common_request_return)
		protobuf_c_message_free_unpacked((ProtobufCMessage*)stub->output, NULL);
	free(req->data);
	free(req);
	return;

}

//收包处理函数
void rpcpu_receive_message(uv_stream_t* stream,
	ssize_t nread,
	const uv_buf_t* buf)
{

	if (buf->len == 0)
		return;

	uint32_t receive_type = rpcpu_get_receive_type((uint8_t*)buf->base);
	if (receive_type == TYPE_UNKNOWN)		//maybe 传输错误
		return;

	char* iter = buf->base + 2;

	if (receive_type == TYPE_RESPONSE)
		//handle the response
	{
		//unpack the response header
		rpcpu_client* cli = (rpcpu_client*)stream;
		uint16_t res_size = *(uint16_t*)iter;
		iter += 2;
		char* response_buf = iter;
		RpcResponse* response = rpc_response__unpack(NULL, res_size, (uint8_t*)response_buf);
		iter += res_size;

		//find the specify stub
		rpcpu_call_stub* stub = (rpcpu_call_stub*)malloc(sizeof(rpcpu_call_stub));
		memcpy(stub, &cli->stub[response->id], sizeof(rpcpu_call_stub));
		InterlockedExchange(&cli->stub[response->id].id, 0xFFFFFFFF);

		//retrieve the message and call the saved callback routine
		uint16_t msg_size = *(uint16_t*)iter;
		iter += 2;
		ProtobufCMessage* rpc_msg = protobuf_c_message_unpack(stub->output,
			NULL,
			msg_size,
			(uint8_t*)iter);
		stub->output = (ProtobufCMessageDescriptor*)rpc_msg;

		uv_work_t* work = (uv_work_t*)malloc(sizeof(uv_work_t));
		work->data = stub;
		uv_queue_work(uv_default_loop(), work, rpcpu_worker_return_cb, 0);

		rpc_response__free_unpacked(response, NULL);
		return;
	}
	else
		//handle the request
	{
		//unpack the request header
		rpcpu_server* srv = (rpcpu_server*)stream;
		uint16_t req_size = *(uint16_t*)iter;
		iter += 2;
		char* request_buf = iter;
		RpcRequest* request = rpc_request__unpack(NULL, req_size, (uint8_t*)request_buf);
		iter += req_size;

		ProtobufCService* service;
		for (int i = 0;i < srv_iter;i++)
		{
			if (!strcmp(services[i]->descriptor->name, request->service))
			{
				service = (ProtobufCService*)services[i];
				break;
			}
		}

		//retrieve the message and call the saved callback routine
		uint16_t msg_size = *(uint16_t*)iter;
		iter += 2;
		ProtobufCMessage* rpc_msg = protobuf_c_message_unpack(service->descriptor->methods[request->request].input,
			NULL,
			msg_size,
			(uint8_t*)iter);

		rpcpu_closure_data closure_data;
		closure_data.req = request;
		closure_data.srv = srv;
		service->invoke(service, request->request, rpc_msg, rpcpu_common_closure, &closure_data);

		protobuf_c_message_free_unpacked(rpc_msg, NULL);
		rpc_request__free_unpacked(request, NULL);

	}

	return;

}

void rpcpu_alloc_cb(uv_handle_t* handle,
	size_t suggested_size,
	uv_buf_t* uv_buf)
{
	static char buf[4096];
	*uv_buf = uv_buf_init(buf, 4096);
}

void rpcpu_connection_cb(uv_stream_t *server, int status) {

	if (status == -1) {
		// error!
		return;
	}

	rpcpu_server* srv = (rpcpu_server*)malloc(sizeof(rpcpu_server));
	uv_tcp_init(uv_default_loop(), &srv->srv_client);

	if (uv_accept(server, (uv_stream_t*)&srv->srv_client) == 0) {
		srv->srv_server = (uv_tcp_s*)server;
		uv_read_start((uv_stream_t*)&srv->srv_client, rpcpu_alloc_cb, rpcpu_receive_message);
	}
	else {
		uv_close((uv_handle_t*)&srv->srv_client, NULL);
		free(srv);
	}

}

void rpcpu_create_server(char* ip, uint16_t port)
{

	//initialize the tcp handle
	uv_tcp_s* server = (uv_tcp_s*)malloc(sizeof(uv_tcp_s));
	uv_tcp_init(uv_default_loop(), server);

	sockaddr_in addr;
	uv_ip4_addr(ip, port, &addr);
	uv_tcp_bind(server, (sockaddr*)&addr, 0);

	int r = uv_listen((uv_stream_t*)server, SOMAXCONN, rpcpu_connection_cb);

}

void rpcpu_connect_cb(uv_connect_t* req, int status)
{

	rpcpu_client* cli = (rpcpu_client*)req->handle;
	cli->status = status;
	if (status != 0) {
		// error!
		free(req);
		return;
	}
	uv_read_start((uv_stream_t*)&cli->cli_client, rpcpu_alloc_cb, rpcpu_receive_message);

}

rpcpu_client* rpcpu_create_client(char* server_ip, uint16_t server_port)
{

	//initialize the tcp handle
	rpcpu_client* cli = (rpcpu_client*)malloc(sizeof(rpcpu_client));
	if (uv_tcp_init(uv_default_loop(), &cli->cli_client))
		return 0;

	sockaddr_in addr;
	uv_ip4_addr(server_ip, server_port, &addr);

	uv_connect_s* conn = (uv_connect_s*)malloc(sizeof(uv_connect_s));
	uv_tcp_connect(conn, &cli->cli_client, (sockaddr*)&addr, rpcpu_connect_cb);

	uv_async_init(uv_default_loop(), &send_async, rpcpu_send_request_cb);

	for (int i = 0;i < 100;i++)
		cli->stub[i].id = 0xFFFFFFFF;

	//set keep alive in case the
	uv_tcp_keepalive(&cli->cli_client, true, 300);
	return cli;

}

void rpcpu_run()
{
	uv_run(uv_default_loop(), UV_RUN_DEFAULT);
}