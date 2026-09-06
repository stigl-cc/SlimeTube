cmake_minimum_required(VERSION 3.16)
project(slimytube CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(CheckIncludeFileCXX)

check_include_file_cxx(any HAS_ANY)
check_include_file_cxx(string_view HAS_STRING_VIEW)
check_include_file_cxx(coroutine HAS_COROUTINE)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Set up the drogon framework
set(BUILD_ORM ON CACHE BOOL "Enable ORM" FORCE)

set(BUILD_REDIS OFF CACHE BOOL "Disable REDIS" FORCE)
set(BUILD_MYSQL OFF CACHE BOOL "Disable MYSQL" FORCE)
set(BUILD_POSTGRESQL OFF CACHE BOOL "Disable POSTGRESQL" FORCE)

set(BUILD_BROTLI OFF CACHE BOOL "Disable BROTLI" FORCE)
set(BUILD_YAML_CONFIG OFF CACHE BOOL "Disable YAML" FORCE)

set(BUILD_SHARED_LIBS OFF CACHE BOOL "Disable SHARED" FORCE)
set(BUILD_EXAMPLES OFF CACHE BOOL "Disable EXAMPLES" FORCE)
set(BUILD_CTL OFF CACHE BOOL "Disable CTL" FORCE)
set(BUILD_DOC OFF CACHE BOOL "Disable DOC" FORCE)

add_subdirectory(lib/drogon ${CMAKE_CURRENT_BINARY_DIR}/drogon SYSTEM)

# Setup the main project
add_executable(${PROJECT_NAME} main.cc)

aux_source_directory(controllers CTL_SRC)
aux_source_directory(filters FILTER_SRC)
aux_source_directory(models MODEL_SRC)

drogon_create_views(${PROJECT_NAME} ${CMAKE_CURRENT_SOURCE_DIR}/views ${CMAKE_CURRENT_BINARY_DIR}/views)

target_link_libraries(${PROJECT_NAME} PRIVATE drogon)
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/models)
target_sources(${PROJECT_NAME}
               PRIVATE
               ${SRC_DIR}
               ${CTL_SRC}
               ${FILTER_SRC}
               ${MODEL_SRC})

target_compile_options(${PROJECT_NAME} PRIVATE -ffunction-sections -fdata-sections)
target_link_options(${PROJECT_NAME} PRIVATE -Wl,--gc-sections -Wl,--as-needed)

add_subdirectory(test ${CMAKE_CURRENT_BINARY_DIR}/test)
