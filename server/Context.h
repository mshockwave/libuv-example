
#ifndef NP_HW1_CONTEXT_H
#define NP_HW1_CONTEXT_H

#include <string>

extern "C" {
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>
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

    uv_buf_t read_buffer;

    Context(const char* client_address_, int port) :
            pwd("Upload"),
            client_address(client_address_),
            client_port(port){
        //Check default upload folder
        auto pwd_str = pwd.c_str();
        DIR *dp;
        if( (dp = opendir(pwd_str)) == NULL ){
            //Not exist
            if(mkdir(pwd_str, S_IRWXU | S_IRWXG | S_IRWXO) < 0){
                fprintf(stderr, "Fail creating folder: %s\n", pwd_str);
            }
        }else{
            closedir(dp);
        }

        //Init buffer
        read_buffer.base = NULL;
        read_buffer.len = 0;
    }

    bool set_current_dir(const char* path){
        DIR *dp;
        if( (dp = opendir(path)) != NULL ){
            pwd = path;
            closedir(dp);
            return true;
        }else{
            return false;
        }
    }
    const char* get_current_dir(){ return pwd.c_str(); }

    static void OnWriteFinish(uv_write_t *req, int status){
        if(status < 0){
            fprintf(stderr, "Error raised in OnWriteFinish: %s\n", uv_strerror(status));
        }

        WriteRequest::Destroy(req);
    }

    static Context* GetContext(uv_stream_t* stream){
        if(stream == NULL || stream->data == NULL) return nullptr;
        return reinterpret_cast<Context*>(stream->data);
    }
};

#endif //NP_HW1_CONTEXT_H
