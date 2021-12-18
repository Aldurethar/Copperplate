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
		const glm::vec2 m_Pos;
		const bool m_isContour;
		HatchingLine *const  m_Line; //The pointer is constant, not the HatchingLine

		bool operator==(const CollisionPoint& other) const {
			return (m_Pos.x == other.m_Pos.x && m_Pos.y == other.m_Pos.y && m_Line == other.m_Line);
		}
	};
	
	class Hatching {
	public:

		//Constants
		static float LineDistance;
		static float CollisionRadius;
		static float CoverRadius;
		static float TrimRadius;
		static float ExtendRadius;
		static float MergeRadius;
		static float SplitAngle;
		static float ParallelAngle;
		static float ExtendStraightVsCloseWeight;
		static float OptiStepSize;
		static int NumOptiSteps;
		static float OptiSeedWeight;
		static float OptiSmoothWeight;
		static float OptiFieldWeight;
		static float OptiSpringWeight;

		static void RecalculateConstants();
		
		Hatching(int viewportWidth, int viewportHeight);
	
		void CreateSeedPoints(std::vector<SeedPoint>& outSeedPoints, Mesh& mesh, unsigned int objectId, int totalPoints);

		void ResetCollisions();

		void UpdateScreenSeed(unsigned int seedId, glm::vec2 pos, bool visible);
		void AddContourCollision(const std::vector<glm::vec2>& contourSegments);

		void CreateHatchingLines();

		void DrawScreenSeeds();
		void DrawHatchingLines();
		void DrawCollisionPoints();
				
		void GrabNormalData();
		void GrabCurvatureData();
		void GrabGradientData();
		void GrabMovementData();
		
		bool HasCollision(glm::vec2 screenPos, bool onlyContours);
		glm::vec2 ViewToScreen(glm::vec2 screenPos);
		glm::vec2 ScreenToView(glm::vec2 pixPos);

	private:

		void UpdateHatchingLines();
		void SnakesAdvect();
		void SnakesResample();
		void SnakesRelax();
		void SnakesDelete();
		void SnakesSplit();
		void SnakesTrim();
		void SnakesExtend();
		void SnakesMerge();
		void SnakesInsert();
		
		void SnakesUpdateCollision();

		void UpdateUnusedSeeds();

		float EvaluatePointPos(HatchingLine& line, int index, glm::vec2 pointPos, const std::deque<glm::vec2>& points);
		bool HasParallelNearby(glm::vec2 point, glm::vec2 dir, const HatchingLine& line);
		void UpdateLineCollision(HatchingLine& line);
		void RemoveLineCollision(const HatchingLine& line);
		void UpdateLineSeeds(HatchingLine& line);
		HatchingLine ConstructLine(ScreenSpaceSeed* seed);
		std::vector<glm::vec2> ExtendLine(glm::vec2 tip, glm::vec2 second);
		std::vector<const CollisionPoint*> FindMergeCandidates(glm::vec2 tip, glm::vec2 tipDir, HatchingLine* line);
		float EvaluateMergeCandidate(const HatchingLine& line, const CollisionPoint* candidate, bool mergeToFront);
		std::vector<ScreenSpaceSeed*> FindSeedsInRadius(glm::vec2 point, float radius);


		void PrepareForHatching();
		bool FindSeedCandidate(ScreenSpaceSeed*& out, HatchingLine* currentLine);
		void AddCollisionPoint(glm::vec2 screenPos, bool isContour, HatchingLine* line);

		void FillGLBuffers();
		void UpdateScreenSeedIdMap();
		
		std::unordered_set<CollisionPoint>* GetCollisionPoints(glm::ivec2 gridPos);
		std::unordered_set<ScreenSpaceSeed*>* GetVisibleScreenSeeds(glm::ivec2 gridPos);
		std::unordered_set<ScreenSpaceSeed*>* GetUnusedScreenSeeds(glm::ivec2 gridPos);
				
		glm::vec2 GetHatchingDir(glm::vec2 screenPos);
		ScreenSpaceSeed* GetScreenSeedById(unsigned int id);
		glm::ivec2 ScreenPosToGridPos(glm::vec2 screenPos);
		bool IsInBounds(glm::vec2 screenPos);

		glm::vec2 m_ViewportSize;
		std::list<HatchingLine> m_HatchingLines;

		std::vector<ScreenSpaceSeed> m_ScreenSeeds;
		std::map<unsigned int, ScreenSpaceSeed*> m_ScreenSeedIdMap;
		
		glm::ivec2 m_GridSize;
		std::vector<std::unordered_set<ScreenSpaceSeed*>> m_VisibleSeedsGrid;
		std::vector<std::unordered_set<ScreenSpaceSeed*>> m_UnusedSeedsGrid;
		std::vector<std::unordered_set<CollisionPoint>> m_CollisionPointsGrid;
		int m_NumUnusedSeeds;

		Unique<Image> m_NormalData;
		Unique<Image> m_CurvatureData;
		Unique<Image> m_GradientData;
		Unique<Image> m_MovementData;

		unsigned int m_ScreenSeedsVAO;
		unsigned int m_ScreenSeedsVBO;
		int m_NumVisibleScreenSeeds;

		unsigned int m_LinesVAO;
		unsigned int m_LinesVertexBuffer;
		unsigned int m_LinesIndexBuffer;
		int m_NumLinesIndices;

		unsigned int m_CollisionVAO;
		unsigned int m_CollisionVBO;
		int m_NumCollisionPoints;

	};
	
}

// Hash Function for CollisionPoint so we can use it in a std::unordered_set
namespace std {
	template<>
	struct hash<Copperplate::CollisionPoint> {
		const size_t operator()(const Copperplate::CollisionPoint& point) const {
			return std::hash<float>()(point.m_Pos.x) ^ std::hash<float>()(point.m_Pos.y);
		}
	};
}