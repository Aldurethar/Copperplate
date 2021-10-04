#pragma once

#include "mesh.h"

namespace Copperplate {

	struct SeedPoint {
		glm::vec3 m_Pos;
		Face& m_Face;
		float m_Importance;
	};
	
	void CreateSeedPoints(std::vector<SeedPoint>& outSeedPoints, Mesh& mesh, int totalPoints, int maxPointsPerFace);

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