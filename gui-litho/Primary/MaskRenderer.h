#pragma once
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

class MaskRenderer
{
public:
	MaskRenderer(){}

	void Init(glm::vec2 anchor, float pixel_size);
	
	void UpdateWindow(glm::vec2 anchor);

	void Draw();

private:
	void Init();
	void Triangulation();
	void UpdateVAO();
	void CreateFBO();
	void CreateShader();
	void UpdateShader();
	

	Shader* shader_;
	GLuint vao;
	GLuint vbo;
	GLuint ebo;

	GLuint tex_;
	GLuint fbo_;

	glm::vec2 anchor_;
	float pixel_size_;

	int res_x = 1000;
	int res_y = 1000;
	
	



};

