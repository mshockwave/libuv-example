cmake_minimum_required(VERSION 2.8)
project(NP_HW1)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O0 -std=c++11")

include_directories(include ${CMAKE_SOURCE_DIR})

if (APPLE)
    link_directories(${CMAKE_SOURCE_DIR}/lib/darwin)
    set(LINK_LIB_NAMES libuv.a)
elseif(WIN32)
    message(FATAL_ERROR "Not support windows yet")
else()
    link_directories(${CMAKE_SOURCE_DIR}/lib/linux)
    set(LINK_LIB_NAMES libuv.a pthread)
endif ()

set(COMMON_SOURCE_FILE
        Common.cpp)

set(CLIENT_SOURCE_FILE
        ${COMMON_SOURCE_FILE}
        client/Main.cpp
        client/CmdHandlers.cpp)

set(SERVER_SOURCE_FILE
        ${COMMON_SOURCE_FILE}
        server/Main.cpp
        server/Handlers.cpp
        server/Context.cpp)

add_executable(HW1_103062222_Cli ${CLIENT_SOURCE_FILE})
target_link_libraries(HW1_103062222_Cli ${LINK_LIB_NAMES})

add_executable(HW1_103062222_Ser ${SERVER_SOURCE_FILE})
target_link_libraries(HW1_103062222_Ser ${LINK_LIB_NAMES})