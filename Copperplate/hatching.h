#pragma once
#include "hatchingline.h"
#include "mesh.h"
#include <unordered_set>
#include <map>
#include "image.h"

namespace Copperplate {

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

	struct CollisionPoint {
		glm::vec2 m_Pos;
		bool m_isContour;
		HatchingLine* m_Line;
	};
	

	class Hatching {
	public:

		//Constants
		static float CoverRadius;
		static float TrimRadius;
		static float ExtendRadius;
		static float MergeRadius;
		static float SplitAngle;
		static float CurvatureBreakAngle;
		static float CurvatureClampAngle;
		static float ParallelAngle;
		
		Hatching(int viewportWidth, int viewportHeight);
	
		void CreateSeedPoints(std::vector<SeedPoint>& outSeedPoints, Mesh& mesh, unsigned int objectId, int totalPoints);

		void ResetCollisions();

		void UpdateScreenSeed(unsigned int seedId, glm::vec2 pos, bool visible);
		void AddContourCollision(const std::vector<glm::vec2>& contourSegments);

		void CreateHatchingLines();

		void DrawScreenSeeds();
		void DrawHatchingLines();
				
		void GrabNormalData();
		void GrabCurvatureData();
		void GrabMovementData();
		
		bool HasCollision(glm::vec2 screenPos, bool onlyContours);
		glm::vec2 ScreenToPix(glm::vec2 screenPos);
		glm::vec2 PixToScreen(glm::vec2 pixPos);

	private:

		void UpdateHatchingLines();
		void SnakesDelete();
		void SnakesSplit();
		
		void PrepareForHatching();
		bool FindSeedCandidate(ScreenSpaceSeed*& out, HatchingLine* currentLine);
		HatchingLine CreateLine(ScreenSpaceSeed* seed);
		void AddLineCollision(HatchingLine* line);
		void AddCollisionPoint(glm::vec2 screenPos, bool isContour, HatchingLine* line);

		void FillGLBuffers();
		void UpdateScreenSeedIdMap();
		
		std::vector<CollisionPoint>* GetCollisionPoints(glm::ivec2 gridPos);
		std::unordered_set<ScreenSpaceSeed*>* GetUnusedScreenSeeds(glm::ivec2 gridPos);
				
		glm::vec2 GetHatchingDir(glm::vec2 screenPos);
		ScreenSpaceSeed* FindNearbySeed(glm::vec2 screenPos, float maxDistance);
		ScreenSpaceSeed* GetScreenSeedById(unsigned int id);
		glm::ivec2 ScreenPosToGridPos(glm::vec2 screenPos);

		glm::vec2 m_ViewportSize;
		std::list<HatchingLine> m_HatchingLines;

		std::vector<ScreenSpaceSeed> m_ScreenSeeds;
		std::map<unsigned int, ScreenSpaceSeed*> m_ScreenSeedIdMap;
		
		glm::ivec2 m_GridSize;
		std::vector<std::unordered_set<ScreenSpaceSeed*>> m_UnusedScreenSeeds;
		std::vector<std::vector<CollisionPoint>> m_CollisionPoints;

		Unique<Image> m_NormalData;
		Unique<Image> m_CurvatureData;
		Unique<Image> m_MovementData;

		unsigned int m_ScreenSeedsVAO;
		unsigned int m_ScreenSeedsVBO;
		int m_NumVisibleScreenSeeds;

		unsigned int m_LinesVAO;
		unsigned int m_LinesVertexBuffer;
		unsigned int m_LinesIndexBuffer;
		int m_NumLinesIndices;

	};
	
}