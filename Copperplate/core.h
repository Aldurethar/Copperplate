#pragma once

#include <iostream>
#include <glad/glad.h>

namespace Copperplate {

	GLenum glCheckError_(const char* file, int line);
	#define glCheckError() glCheckError_(__FILE__, __LINE__)	

	GLenum glCheckFrameBufferError_(const char* file, int line);
	#define glCheckFrameBufferError() glCheckFrameBufferError_(__FILE__, __LINE__)

	template<typename T>
	using Unique = std::unique_ptr<T>;

	template<typename T, typename ... Args>
	constexpr Unique<T> CreateUnique(Args&& ... args) {
		return std::make_unique<T>(std::forward<Args>(args)...);
	}

	template<typename T>
	using Shared = std::shared_ptr<T>;

	template<typename T, typename ... Args>
	constexpr Shared<T> CreateShared(Args&& ... args) {
		return std::make_shared<T>(std::forward<Args>(args)...);
	}

}