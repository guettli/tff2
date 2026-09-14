cmake_minimum_required(VERSION 3.16)

# RP2040 Pico build option
option(PICO_BUILD "Build firmware for Raspberry Pi Pico RP2040" OFF)

if(PICO_BUILD)
    include(pico_sdk_import.cmake)
endif()

project(tff_like_remapper VERSION 1.0.0)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(PICO_BUILD)
    pico_sdk_init()
    add_compile_definitions(PICO_BUILD)
endif()

# Sanitizer options
option(ENABLE_SANITIZERS "Enable AddressSanitizer (ASan) and UndefinedBehaviorSanitizer (UBSan)" OFF)
if(ENABLE_SANITIZERS)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        message(STATUS "Enabling AddressSanitizer and UndefinedBehaviorSanitizer")
        add_compile_options(-fsanitize=address,undefined -fno-sanitize-recover=undefined -fno-omit-frame-pointer -g)
        add_link_options(-fsanitize=address,undefined)
    else()
        message(FATAL_ERROR "Sanitizers requested but compiler ${CMAKE_CXX_COMPILER_ID} is not GNU or Clang")
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

# RP2040 platform source files
set(RP2040_PLATFORM_SOURCES
    src/platform/rp2040/rp2040_platform.cpp
)

# Compiler warning options
option(TFF_WARNINGS_AS_ERRORS "Treat compiler warnings as errors" ON)
if(NOT PICO_BUILD)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        add_compile_options(-Wall -Wextra -Wpedantic)
        if(TFF_WARNINGS_AS_ERRORS)
            add_compile_options(-Werror)
        endif()
    endif()
else()
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        add_compile_options(-Wall -Wextra -Wno-psabi)
        if(TFF_WARNINGS_AS_ERRORS)
            add_compile_options(-Werror)
        endif()
    endif()
endif()

# Create the core library
add_library(tff_core ${CORE_SOURCES})

# Create RP2040 platform library
add_library(rp2040_platform ${RP2040_PLATFORM_SOURCES})
if(PICO_BUILD)
    target_link_libraries(rp2040_platform tff_core pico_stdlib hardware_gpio tinyusb_device tinyusb_host)
else()
    target_link_libraries(rp2040_platform tff_core)
endif()

# Create RP2040 application
add_executable(tff_rp2040
    src/platform/rp2040/main_rp2040.cpp
)

if(PICO_BUILD)
    target_link_libraries(tff_rp2040
        tff_core
        rp2040_platform
        pico_stdlib
        hardware_gpio
        tinyusb_device
        tinyusb_host
    )
    pico_add_extra_outputs(tff_rp2040)
else()
    target_link_libraries(tff_rp2040 tff_core rp2040_platform)
endif()

if(NOT PICO_BUILD)
    # Linux platform source files
    set(LINUX_PLATFORM_SOURCES
        src/platform/linux/linux_platform.cpp
    )

    # Create Linux platform library
    add_library(linux_platform ${LINUX_PLATFORM_SOURCES})
    target_link_libraries(linux_platform tff_core)

    # Create a simple test executable
    add_executable(tff_test
        src/main.cpp
    )
    target_link_libraries(tff_test tff_core)

    # Create YAML to JSON converter
    add_executable(yaml_to_json_converter
        tools/yaml_to_json_converter.cpp
    )

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

    # Ensure assertions are always active in test executables regardless of build type
    set(TEST_TARGETS
        test_key_detector
        test_key_mapper
        test_tff_app
        test_rp2040_platform
        test_config_manager
        test_tff_mappings
        test_tff_go_suite
        test_linux_platform
    )
    foreach(target ${TEST_TARGETS})
        if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
            target_compile_options(${target} PRIVATE -UNDEBUG)
        endif()
    endforeach()

    # Static analysis with Cppcheck
    find_program(CPPCHECK_EXECUTABLE NAMES cppcheck PATHS "$ENV{HOME}/.local/bin" /usr/bin /usr/local/bin)
    if(CPPCHECK_EXECUTABLE)
        add_custom_target(cppcheck
            COMMAND ${CPPCHECK_EXECUTABLE}
                --enable=warning,style,performance,portability
                --inline-suppr
                --suppress=missingIncludeSystem
                --suppress=checkersReport
                --suppress=unusedFunction
                --suppress=useStlAlgorithm
                --suppress=normalCheckLevelMaxBranches
                --suppress=unusedPrivateFunction:*rp2040_platform.h
                --error-exitcode=1
                --std=c++17
                -I ${CMAKE_CURRENT_SOURCE_DIR}/include
                ${CMAKE_CURRENT_SOURCE_DIR}/src
                ${CMAKE_CURRENT_SOURCE_DIR}/tests
                ${CMAKE_CURRENT_SOURCE_DIR}/tools
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
            COMMENT "Running Cppcheck static code analysis..."
        )
    endif()
endif() # NOT PICO_BUILD