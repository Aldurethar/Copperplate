#pragma once

#include "core.h"

namespace Copperplate {

	GLenum glCheckError_(const char* file, int line)
	{
		GLenum errorCode;
		while ((errorCode = glGetError()) != GL_NO_ERROR)
		{
			std::string error;
			switch (errorCode)
			{
			case GL_INVALID_ENUM:                  error = "INVALID_ENUM"; break;
			case GL_INVALID_VALUE:                 error = "INVALID_VALUE"; break;
			case GL_INVALID_OPERATION:             error = "INVALID_OPERATION"; break;
			case GL_STACK_OVERFLOW:                error = "STACK_OVERFLOW"; break;
			case GL_STACK_UNDERFLOW:               error = "STACK_UNDERFLOW"; break;
			case GL_OUT_OF_MEMORY:                 error = "OUT_OF_MEMORY"; break;
			case GL_INVALID_FRAMEBUFFER_OPERATION: error = "INVALID_FRAMEBUFFER_OPERATION"; break;
			}
			std::cout << error << " | " << file << " (" << line << ")" << std::endl;
		}
		return errorCode;
	}

	GLenum glCheckFrameBufferError_(const char* file, int line)
	{
		GLenum errorCode;
		while ((errorCode = glCheckFramebufferStatus(GL_FRAMEBUFFER)) != GL_FRAMEBUFFER_COMPLETE)
		{
			std::string error;
			switch (errorCode)
			{
			case GL_FRAMEBUFFER_UNDEFINED:						error = "FRAMEBUFFER_UNDEFINED"; break;
			case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:          error = "FRAMEBUFFER_INCOMPLETE_ATTACHMENT"; break;
			case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:  error = "FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT"; break;
			case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER:         error = "FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER"; break;
			case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER:         error = "FRAMEBUFFER_INCOMPLETE_READ_BUFFER"; break;
			case GL_FRAMEBUFFER_UNSUPPORTED:					error = "FRAMEBUFFER_UNSUPPORTED"; break;
			case GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE:			error = "FRAMEBUFFER_INCOMPLETE_MULTISAMPLE"; break;
			case GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS:		error = "FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS"; break;
			}
			std::cout << error << " | " << file << " (" << line << ")" << std::endl;
		}
		return errorCode;
	}
}