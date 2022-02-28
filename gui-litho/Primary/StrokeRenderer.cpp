#include "StrokeRenderer.h"

StrokeRenderer::~StrokeRenderer()
{

	glDeleteBuffers(1, &stroke_vao_);
	glDeleteBuffers(1, &stroke_vbo_);
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

void StrokeRenderer::GenerateStrokeData()
{
	for (auto& ring : polygons_)
	{
		std::vector<glm::vec2> curr_stroke;
		
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
			
			curr_stroke.push_back(intersection_first);
			curr_stroke.push_back(intersection_second);
		}

		strokes_data_.push_back(curr_stroke);

		

	}
}



