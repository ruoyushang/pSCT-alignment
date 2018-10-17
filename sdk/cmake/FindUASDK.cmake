# REMOVE ME
file(TO_NATIVE_PATH ${CMAKE_PARENT_LIST_FILE} NATIVE_CMAKE_PARENT_LIST_FILE)
get_filename_component(NATIVE_CMAKE_CURRENT_LIST_PATH ${CMAKE_CURRENT_LIST_FILE} PATH)
file(TO_NATIVE_PATH ${NATIVE_CMAKE_CURRENT_LIST_PATH} NATIVE_CMAKE_CURRENT_LIST_PATH)
message(WARNING "Using the file FindUASDK.cmake is deprecated! Remove the line "
                "'find_package(UASDK REQUIRED)' from '${NATIVE_CMAKE_PARENT_LIST_FILE}' "
                "and use the CMake modules in '${NATIVE_CMAKE_CURRENT_LIST_PATH}' instead.")
