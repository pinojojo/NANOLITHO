#include "LithoExporter.h"





void litho::LithoExporter::FindPixelAlignedBoundingBox(float pixel_size)
{

	for (auto& layer:svg_.data_)
	{
		auto left = layer.left;
		auto top = layer.top;
		auto right = layer.top;
		auto bottom = layer.top;

		glm::vec2 center = svg_.center_;

		int pixel_number_x = ceil((right - left) / pixel_size);
		int pixel_number_y = ceil((top - bottom) / pixel_size);
		if (pixel_number_x % 2 != 0) { pixel_number_x += 1; }
		if (pixel_number_y % 2 != 0) { pixel_number_y += 1; }
		
		float left_to_center = center.x - left;
		float top_to_center = top - center.y;
		float right_to_center = right - center.x;
		float bottom_to_center = center.y-bottom;

		layer.left_aligned   = center.x - (pixel_number_x / 2) * pixel_size;
		layer.top_aligned    = center.y + (pixel_number_y / 2) * pixel_size;
		layer.right_aligned  = center.x + (pixel_number_y / 2) * pixel_size;
		layer.bottom_aligned = center.y - (pixel_number_y / 2) * pixel_size;


		// effective pixels
		layer.pixel_num_x = pixel_number_x;
		layer.pixel_num_y = pixel_number_y;

		
	}
	
}

void litho::LithoExporter::GenerateDataMatrices()
{
	FindPixelAlignedBoundingBox(relative_pixel_size_);

	int layer_id = 0;
	for (auto& layer:svg_.data_ )
	{
		DataMatrix matrix;
		matrix.ID = layer_id++;
		matrix.position = glm::vec2(layer.left_aligned, layer.top_aligned);
		matrix.cols = layer.pixel_num_x;
		matrix.rows = layer.pixel_num_y;
		matrix.hatch_distance = 100;
		
		GenerateStrips(matrix);

		data_matrices_.push_back(matrix);
	}
	
}

void litho::LithoExporter::GenerateStrips(DataMatrix& data_matrix)
{
	int strip_num = ceil(data_matrix.cols / 1000.f);
	

}
