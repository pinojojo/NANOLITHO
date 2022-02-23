#include "LPerformance.h"

void LPerformance::Start()
{
	m_Timepoint = std::chrono::high_resolution_clock::now();
}

float LPerformance::GetDuration()
{
	auto currTp = std::chrono::high_resolution_clock::now();
	std::chrono::duration<float> duration_till_start_raw = currTp - m_Timepoint;
	std::chrono::milliseconds duration_till_start_ms = std::chrono::duration_cast<std::chrono::milliseconds>(duration_till_start_raw);

	return duration_till_start_ms.count();
}
