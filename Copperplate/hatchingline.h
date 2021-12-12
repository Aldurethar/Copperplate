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
		void MovePointsTo(const std::deque<glm::vec2>& newPoints);

		void PruneFront();
		void PruneBack();
		HatchingLine* SplitFromCollision();
		HatchingLine* SplitFromOcclusion();
		HatchingLine* SplitSharpBend();
		HatchingLine* Split(int splitIndex);
		
		void RemoveFromFront(int count);
		void RemoveFromBack(int count);

		void ExtendFront(const std::vector<glm::vec2>& newPoints);
		void ExtendBack(const std::vector<glm::vec2>& newPoints);

		void ReplaceSeeds(const std::vector<ScreenSpaceSeed*>& newSeeds);

		bool NeedsResampling();
		bool HasVisibleSeeds();
		bool HasMiddleCollision();
		bool HasMiddleOcclusion();
		bool HasSharpBend();
		
		bool HasChanged() const { return m_HasChanged; };
		void ResetChangedFlag();

		glm::vec2 getDirAt(glm::vec2 pos);
		std::vector<ScreenSpaceSeed*> getSeedsForPoint(int index);
		glm::vec2 getSegmentDir(int index);

		const std::deque<glm::vec2>& getPoints() const { return m_Points; };
		const std::deque<ScreenSpaceSeed*>& getSeeds() const { return m_AssociatedSeeds; };
		const std::vector<glm::vec2> getPointsBeforeChange() const { return m_PointsBeforeChange; };


	private:
		
		int m_NumPoints;
		std::deque<glm::vec2> m_Points;
		std::deque<ScreenSpaceSeed*> m_AssociatedSeeds;
		std::deque<int> m_SeedPlacements;
		bool m_HasChanged;
		std::vector<glm::vec2> m_PointsBeforeChange;
		Hatching* m_Hatching;
		
		void SetChangedFlag();
		void SetPointsBeforeChange(const std::vector<glm::vec2>& points);
	};


}