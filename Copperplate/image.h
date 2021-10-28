#pragma once

#include "core.h"

#include <glm\ext\vector_float2.hpp>
#include <glm\ext\vector_float4.hpp>
#include <glm\ext\vector_int2.hpp>


namespace Copperplate {	

	class Image {
	public:

		Image(int width, int height);
		~Image();

		glm::vec4 Sample(glm::vec2 uvPos);
		glm::vec4 Sample(glm::ivec2 pixelPos);

		void CopyFrameBuffer();

	private:

		glm::ivec2 m_Size;
		glm::vec4* m_Data;

	};
}