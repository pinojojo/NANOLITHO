#include "InfillRenderer.h"

InfillRenderer::InfillRenderer()
{
}

InfillRenderer::~InfillRenderer()
{
}

void InfillRenderer::Init(litho::LithoSetting setting)
{
	spacing_ = setting.infill_grid_spacing_internal;
	fill_ratio_ = setting.infill_rate;
	pixel_size_ = setting.pixel_size_internal;

	CreateFBO();
	CreateShader();
}

GLuint InfillRenderer::Raster(float left, float right, float bottom, float top, int rows, int cols)
{
	left_ = left;
	right_ = right;
	bottom_ = bottom;
	top_ = top;

	// create vao
	CreateVAO(left, right, bottom, top);

	// resize 
	if ((rows != rows_) || (cols != cols_))
	{
		
	}

	// bind fbo
	glBindFramebuffer(GL_FRAMEBUFFER, fbo_);
	glViewport(0, 0, cols_, rows_);
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// update shader
	UpdateShader();

	// bind vao
	glBindVertexArray(vao_);

	// draw call
	glDrawArrays(GL_QUADS, 0, infill_vertices_.size());

	// unbind
	glBindVertexArray(0);
	glUseProgram(0);



	return tex_;
}

void InfillRenderer::CreateShader()
{
	shader_ = new Shader("shader/infill/1.vert", "shader/infill/1.frag");
}

void InfillRenderer::UpdateShader()
{
	glm::mat4 proj = glm::ortho(left_, right_, bottom_, top_, -1.f, 1.f);
	mvp_ = proj;

	shader_->use();
	shader_->setMat4("mvp", mvp_);
}

void InfillRenderer::CreateVAO(float left, float right, float bottom, float top)
{
	// infill pattern origin at (0,0)
	float size_x = right - left;
	float size_y = top - bottom;

	//std::cout << "a: " << size_x << " b: " << size_y << std::endl;

	float pattern_left = left - size_x * 0.4;
	float pattern_right = right + size_x * 0.4;
	float pattern_bottom = bottom - size_y * 0.4;
	float pattern_top = top + size_y * 0.4;

	int line_id_left = 0;
	int line_id_right = 0;
	int line_id_bottom = 0;
	int line_id_top = 0;

	line_id_left = ceil(pattern_left / spacing_);
	line_id_right = ceil(pattern_right / spacing_);
	line_id_bottom = ceil(pattern_bottom / spacing_);
	line_id_top = ceil(pattern_top / spacing_);

	infill_vertices_.clear();
	for (int line_id_x = line_id_left; line_id_x < line_id_right; line_id_x++)
	{
		glm::vec2 quad[4];
		quad[0] = glm::vec2(line_id_x * spacing_, pattern_top);
		quad[1] = glm::vec2(line_id_x * spacing_, pattern_bottom);
		quad[2] = glm::vec2(line_id_x * spacing_ + spacing_ * fill_ratio_, pattern_bottom);
		quad[3] = glm::vec2(line_id_x * spacing_ + spacing_ * fill_ratio_, pattern_top);

		for (size_t i = 0; i < 4; i++)
		{
			infill_vertices_.push_back(quad[i]);
		}
	}
	for (int line_id_y = line_id_bottom; line_id_y < line_id_top; line_id_y++)
	{
		glm::vec2 quad[4];
		quad[0] = glm::vec2(pattern_left, line_id_y * spacing_);
		quad[1] = glm::vec2(pattern_right, line_id_y * spacing_);
		quad[2] = glm::vec2(pattern_right, line_id_y * spacing_ + spacing_ * fill_ratio_);
		quad[3] = glm::vec2(pattern_left, line_id_y * spacing_ + spacing_ * fill_ratio_);
		for (size_t i = 0; i < 4; i++)
		{
			infill_vertices_.push_back(quad[i]);
		}
	}

	// if exsit already, delete first
	if (vao_)
	{
		// delete
		glDeleteBuffers(1, &vbo_);
		glDeleteVertexArrays(1, &vao_);
	}

	// Create VAO
	glGenVertexArrays(1, &vao_);
	glBindVertexArray(vao_);

	// create vbo
	glGenBuffers(1, &vbo_);
	glBindBuffer(GL_ARRAY_BUFFER, vbo_);
	glBufferData(GL_ARRAY_BUFFER, infill_vertices_.size() * 2 * sizeof(GL_FLOAT), &infill_vertices_[0].x, GL_STATIC_DRAW);

	// vbo attrib
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);

	glBindVertexArray(0);
}

void InfillRenderer::CreateFBO()
{
	glGenFramebuffers(1, &fbo_);
	glBindFramebuffer(GL_FRAMEBUFFER, fbo_);
	// create a color attachment texture
	glGenTextures(1, &tex_);
	glBindTexture(GL_TEXTURE_2D, tex_);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, cols_, rows_, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex_, 0);	// we only need a color buffer

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		std::cout << "ERROR::FRAMEBUFFER:: infill framebuffer is not complete!" << std::endl;
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}
