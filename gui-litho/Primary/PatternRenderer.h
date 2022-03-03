#pragma once
#define GLM_FORCE_RADIANS
#include <GL/glew.h>
#include <gl/GL.h>

#include "shader.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>

class LayerPatternRenderer
{

public:
	

	void DrawPatternOffscreen(GLuint infill_tex,GLuint stroke_tex_,GLuint mask_tex_);

	GLuint GetTextureID() { return pattern_tex_; }

private:
	int layer_id;

	GLuint pattern_tex_;
	Shader* pattern_shader_;


	
	

};

