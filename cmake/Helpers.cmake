
function(createVersionHeader generatedHeaderDir)
  string(REGEX MATCH "^([0-9]+)(\\.([0-9]+))?(\\.([0-9]+))?(\\.([0-9]+))?$" _
    "${PROJECT_VERSION}"
  )

  set(PROJECT_VERSION_MAJOR ${CMAKE_MATCH_1})
  set(PROJECT_VERSION_MINOR ${CMAKE_MATCH_3})
  set(PROJECT_VERSION_PATCH ${CMAKE_MATCH_5})

  if(NOT DEFINED PROJECT_VERSION_MAJOR)
    set(PROJECT_VERSION_MAJOR "0")
  endif()
  if(NOT DEFINED PROJECT_VERSION_MINOR)
    set(PROJECT_VERSION_MINOR "0")
  endif()
  if(NOT DEFINED PROJECT_VERSION_PATCH)
    set(PROJECT_VERSION_PATCH "0")
  endif()

  string(TOUPPER ${PROJECT_NAME} UPPERCASE_PROJECT_NAME)
  # ensure that the generated macro does not include invalid characters
  string(REGEX REPLACE [^a-zA-Z0-9] _ UPPERCASE_PROJECT_NAME ${UPPERCASE_PROJECT_NAME})

  configure_file(
    ${PROJECT_SOURCE_DIR}/version.h.in
    ${generatedHeaderDir}/version.h @ONLY
  )
endfunction()