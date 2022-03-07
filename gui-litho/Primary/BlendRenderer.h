#pragma once

#define GLM_FORCE_RADIANS
#include <GL/glew.h>
#include <gl/GL.h>

#include "shader.h"


class BlendRenderer
{
public:
	BlendRenderer();

	void Init();
	
	GLuint Blend(GLuint infill_tex, GLuint stroke_tex, GLuint mask_tex,int cols,int rows);
	
private:
	void CreateShader();
	void CreateFBO();
	void CreateQuadVAO();
	


	GLuint tex_;
	Shader* shader_;
	GLuint fbo_;
	GLuint quad_vao_;

	int rows_ = 1000;
	int cols_ = 1000;
	
	
};

