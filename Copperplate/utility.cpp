#pragma once

#include "utility.h"

#include <random>
#include <queue>

namespace Copperplate {

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

}