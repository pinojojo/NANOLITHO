#include "BlendRenderer.h"

BlendRenderer::BlendRenderer()
{
}

void BlendRenderer::Init()
{
	CreateFBO();
	CreateShader();
	CreateQuadVAO();
	
}

GLuint BlendRenderer::Blend(GLuint infill_tex, GLuint stroke_tex, GLuint mask_tex, int cols, int rows)
{
	// resize
	if ((cols!=cols_)||(rows!=rows_))
	{
		
	}

	// bind fbo
	glBindFramebuffer(GL_FRAMEBUFFER, fbo_);
	glBindTexture(GL_TEXTURE_2D, tex_);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex_, 0);	// we only need a color buffer
	glViewport(0, 0, cols_, rows_);

	// clear
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);
	glDisable(GL_DEPTH_TEST);

	// bind quad vao
	glBindVertexArray(quad_vao_);

	// bind & update shader 
	shader_->use();
	shader_->bindTextureUnit("infill_tex", 0);
	shader_->bindTextureUnit("stroke_tex", 1);
	shader_->bindTextureUnit("mask_tex", 2);

	// active & bind texture
	glActiveTexture(GL_TEXTURE0 + 0); glBindTexture(GL_TEXTURE_2D, infill_tex);
	glActiveTexture(GL_TEXTURE0 + 1); glBindTexture(GL_TEXTURE_2D, stroke_tex);
	glActiveTexture(GL_TEXTURE0 + 2); glBindTexture(GL_TEXTURE_2D, mask_tex);

	// draw call
	glDrawArrays(GL_TRIANGLES, 0, 6);

	// unbind 
	glBindVertexArray(0);
	glBindTexture(GL_TEXTURE_2D, 0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);


	return tex_;
}

void BlendRenderer::CreateShader()
{
	shader_ = new Shader("shader/blend/1.vert", "shader/blend/1.frag");

}

void BlendRenderer::CreateFBO()
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
		std::cout << "ERROR::FRAMEBUFFER:: framebuffer is not complete!" << std::endl;
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void BlendRenderer::CreateQuadVAO()
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
