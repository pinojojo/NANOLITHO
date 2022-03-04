#pragma once
#include "LithoType.h"
#include "LithoExporter.h"

#include "InfillRenderer.h"
#include "MaskRenderer.h"
#include "StrokeRenderer.h"

class Rasterizer
{
public:

	Rasterizer();
	~Rasterizer();

	void Init(litho::LithoSetting& setting);
	void Raster(glm::vec2 upper_left,glm::vec2 size, int rows,int cols);
	void UpdateData(litho::LithoSVG& svg, int layer_id);
private:

	void InitGL();

	GLuint  tex_;
	GLuint  fbo_;
	Shader* shader_;
	GLuint  fbo_;
	

	StrokeRenderer stroke_;



};

