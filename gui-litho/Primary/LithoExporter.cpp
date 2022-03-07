#include "LithoExporter.h"


#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>




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

void litho::LithoExporter::SaveTexture2XML(GLuint tex, FILE* xml)
{
	glBindTexture(GL_TEXTURE_2D, tex);

	int width, height = 0;
	int channels = 1;
	int miplevel = 0;
	glGetTexLevelParameteriv(GL_TEXTURE_2D, miplevel, GL_TEXTURE_WIDTH, &width);
	glGetTexLevelParameteriv(GL_TEXTURE_2D, miplevel, GL_TEXTURE_HEIGHT, &height);

	std::vector<GLubyte> pixels(channels * width * height);
	glGetTextureImage(tex, 0, GL_RED, GL_UNSIGNED_BYTE, height * width * channels, pixels.data());

	// vertical flipping
	for (int line = 0; line != height / 2; ++line) {
		std::swap_ranges(pixels.begin() + channels * width * line,
			pixels.begin() + channels * width * (line + 1),
			pixels.begin() + channels * width * (height - line - 1));
	}

	// write to xml
	for (auto& p:pixels)
	{
		if (p > 0) 
		{
			fprintf(xml, "f");
		}
		else
		{
			fprintf(xml, "0");
		}
	}


	glBindTexture(GL_TEXTURE_2D, 0);

}

void litho::LithoExporter::SaveTexture2PNG(GLuint tex, std::string png_path)
{
	glBindTexture(GL_TEXTURE_2D,tex);

	int width, height = 0;
	int channels = 1;
	int miplevel = 0;
	glGetTexLevelParameteriv(GL_TEXTURE_2D, miplevel, GL_TEXTURE_WIDTH, &width);
	glGetTexLevelParameteriv(GL_TEXTURE_2D, miplevel, GL_TEXTURE_HEIGHT, &height);

	std::vector<GLubyte> pixels(channels * width * height);
	glGetTextureImage(tex, 0, GL_RED, GL_UNSIGNED_BYTE, height * width * channels, pixels.data());

	// vertical flipping
	for (int line = 0; line != height / 2; ++line) {
		std::swap_ranges(pixels.begin() + channels * width * line,
			pixels.begin() + channels * width * (line + 1),
			pixels.begin() + channels * width * (height - line - 1));
	}

	stbi_write_png(png_path.c_str(), width, height, channels, pixels.data(), width * channels);
	
	glBindTexture(GL_TEXTURE_2D, 0);


}

litho::LithoExporter::LithoExporter(LithoSetting& setting)
{
	// setting
	setting_ = setting;
	setting_.GetInternal();
}

void litho::LithoExporter::ConvertToXML()
{
	// 1. svg import
	svg_.LoadSVGFromStl(setting_.stl_path, setting_.size_internal, setting_.along_x, setting_.thickness_internal);

	// 2. layouts generating
	GenerateAdaptiveLayers();

	// 3. rasterizer
	rasterizer_.Init(setting_);
	for (auto& layer:adaptive_layers_)
	{
		RasterizeAdaptiveLayer(layer);

		std::cout << "layer raster: " << layer.id << std::endl;
	}
	
	
	
	
}

void litho::LithoExporter::ConvertToPNG(int layer_id)
{
	// 1. svg
	svg_.LoadSVGFromStl(setting_.stl_path, setting_.size_internal, setting_.along_x, setting_.thickness_internal);

	// 2. layouts
	GenerateAdaptiveLayers();

	// 3. raster
	rasterizer_.Init(setting_);
	rasterizer_.UpdateData(svg_, layer_id);

	auto& adaptive_layer = adaptive_layers_[layer_id];
	
	if (adaptive_layer.strips.size())
	{
		for (auto& strip : adaptive_layer.strips)
		{
			for (auto& block:strip.blocks)
			{
				float left = block.anchor.x;
				float right = block.anchor.x+block.width*block.pixel_size;
				float bottom = block.anchor.y-block.height*block.pixel_size;
				float top = block.anchor.y;
				
				GLuint tex_id=rasterizer_.Raster(left, right, bottom, top, block.width, block.height);
			
				std::string file_path = "../bin/output/png/";
				file_path += "-layer-" + std::to_string(adaptive_layer.id)+
					"-strip-" + std::to_string(strip.absolute_id) +
					"-block-" + std::to_string(block.anchor.x) + "," + std::to_string(block.anchor.y)+
					".png";

				SaveTexture2PNG(tex_id, file_path);
			}
		}
	}
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
	bool preview = false;
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

void litho::LithoExporter::RasterizeAdaptiveLayer(AdaptiveLayer& adaptive_layer)
{
	// update layer data
	rasterizer_.UpdateData(svg_, adaptive_layer.id);

	for  (auto& strip:adaptive_layer.strips)
	{
		RasterizeStrip(strip);
	}

}

void litho::LithoExporter::RasterizeStrip(Strip& strip)
{
	std::string save_path = setting_.xml_path + std::to_string(strip.absolute_id) + ".xml";
	
	FILE* xml_file = NULL;

	xml_file = fopen(save_path.c_str(), "w");
	if (!xml_file)
	{
		std::cout << "ERROR: xml path creating failed ! " << std::endl;
	}
	else
	{
		fprintf(xml_file, "<Data>\n");
		std::string matrix_id_string = " <data Matrix_ID=\"" + std::to_string(strip.absolute_id) + "\" Matrix=\"";
		fprintf(xml_file, matrix_id_string.c_str());

		int strip_rows = 0;
		for (auto& block:strip.blocks)
		{
			RasterizeBlock(block, xml_file);
			strip_rows += block.height;
		}

		std::string data_tail = "\"";
		data_tail += " Row=";
		data_tail += "\"" + std::to_string(strip_rows) + "\"";
		data_tail += " Col=";
		data_tail += "\"" + std::to_string(strip.width) + "\"";
		data_tail += " />";
		data_tail += "\n";
		fprintf(xml_file, data_tail.c_str());

		fprintf(xml_file, "</Data>\n");
		fclose(xml_file);
	}
}

void litho::LithoExporter::RasterizeBlock(Block& block, FILE* file)
{
	if (file!=NULL)
	{
		float left = block.anchor.x;
		float right = block.anchor.x + block.width * block.pixel_size;
		float bottom = block.anchor.y - block.width * block.pixel_size;
		float top = block.anchor.y;

		GLuint tex_id = rasterizer_.Raster(left, right, bottom, top, block.height, block.width);

		SaveTexture2XML(tex_id, file);

	}
}
