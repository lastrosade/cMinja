cmake_minimum_required(VERSION 3.10)
project(cMinja VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

include_directories(
    ${PROJECT_SOURCE_DIR}/include/nlohmann
    ${PROJECT_SOURCE_DIR}/include/lightweight-yaml-parser
    ${PROJECT_SOURCE_DIR}/include/minja
)

add_executable(cminja src/main.cpp)

install(TARGETS cminja DESTINATION bin)
