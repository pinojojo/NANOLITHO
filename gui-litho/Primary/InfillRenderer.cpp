#include "InfillRenderer.h"

InfillRenderer::~InfillRenderer()
{
	glDeleteTextures(1, &infill_tex_);
	glDeleteFramebuffers(1, &infill_tex_);
}

void InfillRenderer::Init(float spacing_pixels, float filling_rate)
{
	spacing_pixels_ = spacing_pixels;

	filling_rate_ = filling_rate;

	Init();

}

void InfillRenderer::Draw(float anchor_x, float anchor_y, float pixel_size)
{
	pixel_size_ = pixel_size; anchor_x_ = anchor_x; anchor_y_ = anchor_y;

	UpdateVAO();

	// bind fbo
	glBindFramebuffer(GL_FRAMEBUFFER, infill_fbo_);
	glViewport(0, 0, res_x_, res_y_);
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	UpdateShader();

	glBindVertexArray(infill_vao_);
	glDrawArrays(GL_QUADS, 0, indices_number_);
	glBindVertexArray(0);

}

void InfillRenderer::DrawOffscreen(float anchor_x, float anchor_y, float pixel_size, std::string name)
{
	pixel_size_ = pixel_size; anchor_x_ = anchor_x; anchor_y_ = anchor_y;

	UpdateVAO();

	// bind fbo
	glBindFramebuffer(GL_FRAMEBUFFER, infill_fbo_);
	glViewport(0, 0, res_x_, res_y_);
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	UpdateShader();

	glBindVertexArray(infill_vao_);
	glDrawArrays(GL_QUADS, 0, indices_number_ * 2);
	glBindVertexArray(0);

	SaveFBO(infill_fbo_, name);

}

void InfillRenderer::DrawOffscreen(float anchor_x, float anchor_y, float pixel_size)
{
	pixel_size_ = pixel_size; anchor_x_ = anchor_x; anchor_y_ = anchor_y;

	UpdateVAO();

	// bind fbo
	glBindFramebuffer(GL_FRAMEBUFFER, infill_fbo_);
	glViewport(0, 0, res_x_, res_y_);
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	UpdateShader();

	glBindVertexArray(infill_vao_);
	glDrawArrays(GL_QUADS, 0, indices_number_ * 2);
	glBindVertexArray(0);

}

void InfillRenderer::Init()
{
	CreateFBO();
	MakeShader();
}

void InfillRenderer::Resize(int res_x, int res_y)
{
}

void InfillRenderer::UpdateVAO()
{
	// Get bounding box
	glm::vec2 window_size;
	glm::vec2 upper_left;


	window_size.x = pixel_size_ * res_x_;
	window_size.y = pixel_size_ * res_y_;

	upper_left.x = anchor_x_ - window_size.x * 0.2;
	upper_left.y = anchor_y_ + window_size.y * 0.2;


	// Get possible infill lines
	int upper_left_line_index_x;
	int upper_left_line_index_y;
	spacing_ = spacing_pixels_ * pixel_size_;

	upper_left_line_index_x = floor(upper_left.x / (spacing_));
	upper_left_line_index_y = floor(upper_left.y / (spacing_));

	int num_of_line_x = 1.4 * window_size.x / (spacing_);
	int num_of_line_y = 1.4 * window_size.y / (spacing_);


	// Generate Points for each line
	std::vector<float> _data;
	for (int line_index_x = 0; line_index_x < num_of_line_x; line_index_x++)
	{
		// vertical lines
		glm::vec2 q1 = glm::vec2((upper_left_line_index_x + line_index_x) * spacing_, upper_left.y);
		glm::vec2 q2 = glm::vec2((upper_left_line_index_x + line_index_x) * spacing_, upper_left.y - window_size.y * 1.4);
		glm::vec2 q3 = glm::vec2((upper_left_line_index_x + line_index_x) * spacing_ + filling_rate_ * spacing_, upper_left.y - window_size.y * 1.4);
		glm::vec2 q4 = glm::vec2((upper_left_line_index_x + line_index_x) * spacing_ + filling_rate_ * spacing_, upper_left.y);

		_data.push_back(q1.x);
		_data.push_back(q1.y);
		_data.push_back(q2.x);
		_data.push_back(q2.y);
		_data.push_back(q3.x);
		_data.push_back(q3.y);
		_data.push_back(q4.x);
		_data.push_back(q4.y);
	}
	for (int line_index_y = 0; line_index_y < num_of_line_y; line_index_y++)
	{
		// horizontal lines
		glm::vec2 q1 = glm::vec2(upper_left.x, (upper_left_line_index_y - line_index_y) * spacing_);
		glm::vec2 q2 = glm::vec2(upper_left.x + window_size.x * 1.4, (upper_left_line_index_y - line_index_y) * spacing_);
		glm::vec2 q3 = glm::vec2(upper_left.x + window_size.x * 1.4, (upper_left_line_index_y - line_index_y) * spacing_ - spacing_ * filling_rate_);
		glm::vec2 q4 = glm::vec2(upper_left.x, (upper_left_line_index_y - line_index_y) * spacing_ - spacing_ * filling_rate_);

		_data.push_back(q1.x);
		_data.push_back(q1.y);
		_data.push_back(q2.x);
		_data.push_back(q2.y);
		_data.push_back(q3.x);
		_data.push_back(q3.y);
		_data.push_back(q4.x);
		_data.push_back(q4.y);
	}

	indices_number_ = _data.size();



	if (!infill_vbo_)
	{
		glGenVertexArrays(1, &infill_vao_);
		glBindVertexArray(infill_vao_);

		// create vbo
		glGenBuffers(1, &infill_vbo_);
		glBindBuffer(GL_ARRAY_BUFFER, infill_vbo_);
		glBufferData(GL_ARRAY_BUFFER, _data.size() * sizeof(GL_FLOAT), &_data[0], GL_STATIC_DRAW);

		// vbo attrib
		glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
		glEnableVertexAttribArray(0);

		glBindVertexArray(0);
	}
	else
	{
		// not create
		glBindVertexArray(infill_vao_);;
		glBindBuffer(GL_ARRAY_BUFFER, infill_vbo_);
		glBufferData(GL_ARRAY_BUFFER, _data.size() * sizeof(GL_FLOAT), &_data[0], GL_STATIC_DRAW);

		// vbo attrib
		glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
		glEnableVertexAttribArray(0);

		glBindVertexArray(0);
	}



}

void InfillRenderer::MakeShader()
{
	infill_shader_ = new Shader("shader/infill/1.vert", "shader/infill/1.frag");
}

void InfillRenderer::UpdateShader()
{
	float left, right, bottom, top;
	left = anchor_x_;
	right = anchor_x_ + pixel_size_ * res_x_;
	bottom = anchor_y_ - pixel_size_ * res_y_;
	top = anchor_y_;

	glm::mat4 proj = glm::ortho(left, right, bottom, top, -1.f, 1.f);
	mvp_ = proj;

	infill_shader_->use();
	infill_shader_->setMat4("mvp", mvp_);
}

void InfillRenderer::CreateFBO()
{
	glGenFramebuffers(1, &infill_fbo_);
	glBindFramebuffer(GL_FRAMEBUFFER, infill_fbo_);
	// create a color attachment texture
	glGenTextures(1, &infill_tex_);
	glBindTexture(GL_TEXTURE_2D, infill_tex_);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, res_x_, res_y_, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, infill_tex_, 0);	// we only need a color buffer

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		std::cout << "ERROR::FRAMEBUFFER:: infill framebuffer is not complete!" << std::endl;
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void InfillRenderer::SaveFBO(GLuint fbo, std::string name)
{
	FILE* output_image;
	int     output_width, output_height;

	output_width = res_x_;
	output_height = res_y_;

	/// READ THE PIXELS VALUES from FBO AND SAVE TO A .PPM FILE
	int             i, j, k;
	std::vector<uint8_t> pixels(3 * output_width * output_height);

	/// READ THE CONTENT FROM THE FBO

	glReadBuffer(GL_COLOR_ATTACHMENT0);
	glReadPixels(0, 0, output_width, output_height, GL_RGB, GL_UNSIGNED_BYTE, pixels.data());

	// vertical flipping
	for (int line = 0; line != output_height / 2; ++line) {
		std::swap_ranges(pixels.begin() + 3 * output_width * line,
			pixels.begin() + 3 * output_width * (line + 1),
			pixels.begin() + 3 * output_width * (output_height - line - 1));
	}

	std::string path = "output/infill/" + name + ".ppm";

	output_image = fopen(path.c_str(), "wt");
	fprintf(output_image, "P3\n");
	fprintf(output_image, "# Created by lzx\n");
	fprintf(output_image, "%d %d\n", output_width, output_height);
	fprintf(output_image, "255\n");

	k = 0;
	for (i = 0; i < output_width; i++)
	{
		for (j = 0; j < output_height; j++)
		{
			fprintf(output_image, "%u %u %u ", (unsigned int)pixels[k], (unsigned int)pixels[k + 1],
				(unsigned int)pixels[k + 2]);
			k = k + 3;
		}
		fprintf(output_image, "\n");
	}


}
