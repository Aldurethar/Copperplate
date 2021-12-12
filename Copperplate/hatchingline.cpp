#pragma once

#include "hatchingline.h"
#include "hatching.h"


namespace Copperplate {

	HatchingLine::HatchingLine(const std::vector<glm::vec2>& points, const std::vector<ScreenSpaceSeed*>& seeds, Hatching* hatching) {
		m_Hatching = hatching;
		m_NumPoints = points.size();
		m_Points = std::deque<glm::vec2>();
		m_AssociatedSeeds = std::deque<ScreenSpaceSeed*>();
		m_SeedPlacements = std::deque<int>();
		m_HasChanged = true;
		m_PointsBeforeChange = std::vector<glm::vec2>();

		for (int i = 0; i < m_NumPoints; i++) {
			m_Points.push_back(points[i]);
		}

		for (ScreenSpaceSeed* seed : seeds) {
			m_AssociatedSeeds.push_back(seed);
		}
		int currPoint = 0;
		for (int i = 0; i < m_AssociatedSeeds.size(); i++) {
			if (currPoint == m_Points.size() - 1) {
				m_SeedPlacements.push_back(currPoint);
			}
			else {
				for (; currPoint < m_Points.size() - 1; currPoint++) {
					float dist = glm::distance(m_Points[currPoint], m_AssociatedSeeds[i]->m_Pos);
					float nextDist = glm::distance(m_Points[currPoint + 1], m_AssociatedSeeds[i]->m_Pos);
					if (nextDist > dist) {
						m_SeedPlacements.push_back(currPoint);
						break;
					}
				}
				if (currPoint == m_Points.size() - 1) {
					m_SeedPlacements.push_back(currPoint);
				}
			}
		}	
		assert(m_AssociatedSeeds.size() == m_SeedPlacements.size());
	}

	void HatchingLine::Resample() {
		assert(m_AssociatedSeeds.size() == m_SeedPlacements.size());

		SetChangedFlag();

		std::deque<glm::vec2> newPoints;
		std::deque<ScreenSpaceSeed*> newSeeds;
		std::deque<int> newSeedPlacements;
		glm::vec2 lastPoint = m_Points[0];
		newPoints.push_back(lastPoint);
		int currSeed = 0;

		for (int i = 1; i < m_NumPoints; i++) {
			float distance = glm::distance(m_Points[i], lastPoint);
			if (distance > Hatching::ExtendRadius) {
				// if the distance is too big, add additional points
				int numSteps = ceil(distance / Hatching::ExtendRadius);
				glm::vec2 step = (m_Points[i] - m_Points[i - 1]) / (float)numSteps;
				for (int j = 1; j <= numSteps; j++) {
					lastPoint = lastPoint + step;
					newPoints.push_back(lastPoint);
				}

				// distribute the seed points to the newly added points correctly
				std::list<ScreenSpaceSeed*> affectedSeeds;
				while (currSeed < m_AssociatedSeeds.size() && m_SeedPlacements[currSeed] <= i) {
					affectedSeeds.push_back(m_AssociatedSeeds[currSeed]);
					currSeed++;
				}
				while (!affectedSeeds.empty()) {
					ScreenSpaceSeed* closest;
					float closestDist = 1000.0f;
					for (ScreenSpaceSeed* seed : affectedSeeds) {
						float seedDist = glm::distance(seed->m_Pos, m_Points[i - 1]);
						if (seedDist < closestDist) {
							closest = seed;
							closestDist = seedDist;
						}
					}
					affectedSeeds.remove(closest);
					int offset = round(glm::length(closest->m_Pos - m_Points[i - 1]) / glm::length(step));
					if (offset > numSteps) offset = numSteps;
					newSeeds.push_back(closest);
					newSeedPlacements.push_back(newPoints.size() - 1 - numSteps + offset);
				}
			}
			else if (distance < Hatching::ExtendRadius * 0.5f) {
				// if the distance is too small, skip the current point
				while (currSeed < m_AssociatedSeeds.size() && m_SeedPlacements[currSeed] <= i) {
					newSeeds.push_back(m_AssociatedSeeds[currSeed]);
					newSeedPlacements.push_back(newPoints.size() - 1);
					currSeed++;
				}
			}
			else {
				lastPoint = m_Points[i];
				newPoints.push_back(lastPoint);
				while (currSeed < m_AssociatedSeeds.size() && m_SeedPlacements[currSeed] <= i) {
					newSeeds.push_back(m_AssociatedSeeds[currSeed]);
					newSeedPlacements.push_back(newPoints.size() - 1);
					currSeed++;
				}
			}
		}

		m_Points = newPoints;
		m_NumPoints = m_Points.size();
		m_AssociatedSeeds = newSeeds;
		m_SeedPlacements = newSeedPlacements;
	}

	void HatchingLine::MovePointsTo(const std::deque<glm::vec2>& newPoints) {
		assert(newPoints.size() == m_NumPoints);

		SetChangedFlag();

		for (int i = 0; i < m_NumPoints; i++) {
			m_Points[i] = newPoints[i];
		}
	}

	void HatchingLine::PruneFront() {
		int count = 0;
		while (!m_AssociatedSeeds.empty() && !m_AssociatedSeeds.front()->m_Visible) {
			count = m_SeedPlacements.front();
			m_AssociatedSeeds.pop_front();
			m_SeedPlacements.pop_front();
			RemoveFromFront(count + 1);
		}
		if (!m_AssociatedSeeds.empty()) {
			count = 0;
			for (int i = 0; i < m_SeedPlacements.front(); i++) {
				if (m_Hatching->HasCollision(m_Points[i], true)) {
					count = i + 1;
				}
			}
			RemoveFromFront(count);
		}
	}

	void HatchingLine::PruneBack() {
		int count = 0;
		while (!m_AssociatedSeeds.empty() && !m_AssociatedSeeds.back()->m_Visible) {
			count = m_Points.size() - m_SeedPlacements.back();
			m_AssociatedSeeds.pop_back();
			m_SeedPlacements.pop_back();
			RemoveFromBack(count);
		}
		if (!m_AssociatedSeeds.empty()) {
			count = 0;
			for (int i = m_Points.size() - 1; i > m_SeedPlacements.back(); i--) {
				if (m_Hatching->HasCollision(m_Points[i], true)) {
					count = m_Points.size() - i;
				}
			}
			RemoveFromBack(count);
		}
	}

	HatchingLine* HatchingLine::SplitFromCollision() {
		int collisionIndex = 0;
		for (int i = m_SeedPlacements.front(); i <= m_SeedPlacements.back(); i++) {
			if (m_Hatching->HasCollision(m_Points[i], true)) {
				collisionIndex = i;
				break;
			}
		}
		if (collisionIndex == 0) {
			RemoveFromFront(1);
			return nullptr;
		}
		else if (collisionIndex == m_NumPoints - 1) {
			RemoveFromBack(1);
			return nullptr;
		}
		return Split(collisionIndex);
	}

	HatchingLine* HatchingLine::SplitFromOcclusion() {
		int occlusionIndex = 0;
		for (int i = 0; i < m_AssociatedSeeds.size(); i++) {
			if (!m_AssociatedSeeds[i]->m_Visible) {
				occlusionIndex = m_SeedPlacements[i];
				break;
			}
		}
		if (occlusionIndex == 0) {
			RemoveFromFront(1);
			return nullptr;
		}
		else if (occlusionIndex == m_NumPoints - 1) {
			RemoveFromBack(1);
			return nullptr;
		}

		return Split(occlusionIndex);
	}

	HatchingLine* HatchingLine::SplitSharpBend()	{
		int splitIndex = 0;
		for (int i = 1; i < m_Points.size() - 1; i++) {
			glm::vec2 a = glm::normalize(m_Points[i] - m_Points[i - 1]);
			glm::vec2 b = glm::normalize(m_Points[i + 1] - m_Points[i]);
			if (glm::dot(a, b) < cos(Hatching::SplitAngle)) {
				splitIndex = i;
				break;
			}
		}
		return Split(splitIndex);
	}

	HatchingLine* HatchingLine::Split(int splitIndex) {
		assert(splitIndex > 0);
		assert(splitIndex < m_NumPoints - 1);

		SetChangedFlag();

		std::vector<glm::vec2> restPoints;
		std::vector<ScreenSpaceSeed*> restSeeds;
		restPoints.reserve(m_Points.size() - splitIndex);

		for (int i = splitIndex + 1; i < m_Points.size(); i++) {
			restPoints.push_back(m_Points[i]);
		}
		for (int i = 0; i < m_AssociatedSeeds.size(); i++) {
			if (m_SeedPlacements[i] > splitIndex) {
				restSeeds.push_back(m_AssociatedSeeds[i]);
			}
		}

		RemoveFromBack(m_Points.size() - splitIndex);

		HatchingLine* rest = new HatchingLine(restPoints, restSeeds, m_Hatching);
		rest->SetPointsBeforeChange(m_PointsBeforeChange);

		return rest;
	}

	bool HatchingLine::NeedsResampling() {
		for (int i = 1; i < m_NumPoints; i++) {
			float dist = glm::distance(m_Points[i], m_Points[i - 1]);
			if (dist > Hatching::ExtendRadius || dist < (Hatching::ExtendRadius * 0.5f)) {
				return true;
			}
		}
		return false;
	}

	bool HatchingLine::HasVisibleSeeds() {
		bool result = false;
		for (ScreenSpaceSeed* seed : m_AssociatedSeeds) {
			if (seed->m_Visible) result = true;
		}
		return result;
	}

	bool HatchingLine::HasMiddleCollision() {
		for (int i = m_SeedPlacements.front(); i <= m_SeedPlacements.back(); i++) {
			if (m_Hatching->HasCollision(m_Points[i], true)) {
				return true;
			}
		}
		return false;
	}

	bool HatchingLine::HasMiddleOcclusion() {
		for (int i = 0; i < m_AssociatedSeeds.size(); i++) {
			if (!m_AssociatedSeeds[i]->m_Visible) {
				return true;
			}
		}
		return false;
	}

	bool HatchingLine::HasSharpBend() {
		for (int i = 1; i < m_Points.size() - 1; i++) {
			glm::vec2 a = glm::normalize(m_Points[i] - m_Points[i - 1]);		
			glm::vec2 b = glm::normalize(m_Points[i + 1] - m_Points[i]);
			if (glm::dot(a, b) < cos(Hatching::SplitAngle)) {
				return true;
			}
		}
		return false;
	}

	void HatchingLine::RemoveFromFront(int count) {
		if (count > 0) {

			SetChangedFlag();

			m_Points.erase(m_Points.begin(), m_Points.begin() + count);
			while (!m_SeedPlacements.empty() && m_SeedPlacements.front() < count) {
				m_AssociatedSeeds.pop_front();
				m_SeedPlacements.pop_front();
			}
			for (int& placement : m_SeedPlacements) {
				placement -= count;
			}
			m_NumPoints = m_Points.size();
		}
	}

	void HatchingLine::RemoveFromBack(int count) {
		if (count > 0) {

			SetChangedFlag();

			int offset = m_Points.size() - count;
			m_Points.erase(m_Points.begin() + offset, m_Points.end());
			while (!m_SeedPlacements.empty() && m_SeedPlacements.back() >= offset) {
				m_AssociatedSeeds.pop_back();
				m_SeedPlacements.pop_back();
			}
			m_NumPoints = m_Points.size();
		}
	}

	void HatchingLine::ExtendFront(const std::vector<glm::vec2>& newPoints) {
		if (newPoints.size() > 0) {
			SetChangedFlag();

			for (glm::vec2 point : newPoints) {
				m_Points.push_front(point);
			}
			for (int& placement : m_SeedPlacements) {
				placement += newPoints.size();
			}
			m_NumPoints = m_Points.size();
		}
	}

	void HatchingLine::ExtendBack(const std::vector<glm::vec2>& newPoints) {
		if (newPoints.size() > 0) {
			SetChangedFlag();

			for (glm::vec2 point : newPoints) {
				m_Points.push_back(point);
			}
			m_NumPoints = m_Points.size();
		}
	}

	void HatchingLine::ReplaceSeeds(const std::vector<ScreenSpaceSeed*>& newSeeds) {
		m_AssociatedSeeds.clear();
		m_SeedPlacements.clear();
		for (ScreenSpaceSeed* seed : newSeeds) {
			m_AssociatedSeeds.push_back(seed);
		}
		int currPoint = 0;
		for (int i = 0; i < m_AssociatedSeeds.size(); i++) {
			if (currPoint == m_Points.size() - 1) {
				m_SeedPlacements.push_back(currPoint);
			}
			else {
				for (; currPoint < m_Points.size() - 1; currPoint++) {
					float dist = glm::distance(m_Points[currPoint], m_AssociatedSeeds[i]->m_Pos);
					float nextDist = glm::distance(m_Points[currPoint + 1], m_AssociatedSeeds[i]->m_Pos);
					if (nextDist > dist) {
						m_SeedPlacements.push_back(currPoint);
						break;
					}
				}
				if (currPoint == m_Points.size() - 1) {
					m_SeedPlacements.push_back(currPoint);
				}
			}
		}
		assert(newSeeds.size() == m_AssociatedSeeds.size());
		assert(m_AssociatedSeeds.size() == m_SeedPlacements.size());
	}

	glm::vec2 HatchingLine::getDirAt(glm::vec2 pos) {
		glm::vec2 result;
		int closestIndex = 0;
		float closestDist = 1000.0f;
		for (int i = 0; i < m_NumPoints; i++) {
			float dist = glm::distance(pos, m_Points[i]);
			if (dist < closestDist) {
				closestIndex = i;
				closestDist = dist;
			}
		}
		if (closestIndex == 0)
			return glm::normalize(m_Points[closestIndex + 1] - m_Points[closestIndex]);

		if (closestIndex == m_NumPoints - 1)
			return glm::normalize(m_Points[closestIndex] - m_Points[closestIndex - 1]);

		return glm::normalize(m_Points[closestIndex + 1] - m_Points[closestIndex - 1]);
	}

	std::vector<ScreenSpaceSeed*> HatchingLine::getSeedsForPoint(int index) {
		std::vector<ScreenSpaceSeed*> result;
		for (int i = 0; i < m_SeedPlacements.size(); i++) {
			if (m_SeedPlacements[i] == index)
				result.push_back(m_AssociatedSeeds[i]);
		}
		
		return result;
	}

	glm::vec2 HatchingLine::getSegmentDir(int index) {
		int i1 = index;
		int i2 = index + 1;
		if (index < 0) {
			i1 = 0;
			i2 = 1;
		}
		if (index >= m_NumPoints - 1) {
			i1 = m_NumPoints - 2;
			i2 = m_NumPoints - 1;
		}
		glm::vec2 p1 = m_Points[i1];
		glm::vec2 p2 = m_Points[i2];
		float dist = glm::distance(p1, p2);
		return (p2 - p1) / dist;
	}

	void HatchingLine::ResetChangedFlag() {
		m_PointsBeforeChange.clear();
		m_HasChanged = false;
	}

	// PRIVATE FUNCTIONS

	void HatchingLine::SetChangedFlag() {
		if (!m_HasChanged) {
			m_PointsBeforeChange.clear();
			m_PointsBeforeChange.reserve(m_NumPoints);
			for (int i = 0; i < m_NumPoints; i++) {
				m_PointsBeforeChange.push_back(m_Points[i]);
			}
			m_HasChanged = true;
		}
	}

	void HatchingLine::SetPointsBeforeChange(const std::vector<glm::vec2>& points) {
		m_PointsBeforeChange = points;
		m_HasChanged = true;
	}
}