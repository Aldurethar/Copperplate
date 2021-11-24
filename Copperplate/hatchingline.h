#pragma once

#include "core.h"
#include <vector>
#include <deque>
#include <glm\ext\vector_float2.hpp>

namespace Copperplate {

	// Forward declaration
	struct ScreenSpaceSeed;
	class Hatching;

	class HatchingLine {

	public:
		HatchingLine(const std::vector<glm::vec2>& points, const std::vector<ScreenSpaceSeed*>& seeds, Hatching* hatching);

		void Resample();

		void PruneFront();
		void PruneBack();
		HatchingLine* SplitFromCollision();
		HatchingLine* SplitFromOcclusion();
		HatchingLine* SplitSharpBend();
		HatchingLine* Split(int splitIndex);

		bool NeedsResampling();
		bool HasVisibleSeeds();
		bool HasMiddleCollision();
		bool HasMiddleOcclusion();
		bool HasSharpBend();
		
		std::deque<glm::vec2>& getPoints() { return m_Points; };
		const std::deque<ScreenSpaceSeed*>& getSeeds() { return m_AssociatedSeeds; };

	private:

		void RemoveFromFront(int count);
		void RemoveFromBack(int count);

		int m_NumPoints;
		std::deque<glm::vec2> m_Points;
		std::deque<ScreenSpaceSeed*> m_AssociatedSeeds;
		std::deque<int> m_SeedPlacements;
		Hatching* m_Hatching;
	};


}