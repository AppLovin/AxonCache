if(APPLE)
  set(base @loader_path)
else()
  set(base $ORIGIN)
endif()

include(GNUInstallDirs)

file(RELATIVE_PATH relDir
  ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_INSTALL_BINDIR}
  ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_INSTALL_LIBDIR}
)

set(CMAKE_INSTALL_RPATH ${base} ${base}/${relDir})