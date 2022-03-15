#pragma once
#include "hatchingline.h"
#include "mesh.h"
#include "shader.h"
#include "rendering.h"
#include "utility.h"
#include <unordered_set>


namespace Copperplate {

	struct CollisionPoint {
		const glm::vec2 m_Pos;
		const bool m_isContour;
		HatchingLine* const  m_Line; //The pointer is constant, not the HatchingLine

		bool operator==(const CollisionPoint& other) const {
			return (m_Pos.x == other.m_Pos.x && m_Pos.y == other.m_Pos.y && m_Line == other.m_Line);
		}
	};

	struct HatchingSettings {
		float m_LineDistance =		4.0f;
		float m_CollisionRadius =	m_LineDistance * 0.7f;
		float m_CoverRadius =		m_LineDistance * 1.0f;
		float m_TrimRadius =		m_LineDistance * 0.7f;
		float m_ExtendRadius =		m_LineDistance * 1.2f;
		float m_MergeRadius =		m_LineDistance * 1.3f;
		float m_SplitAngle =		degToRad(20);
		float m_ParallelAngle =		degToRad(10);
		float m_ExtendStraightVsCloseWeight = 0.8f;
		float m_OptiStepSize = 1.0f;
		int	  m_NumOptiSteps = 4;
		float m_OptiSeedWeight = 2.0f;
		float m_OptiSmoothWeight = 5.0f;
		float m_OptiFieldWeight = 3.0f;
		float m_OptiSpringWeight = 2.0f;

		float m_MinLineWidth =	1.0f;
		float m_MaxLineWidth =	3.0f;
		float m_MinShade =		0.0f;
		float m_MaxShade =		1.0f;
		EHatchingDirections m_Direction = EHatchingDirections::HD_LargestCurvature;

		HatchingSettings(float lineDistance, float splitAngleDeg, float parallelAngleDeg, float minWidth, float maxWidth, float minShade, float maxShade, EHatchingDirections direction) {
			m_LineDistance = lineDistance;
			m_CollisionRadius = m_LineDistance * 0.7f;
			m_CoverRadius = m_LineDistance * 1.0f;
			m_TrimRadius = m_LineDistance * 0.7f;
			m_ExtendRadius = m_LineDistance * 1.2f;
			m_MergeRadius = m_LineDistance * 1.3f;
			m_SplitAngle = degToRad(splitAngleDeg);
			m_ParallelAngle = degToRad(parallelAngleDeg);

			m_MinLineWidth = minWidth;
			m_MaxLineWidth = maxWidth;
			m_MinShade = minShade;
			m_MaxShade = maxShade;
			m_Direction = direction;
		}
	};

	//Forward Declarations
	struct ScreenSpaceSeed;
	class Hatching;

	class HatchingLayer {

	public:

		HatchingLayer(glm::ivec2 gridSize, Hatching& hatching, HatchingSettings settings);

		void ResetCollisions();
		void AddContourCollision(const std::vector<glm::vec2>& contourSegments);

		void Update();		
		void Draw(Shared<Shader> shader);
		void DrawCollision();
		
		bool HasCollision(glm::vec2 screenPos, bool onlyContours);
		
		HatchingSettings m_Settings;

		//For Statistics
		int CountNearbyColPoints(glm::vec2 screenPos, float radius);

	private:

		void UpdateLines();
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

		HatchingLine ConstructLine(ScreenSpaceSeed* seed);
		std::vector<glm::vec2> ExtendLine(glm::vec2 tip, glm::vec2 second);
		
		void UpdateLineSeeds(HatchingLine& line);
		ScreenSpaceSeed* FindSeedCandidate(HatchingLine* currentLine);
				
		std::vector<const CollisionPoint*> FindMergeCandidates(glm::vec2 tip, glm::vec2 tipSecond, HatchingLine* line);
		float EvaluateMergeCandidate(const HatchingLine& line, const CollisionPoint* candidate, bool mergeToFront);
		float EvaluatePointPos(HatchingLine& line, int index, glm::vec2 pointPos, const std::deque<glm::vec2>& points);
		
		void RemoveLineCollision(const HatchingLine& line);
		void UpdateLineCollision(HatchingLine& line);

		void AddCollisionPoint(glm::vec2 screenPos, bool isContour, HatchingLine* line);
		bool HasParallelNearby(glm::vec2 point, glm::vec2 dir, const HatchingLine& line);

		void ResetUnusedSeeds();
		void UpdateUnusedSeeds();

		void FillGLBuffers();

		std::unordered_set<CollisionPoint>* GetCollisionPoints(glm::ivec2 gridPos);
		std::unordered_set<ScreenSpaceSeed*>* GetUnusedScreenSeeds(glm::ivec2 gridPos);


		//Member Variables

		Hatching& m_Hatching;
		std::list<HatchingLine> m_HatchingLines;
		
		glm::ivec2 m_GridSize;
		std::vector<std::unordered_set<ScreenSpaceSeed*>> m_UnusedSeedsGrid;
		std::vector<std::unordered_set<CollisionPoint>> m_CollisionPointsGrid;
		int m_NumUnusedSeeds;

		unsigned int m_LinesVAO;
		unsigned int m_LinesVertexBuffer;
		unsigned int m_LinesIndexBuffer;
		int m_NumLinesIndices;

		unsigned int m_CollisionVAO;
		unsigned int m_CollisionVBO;
		int m_NumCollisionPoints;


		
	};

}