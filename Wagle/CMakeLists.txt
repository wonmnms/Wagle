cmake_minimum_required(VERSION 3.10)
project(wagle)

set(CMAKE_CXX_STANDARD 17)

# Boost 라이브러리 찾기
find_package(Boost REQUIRED COMPONENTS system)

# ncurses 찾기 (Ubuntu)
find_package(Curses REQUIRED)
include_directories(/usr/include/ncursesw)

# 한글 지원 설정
add_definitions(-D_XOPEN_SOURCE_EXTENDED -DNCURSES_WIDECHAR=1)

# 매크로 충돌 방지를 위한 정의
add_definitions(-DBOOST_ASIO_NO_DEPRECATED)
add_definitions(-DNCURSES_INTERNALS)

# 공통 소스 파일
set(COMMON_SOURCES
    src/common/message.cpp
    src/common/chat_room.cpp
    src/common/chat_room_manager.cpp
)

# 서버 실행 파일
add_executable(wagle_server
    src/server/server_main.cpp
    src/server/socket_manager.cpp
    ${COMMON_SOURCES}
)

# 클라이언트 실행 파일
add_executable(wagle_client
    src/client/client_main.cpp
    ${COMMON_SOURCES}
)

# 헤더 파일 경로 추가
include_directories(include)

# 라이브러리 링크
target_link_libraries(wagle_server PRIVATE 
    Boost::system 
    pthread 
    ncursesw
)

target_link_libraries(wagle_client PRIVATE 
    Boost::system 
    pthread 
    ncursesw
)

# 컴파일 옵션 설정
if(MSVC)
    # Windows 컴파일러 옵션
    target_compile_options(wagle_server PRIVATE /W4)
    target_compile_options(wagle_client PRIVATE /W4)
else()
    # GCC/Clang 컴파일러 옵션
    target_compile_options(wagle_server PRIVATE -Wall -Wextra -Wpedantic)
    target_compile_options(wagle_client PRIVATE -Wall -Wextra -Wpedantic)
endif()

# 빌드 타입 설정
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# 디버그 모드 설정
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_definitions(-DDEBUG)
endif()