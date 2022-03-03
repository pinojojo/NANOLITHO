#pragma once
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
#include <algorithm>

enum PatternMode {
	square,
	hexagon
};

class InfillRenderer
{
public:
	InfillRenderer() {}
	InfillRenderer(float spacing_pixels) :spacing_pixels_(spacing_pixels) { Init(); }
	InfillRenderer(float spacing_pixels, float filling_rate) :spacing_pixels_(spacing_pixels), filling_rate_(filling_rate) { Init(); }
	~InfillRenderer();

	void Init(float spacing_pixels, float filling_rate);

	void Draw(float anchor_x, float anchor_y, float pixel_size);
	void DrawOffscreen(float anchor_x, float anchor_y, float pixel_size, std::string name);
	void DrawOffscreen(float anchor_x, float anchor_y, float pixel_size);


private:
	void Init();
	void Resize(int res_x,int res_y);  //NOTE: resizing ops always very time-consuming, so take attention to use this function.
	void UpdateVAO();
	void MakeShader();
	void UpdateShader();
	void CreateFBO();
	void SaveFBO(GLuint fbo,std::string name);

	int    res_x_ = 1000, res_y_ = 1000;
	GLuint infill_fbo_;
	GLuint infill_tex_=0;
	GLuint infill_vao_=0;
	GLuint infill_vbo_=0;
	Shader* infill_shader_;
	glm::mat4 mvp_;

	
	float pixel_size_;
	float spacing_; 
	float spacing_pixels_;//number of pixels
	float filling_rate_ = 1.0;
	PatternMode pattern_mode_ = square;
	float anchor_x_;
	float anchor_y_;

	int indices_number_;





	
};

