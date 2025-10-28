#pragma once

#ifdef PIL_WINDOWS
	#ifdef PIL_EXPORT
		#define PIL_API __declspec(dllexport)
	#else
		#define PIL_API __declspec(dllimport)
	#endif
#endif

#ifdef PIL_ENABLE_ASSERTS
	#define PIL_ASSERT(x, ...) { if(!(x)) { PIL_ERROR("Assertion Failed: {0}", __VA_ARGS__); __debugbreak(); } }
#define PIL_CORE_ASSERT(x, ...) { if(!(x)) { PIL_CORE_ERROR("Assertion Failed: {0}", __VA_ARGS__); __debugbreak(); } }
#else
	#define PIL_ASSERT(x, ...)
#define PIL_CORE_ASSERT(x, ...)
#endif

// Macro to bind member functions to event callbacks
#define BIND_EVENT_FN(fn) [this](auto&&... args) -> decltype(auto) { return this->fn(std::forward<decltype(args)>(args)...); }