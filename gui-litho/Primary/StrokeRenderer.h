#pragma once
#define GLM_FORCE_RADIANS
#include <GL/glew.h>
#include <gl/GL.h>

#include "shader.h"
#include "LithoModel.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <vector>
#include <string>



class StrokeRenderer
{
public:
	StrokeRenderer(float thickness_pixels,float pixel_size):thickness_pixels_(thickness_pixels),pixel_size_(pixel_size){}
	~StrokeRenderer();

	void UpdatePolygonsData(LithoModel& model,int layer_id);
	void DrawOffscreen(float anchor_x, float anchor_y, float pixel_size);
	void DrawOffscreen(float anchor_x, float anchor_y, float pixel_size,std::string name);

private:
	void GenerateStrokeVAO();
	void MakeShader();
	void UpdateShader();
	void CalcStrokeQuad(glm::vec2& curr, glm::vec2& last,glm::vec2& next, glm::vec2& intersection_first, glm::vec2& intersection_second, float thickness);
	
	float thickness_pixels_;
	float pixel_size_;

	std::vector<std::vector<glm::vec2>> polygons_;
	std::vector<glm::vec2> strokes_data_;

	GLuint  stroke_vao_;
	GLuint  stroke_vbo_;
	GLuint  stroke_fbo_;
	GLuint  stroke_tex_;
	Shader* stroke_shader_;
	glm::mat4 mvp_;

	

	
	
	
	
	
};

