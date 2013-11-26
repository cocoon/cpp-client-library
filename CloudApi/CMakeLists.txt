FIND_PACKAGE(CURL REQUIRED)

ADD_SUBDIRECTORY(${CMAKE_CURRENT_SOURCE_DIR}/liboauthcpp/build)
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/liboauthcpp/include)

ADD_LIBRARY(CloudApi STATIC CloudApi.h CloudApi.cpp Common.h 
	# JSON rpc support files
	JSON/JSON.h
	JSON/JSONRPC.h
	JSON/Object.h
	JSON/Value.h
	JSON/JSON.cpp
	JSON/JSONRPC.cpp
	JSON/Object.cpp
	JSON/Value.cpp
	JSON/Object.hpp

	# Utf8 apis
	U8/U8.h
	U8/U8.cpp

	# Utility
	Util/Util.h
	Util/Data.h
	Util/StructParser.h)
TARGET_LINK_LIBRARIES(CloudApi JSON U8)
TARGET_LINK_LIBRARIES(CloudApi oauthcpp)
