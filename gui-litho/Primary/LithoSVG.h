#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>

#define STL_READER_NO_EXCEPTIONS
#include "stl_reader.h"

#include <string>
#include <string_view>
#include <fstream>
#include <vector>
#include <sstream>
#include <iomanip>
#include <iostream>

namespace litho
{
	using namespace std;

	struct Polygon 
	{
		string type;
		string style;
		vector<glm::vec2> points;
		
		string polygon_string;
	};

	struct Layer
	{
		string name;
		float z_value;
		vector<Polygon> polygons;
		string layer_string;
		float left;
		float right;
		float top;
		float bottom;

		float left_aligned;
		float right_aligned;
		float top_aligned;
		float bottom_aligned;

		int pixel_num_x;
		int pixel_num_y;
		
	};

	class LithoSVG
	{

	public:
		
		void LoadSVGFromStl(string stl_path,glm::vec3 bounding_box);
		void LoadSVG(string svg_path);

		glm::vec2 GetUpperRight();
		glm::vec2 GetBottomLeft();

		glm::vec2 GetCenter();

		vector<Layer> data_;
		glm::vec2 center_;
		glm::vec2 corner_bottom_left_;
		glm::vec2 corner_upper_right_;


	private:
		void ParseXML();
		void ParseLayer(Layer& layer);
		void ParsePolygon(Polygon& polygon);
		
		void GetRawSTLInfo(std::string stl_path, glm::vec3& size, glm::vec3& corner);

		void GetEachLayerBoundingBox();

		void Slic3rCLI(string input_path, string output_path, float scale, float thickness);
		
		string svg_path_;
		string stl_path_;


	};
}


