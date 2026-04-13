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


# Define the build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug")
endif()

# Set the project name
set(CMAKE_PROJECT_NAME usart)

# Enable compile command to ease indexing with e.g. clangd
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

# Core project settings
project(${CMAKE_PROJECT_NAME})
message("Build type: " ${CMAKE_BUILD_TYPE})

# Enable CMake support for ASM and C languages
enable_language(C ASM)

# Create an executable object type
add_executable(${CMAKE_PROJECT_NAME})

# Add STM32CubeMX generated sources
add_subdirectory(cmake/stm32cubemx)

# Link directories setup
target_link_directories(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined library search paths
)

# Add sources to executable
target_sources(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user sources here
)

# Add user application sources
target_sources(${CMAKE_PROJECT_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/Core/Src/freertos_demo.c
)

# Add Core-level user sources
target_sources(${CMAKE_PROJECT_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/Core/Src/iic_soft.c
    ${CMAKE_SOURCE_DIR}/Core/Src/Get_and_Convert.c
    ${CMAKE_SOURCE_DIR}/Core/Src/Safety_Protection.c
)

# Add include paths
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/FreeRTOS/include
    ${CMAKE_SOURCE_DIR}/FreeRTOS/portable/ARM_CM3
    ${CMAKE_SOURCE_DIR}/Core/Inc
)

# Add FreeRTOS source files so RTOS implementation is compiled and linked
file(GLOB FREERTOS_SRC
    "${CMAKE_SOURCE_DIR}/FreeRTOS/source/*.c"
    "${CMAKE_SOURCE_DIR}/FreeRTOS/portable/ARM_CM3/*.c"
    "${CMAKE_SOURCE_DIR}/FreeRTOS/portable/MemMang/*.c"
)
target_sources(${CMAKE_PROJECT_NAME} PRIVATE ${FREERTOS_SRC})

# Add project symbols (macros)
target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined symbols
)

# Remove wrong libob.a library dependency when using cpp files
list(REMOVE_ITEM CMAKE_C_IMPLICIT_LINK_LIBRARIES ob)

# Add linked libraries
target_link_libraries(${CMAKE_PROJECT_NAME}
    stm32cubemx

    # Add user defined libraries
)
