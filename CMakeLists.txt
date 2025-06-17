cmake_minimum_required(VERSION 3.10)
project(wagle)

set(CMAKE_CXX_STANDARD 17)
find_package(Boost REQUIRED COMPONENTS system)
find_package(Curses REQUIRED)
include_directories(/usr/include/ncursesw)

# 한글 지원 설정
add_definitions(-D_XOPEN_SOURCE_EXTENDED -DNCURSES_WIDECHAR=1)

# 서버 실행 파일
add_executable(wagle_server
    src/server/server_main.cpp
    src/server/socket_manager.cpp
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
target_link_libraries(wagle_server PRIVATE Boost::system pthread ncursesw)
target_link_libraries(wagle_client PRIVATE Boost::system pthread ncursesw)

# Clean 타겟 추가
add_custom_target(clean-all
    COMMAND ${CMAKE_BUILD_TOOL} clean
    COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_BINARY_DIR}/CMakeFiles
    COMMAND ${CMAKE_COMMAND} -E remove -f ${CMAKE_BINARY_DIR}/CMakeCache.txt
    COMMAND ${CMAKE_COMMAND} -E remove -f ${CMAKE_BINARY_DIR}/cmake_install.cmake
    COMMAND ${CMAKE_COMMAND} -E remove -f ${CMAKE_BINARY_DIR}/Makefile
    COMMAND ${CMAKE_COMMAND} -E remove -f ${CMAKE_BINARY_DIR}/wagle_server
    COMMAND ${CMAKE_COMMAND} -E remove -f ${CMAKE_BINARY_DIR}/wagle_client
    COMMENT "Cleaning all build files and executables"
)