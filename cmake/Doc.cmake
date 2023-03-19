function(add_doc)
    find_package(Doxygen REQUIRED)
    set(DOXYGEN_IN "${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in")
    set(DOXYGEN_OUT "${CMAKE_CURRENT_BINARY_DIR}/Doc/Doxyfile")
    configure_file("${DOXYGEN_IN}" "${DOXYGEN_OUT}" @ONLY)
    add_custom_target("Doc" ALL
        COMMAND "${CMAKE_COMMAND}" -E make_directory "${PROJECT_BINARY_DIR}/Doc"
        COMMAND "${DOXYGEN_EXECUTABLE}" "${DOXYGEN_OUT}"
        COMMAND "${CMAKE_CURRENT_BINARY_DIR}/Doc/html/index.html"
        WORKING_DIRECTORY "${PROJECT_BINARY_DIR}/Doc"
        COMMENT "Generating API documentation with Doxygen"
        VERBATIM)
endfunction(add_doc)
