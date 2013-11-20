add_library(JSON STATIC JSON.cpp JSON.h JSON.hpp JSONRPC.cpp JSONRPC.h Object.cpp 
	Object.h Object.hpp Value.cpp Value.h)
target_link_libraries(JSON U8)
