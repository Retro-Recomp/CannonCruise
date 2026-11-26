cmake_minimum_required(VERSION 3.16)
project(CannonCruise C CXX)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# MSVC: conforming preprocessor (VS2017 15.8+)
# Traditional preprocessor breaks lambda syntax inside macros (C2059: '[')
# /Zc:preprocessor fixes this at root — RWS_REGISTER_CLASS uses lambda.
if(MSVC)
    add_compile_options(/Zc:preprocessor)
endif()

# ---------------------------------------------------------------------------
# Bullet Physics — portable physics engine instead of Havok (Phase 4.6).
# ---------------------------------------------------------------------------
include(FetchContent)
FetchContent_Declare(bullet3
    GIT_REPOSITORY https://github.com/bulletphysics/bullet3.git
    GIT_TAG        3.25
    GIT_SHALLOW    TRUE)
set(BUILD_UNIT_TESTS         OFF CACHE BOOL "" FORCE)
set(BUILD_EXTRAS             OFF CACHE BOOL "" FORCE)
set(BUILD_BULLET2_DEMOS      OFF CACHE BOOL "" FORCE)
set(BUILD_OPENGL3_DEMOS      OFF CACHE BOOL "" FORCE)
set(BUILD_CPU_DEMOS          OFF CACHE BOOL "" FORCE)
set(BUILD_PYBULLET           OFF CACHE BOOL "" FORCE)
set(BUILD_ENET               OFF CACHE BOOL "" FORCE)
set(BUILD_CLSOCKET           OFF CACHE BOOL "" FORCE)
set(USE_GLUT                 OFF CACHE BOOL "" FORCE)
set(BUILD_SHARED_LIBS        OFF CACHE BOOL "" FORCE)
set(BULLET_REQUIRE_CXX_FLAGS OFF CACHE BOOL "" FORCE)
set(CMAKE_POLICY_VERSION_MINIMUM 3.5)
FetchContent_MakeAvailable(bullet3)

# ---------------------------------------------------------------------------
# librw — RenderWare Graphics 3.x API reimplementation (MIT).
# Platform options:
#   NULL  — device-independent no-op (headless smoke test)
#   GL3   — GLFW + OpenGL 3.x (macOS/Linux build)
#   D3D9  — Direct3D 9 (Windows/MSVC build)
# ---------------------------------------------------------------------------
set(CANNONCRUISE_RW_PLATFORM "NULL" CACHE STRING "librw platform (NULL|GL3|D3D9)")
set_property(CACHE CANNONCRUISE_RW_PLATFORM PROPERTY STRINGS NULL GL3 D3D9)
set(LIBRW_PLATFORM "${CANNONCRUISE_RW_PLATFORM}" CACHE STRING "" FORCE)

FetchContent_Declare(librw
    GIT_REPOSITORY https://github.com/aap/librw.git
    GIT_TAG        18532c2e13efbc1aa43f0b00b4459831f9414de0
    GIT_SHALLOW    TRUE)
FetchContent_MakeAvailable(librw)

if(CANNONCRUISE_RW_PLATFORM STREQUAL "GL3")
    find_package(glfw3 REQUIRED)
endif()

# Source files (all game classes, havok shim, and framework sources)
file(GLOB_RECURSE SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.c"
)

list(FILTER SOURCES EXCLUDE REGEX "[/\\\\]build[^/\\\\]*([/\\\\]|$)")
list(FILTER SOURCES EXCLUDE REGEX "[/\\\\]main\\.cpp$")

if(NOT WIN32)
    list(FILTER SOURCES EXCLUDE REGEX "/framework/startup/win32/.*")
    list(FILTER SOURCES EXCLUDE REGEX "/framework/startup/startup\\.cpp")
endif()

# ---------------------------------------------------------------------------
# Compile and archive sources
# ---------------------------------------------------------------------------
add_library(CannonCruiseLib STATIC ${SOURCES})

target_include_directories(CannonCruiseLib
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/framework
        ${CMAKE_CURRENT_SOURCE_DIR}/framework/core
        ${CMAKE_CURRENT_SOURCE_DIR}/framework/toolkits
        "${bullet3_SOURCE_DIR}/src"
        "${librw_SOURCE_DIR}"
)

target_link_libraries(CannonCruiseLib
    PUBLIC
        librw::librw
        BulletDynamics
        BulletCollision
        LinearMath
)

if(WIN32)
    target_compile_definitions(CannonCruiseLib PUBLIC WIN32 _WINDOWS)
    target_link_libraries(CannonCruiseLib PUBLIC winmm.lib dsound.lib dxguid.lib)
    if(CANNONCRUISE_RW_PLATFORM STREQUAL "D3D9")
        target_compile_definitions(CannonCruiseLib PUBLIC D3D9)
        target_link_libraries(CannonCruiseLib PUBLIC d3d9.lib)
    endif()
else()
    target_compile_definitions(CannonCruiseLib PUBLIC _POSIX_C_SOURCE=200809L)
endif()

# ---------------------------------------------------------------------------
# Phase 4.5 — Link verification target.
# ---------------------------------------------------------------------------
add_executable(CannonCruise main.cpp)
add_dependencies(CannonCruise CannonCruiseLib)
target_link_libraries(CannonCruise PRIVATE
    "$<LINK_LIBRARY:WHOLE_ARCHIVE,CannonCruiseLib>"
)

# Copy audio files to build output directory for runtime access
if(WIN32 AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../Sound/Win32")
    add_custom_command(TARGET CannonCruise POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_CURRENT_SOURCE_DIR}/../Sound/Win32
            $<TARGET_FILE_DIR:CannonCruise>
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMENT "Copying audio assets to build output directory"
        VERBATIM)
endif()

# ---------------------------------------------------------------------------
# SDK-independent unit tests
# ---------------------------------------------------------------------------
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../tests/CMakeLists.txt")
    enable_testing()
    add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../tests ${CMAKE_CURRENT_BINARY_DIR}/tests)
else()
    message(STATUS "tests/CMakeLists.txt not found at ${CMAKE_CURRENT_SOURCE_DIR}/../tests; skipping unit tests.")
endif()