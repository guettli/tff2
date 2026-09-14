cmake_minimum_required(VERSION 3.16)

project(tff_like_remapper VERSION 1.0.0)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Sanitizer options
option(ENABLE_SANITIZERS "Enable AddressSanitizer (ASan) and UndefinedBehaviorSanitizer (UBSan)" OFF)
if(ENABLE_SANITIZERS)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        message(STATUS "Enabling AddressSanitizer and UndefinedBehaviorSanitizer")
        add_compile_options(-fsanitize=address,undefined -fno-omit-frame-pointer -g)
        add_link_options(-fsanitize=address,undefined)
    else()
        message(WARNING "Sanitizers requested but compiler ${CMAKE_CXX_COMPILER_ID} is not GNU or Clang")
    endif()
endif()

# Include directories
include_directories(include)

# Source files
set(CORE_SOURCES
    src/core/key_detector.cpp
    src/core/key_mapper.cpp
    src/core/tff_app.cpp
    src/core/config_manager.cpp
    src/core/tff_yaml_parser.cpp
    src/core/tff_key_codes.cpp
    src/core/tff_engine.cpp
    src/core/tff_parser.cpp
)

# Linux platform source files
set(LINUX_PLATFORM_SOURCES
    src/platform/linux/linux_platform.cpp
)

# RP2040 platform source files
set(RP2040_PLATFORM_SOURCES
    src/platform/rp2040/rp2040_platform.cpp
)

# Create the core library
add_library(tff_core ${CORE_SOURCES})

# Compiler flags
target_compile_options(tff_core PRIVATE
    -Wall
    -Wextra
    -O2
)

# Create Linux platform library
add_library(linux_platform ${LINUX_PLATFORM_SOURCES})
target_link_libraries(linux_platform tff_core)

# Create RP2040 platform library
add_library(rp2040_platform ${RP2040_PLATFORM_SOURCES})
target_link_libraries(rp2040_platform tff_core)

# Create a simple test executable
add_executable(tff_test
    src/main.cpp
)

# Create YAML to JSON converter
add_executable(yaml_to_json_converter
    tools/yaml_to_json_converter.cpp
)

# Create RP2040 test application
add_executable(tff_rp2040
    src/platform/rp2040/main_rp2040.cpp
)

target_link_libraries(tff_test tff_core)

# Link RP2040 application with platform library
target_link_libraries(tff_rp2040 tff_core rp2040_platform)

# Create separate unit test executables
add_executable(test_key_detector
    tests/test_key_detector.cpp
)

target_link_libraries(test_key_detector tff_core)

add_executable(test_key_mapper
    tests/test_key_mapper.cpp
)

target_link_libraries(test_key_mapper tff_core)

add_executable(test_tff_app
    tests/test_tff_app.cpp
)

target_link_libraries(test_tff_app tff_core)

add_executable(test_rp2040_platform
    tests/test_rp2040_platform.cpp
)

target_link_libraries(test_rp2040_platform tff_core rp2040_platform)

add_executable(test_config_manager
    tests/test_config_manager.cpp
)

target_link_libraries(test_config_manager tff_core)

add_executable(test_tff_mappings
    tests/test_tff_mappings.cpp
)

target_link_libraries(test_tff_mappings tff_core)

# Create Go-parity TFF test suite executable
add_executable(test_tff_go_suite
    tests/test_tff_go_suite.cpp
)

target_link_libraries(test_tff_go_suite tff_core)

# Create UpBoard test application
add_executable(test_upboard_setup
    src/platform/linux/test_upboard_setup.cpp
)

target_link_libraries(test_upboard_setup tff_core linux_platform)

# Create TFF demo application
add_executable(test_tff_demo
    src/platform/linux/test_tff_demo.cpp
)

target_link_libraries(test_tff_demo tff_core linux_platform)

# Create Linux TFF remapper daemon
add_executable(tff_linux
    src/platform/linux/main_linux.cpp
)

target_link_libraries(tff_linux tff_core linux_platform)

# Create Linux platform unit/integration test
add_executable(test_linux_platform
    tests/test_linux_platform.cpp
)

target_link_libraries(test_linux_platform tff_core linux_platform)

# Enable testing
enable_testing()
add_test(NAME test_key_detector COMMAND test_key_detector)
add_test(NAME test_key_mapper COMMAND test_key_mapper)
add_test(NAME test_tff_app COMMAND test_tff_app)
add_test(NAME test_rp2040_platform COMMAND test_rp2040_platform)
add_test(NAME test_config_manager COMMAND test_config_manager)
add_test(NAME test_tff_mappings COMMAND test_tff_mappings)
add_test(NAME test_tff_go_suite COMMAND test_tff_go_suite)
add_test(NAME test_linux_platform COMMAND test_linux_platform)