add_executable(sparser_test sparser_test.cpp)

target_link_libraries(sparser_test PRIVATE GTest::gtest_main SparserCpp)

include(GoogleTest)
gtest_discover_tests(sparser_test)
