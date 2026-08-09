# SPDX-License-Identifier: MIT
# Copyright (c) 2025 AppLovin. All rights reserved.

# Rust bindings for AxonCache
cmake_minimum_required(VERSION 3.15)

# Check if Rust is available
find_program(RUSTC rustc)
if(NOT RUSTC)
    message(WARNING "Rust compiler not found. Rust bindings will not be built.")
    return()
endif()

# Check if Cargo is available
find_program(CARGO cargo)
if(NOT CARGO)
    message(WARNING "Cargo not found. Rust bindings will not be built.")
    return()
endif()

# Set Rust target directory
set(RUST_TARGET_DIR "${CMAKE_BINARY_DIR}/rust_target")
set(RUST_RELEASE_DIR "${RUST_TARGET_DIR}/release")
set(RUST_DEBUG_DIR "${RUST_TARGET_DIR}/debug")

# Create Rust target directory
file(MAKE_DIRECTORY "${RUST_TARGET_DIR}")

# Function to build Rust library
function(build_rust_library)
    set(options RELEASE DEBUG)
    set(oneValueArgs TARGET_NAME)
    set(multiValueArgs DEPENDS)
    cmake_parse_arguments(RUST "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    if(NOT RUST_TARGET_NAME)
        set(RUST_TARGET_NAME "axoncache")
    endif()

    # Determine build type
    if(RUST_RELEASE)
        set(BUILD_TYPE "release")
        set(TARGET_DIR "${RUST_RELEASE_DIR}")
    else()
        set(BUILD_TYPE "debug")
        set(TARGET_DIR "${RUST_DEBUG_DIR}")
    endif()

    # Build command
    if(BUILD_TYPE STREQUAL "release")
        set(CARGO_BUILD_CMD "${CARGO}" build --release)
    else()
        set(CARGO_BUILD_CMD "${CARGO}" build)
    endif()

    # Create custom target
    add_custom_target(${RUST_TARGET_NAME}_rust
        COMMAND ${CARGO_BUILD_CMD}
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
        COMMENT "Building Rust library ${RUST_TARGET_NAME}"
        VERBATIM
    )

    # Add dependencies
    if(RUST_DEPENDS)
        add_dependencies(${RUST_TARGET_NAME}_rust ${RUST_DEPENDS})
    endif()

    # Set output properties
    set_target_properties(${RUST_TARGET_NAME}_rust PROPERTIES
        OUTPUT_NAME "${RUST_TARGET_NAME}"
        RUNTIME_OUTPUT_DIRECTORY "${TARGET_DIR}"
        LIBRARY_OUTPUT_DIRECTORY "${TARGET_DIR}"
        ARCHIVE_OUTPUT_DIRECTORY "${TARGET_DIR}"
    )
endfunction()

# Build Rust library in both debug and release modes
build_rust_library(TARGET_NAME "axoncache" DEPENDS "axoncache")

# Create a combined target that builds both debug and release
add_custom_target(axoncache_rust_all
    DEPENDS axoncache_rust
    COMMENT "Building all Rust targets"
)

# Install Rust library
install(FILES
    "${RUST_RELEASE_DIR}/libaxoncache.a"
    DESTINATION lib
    COMPONENT rust
    OPTIONAL
)

# Create example targets
add_custom_target(rust_examples
    COMMAND ${CARGO} build --examples
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
    DEPENDS axoncache_rust
    COMMENT "Building Rust examples"
)

# Test target
add_custom_target(rust_test
    COMMAND ${CARGO} test
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
    DEPENDS axoncache_rust
    COMMENT "Running Rust tests"
)

# Documentation target
add_custom_target(rust_docs
    COMMAND ${CARGO} doc --no-deps
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
    DEPENDS axoncache_rust
    COMMENT "Generating Rust documentation"
)

# Clean target
add_custom_target(rust_clean
    COMMAND ${CARGO} clean
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
    COMMENT "Cleaning Rust build artifacts"
)

# Add Rust targets to the main project
if(TARGET axoncache)
    add_dependencies(axoncache_rust axoncache)
endif()

# Note: Rust build configuration is handled in build.rs
# The build script will automatically detect the build directory and include paths
