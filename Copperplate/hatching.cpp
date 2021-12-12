#pragma once
#include "hatching.h"
#include "rendering.h"
#include "utility.h"

#include <random>
#include <queue>

namespace Copperplate {

	float Hatching::CoverRadius = DisplaySettings::LineTestDistance * 0.8f;
	float Hatching::TrimRadius = DisplaySettings::LineTestDistance;
	float Hatching::ExtendRadius = DisplaySettings::LineTestDistance * 1.2f;
	float Hatching::MergeRadius = DisplaySettings::LineTestDistance * 1.4f;
	float Hatching::SplitAngle = degToRad(20);
	float Hatching::CurvatureBreakAngle = degToRad(20);
	float Hatching::CurvatureClampAngle = degToRad(8);
	float Hatching::ParallelAngle = degToRad(10);
	float Hatching::OptiStepSize = 1.0f;
	int Hatching::NumOptiSteps = 4;
	float Hatching::OptiSeedWeight = 2.0f;
	float Hatching::OptiSmoothWeight = 3.0f;
	float Hatching::OptiFieldWeight = 1.0f;
	float Hatching::OptiSpringWeight = 1.0f;

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

		m_VisibleSeedsGrid = std::vector<std::unordered_set<ScreenSpaceSeed*>>();
		m_VisibleSeedsGrid.reserve(gridSizeX * gridSizeY);

		m_UnusedSeedsGrid = std::vector<std::unordered_set<ScreenSpaceSeed*>>();
		m_UnusedSeedsGrid.reserve(gridSizeX * gridSizeY);

		m_CollisionPointsGrid = std::vector<std::unordered_set<CollisionPoint>>();
		m_CollisionPointsGrid.reserve(gridSizeX * gridSizeY);

		for (int i = 0; i < gridSizeX; i++) {
			for (int j = 0; j < gridSizeY; j++) {
				m_VisibleSeedsGrid.push_back(std::unordered_set<ScreenSpaceSeed*>());
				m_UnusedSeedsGrid.push_back(std::unordered_set<ScreenSpaceSeed*>());
				m_CollisionPointsGrid.push_back(std::unordered_set<CollisionPoint>());
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

		// Collision Points
		glGenVertexArrays(1, &m_CollisionVAO);
		glGenBuffers(1, &m_CollisionVBO);

		glBindVertexArray(m_CollisionVAO);
		glBindBuffer(GL_ARRAY_BUFFER, m_CollisionVBO);

		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (GLvoid*)0);

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
		for (std::unordered_set<CollisionPoint>& gridCell : m_CollisionPointsGrid) {
			gridCell.clear();
		}
	}

	void Hatching::UpdateScreenSeed(unsigned int seedId, glm::vec2 pos, bool visible) {
		ScreenSpaceSeed* seed = GetScreenSeedById(seedId);
		seed->m_Pos = ViewToScreen(pos);
		seed->m_Visible = visible;
	}

	void Hatching::AddContourCollision(const std::vector<glm::vec2>& contourSegments) {
		for (int i = 0; i < contourSegments.size(); i += 2) {
			glm::vec2 pixPos1 = ViewToScreen(contourSegments[i]);
			glm::vec2 pixPos2 = ViewToScreen(contourSegments[i + 1]);
			float length = glm::distance(pixPos1, pixPos2);
			int steps = ceil(length / (DisplaySettings::LineTestDistance * 0.8f));
			for (int j = 0; j <= steps; j++) {
				float p = j / (float)steps;
				glm::vec2 pos = (1.0f - p) * pixPos1 + p * pixPos2;
				AddCollisionPoint(pos, true, nullptr);
			}
		}
	}
	
	void Hatching::CreateHatchingLines() {
		PrepareForHatching();

		UpdateHatchingLines();
		/*std::queue<HatchingLine*> linesQueue;

		if (m_HatchingLines.size() == 0) {
			//start with highest importance seed point
			ScreenSpaceSeed* start = &(m_ScreenSeeds[0]);
			for (ScreenSpaceSeed& current : m_ScreenSeeds) {
				if (current.m_Importance > start->m_Importance) {
					start = &current;
				}
			}
			//construct the first line
			HatchingLine first = ConstructLine(start);
			m_HatchingLines.push_back(first);
			AddLineCollision(&m_HatchingLines.back());
			linesQueue.push(&m_HatchingLines.back());
		}
		else {
			UpdateHatchingLines();
			for (auto it = m_HatchingLines.begin(); it != m_HatchingLines.end(); it++) {
				linesQueue.push(&(*it));
			}
		}

		if (!DisplaySettings::OnlyUpdateHatchLines){
			HatchingLine* currentLine = linesQueue.front();
			//Algorithm from Jobard and Lefer, 1997
			while (linesQueue.size() >= 1) {
				currentLine = linesQueue.front();
				ScreenSpaceSeed* candidate;
				if (FindSeedCandidate(candidate, currentLine)) {
					HatchingLine newLine = ConstructLine(candidate);
					m_HatchingLines.push_back(newLine);
					AddLineCollision(&m_HatchingLines.back());
					linesQueue.push(&m_HatchingLines.back());
					//m_UnusedPoints.erase(candidate);
					//CleanUnusedPoints();
				}
				else {
					linesQueue.pop();
				}
			}
			DisplaySettings::OnlyUpdateHatchLines = true; 
		}*/
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

	void Hatching::DrawCollisionPoints() {
		glBindVertexArray(m_CollisionVAO);
		glDrawArrays(GL_POINTS, 0, m_NumCollisionPoints);
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
		//TODO: SnakesMerge();
		
		// Before creating new Lines we need to know which Seeds are not covered by a line
		UpdateUnusedSeeds();
		SnakesInsert();

		//Sanity check
		for (HatchingLine line : m_HatchingLines) {
			assert(line.getSeeds().size() > 0);
		}
	}

	void Hatching::SnakesAdvect() {
		for (HatchingLine& line : m_HatchingLines) {
			const std::deque<glm::vec2>& points = line.getPoints();
			std::deque<glm::vec2> newPoints;
			for (int i = 0; i < points.size(); i++) {
				glm::vec2 point = points[i];
				glm::vec2 movement = ViewToScreen(glm::vec2(m_MovementData->Sample(point)));
				newPoints.push_back(point + movement);
			}
			line.MovePointsTo(newPoints);			
		}		
	}

	void Hatching::SnakesResample() {
		for (HatchingLine& line : m_HatchingLines) {
			if (line.NeedsResampling()) {
				line.Resample();
			}
			UpdateLineSeeds(line);
		}
	}

	void Hatching::SnakesRelax() {
		for (HatchingLine& line : m_HatchingLines) {
			const std::deque<glm::vec2>& originalPoints = line.getPoints();
			std::deque<glm::vec2> currPoints = originalPoints; //Copy of the starting points to be changed
			for (int iteration = 0; iteration < NumOptiSteps; iteration++) {
				std::deque<glm::vec2> newPoints;
				for (int i = 0; i < currPoints.size(); i++) {
					glm::vec2 bestPos = currPoints[i];
					float bestEval = 1000.0f;
					for (int x = -1; x <= 1; x++) {
						for (int y = -1; y <= 1; y++) {
							glm::vec2 candidatePos = currPoints[i] + glm::vec2(x * OptiStepSize, y * OptiStepSize);
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

	void Hatching::SnakesDelete() {
		for(auto it = m_HatchingLines.begin(); it != m_HatchingLines.end(); ) {
			HatchingLine& line = *it;
			bool lineAlive = line.HasVisibleSeeds();
			if (line.getPoints().size() <= 1) lineAlive = false;
			
			if (lineAlive) {
				line.PruneFront();
				lineAlive = line.HasVisibleSeeds();
			}

			if (lineAlive && line.HasMiddleCollision()) {
				HatchingLine* rest = line.SplitFromCollision();
				if(rest) m_HatchingLines.push_back(*rest);
				lineAlive = line.HasVisibleSeeds();
			}

			if (lineAlive && line.HasMiddleOcclusion()) {
				HatchingLine* rest = line.SplitFromOcclusion();
				if(rest) m_HatchingLines.push_back(*rest);
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

	void Hatching::SnakesSplit() {
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

	void Hatching::SnakesTrim() {
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

	void Hatching::SnakesExtend() {
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
			std::vector<ScreenSpaceSeed*> newSeeds;
			for (glm::vec2 point : extensionFront) {
				ScreenSpaceSeed* seed = FindNearbySeed(point, CoverRadius);
				if (seed) {
					newSeeds.push_back(seed);
				}
			}
			std::reverse(newSeeds.begin(), newSeeds.end());
			for (ScreenSpaceSeed* seed : oldSeeds) {
				newSeeds.push_back(seed);
			}
			for (glm::vec2 point : extensionBack) {
				ScreenSpaceSeed* seed = FindNearbySeed(point, CoverRadius);
				if (seed) {
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

	void Hatching::SnakesInsert() {
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
				ScreenSpaceSeed* candidate = nullptr;
				if (FindSeedCandidate(candidate, lineQueue.front())) {
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

	void Hatching::SnakesUpdateCollision() {
		for (HatchingLine& line : m_HatchingLines) {
			UpdateLineCollision(line);
		}
	}

	void Hatching::UpdateUnusedSeeds() {
		// The UnusedSeedGrid has been filled with all visible seeds in PrepareForHatching()
		// We only need to remove the ones that are covered by a line
		for (HatchingLine& line : m_HatchingLines) {
			const std::deque<ScreenSpaceSeed*>& seeds = line.getSeeds();
			for (ScreenSpaceSeed* seed : seeds) {
				glm::ivec2 gridPos = ScreenPosToGridPos(seed->m_Pos);
				std::unordered_set<ScreenSpaceSeed*>* gridCell = GetUnusedScreenSeeds(gridPos);
				m_NumUnusedSeeds -= gridCell->erase(seed);
			}
		}
	}

	float Hatching::EvaluatePointPos(HatchingLine& line, int index, glm::vec2 pointPos, const std::deque<glm::vec2>& points) {
		const std::deque<glm::vec2>& originalPoints = line.getPoints();

		//Compute Energy for associated Seed Points
		float eSeeds = 0.0f;
		std::vector<ScreenSpaceSeed*> associatedSeeds = line.getSeedsForPoint(index);
		for (ScreenSpaceSeed* seed : associatedSeeds) {
			if (glm::distance(seed->m_Pos, pointPos) <= CoverRadius) { //ignore Seeds that are too far away to still be covered
				float dist = glm::distance(pointPos, seed->m_Pos);
				dist /= CoverRadius;

				//float energy = exp(-2.5f * dist * dist) * dist * dist;  //this tries to get away from seeds that are already somewhat far away from the line to free them up for neighbors or new lines
				float energy = dist * dist;  //this just tries to stay close to the associated seeds
				eSeeds += energy;
			}
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
		glm::vec2 fieldDir = GetHatchingDir(pointPos);
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
			eSpring += abs(prevLengthOriginal - prevLength) / ExtendRadius;
		}
		if (index < points.size() - 1) {
			float nextLength = glm::distance(pointPos, points[index + 1]);
			float nextLengthOriginal = glm::distance(originalPoints[index], originalPoints[index + 1]);
			eSpring += abs(nextLengthOriginal - nextLength) / ExtendRadius;
		}

		float eTotal = eSeeds * OptiSeedWeight + eSmooth * OptiSmoothWeight + eField * OptiFieldWeight + eSpring * OptiSpringWeight;
		return eTotal;
	}

	bool Hatching::HasParallelNearby(glm::vec2 point, glm::vec2 dir, const HatchingLine& line) {
		std::vector<CollisionPoint> candidates;

		glm::vec2 normDir = glm::normalize(dir);

		glm::ivec2 gridPos = ScreenPosToGridPos(point);
		for (int x = -1; x <= 1; x++) {
			for (int y = -1; y <= 1; y++) {
				glm::ivec2 currGridPos = glm::clamp(gridPos + glm::ivec2(x, y), glm::ivec2(0, 0), m_GridSize - glm::ivec2(1, 1));
				std::unordered_set<CollisionPoint>& gridCell = *GetCollisionPoints(currGridPos);
				for (const CollisionPoint& colPoint : gridCell) {
					if (glm::distance(point, colPoint.m_Pos) < TrimRadius 
						&& !colPoint.m_isContour && colPoint.m_Line != &line)
						candidates.push_back(colPoint);
				}
			}
		}
		for (CollisionPoint& candidate : candidates) {
			glm::vec2 candDir = candidate.m_Line->getDirAt(candidate.m_Pos);			
			if (abs(glm::dot(candDir, normDir)) > cos(ParallelAngle))
				return true;
		}
		return false;
	}

	void Hatching::UpdateLineCollision(HatchingLine& line) {
		if (line.HasChanged()) {
			RemoveLineCollision(line);

			// Add the new Collision Points
			const std::deque<glm::vec2>& points = line.getPoints();
			for (glm::vec2 point : points) {
				if (IsInBounds(point)) {
					glm::ivec2 gridPos = ScreenPosToGridPos(point);
					std::unordered_set<CollisionPoint>& gridCell = *GetCollisionPoints(gridPos);
					CollisionPoint colPoint = { point, false, (HatchingLine * const)(&line) };
					gridCell.insert(colPoint);
				}
			}
			line.ResetChangedFlag();
		}
	}

	void Hatching::RemoveLineCollision(const HatchingLine& line) {
		float eps = 1e-6;
		if (line.HasChanged()) {
			const std::vector<glm::vec2>& points = line.getPointsBeforeChange();
			for (glm::vec2 point : points) {
				if (IsInBounds(point)) {
					glm::ivec2 gridPos = ScreenPosToGridPos(point);
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
				if (IsInBounds(point)) {
					glm::ivec2 gridPos = ScreenPosToGridPos(point);
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

	void Hatching::UpdateLineSeeds(HatchingLine& line) {
		// Find the new set of Seeds
		std::vector<ScreenSpaceSeed*> newSeeds;
		const std::deque<glm::vec2>& points = line.getPoints();
		for (glm::vec2 point : points) {
			if (IsInBounds(point)) {
				glm::ivec2 gridPos = ScreenPosToGridPos(point);
				std::unordered_set<ScreenSpaceSeed*>& gridCell = *GetVisibleScreenSeeds(gridPos);
				for (ScreenSpaceSeed* seed : gridCell) {
					if (glm::distance(point, seed->m_Pos) < CoverRadius) {
						// There might be a need to prevent adding the same Seed more than once, if the CoverRadius and ExtendRadius have a weird ratio
						// For now this should be fine
						newSeeds.push_back(seed);
					}
				}
			}
		}
		line.ReplaceSeeds(newSeeds);
	}

	void Hatching::PrepareForHatching() {
		// Reset Screen Seed Grid
		for (int i = 0; i < m_VisibleSeedsGrid.size(); i++) {
			m_VisibleSeedsGrid[i].clear();
			m_UnusedSeedsGrid[i].clear();
		}
		m_NumVisibleScreenSeeds = 0;
		// Fill Screen Seed Grid, this already omits seeds that collide with the contours
		m_NumUnusedSeeds = 0;
		for (ScreenSpaceSeed& seed : m_ScreenSeeds) {
			if (seed.m_Visible && !HasCollision(seed.m_Pos, true)) {
				glm::ivec2 gridPos = ScreenPosToGridPos(seed.m_Pos);
				GetVisibleScreenSeeds(gridPos)->insert(&seed);
				GetUnusedScreenSeeds(gridPos)->insert(&seed);
				m_NumUnusedSeeds++;
			}
		}
	}

	HatchingLine Hatching::ConstructLine(ScreenSpaceSeed* seed) { //This automatically removes any newly covered Seeds from the UnusedSeedGrid
		int maxPoints = DisplaySettings::NumPointsPerHatch;
		if (maxPoints <= 0) maxPoints = 999999;
		int pointsAdded = 0;

		std::vector<glm::vec2> linePoints;

		glm::vec2 first = seed->m_Pos;
		glm::vec2 dir = GetHatchingDir(first);
		glm::vec2 second = first + (ExtendRadius * dir);

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
		
		//Find all Seed Points near the line
		std::vector<ScreenSpaceSeed*> associatedSeeds;
		for (glm::vec2 point : linePoints) {
			ScreenSpaceSeed* newSeed = FindNearbySeed(point, CoverRadius);// FindSeedInRadius(currPos, CoverRadius);
			if (newSeed) {
				glm::ivec2 gridPos = ScreenPosToGridPos(newSeed->m_Pos);
				m_NumUnusedSeeds -= GetUnusedScreenSeeds(gridPos)->erase(newSeed);
				associatedSeeds.push_back(newSeed);
			}
		}

		return HatchingLine(linePoints, associatedSeeds, this);

		/*
		int maxPoints = DisplaySettings::NumPointsPerHatch;
		if (maxPoints <= 0) maxPoints = 999999;

		glm::vec2 dir;
		glm::vec2 sampleDir;

		std::vector<glm::vec2> linePoints;
		
		glm::vec2 currPos = seed->m_Pos;
		linePoints.push_back(currPos);
		dir = GetHatchingDir(currPos);

		//Extrude Left
		while (!HasCollision(currPos, false) && (linePoints.size() < maxPoints)) {
			// Compute direction for next step
			sampleDir = GetHatchingDir(currPos);
			float diffAngle = glm::dot(dir, sampleDir);
			if (diffAngle < 0) sampleDir = -sampleDir;
			dir = sampleDir;

			// Step in the computed direction, add point to line
			currPos = currPos + (ExtendRadius * dir);
			linePoints.push_back(currPos);
		}
		//Reverse the first half of the points
		std::reverse(linePoints.begin(), linePoints.end());

		dir = -GetHatchingDir(seed->m_Pos);
		currPos = seed->m_Pos + (ExtendRadius * dir);
		linePoints.push_back(currPos);
		//Extrude Right
		while (!HasCollision(currPos, false) && (linePoints.size() < (maxPoints * 2 + 1))) {
			// Compute direction for next step
			sampleDir = GetHatchingDir(currPos);
			float diffAngle = glm::dot(dir, sampleDir);
			if (diffAngle < 0) sampleDir = -sampleDir;
			dir = sampleDir;

			// Step in the computed direction, add point to line
			currPos = currPos + (ExtendRadius * dir);
			linePoints.push_back(currPos);
		}

		//Find all Seed Points near the line
		std::vector<ScreenSpaceSeed*> associatedSeeds;
		for (glm::vec2 point : linePoints) {
			ScreenSpaceSeed* newSeed = FindNearbySeed(point, CoverRadius);// FindSeedInRadius(currPos, CoverRadius);
			if (newSeed) {
				glm::ivec2 gridPos = ScreenPosToGridPos(newSeed->m_Pos);
				m_NumUnusedSeeds -= GetUnusedScreenSeeds(gridPos)->erase(newSeed);
				associatedSeeds.push_back(newSeed);
			}
		}

		return HatchingLine(linePoints, associatedSeeds, this);
		*/
	}
	
	std::vector<glm::vec2> Hatching::ExtendLine(glm::vec2 tip, glm::vec2 second) {
		std::vector<glm::vec2> newPoints;
		bool finished = false;

		glm::vec2 currPos = tip;
		glm::vec2 dir = glm::normalize(tip - second);

		while (!finished) {
			float bestScore = 0.0f;
			glm::vec2 bestCandidate;
			for (int i = -2; i <= 2; i++) {
				glm::vec2 deviation = glm::vec2(1.0f, i * ParallelAngle * 0.5f); //in polar form
				glm::vec2 candidateDir = glm::normalize(polarToCartesian(cartesianToPolar(dir) + deviation));
				glm::vec2 candidatePos = currPos + candidateDir * ExtendRadius;
				glm::vec2 candidateSampleDir = GetHatchingDir(candidatePos);
				if (glm::dot(candidateDir, candidateSampleDir) < 0) candidateSampleDir = -candidateSampleDir;

				if (glm::dot(candidateDir, candidateSampleDir) > cos(ParallelAngle)) {
					float score = std::min(glm::dot(dir, candidateDir), glm::dot(candidateDir, candidateSampleDir)) - cos(ParallelAngle);
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

	///////////////////////////////
	//  OLD STUFF BEFORE SNAKES  //
	///////////////////////////////

	bool Hatching::FindSeedCandidate(ScreenSpaceSeed*& out, HatchingLine* currentLine) {
		// TODO: Rework this to make sure even seed points further away from a line get selected eventually
		float closestDistance = 1000.0f;
		bool foundCandidate = false;
		ScreenSpaceSeed* candidate;
		const std::deque<glm::vec2>& currentPoints = currentLine->getPoints();
		for (int i = 0; i < currentPoints.size(); i++) {
			glm::ivec2 gridPos = ScreenPosToGridPos(currentPoints[i]);
			for (int x = -1; x <= 1; x++) {
				for (int y = -1; y <= 1; y++) {
					glm::ivec2 currGridPos = glm::clamp(gridPos + glm::ivec2(x, y), glm::ivec2(0), (m_GridSize - glm::ivec2(1)));
					std::unordered_set<ScreenSpaceSeed*>& gridCell = *GetUnusedScreenSeeds(currGridPos);
					for (ScreenSpaceSeed* currSeed : gridCell) {
						glm::vec2 compCoords = currSeed->m_Pos;
						float dist = glm::distance(currentPoints[i], compCoords);
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
	/*HatchingLine Hatching::CreateLine(ScreenSpaceSeed* seed) {
		//DEBUGGING
		int maxPoints = DisplaySettings::NumPointsPerHatch;
		if (maxPoints <= 0) maxPoints = 999999;
		int pointsLeft = 0;

		float maxAngle = CurvatureClampAngle;
		float terminationAngle = CurvatureBreakAngle;
		float stepsize = ExtendRadius;
		float seedRadius = CoverRadius;
		
		glm::vec2 dir;
		glm::vec2 sampleDir;

		std::vector<glm::vec2> linePoints;
		std::vector<ScreenSpaceSeed*> associatedSeeds;
		int numPoints = 1;

		std::vector<glm::vec2> leftPoints;
		std::vector<ScreenSpaceSeed*> leftSeeds;

		glm::vec2 currPos = seed->m_Pos;
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
			if (abs(diffAngle) < cos(terminationAngle)) break; // stop line if the bend would be more than terminatioAngle
			dir = clampToMaxAngleDifference(sampleDir, dir, radToDeg(maxAngle)); // limit strength of bend to maxangle degrees per step
			//dir = sampleDir;
			
			// Step in the computed direction, add point to line
			currPos = currPos + (stepsize * dir);
			leftPoints.push_back(currPos);
			numPoints++;
			pointsLeft++;
		}
		while (!leftPoints.empty()) {
			linePoints.push_back(leftPoints.back());
			leftPoints.pop_back();
		}
		while (!leftSeeds.empty()) {
			associatedSeeds.push_back(leftSeeds.back());
			leftSeeds.pop_back();
		}

		dir = -GetHatchingDir(seed->m_Pos);		
		currPos = seed->m_Pos + (stepsize * dir);		
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
			if (abs(diffAngle) < cos(terminationAngle)) break; // stop line if the bend would be more than terminatioAngle
			dir = clampToMaxAngleDifference(sampleDir, dir, radToDeg(maxAngle)); // limit strength of bend to maxangle degrees per step
			//dir = sampleDir;

			// Step in the computed direction, add point to line
			currPos = currPos + (stepsize * dir);
			linePoints.push_back(currPos);
			numPoints++;
		}
		return HatchingLine(linePoints, associatedSeeds, this);
	}*/

	void Hatching::AddLineCollision(HatchingLine* line) {
		for (glm::vec2 point : line->getPoints()) {
			AddCollisionPoint(point, false, line);
		}
	}

	void Hatching::AddCollisionPoint(glm::vec2 screenPos, bool isContour, HatchingLine* line) {
		if (!IsInBounds(screenPos))
			return;

		// Add Collision Point
		glm::ivec2 gridPos = ScreenPosToGridPos(screenPos);
		std::unordered_set<CollisionPoint>& colGridCell = *GetCollisionPoints(gridPos);
		CollisionPoint newPoint = { screenPos, isContour, line };
		colGridCell.insert(newPoint);

		// Remove colliding Seed Points
		for (int x = -1; x <= 1; x++) {
			for (int y = -1; y <= 1; y++) {
				glm::ivec2 currGridPos = gridPos + glm::ivec2(x, y);
				if (currGridPos.x >= 0 && currGridPos.x < m_GridSize.x
					&& currGridPos.y >= 0 && currGridPos.y < m_GridSize.y) {
					std::unordered_set<ScreenSpaceSeed*>& seedGridCell = *GetUnusedScreenSeeds(currGridPos);
					for (auto it = seedGridCell.begin(); it != seedGridCell.end(); ) {
						glm::vec2 seedPos = (*it)->m_Pos;
						if (glm::distance(seedPos, screenPos) < DisplaySettings::LineTestDistance) it = seedGridCell.erase(it);
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
				screenSeedPos.push_back(ScreenToView(seed.m_Pos));
				m_NumVisibleScreenSeeds++;
			}
		}

		glBindVertexArray(m_ScreenSeedsVAO);
		glBindBuffer(GL_ARRAY_BUFFER, m_ScreenSeedsVBO);
		glBufferData(GL_ARRAY_BUFFER, screenSeedPos.size() * 2 * sizeof(float), screenSeedPos.data(), GL_DYNAMIC_DRAW);

		// Fill Buffers for Hatching Lines
		std::vector<glm::vec2> vertices;
		std::vector<unsigned int> indices;
		m_NumLinesIndices = 0;
		unsigned int offset = 0;

		for (auto it = m_HatchingLines.begin(); it != m_HatchingLines.end(); it++) {
			HatchingLine& line = *it;
			const std::deque<glm::vec2>& linePoints = line.getPoints();

			// vertex data
			for (int i = 0; i < linePoints.size(); i++) {
				vertices.push_back(ScreenToView(linePoints[i]));
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
				colPoints.push_back(ScreenToView(point.m_Pos));
				m_NumCollisionPoints++;
			}
		}

		glBindVertexArray(m_CollisionVAO);
		glBindBuffer(GL_ARRAY_BUFFER, m_CollisionVBO);
		glBufferData(GL_ARRAY_BUFFER, colPoints.size() * 2 * sizeof(float), colPoints.data(), GL_STREAM_DRAW);
	}
	
	void Hatching::UpdateScreenSeedIdMap() {
		m_ScreenSeedIdMap.clear();
		for (ScreenSpaceSeed& seed : m_ScreenSeeds) {
			m_ScreenSeedIdMap[seed.m_Id] = &seed;
		}
	}

	std::unordered_set<CollisionPoint>* Hatching::GetCollisionPoints(glm::ivec2 gridPos) {
		return &m_CollisionPointsGrid[gridPos.y * m_GridSize.x + gridPos.x];
	}

	std::unordered_set<ScreenSpaceSeed*>* Hatching::GetVisibleScreenSeeds(glm::ivec2 gridPos) {
		return &m_VisibleSeedsGrid[gridPos.y * m_GridSize.x + gridPos.x];
	}

	std::unordered_set<ScreenSpaceSeed*>* Hatching::GetUnusedScreenSeeds(glm::ivec2 gridPos) {
		return &m_UnusedSeedsGrid[gridPos.y * m_GridSize.x + gridPos.x];
	}

	bool Hatching::HasCollision(glm::vec2 screenPos, bool onlyContours) {
		if (!IsInBounds(screenPos)) return true;
		glm::ivec2 gridCenter = ScreenPosToGridPos(screenPos);
		for (int x = -1; x <= 1; x++) {
			for (int y = -1; y <= 1; y++) {
				glm::ivec2 gridPos = gridCenter + glm::ivec2(x, y);
				if (gridPos.x >= 0 && gridPos.x < m_GridSize.x && gridPos.y >= 0 && gridPos.y < m_GridSize.y) {
					std::unordered_set<CollisionPoint>& gridCell = *GetCollisionPoints(gridPos);
					for (CollisionPoint point : gridCell) {
						float dist = glm::distance(screenPos, point.m_Pos);
						if (dist < DisplaySettings::LineTestDistance) {
							if(!onlyContours || point.m_isContour) return true;
						}
					}
				}
			}
		}
		return false;
	}

	glm::vec2 Hatching::ViewToScreen(glm::vec2 screenPos) {
		return screenPos * m_ViewportSize;
	}

	glm::vec2 Hatching::ScreenToView(glm::vec2 pixPos) {
		return pixPos / m_ViewportSize;
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

	ScreenSpaceSeed* Hatching::FindNearbySeed(glm::vec2 screenPos, float maxDistance) { //This only finds Seeds in the UnusedSeedsGrid
		if (!IsInBounds(screenPos))
			return nullptr;

		glm::ivec2 gridPos = ScreenPosToGridPos(screenPos);
		float closestDist = 1000.0f;
		ScreenSpaceSeed* result = nullptr;

		for (int x = -1; x <= 1; x++) {
			for (int y = -1; y <= 1; y++) {
				glm::ivec2 currGridPos = gridPos + glm::ivec2(x, y);
				if (currGridPos.x >= 0 && currGridPos.x < m_GridSize.x
					&& currGridPos.y >= 0 && currGridPos.y < m_GridSize.y) {
					//std::unordered_set<ScreenSpaceSeed*>& seedGridCell = *GetUnusedScreenSeeds(currGridPos); // no more
					std::unordered_set<ScreenSpaceSeed*>& seedGridCell = *GetVisibleScreenSeeds(currGridPos);
					for (ScreenSpaceSeed* seed : seedGridCell) {
						glm::vec2 seedPos = seed->m_Pos;
						float dist = glm::distance(seedPos, screenPos);
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
		glm::vec2 viewPos = ScreenToView(screenPos);
		int x = ceil(viewPos.x * m_GridSize.x) - 1;
		int y = ceil(viewPos.y * m_GridSize.y) - 1;
		return glm::ivec2(x, y);
	}

	bool Hatching::IsInBounds(glm::vec2 screenPos) {
		return (screenPos.x >= 0 && screenPos.x < m_ViewportSize.x
			&& screenPos.y >= 0 && screenPos.y < m_ViewportSize.y);
	}
	   
}