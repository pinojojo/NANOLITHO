#include "LithoExporter.h"





void litho::LithoExporter::FindPixelAlignedBoundingBox(float pixel_size)
{

	for (auto& layer : svg_.data_)
	{
		auto left = layer.left;
		auto top = layer.top;
		auto right = layer.top;
		auto bottom = layer.bottom;

		glm::vec2 center = svg_.center_;

		int pixel_number_x = ceil((right - left) / pixel_size);
		int pixel_number_y = ceil((top - bottom) / pixel_size);
		if (pixel_number_x % 2 != 0) { pixel_number_x += 1; }
		if (pixel_number_y % 2 != 0) { pixel_number_y += 1; }

		float left_to_center = center.x - left;
		float top_to_center = top - center.y;
		float right_to_center = right - center.x;
		float bottom_to_center = center.y - bottom;

		layer.left_aligned = center.x - (pixel_number_x / 2) * pixel_size;
		layer.top_aligned = center.y + (pixel_number_y / 2) * pixel_size;
		layer.right_aligned = center.x + (pixel_number_y / 2) * pixel_size;
		layer.bottom_aligned = center.y - (pixel_number_y / 2) * pixel_size;


		// effective pixels
		layer.pixel_num_x = pixel_number_x;
		layer.pixel_num_y = pixel_number_y;


	}

}

litho::LithoExporter::LithoExporter(LithoSetting& setting)
{
	setting_ = setting;
	setting_.GetInternal();
}

void litho::LithoExporter::ConvertToXML()
{
	// 1. svg import
	svg_.LoadSVGFromStl(setting_.stl_path, setting_.size_internal, setting_.along_x, setting_.thickness_internal);

	// 2. layouts
	GenerateAdaptiveLayers();

}



void litho::LithoExporter::GenerateAdaptiveLayers()
{
	adaptive_layers_.clear();

	// calculate pixel aligned box for each layer
	FindPixelAlignedBoundingBox(setting_.pixel_size_internal);

	int layer_id = 0;

	for (auto& layer : svg_.data_)
	{

		// matrix in different layer may have different size and position, we need to find a minimal size box to render
		AdaptiveLayer curr_layer;
		curr_layer.id = layer_id++;
		curr_layer.position = glm::vec2(layer.left_aligned, layer.top_aligned); // better use aligned anchor 
		curr_layer.cols = layer.pixel_num_x;
		curr_layer.rows = layer.pixel_num_y;
		curr_layer.hatch_distance = 100;

		GenerateStrips(curr_layer);

		adaptive_layers_.push_back(curr_layer);
	}


	// preview
	bool preview = true;
	if (preview)
	{
		for (auto& layer : adaptive_layers_)
		{   
			std::cout << std::endl;
			std::cout << "layer: " << layer.id <<" pos: " << layer.position.x<< "," << layer.position.y << " row: "<<layer.rows<<" col: "<<layer.cols << std::endl;
			for (auto& strip:layer.strips)
			{
				std::cout << ">strip: " << strip.absolute_id << " has blocks " << strip.blocks.size()<<std::endl;
				for (auto& block:strip.blocks)
				{
					std::cout << ">>block position: " << block.anchor.x << " , " << block.anchor.y << std::endl;
				}
			}
		}

	}

}

void litho::LithoExporter::GenerateStrips(AdaptiveLayer& adaptive_layer)
{
	int strip_num = ceil((float)adaptive_layer.cols / (float)setting_.strip_width);

	for (int strip_id = 0; strip_id < strip_num; strip_id++)
	{
		Strip curr_strip;

		int abs_strip_id = 0;
		for (size_t layer_id = 0; layer_id < adaptive_layer.id; layer_id++)
		{
			abs_strip_id += adaptive_layers_[layer_id].strips.size();
		}

		// data matrix id
		curr_strip.absolute_id = abs_strip_id + strip_id;
		curr_strip.relative_id = strip_id;
		curr_strip.pixel_size = setting_.pixel_size_internal;
		curr_strip.anchor = adaptive_layer.position + glm::vec2(strip_id * setting_.strip_width * setting_.pixel_size_internal, 0);
		curr_strip.width = setting_.strip_width;
		curr_strip.height = adaptive_layer.rows;

		

		GenerateBlock(curr_strip);

		adaptive_layer.strips.push_back(curr_strip);
	}
}

void litho::LithoExporter::GenerateBlock(Strip& strip)
{
	int block_num = ceil((float)strip.height / (float)setting_.block_height);

	for (size_t block_id = 0; block_id < block_num; block_id++)
	{
		Block curr_block;

		curr_block.pixel_size = setting_.pixel_size_internal;
		curr_block.width = strip.width;
		curr_block.height = setting_.block_height;
		curr_block.anchor = strip.anchor + glm::vec2(0, -(float)block_id * setting_.block_height * setting_.pixel_size_internal);

		strip.blocks.push_back(curr_block);
	}

}
