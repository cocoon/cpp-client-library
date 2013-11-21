#ifndef __COPYAPI_COMMON__
#define __COPYAPI_COMMON__

#include "curl/curl.h"
#include <exception>
#include <vector>
#include <string>
#include <thread>
#include <mutex>
#include <cstring>
#include <chrono>
#include <map>
#include <math.h>
#include <iostream>
#include <sstream>
#include <algorithm>
#include <iomanip>

#define	BSWAP_64(x)	(((uint64_t)(x) << 56) | \
			(((uint64_t)(x) << 40) & 0xff000000000000ULL) | \
			(((uint64_t)(x) << 24) & 0xff0000000000ULL) | \
			(((uint64_t)(x) << 8)  & 0xff00000000ULL) | \
			(((uint64_t)(x) >> 8)  & 0xff000000ULL) | \
			(((uint64_t)(x) >> 24) & 0xff0000ULL) | \
			(((uint64_t)(x) >> 40) & 0xff00ULL) | \
			((uint64_t)(x)  >> 56))
#define	BSWAP_32(x)	(((uint32_t)(x) << 24) | \
			(((uint32_t)(x) << 8)  & 0xff0000) | \
			(((uint32_t)(x) >> 8)  & 0xff00) | \
			((uint32_t)(x)  >> 24))
#define	BSWAP_16(x)	((((uint16_t)(x)) >> 8) | \
			(((uint16_t)(x))  << 8))

#ifdef ORDER_BIG_ENDIAN
	// CPU -> big endian
	#define CPU64_BE(x)   ((uint64_t)(x))
	#define CPU32_BE(x)   ((uint32_t)(x))
	#define CPU16_BE(x)   ((uint16_t)(x))
	#define CPU8_BE(x)    ((uint8_t) (x))

	// CPU -> little endian
	#define CPU64_LE(x)   BSWAP_64(x)
	#define CPU32_LE(x)   BSWAP_32(x)
	#define CPU16_LE(x)   BSWAP_16(x)
	#define CPU8_LE(x)    ((uint8_t) (x))

	// big endian -> CPU
	#define BE64_CPU(x)   ((uint64_t)(x))
	#define BE32_CPU(x)   ((uint32_t)(x))
	#define BE16_CPU(x)   ((uint16_t)(x))
	#define BE8_CPU(x)    ((uint8_t) (x))

	// little endian -> CPU
	#define LE64_CPU(x)   BSWAP_64(x)
	#define LE32_CPU(x)   BSWAP_32(x)
	#define LE16_CPU(x)   BSWAP_16(x)
	#define LE8_CPU(x)    ((uint8_t)(x))
#else
	// CPU -> big endian
	#define CPU64_BE(x)   BSWAP_64(x)
	#define CPU32_BE(x)   BSWAP_32(x)
	#define CPU16_BE(x)   BSWAP_16(x)
	#define CPU8_BE(x)    ((uint8_t) (x))

	// CPU -> little endian
	#define CPU64_LE(x)   ((uint64_t)(x))
	#define CPU32_LE(x)   ((uint32_t)(x))
	#define CPU16_LE(x)   ((uint16_t)(x))
	#define CPU8_LE(x)    ((uint8_t) (x))

	// big endian -> CPU
	#define BE64_CPU(x)   BSWAP_64(x)
	#define BE32_CPU(x)   BSWAP_32(x)
	#define BE16_CPU(x)   BSWAP_16(x)
	#define BE8_CPU(x)    ((uint8_t)(x))

	// little endian -> CPU
	#define LE64_CPU(x)   ((uint64_t)(x))
	#define LE32_CPU(x)   ((uint32_t)(x))
	#define LE16_CPU(x)   ((uint16_t)(x))
	#define LE8_CPU(x)    ((uint8_t) (x))
#endif

// CPU -> network
#define CPU64_NET(x)	CPU64_BE(x)
#define CPU32_NET(x)	CPU32_BE(x)
#define CPU16_NET(x)	CPU16_BE(x)
#define CPU8_NET(x)	CPU8_BE(x)

// network -> CPU
#define NET64_CPU(x)	BE64_CPU(x)
#define NET32_CPU(x)	BE32_CPU(x)
#define NET16_CPU(x)	BE16_CPU(x)
#define NET8_CPU(x)	BE8_CPU(x)

#include "Util/Util.h"
#include "CloudApi/CloudApi.h"

#endif
