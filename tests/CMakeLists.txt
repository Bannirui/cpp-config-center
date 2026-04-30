file(GLOB_RECURSE INTEGRATION_TEST_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)

foreach(TEST_SRC ${INTEGRATION_TEST_SOURCES})
    get_filename_component(TEST_NAME ${TEST_SRC} NAME_WE)
    add_executable(${TEST_NAME} ${TEST_SRC})
    target_link_libraries(${TEST_NAME} PRIVATE
        config-center-core
        config-center-sdk
        GTest::GTest
        GTest::Main
    )
    add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME})
endforeach()
