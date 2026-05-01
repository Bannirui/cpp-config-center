file(GLOB_RECURSE INTEGRATION_TEST_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)

foreach (TEST_SRC ${INTEGRATION_TEST_SOURCES})
    # 文件名 不带后缀
    get_filename_component(TEST_NAME ${TEST_SRC} NAME_WE)
    add_executable(${TEST_NAME} ${TEST_SRC})
    target_link_libraries(${TEST_NAME} PRIVATE
            ${HEADER_ONLY_NAME} # 项目的头文件
            ${PROJECT_NAME}-sdk # sdk的头文件
            gtest
            gtest_main
    )
    add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME})
endforeach ()
