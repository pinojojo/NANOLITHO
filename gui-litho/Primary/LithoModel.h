#pragma once
#include <iostream>
#include <string>
#include <string_view>
#include <fstream>
#include <vector>
#include <sstream>
#include <iomanip>
#include "LPerformance.h"

#include "slicer/SnowSlicer.h"

using namespace std;

struct LPoint
{
	float x;
	float y;

	void clear() 
	{
		x = 0;
		y = 0;
	}
};


struct SVGPolygon
{
	string type;
	string style;

	vector<LPoint> points;

};

struct LithoLayer 
{
	string name;
	string z_value;
	string layer_string;

	vector<SVGPolygon> polygons;

	void GetBoundingBox(float& left, float& right, float& bottom, float& top) 
	{
		float _left = 9999999;
		float _right = -9999999;
		float _top = -9999999;
		float _bottom = 9999999;
		
		for (auto& polygon:polygons)
		{
			for (auto& point : polygon.points)
			{
				if (point.x<_left)
				{
					_left = point.x;
				}
				if (point.x>_right)
				{
					_right = point.x;
				}
				if (point.y>_top)
				{
					_top = point.y;
				}
				if (point.y<_bottom)
				{
					_bottom = point.x;
				}
			}
		}

		left = _left;
		right = _right;
		bottom = _bottom;
		top = _top;
	}
};

class LithoModel
{
public:

	LithoModel();

	void SlicingSTL(std::string stl_path, float relative_thickness);



	void DoParse(std::string filePath);

	void Slicing();

	


	// Getter
	glm::vec2 GetMinMax();

	// Setter
	void SetThickness(float thickness);
	void SetExpectedSize(float expected_size);
	
	
	
	
	
	
	SnowSlicer::Slicer m_Slicer;
	std::vector<LithoLayer> m_Layers;
private:// stage function

	int  CheckFileType(std::string path);
	void LoadStl(std::string filePath);
	void SliceModelToLithoLayers();
	void ParseSVGToString();
	void ProcessLayer(LithoLayer& layer);
	void ProcessLayerPolygons(LithoLayer& layer);// may take long time 
	void ProcessPolygonString(string& polygonString,SVGPolygon& polygon);
	void ProcessPointsString(string& pointsString,SVGPolygon& polygon);

private:// tool function
	void ConvertToLithoLayer(vector<vector<SnowSlicer::contour>>& layers);


private://data
	std::string m_SVGPath;
	LPerformance m_Performance;
	
	SnowSlicer::SlicingParameter m_SlicerPara;
	
	float m_ExpectedSize = 1.0f;

	double m_Zmin;
	double m_Zmax;
	double m_Xmin;
	double m_Ymin;
	double m_Xmax;
	double m_Ymax;
	
	
	
	
	
};

