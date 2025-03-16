cmake_minimum_required(VERSION 3.20)

project(heartbeat)


set(RESP "TCP" CACHE STRING "RESP")
set(CLOSING_DELAY ON CACHE BOOL "CLOSING_DELAY")

message(STATUS "RESP: ${RESP}")
message(STATUS "CLOSING_DELAY: ${CLOSING_DELAY}")

if("${RESP}" STREQUAL "TCP")
    add_definitions(-DRESP_TCP)
elseif("${RESP}" STREQUAL "HTTP")
    add_definitions(-DRESP_HTTP)
else()
    message(FATAL_ERROR "Invalid -DRESP value: ${RESP}")
endif()

if(CLOSING_DELAY)
    add_definitions(-DNO_CLOSING_DELAY)
endif()


add_executable(${PROJECT_NAME} heartbeat.cpp)

if(WIN32)
    target_link_libraries(${PROJECT_NAME} ws2_32)
endif()
