add_definitions(-DCURL_STATICLIB)

if(WINDOWS)
	set(CURL_LIBRARIES ${PROJECT_SOURCE_DIR}/libs/win/curl-7.28.1/lib.${PROC}/libcurl.lib)
	set(CURL_INCLUDE_DIRS ${PROJECT_SOURCE_DIR}/libs/win/curl-7.28.1/inc)
else()
	FIND_PACKAGE(CURL REQUIRED)
endif()

ADD_SUBDIRECTORY(${CMAKE_CURRENT_SOURCE_DIR}/liboauthcpp/build)
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/liboauthcpp/include)

if(NOT WINDOWS)
	SET(CMAKE_CXX_FLAGS "-std=c++0x ${CMAKE_CXX_FLAGS}")
endif()

test_big_endian(ORDER_BIG_ENDIAN)

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

LINK_DIRECTORIES(${CURL_INCLUDE_DIRS} ${OpenSSL_INCLUDE_DIR})
INCLUDE_DIRECTORIES(${OpenSSL_INCLUDE_DIR} ${CURL_INCLUDE_DIRS})
TARGET_LINK_LIBRARIES(CloudApi ${CURL_LIBRARIES} ${Boost_LIBRARIES} ${OpenSSL_LIBS})
TARGET_LINK_LIBRARIES(CloudApi oauthcpp)
