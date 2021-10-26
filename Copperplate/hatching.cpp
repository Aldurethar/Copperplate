#pragma once

#include "hatching.h"
#include "rendering.h"

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

	void CreateSeedPoints(std::vector<SeedPoint>& outSeedPoints, Mesh& mesh, int totalPoints, int maxPointsPerFace) {
		//Setup Random
		std::random_device rd;
		std::mt19937 engine(rd());
		std::uniform_real_distribution<> dist(0.0, 1.0);

		HaltonSequence halton(3);

		float totalArea = mesh.GetTotalArea();
		float pointsFrac = (float)totalPoints / (float)maxPointsPerFace;
		std::vector<Face>& faces = mesh.GetFaces();

		for (Face& face : faces) {
			float faceArea = getFaceArea(face);
			float pointChance = (faceArea / totalArea) * pointsFrac;
			for (int i = 0; i < maxPointsPerFace; i++) {
				float r = dist(engine);
				if (r < pointChance) {	
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
					float importance = halton.NextNumber();
					SeedPoint sp = { pos, face, importance };
					outSeedPoints.push_back(sp);
				}
			}
		}
	}
	
	Hatching::Hatching(int viewportWidth, int viewportHeight) {
		m_ViewportSize = glm::vec2((float)viewportWidth, (float)viewportHeight);
		int gridSizeX = (int)(m_ViewportSize.x / LINE_SEPARATION_DISTANCE) + 1;
		int gridSizeY = (int)(m_ViewportSize.y / LINE_SEPARATION_DISTANCE) + 1;
		m_GridSize = glm::ivec2(gridSizeX, gridSizeY);
		m_CollisionPoints = std::vector<std::vector<glm::vec2>>();
		m_CollisionPoints.reserve(gridSizeX * gridSizeY);
		for (int i = 0; i < gridSizeX; i++) {
			for (int j = 0; j < gridSizeY; j++) {
				m_CollisionPoints.push_back(std::vector<glm::vec2>());
			}
		}
		
		// setup opengl buffers
		glGenVertexArrays(1, &m_LinesVAO);
		glGenBuffers(1, &m_LinesVertexBuffer);
		glGenBuffers(1, &m_LinesIndexBuffer);

		glBindVertexArray(m_LinesVAO);
		glBindBuffer(GL_ARRAY_BUFFER, m_LinesVertexBuffer);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_LinesIndexBuffer);

		// configure vertex attributes
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (GLvoid*)0);

		glBindVertexArray(0);
		glCheckError();
	}

	void Hatching::ResetSeeds() {
		m_Seedpoints.clear();
		m_UnusedPoints.clear();
		for (std::vector<glm::vec2>& gridCell : m_CollisionPoints) {
			gridCell.clear();
		}
	}

	void Hatching::ResetHatching() {
		m_HatchingLines.clear();
		m_LinesIndexNum = 0;
	}

	void Hatching::AddSeeds(const std::vector<ScreenSpaceSeed> &seeds) {
		for (int i = 0; i < seeds.size(); i++) {
			m_Seedpoints.emplace_back(seeds[i]);
		}
	}

	void Hatching::CreateHatchingLines() {
		HatchingLine currentLine;
		std::queue<HatchingLine> linesQueue;
		bool finished = false;

		//setup unused points list
		for (ScreenSpaceSeed& seed : m_Seedpoints) {
			m_UnusedPoints.insert(&seed);
		}

		//start with highest importance seed point
		ScreenSpaceSeed *start = &(m_Seedpoints[0]);
		for (ScreenSpaceSeed& current : m_Seedpoints) {
			if (current.m_Importance > start->m_Importance) {
				start = &current;
			}
		}
		//construct the first line
		HatchingLine first = CreateLine(start);
		linesQueue.push(first);
		m_HatchingLines.push_back(first);
		//m_UnusedPoints.erase(start);
		CleanUnusedPoints();

		//Algorithm from Jobard and Lefer, 1997
		while (!finished) {
			currentLine = linesQueue.front();
			ScreenSpaceSeed* candidate;
			if (FindSeedCandidate(candidate, currentLine)) {
				HatchingLine newLine = CreateLine(candidate);
				linesQueue.push(newLine);
				m_HatchingLines.push_back(newLine);
				//m_UnusedPoints.erase(candidate);
				CleanUnusedPoints();
			}
			else {
				if (linesQueue.size() <= 1) {
					finished = true;
				}
				else {
					linesQueue.pop();
				}
			}
		}

		FillGLBuffers();
	}

	void Hatching::DrawHatchingLines() {
		glBindVertexArray(m_LinesVAO);
		glDrawElements(GL_LINES, m_LinesIndexNum, GL_UNSIGNED_INT, 0);
	}

	HatchingLine Hatching::CreateLine(ScreenSpaceSeed* seed) {
		//DEBUGGING
		int maxPoints = DisplaySettings::NumPointsPerHatch;
		if (maxPoints <= 0) maxPoints = 999999;
		
		glm::vec2 dir = glm::vec2(-1.0f, 0.0f); 
		glm::vec2 dirChange = glm::vec2(0.0f, seed->m_Importance);

		std::vector<glm::vec2> linePoints;
		int numPoints = 1;
		int pointsLeft = 0;
		glm::vec2 currPos = seed->m_Pos;
		glm::vec2 currPixPos = currPos * m_ViewportSize;
		linePoints.push_back(currPos);
		
		//extrude left
		while (!HasCollision(currPos) && pointsLeft < maxPoints) {
			dir = glm::normalize(dir + dirChange);
			dirChange = glm::vec2(dir.y, -dir.x) * seed->m_Importance;
			AddCollisionPoint(currPos);
			currPixPos = currPixPos + (LINE_SEPARATION_DISTANCE * dir);
			currPos = currPixPos / m_ViewportSize;
			linePoints.push_back(currPos);
			numPoints++;
			pointsLeft++;
		}

		currPixPos = seed->m_Pos * m_ViewportSize + (LINE_SEPARATION_DISTANCE * glm::vec2(1.0f, 0.0f));
		currPos = currPixPos / m_ViewportSize;
		linePoints.push_back(currPos);
		numPoints++;
		dir = glm::vec2(1.0f, 0.0f);
		dirChange = glm::vec2(dir.y, -dir.x) * seed->m_Importance;
		//extrude right
		while (!HasCollision(currPos) && numPoints < maxPoints*2 + 1) {
			dir = glm::normalize(dir + dirChange);
			dirChange = glm::vec2(dir.y, -dir.x) * seed->m_Importance;
			AddCollisionPoint(currPos);
			currPixPos = currPixPos + (LINE_SEPARATION_DISTANCE * dir);
			currPos = currPixPos / m_ViewportSize;
			linePoints.push_back(currPos);
			numPoints++;
		}
		return HatchingLine{ numPoints, pointsLeft, linePoints };
	}

	bool Hatching::FindSeedCandidate(ScreenSpaceSeed*& out, HatchingLine currentLine) {
		// TODO: Rework this to not be terribly inefficient
		float closestDistance = 1000.0f;
		bool foundCandidate = false;
		ScreenSpaceSeed* candidate;
		for (int i = 0; i < currentLine.m_NumPoints; i++) {
			glm::vec2 currCoords = currentLine.m_Points[i] * m_ViewportSize;
			for (ScreenSpaceSeed* currSeed : m_UnusedPoints) {
				glm::vec2 compCoords = currSeed->m_Pos * m_ViewportSize;
				float dist = glm::distance(currCoords, compCoords);
				if (dist < closestDistance && dist >= LINE_SEPARATION_DISTANCE) {
					candidate = currSeed;
					foundCandidate = true;
					closestDistance = dist;
				}
			}			
		}
		if (foundCandidate) {
			out = candidate;
		}
		return foundCandidate;
	}

	void Hatching::FillGLBuffers() {
		// Debugging
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
		m_LinesIndexNum = indices.size();

		glBindVertexArray(m_LinesVAO);
		glBindBuffer(GL_ARRAY_BUFFER, m_LinesVertexBuffer);
		glBufferData(GL_ARRAY_BUFFER, vertices.size() * 2 * sizeof(float), vertices.data(), GL_STREAM_DRAW);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_LinesIndexBuffer);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STREAM_DRAW);
	}

	std::vector<glm::vec2>* Hatching::GetCollisionPoints(glm::ivec2 gridPos) {
		return &m_CollisionPoints[gridPos.y * m_GridSize.x + gridPos.x];
	}

	glm::ivec2 Hatching::ScreenPosToGridPos(glm::vec2 screenPos) {
		int x = ceil(screenPos.x * m_GridSize.x) - 1;
		int y = ceil(screenPos.y * m_GridSize.y) - 1;
		return glm::ivec2(x, y);
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
						if (dist < LINE_TEST_DISTANCE) return true;
					}
				}
			}
		}
		return false;
	}

	void Hatching::AddCollisionPoint(glm::vec2 screenPos) {
		std::vector<glm::vec2>& gridCell = *GetCollisionPoints(ScreenPosToGridPos(screenPos));
		gridCell.push_back(screenPos);
	}

	void Hatching::CleanUnusedPoints() {
		for (auto it = m_UnusedPoints.begin(); it != m_UnusedPoints.end(); ) {
			glm::vec2 pos = (*it)->m_Pos;
			if (HasCollision(pos)) it = m_UnusedPoints.erase(it);
			else ++it;
		}
	}


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