#pragma once
#include "statistics.h"

namespace Copperplate {

	StatTimer::StatTimer(ETimerType type) {
		m_type = type;
		m_start = clock();
	}
	StatTimer::~StatTimer()	{
		clock_t now = clock();
		float elapsedTime = ((float)now - m_start) * 1000.0f / CLOCKS_PER_SEC;
		Statistics::Get().recordTime(m_type, elapsedTime);
	}


	Statistics::Statistics() {
		m_numFrames = 0;
		m_currIndex = 0;
		m_currFrame = StatFrame();
		m_currFrame.number = m_numFrames;
		m_currFrameStart = clock();
	}

	void Statistics::newFrame()	{
		clock_t now = clock();
		m_currFrame.totalTime = ((float)now - m_currFrameStart) * 1000.0f / CLOCKS_PER_SEC;
		m_buffer[m_currIndex] = m_currFrame;
		m_currIndex = (m_currIndex + 1) % 20;

		m_numFrames++;
		m_currFrame = StatFrame();
		m_currFrame.number = m_numFrames;
		m_currFrameStart = now;
	}

	void Statistics::recordTime(ETimerType timeType, float elapsedTime) {
		switch (timeType) {
		case T_RenderContour: m_currFrame.renderContourTime += elapsedTime; break;
		case T_UpdateHatch: m_currFrame.updateHatchTime += elapsedTime; break;
		case T_RenderHatch: m_currFrame.renderHatchTime += elapsedTime; break;
		case T_Advect: m_currFrame.advectTime += elapsedTime; break;
		case T_Resample: m_currFrame.resampleTime += elapsedTime; break;
		case T_Relax: m_currFrame.relaxTime += elapsedTime; break;
		case T_Topology: m_currFrame.topoTime += elapsedTime; break;
		case T_Delete: m_currFrame.deleteTime += elapsedTime; break;
		case T_Split: m_currFrame.splitTime += elapsedTime; break;
		case T_Trim: m_currFrame.trimTime += elapsedTime; break;
		case T_Extend: m_currFrame.extendTime += elapsedTime; break;
		case T_Merge: m_currFrame.mergeTime += elapsedTime; break;
		case T_Insert: m_currFrame.insertTime += elapsedTime; break;
		}
	}

	void Statistics::countInsertion() {
		m_currFrame.numInsertions++;
	}

	void Statistics::countDeletion() {
		m_currFrame.numDeletions++;
	}

	void Statistics::countMerge() {
		m_currFrame.numMerges++;
	}

	void Statistics::countSplit() {
		m_currFrame.numSplits++;
	}

	void Statistics::countLines(int count) {
		m_currFrame.numLines += count;
	}

	void Statistics::printLastFrame() {
		int index = (m_currIndex + 19) % 20;
		std::cout << "Last Frame Data:" << std::endl;
		printFrame(m_buffer[index]);
	}

	void Statistics::printMeanValues() {
		std::cout << "Mean Frame Stats from the last 20 frames:" << std::endl;
		printFrame(getMeanValues());
	}

	void Statistics::printFrame(StatFrame frame) {
		//std::cout << "Frame " << frame.number << "Total Time: " << frame.totalTime << "ms, divided among" << std::endl
		//	<< "Contour Rendering: " << frame.renderContourTime << "ms, Line Updating: " << frame.updateHatchTime << "ms, Line Rendering: " << frame.renderHatchTime << "ms" << std::endl;
		//std::cout << "Update had the steps: Advect " << frame.advectTime << "ms, Resample " << frame.resampleTime << "ms, Relax " << frame.relaxTime << "ms, Delete " << frame.deleteTime
		//	<< "ms, Split " << frame.splitTime << "ms, Trim " << frame.trimTime << "ms, Extend " << frame.extendTime << "ms, Merge " << frame.mergeTime << "ms, and Insert " << frame.insertTime << "ms." << std::endl;
		std::cout << "Frame " << frame.number << ": " << frame.numDeletions << " Deletions, " << frame.numSplits << " Splits, " << frame.numMerges << " Merges, " << frame.numInsertions << " Insertions. " << frame.numLines << " Lines in total." << std::endl;
	}

	StatFrame Statistics::getMeanValues() {
		StatFrame result = StatFrame();
		for (int i = 0; i < 20; i++) {
			result.totalTime += m_buffer[i].totalTime;
			result.renderContourTime += m_buffer[i].renderContourTime;
			result.updateHatchTime += m_buffer[i].updateHatchTime;
			result.renderHatchTime += m_buffer[i].renderHatchTime;
			result.advectTime += m_buffer[i].advectTime;
			result.resampleTime += m_buffer[i].resampleTime;
			result.relaxTime += m_buffer[i].relaxTime;
			result.topoTime += m_buffer[i].topoTime;
			result.deleteTime += m_buffer[i].deleteTime;
			result.splitTime += m_buffer[i].splitTime;
			result.trimTime += m_buffer[i].trimTime;
			result.extendTime += m_buffer[i].extendTime;
			result.mergeTime += m_buffer[i].mergeTime;
			result.insertTime += m_buffer[i].insertTime;
			result.numInsertions += m_buffer[i].numInsertions;
			result.numDeletions += m_buffer[i].numDeletions;
			result.numMerges += m_buffer[i].numMerges;
			result.numSplits += m_buffer[i].numSplits;
			result.numLines += m_buffer[i].numLines;
		}
		result.totalTime			/= 20.0f;
		result.renderContourTime	/= 20.0f;
		result.updateHatchTime		/= 20.0f;
		result.renderHatchTime		/= 20.0f;
		result.advectTime			/= 20.0f;
		result.resampleTime			/= 20.0f;
		result.relaxTime			/= 20.0f;
		result.topoTime				/= 20.0f;
		result.deleteTime			/= 20.0f;
		result.splitTime			/= 20.0f;
		result.trimTime				/= 20.0f;
		result.extendTime			/= 20.0f;
		result.mergeTime			/= 20.0f;
		result.insertTime			/= 20.0f;
		result.numInsertions		/= 20.0f;
		result.numDeletions			/= 20.0f;
		result.numMerges			/= 20.0f;
		result.numSplits			/= 20.0f;
		result.numLines				/= 20.0f;

		return result;
	}

	Statistics& Statistics::Get() {
		static Statistics instance;
		return instance;
	}
}