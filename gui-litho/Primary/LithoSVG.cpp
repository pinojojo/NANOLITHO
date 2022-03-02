#include "LithoSVG.h"

void litho::LithoSVG::LoadSVGFromStl(string stl_path, glm::vec3 bounding_box)
{
    glm::vec3 box, corner;
    GetRawSTLInfo(stl_path, box, corner);

    // calc scaling ratio
    float scaling_value = 0;
    float ratio_raw = box.y / box.x;
    float ratio_expected = bounding_box.y / bounding_box.x;
    
    if (ratio_raw>ratio_expected)
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
    
}

void litho::LithoSVG::LoadSVG(string svg_path)
{
    svg_path_ = svg_path;
    ParseXML();

    for (auto& layer:data_)
    {
        ParseLayer(layer);
    }

    //print info
    int layer_id = 0;
    for (auto& layer:data_)
    {
        std::cout << "layer: " + std::to_string(layer_id++)
            << " z: "<<layer.z_value
            << " has " << std::to_string(layer.polygons.size()) << " polygons "<<std::endl;
    }
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
        for (size_t polygonId = 0; polygonId < startPos.size(); polygonId++)
        {
            Polygon temp_polygon;

            temp_polygon.polygon_string = layer.layer_string.substr(startPos[polygonId], endPos[polygonId] - startPos[polygonId]);
            
            ParsePolygon(temp_polygon);

            layer.polygons.push_back(temp_polygon);
        }

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

            
            if (point.x< bbox_corner_lower.x)
            {
                bbox_corner_lower.x = point.x;
            } 
            if (point.y< bbox_corner_lower.y)
            {
                bbox_corner_lower.y = point.y;
            } 
            if (point.z< bbox_corner_lower.z)
            {
                bbox_corner_lower.z = point.z;
            }

            if (point.x> bbox_corner_upper.x)
            {
                bbox_corner_upper.x = point.x;
            } 
            if (point.y> bbox_corner_upper.y)
            {
                bbox_corner_upper.y = point.y;
            } 
            if (point.z> bbox_corner_upper.z)
            {
                bbox_corner_upper.z = point.z;
            }
        }
       
    }

    std::cout
        << "upper " << bbox_corner_upper.x << " " << bbox_corner_upper.y << " " << bbox_corner_upper.z << " " << std::endl
        << "lower " << bbox_corner_lower.x << " " << bbox_corner_lower.y << " " << bbox_corner_lower.z << " " << std::endl;

    size = bbox_corner_upper - bbox_corner_lower;
    
    corner = bbox_corner_lower;
}

void litho::LithoSVG::Slic3rCLI(string input_path, string output_path, float scale, float thickness)
{
    std::string cmd_args = "Slic3r-console.exe";
    cmd_args += " --export-svg";
    cmd_args += " ../bin/stl/micro-lens.stl";
    cmd_args += " --scale " + std::to_string(scale);
    cmd_args += " --layer-height "+std::to_string(thickness);
    cmd_args += " -o " + output_path;


    PROCESS_INFORMATION pi;

    STARTUPINFO si;
    ZeroMemory(&si, sizeof(STARTUPINFO));
    si.cb = sizeof(STARTUPINFO);

    if (CreateProcess(
        "../bin/slic3r-lzx/Slic3r-console.exe",
        &cmd_args[0],
        NULL, 
        NULL, 
        TRUE, 
        0,
        NULL,
        NULL, 
        &si, &pi))
    {
        
    }
    else
    {
        std::cout << "ERROR: CLI failed!" << std::endl;
    }


}

