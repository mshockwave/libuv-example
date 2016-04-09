#include "Handlers.h"

namespace handlers {

    DECL_HANDLER(gLSHandler)
    DECL_HANDLER(gCDHandler)
    DECL_HANDLER(gGETHandler)
    DECL_HANDLER(gPUTHandler)

    void InitHandlers(){
        gLSHandler = HANDLER_FUNC(){

        };

        gCDHandler = HANDLER_FUNC(){
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
            WriteRequest::TransferData(write_req, builder.GetBufferPointer(), builder.GetSize());

            int ret = uv_write(write_req, stream,
                               WriteRequest::GetWriteBuffer(write_req), 1,
                               Context::OnWriteFinish);
            if(ret != 0){
                fprintf(stderr, "Error writing response in CD handler\n");
            }
        };
    }
}
