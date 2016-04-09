#include <cstdio>
#include <cstdlib>

extern "C"{
#include <sys/socket.h>
#include <uv/uv.h>
}

#include "Handlers.h"

static void OnClientRead(uv_stream_t *stream, ssize_t nread, const uv_buf_t *buf);
static void OnShutdown(uv_shutdown_t* req, int status);
static void OnClientClose(uv_handle_t* handle);

static void OnClientConnection(uv_stream_t *stream, int status){
    if(status >= 0){
        //Get client info
        uv_tcp_t* tcp_handle = reinterpret_cast<uv_tcp_t*>(stream);
        socket_addr_t sa;
        int name_len;
        uv_tcp_getpeername(const_cast<const uv_tcp_t*>(tcp_handle), &sa, &name_len);

        std::string address = get_address(&sa);
        int port = get_port(&sa);
        fprintf(stdout, "Client connect: %s:%d\n", address.c_str(), port);

        //Init client handle and context
        uv_tcp_t* client = (uv_tcp_t*)malloc(sizeof(uv_tcp_t));
        uv_tcp_init(stream->loop, client);

        client->data = new Context();

        if(uv_accept(stream, reinterpret_cast<uv_stream_t*>(client)) >= 0){
            //Success
            uv_read_start(reinterpret_cast<uv_stream_t *>(client), BufferAllocator, OnClientRead);
        }else{
            //Error
            uv_close(reinterpret_cast<uv_handle_t*>(client), NULL);
        }
    }
}

static void OnClientRead(uv_stream_t *stream, ssize_t nread, const uv_buf_t *buf){

    if(nread <= 0 && buf->base != NULL && buf->len > 0){
        delete[] buf->base;
        return;
    }

    auto request = msg::GetRequest(buf->base);
    if(request != nullptr){
        switch(request->command()){
            case msg::Cmd_CD:{
                handlers::gCDHandler(stream, nread, buf);
                break;
            }
            case fbs::hw1::Cmd_LS:{
                handlers::gLSHandler(stream, nread, buf);
                break;
            }
            case fbs::hw1::Cmd_PUT:break;
            case fbs::hw1::Cmd_GET:break;
            case fbs::hw1::Cmd_QUIT:{
                auto req = new uv_shutdown_t;
                uv_shutdown(req, stream, OnShutdown);
                break;
            }
        }
    }
}

static void OnShutdown(uv_shutdown_t* req, int status){
    auto stream = req->handle;
    auto ctx = Context::GetContext(stream);
    if(ctx != nullptr) delete ctx;

    delete req;

    uv_close(reinterpret_cast<uv_handle_t*>(stream), OnClientClose);
}
static void OnClientClose(uv_handle_t* handle){
    //Reserve
}

int main(int argc, char** argv){

    auto loop = uv_default_loop();

    if(argc - 1 < 1){
        fprintf(stdout, "Usage: %s <port number>\n", argv[0]);
        return 1;
    }

    char** args = uv_setup_args(argc, argv);
    int port = atoi(args[1]);
    if(port <= 0){
        fprintf(stderr, "Port number must be positive\n");
        return 1;
    }

    handlers::InitHandlers();

    uv_tcp_t server;

    uv_tcp_init(loop, &server);

    struct sockaddr_in bind_addr;
    uv_ip4_addr("127.0.0.1", port, &bind_addr);

    int err;

    if( (err = uv_tcp_bind(&server, reinterpret_cast<const struct sockaddr*>(&bind_addr), 0)) < 0){
        fprintf(stderr, "Error binding: %s\n", uv_strerror(err));
        return 1;
    }

    if( (err = uv_listen(reinterpret_cast<uv_stream_t*>(&server), 256, OnClientConnection)) < 0){
        fprintf(stderr, "Error listening: %s\n", uv_strerror(err));
        return 1;
    }

    fprintf(stdout, "Listening on 127.0.0.1:%d...\n", port);

    uv_run(loop, UV_RUN_DEFAULT);

    return 0;
}

