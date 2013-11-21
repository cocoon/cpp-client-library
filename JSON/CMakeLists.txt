ADD_LIBRARY(JSON STATIC JSON.cpp JSON.h JSON.hpp JSONRPC.cpp JSONRPC.h Object.cpp 
	Object.h Object.hpp Value.cpp Value.h)
TARGET_LINK_LIBRARIES(JSON U8)
