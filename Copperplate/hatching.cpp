#pragma once
#include "hatching.h"
#include "utility.h"

#include <random>
#include <queue>

float GridCellSize = 8.0f;

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
		int gridSizeX = (int)(m_ViewportSize.x / GridCellSize) + 1;
		int gridSizeY = (int)(m_ViewportSize.y / GridCellSize) + 1;
		m_GridSize = glm::ivec2(gridSizeX, gridSizeY);

		m_VisibleSeedsGrid = std::vector<std::unordered_set<ScreenSpaceSeed*>>();
		m_VisibleSeedsGrid.reserve(gridSizeX * gridSizeY);
		
		for (int i = 0; i < gridSizeX; i++) {
			for (int j = 0; j < gridSizeY; j++) {
				m_VisibleSeedsGrid.push_back(std::unordered_set<ScreenSpaceSeed*>());
			}
		}

		m_NormalData = CreateUnique<Image>(m_ViewportSize.x, m_ViewportSize.y);
		m_CurvatureData = CreateUnique<Image>(m_ViewportSize.x, m_ViewportSize.y);
		m_GradientData = CreateUnique<Image>(m_ViewportSize.x, m_ViewportSize.y);
		m_MovementData = CreateUnique<Image>(m_ViewportSize.x, m_ViewportSize.y);

		/* Setup Hatching Layers and their parameters*/
		m_Layers = std::vector<Unique<HatchingLayer>>();
		HatchingSettings settings(3.0f, 20.0f, 10.0f, 1.5f, 3.0f, 0.0f, 0.4f, HD_LargestCurvature);
		m_Layers.push_back(CreateUnique<HatchingLayer>(m_GridSize, *this, settings));
		settings = HatchingSettings(3.0f, 20.0f, 10.0f, 1.5f, 3.0f, 0.4f, 0.7f, HD_ShadeNormal);
		m_Layers.push_back(CreateUnique<HatchingLayer>(m_GridSize, *this, settings));
		//settings = HatchingSettings(3.0f, 20.0f, 10.0f, 1.5f, 3.0f, 0.7f, 1.0f, HD_ShadeNormal);
		//m_Layers.push_back(CreateUnique<HatchingLayer>(m_GridSize, *this, settings));
		
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
		for (auto& layer : m_Layers) {
			layer->ResetCollisions();
		}
	}

	void Hatching::UpdateScreenSeed(unsigned int seedId, glm::vec2 pos, bool visible) {
		ScreenSpaceSeed* seed = GetScreenSeedById(seedId);
		seed->m_Pos = ViewToScreen(pos);
		seed->m_Visible = visible;
	}

	void Hatching::AddContourCollision(const std::vector<glm::vec2>& contourSegments) {
		for (auto& layer : m_Layers) {
			layer->AddContourCollision(contourSegments);
		}
	}
	
	void Hatching::CreateHatchingLines() {
		PrepareForHatching();

		for (auto& layer : m_Layers) {
			layer->Update();
		}		
		
		FillGLBuffers();
	}
	
	void Hatching::DrawScreenSeeds() {
		glBindVertexArray(m_ScreenSeedsVAO);
		glDrawArrays(GL_POINTS, 0, m_NumVisibleScreenSeeds);
	}

	void Hatching::DrawHatchingLines(Shared<Shader> shader) {
		for (auto& layer : m_Layers) {
			layer->Draw(shader);
		}
	}

	void Hatching::DrawCollisionPoints() {
		for (auto& layer : m_Layers) {
			layer->DrawCollision();
		}
	}

	void Hatching::GrabNormalData() {
		m_NormalData->CopyFrameBuffer();
	}

	void Hatching::GrabCurvatureData() {
		m_CurvatureData->CopyFrameBuffer();
	}

	void Hatching::GrabGradientData() {
		m_GradientData->CopyFrameBuffer();
	}

	void Hatching::GrabMovementData() {
		m_MovementData->CopyFrameBuffer();
	}

	glm::vec2 Hatching::SampleMovement(glm::vec2 point) {
		return ViewToScreen(glm::vec2(m_MovementData->Sample(point)));
	}

	void Hatching::SetLayer1Direction(EHatchingDirections newDir) {
		m_Layers.front()->m_Settings.m_Direction = newDir;
	}

	void Hatching::measureHatchingDensity(int numPoints, float radius) {
		std::map<int, int> histogram;
		HaltonSequence xSequence(5);
		HaltonSequence ySequence(7);
		for (int i = 0; i < numPoints; i++) {
			glm::vec2 pos = ViewToScreen(glm::vec2(xSequence.NextNumber(), ySequence.NextNumber()));
			int count = m_Layers.front()->CountNearbyColPoints(pos, radius);
			histogram[count] = histogram[count] + 1;
		}
		std::cout << "Histogram of point counts for " << numPoints << " Points, radius " << radius << std::endl;
		for (auto& elem : histogram) {
			std::cout << elem.first << " Points: " << elem.second << " Times" << std::endl;
		}
	}

	// PRIVATE FUNCTIONS //
		
	void Hatching::PrepareForHatching() {
		// Reset Screen Seed Grid
		for (int i = 0; i < m_VisibleSeedsGrid.size(); i++) {
			m_VisibleSeedsGrid[i].clear();
		}
		m_NumVisibleScreenSeeds = 0;
		// Fill Screen Seed Grid, this already omits seeds that collide with the contours
		for (ScreenSpaceSeed& seed : m_ScreenSeeds) {
			//TODO: is it a problem if i dont check for collision here?
			//if (seed.m_Visible && !HasCollision(seed.m_Pos, true)) {
			if (seed.m_Visible && IsInBounds(seed.m_Pos)) {
				glm::ivec2 gridPos = ScreenPosToGridPos(seed.m_Pos);
				GetVisibleScreenSeeds(gridPos)->insert(&seed);
			}
		}
	}

	std::vector<ScreenSpaceSeed*> Hatching::FindVisibleSeedsInRadius(glm::vec2 point, float radius) {
		std::vector<ScreenSpaceSeed*> out;
		if (IsInBounds(point)) {
			glm::ivec2 gridCenter = ScreenPosToGridPos(point);
			for (int x = -1; x <= 1; x++) {
				for (int y = -1; y <= 1; y++) {
					glm::ivec2 gridPos = gridCenter + glm::ivec2(x, y);
					gridPos = glm::clamp(gridPos, glm::ivec2(0), (m_GridSize - glm::ivec2(1)));
					std::unordered_set<ScreenSpaceSeed*>& gridCell = *GetVisibleScreenSeeds(gridPos);
					for (ScreenSpaceSeed* seed : gridCell) {
						if (glm::distance(point, seed->m_Pos) <= radius) {
							out.push_back(seed);
						}
					}
				}
			}
		}
		return out;
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

	}
	
	void Hatching::UpdateScreenSeedIdMap() {
		m_ScreenSeedIdMap.clear();
		for (ScreenSpaceSeed& seed : m_ScreenSeeds) {
			m_ScreenSeedIdMap[seed.m_Id] = &seed;
		}
	}

	std::unordered_set<ScreenSpaceSeed*>* Hatching::GetVisibleScreenSeeds(glm::ivec2 gridPos) {
		return &m_VisibleSeedsGrid[gridPos.y * m_GridSize.x + gridPos.x];
	}

	glm::vec2 Hatching::ViewToScreen(glm::vec2 screenPos) {
		return screenPos * m_ViewportSize;
	}

	glm::vec2 Hatching::ScreenToView(glm::vec2 pixPos) {
		return pixPos / m_ViewportSize;
	}

	glm::vec2 Hatching::GetHatchingDir(glm::vec2 screenPos, EHatchingDirections direction) {
		float eps = 1e-8;
		glm::vec2 dir;
		if (direction == EHatchingDirections::HD_LargestCurvature ||
			direction == EHatchingDirections::HD_SmallestCurvature) {
			dir = glm::vec2(m_CurvatureData->Sample(screenPos));
		}
		else if (direction == EHatchingDirections::HD_Normal ||
				direction == EHatchingDirections::HD_Tangent){
			dir = glm::vec2(m_NormalData->Sample(screenPos));
		}
		else {
			dir = glm::vec2(m_GradientData->Sample(screenPos));
		}

		if (glm::length(dir) > eps) dir = glm::normalize(dir);
		else dir = glm::vec2(0.0f);
		
		if (direction == EHatchingDirections::HD_SmallestCurvature ||
			direction == EHatchingDirections::HD_Tangent ||
			direction == EHatchingDirections::HD_ShadeNormal) {
			dir = glm::vec2(-dir.y, dir.x);
		}

		return dir;
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
		return (screenPos.x > 0 && screenPos.x < m_ViewportSize.x
			&& screenPos.y > 0 && screenPos.y < m_ViewportSize.y);
	}
	   
}