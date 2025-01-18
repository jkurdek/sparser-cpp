add_subdirectory(sparser)

add_executable(SparserMain main.cpp)
target_link_libraries(SparserMain PRIVATE SparserCpp)
target_link_libraries(SparserMain PRIVATE rapidjson)
