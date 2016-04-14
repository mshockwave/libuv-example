#include <uv/uv.h>
#include "Handlers.h"

namespace handlers {

    DECL_HANDLER(LSHandler)
    DECL_HANDLER(CDHandler)
    DECL_HANDLER(GETHandler)
    DECL_HANDLER(PUTHandler)

    void OnFileRead(uv_fs_t* req){
        auto read_req = FsRequest::GetFsRequest(req);
        auto socket_write_req = WriteRequest::New();
        if(req->result > 0){
            auto result_buf = read_req->write_buffer;

            flatbuffers::FlatBufferBuilder builder;

            auto buf_vector = builder.CreateVector((int8_t*)result_buf.base, result_buf.len);
            auto bin_payload = msg::CreateBinaryPayload(builder, buf_vector);
            auto resp = msg::CreateResponse(builder,
                                            msg::Status_OK,
                                            msg::Payload_BinaryPayload,
                                            bin_payload.Union());
            msg::FinishResponseBuffer(builder, resp);

            FsRequest::TransferWqData(socket_write_req, builder.GetBufferPointer(), builder.GetSize());
            int ret = uv_write(socket_write_req, read_req->socket_stream,
                               WriteRequest::GetWriteBuffer(socket_write_req), 1,
                               Context::OnWriteFinish);
            if(ret != 0){
                fprintf(stderr, "Error writing response in OnFileRead\n");
            }
        }else if(req->result == UV_EOF || req->result == 0){

            flatbuffers::FlatBufferBuilder builder;
            auto eof_payload = msg::CreateEofPayload(builder, 1);
            auto resp = msg::CreateResponse(builder,
                                            msg::Status_OK,
                                            msg::Payload_EofPayload,
                                            eof_payload.Union());
            msg::FinishResponseBuffer(builder, resp);
            FsRequest::TransferWqData(socket_write_req, builder.GetBufferPointer(), builder.GetSize());
            int ret = uv_write(socket_write_req, read_req->socket_stream,
                               WriteRequest::GetWriteBuffer(socket_write_req), 1,
                               Context::OnWriteFinish);
            if(ret != 0){
                fprintf(stderr, "Error writing response in OnFileRead\n");
            }

            auto ctx = Context::GetContext(read_req->socket_stream);
            ctx->pending_fd = -1;

            //Close
            uv_fs_t close_req;
            //Since we want blocking close
            //close_req doesn't need to be preserved through heap
            uv_fs_close(req->loop,
                        &close_req,
                        (uv_file)FsRequest::GetFsRequest(req)->open_fd,
                        NULL /*sync*/);

            uv_fs_req_cleanup(req);
            FsRequest::Destroy(req);
        }else{
            ResponseError(read_req->socket_stream, "Read File Error");
        }
    }
    void OnReadFileOpen(uv_fs_t *open_req){
        if(open_req->result >= 0){
            auto read_req = FsRequest::New(FsRequest::GetFsRequest(open_req)->socket_stream);
            FsRequest::GetFsRequest(read_req)->open_fd = open_req->result;

            //Get file size
            uv_fs_t size_req;
            uv_fs_fstat(open_req->loop, &size_req, (uv_file)open_req->result, NULL);
            auto file_size = size_req.statbuf.st_size;
            FsRequest::AllocateBuffer(read_req, file_size);
            uv_fs_req_cleanup(&size_req);

            uv_fs_read(open_req->loop, read_req,
                       (uv_file)open_req->result,
                       FsRequest::GetWriteBuffer(read_req), 1, 0, OnFileRead);
        }else{
            flatbuffers::FlatBufferBuilder builder;

            auto str = builder.CreateString("Open File Error");
            auto resp = msg::CreateResponse(builder,
                                            msg::Status_ERROR,
                                            msg::Payload_StringPayload,
                                            msg::CreateStringPayload(builder, str).Union());
            msg::FinishResponseBuffer(builder, resp);

            auto write_req = WriteRequest::New();
            WriteRequest::TransferWqData(write_req, builder.GetBufferPointer(), builder.GetSize());

            int ret = uv_write(write_req, FsRequest::GetFsRequest(open_req)->socket_stream,
                               WriteRequest::GetWriteBuffer(write_req), 1,
                               Context::OnWriteFinish);
            if(ret != 0){
                fprintf(stderr, "Error writing response in OnReadFileOpen handler\n");
            }
        }

        uv_fs_req_cleanup(open_req);
        FsRequest::Destroy(open_req);
    }
    /* FIXME
    void OnFileWrite(uv_fs_t* req){
        uv_stream_t* stream = FsRequest::GetFsRequest(req)->socket_stream;
        if(req->result > 0){
            ResponseOk(stream);
        }else{
            ResponseError(stream, "Write file Error");
        }

        uv_fs_req_cleanup(req);
        FsRequest::Destroy(req);
    }
     */

    void InitHandlers(){
        LSHandler = HANDLER_FUNC(){
            //auto request = msg::GetRequest(buf->base);

            flatbuffers::FlatBufferBuilder builder;
            flatbuffers::Offset<msg::Response> resp;

            DIR* dp;
            struct dirent *ep;
            std::string ret_str("");

            auto ctx = Context::GetContext(stream);
            if( (dp = opendir(ctx->get_current_dir())) != NULL){
                while( (ep = readdir(dp)) != NULL ){
                    ret_str += (ep->d_name);
                    ret_str += ";"; //Column separated
                }
                closedir(dp);

                auto str = builder.CreateString(ret_str);
                resp = msg::CreateResponse(builder,
                                           msg::Status_OK,
                                           msg::Payload_StringPayload,
                                           msg::CreateStringPayload(builder, str).Union());
            }else{
                //Error
                auto error_str = builder.CreateString("Error opening dir");
                resp = msg::CreateResponse(builder,
                                           msg::Status_ERROR,
                                           msg::Payload_StringPayload,
                                           msg::CreateStringPayload(builder, error_str).Union());
            }
            msg::FinishResponseBuffer(builder, resp);

            auto write_req = WriteRequest::New();
            WriteRequest::TransferWqData(write_req, builder.GetBufferPointer(), builder.GetSize());

            int ret = uv_write(write_req, stream,
                               WriteRequest::GetWriteBuffer(write_req), 1,
                               Context::OnWriteFinish);
            if(ret != 0){
                fprintf(stderr, "Error writing response in LS handler\n");
            }
        };

        CDHandler = HANDLER_FUNC(){
            auto request = msg::GetRequest(buf->base);

            flatbuffers::FlatBufferBuilder builder;
            flatbuffers::Offset<msg::Response> resp;

            if(request->payload_type() == msg::Payload_StringPayload){
                auto raw_payload = CAST_2_STRING_PAYLOAD(request->payload());
                auto path = raw_payload->content();

                const char* pathStr = path->data();
                if(Context::GetContext(stream)->set_current_dir(pathStr)){
                    resp = msg::CreateResponse(builder, msg::Status_OK);
                }else{
                    //Path Error
                    auto str = builder.CreateString("Invalid Path");
                    resp = msg::CreateResponse(builder,
                                               msg::Status_ERROR,
                                               msg::Payload_StringPayload,
                                               msg::CreateStringPayload(builder, str).Union());
                }
            }else{
                //Payload Format Error
                auto str = builder.CreateString("Invalid payload format");
                resp = msg::CreateResponse(builder,
                                           msg::Status_ERROR,
                                           msg::Payload_StringPayload,
                                           msg::CreateStringPayload(builder, str).Union());
            }
            msg::FinishResponseBuffer(builder, resp);

            auto write_req = WriteRequest::New();
            WriteRequest::TransferWqData(write_req, builder.GetBufferPointer(), builder.GetSize());

            int ret = uv_write(write_req, stream,
                               WriteRequest::GetWriteBuffer(write_req), 1,
                               Context::OnWriteFinish);
            if(ret != 0){
                fprintf(stderr, "Error writing response in CD handler\n");
            }
        };

        GETHandler = HANDLER_FUNC(){
            auto request = msg::GetRequest(buf->base);
            auto ctx = Context::GetContext(stream);

            if(request->payload_type() == msg::Payload_StringPayload){
                auto raw_payload = CAST_2_STRING_PAYLOAD(request->payload());
                auto file_path = STRING_PAYLOAD_2_STR(raw_payload);
                std::string full_path(ctx->get_current_dir());
                full_path += "/";
                full_path += file_path;

                ssize_t fd = ctx->pending_fd;

                if(fd < 0){
                    //Not opened
                    uv_fs_t open_req;
                    uv_fs_open(stream->loop,
                               &open_req, full_path.c_str(),
                               O_CREAT | O_RDWR,
                               S_IRWXU | S_IRWXG | S_IRWXO, NULL/*sync*/);
                    fd = open_req.result;
                    uv_fs_req_cleanup(&open_req);
                    ctx->pending_fd = fd;
                }

                auto read_req = FsRequest::New(stream);
                FsRequest::GetFsRequest(read_req)->open_fd = fd;
                FsRequest::AllocateBuffer(read_req, FILE_CHUNK_SIZE);

                uv_fs_read(stream->loop, read_req,
                           (uv_file)fd,
                           FsRequest::GetWriteBuffer(read_req), 1, 0, OnFileRead);
            }else{
                //Payload Format Error
                ResponseError(stream, "Invalid Payload Format");
            }
        };

        PUTHandler = HANDLER_FUNC(){
            auto request = msg::GetRequest(buf->base);
            auto ctx = Context::GetContext(stream);

            if(request->payload_type() == msg::Payload_EofPayload){
                ssize_t fd = ctx->pending_fd;

                //End of transmission

                //Close file
                uv_fs_t close_req;
                //Since we want blocking close
                //close_req doesn't need to be preserved through heap
                uv_fs_close(stream->loop,
                            &close_req,
                            (uv_file)fd,
                            NULL /*sync*/);
                uv_fs_req_cleanup(&close_req);
                ctx->pending_fd = -1; //Clear

                ResponseOk(stream);

            }else if(request->payload_type() == msg::Payload_PairPayload){
                auto pair_payload = static_cast<const msg::PairPayload*>(request->payload());
                auto file_name = pair_payload->name()->data();
                auto file_data = pair_payload->content()->data();
                auto file_size = pair_payload->content()->size();

                ssize_t fd = ctx->pending_fd;

                std::string full_file_path(ctx->get_current_dir());
                full_file_path += "/";
                full_file_path += file_name;

                if(fd < 0){
                    //File hasn't been opened
                    uv_fs_t open_req;
                    uv_fs_open(stream->loop,
                               &open_req, full_file_path.c_str(),
                               O_CREAT | O_RDWR,
                               S_IRWXU | S_IRWXG | S_IRWXO, NULL/*sync*/);
                    fd = open_req.result;
                    uv_fs_req_cleanup(&open_req);
                    ctx->pending_fd = fd;
                }

                /*FIXME
                auto write_req = FsRequest::New(stream);
                FsRequest::TransferFsData(write_req, (void*)file_data, file_size);
                FsRequest::GetFsRequest(write_req)->socket_stream = stream;
                uv_fs_write(stream->loop, write_req, (uv_file)fd,
                            FsRequest::GetWriteBuffer(write_req), 1, 0, OnFileWrite);
                            */
                write((int)fd, (void*)file_data, file_size);

                ResponseOk(stream);
            }else{
                //Payload Format Error
                ResponseError(stream, "Invalid Payload Format");
            }
        };
    }
}
