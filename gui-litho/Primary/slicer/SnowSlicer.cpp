#include "SnowSlicer.h"


namespace SnowSlicer
{
	Slicer::Slicer()
	{
		
	}
	Slicer::~Slicer()
	{
	}
	void Slicer::SetParameter(SlicingParameter& para)
	{
		m_Parameter = para;
	}
	glm::vec2 Slicer::LoadFile()
	{
		// setting prameters
		string model = m_Parameter.ModelPath;
		string rotate_string = "false";
		char* rotate = (char*)rotate_string.c_str();


		// imoport stl files
		TriangleMesh mesh_temp;
		switch (checkASCIIFile(model)) 
		{
		case 1:
			StlToMeshInMemery(model, &mesh_temp, false, rotate, m_Eps) != 0;
		break;
		case 0:
			StlToMeshInMemery(model, &mesh_temp, true, rotate, m_Eps) != 0;
		break;
		default:
			cerr << "SnowSlicer: Unexpected error" << endl;
		}
		m_Mesh = mesh_temp;
		
		m_Zmax = std::max(m_Mesh.getUpperRightVertex().z, m_Mesh.meshAABBSize().z);
		m_Zmin = m_Mesh.getBottomLeftVertex().z;

		
		m_ModelInfo.bottom_left_vertex.x = m_Mesh.getBottomLeftVertex().x;
		m_ModelInfo.bottom_left_vertex.y = m_Mesh.getBottomLeftVertex().y;
		m_ModelInfo.bottom_left_vertex.z = m_Mesh.getBottomLeftVertex().z;

		m_ModelInfo.upper_right_vertex.x = m_Mesh.getUpperRightVertex().x;
		m_ModelInfo.upper_right_vertex.y = m_Mesh.getUpperRightVertex().y;
		m_ModelInfo.upper_right_vertex.z = m_Mesh.getUpperRightVertex().z;

		m_ModelInfo.aabb_size.x = m_Mesh.meshAABBSize().x;
		m_ModelInfo.aabb_size.y = m_Mesh.meshAABBSize().y;
		m_ModelInfo.aabb_size.z = m_Mesh.meshAABBSize().z;
		
		return glm::vec2(m_Zmin,m_Zmax);
	}

	void Slicer::SetThickness(float thickness)
	{
		m_Parameter.Thickness = thickness;
	}
	
	void Slicer::DoSlice()
	{
		// 1. computing planes
		float max_thickness = m_Parameter.Thickness;
		float delta;
		string adaptive_string = "false";
		char* adaptive = (char*)adaptive_string.c_str();
		m_Planes = ComputePlanes(&m_Mesh, max_thickness, adaptive, m_Eps, &delta);

		// 2. compute polygons
		bool srt = false;
		bool chaining = true;
		bool orienting = true;
		m_Polygons.resize(m_Planes.size());
		IncrementalSlicing(&m_Mesh, m_Planes, delta, srt, m_Polygons, chaining, orienting);

		// 3. save results
		if (m_Parameter.SaveToSVGFile)
		{
			ScalePolygonData(m_Polygons);
			
			export_svg_2d("sss.svg", m_Polygons, m_Planes.size(), m_Mesh.meshAABBSize());
		}
		


	}
	void Slicer::ClearData()
	{
	}
	int Slicer::StlToMeshInMemery(string stlFile, TriangleMesh* mesh, bool isBinaryFormat, const char* rotate, double eps)
	{
		
		int ndegenerated = 0;

		if (!isBinaryFormat) {

			ifstream in(stlFile);

			if (!in.good()) {
				return 1;
			}

			std::string s0, s1;

			float n0, n1, n2;
			float f0, f1, f2;
			float f3, f4, f5;
			float f6, f7, f8;

			while (!in.eof()) {
				in >> s0; /*s0 can be facet or solid!*/
				if (s0 == "facet") {
					in >> s1 >> n0 >> n1 >> n2; /* normal x y z. */
					in >> s0 >> s1;             /* loop. */
					in >> s0 >> f0 >> f1 >> f2; /* vertex x y z. */
					in >> s0 >> f3 >> f4 >> f5; /* vertex x y z. */
					in >> s0 >> f6 >> f7 >> f8; /* vertex x y z. */
					in >> s0;                   /* endloop. */
					in >> s0;                   /* endfacet.*/
					Triangle t = make_triangle(n0, n1, n2, f0, f2, f1, f3, f5, f4, f6, f8, f7, rotate, eps);
					if (!degenerate(t)) {
						mesh->push_back(t);
					}
					else {
						ndegenerated++;
					}
				}
				else if (s0 == "endsolid") {
					break;
				}
			}
			in.close();
		}
		else {
			FILE* f = fopen(stlFile.c_str(), "rb");
			if (!f) {
				return 1;
			}
			char title[80];
			int nFaces;
			int err;
			err = fread(title, 80, 1, f);
			err = fread((void*)&nFaces, 4, 1, f);
			float v[12]; /* normal = 3, vertices = 9 (12) */
			unsigned short uint16;
			/* Every Face is 50 Bytes: Normal(3*float), Vertices(9*float), 2 Bytes Spacer */
			for (size_t i = 0; i < nFaces; ++i) {
				for (size_t j = 0; j < 12; ++j) {
					err = fread((void*)&v[j], sizeof(float), 1, f);
				}
				err = fread((void*)&uint16, sizeof(unsigned short), 1, f); // spacer between successive faces
				Triangle t = make_triangle(v[0], v[1], v[2], v[3], v[4], v[5], v[6], v[7], v[8], v[9], v[10], v[11], rotate, eps);
				if (!degenerate(t)) {
					mesh->push_back(t);
				}
				else {
					ndegenerated++;
				}
			}
			fclose(f);
		}
		cout << "number of degenerated triangles = " << ndegenerated << endl;
		return 0;


	}

	vector<float> Slicer::ComputePlanes(const TriangleMesh* mesh, float max_thickness, char* adaptive, double eps, float* delta)
	{
		bool rounding = true; /*To avoid that the Z-coordinates of all planes are distinct from the Z-coordinates of all vertices.*/

		/* Vector to keep the plane coordinates: */
		vector<float> Planes;

		/* Assuming the model as a 3D axis-aligned bounding-box: */
		double model_zmax = std::max(mesh->getUpperRightVertex().z, mesh->meshAABBSize().z);

		double model_zmin = mesh->getBottomLeftVertex().z;

	


		if (strcmp(adaptive, "false") == 0) { /*Uniform slicing: */

			double spacing = (rounding ? xround(max_thickness, eps, 2, 0) : max_thickness); /*Plane spacing even multiple of {eps}*/

			double P0 = xround(model_zmin - spacing, eps, 2, 1); /*First plane odd multiple of {eps}.*/

			int no_planes = 1 + (int)((model_zmax + spacing - P0) / spacing); /* Number of planes: */

			cout << "eps = " << eps << endl;
			cout << "max thickness = " << max_thickness << endl;
			cout << "rounded plane spacing spacing = " << spacing << endl;
			cout << "model zmin = " << model_zmin << ", model zmax = " << model_zmax << ", first plane Z = " << P0 << ", number of planes = " << no_planes << endl;

			for (size_t i = 0; i < no_planes; i++) {
				/* Building the vector with the slice z coordinates: */
				float Pi = (float)(P0 + i * spacing);
				if ((Pi > model_zmin) && (Pi < model_zmax)) {
					Planes.push_back((float)(P0 + i * spacing));
				}
			}
			*delta = (float)(spacing);
		}
		else { /*Adaptive slicing z-planes: */
			std::cout << "adaptive mode" << std::endl;
			float zplane = 0.0;
			float min_thickness = 0.016;
			Planes.push_back(model_zmin + zplane);

			while ((model_zmin + zplane) <= model_zmax) {
				double vrandom = min_thickness + (max_thickness - min_thickness) * (rand() / (double)RAND_MAX);
				double coordinate = xround(model_zmin + zplane + vrandom, eps, 2, 1);
				if (coordinate >= model_zmax) { break; }
				Planes.push_back(coordinate);
				zplane += vrandom;
			}
		}

		return Planes;
	}

	void Slicer::IncrementalSlicing(const TriangleMesh* mesh, vector<float> P, float delta, bool srt, vector<vector<contour>>& polygons, bool chaining, bool orienting)
	{

		/*Slicing*/
		clock_t slice_begin = clock();

		int k = P.size();

		//vector<LineSegment> segs[k];
		vector<vector<LineSegment>> segs;
		segs.resize(k);

		/* Classify triangles by the plane gaps that contain their {zMin}: */
		Mesh_Triangle_List_t** L = IncrementalSlicing_buildLists(srt, delta, mesh, P);
		/* Now perform a plane sweep from bottom to top: */

		Mesh_Triangle_List_t* A = Mesh_Triangle_List_create(); /* Active triangle list. */

		//cout << "start find vertices on each plane" << endl;

		for (int p = 0; p < k; p++) {
			/* Add triangles that start between {P[p-1]} and {P[p]}: */
			Mesh_Triangle_List_union(A, L[p]);
			/* Scan the active triangles: */
			Mesh_Triangle_Node_t* aux = A->head;






			while (aux != NULL) {
				Mesh_Triangle_Node_t* next = aux->next;
				if (aux->t.zMax < P[p]) {
					/* Triangle is exhausted: */
					Mesh_Triangle_List_remove(A, aux);
				}
				else {
					/* Compute intersection: */
					if ((aux->t.zMin < P[p]) && (aux->t.zMax > P[p])) {
						LineSegment seg = R3_Mesh_Triangle_slice(aux, P[p]);
						segs[p].push_back(seg);
						intersections++;
					}
				}
				aux = next;
			}
		}
		free(L);
		clock_t slice_end = clock();
		/*End-Slicing*/

		if (chaining) {
			/*Contour construction:*/
			for (size_t p = 0; p < k; p++) {
				if (!segs[p].empty()) {
					ContourConstruction(segs[p], polygons, p);

					if (orienting) {
						ray_casting(polygons[p]);
					}
					segs[p].clear();
				}
			}
			/*End construction.*/
			
		}
		else {
			//export_svg_no_chaining("segments.svg", segs, k, mesh->meshAABBSize());
			export_svg_no_chaining("segments.svg", segs, k, mesh->meshAABBSize());
		}
	}

	void Slicer::ScalePolygonData(vector<vector<contour>>& polygons)
	{
		auto aabb = m_Mesh.meshAABBSize();
		
		float size = std::max(aabb.x, aabb.y);

		float scale = 100.f / size;

		ScalePolygonData(polygons, scale);
	}

	void Slicer::ScalePolygonData(vector<vector<contour>>& polygons, float scale)
	{
		

		for (auto& plane:polygons)
		{
			for (auto& polygon:plane)
			{
				for (auto& point:polygon.P)
				{
			
					point = point * scale;
					
				}
			}
		}
	}




	Triangle Slicer::make_triangle(float n0, float n1, float n2, float f0, float f1, float f2, float f3, float f4, float f5, float f6, float f7, float f8, const char* rotate, double eps)
	{
		if (strcmp(rotate, "true") == 0) {
			return Triangle(v3(n0, n1, n2), v3_round(f0, f2, f1, eps), v3_round(f3, f5, f4, eps), v3_round(f6, f8, f7, eps));
		}
		else {
			return Triangle(v3(n0, n1, n2), v3_round(f0, f1, f2, eps), v3_round(f3, f4, f5, eps), v3_round(f6, f7, f8, eps));
		}

	}
	int Slicer::checkASCIIFile(string fileName)
	{
		string line1, line2;
		ifstream input(fileName);
		if (!input.good()) {
			return -1;
		}
		getline(input, line1);
		getline(input, line2);
		if (line1.find("solid") != string::npos && line2.find("facet") != string::npos) {
			return 1;
		}
		if (line1.find("xml") != string::npos && line2.find("amf") != string::npos) {
			return 2;
		}
		return 0;
	}
	v3 Slicer::v3_round(float x, float y, float z, double eps)
	{
		v3 p;
		p.x = xround(x, eps, 2, 0);
		p.y = xround(y, eps, 2, 0);
		p.z = xround(z, eps, 2, 0);
		return p;

	}
	float Slicer::xround(float x, double eps, int mod, int rem)
	{
		double y = round((double)x / (mod * eps));
		double z = (y * mod + rem) * eps;
		return (float)z;
	}
	bool Slicer::degenerate(Triangle t)
	{
		if (t.v[0].distTo(t.v[1]) < 0.000001) { return true; }
		if (t.v[1].distTo(t.v[2]) < 0.000001) { return true; }
		if (t.v[2].distTo(t.v[0]) < 0.000001) { return true; }
		return false;
	}
	Mesh_Triangle_List_t** Slicer::IncrementalSlicing_buildLists(bool srt, double delta, const TriangleMesh* mesh, vector<float> P)
	{

		int k = P.size(); /* Number of planes. */

		Mesh_Triangle_List_t** L = (Mesh_Triangle_List_t**)malloc((k + 1) * sizeof(Mesh_Triangle_List_t*));

		for (size_t p = 0; p <= k; p++) { L[p] = Mesh_Triangle_List_create(); }

		const vector<Triangle>& T = mesh->getvTriangle();

		int n = T.size(); /* Number of triangles. */

		if (delta > 0.0) {
			/* Uniform slicing - compute list index: */
			for (auto it = T.begin(), itEnd = T.end(); it != itEnd; ++it) {
				Triangle t = *it;
				int p;
				if (t.zMin < P[0]) {
					p = 0;
				}
				else if (t.zMin > P[k - 1]) {
					p = k;
				}
				else {
					p = floor((t.zMin - P[0]) / delta) + 1;
				}
				Mesh_Triangle_List_insert(t, L[p]);
			}
		}
		else if (srt) {
			/* Slicing of a pre-sorted mesh - merge {zMin}s and {P}: */
			auto it = T.begin();
			auto itEnd = T.end();
			double zprev = -INFINITY;
			for (int p = 0; p <= k; p++) {
				float Zp = (p < k ? P[k] : +INFINITY);
				const Triangle t = *it;
				assert(t.zMin >= zprev);
				while ((it != itEnd) && (t.zMin < Zp)) {
					Mesh_Triangle_List_insert(t, L[p]);
					zprev = t.zMin;
					it++;
				}
			}
		}
		else {
			/* General case: */
			for (auto it = T.begin(), itEnd = T.end(); it != itEnd; ++it) {
				const Triangle t = *it;
				int p = IncrementalSlicing_binary_search(t.zMin, P);
				assert((p >= 0) && (p <= k));
				Mesh_Triangle_List_insert(t, L[p]);
			}
		}
		return L;
	}
	Mesh_Triangle_List_t* Slicer::Mesh_Triangle_List_create(void)
	{
		Mesh_Triangle_List_t* L = (Mesh_Triangle_List_t*)malloc(sizeof(Mesh_Triangle_List_t));
		L->head = NULL;
		L->tail = NULL;
		return L;
	}
	void Slicer::Mesh_Triangle_List_insert(Triangle t, Mesh_Triangle_List_t* L)
	{
		Mesh_Triangle_Node_t* node = (Mesh_Triangle_Node_t*)malloc(sizeof(Mesh_Triangle_Node_t));
		node->t = t;
		node->next = L->head;
		node->prev = NULL;
		if (L->head == NULL) {
			/*New head*/
			L->head = L->tail = node;
		}
		else {
			L->head->prev = node;
			L->head = node;
		}
	}
	int Slicer::IncrementalSlicing_binary_search(float zMin, vector<float> P)
	{
		int k = P.size();
		assert(k >= 1);
		if (zMin >= P[k - 1]) { return k; }
		/* Binary search: */
		int l = -1; /* Inferior Z index. */
		int r = k;  /* Superior Z index. */
		while (r - l > 1) {
			/* At this point, {zMin} is between {P[l]} and {P[r]}. */
			int m = (l + r) / 2;
			assert((0 <= m) && (m < k));
			if (zMin >= P[m]) {
				l = m;
			}
			else {
				r = m;
			}
		}
		return r;
	}
	void Slicer::Mesh_Triangle_List_union(Mesh_Triangle_List_t* L1, Mesh_Triangle_List_t* L2)
	{
		if ((L1->head != NULL) && (L2->head != NULL)) {
			L1->tail->next = L2->head;
			L2->head->prev = L1->tail;
			L1->tail = L2->tail;;
		}
		else if (L2->head != NULL) {
			L1->head = L2->head;
			L1->tail = L2->tail;
		}
	}
	Mesh_Triangle_Node_t* Slicer::Mesh_Triangle_List_remove(Mesh_Triangle_List_t* L, Mesh_Triangle_Node_t* node)
	{
		if ((node->prev == NULL) && (node->next == NULL)) {
			free(node);
			L->head = NULL;
			L->tail = NULL;
			return NULL;
		}
		else if (node->prev == NULL) {
			node->next->prev = NULL;
			L->head = node->next;
			free(node);
			return L->head;
		}
		else if (node->next == NULL) {
			node->prev->next = NULL;
			L->tail = node->prev;
			free(node);
			return NULL;
		}
		else {
			Mesh_Triangle_Node_t* next = node->next;
			node->next->prev = node->prev;
			node->prev->next = next;
			free(node);
			return next;
		}
	}
	LineSegment Slicer::R3_Mesh_Triangle_slice(Mesh_Triangle_Node_t* t, float Z)
	{
		assert((t->t.zMin < Z) && (t->t.zMax > Z));
		int np = 0; /* Number of segment endpoints found */
		LineSegment seg;
		for (int i = 0; i < 3; i++) {
			/* Get side {i} of triangle: */
			int j = (i == 2 ? 0 : i + 1);
			v3 vi = (t->t.v[i]);
			v3 vj = (t->t.v[j]);
			/* Check for intersection of plane with {vi--vj}. */
			/* Must consider segment closed at bottom and open at top in case {Z} goes through a vertex. */
			float vzMin = (vi.z < vj.z ? vi.z : vj.z);
			float vzMax = (vi.z > vj.z ? vi.z : vj.z);
			if ((vzMin <= Z) && (vzMax > Z)) {
				v3 p = R3_Mesh_Side_slice(vi, vj, Z);
				assert(np < 2);
				seg.v[np] = p;
				np++;
			}
		}
		assert(np == 2);
		return seg;
	}
	v3 Slicer::R3_Mesh_Side_slice(v3 vi, v3 vj, float Z)
	{
		double dx = vj.x - vi.x;
		double dy = vj.y - vi.y;
		double dz = vj.z - vi.z;
		assert(dz != 0);
		double frac = (Z - vi.z) / dz;
		float xint = (float)(frac * dx + (double)vi.x);
		float yint = (float)(frac * dy + (double)vi.y);
		v3 ret;
		ret.x = xint;
		ret.y = yint;
		ret.z = Z;
		return ret;
	}
	void Slicer::ContourConstruction(vector<LineSegment> segs, vector<vector<contour>>& polygons, int plane)
	{
		bool verbose = true;

		clock_t contour_begin = clock();

		/*Creating the hash table.*/
		vector<PointMesh> H(1);

		/*Rounding vertices and filling the hash table.*/
		double eps = 1 / 128.0;
		for (std::vector<LineSegment>::iterator i = segs.begin(); i != segs.end(); i++) {
			LineSegment q = *i;
			q.v[0].x = round(q.v[0].x / eps) * eps;
			q.v[0].y = round(q.v[0].y / eps) * eps;
			q.v[0].z = plane;
			q.v[1].x = round(q.v[1].x / eps) * eps;
			q.v[1].y = round(q.v[1].y / eps) * eps;
			q.v[1].z = plane;
			if (q.v[0].distTo(q.v[1]) > 0.0001) {
				(H[0][q.v[0]]).push_back(q.v[1]);
				(H[0][q.v[1]]).push_back(q.v[0]);
			}
		}

		/* Count vertices by degree: */
		if (verbose) {
			int degmax = 10;
			int ctdeg[10 + 1];
			for (int deg = 0; deg <= degmax; deg++) { ctdeg[deg] = 0; }
			for (auto i = (H[0]).begin(); i != (H[0]).end(); i++) {
				vector<v3> L = (*i).second;
				int deg = L.size();
				if (deg > degmax) { deg = degmax; }
				ctdeg[deg]++;
			}
			assert(ctdeg[0] == 0);
			bool closedSlice = true;
			for (int deg = 1; deg <= degmax; deg++) {
				if (((deg % 2) != 0) && (ctdeg[deg] > 0)) { closedSlice = false; }
				if ((verbose || (deg != 2)) && (ctdeg[deg] != 0))
				{
					//cout << "there are " << ctdeg[deg] << " vertices of degree " << deg << " on plane " << plane << endl;
				}
			}
			if (!closedSlice) { cout << "** contours of plane " << plane << " are not closed" << endl; }
		}

		/*Contour construction.*/
		bool maximal = true;
		while (!(H[0]).empty()) {
			if (maximal) {
				vector<v3> P = IncrementalStartLoop(H);
				IncrementalExtendLoop(P, H);
				if (P.front() != P.back()) { //Chain {P} is open
					IncrementalReverseLoop(P);
					IncrementalExtendLoop(P, H);
				}
				polygons[plane].push_back({ false, false, P });
			}
			else {
				vector<v3> P = IncrementalStartLoop(H);
				IncrementalExtendLoop(P, H);
				polygons[plane].push_back({ false, false, P });
			}
		}
		clock_t contour_end = clock();
		//loopclosure_time += double(contour_end - contour_begin) / CLOCKS_PER_SEC;
	}
	void Slicer::ray_casting(vector<contour>& polygons)
	{
		vector<LineSegment> segments;

		bounding_box bb = create_bounding_box();

		/*Creating the line segments of each contour: */
		for (int i = 0; i < polygons.size(); i++) {
			double area = 0.0;
			vector<v3> Pi = polygons.at(i).P;
			for (int j = 1; j < Pi.size(); j++) {
				v3 p0 = Pi.at(j - 1);
				v3 p1 = Pi.at(j + 0);
				area += (p0.x * p1.y - p0.y * p1.x);
				add_point(p0, p1, segments, &bb, (j == 1 ? true : false), i);
				if (j == Pi.size() - 1) {
					add_point(p1, Pi.at(0), segments, &bb, (j == 1 ? true : false), i);
					area += (p1.x * Pi.at(0).y - p1.y * Pi.at(0).x);
				}
			}
			area /= 2.0;
			if (area < 0.0) {
				polygons.at(i).clockwise = true;
			}
			else {
				polygons.at(i).clockwise = false;
			}
		}

		/*Using the point in polygon algorithm to test the first segment of each contour: */
		for (int i = 0; i < polygons.size(); i++) {
			vector<v3> Pi = polygons.at(i).P;
			if (contains(Pi.at(0), bb, segments, i)) {
				/*Internal contour: */
				polygons.at(i).external = false;
			}
			else {
				/*External contour: */
				polygons.at(i).external = true;
			}

			/*Reversing contours: */
			if (polygons.at(i).external && polygons.at(i).clockwise) {
				std::reverse(polygons.at(i).P.begin(), polygons.at(i).P.end());
				polygons.at(i).clockwise = false;
			}
			else if (!polygons.at(i).external && !polygons.at(i).clockwise) {
				std::reverse(polygons.at(i).P.begin(), polygons.at(i).P.end());
				polygons.at(i).clockwise = true;
			}
		}
		segments.clear();
	}
	bounding_box Slicer::create_bounding_box()
	{
		bounding_box bb;
		bb.xMax = std::numeric_limits<double>::min();
		bb.xMin = std::numeric_limits<double>::max();
		bb.yMax = std::numeric_limits<double>::min();
		bb.yMin = std::numeric_limits<double>::max();
		return bb;
	}
	void Slicer::export_svg_no_chaining(string fileName, vector<vector<LineSegment>> Segments, int k, const v3& aabbSize)
	{
		int nrgb = 8;
		rgb colors[8];
		fill_colors(colors, nrgb);

		FILE* file = NULL;

		float dx = 0.0, dy = 0.0;

		file = fopen(fileName.c_str(), "w");
		printf("\n\nwriting output file: %s\n", fileName.c_str());

		if (!file) { exit(1); }

		add_svg_information(file);

		const size_t slicePerRow = (size_t)sqrt((float)k) * 2;

		for (size_t i = 0; i < k; ++i) {
			dx = (float)(i % slicePerRow) * (aabbSize.x * 1.05f);
			dy = (float)(i / slicePerRow) * (aabbSize.y * 1.05f);
			for (const LineSegment& ls : Segments[i]) {
				fprintf(file, "   <line x1=\"%f\" y1=\"%f\" x2=\"%f\" y2=\"%f\" stroke-width=\"1\" stroke=\"rgb(%d,%d,%d)\"/>\n",
					dx + ls.v[0].x, dy + ls.v[0].y, dx + ls.v[1].x, dy + ls.v[1].y,
					colors[i % nrgb].r, colors[i % nrgb].g, colors[i % nrgb].b);
			}
			Segments[i].clear();
		}
		fprintf(file, "</svg>\n");
		fclose(file);
	}
	vector<v3> Slicer::IncrementalStartLoop(vector<PointMesh>& H)
	{
		vector<v3> P;
		auto it = (H[0]).begin();
		v3 u = (*it).first;
		vector<v3> vw = (*it).second;
		v3 v = vw.at(0);
		P.push_back(u);
		P.push_back(v);
		(H[0][u]).erase(std::remove((H[0][u]).begin(), (H[0][u]).end(), v), (H[0][u]).end());
		if (H[0][u].size() == 0) { (H[0]).erase(u); }
		(H[0][v]).erase(std::remove((H[0][v]).begin(), (H[0][v]).end(), u), (H[0][v]).end());
		if (H[0][v].size() == 0) { (H[0]).erase(v); }
		return P;
	}
	void Slicer::IncrementalExtendLoop(vector<v3>& P, vector<PointMesh>& H)
	{
		int index = 0;
		int n = P.size();
		v3 first = P.front();
		v3 current = P.back();
		v3 last;

		/* Collect other vertices: */
		while (true) {
			auto it = (H[0]).find(current);
			if (it == (H[0]).end()) { /*Vertex {current} is a dead end:*/ break; }
			v3 key1 = (*it).first; assert(key1 == current);  /*Paranoia check.*/

			/*Get {next}, the first unused neighbor of {current}:*/
			vector<v3> vw = (*it).second; /*Unused neighbors of {current}.*/
			assert(vw.size() != 0);
			v3 next = vw.at(0); /*First unused neighbor of {current}.*/

			/*Append the segment {(current,next)} to {P} and delete from {H}:*/
			P.push_back(next);

			/*Remove the segment {(current,next)} from {H}:*/
			(H[0][current]).erase(std::remove((H[0][current]).begin(), (H[0][current]).end(), next), (H[0][current]).end());
			if (H[0][current].size() == 0) { (H[0]).erase(current); }
			(H[0][next]).erase(std::remove((H[0][next]).begin(), (H[0][next]).end(), current), (H[0][next]).end());
			if (H[0][next].size() == 0) { (H[0]).erase(next); }

			if (next == first) { /*We closed a loop:*/ break; }

			/*Move on:*/
			current = next;
		}
	}
	void Slicer::IncrementalReverseLoop(vector<v3>& P)
	{
		std::reverse(P.begin(), P.end());

	}
	void Slicer::add_point(v3 p1, v3 p2, vector<LineSegment>& t, bounding_box* bb, bool first, int index)
	{
		if (first) {
			update_bounding_box(p1, bb);
		}
		update_bounding_box(p2, bb);
		LineSegment line(p1, p2, index);
		t.push_back(line);
	}
	bool Slicer::contains(v3 point, bounding_box bb, vector<LineSegment> sides, int index)
	{
		if (insided_bounding_box(point, bb)) {
			LineSegment ray = create_ray(point, bb, index);
			int intersection = 0;
			for (int i = 0; i < sides.size(); i++) {
				if ((sides.at(i).index != index) && ray_intersect(ray, sides.at(i))) {
					intersection++;
				}
			}
			/* If the number of intersections is odd, then the point is inside the polygon: */
			if ((intersection % 2) == 1) {
				return true;
			}
		}
		return false;
	}
	void Slicer::fill_colors(rgb colors[], int nrgb)
	{
		assert(nrgb == 8);
		colors[0] = { 0,   0,   0 }; /*Black*/
		colors[1] = { 255,   0,   0 }; /*Red*/
		colors[2] = { 128,   0, 128 }; /*Purple*/
		colors[3] = { 0, 128,   0 }; /*Green*/
		colors[4] = { 0,   0, 255 }; /*Blue*/
		colors[5] = { 0, 255, 255 }; /*Cyan*/
		colors[6] = { 128, 128,   0 }; /*Olive*/
		colors[7] = { 128, 128, 128 }; /*Gray*/
	}
	void Slicer::add_svg_information(FILE* file)
	{
		fprintf(file, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
		fprintf(file, "<!DOCTYPE svg PUBLIC \"-//W3C//DTD SVG 1.1//EN\" \"http://www.w3.org/Graphics/SVG/1.1/DTD/svg11.dtd\">\n");
		fprintf(file, "<svg preserveAspectRatio=\"xMidYMid meet\" width=\"1024\" height=\"768\" viewBox=\"0 0 1024 768\"");
		fprintf(file, " xmlns=\"http://www.w3.org/2000/svg\" xmlns:xlink=\"http://www.w3.org/1999/xlink\"");
		fprintf(file, " xmlns:rdf=\"http://www.w3.org/1999/02/22-rdf-syntax-ns#\" xmlns:dc=\"http://purl.org/dc/elements/1.1/\"");
		fprintf(file, " xmlns:cc=\"http://web.resource.org/cc/\">\n");
		fprintf(file, " <defs>\n");
		fprintf(file, "  <marker id=\"arrow\" markerWidth=\"10\" markerHeight=\"10\" refx=\"0\" refy=\"3\" orient=\"auto\" markerUnits=\"strokeWidth\" viewBox=\"0 0 20 20\">\n");
		fprintf(file, "    <path d=\"M0,0 L0,6 L9,3 z\" fill=\"#f00\" />\n");
		fprintf(file, "  </marker>\n");
		fprintf(file, "</defs>\n");

	}
	void Slicer::update_bounding_box(v3 point, bounding_box* bb)
	{
		/* Setting the bounding box: */
		if (point.x > bb->xMax) {
			bb->xMax = point.x;
		}
		else if (point.x < bb->xMin) {
			bb->xMin = point.x;
		}
		if (point.y > bb->yMax) {
			bb->yMax = point.y;
		}
		else if (point.y < bb->yMin) {
			bb->yMin = point.y;
		}
	}
	bool Slicer::insided_bounding_box(v3 point, bounding_box bb)
	{
		if ((point.x < bb.xMin) || (point.x > bb.xMax) || (point.y < bb.yMin) || (point.y > bb.yMax)) {
			return false;
		}
		return true;
	}
	LineSegment Slicer::create_ray(v3 point, bounding_box bb, int index)
	{
		/* Create outside point: */
		double epsilon = (bb.xMax - bb.xMin) / 100.0;
		v3 outside(bb.xMin - epsilon, bb.yMin);
		LineSegment v(outside, point, index);
		return v;
	}
	bool Slicer::ray_intersect(LineSegment ray, LineSegment side)
	{
		v3 intersectPoint;
		/* If both vectors aren't from the kind of x=1 lines then go into: */
		if (!ray.vertical && !side.vertical) {
			/* Check if both vectors are parallel. If they are parallel then no intersection point will exist: */
			if (ray.a - side.a == 0) {
				return false;
			}
			intersectPoint.x = ((side.b - ray.b) / (ray.a - side.a));
			intersectPoint.y = side.a * intersectPoint.x + side.b;
		}
		else if (ray.vertical && !side.vertical) {
			intersectPoint.x = ray.v[0].x;
			intersectPoint.y = side.a * intersectPoint.x + side.b;
		}
		else if (!ray.vertical && side.vertical) {
			intersectPoint.x = side.v[0].x;
			intersectPoint.y = ray.a * intersectPoint.x + ray.b;
		}
		else {
			return false;
		}
		if (is_inside(side, intersectPoint) && is_inside(ray, intersectPoint)) {
			return true;
		}
		return false;
	}
	bool Slicer::is_inside(LineSegment line, v3 point)
	{
		double maxX = (line.v[0].x > line.v[1].x) ? line.v[0].x : line.v[1].x;
		double minX = (line.v[0].x < line.v[1].x) ? line.v[0].x : line.v[1].x;
		double maxY = (line.v[0].y > line.v[1].y) ? line.v[0].y : line.v[1].y;
		double minY = (line.v[0].y < line.v[1].y) ? line.v[0].y : line.v[1].y;
		if ((point.x >= minX && point.x <= maxX) && (point.y >= minY && point.y <= maxY)) {
			return true;
		}
		return false;
	}
	void Slicer::export_svg_2d(string fileName, vector<vector<contour>> polygons, int nplanes, const v3& aabbSize)
	{
		int nrgb = 8;
		rgb colors[8];
		fill_colors(colors, nrgb);

		FILE* file = NULL;

		float dx = 0.0, dy = 0.0;

		file = fopen(fileName.c_str(), "w");
		printf("\n\nwriting output file: %s\n", fileName.c_str());

		if (!file) { exit(1); }

		add_svg_information(file);

		for (int p = 0; p < nplanes; p++) {
			//cout << "plane: " << p << endl;
			vector<contour> P = polygons[p];
			const size_t k = P.size();
			const size_t slicePerRow = (size_t)sqrt((float)nplanes) * 2;
			for (size_t i = 0; i < k; ++i) {
				//cout << "polygon: " << i << endl;
				for (size_t index = 0; index < P[i].P.size(); index++) {
					const v3& p0 = P[i].P.at(index);
					//cout << "point " << index <<" " << p0.x << " " << p0.y << endl;
					dx = (float)(p % slicePerRow) * (aabbSize.x * 100.05f);
					dy = (float)(p / slicePerRow) * (aabbSize.y * 100.05f);
					if (index < (P[i].P.size() - 1)) {
						const v3& p1 = P[i].P.at(index + 1);
						//if (P[i].external) {
						//if (true) {
						if (P[i].clockwise) {
							//fprintf (file, "   <line x1=\"%f\" y1=\"%f\" x2=\"%f\" y2=\"%f\" stroke-width=\"1\" stroke=\"rgb(%d,%d,%d)\" marker-end=\"url(#arrow)\"/> \n",
							fprintf(file, "   <line x1=\"%f\" y1=\"%f\" x2=\"%f\" y2=\"%f\" stroke-width=\"1\" stroke=\"rgb(%d,%d,%d)\"/>\n",
								dx + p0.x, dy + p0.y, dx + p1.x, dy + p1.y, colors[0].r, colors[0].g, colors[0].b);
							//dx + p0.x, dy + p0.y, dx + p1.x, dy + p1.y, colors[i % nrgb].r, colors[i % nrgb].g, colors[i % nrgb].b);
						}
						else {
							//fprintf (file, "   <line x1=\"%f\" y1=\"%f\" x2=\"%f\" y2=\"%f\" stroke-width=\"1\" stroke=\"rgb(%d,%d,%d)\" marker-end=\"url(#arrow)\"/>\n",
							fprintf(file, "   <line x1=\"%f\" y1=\"%f\" x2=\"%f\" y2=\"%f\" stroke-width=\"1\" stroke=\"rgb(%d,%d,%d)\"/>\n",
								dx + p0.x, dy + p0.y, dx + p1.x, dy + p1.y, colors[1].r, colors[1].g, colors[1].b);
							//dx + p0.x, dy + p0.y, dx + p1.x, dy + p1.y, colors[i % nrgb].r, colors[i % nrgb].g, colors[i % nrgb].b);
						}
					}
				}
			}
		}
		fprintf(file, "</svg>\n");
		fclose(file);
	}
}