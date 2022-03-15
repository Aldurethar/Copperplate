#pragma once
#include "core.h"
#include <chrono>
#include <map>

namespace Copperplate {

	enum ETimerType {
		T_RenderContour,
		T_UpdateHatch,
		T_RenderHatch,
		T_Advect,
		T_Resample,
		T_Relax,
		T_Topology,
		T_Delete,
		T_Split,
		T_Trim,
		T_Extend,
		T_Merge,
		T_Insert,
	};

	class StatTimer {
	public:
		StatTimer(ETimerType type);
		~StatTimer();

	private:
		ETimerType m_type;
		clock_t m_start;
	};

	struct StatFrame {
		int number;
		float totalTime;
		float renderContourTime;
		float updateHatchTime;
		float renderHatchTime;
		float advectTime;
		float resampleTime;
		float relaxTime;
		float topoTime;
		float deleteTime;
		float splitTime;
		float trimTime;
		float extendTime;
		float mergeTime;
		float insertTime;
		int numInsertions;
		int numDeletions;
		int numMerges;
		int numSplits;
		int numLines;
	};

	class Statistics {
	public:

		Statistics();

		void newFrame();

		void recordTime(ETimerType timeType, float elapsedTime);

		void countInsertion();
		void countDeletion();
		void countMerge();
		void countSplit();
		void countLines(int count);
		
		void printLastFrame();
		void printMeanValues();


		static Statistics& Get();

	private:
		void printFrame(StatFrame frame);
		StatFrame getMeanValues();

		int m_numFrames;

		clock_t m_currFrameStart;
		StatFrame m_currFrame;

		int m_currIndex;
		StatFrame m_buffer[20];
	};
	
#define TIME_FUNCTION(timertype) Unique<StatTimer> timer = CreateUnique<StatTimer>(timertype)
#define STAT_COUNT_INSERTION Statistics::Get().countInsertion()
#define STAT_COUNT_DELETION Statistics::Get().countDeletion()
#define STAT_COUNT_MERGE Statistics::Get().countMerge()
#define STAT_COUNT_SPLIT Statistics::Get().countSplit()
#define STAT_COUNT_LINES(count) Statistics::Get().countLines(count)

}