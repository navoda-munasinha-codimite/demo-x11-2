cmake_minimum_required(VERSION 3.10)
project(demo_x11)

# Set C standard
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Find X11 libraries
find_package(X11 REQUIRED)
if(NOT X11_FOUND)
    message(FATAL_ERROR "X11 libraries not found")
endif()

# Get vcpkg root from environment variable
set(VCPKG_ROOT $ENV{VCPKG_ROOT}/installed/x64-linux)

# Verify vcpkg root exists
if(NOT EXISTS ${VCPKG_ROOT})
    message(FATAL_ERROR "VCPKG_ROOT environment variable not set or path doesn't exist: ${VCPKG_ROOT}")
endif()

# Find required libraries
find_library(FREERDP_LIB        
    NAMES freerdp3
    PATHS "${VCPKG_ROOT}/lib" 
    REQUIRED
    NO_DEFAULT_PATH)

find_library(FREERDP_CLIENT_LIB 
    NAMES freerdp-client3
    PATHS "${VCPKG_ROOT}/lib" 
    REQUIRED
    NO_DEFAULT_PATH)

find_library(WINPR_LIB          
    NAMES winpr3
    PATHS "${VCPKG_ROOT}/lib" 
    REQUIRED
    NO_DEFAULT_PATH)

# Find OpenSSL libraries
find_library(OPENSSL_CRYPTO_LIB
    NAMES crypto
    PATHS "${VCPKG_ROOT}/lib"
    REQUIRED
    NO_DEFAULT_PATH)

find_library(OPENSSL_SSL_LIB
    NAMES ssl
    PATHS "${VCPKG_ROOT}/lib"
    REQUIRED
    NO_DEFAULT_PATH)

# Find cJSON library
find_library(CJSON_LIB
    NAMES cjson
    PATHS "${VCPKG_ROOT}/lib"
    REQUIRED
    NO_DEFAULT_PATH)

set(SOURCES
    main.c
    client/client.c
    client/client_hooks.c
    channels/remdesk/common/remdesk_common.c
    channels/remdesk/client/remdesk_main.c
    errors/error.c
    components/xf_monitor.c
    components/xf_utils.c
    components/xf_window.c
)

# Create executable with all source files
add_executable(${PROJECT_NAME} ${SOURCES})

# Include directories
target_include_directories(${PROJECT_NAME}
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/channels/remdesk/common
        ${CMAKE_CURRENT_SOURCE_DIR}/channels/remdesk/client
        "${VCPKG_ROOT}/include"
        "${VCPKG_ROOT}/include/freerdp3"
        "${VCPKG_ROOT}/include/winpr3"
        ${X11_INCLUDE_DIR}
)

# Link libraries
target_link_libraries(${PROJECT_NAME}
    PRIVATE
        ${FREERDP_CLIENT_LIB}
        ${FREERDP_LIB}
        ${WINPR_LIB}
        ${OPENSSL_SSL_LIB}
        ${OPENSSL_CRYPTO_LIB}
        ${CJSON_LIB}
        ${X11_LIBRARIES}
        ${X11_Xext_LIB}
        ${X11_Xrandr_LIB}
        ${X11_Xinerama_LIB}
        ${X11_Xcursor_LIB}
        ${X11_Xfixes_LIB}
        z  # for compress function
        pthread
        m
        dl
)

# Compile definitions
target_compile_definitions(${PROJECT_NAME} PRIVATE
    _GNU_SOURCE
)

# Compile options
target_compile_options(${PROJECT_NAME} PRIVATE
    -Wall
    -Wextra
    -Wno-unused-parameter
)