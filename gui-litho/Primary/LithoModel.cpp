#include "LithoModel.h"

void LithoModel::DoParse(std::string filePath)
{
    m_Layers.clear();
    m_Performance.Start();
    int fileType = CheckFileType(filePath);
    switch (fileType)
    {
    case 0:
        break;
    case 1:
    {
        SnowSlicer::SlicingParameter para;
        para.ModelPath = filePath;
        SliceModelToLithoLayers(para);
        cout << "parse stl finished in: " << m_Performance.GetDuration() << " ms." << endl;
        break;
       
    }
        
    case 2:
    {
        m_SVGPath = filePath;
        // Get All layers
        ParseSVGToString();
        // Process Each Layer
        for (auto& layer : m_Layers)
        {
            ProcessLayer(layer);
        }
        cout << "parse svg finished in: " << m_Performance.GetDuration() << " ms." << endl;
        break;
    }
    }

    
    
}

int LithoModel::CheckFileType(std::string path)
{
    std::string substrSvg = ".svg";
    std::string substrStl = ".stl";

    if (path.find(substrStl)!=std::string::npos)
    {
        return 1;
    }
    if (path.find(substrSvg) != std::string::npos)
    {
        return 2;
    }
    return 0;
}

void LithoModel::SliceModelToLithoLayers(SnowSlicer::SlicingParameter& parameter)
{
    SnowSlicer::Slicer slicer(parameter);

    slicer.DoSlice();

    ConvertToLithoLayer(slicer.m_Polygons);


}

void LithoModel::ParseSVGToString()
{
    std::string line;
    std::ifstream in(m_SVGPath);

    if (!in.good())
    {
        cout << "file open failed: " << m_SVGPath << endl;
    }
    
    bool layer_tag = false;
    int layer_num = 0;

    LithoLayer tmp_layer;

    while (getline(in, line)) 
    {
        std::string tmp; 

       
        
		// find layer start
		if (line.find("<g") != std::string::npos) {
			layer_num++;
            layer_tag = true;
            tmp_layer.layer_string += line;
        }
        else
        {
            // find layer end
            if (line.find("</g") != std::string::npos) {
                tmp_layer.layer_string += line;

                m_Layers.push_back(tmp_layer);
                tmp_layer.layer_string = "";
            }
            else
            {
                if (layer_tag)
                {
                    tmp_layer.layer_string += line;
                }
               
            }
            
        }

        
    }


    //std::cout << "layer num: " << layer_num << std::endl;
}

void LithoModel::ProcessLayer(LithoLayer& layer)
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
            // std::cout << layer.layer_string.substr(namePosStart+4, namePosEnd - namePosStart-4);
            layer.z_value = layer.layer_string.substr(namePosStart + 3, namePosEnd - namePosStart - 3);

        }
    }

    //  3. polygons in this layer
    ProcessLayerPolygons(layer);
}

void LithoModel::ProcessLayerPolygons(LithoLayer& layer)
{
   
    // find all occurances 
    vector<size_t> startPos;
    vector<size_t> endPos;

    string toSearch = "<polygon";
    size_t pos = layer.layer_string.find(toSearch);

    while (pos != std::string::npos)
    {
        // Add position to the vector
        startPos.push_back(pos + toSearch.size());

        size_t ended = layer.layer_string.find("/>", pos + toSearch.size());
        if (ended!=string::npos)
        {
            endPos.push_back(ended);
        }

        // Get the next occurrence from the current position
        pos = layer.layer_string.find(toSearch, pos + toSearch.size());
    }

    //cout << layer.name << " has " << startPos.size()<<" "<<endPos.size() << endl;


    // parsing each polyogn to fill polygons with data 
    for (size_t polygonId = 0; polygonId < startPos.size(); polygonId++)
    {
        SVGPolygon polygon;
        string polygonString = layer.layer_string.substr(startPos[polygonId], endPos[polygonId]-startPos[polygonId]);
        ProcessPolygonString(polygonString, polygon);

        layer.polygons.push_back(polygon);
    }




}

void LithoModel::ProcessPolygonString(string& polygonString,SVGPolygon& polygon)
{
   
    
    // points
    string toSearch = "points=\"";
    size_t pointsPos = polygonString.find(toSearch);
    
    if (pointsPos != std::string::npos)
    {
        string endToSearch = "\"";
        size_t endedPos = polygonString.find(endToSearch, pointsPos + toSearch.size());
        if (endedPos != std::string::npos)
        {
            string pointsString = polygonString.substr(pointsPos + toSearch.size(), endedPos-(pointsPos + toSearch.size()));

            ProcessPointsString(pointsString,polygon);
        }
    }

}

void LithoModel::ProcessPointsString(string& pointsString,SVGPolygon& polygon)
{
    LPoint aPoint;
    size_t lastWhiteSpcaePos = 0;
    size_t whiteSpacePos = 0;

    string toSearch = " ";
    whiteSpacePos= pointsString.find(toSearch);

    polygon.points.clear();

    while (whiteSpacePos!=std::string::npos)
    {
        string aPointString = pointsString.substr(lastWhiteSpcaePos, whiteSpacePos - lastWhiteSpcaePos);
        lastWhiteSpcaePos = whiteSpacePos+toSearch.size();
        whiteSpacePos= pointsString.find(toSearch,lastWhiteSpcaePos);

        aPoint.clear();
        size_t commaPos = aPointString.find(",");
        aPoint.x = std::stof(aPointString.substr(0, commaPos));
        aPoint.y = std::stof(aPointString.substr(commaPos+1));

        polygon.points.push_back(aPoint);
    }

}

void LithoModel::ConvertToLithoLayer(vector<vector<SnowSlicer::contour>>& layers)
{
    int layerIndex = 0;
    for (auto& layer:layers)
    {
        LithoLayer aLithoLayer;
        aLithoLayer.name = std::to_string(layerIndex);

        int polygonIndex = 0;
        for (auto& contour:layer)
        {
            SVGPolygon aSVGPolygon;
            
            int pointNum = contour.P.size();

            for (int pointIndex = 0; pointIndex < pointNum-1; pointIndex++)
            {
                LPoint aPoint;
                aPoint.x = contour.P.at(pointIndex).x;
                aPoint.y = contour.P.at(pointIndex).y;
                aSVGPolygon.points.push_back(aPoint);
            }

            

            aLithoLayer.polygons.push_back(aSVGPolygon);

            polygonIndex += 1;
        }

        m_Layers.push_back(aLithoLayer);

        layerIndex += 1;
    }
    //std::cout << "stl sliced to " << m_Layers.size() << std::endl;


  
}

