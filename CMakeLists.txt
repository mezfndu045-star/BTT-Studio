cmake_minimum_required(VERSION 3.16)

set(CMAKE_TOOLCHAIN_FILE "${CMAKE_SOURCE_DIR}/sys/cmake/toolchain.cmake")
project(BTT-Studio VERSION 1.0 LANGUAGES ASM C CXX)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(config/config.cmake)

file(GLOB_RECURSE SOURCES_ASM ${PROJECT_SOURCE_DIR}/src/*.S)
file(GLOB_RECURSE SOURCES_C ${PROJECT_SOURCE_DIR}/src/*.c)
file(GLOB_RECURSE SOURCES_CXX ${PROJECT_SOURCE_DIR}/src/*.cpp)
set(SOURCES ${SOURCES_ASM} ${SOURCES_C} ${SOURCES_CXX})
add_executable(${PROJECT_NAME} ${SOURCES})

target_include_directories(${PROJECT_NAME} PRIVATE lib)
target_include_directories(${PROJECT_NAME} PRIVATE lib/imgui)
target_include_directories(${PROJECT_NAME} PRIVATE lib/custom)
target_include_directories(${PROJECT_NAME} PRIVATE lib/OdysseyHeaders)
target_include_directories(${PROJECT_NAME} PRIVATE .)

file(GLOB LIBS_OH LIST_DIRECTORIES true lib/OdysseyHeaders/*)
file(GLOB LIBS_CUST LIST_DIRECTORIES true lib/custom/*)
target_include_directories(${PROJECT_NAME} PRIVATE ${LIBS_OH} ${LIBS_CUST})

include(sys/cmake/module.cmake)
include(sys/cmake/deploy.cmake)
include(sys/cmake/sail.cmake)
usesail(${PROJECT_NAME})

if(DEBUG)
    target_compile_definitions(${PROJECT_NAME} PRIVATE BTTDEBUG)
endif()

# Practice mod BTT 1.3: build for Super Mario Odyssey 1.3.0, without the options not ported to 1.3 yet
# Developer test channel (sd:/BTT-Studio/dbg_cmd.txt, src/menu/DebugCommands.cpp): cmake -DBTT_DEBUG_COMMANDS=ON
if(BTT_DEBUG_COMMANDS)
    target_compile_definitions(${PROJECT_NAME} PRIVATE BTT_DEBUG_COMMANDS=1)
endif()
if(BTT_V130)
    target_compile_definitions(${PROJECT_NAME} PRIVATE BTT_V130=1)
endif()
