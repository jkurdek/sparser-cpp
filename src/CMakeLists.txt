set(SOURCES main.cpp)

add_executable(SparserCpp ${SOURCES})

target_include_directories(SparserCpp PRIVATE ${RAPIDJSON_INCLUDE_DIRS})
