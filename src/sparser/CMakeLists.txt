add_library(SparserCpp sparser.cpp json_facade.cpp)
target_include_directories(SparserCpp PUBLIC ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(SparserCpp PRIVATE rapidjson)
