#pragma once
#ifndef CLARINET_H
#define CLARINET_H
#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/**
 * Utility macros
 */
#define CLARINET_STR(s) #s
#define CLARINET_XSTR(s) CLARINET_STR(s)

/**
 * API symbols
 *
 * CLARINET_EXPORT is defined by the build system when the target is a shared library. In this case we can arrange to 
 * export only the necessary symbols by defining CLARINET_API accordingly. 
 * 
 * On Windows it may be advantageous for headers accompanying DLLs to declare exported symbols with 
 * __declspec(dllimport) because the compiler can alledgedly produce more efficient code if the attribute is present.
 * See https://docs.microsoft.com/en-us/cpp/build/importing-into-an-application-using-declspec-dllimport?view=msvc-160
 * 
 * The common case is for the header to accompany a shared library so the user must define CLARINET_STATIC when 
 * using static linkage. This is more convenient because static libraries tend to be compiled beside the final binary 
 * (either executable or shared library) and build requirements offer less attriction.
 */
#ifdef CLARINET_STATIC
	#define CLARINET_VISIBLITY
#else 
	#ifdef _WIN32
		#ifdef CLARINET_EXPORT
			#define CLARINET_VISIBLITY 	__declspec(dllexport)
		#else
			#define CLARINET_VISIBLITY 	__declspec(dllimport)
		#endif
	#else /* UN*X */
		#ifdef CLARINET_EXPORT
			#if CLARINET_IS_AT_LEAST_GNUC_VERSION(3,4)
				/*
				* GCC 3.4 and later (or some compiler asserting compatibility with
				* GCC 3.4 and later) so we have __attribute__((visibility()).
				*/
				#define CLARINET_VISIBLITY	__attribute__((visibility("default")))
			#else 
				#define CLARINET_VISIBLITY
			#endif
		#else 
			#define CLARINET_VISIBLITY
		#endif
	#endif
#endif

#define CLARINET_API CLARINET_VISIBLITY extern
	
/** 
 * Data Structures 
 *
 * In POSIX, names ending with _t are reserved. Since we're targeting at least one POSIX system (i.e. Linux) typenames 
 * defined here MUST NOT end with _t.
 */



/**
 * Properties Interface
 */
 
CLARINET_API    uint32_t clarinet_getlibnver(void);
CLARINET_API const char* clarinet_getlibsver(void);
CLARINET_API const char* clarinet_getlibname(void);
CLARINET_API const char* clarinet_getlibdesc(void);

/**
 * Control Interface
 */
 
CLARINET_API         int clarinet_initialize(void);
CLARINET_API         int clarinet_finalize(void);


#ifdef __cplusplus
}
#endif
#endif // CLARINET_H