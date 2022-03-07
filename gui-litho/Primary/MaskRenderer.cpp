#include "MaskRenderer.h"

#include "mapbox/earcut.hpp"
void MaskRenderer::Init(litho::LithoSetting setting)
{
	cols_ = setting.strip_width;
	rows_ = setting.block_height;

	CreateFBO();
	CreateQuadVAO();
	CreateShader();
}

void MaskRenderer::UpdateLayer(litho::LithoSVG& svg, int layer_id)
{
	using namespace ear;
	// 1. get layer
	auto& layer = svg.data_[layer_id];

	earcut_polygons_.clear();
	// 2. copy layer to earcut data
	for (auto& polygon : layer.polygons)
	{
		EarcutPolygon curr_e_polygon;
		std::vector<Point> curr_polygon;
		for (auto& point : polygon.points)
		{
			Point curr_point;
			curr_point[0] = point.x;
			curr_point[1] = point.y;
			curr_polygon.push_back(curr_point);
		}


		curr_e_polygon.polygons.push_back(curr_polygon);
		curr_e_polygon.is_hole = polygon.is_hole;
		
		earcut_polygons_.push_back(curr_e_polygon);
	}

	// 3. use earcut to triangulation
	for (auto& e_polygon:earcut_polygons_)
	{
		e_polygon.indices = mapbox::earcut<N>(e_polygon.polygons);
	}
	
	// 4. convert to drawalble data
	drawable_polygons_.clear();
	for (auto& e_polygon : earcut_polygons_)
	{
		DrawablePolygon curr_d_polygon;

		// polygon type
		curr_d_polygon.is_hole = e_polygon.is_hole;

		// point data
		for (auto& p : e_polygon.polygons[0])
		{
			curr_d_polygon.points.push_back(glm::vec2(p[0], p[1]));
		}
		 
		// index data
		for (auto& index:e_polygon.indices)
		{
			curr_d_polygon.indices.push_back((unsigned int)index);
		}

		drawable_polygons_.push_back(curr_d_polygon);
	}

	// 5. make vao
	UpdatePolygonsVAO();


}

GLuint MaskRenderer::Raster(float left, float right, float bottom, float top, int rows, int cols)
{
	left_ = left;
	right_ = right;
	bottom_ = bottom;
	top_ = top;

	// resize 
	if ((rows != rows_) || (cols != cols_))
	{
		Resize();
	}

	// draw in order
	DrawInOrder();

	return mask_tex_;
}

void MaskRenderer::DrawInOrder()
{
	// draw contour first
	bool need_clear = true;
	for (auto& drawable_polygon : drawable_polygons_)
	{
		if (!drawable_polygon.is_hole)
		{
			DrawPolygon(drawable_polygon, need_clear);
			need_clear = false;
		}
	}

	// draw hole 
	need_clear = true;
	for (auto& drawable_polygon : drawable_polygons_)
	{
		if (drawable_polygon.is_hole)
		{
			DrawPolygon(drawable_polygon, need_clear);
			need_clear = false;
		}
	}

	// mixing hole on contour
	DrawMix(contour_tex_, hole_tex_);

}

void MaskRenderer::DrawPolygon(DrawablePolygon& drawable_polygon, bool clear)
{

	// bind fbo with corrrect texture
	if (drawable_polygon.is_hole)
	{
		glBindFramebuffer(GL_FRAMEBUFFER, fbo_);
		glBindTexture(GL_TEXTURE_2D, hole_tex_);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, hole_tex_, 0);	// we only need a color buffer
		glViewport(0, 0, cols_, rows_);
	}
	else
	{
		glBindFramebuffer(GL_FRAMEBUFFER, fbo_);
		glBindTexture(GL_TEXTURE_2D, contour_tex_);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, contour_tex_, 0);	// we only need a color buffer
		glViewport(0, 0, cols_, rows_);
	}

	// clear fbo
	if (clear)
	{
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	}

	// update shader
	UpdateShader(left_, right_, bottom_, top_);

	// bind vao
	glBindVertexArray(drawable_polygon.vao);

	// draw call
	glDrawElements(GL_TRIANGLES, drawable_polygon.indices.size(), GL_UNSIGNED_INT, nullptr);

	glBindVertexArray(0);
	glUseProgram(0);
}

void MaskRenderer::DrawMix(GLuint contour, GLuint hole)
{
	// bind fbo
	glBindFramebuffer(GL_FRAMEBUFFER, fbo_);
	glBindTexture(GL_TEXTURE_2D, mask_tex_);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, mask_tex_, 0);	// we only need a color buffer
	glViewport(0, 0, cols_, rows_);

	// clear
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);
	glDisable(GL_DEPTH_TEST);

	// bind quad vao
	glBindVertexArray(quad_vao_);

	// bind & update shader 
	mix_shader_->use();
	mix_shader_->bindTextureUnit("contour_tex", 0);
	mix_shader_->bindTextureUnit("hole_tex", 1);

	// active & bind texture
	glActiveTexture(GL_TEXTURE0 + 0); glBindTexture(GL_TEXTURE_2D, contour);
	glActiveTexture(GL_TEXTURE0 + 1); glBindTexture(GL_TEXTURE_2D, hole);

	// draw call
	glDrawArrays(GL_TRIANGLES, 0, 6);

	// unbind 
	glBindVertexArray(0);
	glBindTexture(GL_TEXTURE_2D, 0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void MaskRenderer::Resize()
{
}

void MaskRenderer::CreateQuadVAO()
{
	float quad_vertices[] = {   // vertex attributes for a quad that fills the entire screen in Normalized Device Coordinates.
		// positions   // texCoords
		-1.0f,  1.0f,  0.0f, 1.0f,
		-1.0f, -1.0f,  0.0f, 0.0f,
		 1.0f, -1.0f,  1.0f, 0.0f,

		-1.0f,  1.0f,  0.0f, 1.0f,
		 1.0f, -1.0f,  1.0f, 0.0f,
		 1.0f,  1.0f,  1.0f, 1.0f
	};

	unsigned int quad_vbo;
	glGenVertexArrays(1, &quad_vao_);
	glGenBuffers(1, &quad_vbo);
	glBindVertexArray(quad_vao_);
	glBindBuffer(GL_ARRAY_BUFFER, quad_vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(quad_vertices), &quad_vertices, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
}

void MaskRenderer::UpdatePolygonsVAO()
{
	for (auto& d_polygon : drawable_polygons_)
	{
		// create vao
		glGenVertexArrays(1, &d_polygon.vao);
		glBindVertexArray(d_polygon.vao);

		// create vbo
		glGenBuffers(1, &d_polygon.vbo);
		glBindBuffer(GL_ARRAY_BUFFER, d_polygon.vbo);
		glBufferData(GL_ARRAY_BUFFER, 2 * d_polygon.points.size() * sizeof(GL_FLOAT), &d_polygon.points[0].x, GL_STATIC_DRAW);

		// vbo attrib
		glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
		glEnableVertexAttribArray(0);

		// create ebo
		glGenBuffers(1, &d_polygon.ebo);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, d_polygon.ebo);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, d_polygon.indices.size() * sizeof(unsigned int), &d_polygon.indices[0], GL_STATIC_DRAW);

		glBindVertexArray(0);
	}
}

void MaskRenderer::CreateFBO()
{
	glGenFramebuffers(1, &fbo_);
	glBindFramebuffer(GL_FRAMEBUFFER, fbo_);
	// create a color attachment texture
	glGenTextures(1, &contour_tex_);
	glBindTexture(GL_TEXTURE_2D, contour_tex_);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, cols_, rows_, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glGenTextures(1, &hole_tex_);
	glBindTexture(GL_TEXTURE_2D, hole_tex_);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, cols_, rows_, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glGenTextures(1, &mask_tex_);
	glBindTexture(GL_TEXTURE_2D, mask_tex_);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, cols_, rows_, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, contour_tex_, 0);	// we only need a color buffer

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		std::cout << "ERROR::FRAMEBUFFER:: framebuffer is not complete!" << std::endl;
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

}

void MaskRenderer::CreateShader()
{
	shader_ = new Shader("shader/mask/1.vert", "shader/mask/1.frag");
	mix_shader_ = new Shader("shader/mask/mix.vert", "shader/mask/mix.frag");
}

void MaskRenderer::UpdateShader(float left, float right, float bottom, float top)
{

	glm::mat4 proj = glm::ortho(left, right, bottom, top, -1.f, 1.f);
	mvp_ = proj;

	shader_->use();
	shader_->setMat4("mvp", mvp_);

}

