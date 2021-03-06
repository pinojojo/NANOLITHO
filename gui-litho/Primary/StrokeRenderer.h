#pragma once
#include "LithoType.h"
#include "LithoSVG.h"


#define GLM_FORCE_RADIANS
#include <GL/glew.h>
#include <gl/GL.h>

#include "shader.h"


#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <vector>
#include <string>



class StrokeRenderer
{
public:
	StrokeRenderer();
	~StrokeRenderer();

	void Init(float thickness_pixels, float pixel_size);
	void Init(litho::LithoSetting setting);

	void UpdateLayer(litho::LithoSVG& svg, int layer_id);

	GLuint Raster(float left, float right, float bottom, float top, int rows, int cols);

private:
	void CreateGL();
	void ReSize();
	void GenerateStrokeVAO();
	void CreateShader();
	void CreateFBO();
	void UpdateShader(float left,float right,float bottom,float top);
	void CalcStrokeQuad(glm::vec2& curr, glm::vec2& last, glm::vec2& next, glm::vec2& intersection_first, glm::vec2& intersection_second, float thickness);
	void SaveFBO(GLuint fbo, std::string name);

	float shell_thickness_;
	float pixel_size_;

	std::vector<std::vector<glm::vec2>> polygons_;
	std::vector<glm::vec2> strokes_data_;

	GLuint  stroke_vao_ = 0;
	GLuint  stroke_vbo_ = 0;
	GLuint  stroke_fbo_ = 0;
	GLuint  stroke_tex_ = 0;
	Shader* stroke_shader_ = NULL;
	glm::mat4 mvp_;

	glm::vec2 anchor_;
	glm::vec2 center_ = glm::vec2(0);


	int cols_ = 1000;
	int rows_ = 1000;


};

