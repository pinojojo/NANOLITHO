#pragma once
#include "LithoType.h"

#include "InfillRenderer.h"
#include "MaskRenderer.h"
#include "StrokeRenderer.h"
#include "BlendRenderer.h"

class LithoRasterizer
{
public:

	LithoRasterizer();
	~LithoRasterizer();

	void Init(litho::LithoSetting& setting);
	GLuint Raster(float left,float right,float bottom,float top, int rows,int cols);
	void UpdateData(litho::LithoSVG& svg, int layer_id);
	
	
	
private:



	

	
	GLuint  tex_=0;
	GLuint  fbo_=0;
	Shader* shader_=NULL;

	
	InfillRenderer infill_;
	StrokeRenderer stroke_;
	MaskRenderer mask_;
	BlendRenderer blend_;


};

