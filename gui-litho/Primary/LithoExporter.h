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


struct SubWindow
{
	int res_x;
	int rex_y;
	glm::vec2 anchor;
	glm::vec2 size;
};

struct DataMatrix
{
	int ID;
	int cols;
	int rows;
	std::vector<SubWindow> windows;
	glm::vec2 position;
	float hatch_distance;

	
};


namespace litho
{

	class LithoExporter
	{

	public:
		LithoExporter(LithoSetting& setting);


		void Slice();



	private:

		void WriteMatrix(FILE* file, DataMatrix data_matrix);

		void WritePixelData(FILE* file, GLuint tex);

		void GenerateSubWindows();

		GLuint RasterizeSingleSubWindow(SubWindow sub_window);

		void GetLayerBoundingbox(glm::vec2& anchor, glm::vec2& box, int layer_id);




		litho::LithoSVG svg_;

		std::vector<DataMatrix> data_matrices_;

	};


}
