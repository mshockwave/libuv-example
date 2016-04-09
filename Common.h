
#ifndef NP_HW1_UTILS_H
#define NP_HW1_UTILS_H

extern "C"{
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <uv/uv.h>
};

#include <cstring>
#include <cstdlib>
#include <string>

#include "flatbuffers/request_generated.h"
#include "flatbuffers/response_generated.h"

#define TRANS_BUF_SIZE 2048

typedef struct sockaddr socket_addr_t;

#define SOCK_ADDR_IN(sa_ptr) \
    reinterpret_cast<struct sockaddr_in*>((sa_ptr))
#define SOCK_ADDR_IN6(sa_ptr) \
    reinterpret_cast<struct sockaddr_in6*>((sa_ptr))

extern int get_port(socket_addr_t *sa);
extern std::string get_address(socket_addr_t *sa);

extern void BufferAllocator(uv_handle_t*, size_t, uv_buf_t*);

struct WriteRequest {

private:
    WriteRequest(){
        //Initialize
        write_buffer.base = NULL;
        write_buffer.len = 0;
    }

public:
    uv_buf_t write_buffer;

    static uv_write_t* New(){
        uv_write_t* req = new uv_write_t;
        req->data = new WriteRequest();
        return req;
    }
    static void Destroy(uv_write_t* req){
        if(req == NULL) return;

        if(req->data != NULL){
            auto payload = reinterpret_cast<WriteRequest*>(req->data);
            delete payload;
        }

        delete req;
    }

    static uv_buf_t* GetWriteBuffer(uv_write_t* req){
        if(req == NULL || req->data == NULL) return nullptr;
        auto wq = reinterpret_cast<WriteRequest*>(req->data);
        return &(wq->write_buffer);
    }
    static void TransferData(uv_write_t* req, void* data, size_t size){
        if(req == NULL || req->data == NULL) return;
        auto wq = reinterpret_cast<WriteRequest*>(req->data);

        char* buf = new char[size / sizeof(char)];
        memcpy(buf, data, size);
        wq->write_buffer = uv_buf_init(buf, (unsigned int)(size / sizeof(char)));
    }

    ~WriteRequest(){
        if(write_buffer.base != NULL){
            delete[] write_buffer.base;
        }
    }
};

//flatbuffers related
namespace msg = fbs::hw1;

#define CAST_2_STRING_PAYLOAD(P) \
    static_cast<const msg::StringPayload*>(P)

#define STRING_PAYLOAD_2_STR(P) \
    (P)->content()->data()

#endif //NP_HW1_UTILS_H
