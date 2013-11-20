find_package(CURL REQUIRED)
add_executable(CopyExample Main.cpp)
add_dependencies(CopyExample JSON CloudApi)
link_directories(${CURL_INCLUDE_DIRS})
target_link_libraries(CopyExample JSON CloudApi ${CURL_LIBRARIES})

