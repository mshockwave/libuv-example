
#ifndef NP_HW1_CMDHANDLERS_H
#define NP_HW1_CMDHANDLERS_H

#include <Common.h>
#include <functional>

extern "C"{
#include <unistd.h>
};

namespace handlers{
    typedef std::function<void(int,const char*)> cmd_handler_t;

#define DECL_EXT_CMD_HANDLER(H) \
    extern cmd_handler_t H ;

#define DECL_CMD_HANDLER(H) \
    cmd_handler_t H ;

#define HANDLER_FUNC() \
    [](int socketFd, const char* arg_) -> void

    inline void trim(char* str, char c){
        //Find the first non c
        int i;
        for(i = 0; i < strlen(str); i++){
            if(str[i] != c) break;
        }
        //Shift head
        int j;
        for(j = 0; j < strlen(str) - i; j++){
            str[j] = str[j + i];
        }

        //Trim tail
        for(j-- ; j >= 0; j--){
            if(str[j] != c){
                str[j + 1] = '\0';
                break;
            }
        }
    }
    inline void trim(char* str){ trim(str, ' '); }

    extern void InitCmdHandlers();

    DECL_EXT_CMD_HANDLER(HandleCD)
    DECL_EXT_CMD_HANDLER(HandleLS)
    DECL_EXT_CMD_HANDLER(HandleGET)
    DECL_EXT_CMD_HANDLER(HandlePUT)
}

#endif //NP_HW1_CMDHANDLERS_H
