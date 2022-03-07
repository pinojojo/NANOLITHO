#pragma once
#include "LithoType.h"
#include "LithoSVG.h"


#define GLM_FORCE_RADIANS
#include <GL/glew.h>
#include <gl/GL.h>
#include "shader.h"
#include <array>
#include <algorithm>
#include "mapbox/earcut.hpp"
#define _USE_MATH_DEFINES
#include <math.h>


#include "shader.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <vector>
#include <string>
#include <array>
#include <algorithm>

namespace ear 
{
	using Coord = float;
	using N = unsigned int;
	using Point = std::array<Coord, 2>;
}

struct EarcutPolygon
{
	std::vector<std::vector<ear::Point>> polygons;// why vector of vector? cause earcut only accept this
	std::vector<ear::N> indices;
	bool is_hole;
};

struct DrawablePolygon
{
	std::vector<glm::vec2> points;
	std::vector<unsigned int> indices;
	GLuint vao;
	GLuint vbo;
	GLuint ebo;
	bool is_hole;

	void clear() {
		points.clear();
		indices.clear();

		glDeleteBuffers(1, &vbo);
		glDeleteBuffers(1, &ebo);
		glDeleteVertexArrays(1, &vao);

	}
};

class MaskRenderer
{
public:
	MaskRenderer() {}

	void Init(litho::LithoSetting setting);

	void UpdateLayer(litho::LithoSVG& svg, int layer_id);

	GLuint Raster(float left, float right, float bottom, float top, int rows, int cols);

	std::vector<EarcutPolygon> earcut_polygons_;
	std::vector<DrawablePolygon> drawable_polygons_;

private:
	void DrawInOrder();// firstly draw contour, then draw hole.
	void DrawPolygon(DrawablePolygon& drawable_polygon, bool clear);
	void DrawMix(GLuint contour, GLuint hole);

	void Resize();
	void CreateQuadVAO();
	void UpdatePolygonsVAO();
	void CreateFBO();
	void CreateShader();
	void UpdateShader(float left, float right, float bottom, float top);


	Shader* shader_;
	Shader* mix_shader_;
	GLuint fbo_;
	glm::mat4 mvp_;
	GLuint quad_vao_;
	GLuint contour_tex_;
	GLuint hole_tex_;
	GLuint mask_tex_;

	float left_;
	float right_;
	float bottom_;
	float top_;

	int cols_ = 1000;
	int rows_ = 1000;





};



