
#ifndef NP_HW1_CONTEXT_H
#define NP_HW1_CONTEXT_H

#include <string>

extern "C" {
#include <unistd.h>
#include <uv/uv.h>
};

#include <Common.h>

//Context of each stream
class Context {
private:
    std::string pwd;

public:
    std::string client_address;
    int client_port;

    Context(const char* client_address_, int port) :
            pwd("Upload"),
            client_address(client_address_),
            client_port(port) {}

    bool set_current_dir(const char* path){
        if(!access(path, F_OK) && !access(path, X_OK) && !access(path, R_OK)){
            pwd = path;
            return true;
        }else{
            return false;
        }
    }
    const char* get_current_dir(){ return pwd.c_str(); }

    static void OnWriteFinish(uv_write_t *req, int status){
        if(status < 0){
            fprintf(stderr, "Error raised in OnWriteFinish: %d\n", status);
        }

        WriteRequest::Destroy(req);
    }

    static Context* GetContext(uv_stream_t* stream){
        if(stream == NULL || stream->data == NULL) return nullptr;
        return reinterpret_cast<Context*>(stream->data);
    }
};

#endif //NP_HW1_CONTEXT_H
