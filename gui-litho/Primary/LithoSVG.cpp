#include <Windows.h>
#include "LithoSVG.h"

void litho::LithoSVG::LoadSVGFromStl(string stl_path, glm::vec3 bounding_box)
{
	glm::vec3 box, corner;
	GetRawSTLInfo(stl_path, box, corner);

	// calc scaling ratio
	float scaling_value = 0;
	float ratio_raw = box.y / box.x;
	float ratio_expected = bounding_box.y / bounding_box.x;

	if (ratio_raw > ratio_expected)
	{
		scaling_value = bounding_box.y / box.y;
	}
	else
	{
		scaling_value = bounding_box.x / box.x;
	}

	glm::vec3 new_box = box * scaling_value;


	// slic3r
	Slic3rCLI("../bin/stl/micro-s", "../bin/output/ooo.svg", scaling_value, new_box.z / 100.f);

	LoadSVG("../bin/output/ooo.svg");

}

void litho::LithoSVG::LoadSVGFromStl(string stl_path, float size, bool along_x,float thickness)
{
	glm::vec3 box, corner;
	GetRawSTLInfo(stl_path, box, corner);

	float scaling_value = 0;
	if (along_x)
	{
		scaling_value = size / box.x;
	}
	else
	{
		scaling_value = size / box.y;
	}
	
	Slic3rCLI(stl_path, "../bin/output/ooo.svg", scaling_value, thickness);

	LoadSVG("../bin/output/ooo.svg");
	
	
}

void litho::LithoSVG::LoadSVG(string svg_path)
{
	svg_path_ = svg_path;
	ParseXML();

	for (auto& layer : data_)
	{
		ParseLayer(layer);
	}

	center_=GetCenter();


	//print info
	if (false)
	{
		int layer_id = 0;
		for (auto& layer : data_)
		{
			std::cout << "layer: " + std::to_string(layer_id++)
				<< " z: " << layer.z_value
				<< " has " << std::to_string(layer.polygons.size()) << " polygons " << std::endl;
		}
	}
	
}

glm::vec2 litho::LithoSVG::GetUpperRight()
{
	glm::vec2 ret(-9999999, -9999999);
	for (auto& layer : data_)
	{
		for (auto& polygon : layer.polygons)
		{
			for (auto& point : polygon.points)
			{
				if (point.x > ret.x)
				{
					ret.x = point.x;
				}
				if (point.y > ret.y)
				{
					ret.y = point.y;
				}
			}
		}
	}

	return ret;
}

glm::vec2 litho::LithoSVG::GetBottomLeft()
{
	glm::vec2 ret(9999999, 9999999);
	for (auto& layer : data_)
	{
		for (auto& polygon : layer.polygons)
		{
			for (auto& point : polygon.points)
			{
				if (point.x < ret.x)
				{
					ret.x = point.x;
				}
				if (point.y < ret.y)
				{
					ret.y = point.y;
				}
			}
		}
	}

	return ret;
}

glm::vec2 litho::LithoSVG::GetCenter()
{
	glm::vec2 corner_first = GetUpperRight();
	glm::vec2 corner_second = GetBottomLeft();
	glm::vec2 ret = corner_first + corner_second;
	ret = ret / 2.f;

	return ret;
}

void litho::LithoSVG::ParseXML()
{
	using namespace std;
	std::string line;
	std::ifstream in(svg_path_);

	if (!in.good())
	{
		cout << "ERROR: svg path cannot open!" << svg_path_ << endl;
	}

	bool layer_tag = false;
	int layer_num = 0;
	Layer temp_layer;

	while (getline(in, line))
	{
		// find layer start
		if (line.find("<g") != std::string::npos) {
			layer_num++;
			layer_tag = true;
			temp_layer.layer_string += line;
		}
		else
		{
			// find layer end
			if (line.find("</g") != std::string::npos) {
				temp_layer.layer_string += line;

				data_.push_back(temp_layer);
				temp_layer.layer_string = "";
			}
			else
			{
				if (layer_tag)
				{
					temp_layer.layer_string += line;
				}
			}
		}
	}
}

void litho::LithoSVG::ParseLayer(Layer& layer)
{
	// 1. name
	if (int namePosStart = layer.layer_string.find("id=\""))
	{
		if (int namePosEnd = layer.layer_string.find("\"", namePosStart + 4))
		{
			// std::cout << layer.layer_string.substr(namePosStart+4, namePosEnd - namePosStart-4);
			layer.name = layer.layer_string.substr(namePosStart + 4, namePosEnd - namePosStart - 4);

		}
	}

	// 2.  z value
	if (int namePosStart = layer.layer_string.find("z=\""))
	{
		if (int namePosEnd = layer.layer_string.find("\"", namePosStart + 3))
		{
			layer.z_value = std::stof(layer.layer_string.substr(namePosStart + 3, namePosEnd - namePosStart - 3));
		}
	}

	//  3. polygons 
	{
		vector<size_t> startPos;
		vector<size_t> endPos;

		string toSearch = "<polygon";
		size_t pos = layer.layer_string.find(toSearch);

		while (pos != std::string::npos)
		{
			// Add position to the vector
			startPos.push_back(pos + toSearch.size());

			size_t ended = layer.layer_string.find("/>", pos + toSearch.size());
			if (ended != string::npos)
			{
				endPos.push_back(ended);
			}

			// Get the next occurrence from the current position
			pos = layer.layer_string.find(toSearch, pos + toSearch.size());
		}

		// parsing each polyogn to fill polygons with data 
		float _left = 9999999;
		float _right = -9999999;
		float _bottom = 9999999;
		float _top = -9999999;
		for (size_t polygonId = 0; polygonId < startPos.size(); polygonId++)
		{
			Polygon temp_polygon;

			temp_polygon.polygon_string = layer.layer_string.substr(startPos[polygonId], endPos[polygonId] - startPos[polygonId]);

			ParsePolygon(temp_polygon);

			if (temp_polygon.left < _left)     { _left   = temp_polygon.left; }
			if (temp_polygon.right > _right)   { _right  = temp_polygon.right; }
			if (temp_polygon.bottom < _bottom) { _bottom = temp_polygon.bottom; }
			if (temp_polygon.top > _top)       { _top    = temp_polygon.top ; }

			layer.polygons.push_back(temp_polygon);
		}

		layer.left = _left;
		layer.right = _right;
		layer.bottom = _bottom;
		layer.top = _top;

	}

}

void litho::LithoSVG::ParsePolygon(Polygon& polygon)
{
	// 1. points
	{
		string toSearch = "points=\"";
		size_t pointsPos = polygon.polygon_string.find(toSearch);

		if (pointsPos != std::string::npos)
		{
			string endToSearch = "\"";
			size_t endedPos = polygon.polygon_string.find(endToSearch, pointsPos + toSearch.size());
			if (endedPos != std::string::npos)
			{
				string pointsString = polygon.polygon_string.substr(pointsPos + toSearch.size(), endedPos - (pointsPos + toSearch.size()));

				// parse points string 
				{
					glm::vec2 point;
					size_t lastWhiteSpcaePos = 0;
					size_t whiteSpacePos = 0;

					string toSearch = " ";
					whiteSpacePos = pointsString.find(toSearch);

					polygon.points.clear();

					while (whiteSpacePos != std::string::npos)
					{
						string aPointString = pointsString.substr(lastWhiteSpcaePos, whiteSpacePos - lastWhiteSpcaePos);
						lastWhiteSpcaePos = whiteSpacePos + toSearch.size();
						whiteSpacePos = pointsString.find(toSearch, lastWhiteSpcaePos);

						point = glm::vec2(0);
						size_t commaPos = aPointString.find(",");
						point.x = std::stof(aPointString.substr(0, commaPos));
						point.y = std::stof(aPointString.substr(commaPos + 1));

						polygon.points.push_back(point);
					}

					// get bounding box of this polygon
					float _left = 9999999;
					float _right = -9999999;
					float _bottom = 9999999;
					float _top = -9999999;
					for (auto& point:polygon.points)
					{
						if (point.x < _left) { _left = point.x; }
						if (point.x > _right) { _right = point.x; }
						if (point.y < _bottom) { _bottom = point.y; }
						if (point.y > _top) { _top = point.y; }
					}

					polygon.left = _left;
					polygon.right = _right;
					polygon.bottom = _bottom;
					polygon.top = _top;

				}

			}
		}
	}


	// 2. type
	{
		string toSearch = "type=\"";
		size_t pointsPos = polygon.polygon_string.find(toSearch);

		if (pointsPos != std::string::npos)
		{
			string endToSearch = "\"";
			size_t endedPos = polygon.polygon_string.find(endToSearch, pointsPos + toSearch.size());
			if (endedPos != std::string::npos)
			{
				string type_string = polygon.polygon_string.substr(pointsPos + toSearch.size(), endedPos - (pointsPos + toSearch.size()));
				
				polygon.type = type_string;
				if (type_string=="contour")
				{
					polygon.is_hole = false;
				}
				if (type_string == "hole")
				{
					polygon.is_hole = true;

				}

			}
		}
	}
}

void litho::LithoSVG::GetRawSTLInfo(std::string stl_path, glm::vec3& size, glm::vec3& corner)
{
	stl_reader::StlMesh <float, unsigned int> mesh(stl_path);

	glm::vec3 bbox_corner_lower = glm::vec3(9999999, 9999999, 9999999);
	glm::vec3 bbox_corner_upper = glm::vec3(-999999, -999999, -999999);

	for (size_t itri = 0; itri < mesh.num_tris(); ++itri) {
		//std::cout << "coordinates of triangle " << itri << ": ";
		for (size_t icorner = 0; icorner < 3; ++icorner) {
			const float* c = mesh.tri_corner_coords(itri, icorner);
			// or alternatively:
			// float* c = mesh.vrt_coords (mesh.tri_corner_ind (itri, icorner));
			//std::cout << "(" << c[0] << ", " << c[1] << ", " << c[2] << ") ";

			glm::vec3 point = glm::vec3(c[0], c[1], c[2]);


			if (point.x < bbox_corner_lower.x)
			{
				bbox_corner_lower.x = point.x;
			}
			if (point.y < bbox_corner_lower.y)
			{
				bbox_corner_lower.y = point.y;
			}
			if (point.z < bbox_corner_lower.z)
			{
				bbox_corner_lower.z = point.z;
			}

			if (point.x > bbox_corner_upper.x)
			{
				bbox_corner_upper.x = point.x;
			}
			if (point.y > bbox_corner_upper.y)
			{
				bbox_corner_upper.y = point.y;
			}
			if (point.z > bbox_corner_upper.z)
			{
				bbox_corner_upper.z = point.z;
			}
		}

	}

	std::cout
		<< "lower " << bbox_corner_lower.x << " " << bbox_corner_lower.y << " " << bbox_corner_lower.z << " " << std::endl
		<< "upper " << bbox_corner_upper.x << " " << bbox_corner_upper.y << " " << bbox_corner_upper.z << " " << std::endl;

	size = bbox_corner_upper - bbox_corner_lower;

	corner = bbox_corner_lower;
}

void litho::LithoSVG::GetEachLayerBoundingBox()
{
	for (auto& layer : data_)
	{
		float left = 9999999;
		float right = -9999999;
		float bottom = 9999999;
		float top = -9999999;

		for (auto& polygon : layer.polygons)
		{
			for (auto& point : polygon.points)
			{
				if (point.x < left)
				{
					left = point.x;
				}
				if (point.x > right)
				{
					right = point.x;
				}
				if (point.y < bottom)
				{
					bottom = point.y;
				}
				if (point.y > top)
				{
					top = point.y;
				}
			}
		}

		layer.left = left;
		layer.right = right;
		layer.bottom = bottom;
		layer.top = top;
	}
}

void litho::LithoSVG::Slic3rCLI(string input_path, string output_path, float scale, float thickness)
{
	std::string cmd_args = "Slic3r-console.exe";
	cmd_args += " --export-svg";
	cmd_args += " " + input_path;
	cmd_args += " --scale " + std::to_string(scale);
	cmd_args += " --layer-height " + std::to_string(thickness);
	cmd_args += " -o " + output_path;


	PROCESS_INFORMATION pi;

	STARTUPINFO si;
	ZeroMemory(&si, sizeof(STARTUPINFO));
	si.cb = sizeof(STARTUPINFO);

	if (CreateProcess(
		"../bin/Slic3r/Slic3r-console.exe",
		&cmd_args[0],
		NULL,
		NULL,
		TRUE,
		0,
		NULL,
		NULL,
		&si, &pi))
	{
		WaitForSingleObject(pi.hProcess, INFINITE);
		CloseHandle(pi.hThread);
		CloseHandle(pi.hProcess);
		std::cout << "slic3r done! " << std::endl;
	}
	else
	{
		std::cout << "ERROR: CLI failed!" << std::endl;
	}


}

