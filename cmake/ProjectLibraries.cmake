# link all dependencies to our library
# ${PROJECT_LIB_NAME} can be polluted from other cpm packages
# we should discuss moving from a var name to hardcoded string
target_link_libraries(axoncache PUBLIC)

if(UNIX AND NOT APPLE)
    find_library(NUMA_LIBRARY NAMES numa)
    find_path(NUMA_INCLUDE_DIR NAMES numaif.h)
    if(NUMA_LIBRARY AND NUMA_INCLUDE_DIR)
        target_link_libraries(axoncache PUBLIC ${NUMA_LIBRARY})
        target_include_directories(axoncache PRIVATE ${NUMA_INCLUDE_DIR})
        target_compile_definitions(axoncache PRIVATE HAVE_LIBNUMA)
    else()
        message(STATUS "libnuma not found")
    endif()
endif()
