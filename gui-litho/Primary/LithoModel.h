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
	
};

class LithoModel
{
public:

	LithoModel();

	void DoParse(std::string filePath);

	// Getter
	glm::vec2 GetMinMax();
	
	
	
	

	std::vector<LithoLayer> m_Layers;
private:// stage function

	int  CheckFileType(std::string path);
	void LoadStl(std::string filePath);
	void SliceModelToLithoLayers(SnowSlicer::SlicingParameter& parameter);
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
	SnowSlicer::Slicer m_Slicer;
	SnowSlicer::SlicingParameter m_SlicerPara;
	

	double m_zmin;
	double m_zmax;
	
	
	
	
	
};

