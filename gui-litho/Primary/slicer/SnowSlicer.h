#pragma once
#include <string>
#include <vector>
#include <cmath>
#include <ctime>
#include <set>
#include <unordered_map>
#include <limits>
#include <array>
#include <map>
#include <sys/stat.h>
#include <io.h>

#define GLM_ENABLE_EXPERIMENTAL
#include <CGAL/Cartesian.h>
#include <CGAL/MP_Float.h>
#include <CGAL/Quotient.h>
#include <CGAL/Arr_segment_traits_2.h>
#include <CGAL/Sweep_line_2_algorithms.h>

#define GLM_FORCE_RADIANS

#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "glm/gtx/vector_angle.hpp"

#include "DataType.h"

namespace SnowSlicer 
{
	
	using namespace std;

	struct SlicingParameter
	{
		glm::vec2 FramePhysicalSize;
		glm::vec3 ModelAABBPhysicalSizeLimits; 
		int       NumLayers;
		string    ModelPath;
		bool	  SaveToSVGFile = true;
		float     Thickness = 1.0;
	};

	struct ModelInfo 
	{
		glm::vec3 bottom_left_vertex;
		glm::vec3 upper_right_vertex;
		glm::vec3 aabb_size;
	};


	class Slicer
	{
	public:// public function
		Slicer();
		~Slicer();
		void SetParameter(SlicingParameter& para);
		glm::vec2 LoadFile();
		void SetThickness(float thickness);
		void DoSlice();
		void ClearData();

	private://stage funciton
		
	
		
		int StlToMeshInMemery(string stlFile, TriangleMesh* mesh, bool isBinaryFormat, const char* rotate, double eps);

		vector<float> ComputePlanes(const TriangleMesh* mesh, float max_thickness, char* adaptive, double eps, float* delta);

		void IncrementalSlicing(const TriangleMesh* mesh, vector<float> P, float delta, bool srt, vector<vector<contour>>& polygons, bool chaining, bool orienting);
		
		


	private:// tool function
		Triangle make_triangle(
			float n0, float n1, float n2,
			float f0, float f1, float f2,
			float f3, float f4, float f5,
			float f6, float f7, float f8,
			const char* rotate, double eps
		);
		int checkASCIIFile(string fileName);
		v3 v3_round(float x, float y, float z, double eps);

		float xround(float x, double eps, int mod, int rem);

		bool degenerate(Triangle t);

		Mesh_Triangle_List_t** IncrementalSlicing_buildLists(bool srt, double delta, const TriangleMesh* mesh, vector<float> P);

		Mesh_Triangle_List_t* Mesh_Triangle_List_create(void);

		void Mesh_Triangle_List_insert(Triangle t, Mesh_Triangle_List_t* L);

		int IncrementalSlicing_binary_search(float zMin, vector<float> P);

		void Mesh_Triangle_List_union(Mesh_Triangle_List_t* L1, Mesh_Triangle_List_t* L2);
		Mesh_Triangle_Node_t* Mesh_Triangle_List_remove(Mesh_Triangle_List_t* L, Mesh_Triangle_Node_t* node);
		LineSegment R3_Mesh_Triangle_slice(Mesh_Triangle_Node_t* t, float Z);
		v3 R3_Mesh_Side_slice(v3 vi, v3 vj, float Z);
		void ContourConstruction(vector<LineSegment> segs, vector<vector<contour>>& polygons, int plane);
		void ray_casting(vector<contour>& polygons);
		bounding_box create_bounding_box();
		void export_svg_no_chaining(string fileName, vector<vector<LineSegment>> Segments, int k, const v3& aabbSize);
		vector<v3> IncrementalStartLoop(vector<PointMesh>& H);
		void IncrementalExtendLoop(vector<v3>& P, vector<PointMesh>& H);
		void IncrementalReverseLoop(vector<v3>& P);
		void add_point(v3 p1, v3 p2, vector<LineSegment>& t, bounding_box* bb, bool first, int index);
		bool contains(v3 point, bounding_box bb, vector<LineSegment> sides, int index);
		void fill_colors(rgb colors[], int nrgb);
		void add_svg_information(FILE* file);
		void update_bounding_box(v3 point, bounding_box* bb);
		bool insided_bounding_box(v3 point, bounding_box bb);
		LineSegment create_ray(v3 point, bounding_box bb, int index);
		bool ray_intersect(LineSegment ray, LineSegment side);
		bool is_inside(LineSegment line, v3 point);
		void export_svg_2d(string fileName, vector<vector<contour>> polygons, int nplanes, const v3& aabbSize);

	public:// data
		TriangleMesh m_Mesh;
		vector<float> m_Planes;
		vector<vector<contour>> m_Polygons;
		SlicingParameter m_Parameter;
		ModelInfo m_ModelInfo;

	private:// global var
		long intersections = 0;
		double m_Zmin;
		double m_Zmax;
		double m_Eps = 0.004;
		
		

	};

	
}