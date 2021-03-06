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

            char recv_buffer[FILE_CHUNK_SIZE];
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

            char recv_buffer[FILE_CHUNK_SIZE];
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

            bool is_eof = false;
            int fd = -1;

            while(!is_eof){
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

                char tmp_recv_buffer[RECV_BUF_SIZE];
                char *recv_buffer = NULL;
                size_t recv_buffer_size = 0;
                ssize_t ret;
                while( (ret = read(socketFd, (void*)tmp_recv_buffer, sizeof(char) * RECV_BUF_SIZE)) != 0 ){
                    if(ret < 0){
                        fprintf(stderr, "Read response from server error\n");
                        perror("Read socket");
                        return;
                    }

                    //Save content
                    recv_buffer = (char*)realloc(recv_buffer, recv_buffer_size + (size_t)ret);
                    memcpy(recv_buffer + recv_buffer_size, tmp_recv_buffer, (size_t)ret);
                    recv_buffer_size += (size_t)ret;

                    //Verify and break
                    flatbuffers::Verifier verifier((const uint8_t*)recv_buffer, recv_buffer_size);
                    if(msg::VerifyResponseBuffer(verifier)) break;
                }

                auto resp = msg::GetResponse(reinterpret_cast<const void*>(recv_buffer));
                if(resp->status_code() == msg::Status_ERROR){
                    fprintf(stderr, "Response error:\n");
                    if(resp->extra_content_type() == msg::Payload_StringPayload){
                        auto str_resp = CAST_2_STRING_PAYLOAD(resp->extra_content());
                        fprintf(stderr, "%s\n", STRING_PAYLOAD_2_STR(str_resp));
                    }
                    return;
                }else if(resp->extra_content_type() == msg::Payload_BinaryPayload){
                    std::string file_path(DOWNLOAD_DIR);
                    file_path += "/";
                    file_path += arg;

                    if(fd < 0){
                        //Not opened
                        fd = open(file_path.c_str(),
                                  O_CREAT | O_RDWR,
                                  S_IRWXU | S_IRWXG | S_IRWXO);
                        if(fd < 0){
                            fprintf(stderr, "Error opening %s\n", file_path.c_str());
                            if(recv_buffer != NULL) free(recv_buffer);
                            return;
                        }
                    }

                    auto bin_content = static_cast<const msg::BinaryPayload*>(resp->extra_content())->content();

                    if(write(fd, bin_content->data(), bin_content->size()) < 0){
                        fprintf(stderr, "Error writing to %s\n", file_path.c_str());
                        perror("Write file:");
                    }
                }else if(resp->extra_content_type() == msg::Payload_EofPayload){
                    is_eof = true;
                    if(fd >= 0) close(fd);
                    fprintf(stdout, "OK\n");
                }else{
                    fprintf(stderr, "Wrong payload format\n");
                }

                if(recv_buffer != NULL) free(recv_buffer);
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

            char file_read_buffer[FILE_CHUNK_SIZE];
            ssize_t size_read;
            while( (size_read = read(fd, (void*)file_read_buffer, sizeof(char) * FILE_CHUNK_SIZE)) > 0){
                flatbuffers::FlatBufferBuilder builder;
                auto file_bin_content = builder.CreateVector((int8_t*)file_read_buffer, (size_t)size_read);
                auto payload = msg::CreatePairPayload(builder,
                                                      builder.CreateString(arg), file_bin_content);
                auto req = msg::CreateRequest(builder,
                                              msg::Cmd_PUT,
                                              msg::Payload_PairPayload,
                                              payload.Union());
                msg::FinishRequestBuffer(builder, req);

                if(write(socketFd, builder.GetBufferPointer(), builder.GetSize()) < 0){
                    fprintf(stderr, "Request to server error\n");
                    close(fd);
                    return;
                }

                char recv_buffer[FILE_CHUNK_SIZE << 1]; //Only small amount of data, use static buffer
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
                }
            }
            if(size_read < 0){
                //Error
                fprintf(stderr, "Read file error\n");
                close(fd);
            }else{
                //EOF
                //Construct EOF message
                flatbuffers::FlatBufferBuilder builder;
                auto payload = msg::CreateEofPayload(builder, 1);
                auto req = msg::CreateRequest(builder,
                                              msg::Cmd_PUT,
                                              msg::Payload_EofPayload,
                                              payload.Union());
                msg::FinishRequestBuffer(builder, req);

                if(write(socketFd, builder.GetBufferPointer(), builder.GetSize()) < 0){
                    fprintf(stderr, "Request to server error\n");
                    close(fd);
                    return;
                }
            }
            close(fd);

            char recv_buffer[FILE_CHUNK_SIZE << 1]; //Only small amount of data, use static size buffer
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
