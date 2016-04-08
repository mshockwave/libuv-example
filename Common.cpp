#include <cstdlib>
#include "Common.h"

int get_port(socket_addr_t *sa){
    if(sa->sa_family == AF_INET){
        struct sockaddr_in *sa_info = SOCK_ADDR_IN(sa);

        return ntohs(sa_info->sin_port);
    }else{
        struct sockaddr_in6 *sa_info = SOCK_ADDR_IN6(sa);

        return ntohs(sa_info->sin6_port);
    }
}

std::string get_address(socket_addr_t *sa){
    if(sa->sa_family == AF_INET){
        struct sockaddr_in *sa_info = SOCK_ADDR_IN(sa);

        char out_buffer[201];
        uv_ip4_name(sa_info, out_buffer, sizeof(char) * 201);

        return std::string(out_buffer);
    }else{
        struct sockaddr_in6 *sa_info = SOCK_ADDR_IN6(sa);

        char out_buffer[201];
        uv_ip6_name(sa_info, out_buffer, sizeof(char) * 201);

        return std::string(out_buffer);
    }
}

void BufferAllocator(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf){
    //TODO: Improve
    buf->len = suggested_size / sizeof(char);
    buf->base = new char[buf->len];
}
