#pragma once

namespace Copperplate {

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