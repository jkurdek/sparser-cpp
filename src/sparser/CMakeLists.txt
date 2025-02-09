file(GLOB SRC_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")
add_library(SparserCpp ${SRC_FILES})
target_include_directories(SparserCpp PUBLIC ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(SparserCpp PUBLIC 
    rapidjson
    simdjson::simdjson
)
