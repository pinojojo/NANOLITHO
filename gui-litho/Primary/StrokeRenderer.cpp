#include "StrokeRenderer.h"



StrokeRenderer::StrokeRenderer()
{
}

StrokeRenderer::~StrokeRenderer()
{

	glDeleteBuffers(1, &stroke_vbo_);
	glDeleteVertexArrays(1, &stroke_vao_);
}

void StrokeRenderer::Init(float thickness_pixels, float pixel_size)
{
	thickness_pixels_ = thickness_pixels;
	pixel_size_ = pixel_size;
	CreateGL();
}

void StrokeRenderer::UpdateLayer(litho::LithoSVG& svg, int layer_id)
{
	if (layer_id<svg.data_.size())
	{
		polygons_.clear();
		auto& layer = svg.data_[layer_id];
		for (auto& polygon:layer.polygons)
		{
			std::vector<glm::vec2> curr_polygon;
			for (auto& point:polygon.points)
			{
				curr_polygon.push_back(point);
			}
			polygons_.push_back(curr_polygon);
		}
	}

	center_ = svg.GetCenter();

	if (stroke_vao_)
	{
		glDeleteBuffers(1, &stroke_vbo_);
		glDeleteVertexArrays(1, &stroke_vao_);
		GenerateStrokeVAO();
	}
	else
	{
		GenerateStrokeVAO();
	}


}

// Use coordinate system for print space
void StrokeRenderer::DrawOffscreen(float anchor_x, float anchor_y, float pixel_size, std::string name)
{
	DrawOffscreen(anchor_x, anchor_y, pixel_size);
	SaveFBO(stroke_fbo_, name);
}

GLuint StrokeRenderer::Raster(float left, float right, float bottom, float top, int rows, int cols)
{
	// resize 
	if ((rows!=rows_)||(cols!=cols_))
	{
		ReSize();
	}

	// bind fbo
	glBindFramebuffer(GL_FRAMEBUFFER, stroke_fbo_);
	glViewport(0, 0, cols_, rows_);
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// update shader
	UpdateShader(left, right, bottom,top );

	// bind vao
	glBindVertexArray(stroke_vao_);

	// draw call
	glDrawArrays(GL_QUADS, 0, strokes_data_.size());

	// unbind
	glBindVertexArray(0);
	glUseProgram(0);
	
	return stroke_tex_;
}

void StrokeRenderer::CalcStrokeQuad(glm::vec2& curr, glm::vec2& last, glm::vec2& next, glm::vec2& intersection_first, glm::vec2& intersection_second, float thickness)
{
	// normlized vector curr-> next 
	glm::vec2 to_next;
	to_next = glm::normalize(next - curr);

	// normlized vector curr-> last 
	glm::vec2 to_last;
	to_last = glm::normalize(last - curr);

	// normalized vector 
	glm::vec2 to_quad = (to_next + to_last); // may be zero vector, when normalized func being used,must notice its arguments not be zero 
	float checkZeroVector = glm::length(to_quad);
	if (checkZeroVector < 0.000001)
	{
		to_quad = glm::normalize(glm::vec2(-to_next.y, to_next.x));
	}
	else {
		to_quad = glm::normalize(to_quad);
	}


	// check left side or right side
	float checkLeft = to_next.x * to_quad.y - to_next.y * to_quad.x;
	if (checkLeft < 0)
		to_quad = -to_quad;

	// left quad point 
	glm::vec2 normOfNext = glm::normalize(glm::vec2(-to_next.y, to_next.x));
	float cosValue = abs(normOfNext.x * to_quad.x + normOfNext.y * to_quad.y);
	float to_quadLength = thickness / cosValue;
	to_quad = to_quadLength * to_quad;

	// quad point this node
	intersection_first = curr + to_quad;
	intersection_second = curr - to_quad;
}

void StrokeRenderer::SaveFBO(GLuint fbo, std::string name)
{
	FILE* output_image;
	int     output_width, output_height;

	output_width = cols_;
	output_height = rows_;

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

	std::string path = "output/stroke/" + name + ".ppm";

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

void StrokeRenderer::CreateGL()
{
	CreateFBO();
	CreateShader();
}

void StrokeRenderer::ReSize()
{
	// recreate opengl stuff
	

}

void StrokeRenderer::GenerateStrokeVAO()
{
	strokes_data_.clear();

	for (auto& ring : polygons_)
	{
		glm::vec2 last_intersection_first;
		glm::vec2 last_intersection_second;
		glm::vec2 head_intersection_first;
		glm::vec2 head_intersection_second;

		for (auto& pt : ring)
		{
			glm::vec2 curr, next, last, intersection_first, intersection_second;

			if (&pt == &ring.front())	   // case a: curr is head  
			{
				curr = pt;
				next = *(&pt + 1);
				last = ring.back();
			}
			else if (&pt == &ring.back())  // case b: curr is tail
			{
				curr = pt;
				next = ring.front();
				last = *(&pt - 1);
			}
			else						   // case c: curr in between 
			{
				curr = pt;
				next = *(&pt + 1);
				last = *(&pt - 1);
			}

			CalcStrokeQuad(curr, last, next, intersection_first, intersection_second, thickness_pixels_ * pixel_size_);
			
			if (&pt != &ring.front())
			{
				// make a quad
				strokes_data_.push_back(last_intersection_first);
				strokes_data_.push_back(last_intersection_second);
				strokes_data_.push_back(intersection_second);
				strokes_data_.push_back(intersection_first);
			}
			else
			{
				//
				head_intersection_first = intersection_first;
				head_intersection_second = intersection_second;
			}

			last_intersection_first = intersection_first;
			last_intersection_second = intersection_second;
		}

		// quad between head and tail
		strokes_data_.push_back(last_intersection_first);
		strokes_data_.push_back(last_intersection_second);
		strokes_data_.push_back(head_intersection_second);
		strokes_data_.push_back(head_intersection_first);
	}

	// Create VAO
	glGenVertexArrays(1, &stroke_vao_);
	glBindVertexArray(stroke_vao_);

	// create vbo
	glGenBuffers(1, &stroke_vbo_);
	glBindBuffer(GL_ARRAY_BUFFER, stroke_vbo_);
	glBufferData(GL_ARRAY_BUFFER, strokes_data_.size() * 2 * sizeof(GL_FLOAT), &strokes_data_[0].x, GL_STATIC_DRAW);
	
	// vbo attrib
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);

	glBindVertexArray(0);
}

void StrokeRenderer::CreateShader()
{
	stroke_shader_ = new Shader("shader/stroke/1.vert", "shader/stroke/1.frag");
}

void StrokeRenderer::CreateFBO()
{
	glGenFramebuffers(1, &stroke_fbo_);
	glBindFramebuffer(GL_FRAMEBUFFER, stroke_fbo_);
	// create a color attachment texture
	glGenTextures(1, &stroke_tex_);
	glBindTexture(GL_TEXTURE_2D, stroke_tex_);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, cols_, rows_, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, stroke_tex_, 0);	// we only need a color buffer

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		std::cout << "ERROR::FRAMEBUFFER:: infill framebuffer is not complete!" << std::endl;
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void StrokeRenderer::UpdateShader(float left, float right, float bottom, float top)
{
	

	glm::mat4 proj = glm::ortho(left, right, bottom, top, -1.f, 1.f);
	mvp_ = proj;

	stroke_shader_->use();
	stroke_shader_->setMat4("mvp", mvp_);
}



