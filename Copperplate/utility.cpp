#pragma once

#include "utility.h"

#include <random>
#include <queue>

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
		return glm::vec2(vec.length(), a);
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

}