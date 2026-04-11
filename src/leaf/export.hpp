#pragma once

#if defined(_WIN32)
	#if defined(LEAF_FRAMEWORK_EXPORTS)
		#define LF_API __declspec(dllexport)
	#else
		#define LF_API __declspec(dllimport)
	#endif
#else
	#define LF_API
#endif
