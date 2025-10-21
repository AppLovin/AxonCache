#----------------------------------------------------------------
# Generated CMake target import file for configuration "Release".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "app::axoncache" for configuration "Release"
set_property(TARGET app::axoncache APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(app::axoncache PROPERTIES
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libaxoncache.dylib"
  IMPORTED_SONAME_RELEASE "@rpath/libaxoncache.dylib"
  )

list(APPEND _cmake_import_check_targets app::axoncache )
list(APPEND _cmake_import_check_files_for_app::axoncache "${_IMPORT_PREFIX}/lib/libaxoncache.dylib" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
