function(set_if_not_defined var_name var_value)
    if(NOT DEFINED ${var_name})
        set(${var_name} ${var_value} PARENT_SCOPE)
    else()
        if(NOT ${${var_name}} STREQUAL ${var_value})
            MESSAGE(WARNING "${var_name} is pre-defined with ${${var_name}}\nset_if_not_defined(${var_name} ${var_value}) in ${PROJECT_NAME} is ignored")
        endif()
    endif()
endfunction()

# Versions of libraries used by unittests and cli, not by the C++ AxonCache lib itself.
# Those versions will be used by cmake when it does fetch dependencies through CPM (CMake Package Manager).
set(cxxopts_version "3.0.0")
set(doctest_version "2.4.12")
set(package_project_version "1.9.0")
set(benchmark_version "1.6.1")
set(spdlog_version "1.13.0")
