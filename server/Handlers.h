#ifndef NP_HW1_HANDLERS_H
#define NP_HW1_HANDLERS_H

#include <functional>

#include <Common.h>
#include "Context.h"

extern "C"{
#include <unistd.h>
};

typedef std::function<void(uv_stream_t*, ssize_t, const uv_buf_t *)> read_handler_t;

#define HANDLER_FUNC() \
    [](uv_stream_t *stream, ssize_t nread, const uv_buf_t *buf) -> void

#define DECL_EXT_HANDLER(H) \
    extern read_handler_t H ;

#define DECL_HANDLER(H) \
    read_handler_t H ;

namespace handlers {

    extern void InitHandlers();

    DECL_EXT_HANDLER(LSHandler)
    DECL_EXT_HANDLER(CDHandler)
    DECL_EXT_HANDLER(GETHandler)
    DECL_EXT_HANDLER(PUTHandler)

    inline void ResponseError(uv_stream_t* stream, const char* msg){
        flatbuffers::FlatBufferBuilder builder;

        auto str = builder.CreateString(msg);
        auto resp = msg::CreateResponse(builder,
                                        msg::Status_ERROR,
                                        msg::Payload_StringPayload,
                                        msg::CreateStringPayload(builder, str).Union());
        msg::FinishResponseBuffer(builder, resp);

        auto write_req = WriteRequest::New();
        WriteRequest::TransferWqData(write_req, builder.GetBufferPointer(), builder.GetSize());

        int ret = uv_write(write_req, stream,
                           WriteRequest::GetWriteBuffer(write_req), 1,
                           Context::OnWriteFinish);
        if(ret != 0){
            fprintf(stderr, "Error writing error response\n");
        }
    }
    inline void ResponseOk(uv_stream_t* stream){
        flatbuffers::FlatBufferBuilder builder;
        auto resp = msg::CreateResponse(builder, msg::Status_OK);
        msg::FinishResponseBuffer(builder, resp);

        auto write_req = WriteRequest::New();
        WriteRequest::TransferWqData(write_req, builder.GetBufferPointer(), builder.GetSize());

        int ret = uv_write(write_req, stream,
                           WriteRequest::GetWriteBuffer(write_req), 1,
                           Context::OnWriteFinish);
        if(ret != 0){
            fprintf(stderr, "Error writing ok response handler\n");
        }
    }
}

#endif //NP_HW1_HANDLERS_H
