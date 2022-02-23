#pragma once
#include <string>

struct ConigurationParameter
{
	// scaling
	float expected_size; //this will scale max of AABB box dimensions to this value

	// infill 
	float infill_gap;
	float infill_rate;

	// slicing
	float thickness;
	float pixel_size;

	// block
	int block_size;//each sub-field(block) has a maximum pixel number.for [rotating mirror system] usually set to 1000,
};

class SlicingEngine
{
public:
	SlicingEngine();
	~SlicingEngine();

	void Create();
	void Start();

private:

	
	
private:
};

