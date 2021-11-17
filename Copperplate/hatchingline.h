#pragma once

#include "mesh.h"
#include <unordered_set>
#include <map>
#include "image.h"

namespace Copperplate {

	// Forward declaration
	struct ScreenSpaceSeed;

	struct HatchingLine {
		int m_NumPoints;
		int m_StartSeed;
		std::vector<glm::vec2> m_Points;
		std::vector<ScreenSpaceSeed*> m_AssociatedSeeds;
	};


}