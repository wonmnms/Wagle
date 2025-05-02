cmake_minimum_required(VERSION 3.10)
project(wagle)

set(CMAKE_CXX_STANDARD 17)
find_package(Boost REQUIRED COMPONENTS system)

# 서버 실행 파일
add_executable(wagle_server
    src/server/server_main.cpp
    src/common/message.cpp
    src/common/chat_room.cpp
)

# 클라이언트 실행 파일
add_executable(wagle_client
    src/client/client_main.cpp
    src/common/message.cpp
)

# 헤더 파일 경로 추가
include_directories(include)

# 라이브러리 링크
target_link_libraries(wagle_server PRIVATE Boost::system pthread)
target_link_libraries(wagle_client PRIVATE Boost::system pthread)