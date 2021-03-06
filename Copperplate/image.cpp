#pragma once

#include "image.h"
#include <glm\common.hpp>

namespace Copperplate {

	Image::Image(int width, int height) {
		m_Size = glm::ivec2(width, height);
		m_Data = new glm::vec4[width * height];
	}

	Image::~Image() {
		delete[] m_Data;
	}

	glm::vec4 Image::SampleUV(glm::vec2 uvPos) {
		return Sample(uvPos * glm::vec2(m_Size));
	}

	glm::vec4 Image::Sample(glm::vec2 screenPos) {
		glm::vec2 pos = glm::clamp(screenPos, glm::vec2(0.01f), glm::vec2(m_Size) - glm::vec2(1.0f));
		glm::vec2 fraction = glm::fract(pos);

		if (fraction == glm::vec2(0.0f))
			return Sample(glm::ivec2(pos));

		glm::ivec2 samplePos = glm::ivec2(floor(pos.x), floor(pos.y));
		glm::vec4 bottom = Sample(samplePos);
		samplePos = glm::ivec2(floor(pos.x), ceil(pos.y));
		glm::vec4 top = Sample(samplePos);
		glm::vec4 left = (1 - fraction.y) * bottom + fraction.y * top;

		samplePos = glm::ivec2(ceil(pos.x), floor(pos.y));
		bottom = Sample(samplePos);
		samplePos = glm::ivec2(ceil(pos.x), ceil(pos.y));
		top = Sample(samplePos);
		glm::vec4 right = (1 - fraction.y) * bottom + fraction.y * top;

		return (1 - fraction.x) * left + fraction.x * right;
	}

	glm::vec4 Image::Sample(glm::ivec2 pixelPos) {
		return m_Data[pixelPos.y * m_Size.x + pixelPos.x];
	}

	void Image::CopyFrameBuffer() {
		glReadPixels(0, 0, m_Size.x, m_Size.y, GL_RGBA, GL_FLOAT, m_Data);
	}

}