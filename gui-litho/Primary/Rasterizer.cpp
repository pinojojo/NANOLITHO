#include "Rasterizer.h"

void Rasterizer::UpdateData(litho::LithoSVG& svg, int layer_id)
{
	stroke_.UpdateLayer(svg, layer_id);
}
