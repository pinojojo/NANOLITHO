#include "LithoRasterizer.h"

LithoRasterizer::LithoRasterizer()
{
}

LithoRasterizer::~LithoRasterizer()
{
}

void LithoRasterizer::Init(litho::LithoSetting& setting)
{
	//stroke_.Init(setting);
	mask_.Init(setting);

}

GLuint LithoRasterizer::Raster(float left, float right, float bottom, float top, int rows, int cols)
{
	
	//tex_=stroke_.Raster(left, right, bottom, top, rows, cols);

	tex_ = mask_.Raster(left, right, bottom, top, rows, cols);

	return tex_;
}

void LithoRasterizer::UpdateData(litho::LithoSVG& svg, int layer_id)
{
	// update renderer's layer data 
	//stroke_.UpdateLayer(svg, layer_id);
	mask_.UpdateLayer(svg, layer_id);
}

