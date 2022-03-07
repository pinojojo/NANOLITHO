#pragma once
#define GLM_FORCE_RADIANS
#include "LithoType.h"
#include <GL/glew.h>
#include <gl/GL.h>

#include "shader.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <vector>
#include <string>
#include <algorithm>

enum PatternMode {
	square,
	hexagon
};

class InfillRenderer
{
public:
	InfillRenderer();
	~InfillRenderer();

	void Init(litho::LithoSetting setting);

	GLuint Raster(float left, float right, float bottom, float top, int rows, int cols);

private:
	void CreateShader();
	void UpdateShader();
	void CreateVAO(float left, float right, float bottom, float top);
	void CreateFBO();

	int cols_ = 1000;
	int rows_ = 1000;

	Shader* shader_;
	GLuint vao_=0;
	GLuint vbo_=0;
	GLuint fbo_;
	GLuint tex_;
	float spacing_;
	float fill_ratio_;
	glm::mat4 mvp_;

	float left_;
	float right_;
	float bottom_;
	float top_;

	float pixel_size_;

	std::vector<glm::vec2> infill_vertices_;
	
};

