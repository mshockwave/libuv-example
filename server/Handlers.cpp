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
            msg::ResponseBuilder resp_builder(builder);

            if(request->payload_type() == msg::Payload_StringPayload){
                auto raw_payload = static_cast<const msg::StringPayload*>(request->payload());
                auto path = raw_payload->content();

                const char* pathStr = path->data();
                if(Context::GetContext(stream)->set_current_dir(pathStr)){
                    resp_builder.add_status_code(msg::Status_OK);
                }else{
                    //Path Error
                    resp_builder.add_status_code(msg::Status_ERROR);
                    auto str = builder.CreateString("Invalid Path");
                    resp_builder.add_extra_content(msg::CreateStringPayload(builder, str).Union());
                }
            }else{
                //Payload Format Error
                resp_builder.add_status_code(msg::Status_ERROR);
                auto str = builder.CreateString("Invalid payload format");
                resp_builder.add_extra_content(msg::CreateStringPayload(builder, str).Union());
            }

            auto resp = resp_builder.Finish();
            builder.Finish(resp);

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
