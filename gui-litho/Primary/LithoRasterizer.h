#pragma once
#include "LithoType.h"

#include "InfillRenderer.h"
#include "MaskRenderer.h"
#include "StrokeRenderer.h"

class LithoRasterizer
{
public:

	LithoRasterizer();
	~LithoRasterizer();

	void Init(litho::LithoSetting& setting);
	GLuint Raster(float left,float right,float bottom,float top, int rows,int cols);
	void UpdateData(litho::LithoSVG& svg, int layer_id);
	
	
	
private:

	void InitGL();

	

	
	GLuint  tex_;
	GLuint  fbo_;
	Shader* shader_;

	

	StrokeRenderer stroke_;



};

