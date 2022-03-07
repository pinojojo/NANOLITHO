#pragma once

#define GLM_FORCE_RADIANS
#include <GL/glew.h>
#include <gl/GL.h>

#include "LithoType.h"
#include "LithoSVG.h"
#include "LithoRasterizer.h"


#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <vector>


struct Block
{
	glm::vec2 anchor;
	int width;
	int height;
	float pixel_size;


};

struct Strip
{
	int absolute_id;
	int relative_id;
	glm::vec2 anchor;
	std::vector<Block> blocks;
	int width;
	int height;
	float pixel_size;
	float hatch_distance;
	
};

struct AdaptiveLayer
{
	int id;
	int cols;
	int rows;
	std::vector<Strip> strips;
	glm::vec2 position;
	float hatch_distance;

	
};


namespace litho
{

	class LithoExporter
	{

	public:
		LithoExporter(LithoSetting& setting);

		void ConvertToXML();

		void ConvertToPNG(int layer_id);

		
		litho::LithoSVG svg_;
	private:
	

		void GenerateAdaptiveLayers();
		void GenerateStrips(AdaptiveLayer& adaptive_layer);
		void GenerateBlock(Strip& strip);

		void RasterizeAdaptiveLayer(AdaptiveLayer& adaptive_layer);
		void RasterizeStrip(Strip& strip);
		void RasterizeBlock(Block& block,FILE* file);

		void FindPixelAlignedBoundingBox(float pixel_size);

		void SaveTexture2XML(GLuint tex,FILE* xml);
		
		void SaveTexture2PNG(GLuint tex, std::string png_path);
		
		

		
		LithoRasterizer rasterizer_;
		

		std::vector<AdaptiveLayer> adaptive_layers_;
		LithoSetting setting_;



	};


}
