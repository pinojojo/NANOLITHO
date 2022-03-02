#pragma once
#include <Windows.h>

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
	};

	class LithoSVG
	{

	public:
		
		void LoadSVGFromStl(string stl_path,glm::vec3 bounding_box);
		void LoadSVG(string svg_path);

		glm::vec2 GetUpperRight();
		glm::vec2 GetBottomLeft();

		
	private:
		void ParseXML();
		void ParseLayer(Layer& layer);
		void ParsePolygon(Polygon& polygon);
		
		void GetRawSTLInfo(std::string stl_path, glm::vec3& size, glm::vec3& corner);

		void Slic3rCLI(string input_path, string output_path, float scale, float thickness);
		

		vector<Layer> data_;
		
		string svg_path_;
		string stl_path_;


	};
}

