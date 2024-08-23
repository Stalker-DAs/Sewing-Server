#指定CMake 的最低版本要求
cmake_minimum_required(VERSION 3.2)

#设置项目名称
project(server)

#Makefile 中包含详细的编译命令，可以帮助调试和了解编译过程中的细节。
set(CMAKE_VERBOSE_MAKEFILE ON)

include_directories(.)
include_directories(/usr/local/lib)

set(CMAKE_CXX_STANDARD 11)

set(LIB_SRC
    server/address.cpp
    server/bytearray.cpp
    server/config.cpp
    server/fd_manager.cpp
    server/fiber.cpp
    server/hook.cpp
    server/iomanager.cpp
    server/log.cpp
    server/mutex.cpp
    server/scheduler.cpp
    server/socket_stream.cpp
    server/socket.cpp
    server/stream.cpp
    server/tcp_server.cpp
    server/thread.cpp
    server/timer.cpp
    server/uri.cpp
    server/util.cpp
    server/http/http.cpp
    server/http/http_connection.cpp
    server/http/http_parser.cpp
    server/http/http_session.cpp
    server/http/http_server.cpp
    server/http/servlet.cpp
    server/http/http11_parser.rl.cpp
    server/http/httpclient_parser.rl.cpp
)

# 创建共享库 my_server
add_library(my_server SHARED ${LIB_SRC})

# 设置要链接的库
set(LIBS
    my_server
    dl
    pthread
    /usr/local/lib/libyaml-cpp.a
)

# 创建可执行文件 test_http_server，并链接库
add_executable(test_http_server tests/test_http_server.cpp)
target_link_libraries(test_http_server ${LIBS})

#设置输出路径
SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
#设置共享库路径
SET(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)


