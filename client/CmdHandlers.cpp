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

        HandleLS = HANDLER_FUNC(){
            flatbuffers::FlatBufferBuilder builder;

            auto req = msg::CreateRequest(builder, msg::Cmd_LS);
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
            }else if(resp->extra_content_type() == msg::Payload_StringPayload){
                auto str_resp = STRING_PAYLOAD_2_STR(CAST_2_STRING_PAYLOAD(resp->extra_content()));
                char mut_str_resp[strlen(str_resp) + 1];
                strncpy(mut_str_resp, str_resp, sizeof(mut_str_resp));

                auto file_name = strtok(mut_str_resp, ";");
                while(file_name){
                    fprintf(stdout, "%s\n", file_name);
                    file_name = strtok(NULL, ";");
                }
                fprintf(stdout, "\n");
            }else{
                fprintf(stderr, "Wrong response format\n");
            }
        };

        HandleGET = HANDLER_FUNC() {
            char arg[strlen(arg_) + 1];
            strcpy(arg, arg_);
            trim(arg);

            flatbuffers::FlatBufferBuilder builder;

            auto file_str = msg::CreateStringPayload(builder, builder.CreateString(arg));
            auto req = msg::CreateRequest(builder,
                                          msg::Cmd_GET,
                                          msg::Payload_StringPayload,
                                          file_str.Union());
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
            }else if(resp->extra_content_type() == msg::Payload_BinaryPayload){
                auto bin_content = static_cast<const msg::BinaryPayload*>(resp->extra_content())->content();

                std::string file_path(DOWNLOAD_DIR);
                file_path += "/";
                file_path += arg;

                int fd = open(file_path.c_str(),
                              O_CREAT | O_RDWR,
                              S_IRWXU | S_IRWXG | S_IRWXO);
                if(fd < 0){
                    fprintf(stderr, "Error opening %s\n", file_path.c_str());
                    return;
                }
                if(write(fd, bin_content->data(), bin_content->size()) < 0){
                    fprintf(stderr, "Error writing to %s\n", file_path.c_str());
                }
            }else{
                fprintf(stderr, "Wrong payload format\n");
            }
        };

        HandlePUT = HANDLER_FUNC() {
            char arg[strlen(arg_) + 1];
            strcpy(arg, arg_);
            trim(arg);

            //Check file exist
            int fd;
            if( (fd = open(arg, O_RDONLY)) < 0){
                fprintf(stderr, "Opening %s error\n", arg);
                return;
            }

            struct stat f_stat;
            fstat(fd, &f_stat);
            auto file_size = f_stat.st_size;
            char file_buffer[file_size];
            if(read(fd, (void*)file_buffer, sizeof(file_buffer)) < 0){
                fprintf(stderr, "Reading %s error\n", arg);
                return;
            }

            flatbuffers::FlatBufferBuilder builder;
            auto file_bin_content = builder.CreateVector((int8_t*)file_buffer, sizeof(file_buffer));
            auto payload = msg::CreatePairPayload(builder,
                                                  builder.CreateString(arg), file_bin_content);
            auto req = msg::CreateRequest(builder,
                                          msg::Cmd_PUT,
                                          msg::Payload_PairPayload,
                                          payload.Union());
            msg::FinishRequestBuffer(builder, req);

            if(write(socketFd, builder.GetBufferPointer(), builder.GetSize()) < 0){
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