cmake_minimum_required(VERSION 3.2)

project(sqlite_adapter VERSION 1.0.0)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O0 -std=c++17 -pthread")

add_compile_options(-Werror=sign-compare)
add_compile_options(-Werror=unused-variable)
add_compile_options(-Wuninitialized)
add_compile_options(-Wformat)
add_compile_options(-Wno-deprecated-declarations)


set(BASE_PATH ${CMAKE_CURRENT_SOURCE_DIR})
#set(BASE_PATH ${PROJECT_SOURCE_DIR})


message("build sqlite begin")

# build sqlite
add_subdirectory(third_party/sqlite)
message("build sqlite end")


set(SQLITE_ADAPTER_PATH ${CMAKE_CURRENT_SOURCE_DIR})

include_directories(
    ${SQLITE_ADAPTER_PATH}/include
)

include_directories(
    ${BASE_PATH}/third_party/sqlite
    ${BASE_PATH}/third_party/sqlite/build/third_party/huawei_secure_c/include
)


set(SOURCE_FILES
    ${SQLITE_ADAPTER_PATH}/src/tokenizer_api_mgr.cpp
    ${SQLITE_ADAPTER_PATH}/src/tokenizer_api.cpp
    ${SQLITE_ADAPTER_PATH}/src/tokenizer_sqlite.cpp
)

add_library(customtokenizer SHARED ${SOURCE_FILES})

target_link_libraries(customtokenizer PUBLIC sqlite3 securec)

target_include_directories(customtokenizer PUBLIC include)

install(TARGETS customtokenizer LIBRARY DESTINATION lib)
