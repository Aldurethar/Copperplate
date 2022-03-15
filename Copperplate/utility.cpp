#pragma once

#include "utility.h"

#include <random>
#include <queue>
#include <glm\detail\func_geometric.inl>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#define STBIW_WINDOWS_UTF8
#include "stb_image_write.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include <chrono>
#include <iomanip>
#include <sstream>
#include <iostream>
#include <glad\glad.h>

namespace Copperplate {

	const float PI = 3.1415926535f;
	const float HALFPI = 1.570796327f;
	const float TWOPI = 6.283185307f;

	// HALTON SEQUENCE IMPLEMENTATION
	HaltonSequence::HaltonSequence(int base) {
		m_Base = base;
		m_Numerator = 0;
		m_Denominator = 1;
	}

	void HaltonSequence::Skip(int amount) {
		for (int i = 0; i < amount; i++) {
			this->NextNumber();
		}
	}

	float HaltonSequence::NextNumber() {
		//Algorithm taken from https://en.wikipedia.org/wiki/Halton_sequence
		int x = m_Denominator - m_Numerator;
		if (x == 1) {
			m_Numerator = 1;
			m_Denominator *= m_Base;
		}
		else {
			int y = m_Denominator / m_Base;
			while (x <= y) {
				y /= m_Base;
			}
			m_Numerator = (m_Base + 1) * y - x;
		}
		return (float)m_Numerator / (float)m_Denominator;
	}
	
	glm::vec2 cartesianToPolar(glm::vec2 vec) {
		float a = acos(vec.x);
		if (vec.y < 0) a = TWOPI - a;
		return glm::vec2(glm::length(vec), a);
	}

	glm::vec2 polarToCartesian(glm::vec2 polVec) {
		float x = polVec.x * cos(polVec.y);
		float y = polVec.x * sin(polVec.y);
		return glm::vec2(x, y);
	}

	float degToRad(float deg) {
		return deg * TWOPI / 360.0f;
	}

	float radToDeg(float rad) {
		return rad * 360.0f / TWOPI;
	}

	glm::vec2 clampToMaxAngleDifference(glm::vec2 vec, glm::vec2 base, float angle) {
		float radAngle = degToRad(angle);
		glm::vec2 polVec = cartesianToPolar(vec);
		glm::vec2 polBase = cartesianToPolar(base);

		float angleDiff = polVec.y - polBase.y;
		if (angleDiff > PI) angleDiff -= TWOPI;
		else if (angleDiff < -PI) angleDiff += TWOPI;

		float newAngle = polVec.y;
		if (angleDiff > radAngle) newAngle = polBase.y + radAngle;
		if (angleDiff < -radAngle) newAngle = polBase.y - radAngle;

		return polarToCartesian(glm::vec2(1.0f, newAngle));
	}

	void writePngImage(const std::string& path, glm::ivec2 size, unsigned char* data, int channels) {
		stbi_flip_vertically_on_write(1);
		int stride = (int)ceil((float)(size.x * channels) / 4.0f) * 4;
		int success = stbi_write_png(path.c_str(), size.x, size.y, channels, data, stride);
		if (!success) {
			std::cout << "Image write failed" << std::endl;
		}
	}

	unsigned int loadTextureFile(const std::string& path, int& widthOut, int& heightOut, int& nrChannelsOut) {
		unsigned int texture;
		glGenTextures(1, &texture);
		glBindTexture(GL_TEXTURE_2D, texture);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		unsigned char* imageData = stbi_load(path.c_str(), &widthOut, &heightOut, &nrChannelsOut, 0);
		if (imageData) {
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, widthOut, heightOut, 0, GL_RGB, GL_UNSIGNED_BYTE, imageData);
			glGenerateMipmap(GL_TEXTURE_2D);
		}
		else {
			std::cout << "Loading Texture " << path << "failed!" << std::endl;
		}
		stbi_image_free(imageData);
		return texture;
	}

	std::string currTimeToString() {
		auto t = std::time(nullptr);
		auto time = *std::localtime(&t);

		std::ostringstream stream;
		//stream << std::put_time(&time, "%Y-%m-%d %X");
		stream << std::put_time(&time, "%H%M%S");
		return stream.str();
	}

}