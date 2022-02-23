#pragma once

#include <chrono>

class LPerformance
{
public: 

	void Start();

	float GetDuration();
	float GetFPS();


private:

	std::chrono::high_resolution_clock::time_point m_Timepoint;
		
};

