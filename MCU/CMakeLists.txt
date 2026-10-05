cmake_minimum_required(VERSION 3.22)

#
# This file is generated only once,
# and is not re-generated if converter is called multiple times.
#
# User is free to modify the file as much as necessary
#

# Setup compiler settings
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Workaround for some libraries
set(CMAKE_LINK_LIBRARY_USING_WHOLE_ARCHIVE
    "LINKER:--whole-archive"
    "<LINK_ITEM>"
    "LINKER:--no-whole-archive"
)

set(CMAKE_LINK_LIBRARY_USING_WHOLE_ARCHIVE_SUPPORTED TRUE)

# Define the build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug")
endif()

# Set the project name
set(CMAKE_PROJECT_NAME master)

# Enable compile command to ease indexing with e.g. clangd
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

# Core project settings
project(${CMAKE_PROJECT_NAME})
message("Build type: " ${CMAKE_BUILD_TYPE})

# Enable CMake support for ASM and C languages
enable_language(C CXX ASM)

# Create an executable object type
add_executable(${CMAKE_PROJECT_NAME}
    Core/Inc/ModbusConfig.h
    Core/Src/acceleration.cpp
    Core/Inc/StepperMotor.hpp
    Core/Inc/acceleration.h
    Core/Src/debug.c
    Core/Inc/debug.h
    Core/Src/communication.c
    Core/Inc/modbus_defs.h
)

# Add STM32CubeMX generated sources
add_subdirectory(cmake/stm32cubemx)

# Include external dependencies
add_subdirectory(cmake/modbus-stm32-hal-freertos)

# Link directories setup
target_link_directories(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined library search paths
)

# Add sources to executable

# Add include paths
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined include paths
)

# Add project symbols (macros)
target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined symbols
)

# Remove wrong libob.a library dependency when using cpp files
list(REMOVE_ITEM CMAKE_C_IMPLICIT_LINK_LIBRARIES ob)

# Add linked libraries
target_link_libraries(${CMAKE_PROJECT_NAME}
    Modbus_STM32_HAL_FreeRTOS
    stm32cubemx
    # Add user defined libraries
)
