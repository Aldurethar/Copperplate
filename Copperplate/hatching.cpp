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
		return glm::cross(ab, ac).length() * 0.5f;
	}

	Hatching::Hatching(int viewportWidth, int viewportHeight) {
		m_ViewportSize = glm::vec2((float)viewportWidth, (float)viewportHeight);
		int gridSizeX = (int)(m_ViewportSize.x / DisplaySettings::LineSeparationDistance) + 1;
		int gridSizeY = (int)(m_ViewportSize.y / DisplaySettings::LineSeparationDistance) + 1;
		m_GridSize = glm::ivec2(gridSizeX, gridSizeY);

		m_UnusedScreenSeeds = std::vector<std::unordered_set<ScreenSpaceSeed*>>();
		m_UnusedScreenSeeds.reserve(gridSizeX * gridSizeY);

		m_CollisionPoints = std::vector<std::vector<glm::vec2>>();
		m_CollisionPoints.reserve(gridSizeX * gridSizeY);

		for (int i = 0; i < gridSizeX; i++) {
			for (int j = 0; j < gridSizeY; j++) {
				m_UnusedScreenSeeds.push_back(std::unordered_set<ScreenSpaceSeed*>());
				m_CollisionPoints.push_back(std::vector<glm::vec2>());
			}
		}

		m_NormalData = CreateUnique<Image>(m_ViewportSize.x, m_ViewportSize.y);
		m_CurvatureData = CreateUnique<Image>(m_ViewportSize.x, m_ViewportSize.y);
		
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

	void Hatching::CreateSeedPoints(std::vector<SeedPoint>& outSeedPoints, Mesh& mesh, unsigned int objectId, int totalPoints, int maxPointsPerFace) {
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
			// binary search for the correct index
			bool finished = false;
			int index = faces.size() / 2;
			int step = (int)ceil(faces.size() / 4.0f);
			while (!finished) {
				if (index == 0 || (faceSelect <= summedAreas[index] && faceSelect > summedAreas[index - 1])) {
					finished = true;
				}
				else {
					if (faceSelect <= summedAreas[index])
						index -= step;
					else
						index += step;

					step = (int)ceil(step / 2.0f);
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
		for (std::vector<glm::vec2>& gridCell : m_CollisionPoints) {
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
				AddCollisionPoint(pos);
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
			HatchingLine first = CreateLine(start, 0, 0);
			m_HatchingLines.push_back(first);
			linesQueue.push(first);
		}
		else {
			UpdateHatchingLines();
			for (int i = 0; i < m_HatchingLines.size(); i++) {
				linesQueue.push(m_HatchingLines[i]);
			}
		}

		if (!DisplaySettings::OnlyUpdateHatchLines){
			//Algorithm from Jobard and Lefer, 1997
			while (linesQueue.size() >= 1) {
				currentLine = linesQueue.front();
				ScreenSpaceSeed* candidate;
				if (FindSeedCandidate(candidate, currentLine)) {
					HatchingLine newLine = CreateLine(candidate, 0, 0);
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

	// PRIVATE FUNCTIONS //

	void Hatching::UpdateHatchingLines() {
		std::vector<HatchingLine> newLines;
		for (HatchingLine& line : m_HatchingLines) {
			if (line.m_AssociatedSeeds.size() < 1) {
				continue;
			}
			ScreenSpaceSeed* startSeed = line.m_AssociatedSeeds[0];
			if (startSeed->m_Visible && !HasCollision(startSeed->m_Pos)) {
				HatchingLine newLine = CreateLine(startSeed, line.m_NumPoints, line.m_LeftPoints);
				newLines.push_back(newLine);
				continue;
			}
			if (!startSeed->m_Visible && line.m_AssociatedSeeds.size() > 1) {
				for (int i = 1; i < line.m_AssociatedSeeds.size(); i++) {
					ScreenSpaceSeed* currSeed = line.m_AssociatedSeeds[i];
					if (currSeed->m_Visible && !HasCollision(currSeed->m_Pos)) {
						HatchingLine newLine = CreateLine(currSeed, line.m_NumPoints, line.m_LeftPoints);
						newLines.push_back(newLine);
						break;
					}
				}
				continue;
			}
		}
		m_HatchingLines = newLines;
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
			if (seed.m_Visible && !HasCollision(seed.m_Pos)) {
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

	// Create a single Hatching Line, this already inserts the collision points for the line and removes colliding seed points
	HatchingLine Hatching::CreateLine(ScreenSpaceSeed* seed, int prevNumPoints, int prevLeftPoints) {
		//DEBUGGING
		int maxPoints = DisplaySettings::NumPointsPerHatch;
		if (maxPoints <= 0) maxPoints = 999999;
		if (prevNumPoints <= 0) prevNumPoints = 999999;
		if (prevLeftPoints <= 0) prevLeftPoints = 999999;

		float maxAngle = 10.0f;
		float terminationAngle = 30.0f;
		float stepsize = DisplaySettings::LineTestDistance * 1.4f;
		float seedRadius = DisplaySettings::LineTestDistance * 0.8f;
		
		glm::vec2 dir;
		glm::vec2 sampleDir;

		std::vector<glm::vec2> linePoints;
		std::vector<ScreenSpaceSeed*> associatedSeeds;
		int numPoints = 1;
		int pointsLeft = 0;

		glm::vec2 currPos = seed->m_Pos;
		glm::vec2 currPixPos = currPos * m_ViewportSize;
		linePoints.push_back(currPos);
		dir = GetHatchingDir(currPos);
		
		//extrude left
		while (!HasCollision(currPos) && (pointsLeft < maxPoints) /*&& (pointsLeft <= (prevLeftPoints + 1))*/) {
			// Add Collision for previous point
			ScreenSpaceSeed* newSeed = FindNearbySeed(currPos, seedRadius);
			if (newSeed) associatedSeeds.push_back(newSeed);
			AddCollisionPoint(currPos);

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
			pointsLeft++;
		}

		dir = -GetHatchingDir(seed->m_Pos);		
		currPixPos = seed->m_Pos * m_ViewportSize + (stepsize * dir);
		currPos = currPixPos / m_ViewportSize;
		linePoints.push_back(currPos);
		numPoints++;
		//extrude right
		while (!HasCollision(currPos) && (numPoints < (maxPoints*2 + 1)) /*&& (numPoints <= (prevNumPoints + 2))*/) {
			// Add Collision for previous point
			ScreenSpaceSeed* newSeed = FindNearbySeed(currPos, seedRadius);
			if (newSeed) associatedSeeds.push_back(newSeed);
			AddCollisionPoint(currPos);

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
		return HatchingLine{ numPoints, pointsLeft, linePoints, associatedSeeds };
	}

	void Hatching::AddCollisionPoint(glm::vec2 screenPos) {
		if (screenPos.x <= 0.0f || screenPos.x > 1.0f || screenPos.y <= 0.0f || screenPos.y > 1.0f)
			return;

		// Add Collision Point
		glm::ivec2 gridPos = ScreenPosToGridPos(screenPos);
		std::vector<glm::vec2>& colGridCell = *GetCollisionPoints(gridPos);
		colGridCell.push_back(screenPos);

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

		for (int l = 0; l < maxLines; l++){
			HatchingLine line = m_HatchingLines[l];
		
			// vertex data
			for (int i = 0; i < line.m_NumPoints; i++) {
				vertices.push_back(line.m_Points[i]);
			}
			// index data for left half of line
			for (int i = 0; i < line.m_LeftPoints; i++) {
				indices.push_back(offset + i);
				indices.push_back(offset + i + 1);
			}
			// index data for right half of line
			indices.push_back(offset + 0);
			indices.push_back(offset + line.m_LeftPoints + 1);
			for (int i = line.m_LeftPoints + 1; i < line.m_NumPoints - 1; i++) {
				indices.push_back(offset + i);
				indices.push_back(offset + i + 1);
			}
			offset += line.m_NumPoints;
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

	std::vector<glm::vec2>* Hatching::GetCollisionPoints(glm::ivec2 gridPos) {
		return &m_CollisionPoints[gridPos.y * m_GridSize.x + gridPos.x];
	}

	std::unordered_set<ScreenSpaceSeed*>* Hatching::GetUnusedScreenSeeds(glm::ivec2 gridPos) {
		return &m_UnusedScreenSeeds[gridPos.y * m_GridSize.x + gridPos.x];
	}

	bool Hatching::HasCollision(glm::vec2 screenPos) {
		if (screenPos.x < 0.0f || screenPos.x > 1.0f) return true;
		if (screenPos.y < 0.0f || screenPos.y > 1.0f) return true;
		glm::vec2 pixelPos = screenPos * m_ViewportSize;
		glm::ivec2 gridCenter = ScreenPosToGridPos(screenPos);
		for (int x = -1; x <= 1; x++) {
			for (int y = -1; y <= 1; y++) {
				glm::ivec2 gridPos = gridCenter + glm::ivec2(x, y);
				if (gridPos.x >= 0 && gridPos.x < m_GridSize.x && gridPos.y >= 0 && gridPos.y < m_GridSize.y) {
					std::vector<glm::vec2>& gridCell = *GetCollisionPoints(gridPos);
					for (glm::vec2 point : gridCell) {
						glm::vec2 pixelPoint = point * m_ViewportSize;
						float dist = glm::distance(pixelPos, pixelPoint);
						if (dist < DisplaySettings::LineTestDistance) return true;
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