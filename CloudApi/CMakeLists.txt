find_package(CURL REQUIRED)
add_library(CloudApi STATIC CloudApi.h CloudApi.cpp ../Common.h)
target_link_libraries(CloudApi JSON U8)
