#pragma once

enum class InscribeMode
{
	SCANNING,
	DLP,
};

struct LithoParameter {
	int   size_mode; // 0:x 1:y 2:diagnal
	float size;
	float thickness;
	float object_centerx;
	float object_centery;
	float fovx;
	float fovy;

	float pixel_size;
		   
	
};

class LithoDataGenerator
{
public:

private:

public:
	
private:
	LithoParameter litho_parameter_;




};

