#include "LithoRasterizer.h"

LithoRasterizer::LithoRasterizer()
{
}

LithoRasterizer::~LithoRasterizer()
{
}

void LithoRasterizer::Init(litho::LithoSetting& setting)
{
}

GLuint LithoRasterizer::Raster(float left, float right, float bottom, float top, int rows, int cols)
{
	
	return GLuint();
}

void LithoRasterizer::UpdateData(litho::LithoSVG& svg, int layer_id)
{
	// update renderer's layer data 
	stroke_.UpdateLayer(svg, layer_id);
}

void LithoRasterizer::InitGL()
{
}

