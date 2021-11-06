#pragma once

#include "mesh.h"
#include <unordered_set>
#include <map>
#include "image.h"

namespace Copperplate {

	const float LINE_SEPARATION_DISTANCE = 8.0f;
	const float LINE_TEST_DISTANCE = 4.0f;

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
	};
	

	class Hatching {
	public:
		
		Hatching(int viewportWidth, int viewportHeight);
	
		void CreateSeedPoints(std::vector<SeedPoint>& outSeedPoints, Mesh& mesh, unsigned int objectId, int totalPoints, int maxPointsPerFace);

		void ResetCollisions();
		//void ResetHatching();

		void UpdateScreenSeed(unsigned int seedId, glm::vec2 pos, bool visible);
		void AddContourCollision(const std::vector<glm::vec2>& contourSegments);
		//void AddSeeds(const std::vector<ScreenSpaceSeed>& seeds);
		void GrabNormalData();
		void GrabCurvatureData();


		void DrawScreenSeeds();

		void CreateHatchingLines();
		void DrawHatchingLines();

	private:

		void PrepareForHatching();
		HatchingLine CreateLine(ScreenSpaceSeed* seed);
		bool FindSeedCandidate(ScreenSpaceSeed*& out, HatchingLine currentLine);
		void FillGLBuffers();
		std::vector<glm::vec2>* GetCollisionPoints(glm::ivec2 gridPos);
		std::unordered_set<ScreenSpaceSeed*>* GetUnusedScreenSeeds(glm::ivec2 gridPos);
		glm::ivec2 ScreenPosToGridPos(glm::vec2 screenPos);
		bool HasCollision(glm::vec2 screenPos);
		void AddCollisionPoint(glm::vec2 screenPos);
		//void CleanUnusedPoints();
		glm::vec2 GetHatchingDir(glm::vec2 screenPos);
		ScreenSpaceSeed* GetScreenSeedById(unsigned int id);
		void UpdateScreenSeedIdMap();

		glm::vec2 m_ViewportSize;
		std::vector<HatchingLine> m_HatchingLines;

		std::vector<ScreenSpaceSeed> m_ScreenSeeds;
		std::map<unsigned int, ScreenSpaceSeed*> m_ScreenSeedIdMap;

		//std::vector<ScreenSpaceSeed> m_Seedpoints;
		//std::unordered_set<ScreenSpaceSeed*> m_UnusedPoints;

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