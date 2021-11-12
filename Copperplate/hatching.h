#pragma once

#include "mesh.h"
#include <unordered_set>
#include <map>
#include "image.h"

namespace Copperplate {

	//const float LINE_SEPARATION_DISTANCE = 8.0f;
	//const float LINE_TEST_DISTANCE = 4.0f;

	struct SeedPoint {
		glm::vec3 m_Pos;
		Face& m_Face;
		float m_Importance;
		unsigned int m_Id;
	};

	struct ScreenSpaceSeed {
		glm::vec2 m_Pos;
		float m_Importance;
		unsigned int m_Id;
		bool m_Visible;
	};

	struct HatchingLine {
		int m_NumPoints;
		int m_LeftPoints;
		std::vector<glm::vec2> m_Points;
		std::vector<ScreenSpaceSeed*> m_AssociatedSeeds;
	};
	

	class Hatching {
	public:
		
		Hatching(int viewportWidth, int viewportHeight);
	
		void CreateSeedPoints(std::vector<SeedPoint>& outSeedPoints, Mesh& mesh, unsigned int objectId, int totalPoints, int maxPointsPerFace);

		void ResetCollisions();

		void UpdateScreenSeed(unsigned int seedId, glm::vec2 pos, bool visible);
		void AddContourCollision(const std::vector<glm::vec2>& contourSegments);

		void CreateHatchingLines();

		void DrawScreenSeeds();
		void DrawHatchingLines();
		
		void GrabNormalData();
		void GrabCurvatureData();

	private:

		void UpdateHatchingLines();

		void PrepareForHatching();
		bool FindSeedCandidate(ScreenSpaceSeed*& out, HatchingLine currentLine);
		HatchingLine CreateLine(ScreenSpaceSeed* seed, int prevNumPoints, int prevLeftPoints);
		void AddCollisionPoint(glm::vec2 screenPos);

		void FillGLBuffers();
		void UpdateScreenSeedIdMap();
		
		std::vector<glm::vec2>* GetCollisionPoints(glm::ivec2 gridPos);
		std::unordered_set<ScreenSpaceSeed*>* GetUnusedScreenSeeds(glm::ivec2 gridPos);

		bool HasCollision(glm::vec2 screenPos);
		glm::vec2 GetHatchingDir(glm::vec2 screenPos);
		ScreenSpaceSeed* FindNearbySeed(glm::vec2 screenPos, float maxDistance);
		ScreenSpaceSeed* GetScreenSeedById(unsigned int id);
		glm::ivec2 ScreenPosToGridPos(glm::vec2 screenPos);

		glm::vec2 m_ViewportSize;
		std::vector<HatchingLine> m_HatchingLines;

		std::vector<ScreenSpaceSeed> m_ScreenSeeds;
		std::map<unsigned int, ScreenSpaceSeed*> m_ScreenSeedIdMap;
		
		glm::ivec2 m_GridSize;
		std::vector<std::unordered_set<ScreenSpaceSeed*>> m_UnusedScreenSeeds;
		std::vector<std::vector<glm::vec2>> m_CollisionPoints;

		Unique<Image> m_NormalData;
		Unique<Image> m_CurvatureData;

		unsigned int m_ScreenSeedsVAO;
		unsigned int m_ScreenSeedsVBO;
		int m_NumVisibleScreenSeeds;

		unsigned int m_LinesVAO;
		unsigned int m_LinesVertexBuffer;
		unsigned int m_LinesIndexBuffer;
		int m_NumLinesIndices;

	};
	
}