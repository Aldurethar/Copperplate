#pragma once
#include "hatchinglayer.h"
#include "mesh.h"
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
	
	class Hatching {
		friend class HatchingLayer;
	public:
		
		Hatching(int viewportWidth, int viewportHeight);
	
		void CreateSeedPoints(std::vector<SeedPoint>& outSeedPoints, Mesh& mesh, unsigned int objectId, int totalPoints);

		void ResetCollisions();

		void UpdateScreenSeed(unsigned int seedId, glm::vec2 pos, bool visible);
		void AddContourCollision(const std::vector<glm::vec2>& contourSegments);

		void CreateHatchingLines();

		void DrawScreenSeeds();
		void DrawHatchingLines(Shared<Shader> shader);
		void DrawCollisionPoints();
				
		void GrabNormalData();
		void GrabCurvatureData();
		void GrabGradientData();
		void GrabMovementData();
		
		bool IsInBounds(glm::vec2 screenPos);
		glm::vec2 ViewToScreen(glm::vec2 screenPos);
		glm::vec2 ScreenToView(glm::vec2 pixPos);

		glm::vec2 SampleMovement(glm::vec2 point);	

		//DEBUG
		void SetLayer1Direction(EHatchingDirections newDir);
		void measureHatchingDensity(int numPoints, float radius);

	private:

		std::vector<ScreenSpaceSeed*> FindVisibleSeedsInRadius(glm::vec2 point, float radius);

		void PrepareForHatching();
		void FillGLBuffers();

		void UpdateScreenSeedIdMap();

		std::unordered_set<ScreenSpaceSeed*>* GetVisibleScreenSeeds(glm::ivec2 gridPos);
				
		glm::vec2 GetHatchingDir(glm::vec2 screenPos, EHatchingDirections direction);
		ScreenSpaceSeed* GetScreenSeedById(unsigned int id);
		glm::ivec2 ScreenPosToGridPos(glm::vec2 screenPos);

		// Member Variables
		glm::vec2 m_ViewportSize;

		std::vector<Unique<HatchingLayer>> m_Layers;

		std::vector<ScreenSpaceSeed> m_ScreenSeeds;
		std::map<unsigned int, ScreenSpaceSeed*> m_ScreenSeedIdMap;
		
		glm::ivec2 m_GridSize;
		std::vector<std::unordered_set<ScreenSpaceSeed*>> m_VisibleSeedsGrid;

		Unique<Image> m_NormalData;
		Unique<Image> m_CurvatureData;
		Unique<Image> m_GradientData;
		Unique<Image> m_MovementData;

		unsigned int m_ScreenSeedsVAO;
		unsigned int m_ScreenSeedsVBO;
		int m_NumVisibleScreenSeeds;
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