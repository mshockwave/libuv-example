#include "CmdHandlers.h"

namespace handlers{
    DECL_CMD_HANDLER(HandleCD)
    DECL_CMD_HANDLER(HandleLS)
    DECL_CMD_HANDLER(HandleGET)
    DECL_CMD_HANDLER(HandlePUT)

    void InitCmdHandlers(){
        HandleCD = HANDLER_FUNC(){
            char arg[strlen(arg_) + 1];
            strcpy(arg, arg_);
            trim(arg);

            flatbuffers::FlatBufferBuilder builder;

            auto folder_str = msg::CreateStringPayload(builder, builder.CreateString(arg));
            auto req = msg::CreateRequest(builder,
                                          msg::Cmd_CD,
                                          msg::Payload_StringPayload,
                                          folder_str.Union());
            msg::FinishRequestBuffer(builder, req);

            auto req_bin = builder.GetBufferPointer();
            auto req_size = builder.GetSize();

            if(write(socketFd, (const void*)req_bin, req_size) < 0){
                fprintf(stderr, "Request to server error\n");
                return;
            }

            char recv_buffer[TRANS_BUF_SIZE];
            if(read(socketFd, (void*)recv_buffer, sizeof(recv_buffer)) < 0){
                fprintf(stderr, "Read response from server error\n");
                return;
            }

            auto resp = msg::GetResponse(reinterpret_cast<const void*>(recv_buffer));
            if(resp->status_code() == msg::Status_ERROR){
                fprintf(stderr, "Response error:\n");
                if(resp->extra_content_type() == msg::Payload_StringPayload){
                    auto str_resp = CAST_2_STRING_PAYLOAD(resp->extra_content());
                    fprintf(stderr, "%s\n", STRING_PAYLOAD_2_STR(str_resp));
                }
            }else{
                fprintf(stdout, "OK\n");
            }
        };
    }
}