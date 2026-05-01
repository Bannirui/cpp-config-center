file(GLOB_RECURSE SDK_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)

add_library(${PROJECT_NAME}-sdk STATIC ${SDK_SOURCES})
target_include_directories(${PROJECT_NAME}-sdk PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include # 自己的头文件
)
target_link_libraries(${PROJECT_NAME}-sdk PUBLIC
        ${HEADER_ONLY_NAME} # 还要依赖项目的头文件
)
