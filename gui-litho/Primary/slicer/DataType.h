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

#define GLM_FORCE_RADIANS
#define DEG_TO_RAD(x) (x*0.0174532925199f)

using namespace std;

namespace SnowSlicer{

	class v3 {

	public:

		v3(float _x = 0, float _y = 0, float _z = 0) : x(_x), y(_y), z(_z) {}

		float distTo(const v3& pt) const {
			return sqrt(pow(fabs(x - pt.x), 2.0) + pow(fabs(y - pt.y), 2.0) + pow(fabs(z - pt.z), 2.0));
		}

		array<float, 3> getCoords() {
			array<float, 3> c = { {x, y, z} };
			return c;
		}

		float dotproduct(const v3& v) const {
			return (x * v.x + y * v.y + z * v.z);
		}

		void transform(const glm::mat4& mat) {
			glm::vec4 v = glm::vec4(x, y, z, 1.0);
			glm::vec4 vt = mat * v;
			x = (vt.x); y = (vt.y); z = (vt.z);
		}

		v3& operator-=(const v3& pt) {
			x = (x - pt.x);
			y = (y - pt.y);
			z = (z - pt.z);
			return *this;
		}

		v3 operator-(const v3& pt) {
			return v3((x - pt.x), (y - pt.y), (z - pt.z));
		}

		v3 operator+(const v3& pt) {
			return v3((x + pt.x), (y + pt.y), (z + pt.z));
		}

		v3 operator/(float a) {
			return v3((x / a), (y / a), (z / a));
		}

		v3 operator*(float a) {
			return v3((x * a), (y * a), (z * a));
		}

		bool operator<(const v3& pt) const {
			return z < pt.z;
		}

		bool operator>(const v3& pt) const {
			return z > pt.z;
		}

		bool operator==(const v3& pt) const {
			return distTo(pt) < 0.005;
		}

		bool operator!=(const v3& pt) const {
			return distTo(pt) > 0.005;
		}

		float normalize() const {
			return sqrt(x * x + y * y + z * z);
		}

		string getLabel() const {
			stringstream ss;
			ss << x << "|" << y << "|" << z;
			return ss.str();
		}

		friend ostream& operator<<(ostream& os, const v3& v) {
			os << "x: " << v.x << "; y: " << v.y << "; z: " << v.z;
			return os;
		}

	public:

		float x;
		float y;
		float z;
	};

	
	inline v3 operator-(const v3& a, const v3& b) { return v3((a.x - b.x), (a.y - b.y), (a.z - b.z)); }
	inline v3 operator+(const v3& a, const v3& b) { return v3((a.x + b.x), (a.y + b.y), (a.z + b.z)); }


	class LineSegment {

	public:

		LineSegment(v3 p0 = v3(), v3 p1 = v3(), int i = 0) {
			v[0] = p0;
			v[1] = p1;
			index = i;
			vertical = false;
			if ((v[1].x - v[0].x) != 0) {
				a = (v[1].y - v[0].y) / (v[1].x - v[0].x);
				b = (v[0].y - (a * v[0].x));
			}
			else {
				vertical = true;
			}
		}

		bool operator==(const LineSegment& ls) const {
			return ((v[0] == ls.v[0]) && (v[1] == ls.v[1]));
		}

		friend ostream& operator<<(ostream& os, const LineSegment& ls) {
			os << "V0: (" << ls.v[0] << "); V1: (" << ls.v[1] << ")";
			return os;
		}

	public:

		v3 v[2];
		double a;
		double b;
		bool vertical;
		int index;
	};

	template<typename T> inline void hash_combine(size_t& seed, const T& v) {
		hash<T> hasher;
		seed ^= hasher(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
	}

	struct HashV3 {
		size_t operator() (const v3& v) const {
			size_t h = hash<float>()(v.x);
			hash_combine(h, v.y);
			hash_combine(h, v.z);
			return h;
		}
	};


	typedef unordered_map<v3, vector<v3>, HashV3> PointMesh;



	class Triangle {

	public:

		Triangle(v3 n, v3 v0, v3 v1, v3 v2) : normal(n) {
			v[0] = v0;
			v[1] = v1;
			v[2] = v2;
			zMin = +99999999.9;
			zMax = -99999999.9;
			setZMin(v0.z); setZMin(v1.z); setZMin(v2.z);
			setZMax(v0.z); setZMax(v1.z); setZMax(v2.z);
		}

		void setZMin(float z) {
			if (z < zMin) {
				zMin = z;
			}
		}

		void setZMax(float z) {
			if (z > zMax) {
				zMax = z;
			}
		}

		Triangle& operator-=(const v3& pt) {
			v[0] -= pt;
			v[1] -= pt;
			v[2] -= pt;
			return *this;
		}

		bool operator<(const Triangle& t) {
			return zMin < t.zMin;
		}

		friend ostream& operator<<(ostream& os, const Triangle& t) {
			os << "V0: (" << t.v[0] << "); V1: (" << t.v[1] << "); V2: (" << t.v[2] << ")";
			return os;
		}

	public:

		v3 v[3];
		v3 normal;
		float zMin;
		float zMax;
	};

	class TriangleMesh {

	public:

		TriangleMesh() : bottomLeftVertex(999999, 999999, 999999), upperRightVertex(-999999, -999999, -999999) { meshSize = 0; }

		size_t size() const {
			return meshSize;
		}

		void push_back(Triangle& t) {
			meshSize++;
			vTriangle.push_back(t);
			for (size_t i = 0; i < 3; ++i) {
				if (t.v[i].x < bottomLeftVertex.x) { bottomLeftVertex.x = t.v[i].x; }
				if (t.v[i].y < bottomLeftVertex.y) { bottomLeftVertex.y = t.v[i].y; }
				if (t.v[i].z < bottomLeftVertex.z) { bottomLeftVertex.z = t.v[i].z; }
				if (t.v[i].x > upperRightVertex.x) { upperRightVertex.x = t.v[i].x; }
				if (t.v[i].y > upperRightVertex.y) { upperRightVertex.y = t.v[i].y; }
				if (t.v[i].z > upperRightVertex.z) { upperRightVertex.z = t.v[i].z; }
			}
		}

		v3 meshAABBSize() const {
			return v3(upperRightVertex.x - bottomLeftVertex.x,
				upperRightVertex.y - bottomLeftVertex.y,
				upperRightVertex.z - bottomLeftVertex.z);
		}

		const vector<Triangle>& getvTriangle() const { return vTriangle; }

		v3 getBottomLeftVertex() const { return bottomLeftVertex; }

		v3 getUpperRightVertex() const { return upperRightVertex; }

	public:

		int meshSize;
		v3 bottomLeftVertex;
		v3 upperRightVertex;
		vector<Triangle> vTriangle;
	};

	typedef struct _rgb {
		int r, g, b;
	} rgb;

	typedef struct _bounding_box {
		double xMin;
		double xMax;
		double yMin;
		double yMax;
		bool firstPoint;
	} bounding_box;

	typedef struct _contour {
		bool external;
		bool clockwise;
		vector<v3> P;
	} contour;

	typedef struct node {
		Triangle t;
		struct node* next;
		struct node* prev;
	} Mesh_Triangle_Node_t;

	typedef struct _list {
		Mesh_Triangle_Node_t* head;
		Mesh_Triangle_Node_t* tail;
	} Mesh_Triangle_List_t;


}