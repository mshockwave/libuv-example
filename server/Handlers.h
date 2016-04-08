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

    DECL_EXT_HANDLER(gLSHandler)
    DECL_EXT_HANDLER(gCDHandler)
    DECL_EXT_HANDLER(gGETHandler)
    DECL_EXT_HANDLER(gPUTHandler)
}

#endif //NP_HW1_HANDLERS_H
