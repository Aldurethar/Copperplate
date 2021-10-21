#pragma once

#include "mesh.h"
#include <unordered_set>

namespace Copperplate {

	const float LINE_SEPARATION_DISTANCE = 12.0f;
	const float LINE_TEST_DISTANCE = 8.0f;

	struct SeedPoint {
		glm::vec3 m_Pos;
		Face& m_Face;
		float m_Importance;
	};

	struct ScreenSpaceSeed {
		glm::vec2 m_Pos;
		float m_Importance;
	};

	struct HatchingLine {
		int m_NumPoints;
		int m_LeftPoints;
		std::vector<glm::vec2> m_Points;
	};
	class Hatching {
	public:
		
		Hatching(int viewportWidth, int viewportHeight);

		void ResetSeeds();
		void ResetHatching();
		void AddSeeds(const std::vector<ScreenSpaceSeed>& seeds);
		void CreateHatchingLines();
		void DrawHatchingLines();

	private:

		HatchingLine CreateLine(ScreenSpaceSeed* seed);
		bool FindSeedCandidate(ScreenSpaceSeed*& out, HatchingLine currentLine);
		void FillGLBuffers();
		std::vector<glm::vec2>* GetCollisionPoints(glm::ivec2 gridPos);
		glm::ivec2 ScreenPosToGridPos(glm::vec2 screenPos);
		bool HasCollision(glm::vec2 screenPos);
		void AddCollisionPoint(glm::vec2 screenPos);
		void CleanUnusedPoints();

		glm::vec2 m_ViewportSize;
		std::vector<HatchingLine> m_HatchingLines;

		std::vector<ScreenSpaceSeed> m_Seedpoints;
		std::unordered_set<ScreenSpaceSeed*> m_UnusedPoints;

		glm::ivec2 m_GridSize;
		std::vector<std::vector<glm::vec2>> m_CollisionPoints;

		unsigned int m_LinesVAO;
		unsigned int m_LinesVertexBuffer;
		unsigned int m_LinesIndexBuffer;
		int m_LinesIndexNum;
	};
	void CreateSeedPoints(std::vector<SeedPoint>& outSeedPoints, Mesh& mesh, int totalPoints, int maxPointsPerFace);

	class HaltonSequence {
	public:

		HaltonSequence(int base);

		void Skip(int amount);

		float NextNumber();

	private:

		int m_Base;
		int m_Numerator;
		int m_Denominator;
	};
}