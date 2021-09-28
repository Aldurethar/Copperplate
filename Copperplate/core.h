#pragma once

#include <iostream>
#include <glad/glad.h>

namespace Copperplate {

	GLenum glCheckError_(const char* file, int line);
	#define glCheckError() glCheckError_(__FILE__, __LINE__)	

}