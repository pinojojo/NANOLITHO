#pragma once

#define GLM_FORCE_RADIANS
#include <GL/glew.h>
#include <gl/GL.h>

#include "LithoType.h"
#include "LithoSVG.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <vector>


struct Block
{
	glm::vec2 anchor;
	glm::vec2 size;
	int res_x;
	int res_y;
};

struct Strip
{
	glm::vec2 anchor;
	glm::vec2 size;
	std::vector<Block> blocks;
	int width;
};

struct DataMatrix
{
	int ID;
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

	private:
		void GenerateDataMatrices();
		void GenerateStrips(DataMatrix& data_matrix);
		void GenerateBlock(Strip& strip);


		void WriteMatrix(FILE* file, DataMatrix data_matrix);

		void WritePixelData(FILE* file, GLuint tex);




		GLuint RasterizeSingleBlock(Block block);

		void GetLayerBoundingbox(glm::vec2& anchor, glm::vec2& box, int layer_id);

		void FindPixelAlignedBoundingBox(float pixel_size);

		
		

		litho::LithoSVG svg_;
		float relative_pixel_size_;
		std::vector<DataMatrix> data_matrices_;

	};


}
