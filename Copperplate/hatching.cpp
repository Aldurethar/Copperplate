#pragma once
#include "hatching.h"
#include "rendering.h"
#include "utility.h"

#include <random>
#include <queue>

namespace Copperplate {

	float getFaceArea(Face& face) {
		glm::vec3 a = face.outer->origin->position;
		glm::vec3 b = face.outer->next->origin->position;
		glm::vec3 c = face.outer->next->next->origin->position;
		
		glm::vec3 ab = b - a;
		glm::vec3 ac = c - a;
		return glm::length(glm::cross(ab, ac)) * 0.5f;
	}

	Hatching::Hatching(int viewportWidth, int viewportHeight) {
		m_ViewportSize = glm::vec2((float)viewportWidth, (float)viewportHeight);
		int gridSizeX = (int)(m_ViewportSize.x / DisplaySettings::LineSeparationDistance) + 1;
		int gridSizeY = (int)(m_ViewportSize.y / DisplaySettings::LineSeparationDistance) + 1;
		m_GridSize = glm::ivec2(gridSizeX, gridSizeY);

		m_UnusedScreenSeeds = std::vector<std::unordered_set<ScreenSpaceSeed*>>();
		m_UnusedScreenSeeds.reserve(gridSizeX * gridSizeY);

		m_CollisionPoints = std::vector<std::vector<CollisionPoint>>();
		m_CollisionPoints.reserve(gridSizeX * gridSizeY);

		for (int i = 0; i < gridSizeX; i++) {
			for (int j = 0; j < gridSizeY; j++) {
				m_UnusedScreenSeeds.push_back(std::unordered_set<ScreenSpaceSeed*>());
				m_CollisionPoints.push_back(std::vector<CollisionPoint>());
			}
		}

		m_NormalData = CreateUnique<Image>(m_ViewportSize.x, m_ViewportSize.y);
		m_CurvatureData = CreateUnique<Image>(m_ViewportSize.x, m_ViewportSize.y);
		m_MovementData = CreateUnique<Image>(m_ViewportSize.x, m_ViewportSize.y);
		
		// setup opengl buffers
		// Screen Space Seed Points
		glGenVertexArrays(1, &m_ScreenSeedsVAO);
		glGenBuffers(1, &m_ScreenSeedsVBO);

		glBindVertexArray(m_ScreenSeedsVAO);
		glBindBuffer(GL_ARRAY_BUFFER, m_ScreenSeedsVBO);
		glBufferData(GL_ARRAY_BUFFER, m_ScreenSeeds.size() * 2 * sizeof(float), NULL, GL_DYNAMIC_DRAW);

		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (GLvoid*)0);

		glCheckError();

		// Hatching lines
		glGenVertexArrays(1, &m_LinesVAO);
		glGenBuffers(1, &m_LinesVertexBuffer);
		glGenBuffers(1, &m_LinesIndexBuffer);

		glBindVertexArray(m_LinesVAO);
		glBindBuffer(GL_ARRAY_BUFFER, m_LinesVertexBuffer);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_LinesIndexBuffer);

		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (GLvoid*)0);

		glBindVertexArray(0);
		glCheckError();
	}

	void Hatching::CreateSeedPoints(std::vector<SeedPoint>& outSeedPoints, Mesh& mesh, unsigned int objectId, int totalPoints) {
		//Setup Random
		std::random_device rd;
		std::mt19937 engine(rd());
		std::uniform_real_distribution<> dist(0.0, 1.0);

		HaltonSequence haltonImportance(3);
		HaltonSequence haltonFaceSelect(7);

		std::vector<Face>& faces = mesh.GetFaces();
		float totalArea = mesh.GetTotalArea();
		// construct vector of summed face areas for face selection
		std::vector<float> summedAreas;
		summedAreas.reserve(faces.size());
		float sum = 0.0f;
		for (int i = 0; i < faces.size(); i++) {
			sum += getFaceArea(faces[i]);
			summedAreas.push_back(sum);
		}

		unsigned int seedId = 0;
		for (int i = 0; i < totalPoints; i++) {
			// pseudo-randomly select a face weighted by its area
			float faceSelect = haltonFaceSelect.NextNumber() * totalArea;
			int index = 0;
			for (int i = 0; i < summedAreas.size(); i++) {
				if (summedAreas[i] > faceSelect) {
					index = i;
					break;
				}
			}

			// construct a new Seed Point at a random position within the face
			Face face = faces[index];
			seedId++;
			glm::vec3 v1 = face.outer->origin->position;
			glm::vec3 v2 = face.outer->next->origin->position;
			glm::vec3 v3 = face.outer->next->next->origin->position;
			float a = dist(engine);
			float b = dist(engine);
			float c = dist(engine);
			float sum = a + b + c;
			a /= sum;
			b /= sum;
			c /= sum;
			glm::vec3 pos = (a * v1) + (b * v2) + (c * v3);
			float importance = haltonImportance.NextNumber();
			SeedPoint sp = { pos, face, importance, seedId };
			outSeedPoints.push_back(sp);

			unsigned int id = (objectId << 16) + seedId;
			ScreenSpaceSeed ssp = { glm::vec2(pos), importance, id, false };
			m_ScreenSeeds.push_back(ssp);
		}

		UpdateScreenSeedIdMap();
	}
	
	void Hatching::ResetCollisions() {
		for (std::vector<CollisionPoint>& gridCell : m_CollisionPoints) {
			gridCell.clear();
		}
	}

	void Hatching::UpdateScreenSeed(unsigned int seedId, glm::vec2 pos, bool visible) {
		ScreenSpaceSeed* seed = GetScreenSeedById(seedId);
		seed->m_Pos = pos;
		seed->m_Visible = visible;
	}

	void Hatching::AddContourCollision(const std::vector<glm::vec2>& contourSegments) {
		for (int i = 0; i < contourSegments.size(); i += 2) {
			glm::vec2 pixPos1 = contourSegments[i] * m_ViewportSize;
			glm::vec2 pixPos2 = contourSegments[i + 1] * m_ViewportSize;
			float length = glm::distance(pixPos1, pixPos2);
			int steps = ceil(length / DisplaySettings::LineTestDistance);
			for (int j = 0; j <= steps; j++) {
				float p = j / (float)steps;
				glm::vec2 pos = (1.0f - p) * contourSegments[i] + p * contourSegments[i+1];
				AddCollisionPoint(pos, true, nullptr);
			}
		}
	}
	
	void Hatching::CreateHatchingLines() {
		PrepareForHatching();

		HatchingLine currentLine;
		std::queue<HatchingLine> linesQueue;

		if (m_HatchingLines.size() == 0) {
			//start with highest importance seed point
			ScreenSpaceSeed* start = &(m_ScreenSeeds[0]);
			for (ScreenSpaceSeed& current : m_ScreenSeeds) {
				if (current.m_Importance > start->m_Importance) {
					start = &current;
				}
			}
			//construct the first line
			HatchingLine first = CreateLine(start);
			AddLineCollision(&first);
			m_HatchingLines.push_back(first);
			linesQueue.push(first);
		}
		else {
			UpdateHatchingLines();
			for (auto it = m_HatchingLines.begin(); it != m_HatchingLines.end(); it++) {
				linesQueue.push(*it);
			}
		}

		if (!DisplaySettings::OnlyUpdateHatchLines){
			//Algorithm from Jobard and Lefer, 1997
			while (linesQueue.size() >= 1) {
				currentLine = linesQueue.front();
				ScreenSpaceSeed* candidate;
				if (FindSeedCandidate(candidate, currentLine)) {
					HatchingLine newLine = CreateLine(candidate);
					AddLineCollision(&newLine);
					linesQueue.push(newLine);
					m_HatchingLines.push_back(newLine);
					//m_UnusedPoints.erase(candidate);
					//CleanUnusedPoints();
				}
				else {
					linesQueue.pop();
				}
			}
		}
		FillGLBuffers();
	}
	
	void Hatching::DrawScreenSeeds() {
		glBindVertexArray(m_ScreenSeedsVAO);
		glDrawArrays(GL_POINTS, 0, m_NumVisibleScreenSeeds);
	}

	void Hatching::DrawHatchingLines() {
		glBindVertexArray(m_LinesVAO);
		glDrawElements(GL_LINES, m_NumLinesIndices, GL_UNSIGNED_INT, 0);
	}

	void Hatching::GrabNormalData() {
		m_NormalData->CopyFrameBuffer();
	}

	void Hatching::GrabCurvatureData() {
		m_CurvatureData->CopyFrameBuffer();
	}

	void Hatching::GrabMovementData() {
		m_MovementData->CopyFrameBuffer();
	}

	// PRIVATE FUNCTIONS //

	void Hatching::UpdateHatchingLines() {

		if (DisplaySettings::RenderCurrentDebug) {
			// Move Hatching Lines based on the Motion Field
			for (HatchingLine& line : m_HatchingLines) {
				for (int i = 0; i < line.m_Points.size(); i++) {
					glm::vec2 point = line.m_Points[i];
					glm::vec2 movement = glm::vec2(m_MovementData->Sample(point));
					line.m_Points[i] = point + movement;
				}
			}
			//TODO: Deform Lines slightly to ensure they follow the curvature

			//Perform the Operations from Active Strokes
			SnakesDelete();
			//TODO: the rest of the operators
		}
		else {
			
			std::list<HatchingLine> newLines;
			for (HatchingLine& line : m_HatchingLines) {
				std::vector<ScreenSpaceSeed*> coveredSeeds;
				if (line.m_AssociatedSeeds.size() < 1) {
					continue;
				}
				for (int i = line.m_StartSeed; i < line.m_AssociatedSeeds.size(); i++) {
					ScreenSpaceSeed* seed = line.m_AssociatedSeeds[i];
					bool covered = (std::find(coveredSeeds.begin(), coveredSeeds.end(), seed) != coveredSeeds.end());
					if (seed->m_Visible && !HasCollision(seed->m_Pos, false) && !covered) {
						HatchingLine newLine = CreateLine(seed);
						AddLineCollision(&newLine);
						newLines.push_back(newLine);
						for (ScreenSpaceSeed* addedSeed : newLine.m_AssociatedSeeds) {
							coveredSeeds.push_back(addedSeed);
						}
					}
				}
				for (int i = 0; i < line.m_StartSeed; i++) {
					ScreenSpaceSeed* seed = line.m_AssociatedSeeds[i];
					bool covered = (std::find(coveredSeeds.begin(), coveredSeeds.end(), seed) != coveredSeeds.end());
					if (seed->m_Visible && !HasCollision(seed->m_Pos, false) && !covered) {
						HatchingLine newLine = CreateLine(seed);
						AddLineCollision(&newLine);
						newLines.push_back(newLine);
						for (ScreenSpaceSeed* addedSeed : newLine.m_AssociatedSeeds) {
							coveredSeeds.push_back(addedSeed);
						}
					}
				}
			}
			m_HatchingLines.clear();
			m_HatchingLines = newLines;
		}
	}

	void Hatching::SnakesDelete() {
		for(auto it = m_HatchingLines.begin(); it != m_HatchingLines.end(); ) {
			HatchingLine& line = *it;
			int visibleSeeds = 0;
			for (ScreenSpaceSeed* seed : line.m_AssociatedSeeds) {
				if (seed->m_Visible) visibleSeeds++;
			}

			// Lines with no visible Seeds get removed
			if (visibleSeeds == 0) {
				it = m_HatchingLines.erase(it);
				continue;
			}
			else {
				it++;
			}

			// Lines where all Seeds are still visible need not change
			if (visibleSeeds == line.m_AssociatedSeeds.size()) continue;

			std::deque<ScreenSpaceSeed*> seeds;
			std::move(begin(line.m_AssociatedSeeds), end(line.m_AssociatedSeeds), back_inserter(seeds));
			std::deque<glm::vec2> points;
			std::move(begin(line.m_Points), end(line.m_Points), back_inserter(points));

			// Remove leading seeds if they aren't visible, including the points closest to them
			while (!seeds.front()->m_Visible) {
				glm::vec2 seedPos = seeds.front()->m_Pos;
				seeds.pop_front();
				glm::vec2 nextSeedPos = seeds.front()->m_Pos;
				while (glm::distance(points.front(), seedPos) < glm::distance(points.front(), nextSeedPos)) {
					points.pop_front();
				}
			}

			// Remove trailing seeds if they aren't visible, including the points closest to them
			while (!seeds.back()->m_Visible) {
				glm::vec2 seedPos = seeds.back()->m_Pos;
				seeds.pop_back();
				glm::vec2 nextSeedPos = seeds.back()->m_Pos;
				while (glm::distance(points.back(), seedPos) < glm::distance(points.back(), nextSeedPos)) {
					points.pop_back();
				}
			}

			// If there are still invisible seeds, they must be somewhere in the middle and the line must be split
			if (visibleSeeds < seeds.size()) {
				int splitIndex = 0;
				int seedIndex = 0;
				for (; splitIndex < points.size(); splitIndex++) {
					if (glm::distance(points[splitIndex], seeds[seedIndex]->m_Pos) > glm::distance(points[splitIndex], seeds[seedIndex + 1]->m_Pos)) {
						seedIndex++;
					}
					if (!seeds[seedIndex]->m_Visible) break;
				}

				std::vector<glm::vec2> restPoints;
				std::vector<ScreenSpaceSeed*> restSeeds;
				for (int j = splitIndex + 1; j < points.size(); j++) restPoints.push_back(points[j]);
				for (int j = seedIndex + 1; j < seeds.size(); j++) restSeeds.push_back(seeds[j]);

				for (int j = 0; j <= restPoints.size(); j++) points.pop_back();
				for (int j = 0; j <= restSeeds.size(); j++) seeds.pop_back();

				// the split off part may still have invisible seeds and will be processed by the loop later on
				m_HatchingLines.push_back(HatchingLine{ (int)restPoints.size(), 0, restPoints, restSeeds }); 
			}

			//TODO: I may have to cut lines short/possibly even split them when a contour collision cuts through them

			// write the data back into the line
			line.m_NumPoints = points.size();
			line.m_AssociatedSeeds.clear();
			std::move(begin(seeds), end(seeds), back_inserter(line.m_AssociatedSeeds));
			line.m_Points.clear();
			std::move(begin(points), end(points), back_inserter(line.m_Points));
		}
	}

	void Hatching::PrepareForHatching() {
		// Reset Hatching Lines
		//m_HatchingLines.clear();
		m_NumLinesIndices = 0;
		// Reset Screen Seed Grid
		for (std::unordered_set<ScreenSpaceSeed*>& gridCell : m_UnusedScreenSeeds) {
			gridCell.clear();
		}
		m_NumVisibleScreenSeeds = 0;
		// Fill Screen Seed Grid, this already omits seeds that collide with the contours
		for (ScreenSpaceSeed& seed : m_ScreenSeeds) {
			if (seed.m_Visible && !HasCollision(seed.m_Pos, false)) {
				glm::ivec2 gridPos = ScreenPosToGridPos(seed.m_Pos);
				std::unordered_set<ScreenSpaceSeed*>& gridCell = *GetUnusedScreenSeeds(gridPos);
				gridCell.insert(&seed);
			}
		}
	}
	
	bool Hatching::FindSeedCandidate(ScreenSpaceSeed*& out, HatchingLine currentLine) {
		// TODO: Rework this to make sure even seed points further away from a line get selected eventually
		float closestDistance = 1000.0f;
		bool foundCandidate = false;
		ScreenSpaceSeed* candidate;
		for (int i = 0; i < currentLine.m_NumPoints; i++) {
			glm::ivec2 gridPos = ScreenPosToGridPos(currentLine.m_Points[i]);
			glm::vec2 currPixCoords = currentLine.m_Points[i] * m_ViewportSize;
			for (int x = -1; x <= 1; x++) {
				for (int y = -1; y <= 1; y++) {
					glm::ivec2 currGridPos = glm::clamp(gridPos + glm::ivec2(x, y), glm::ivec2(0), (m_GridSize - glm::ivec2(1)));
					std::unordered_set<ScreenSpaceSeed*>& gridCell = *GetUnusedScreenSeeds(currGridPos);
					for (ScreenSpaceSeed* currSeed : gridCell) {
						glm::vec2 compCoords = currSeed->m_Pos * m_ViewportSize;
						float dist = glm::distance(currPixCoords, compCoords);
						if (dist < closestDistance && dist >= DisplaySettings::LineSeparationDistance) {
							candidate = currSeed;
							foundCandidate = true;
							closestDistance = dist;
						}
					}
				}
			}			
		}
		if (foundCandidate) {
			out = candidate;
		}
		return foundCandidate;
	}

	// Create a single Hatching Line
	HatchingLine Hatching::CreateLine(ScreenSpaceSeed* seed) {
		//DEBUGGING
		int maxPoints = DisplaySettings::NumPointsPerHatch;
		if (maxPoints <= 0) maxPoints = 999999;
		int pointsLeft = 0;

		float maxAngle = 10.0f;
		float terminationAngle = 30.0f;
		float stepsize = DisplaySettings::LineTestDistance * 1.4f;
		float seedRadius = DisplaySettings::LineTestDistance * 0.8f;
		
		glm::vec2 dir;
		glm::vec2 sampleDir;

		std::vector<glm::vec2> linePoints;
		std::vector<ScreenSpaceSeed*> associatedSeeds;
		int numPoints = 1;

		std::vector<glm::vec2> leftPoints;
		std::vector<ScreenSpaceSeed*> leftSeeds;

		glm::vec2 currPos = seed->m_Pos;
		glm::vec2 currPixPos = currPos * m_ViewportSize;
		leftPoints.push_back(currPos);
		dir = GetHatchingDir(currPos);
		
		//extrude left
		while (!HasCollision(currPos, false) && (pointsLeft < maxPoints)) {
			// Add Collision for previous point
			ScreenSpaceSeed* newSeed = FindNearbySeed(currPos, seedRadius);
			if (newSeed) leftSeeds.push_back(newSeed);

			// Compute Direction for next step
			sampleDir = GetHatchingDir(currPos);
			float diffAngle = glm::dot(dir, sampleDir);
			if (diffAngle < 0) sampleDir = -sampleDir; // bring sampled direction into the right hemicircle
			if (abs(diffAngle) < cos(degToRad(terminationAngle))) break; // stop line if the bend would be more than terminatioAngle
			dir = clampToMaxAngleDifference(sampleDir, dir, maxAngle); // limit strength of bend to maxangle degrees per step
			//dir = sampleDir;
			
			// Step in the computed direction, add point to line
			currPixPos = currPixPos + (stepsize * dir);
			currPos = currPixPos / m_ViewportSize;
			leftPoints.push_back(currPos);
			numPoints++;
			pointsLeft++;
		}
		int startSeed = leftSeeds.size() - 1;
		while (!leftPoints.empty()) {
			linePoints.push_back(leftPoints.back());
			leftPoints.pop_back();
		}
		while (!leftSeeds.empty()) {
			associatedSeeds.push_back(leftSeeds.back());
			leftSeeds.pop_back();
		}

		dir = -GetHatchingDir(seed->m_Pos);		
		currPixPos = seed->m_Pos * m_ViewportSize + (stepsize * dir);
		currPos = currPixPos / m_ViewportSize;
		linePoints.push_back(currPos);
		numPoints++;
		//extrude right
		while (!HasCollision(currPos, false) && (numPoints < (maxPoints*2 + 1))) {
			// Add Collision for previous point
			ScreenSpaceSeed* newSeed = FindNearbySeed(currPos, seedRadius);
			if (newSeed) associatedSeeds.push_back(newSeed);

			// Compute Direction for next step
			sampleDir = GetHatchingDir(currPos);			
			float diffAngle = glm::dot(dir, sampleDir);
			if (diffAngle < 0) sampleDir = -sampleDir; // bring sampled direction into the right hemicircle
			if (abs(diffAngle) < cos(degToRad(terminationAngle))) break; // stop line if the bend would be more than terminatioAngle
			dir = clampToMaxAngleDifference(sampleDir, dir, maxAngle); // limit strength of bend to maxangle degrees per step
			//dir = sampleDir;

			// Step in the computed direction, add point to line
			currPixPos = currPixPos + (stepsize * dir);
			currPos = currPixPos / m_ViewportSize;
			linePoints.push_back(currPos);
			numPoints++;
		}
		return HatchingLine{ numPoints, startSeed, linePoints, associatedSeeds };
	}

	void Hatching::AddLineCollision(HatchingLine* line) {
		for (glm::vec2 point : line->m_Points) {
			AddCollisionPoint(point, false, line);
		}
	}

	void Hatching::AddCollisionPoint(glm::vec2 screenPos, bool isContour, HatchingLine* line) {
		if (screenPos.x <= 0.0f || screenPos.x > 1.0f || screenPos.y <= 0.0f || screenPos.y > 1.0f)
			return;

		// Add Collision Point
		glm::ivec2 gridPos = ScreenPosToGridPos(screenPos);
		std::vector<CollisionPoint>& colGridCell = *GetCollisionPoints(gridPos);
		CollisionPoint newPoint = { screenPos, isContour, line };
		colGridCell.push_back(newPoint);

		// Remove colliding Seed Points
		glm::vec2 pixPos = screenPos * m_ViewportSize;
		for (int x = -1; x <= 1; x++) {
			for (int y = -1; y <= 1; y++) {
				glm::ivec2 currGridPos = gridPos + glm::ivec2(x, y);
				if (currGridPos.x >= 0 && currGridPos.x < m_GridSize.x
					&& currGridPos.y >= 0 && currGridPos.y < m_GridSize.y) {
					std::unordered_set<ScreenSpaceSeed*>& seedGridCell = *GetUnusedScreenSeeds(currGridPos);
					for (auto it = seedGridCell.begin(); it != seedGridCell.end(); ) {
						glm::vec2 seedPixPos = ((*it)->m_Pos) * m_ViewportSize;
						if (glm::distance(seedPixPos, pixPos) < DisplaySettings::LineTestDistance) it = seedGridCell.erase(it);
						else ++it;
					}
				}
			}
		}
	}

	void Hatching::FillGLBuffers() {
		// Fill Buffers for Screen Space Seeds
		std::vector<glm::vec2> screenSeedPos;
		for (ScreenSpaceSeed& seed : m_ScreenSeeds) {
			if (seed.m_Visible) {
				screenSeedPos.push_back(seed.m_Pos);
				m_NumVisibleScreenSeeds++;
			}
		}

		glBindVertexArray(m_ScreenSeedsVAO);
		glBindBuffer(GL_ARRAY_BUFFER, m_ScreenSeedsVBO);
		glBufferData(GL_ARRAY_BUFFER, screenSeedPos.size() * 2 * sizeof(float), screenSeedPos.data(), GL_DYNAMIC_DRAW);

		// Fill Buffers for Hatching Lines
		// Debug Display
		int maxLines = DisplaySettings::NumHatchingLines;
		if (maxLines < 0 || maxLines > m_HatchingLines.size()) maxLines = m_HatchingLines.size();

		std::vector<glm::vec2> vertices;
		std::vector<unsigned int> indices;
		unsigned int offset = 0;

		int lines = 0;
		for(auto it = m_HatchingLines.begin(); it != m_HatchingLines.end(); it++) {
			if (lines <= maxLines) {
				lines++;
				HatchingLine line = *it;

				// vertex data
				for (int i = 0; i < line.m_NumPoints; i++) {
					vertices.push_back(line.m_Points[i]);
				}
				//index data
				for (int i = 0; i < line.m_NumPoints - 1; i++) {
					indices.push_back(offset + i);
					indices.push_back(offset + i + 1);
				}
				offset += line.m_NumPoints;
			}
		}
		m_NumLinesIndices = indices.size();

		glBindVertexArray(m_LinesVAO);
		glBindBuffer(GL_ARRAY_BUFFER, m_LinesVertexBuffer);
		glBufferData(GL_ARRAY_BUFFER, vertices.size() * 2 * sizeof(float), vertices.data(), GL_STREAM_DRAW);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_LinesIndexBuffer);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STREAM_DRAW);
	}
	
	void Hatching::UpdateScreenSeedIdMap() {
		m_ScreenSeedIdMap.clear();
		for (ScreenSpaceSeed& seed : m_ScreenSeeds) {
			m_ScreenSeedIdMap[seed.m_Id] = &seed;
		}
	}

	std::vector<CollisionPoint>* Hatching::GetCollisionPoints(glm::ivec2 gridPos) {
		return &m_CollisionPoints[gridPos.y * m_GridSize.x + gridPos.x];
	}

	std::unordered_set<ScreenSpaceSeed*>* Hatching::GetUnusedScreenSeeds(glm::ivec2 gridPos) {
		return &m_UnusedScreenSeeds[gridPos.y * m_GridSize.x + gridPos.x];
	}

	bool Hatching::HasCollision(glm::vec2 screenPos, bool onlyContours) {
		if (screenPos.x < 0.0f || screenPos.x > 1.0f) return true;
		if (screenPos.y < 0.0f || screenPos.y > 1.0f) return true;
		glm::vec2 pixelPos = screenPos * m_ViewportSize;
		glm::ivec2 gridCenter = ScreenPosToGridPos(screenPos);
		for (int x = -1; x <= 1; x++) {
			for (int y = -1; y <= 1; y++) {
				glm::ivec2 gridPos = gridCenter + glm::ivec2(x, y);
				if (gridPos.x >= 0 && gridPos.x < m_GridSize.x && gridPos.y >= 0 && gridPos.y < m_GridSize.y) {
					std::vector<CollisionPoint>& gridCell = *GetCollisionPoints(gridPos);
					for (CollisionPoint point : gridCell) {
						glm::vec2 pixelPoint = point.m_Pos * m_ViewportSize;
						float dist = glm::distance(pixelPos, pixelPoint);
						if (dist < DisplaySettings::LineTestDistance) {
							if(!onlyContours || point.m_isContour) return true;
						}
					}
				}
			}
		}
		return false;
	}

	glm::vec2 Hatching::GetHatchingDir(glm::vec2 screenPos) {
		float eps = 1e-8;
		glm::vec2 dir;
		if (DisplaySettings::HatchingDirection == EHatchingDirections::HD_LargestCurvature ||
			DisplaySettings::HatchingDirection == EHatchingDirections::HD_SmallestCurvature) {
			dir = glm::vec2(m_CurvatureData->Sample(screenPos));
		}
		else {
			dir = glm::vec2(m_NormalData->Sample(screenPos));
		}

		if (glm::length(dir) > eps) dir = glm::normalize(dir);
		else dir = glm::vec2(0.0f);
		
		if (DisplaySettings::HatchingDirection == EHatchingDirections::HD_SmallestCurvature ||
			DisplaySettings::HatchingDirection == EHatchingDirections::HD_Tangent) {
			dir = glm::vec2(-dir.y, dir.x);
		}

		return dir;
	}

	ScreenSpaceSeed* Hatching::FindNearbySeed(glm::vec2 screenPos, float maxDistance) {
		if (screenPos.x <= 0.0f || screenPos.x > 1.0f || screenPos.y <= 0.0f || screenPos.y > 1.0f)
			return nullptr;

		glm::ivec2 gridPos = ScreenPosToGridPos(screenPos);
		glm::vec2 pixPos = screenPos * m_ViewportSize;
		float closestDist = 1000.0f;
		ScreenSpaceSeed* result = nullptr;

		for (int x = -1; x <= 1; x++) {
			for (int y = -1; y <= 1; y++) {
				glm::ivec2 currGridPos = gridPos + glm::ivec2(x, y);
				if (currGridPos.x >= 0 && currGridPos.x < m_GridSize.x
					&& currGridPos.y >= 0 && currGridPos.y < m_GridSize.y) {
					std::unordered_set<ScreenSpaceSeed*>& seedGridCell = *GetUnusedScreenSeeds(currGridPos);
					for (ScreenSpaceSeed* seed : seedGridCell) {
						glm::vec2 seedPixPos = seed->m_Pos * m_ViewportSize;
						float dist = glm::distance(seedPixPos, pixPos);
						if (dist < maxDistance && dist < closestDist) {
							closestDist = dist;
							result = seed;
						}
					}
				}
			}
		}
		return result;
	}

	ScreenSpaceSeed* Hatching::GetScreenSeedById(unsigned int id) {
		return m_ScreenSeedIdMap[id];
	}

	glm::ivec2 Hatching::ScreenPosToGridPos(glm::vec2 screenPos) {
		int x = ceil(screenPos.x * m_GridSize.x) - 1;
		int y = ceil(screenPos.y * m_GridSize.y) - 1;
		return glm::ivec2(x, y);
	}
	   
}