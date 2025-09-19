# link all dependencies to our library
# ${PROJECT_LIB_NAME} can be polluted from other cpm packages
# we should discuss moving from a var name to hardcoded string
target_link_libraries(axoncache PUBLIC)
