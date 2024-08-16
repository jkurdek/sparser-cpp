add_executable(sample_test sample_test.cpp)

target_link_libraries(sample_test PRIVATE GTest::gtest_main)
target_include_directories(sample_test PRIVATE ${rapidjson_INCLUDE_DIRS})


include(GoogleTest)
gtest_discover_tests(sample_test)
