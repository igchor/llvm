set(TARGET_NAME ur_level_zero)

add_sycl_library(${TARGET_NAME}
    SHARED
    TOOLCHAIN
    SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/ur_level_zero.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/ur_level_zero.hpp
        # These are short-term shared with Unified Runtime
        # The two plugins define a few things differrently so must
        # be built separately. This difference is spelled in
        # their "ur_bindings.hpp" files.
        #
        ${CMAKE_CURRENT_SOURCE_DIR}/../../../ur_bindings.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../../../pi2ur.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../../../pi2ur.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../../ur.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../../ur.cpp
    INCLUDE_DIRS
        ${CMAKE_CURRENT_SOURCE_DIR}/../..
        ${CMAKE_CURRENT_SOURCE_DIR}/../../..
    LIBRARIES
        LevelZeroLoader-Headers
        LevelZeroLoader
)

# if(UNIX)
#     set(GCC_COVERAGE_COMPILE_FLAGS "-fvisibility=hidden -fvisibility-inlines-hidden -fno-strict-aliasing")
#     set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} ${GCC_COVERAGE_COMPILE_FLAGS}")
# endif()
