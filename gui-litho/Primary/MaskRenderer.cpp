#include "MaskRenderer.h"

void MaskRenderer::Init(glm::vec2 anchor, float pixel_size)
{
	anchor_ = anchor;
	pixel_size_ = pixel_size;
	Init();
}

void MaskRenderer::Init()
{
	UpdateVAO();
	
}
