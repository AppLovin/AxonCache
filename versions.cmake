function(set_if_not_defined var_name var_value)
    if(NOT DEFINED ${var_name})
        set(${var_name} ${var_value} PARENT_SCOPE)
    else()
        if(NOT ${${var_name}} STREQUAL ${var_value})
            MESSAGE(WARNING "${var_name} is pre-defined with ${${var_name}}\nset_if_not_defined(${var_name} ${var_value}) in ${PROJECT_NAME} is ignored")
        endif()
    endif()
endfunction()

# AppLovin Libraries
set_if_not_defined(alcommons_version "v4897")

# Third party libraries
set(xxhash_version "v0.8.1")

# test libraries
set(cxxopts_version "3.0.0")
set(doctest_version "2.4.12")
set(cmake_format_version "1.7.3")
set(package_project_version "1.9.0")
set(benchmark_version "1.6.1")
set(spdlog_version "1.13.0")

# static build
set(unwind_version "v1.5")
set(mimalloc_version "v2.0.5")
