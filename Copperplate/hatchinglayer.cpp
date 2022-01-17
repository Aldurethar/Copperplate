#pragma once
#include "hatchinglayer.h"
#include "hatching.h"
#include "utility.h"

#include <random>
#include <queue>

namespace Copperplate {

	HatchingLayer::HatchingLayer(glm::ivec2 gridSize, Hatching& hatching, float minLineWidth, float maxLineWidth, float minShade, float maxShade, EHatchingDirections direction)
		: m_GridSize(gridSize)
		, m_Hatching(hatching)
		, m_MinLineWidth(minLineWidth)
		, m_MaxLineWidth(maxLineWidth)
		, m_MinShade(minShade)
		, m_MaxShade(maxShade)
		, m_Direction(direction) {

		m_UnusedSeedsGrid = std::vector<std::unordered_set<ScreenSpaceSeed*>>();
		m_UnusedSeedsGrid.reserve(gridSize.x * gridSize.y);

		m_CollisionPointsGrid = std::vector<std::unordered_set<CollisionPoint>>();
		m_CollisionPointsGrid.reserve(gridSize.x * gridSize.y);

		for (int i = 0; i < gridSize.x; i++) {
			for (int j = 0; j < gridSize.y; j++) {
				m_UnusedSeedsGrid.push_back(std::unordered_set<ScreenSpaceSeed*>());
				m_CollisionPointsGrid.push_back(std::unordered_set<CollisionPoint>());
			}
		}

		//setup opengl buffers
		// Hatching Lines
		glGenVertexArrays(1, &m_LinesVAO);
		glGenBuffers(1, &m_LinesVertexBuffer);
		glGenBuffers(1, &m_LinesIndexBuffer);

		glBindVertexArray(m_LinesVAO);
		glBindBuffer(GL_ARRAY_BUFFER, m_LinesVertexBuffer);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_LinesIndexBuffer);

		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (GLvoid*)0);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (GLvoid*)(2 * sizeof(float)));

		glBindVertexArray(0);
		glCheckError();

		// Collision Points
		glGenVertexArrays(1, &m_CollisionVAO);
		glGenBuffers(1, &m_CollisionVBO);

		glBindVertexArray(m_CollisionVAO);
		glBindBuffer(GL_ARRAY_BUFFER, m_CollisionVBO);

		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (GLvoid*)0);

		glCheckError();
	}

	void HatchingLayer::ResetCollisions() {
		for (std::unordered_set<CollisionPoint>& gridCell : m_CollisionPointsGrid) {
			gridCell.clear();
		}
	}
	
	void HatchingLayer::AddContourCollision(const std::vector<glm::vec2>& contourSegments) {
		for (int i = 0; i < contourSegments.size(); i += 2) {
			glm::vec2 pos1 = m_Hatching.ViewToScreen(contourSegments[i]);
			glm::vec2 pos2 = m_Hatching.ViewToScreen(contourSegments[i + 1]);
			float length = glm::distance(pos1, pos2);
			int steps = ceil(length / (m_Hatching.CollisionRadius * 0.8f));
			for (int j = 0; j <= steps; j++) {
				float p = j / (float)steps;
				glm::vec2 pos = (1.0f - p) * pos1 + p * pos2;
				if (m_Hatching.IsInBounds(pos))
					AddCollisionPoint(pos, true, nullptr);
			}
		}
	}

	void HatchingLayer::Update() {
		ResetUnusedSeeds();

		if (DisplaySettings::RegenerateHatching) {
			m_HatchingLines.clear();
		}

		UpdateLines();

		FillGLBuffers();
	}

	void HatchingLayer::Draw(Shared<Shader> shader) {
		glBindVertexArray(m_LinesVAO);
		shader->SetFloat("minLineWidth", m_MinLineWidth);
		shader->SetFloat("maxLineWidth", m_MaxLineWidth);
		shader->SetFloat("minShade", m_MinShade);
		shader->SetFloat("maxShade", m_MaxShade);
		shader->UpdateUniforms();
		glDrawElements(GL_LINES, m_NumLinesIndices, GL_UNSIGNED_INT, 0);
	}

	void HatchingLayer::DrawCollision() {
		glBindVertexArray(m_CollisionVAO);
		glDrawArrays(GL_POINTS, 0, m_NumCollisionPoints);
	}

	bool HatchingLayer::HasCollision(glm::vec2 screenPos, bool onlyContours) {
		if (!m_Hatching.IsInBounds(screenPos)) return true;
		glm::ivec2 gridCenter = m_Hatching.ScreenPosToGridPos(screenPos);
		for (int x = -1; x <= 1; x++) {
			for (int y = -1; y <= 1; y++) {
				glm::ivec2 gridPos = gridCenter + glm::ivec2(x, y);
				if (gridPos.x >= 0 && gridPos.x < m_GridSize.x && gridPos.y >= 0 && gridPos.y < m_GridSize.y) {
					std::unordered_set<CollisionPoint>& gridCell = *GetCollisionPoints(gridPos);
					for (CollisionPoint point : gridCell) {
						float dist = glm::distance(screenPos, point.m_Pos);
						if (dist < m_Hatching.CollisionRadius) {
							if (!onlyContours || point.m_isContour) return true;
						}
					}
				}
			}
		}
		return false;
	}

	// PRIVATE FUNCTIONS //

	void HatchingLayer::UpdateLines() {
		// Move Hatching Lines based on the Motion Field and resample them if necessary
		SnakesAdvect();

		// Resample Lines if necessary and update their associated SeedPoints
		SnakesResample();

		// Deform Lines slightly to ensure they follow the curvature
		SnakesRelax();

		// Delete and Split Lines where necessary
		SnakesDelete();
		SnakesSplit();

		// From here on we need Collision from the Hatching Lines
		SnakesUpdateCollision();

		SnakesTrim();
		SnakesExtend();
		SnakesMerge();

		// Before creating new Lines we need to know which Seeds are not covered by a line
		UpdateUnusedSeeds();
		SnakesInsert();

		//Sanity check
		for (HatchingLine line : m_HatchingLines) {
			assert(line.getSeeds().size() > 0);
		}
	}

	void HatchingLayer::SnakesAdvect() {
		for (HatchingLine& line : m_HatchingLines) {
			const std::deque<glm::vec2>& points = line.getPoints();
			std::deque<glm::vec2> newPoints;
			for (int i = 0; i < points.size(); i++) {
				glm::vec2 point = points[i];
				glm::vec2 movement = m_Hatching.SampleMovement(point);
				newPoints.push_back(point + movement);
			}
			line.MovePointsTo(newPoints);
		}
	}

	void HatchingLayer::SnakesResample() {
		for (HatchingLine& line : m_HatchingLines) {
			if (line.NeedsResampling()) {
				line.Resample();
			}
			UpdateLineSeeds(line);
		}
	}

	void HatchingLayer::SnakesRelax() {
		int numSteps = m_Hatching.NumOptiSteps;
		float stepSize = m_Hatching.OptiStepSize;
		for (HatchingLine& line : m_HatchingLines) {
			const std::deque<glm::vec2>& originalPoints = line.getPoints();
			std::deque<glm::vec2> currPoints = originalPoints; //Copy of the starting points to be changed
			for (int iteration = 0; iteration < numSteps; iteration++) {
				std::deque<glm::vec2> newPoints;
				for (int i = 0; i < currPoints.size(); i++) {
					glm::vec2 bestPos = currPoints[i];
					float bestEval = 1000.0f;
					for (int x = -1; x <= 1; x++) {
						for (int y = -1; y <= 1; y++) {
							glm::vec2 candidatePos = currPoints[i] + glm::vec2(x * stepSize, y * stepSize);
							float eval = EvaluatePointPos(line, i, candidatePos, currPoints);
							if (eval < bestEval) {
								bestEval = eval;
								bestPos = candidatePos;
							}
						}
					}
					newPoints.push_back(bestPos);
				}
				for (int i = 0; i < currPoints.size(); i++) {
					currPoints[i] = newPoints[i];
				}
			}
			line.MovePointsTo(currPoints);
		}
	}

	void HatchingLayer::SnakesDelete() {
		for (auto it = m_HatchingLines.begin(); it != m_HatchingLines.end(); ) {
			HatchingLine& line = *it;
			bool lineAlive = line.HasVisibleSeeds();
			if (line.getPoints().size() <= 1) lineAlive = false;

			if (lineAlive) {
				line.PruneFront();
				lineAlive = line.HasVisibleSeeds();
			}

			if (lineAlive && line.HasMiddleCollision()) {
				HatchingLine* rest = line.SplitFromCollision();
				if (rest) m_HatchingLines.push_back(*rest);
				lineAlive = line.HasVisibleSeeds();
			}

			if (lineAlive && line.HasMiddleOcclusion()) {
				HatchingLine* rest = line.SplitFromOcclusion();
				if (rest) m_HatchingLines.push_back(*rest);
				lineAlive = line.HasVisibleSeeds();
			}

			if (lineAlive) {
				line.PruneBack();
				lineAlive = line.HasVisibleSeeds();
			}

			if (line.getPoints().size() <= 1) lineAlive = false;

			// Lines with no visible Seeds get removed
			if (!lineAlive) {
				RemoveLineCollision(line);
				it = m_HatchingLines.erase(it);
			}
			else {
				it++;
			}
		}
	}

	void HatchingLayer::SnakesSplit() {
		for (auto it = m_HatchingLines.begin(); it != m_HatchingLines.end(); ) {
			HatchingLine& line = *it;
			if (line.HasSharpBend()) {
				HatchingLine* rest = line.SplitSharpBend();
				if (rest && rest->HasVisibleSeeds() && rest->getPoints().size() > 1)
					m_HatchingLines.push_back(*rest);
			}
			if (line.HasVisibleSeeds() && line.getPoints().size() > 1) {
				it++;
			}
			else {
				RemoveLineCollision(line);
				it = m_HatchingLines.erase(it);
			}
		}
	}

	void HatchingLayer::SnakesTrim() {
		for (auto it = m_HatchingLines.begin(); it != m_HatchingLines.end();) {
			HatchingLine& line = *it;
			const std::deque<glm::vec2>& points = line.getPoints();
			int count = 0;
			for (int i = 0; i < points.size() - 1; i++) {
				glm::vec2 dir = points[i + 1] - points[i];
				if (!HasParallelNearby(points[i], dir, line)) {
					count = i;
					break;
				}
			}
			line.RemoveFromFront(count);

			count = 0;
			for (int i = points.size() - 1; i > 0; i--) {
				glm::vec2 dir = points[i] - points[i - 1];
				if (!HasParallelNearby(points[i], dir, line)) {
					count = points.size() - i - 1;
					break;
				}
			}
			line.RemoveFromBack(count);

			if (line.HasVisibleSeeds() && line.getPoints().size() > 1) {
				UpdateLineCollision(line);
				it++;
			}
			else {
				RemoveLineCollision(line);
				it = m_HatchingLines.erase(it);
			}
		}
	}

	void HatchingLayer::SnakesExtend() {
		for (HatchingLine& line : m_HatchingLines) {
			const std::deque<glm::vec2>& points = line.getPoints();
			int numOldPoints = points.size();
			const std::deque<ScreenSpaceSeed*>& oldSeeds = line.getSeeds();

			glm::vec2 first = points[0];
			glm::vec2 second = points[1];
			std::vector<glm::vec2> extensionFront = ExtendLine(first, second);

			glm::vec2 last = points[points.size() - 1];
			glm::vec2 secondLast = points[points.size() - 2];
			std::vector<glm::vec2> extensionBack = ExtendLine(last, secondLast);

			line.ExtendFront(extensionFront);
			line.ExtendBack(extensionBack);

			// Update associated Seeds of the line
			std::unordered_set<ScreenSpaceSeed*> newSeedsUnique;
			std::vector<ScreenSpaceSeed*> newSeeds;
			for (glm::vec2 point : extensionFront) {
				std::vector<ScreenSpaceSeed*> currSeeds = m_Hatching.FindVisibleSeedsInRadius(point, m_Hatching.CoverRadius);
				for (ScreenSpaceSeed* seed : currSeeds) {
					if (newSeedsUnique.insert(seed).second)
						newSeeds.push_back(seed);
				}
			}
			std::reverse(newSeeds.begin(), newSeeds.end());
			for (ScreenSpaceSeed* seed : oldSeeds) {
				if (newSeedsUnique.insert(seed).second)
					newSeeds.push_back(seed);
			}
			for (glm::vec2 point : extensionBack) {
				std::vector<ScreenSpaceSeed*> currSeeds = m_Hatching.FindVisibleSeedsInRadius(point, m_Hatching.CoverRadius);
				for (ScreenSpaceSeed* seed : currSeeds) {
					if (newSeedsUnique.insert(seed).second)
						newSeeds.push_back(seed);
				}
			}
			if (newSeeds.size() != oldSeeds.size()) {
				line.ReplaceSeeds(newSeeds);
			}

			//Sanity check
			assert(line.getPoints().size() == (numOldPoints + extensionFront.size() + extensionBack.size()));

			//Update Line Collision if necessary
			UpdateLineCollision(line);
		}
	}

	void HatchingLayer::SnakesMerge() {
		std::unordered_set<HatchingLine*> linesToDelete;
		for (auto it = m_HatchingLines.begin(); it != m_HatchingLines.end(); it++) {
			HatchingLine& line = *it;
			if (linesToDelete.count(&line) == 0) {
				// Find all Candidates for merging
				std::vector<const CollisionPoint*> mergeCandidates;
				std::vector<bool> candidatesToFront;
				glm::vec2 front = line.getPoints().front();
				glm::vec2 frontSecond = line.getPoints()[1];

				std::vector<const CollisionPoint*> frontCandidates = FindMergeCandidates(front, frontSecond, &line);
				for (const CollisionPoint* candidate : frontCandidates) {
					if (linesToDelete.count(candidate->m_Line) == 0) {
						mergeCandidates.push_back(candidate);
						candidatesToFront.push_back(true);
					}
				}

				glm::vec2 back = line.getPoints().back();
				glm::vec2 backSecond = line.getPoints()[line.getPoints().size() - 2];

				std::vector<const CollisionPoint*> backCandidates = FindMergeCandidates(back, backSecond, &line);
				for (const CollisionPoint* candidate : backCandidates) {
					if (linesToDelete.count(candidate->m_Line) == 0) {
						mergeCandidates.push_back(candidate);
						candidatesToFront.push_back(false);
					}
				}

				// select the best merger candidate
				if (!mergeCandidates.empty()) {
					float bestScore = 0.0f;
					HatchingLine* bestMerge = nullptr;
					bool mergeToFront = false;
					for (int i = 0; i < mergeCandidates.size(); i++) {
						const CollisionPoint* candidate = mergeCandidates[i];
						bool candToFront = candidatesToFront[i];
						float score = EvaluateMergeCandidate(line, candidate, candToFront);

						if (score > bestScore) {
							bestScore = score;
							bestMerge = candidate->m_Line;
							mergeToFront = candToFront;
						}
						//sanity check
						assert(bestScore > 0.0f);
					}

					//Merge the two lines, mark both original lines for removal
					HatchingLine merged = line.CreateMerged(bestMerge, mergeToFront);
					m_HatchingLines.push_back(merged);
					linesToDelete.insert(&line);
					linesToDelete.insert(bestMerge);
				}
			}
		}
		//clean up all the lines marked for removal
		for (auto it = m_HatchingLines.begin(); it != m_HatchingLines.end();) {
			if (linesToDelete.count(&(*it)) > 0) {
				it = m_HatchingLines.erase(it);
			}
			else {
				it++;
			}
		}
	}

	void HatchingLayer::SnakesInsert() {
		//DEBUG DISPLAY
		int maxLines = DisplaySettings::NumHatchingLines;
		if (maxLines < 0) maxLines = 999999999;

		std::queue<HatchingLine*> lineQueue;
		for (HatchingLine& line : m_HatchingLines) {
			lineQueue.push(&line);
		}

		while (m_NumUnusedSeeds > 0 && m_HatchingLines.size() <= maxLines) {
			if (lineQueue.empty()) {
				//Find highest importance unused seed
				float highestImp = 0.0f;
				ScreenSpaceSeed* candidate = nullptr;
				for (std::unordered_set<ScreenSpaceSeed*>& gridCell : m_UnusedSeedsGrid) {
					for (ScreenSpaceSeed* seed : gridCell) {
						if (seed->m_Importance > highestImp) {
							highestImp = seed->m_Importance;
							candidate = seed;
						}
					}
				}
				assert(candidate != nullptr);

				//Construct a line from it and add it to the queue
				HatchingLine newLine = ConstructLine(candidate);
				UpdateLineCollision(newLine);
				m_HatchingLines.push_back(newLine);
				lineQueue.push(&m_HatchingLines.back());
			}
			else {
				//Algorithm from Jobard and Lefer, 1997
				ScreenSpaceSeed* candidate = FindSeedCandidate(lineQueue.front());
				if (candidate) {
					HatchingLine newLine = ConstructLine(candidate);
					UpdateLineCollision(newLine);
					m_HatchingLines.push_back(newLine);
					lineQueue.push(&m_HatchingLines.back());
				}
				else {
					lineQueue.pop();
				}
			}
		}
	}
	
	void HatchingLayer::SnakesUpdateCollision() {
		for (HatchingLine& line : m_HatchingLines) {
			UpdateLineCollision(line);
		}
	}

	HatchingLine HatchingLayer::ConstructLine(ScreenSpaceSeed* seed) {
		int maxPoints = DisplaySettings::NumPointsPerHatch;
		if (maxPoints <= 0) maxPoints = 999999;
		int pointsAdded = 0;

		std::vector<glm::vec2> linePoints;

		glm::vec2 first = seed->m_Pos;
		glm::vec2 dir = m_Hatching.GetHatchingDir(first, m_Direction);
		glm::vec2 second = first + (m_Hatching.ExtendRadius * dir);

		std::vector<glm::vec2> leftPoints = ExtendLine(first, second);
		for (auto it = leftPoints.rbegin(); it != leftPoints.rend(); it++) {
			if (pointsAdded < maxPoints) {
				linePoints.push_back(*it);
				pointsAdded++;
			}
		}
		linePoints.push_back(first);
		linePoints.push_back(second);
		pointsAdded = 0;
		std::vector<glm::vec2> rightPoints = ExtendLine(second, first);
		for (auto it = rightPoints.begin(); it != rightPoints.end(); it++) {
			if (pointsAdded < maxPoints) {
				linePoints.push_back(*it);
				pointsAdded++;
			}
		}

		std::unordered_set<ScreenSpaceSeed*> seedsUnique;
		std::vector<ScreenSpaceSeed*> associatedSeeds;

		//Find all Seed Points near the line
		for (glm::vec2 point : linePoints) {
			std::vector<ScreenSpaceSeed*> closestSeeds = m_Hatching.FindVisibleSeedsInRadius(point, m_Hatching.CoverRadius);
			for (ScreenSpaceSeed* seed : closestSeeds) {
				if (seedsUnique.insert(seed).second) {
					glm::ivec2 gridPos = m_Hatching.ScreenPosToGridPos(seed->m_Pos);
					m_NumUnusedSeeds -= GetUnusedScreenSeeds(gridPos)->erase(seed);
					associatedSeeds.push_back(seed);
				}
			}
		}

		return HatchingLine(linePoints, associatedSeeds, *this);
	}

	std::vector<glm::vec2> HatchingLayer::ExtendLine(glm::vec2 tip, glm::vec2 second) {
		std::vector<glm::vec2> newPoints;
		bool finished = false;

		glm::vec2 currPos = tip;
		glm::vec2 dir = glm::normalize(tip - second);

		while (!finished) {
			float bestScore = 0.0f;
			glm::vec2 bestCandidate;
			for (int i = -2; i <= 2; i++) {
				glm::vec2 deviation = glm::vec2(1.0f, i * Hatching::ParallelAngle * 0.5f); //in polar form
				glm::vec2 candidateDir = glm::normalize(polarToCartesian(cartesianToPolar(dir) + deviation));
				glm::vec2 candidatePos = currPos + candidateDir * Hatching::ExtendRadius;
				glm::vec2 candidateSampleDir = m_Hatching.GetHatchingDir(candidatePos, m_Direction);
				if (glm::dot(candidateDir, candidateSampleDir) < 0) candidateSampleDir = -candidateSampleDir;

				if (glm::dot(candidateDir, candidateSampleDir) > cos(Hatching::ParallelAngle)) {
					float score = std::min(glm::dot(dir, candidateDir), glm::dot(candidateDir, candidateSampleDir)) - cos(Hatching::ParallelAngle);
					if (score > bestScore) {
						bestScore = score;
						bestCandidate = candidatePos;
					}
				}
			}

			if (bestScore > 0.0f && !HasCollision(bestCandidate, false)) {
				newPoints.push_back(bestCandidate);
				dir = glm::normalize(bestCandidate - currPos);
				currPos = bestCandidate;
			}
			else {
				finished = true;
			}
		}
		return newPoints;
	}

	void HatchingLayer::UpdateLineSeeds(HatchingLine& line) {
		// Find the new set of Seeds
		std::unordered_set<ScreenSpaceSeed*> newSeedsUnique;
		std::vector<ScreenSpaceSeed*> newSeeds;
		const std::deque<glm::vec2>& points = line.getPoints();
		for (glm::vec2 point : points) {
			std::vector<ScreenSpaceSeed*> closestSeeds = m_Hatching.FindVisibleSeedsInRadius(point, Hatching::CoverRadius);
			for (ScreenSpaceSeed* seed : closestSeeds) {
				if (newSeedsUnique.insert(seed).second) {
					newSeeds.push_back(seed);
				}
			}
		}
		line.ReplaceSeeds(newSeeds);
	}

	ScreenSpaceSeed* HatchingLayer::FindSeedCandidate(HatchingLine* currentLine) {
		float closestDistance = 1000.0f;
		ScreenSpaceSeed* candidate = nullptr;
		const std::deque<glm::vec2>& currentPoints = currentLine->getPoints();
		for (int i = 0; i < currentPoints.size(); i++) {
			glm::ivec2 gridPos = m_Hatching.ScreenPosToGridPos(currentPoints[i]);
			for (int x = -1; x <= 1; x++) {
				for (int y = -1; y <= 1; y++) {
					glm::ivec2 currGridPos = glm::clamp(gridPos + glm::ivec2(x, y), glm::ivec2(0), (m_GridSize - glm::ivec2(1)));
					std::unordered_set<ScreenSpaceSeed*>& gridCell = *GetUnusedScreenSeeds(currGridPos);
					for (ScreenSpaceSeed* currSeed : gridCell) {
						glm::vec2 compCoords = currSeed->m_Pos;
						float dist = glm::distance(currentPoints[i], compCoords);
						if (dist < closestDistance && dist >= Hatching::LineDistance) {
							candidate = currSeed;
							closestDistance = dist;
						}
					}
				}
			}
		}
		return candidate;
	}

	std::vector<const CollisionPoint*> HatchingLayer::FindMergeCandidates(glm::vec2 tip, glm::vec2 tipSecond, HatchingLine* line) {
		std::vector<const CollisionPoint*> mergeCandidates;
		glm::ivec2 gridCenter = m_Hatching.ScreenPosToGridPos(tip);
		for (int x = -1; x <= 1; x++) {
			for (int y = -1; y <= 1; y++) {
				glm::ivec2 gridPos = gridCenter + glm::ivec2(x, y);
				if (gridPos.x >= 0 && gridPos.x < m_GridSize.x && gridPos.y >= 0 && gridPos.y < m_GridSize.y) {
					std::unordered_set<CollisionPoint>* gridCell = GetCollisionPoints(gridPos);
					for (auto gridIt = gridCell->begin(); gridIt != gridCell->end(); gridIt++) {
						const CollisionPoint* colPoint = &(*gridIt);
						HatchingLine* colLine = colPoint->m_Line;
						if (!colPoint->m_isContour && colLine != line) {
							float dist = glm::distance(tip, colPoint->m_Pos);

							glm::vec2 candTip = colPoint->m_Pos;
							glm::vec2 candSecond;
							if (colLine->getPoints().front() == colPoint->m_Pos) {
								candSecond = colLine->getPoints()[1];
							}
							else {
								candSecond = colLine->getPoints()[colLine->getPoints().size() - 2];
							}

							glm::vec2 tipDir = glm::normalize(tip - tipSecond);
							glm::vec2 newTipDir = glm::normalize(candTip - tipSecond);
							glm::vec2 candDir = glm::normalize(candSecond - candTip);
							glm::vec2 newCandDir = glm::normalize(candSecond - tip);

							if (dist < Hatching::MergeRadius
								&& glm::dot(tipDir, newCandDir) > cos(Hatching::ParallelAngle)
								&& glm::dot(newTipDir, candDir) > cos(Hatching::ParallelAngle)) {
								mergeCandidates.push_back(colPoint);
							}
						}
					}
				}
			}
		}
		return mergeCandidates;
	}

	float HatchingLayer::EvaluateMergeCandidate(const HatchingLine& line, const CollisionPoint* candidate, bool mergeToFront) {
		HatchingLine* candLine = candidate->m_Line;
		glm::vec2 candidateTip = candidate->m_Pos;
		glm::vec2 candidateSecond;
		if (candLine->getPoints().front() == candidateTip)
			candidateSecond = candLine->getPoints()[1];
		else
			candidateSecond = candLine->getPoints()[candLine->getPoints().size() - 2];

		glm::vec2 ownTip;
		glm::vec2 ownSecond;
		if (mergeToFront) {
			ownTip = line.getPoints()[0];
			ownSecond = line.getPoints()[1];
		}
		else {
			ownTip = line.getPoints().back();
			ownSecond = line.getPoints()[line.getPoints().size() - 2];
		}

		glm::vec2 ownDir = glm::normalize(ownTip - ownSecond);
		glm::vec2 newOwnDir = glm::normalize(candidateTip - ownSecond);
		glm::vec2 candDir = glm::normalize(candidateSecond - candidateTip);
		glm::vec2 newCandDir = glm::normalize(candidateSecond - ownTip);

		float straightScore = std::min(glm::dot(ownDir, newCandDir), glm::dot(candDir, newOwnDir)) - cos(Hatching::ParallelAngle);
		straightScore /= (1.0f - cos(Hatching::ParallelAngle));
		float closeScore = (Hatching::MergeRadius - glm::distance(ownTip, candidateTip)) / (Hatching::MergeRadius - Hatching::CoverRadius);
		float score = Hatching::ExtendStraightVsCloseWeight * straightScore + (1 - Hatching::ExtendStraightVsCloseWeight) * closeScore;
		return score;
	}

	float HatchingLayer::EvaluatePointPos(HatchingLine& line, int index, glm::vec2 pointPos, const std::deque<glm::vec2>& points) {
		const std::deque<glm::vec2>& originalPoints = line.getPoints();

		//Compute Energy for associated Seed Points
		float eSeeds = 0.0f;
		std::vector<ScreenSpaceSeed*> associatedSeeds = line.getSeedsForPoint(index);
		for (ScreenSpaceSeed* seed : associatedSeeds) {
			float dist = glm::distance(pointPos, seed->m_Pos);
			dist /= Hatching::CoverRadius;

			//float energy = exp(-2.5f * dist * dist) * dist * dist;  //this tries to get away from seeds that are already somewhat far away from the line to free them up for neighbors or new lines
			float energy = dist * dist;  //this just tries to stay close to the associated seeds
			eSeeds += energy;
		}

		//Compute Energy for Line Smoothness
		float eSmooth = 0.0f;
		if (index > 0 && index < points.size() - 1) {
			glm::vec2 prevPoint = points[index - 1];
			glm::vec2 nextPoint = points[index + 1];
			glm::vec2 prevDir = (pointPos - prevPoint) / glm::distance(pointPos, prevPoint);
			glm::vec2 nextDir = (nextPoint - pointPos) / glm::distance(nextPoint, pointPos);
			eSmooth += (1 - glm::dot(prevDir, nextDir));
		}

		//Compute Energy for following the vector field
		float eField = 0.0f;
		glm::vec2 fieldDir = m_Hatching.GetHatchingDir(pointPos, m_Direction);
		if (index > 0) {
			glm::vec2 prevPoint = points[index - 1];
			glm::vec2 prevDir = (pointPos - prevPoint) / glm::distance(pointPos, prevPoint);
			eField += (1 - abs(glm::dot(prevDir, fieldDir)));
		}
		if (index < points.size() - 1) {
			glm::vec2 nextPoint = points[index + 1];
			glm::vec2 nextDir = (nextPoint - pointPos) / glm::distance(nextPoint, pointPos);
			eField += (1 - abs(glm::dot(nextDir, fieldDir)));
		}

		//Compute Energy for keeping the line segment lengths
		float eSpring = 0.0f;
		if (index > 0) {
			float prevLength = glm::distance(points[index - 1], pointPos);
			float prevLengthOriginal = glm::distance(originalPoints[index - 1], originalPoints[index]);
			eSpring += abs(prevLengthOriginal - prevLength) / Hatching::ExtendRadius;
		}
		if (index < points.size() - 1) {
			float nextLength = glm::distance(pointPos, points[index + 1]);
			float nextLengthOriginal = glm::distance(originalPoints[index], originalPoints[index + 1]);
			eSpring += abs(nextLengthOriginal - nextLength) / Hatching::ExtendRadius;
		}

		float eTotal = eSeeds * Hatching::OptiSeedWeight + eSmooth * Hatching::OptiSmoothWeight + eField * Hatching::OptiFieldWeight + eSpring * Hatching::OptiSpringWeight;
		return eTotal;
	}

	void HatchingLayer::RemoveLineCollision(const HatchingLine& line) {
		float eps = 1e-6;
		if (line.HasChanged()) {
			const std::vector<glm::vec2>& points = line.getPointsBeforeChange();
			for (glm::vec2 point : points) {
				if (m_Hatching.IsInBounds(point)) {
					glm::ivec2 gridPos = m_Hatching.ScreenPosToGridPos(point);
					std::unordered_set<CollisionPoint>& gridCell = *GetCollisionPoints(gridPos);
					for (auto it = gridCell.begin(); it != gridCell.end(); it++) {
						const CollisionPoint& colPoint = *it;
						if (glm::distance(point, colPoint.m_Pos) < eps && colPoint.m_Line == &line) {
							gridCell.erase(it);
							break;
						}
					}
				}
			}
		}
		else {
			const std::deque<glm::vec2>& points = line.getPoints();
			for (glm::vec2 point : points) {
				if (m_Hatching.IsInBounds(point)) {
					glm::ivec2 gridPos = m_Hatching.ScreenPosToGridPos(point);
					std::unordered_set<CollisionPoint>& gridCell = *GetCollisionPoints(gridPos);
					for (auto it = gridCell.begin(); it != gridCell.end(); it++) {
						const CollisionPoint& colPoint = *it;
						if (glm::distance(point, colPoint.m_Pos) < eps && colPoint.m_Line == &line) {
							gridCell.erase(it);
							break;
						}
					}
				}
			}
		}
	}

	void HatchingLayer::UpdateLineCollision(HatchingLine& line) {
		if (line.HasChanged()) {
			RemoveLineCollision(line);

			// Add the new Collision Points
			const std::deque<glm::vec2>& points = line.getPoints();
			for (glm::vec2 point : points) {
				AddCollisionPoint(point, false, (HatchingLine * const)(&line));
			}
			line.ResetChangedFlag();
		}
	}

	void HatchingLayer::AddCollisionPoint(glm::vec2 screenPos, bool isContour, HatchingLine* line) {
		if (!m_Hatching.IsInBounds(screenPos))
			return;

		glm::ivec2 gridPos = m_Hatching.ScreenPosToGridPos(screenPos);
		std::unordered_set<CollisionPoint>& colGridCell = *GetCollisionPoints(gridPos);
		CollisionPoint newPoint = { screenPos, isContour, line };
		colGridCell.insert(newPoint);
	}

	bool HatchingLayer::HasParallelNearby(glm::vec2 point, glm::vec2 dir, const HatchingLine& line) {
		std::vector<CollisionPoint> candidates;

		glm::vec2 normDir = glm::normalize(dir);

		glm::ivec2 gridPos = m_Hatching.ScreenPosToGridPos(point);
		for (int x = -1; x <= 1; x++) {
			for (int y = -1; y <= 1; y++) {
				glm::ivec2 currGridPos = glm::clamp(gridPos + glm::ivec2(x, y), glm::ivec2(0, 0), m_GridSize - glm::ivec2(1, 1));
				std::unordered_set<CollisionPoint>& gridCell = *GetCollisionPoints(currGridPos);
				for (const CollisionPoint& colPoint : gridCell) {
					if (glm::distance(point, colPoint.m_Pos) < Hatching::TrimRadius
						&& !colPoint.m_isContour && colPoint.m_Line != &line)
						candidates.push_back(colPoint);
				}
			}
		}
		for (CollisionPoint& candidate : candidates) {
			glm::vec2 candDir = candidate.m_Line->getDirAt(candidate.m_Pos);
			if (abs(glm::dot(candDir, normDir)) > cos(Hatching::ParallelAngle))
				return true;
		}
		return false;
	}

	void HatchingLayer::ResetUnusedSeeds() {
		for (int i = 0; i < m_UnusedSeedsGrid.size(); i++) {
			m_UnusedSeedsGrid[i].clear();
		}
		m_NumUnusedSeeds = 0;
		for (ScreenSpaceSeed& seed : m_Hatching.m_ScreenSeeds) {
			if (seed.m_Visible && !HasCollision(seed.m_Pos, true)) {
				glm::ivec2 gridPos = m_Hatching.ScreenPosToGridPos(seed.m_Pos);
				GetUnusedScreenSeeds(gridPos)->insert(&seed);
				m_NumUnusedSeeds++;
			}
		}
	}

	void HatchingLayer::UpdateUnusedSeeds() {
		// The UnusedSeedGrid has been filled with all visible seeds in ResetUnusedSeeds()
		// We only need to remove the ones that are covered by a line
		for (HatchingLine& line : m_HatchingLines) {
			const std::deque<ScreenSpaceSeed*>& seeds = line.getSeeds();
			for (ScreenSpaceSeed* seed : seeds) {
				glm::ivec2 gridPos = m_Hatching.ScreenPosToGridPos(seed->m_Pos);
				std::unordered_set<ScreenSpaceSeed*>* gridCell = GetUnusedScreenSeeds(gridPos);
				m_NumUnusedSeeds -= gridCell->erase(seed);
			}
		}
	}

	void HatchingLayer::FillGLBuffers() {
		// Fill Buffers for Hatching Lines
		std::vector<glm::vec2> vertices;
		std::vector<unsigned int> indices;
		m_NumLinesIndices = 0;
		unsigned int offset = 0;

		for (auto it = m_HatchingLines.begin(); it != m_HatchingLines.end(); it++) {
			HatchingLine& line = *it;
			const std::deque<glm::vec2>& linePoints = line.getPoints();

			// vertex data
			vertices.push_back(m_Hatching.ScreenToView(linePoints[0]));
			vertices.push_back(m_Hatching.ScreenToView(linePoints[1]) - m_Hatching.ScreenToView(linePoints[0]));
			for (int i = 1; i < linePoints.size(); i++) {
				vertices.push_back(m_Hatching.ScreenToView(linePoints[i]));
				vertices.push_back(m_Hatching.ScreenToView(linePoints[i]) - m_Hatching.ScreenToView(linePoints[i - 1]));
			}
			//index data
			for (int i = 0; i < ((int)linePoints.size() - 1); i++) {
				indices.push_back(offset + i);
				indices.push_back(offset + i + 1);
			}
			offset += linePoints.size();
		}
		m_NumLinesIndices = indices.size();

		glBindVertexArray(m_LinesVAO);
		glBindBuffer(GL_ARRAY_BUFFER, m_LinesVertexBuffer);
		glBufferData(GL_ARRAY_BUFFER, vertices.size() * 2 * sizeof(float), vertices.data(), GL_STREAM_DRAW);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_LinesIndexBuffer);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STREAM_DRAW);

		// Fill Buffers for Collision Points
		m_NumCollisionPoints = 0;
		std::vector<glm::vec2> colPoints;
		for (std::unordered_set<CollisionPoint> gridCell : m_CollisionPointsGrid) {
			for (const CollisionPoint& point : gridCell) {
				colPoints.push_back(m_Hatching.ScreenToView(point.m_Pos));
				m_NumCollisionPoints++;
			}
		}

		glBindVertexArray(m_CollisionVAO);
		glBindBuffer(GL_ARRAY_BUFFER, m_CollisionVBO);
		glBufferData(GL_ARRAY_BUFFER, colPoints.size() * 2 * sizeof(float), colPoints.data(), GL_STREAM_DRAW);
	}

	std::unordered_set<CollisionPoint>* HatchingLayer::GetCollisionPoints(glm::ivec2 gridPos) {
		return &m_CollisionPointsGrid[gridPos.y * m_GridSize.x + gridPos.x];
	}

	std::unordered_set<ScreenSpaceSeed*>* HatchingLayer::GetUnusedScreenSeeds(glm::ivec2 gridPos) {
		return &m_UnusedSeedsGrid[gridPos.y * m_GridSize.x + gridPos.x];
	}


}