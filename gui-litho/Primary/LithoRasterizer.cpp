#include "LithoRasterizer.h"

LithoRasterizer::LithoRasterizer()
{
}

LithoRasterizer::~LithoRasterizer()
{
}

void LithoRasterizer::Init(litho::LithoSetting& setting)
{
	stroke_.Init(setting);
	mask_.Init(setting);
	infill_.Init(setting);
	blend_.Init();
}

GLuint LithoRasterizer::Raster(float left, float right, float bottom, float top, int rows, int cols)
{
	
	GLuint tex_stroke=stroke_.Raster(left, right, bottom, top, rows, cols);

	GLuint tex_mask = mask_.Raster(left, right, bottom, top, rows, cols);

	GLuint tex_infill = infill_.Raster(left, right, bottom, top, rows, cols);

	tex_ = blend_.Blend(tex_infill, tex_stroke, tex_mask, cols, rows);

	return tex_;
}

void LithoRasterizer::UpdateData(litho::LithoSVG& svg, int layer_id)
{
	// update renderer's layer data 
	stroke_.UpdateLayer(svg, layer_id);
	mask_.UpdateLayer(svg, layer_id);
}

