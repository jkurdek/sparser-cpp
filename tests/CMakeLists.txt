add_executable(sparser_test sparser_test.cpp)

target_link_libraries(sparser_test PRIVATE GTest::gtest_main SparserCpp)
target_include_directories(sparser_test PRIVATE SparserCpp ${CMAKE_SOURCE_DIR}/include)


include(GoogleTest)
gtest_discover_tests(sparser_test)
