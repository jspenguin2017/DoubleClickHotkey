find_program(CLANG_FORMAT_EXECUTABLE NAMES clang-format)

if(CLANG_FORMAT_EXECUTABLE)
    add_custom_target(
        format
        COMMAND "${CLANG_FORMAT_EXECUTABLE}" --style=file -i ${DOUBLE_CLICK_HOTKEY_FORMAT_FILES}
        WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
        COMMENT "Formatting C++ sources"
        VERBATIM
    )
    add_custom_target(
        format-check
        COMMAND "${CLANG_FORMAT_EXECUTABLE}" --style=file --dry-run --Werror ${DOUBLE_CLICK_HOTKEY_FORMAT_FILES}
        WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
        COMMENT "Checking C++ formatting"
        VERBATIM
    )
else()
    message(STATUS "clang-format was not found; the format and format-check targets are unavailable")
endif()
