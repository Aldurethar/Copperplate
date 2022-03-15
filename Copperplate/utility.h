#pragma once
#include <glm\ext\vector_float2.hpp>
#include <glm\ext\vector_int2.hpp>
#include <string>

namespace Copperplate {

	glm::vec2 cartesianToPolar(glm::vec2 vec);

	glm::vec2 polarToCartesian(glm::vec2 polVec);

	float degToRad(float deg);

	float radToDeg(float rad);

	glm::vec2 clampToMaxAngleDifference(glm::vec2 vec, glm::vec2 base, float angle);

	void writePngImage(const std::string& path, glm::ivec2 size, unsigned char* data, int channels);

	unsigned int loadTextureFile(const std::string& path, int& widthOut, int& heightOut, int& nrChannelsOut);

	std::string currTimeToString();

	class HaltonSequence {
	public:

		HaltonSequence(int base);

		void Skip(int amount);

		float NextNumber();

	private:

		int m_Base;
		int m_Numerator;
		int m_Denominator;
	};
}