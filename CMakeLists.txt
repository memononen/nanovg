cmake_minimum_required(VERSION 3.1.0)

set(PROJECT_NAME nanovg)

project(${PROJECT_NAME})

set(SRC_DIR "./src")
file(GLOB SRCS ${SRC_DIR}/*.c)
file(GLOB HDRS ${SRC_DIR}/*.h)

add_library(${PROJECT_NAME} ${SRCS})


install(TARGETS ${PROJECT_NAME}
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        RUNTIME DESTINATION bin
        INCLUDES DESTINATION include/${PROJECT_NAME}
)

install(FILES ${HDRS} DESTINATION include/${PROJECT_NAME}/)