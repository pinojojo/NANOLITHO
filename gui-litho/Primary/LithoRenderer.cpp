#include "LithoRenderer.h"


void LithoRenderer::Init()
{
	m_Polygon.push_back({ { 0.8, -0.3 }, { 0.9, 0.9 }, { -0.3, 0.8 }, { -0.3, 0} });
	//// Following polylines define holes.
	m_Polygon.push_back({ { 0.4, 0.2 }, { 0.4, 0.4 }, {0.2, 0.4 }, { 0.2,0.2} });

	m_Indices = mapbox::earcut<N>(m_Polygon);




	if (1)
	{
		for (auto ring : m_Polygon)
		{
			for (auto point : ring)
			{
				//std::cout << point[0] << " " << point[1] << std::endl;

				m_PolygonDumb.push_back((float)point[0]);
				m_PolygonDumb.push_back((float)point[1]);
			}
		}

		for (auto m : m_Indices)
		{
			m_IndicesDumb.push_back((unsigned int)m);
		}




	}

	std::cout << "vertices size" << m_PolygonDumb.size() << std::endl;

	std::cout << "indices size" << m_IndicesDumb.size() << std::endl;

	MakeThickPolygonVAO();
	MakeTriangulatedVAO();
	MakeInfillVAO();
	MakePostAAVAO();
	MakeBlendVAO();

	MakeAAShader();
	MakePostAAShader();
	MakeBlendShader();

	CreateAAFBO();

	MakeInfillFBO();

	MakeMaskFBO();

	// init line mode
	{
		PrepareLineMode();
		MakeLineModeVAO();
	}


	PrepareOffScreen(m_TextureThickPolygon, m_FBOThickPolygon);
	

}

void LithoRenderer::RemakePolygonVAO(LithoModel& model, int layer, int polygon)
{
	// clear old data.
	m_Polygon.clear();

	// convert svg-polygon to m_Polygon.
	auto& svgPolygon = model.m_Layers[layer].polygons[polygon];
	std::vector<Point> aRing;
	for (auto& pt : svgPolygon.points)
	{
		Point aPoint;
		aPoint[0] = pt.x;
		aPoint[1] = pt.y;
		aRing.push_back(aPoint);
	}
	m_Polygon.push_back(aRing);

	// triangulation with ear cut, then we got the indices of trianles.
	if (true)
	{
		m_Indices = mapbox::earcut<N>(m_Polygon);
	}

	// make vao from new polygon data.
	MakeThickPolygonVAO();
	MakeLineModeVAO();
	MakeTriangulatedVAO();

}

void LithoRenderer::RemakeInfillVAO(float lineWidth, float lineGap, int lineNum)
{
	m_InfillLineWidth = lineWidth;
	m_InfillLineGap = lineGap;
	m_InfillSize = lineNum * lineGap;

	MakeInfillVAO();

}

glm::vec2 LithoRenderer::GetCenter()
{
	return glm::vec2(m_CenterX,m_CenterY);
}



void LithoRenderer::SetPixelSize(float pixelSize)
{
	m_PixelSize = pixelSize;
}

void LithoRenderer::SetPatternRotation(float rotDeg)
{
	m_InfillPatternRotation = glm::radians(rotDeg);
}

void LithoRenderer::SetLineWidth(float lineWidth)
{
	m_ThickPolygonWidth = lineWidth;

	MakeThickPolygonVAO();
}

void LithoRenderer::ShiftHorizontal(int pixelNumber)
{
	m_CenterX += pixelNumber * m_PixelSize;

}

void LithoRenderer::ShiftVertical(int pixelNumber)
{
	m_CenterY += pixelNumber * m_PixelSize;
}

void LithoRenderer::MoveByPixel(int x, int y,bool release)
{
	m_CenterX = m_CenterXLast -x * m_PixelSize;
	m_CenterY = m_CenterYLast+ y * m_PixelSize;
	if (release)
	{
		
		m_CenterXLast = m_CenterX;
		m_CenterYLast = m_CenterY;
		std::cout << "changed" << std::endl;

	}

	

}

void LithoRenderer::SaveOnceMask()
{
	m_SaveFlag = true;
}

void LithoRenderer::SaveOnceInfill()
{
	m_SaveInfillFalg = true;
}

void LithoRenderer::RenderSpecificMode(int mode)
{
	switch (mode)
	{
	case -1:
		break;
	case 0:
	{
		DrawLineMode();
	}
	break;
	case 1:
	{
		DrawThickPolygon();
	}
	break;
	case 2:
	{
		DrawTriangulatedPolygonAA();
	}
	break;
	case 3:
	{
		DrawInfill();
	}
	break;
	case 4:
	{
		DrawBlend();

	}
	break;
	}
}

void LithoRenderer::DrawPolygon()
{



	glViewport(0, 0, m_WinWidth, m_WinHeight);

	glScissor(0, 0, m_WinWidth, m_WinHeight);
	glEnable(GL_SCISSOR_TEST);
	glClearColor(0.0f, 0.0f, 1.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);



	m_Shader->use();

	glBindVertexArray(m_TriangulatedVAO);

	//glDrawArrays(GL_TRIANGLES, 0, 3);
	glDrawElements(GL_TRIANGLES, m_Indices.size(), GL_UNSIGNED_INT, nullptr);

	glBindVertexArray(0);
}

void LithoRenderer::MakeTriangulatedVAO()
{
	// convert raw data to dumb data for glbufferdata
	m_PolygonDumb.clear();
	m_IndicesDumb.clear();
	{
		for (auto ring : m_Polygon)
		{
			for (auto point : ring)
			{
				m_PolygonDumb.push_back((float)point[0]);
				m_PolygonDumb.push_back((float)point[1]);
			}
		}

		for (auto m : m_Indices)
		{
			m_IndicesDumb.push_back((unsigned int)m);
		}
	}
	

	// create vao
	glGenVertexArrays(1, &m_TriangulatedVAO);
	glBindVertexArray(m_TriangulatedVAO);

	// create vbo
	GLuint vboVertices;
	glGenBuffers(1, &vboVertices);
	glBindBuffer(GL_ARRAY_BUFFER, vboVertices);
	glBufferData(GL_ARRAY_BUFFER, m_PolygonDumb.size() * sizeof(GL_FLOAT), &m_PolygonDumb[0], GL_STATIC_DRAW);

	// vbo attrib
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);

	// create ebo to indexing
	glGenBuffers(1, &m_TriangulatedEBO);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_TriangulatedEBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, m_IndicesDumb.size() * sizeof(unsigned int), &m_IndicesDumb[0], GL_STATIC_DRAW);

	glBindVertexArray(0);

}

void LithoRenderer::MakePostAAVAO()
{
	float quadVertices[] = {   // vertex attributes for a quad that fills the entire screen in Normalized Device Coordinates.
		// positions   // texCoords
		-1.0f,  1.0f,  0.0f, 1.0f,
		-1.0f, -1.0f,  0.0f, 0.0f,
		 1.0f, -1.0f,  1.0f, 0.0f,

		-1.0f,  1.0f,  0.0f, 1.0f,
		 1.0f, -1.0f,  1.0f, 0.0f,
		 1.0f,  1.0f,  1.0f, 1.0f
	};

	unsigned int quadVBO;
	glGenVertexArrays(1, &m_PostAAVAO);
	glGenBuffers(1, &quadVBO);
	glBindVertexArray(m_PostAAVAO);
	glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));


}

void LithoRenderer::MakeInfillVAO()
{
	int infillLineNum = m_InfillSize / m_InfillLineGap;

	m_InfillVertices.clear();
	// horizontal line, each line represent as a quad
	for (int lineId = 1 - infillLineNum; lineId < infillLineNum; lineId++)
	{
		float coord_x, coord_y;
		// left up point 
		coord_x = -m_InfillSize;
		coord_y = lineId * m_InfillLineGap + m_InfillLineWidth;
		m_InfillVertices.push_back(coord_x);
		m_InfillVertices.push_back(coord_y);
		// right up point 
		coord_x = m_InfillSize;
		coord_y = lineId * m_InfillLineGap + m_InfillLineWidth;
		m_InfillVertices.push_back(coord_x);
		m_InfillVertices.push_back(coord_y);
		// right bottom point 
		coord_x = m_InfillSize;
		coord_y = (float)lineId * m_InfillLineGap;
		m_InfillVertices.push_back(coord_x);
		m_InfillVertices.push_back(coord_y);
		// left bottom point 
		coord_x = -m_InfillSize;
		coord_y = (float)lineId * m_InfillLineGap;
		m_InfillVertices.push_back(coord_x);
		m_InfillVertices.push_back(coord_y);
	}

	//vertical line
	for (int lineId = 1 - infillLineNum; lineId < infillLineNum; lineId++)
	{
		float coord_x, coord_y;
		// left up point 
		coord_x = -m_InfillSize;
		coord_y = lineId * m_InfillLineGap + m_InfillLineWidth;
		m_InfillVertices.push_back(coord_y);
		m_InfillVertices.push_back(coord_x);
		// right up point 
		coord_x = m_InfillSize;
		coord_y = lineId * m_InfillLineGap + m_InfillLineWidth;
		m_InfillVertices.push_back(coord_y);
		m_InfillVertices.push_back(coord_x);
		// right bottom point 
		coord_x = m_InfillSize;
		coord_y = (float)lineId * m_InfillLineGap;
		m_InfillVertices.push_back(coord_y);
		m_InfillVertices.push_back(coord_x);
		// left bottom point 
		coord_x = -m_InfillSize;
		coord_y = (float)lineId * m_InfillLineGap;
		m_InfillVertices.push_back(coord_y);
		m_InfillVertices.push_back(coord_x);
	}


	glGenVertexArrays(1, &m_InfillVAO);
	glBindVertexArray(m_InfillVAO);

	// create vbo
	GLuint vboVertices;
	glGenBuffers(1, &vboVertices);
	glBindBuffer(GL_ARRAY_BUFFER, vboVertices);
	glBufferData(GL_ARRAY_BUFFER, m_InfillVertices.size() * sizeof(GL_FLOAT), &m_InfillVertices[0], GL_STATIC_DRAW);

	// vbo attrib
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);

	glBindVertexArray(0);
}

void LithoRenderer::MakeLineModeVAO()
{
	// remake data
	m_PolygonVerticesData.clear();
	for (auto& ring : m_Polygon)
	{
		for (auto& point : ring)
		{
			glm::vec2 newPoint = glm::vec2(point[0], point[1]);
			m_PolygonVerticesData.push_back(newPoint);
		}
	}

	// create vao
	glGenVertexArrays(1, &m_VAOLineMode);
	glBindVertexArray(m_VAOLineMode);

	// create vbo
	GLuint vboVertices;
	glGenBuffers(1, &vboVertices);
	glBindBuffer(GL_ARRAY_BUFFER, vboVertices);
	glBufferData(GL_ARRAY_BUFFER, m_PolygonVerticesData.size() * 2 * sizeof(GL_FLOAT), &m_PolygonVerticesData[0].x, GL_STATIC_DRAW);

	// config vbo attrib
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);

	// unbind vao
	glBindVertexArray(0);
}

void LithoRenderer::MakeThickPolygonVAO()
{
	m_ThickPolygonQuads.clear();
	m_ThickPolygonQuadsIndices.clear();

	for (auto& ring : m_Polygon)
	{


		unsigned int currRingStart = m_ThickPolygonQuads.size();

		for (auto& pt : ring)
		{
			glm::vec2 curr, next, last, interFirst, interSecond;

			if (&pt == &ring.front())
			{
				auto& ptLast = ring.back();
				auto& ptNext = *(&pt + 1);
				curr = glm::vec2(pt[0], pt[1]);
				next = glm::vec2(ptNext[0], ptNext[1]);
				last = glm::vec2(ptLast[0], ptLast[1]);
			}
			else if (&pt == &ring.back())
			{
				auto& ptLast = *(&pt - 1);
				auto& ptNext = ring.front();
				curr = glm::vec2(pt[0], pt[1]);
				next = glm::vec2(ptNext[0], ptNext[1]);
				last = glm::vec2(ptLast[0], ptLast[1]);
			}
			else
			{
				auto& ptLast = *(&pt - 1);
				auto& ptNext = *(&pt + 1);
				curr = glm::vec2(pt[0], pt[1]);
				next = glm::vec2(ptNext[0], ptNext[1]);
				last = glm::vec2(ptLast[0], ptLast[1]);
			}

			CalculateThickLineQuad(curr, last, next, interFirst, interSecond, m_ThickPolygonWidth);
			m_ThickPolygonQuads.push_back(interFirst);
			m_ThickPolygonQuads.push_back(interSecond);

			

		}

		// after quads being made then make indices
		unsigned int currRingQudsPointPairNum = (m_ThickPolygonQuads.size() - currRingStart) / 2;

		for (int i = 0; i < currRingQudsPointPairNum; i++)
		{

			unsigned int t1, t2, t3, t4;

			if (i < currRingQudsPointPairNum - 1)
			{

				t1 = currRingStart + i * 2 + 0;
				t2 = currRingStart + i * 2 + 1;
				t3 = currRingStart + (i + 1) * 2 + 1;
				t4 = currRingStart + (i + 1) * 2 + 0;


			}
			else
			{
				t1 = currRingStart + i * 2 + 0;
				t2 = currRingStart + i * 2 + 1;
				t3 = currRingStart + 1;
				t4 = currRingStart + 0;

			}

			m_ThickPolygonQuadsIndices.push_back(t1);
			m_ThickPolygonQuadsIndices.push_back(t2);
			m_ThickPolygonQuadsIndices.push_back(t3);

			m_ThickPolygonQuadsIndices.push_back(t1);
			m_ThickPolygonQuadsIndices.push_back(t3);
			m_ThickPolygonQuadsIndices.push_back(t4);
		}
	}



	glGenVertexArrays(1, &m_ThickPolygonVAO);
	glBindVertexArray(m_ThickPolygonVAO);

	// create vbo
	GLuint vboVertices;
	glGenBuffers(1, &vboVertices);
	glBindBuffer(GL_ARRAY_BUFFER, vboVertices);
	glBufferData(GL_ARRAY_BUFFER, m_ThickPolygonQuads.size() * 2 * sizeof(GL_FLOAT), &m_ThickPolygonQuads[0].x, GL_STATIC_DRAW);

	// vbo attrib
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);

	// create ebo to indexing
	GLuint ebo;
	glGenBuffers(1, &ebo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, m_ThickPolygonQuadsIndices.size() * sizeof(unsigned int), &m_ThickPolygonQuadsIndices[0], GL_STATIC_DRAW);

	glBindVertexArray(0);

}

void LithoRenderer::MakeAAShader()
{
	m_Shader = new Shader("shader/polygon.vert", "shader/polygon.frag");

	m_Shader->use();

	glm::mat4 projMat = glm::ortho(-10.f, 400.f, -10.f, 400.f, -1.f, 1.f);

	glm::mat4 mvp = projMat;

	m_Shader->setMat4("mvp", mvp);



}

void LithoRenderer::MakePostAAShader()
{

	m_PostAAShader = new Shader("shader/onscreen.vert", "shader/onscreen.frag");

	m_PostAAShader->use();


}

void LithoRenderer::MakeBlendShader()
{
	m_BlendShader = new Shader("shader/blend.vert", "shader/blend.frag");

	m_BlendShader->use();
}

void LithoRenderer::MakeBlendVAO()
{
	float quadVertices[] = {   // vertex attributes for a quad that fills the entire screen in Normalized Device Coordinates.
		// positions   // texCoords
		-1.0f,  1.0f,  0.0f, 1.0f,
		-1.0f, -1.0f,  0.0f, 0.0f,
		 1.0f, -1.0f,  1.0f, 0.0f,

		-1.0f,  1.0f,  0.0f, 1.0f,
		 1.0f, -1.0f,  1.0f, 0.0f,
		 1.0f,  1.0f,  1.0f, 1.0f
	};

	unsigned int quadVBO;
	glGenVertexArrays(1, &m_BlendVAO);
	glGenBuffers(1, &quadVBO);
	glBindVertexArray(m_BlendVAO);
	glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));



}

void LithoRenderer::MakeMaskFBO()
{
	glGenFramebuffers(1, &m_MaskFBO);
	glBindFramebuffer(GL_FRAMEBUFFER, m_MaskFBO);
	// create a color attachment texture
	glGenTextures(1, &m_MaskTexture);
	glBindTexture(GL_TEXTURE_2D, m_MaskTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, m_WinWidth, m_WinHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_MaskTexture, 0);	// we only need a color buffer

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		std::cout << "ERROR::FRAMEBUFFER:: mask framebuffer is not complete!" << std::endl;
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

}

void LithoRenderer::MakeInfillFBO()
{
	glGenFramebuffers(1, &m_InfillFBO);
	glBindFramebuffer(GL_FRAMEBUFFER, m_InfillFBO);
	// create a color attachment texture
	glGenTextures(1, &m_InfillTexture);
	glBindTexture(GL_TEXTURE_2D, m_InfillTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, m_WinWidth, m_WinHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_InfillTexture, 0);	// we only need a color buffer

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		std::cout << "ERROR::FRAMEBUFFER:: infill framebuffer is not complete!" << std::endl;
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void LithoRenderer::MakeBlendFBO()
{

}

void LithoRenderer::SaveFromFBO(std::string name, GLuint fbo)
{
	FILE* output_image;
	int     output_width, output_height;

	output_width = m_WinWidth;
	output_height = m_WinHeight;

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

	std::string path = "output/" + name + ".ppm";

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

void LithoRenderer::CalculateThickLineQuad(glm::vec2& curr, glm::vec2& last, glm::vec2& next, glm::vec2& interFirst, glm::vec2& interSecond, float lineWidth)
{
	// normlized vector curr-> next 
	glm::vec2 toNext;
	toNext = glm::normalize(next - curr);

	// normlized vector curr-> last 
	glm::vec2 toLast;
	toLast = glm::normalize(last - curr);

	// normalized vector 
	glm::vec2 toQuad = (toNext + toLast); // may be zero vector, when normalized func being used,must notice its arguments not be zero 
	float checkZeroVector = toQuad.x * toQuad.x + toQuad.y * toQuad.y;
	if (checkZeroVector<0.0001)
	{
		toQuad = glm::normalize(glm::vec2(-toNext.y, toNext.x));
	}
	else {
		toQuad = glm::normalize(toQuad);
	}
	

	// check left side or right side
	float checkLeft = toNext.x * toQuad.y - toNext.y * toQuad.x;
	if (checkLeft < 0)
		toQuad = -toQuad;

	// left quad point 
	glm::vec2 normOfNext = glm::normalize(glm::vec2(-toNext.y, toNext.x));
	float cosValue = abs(normOfNext.x * toQuad.x + normOfNext.y * toQuad.y);
	float toQuadLength = lineWidth / cosValue;
	toQuad = toQuadLength * toQuad;

	// quad point this node
	interFirst = curr + toQuad;
	interSecond = curr - toQuad;

	/*std::cout << "curr: " << curr.x << " " << curr.y << " " << " first: " << interFirst.x << " " << interFirst.y << " second: " << interSecond.x << " " << interSecond.y
		<<" monitor:"<< toNext.x <<" "<< toNext.y
	<< " monitor2:" << toLast.x << " " << toLast.y << std::endl;*/

}

void LithoRenderer::PrepareOffScreen(GLuint& textureId, GLuint& fboId)
{
	glGenFramebuffers(1, &fboId);
	glBindFramebuffer(GL_FRAMEBUFFER, fboId);
	// create a color attachment texture
	glGenTextures(1, &textureId);
	glBindTexture(GL_TEXTURE_2D, textureId);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, m_WinWidth, m_WinHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	// bind together
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, textureId, 0);	// we only need a color buffer

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		std::cout << "ERROR::FRAMEBUFFER:: offscreen framebuffer is not complete!" << std::endl;
	glBindFramebuffer(GL_FRAMEBUFFER, 0);


}



void LithoRenderer::DrawTriangulatedPolygonAA()
{
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// bind fbo
	glBindFramebuffer(GL_FRAMEBUFFER, m_AAFBO);
	glViewport(0, 0, m_WinWidth, m_WinHeight);
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glEnable(GL_DEPTH_TEST);

	// draw aa
	m_Shader->use();
	{
		float framePhysicalSize = m_PixelSize * (m_WinWidth / 2);

		glm::mat4 projMat = glm::ortho(-framePhysicalSize + m_CenterX, framePhysicalSize + m_CenterX, -framePhysicalSize + m_CenterY, framePhysicalSize + m_CenterY, -1.f, 1.f);
		glm::mat4 rotMat = glm::rotate(glm::mat4(1.f), m_InfillPatternRotation, glm::vec3(0, 0, 1));
		glm::mat4 mvp = projMat * rotMat;

		m_Shader->setMat4("mvp", mvp);
	}
	glBindVertexArray(m_TriangulatedVAO);
	glDrawElements(GL_TRIANGLES, m_Indices.size(), GL_UNSIGNED_INT, nullptr);
	glBindVertexArray(0);

	// blit 
	glBindFramebuffer(GL_READ_FRAMEBUFFER, m_AAFBO);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_PostAAFBO);
	glBlitFramebuffer(0, 0, m_WinWidth, m_WinHeight, 0, 0, m_WinWidth, m_WinHeight, GL_COLOR_BUFFER_BIT, GL_NEAREST);

	// unbind fbo and draw on screen
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);
	glDisable(GL_DEPTH_TEST);

	m_PostAAShader->use();
	glBindVertexArray(m_PostAAVAO);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, m_PostAATexture); // use the now resolved color attachment as the quad's texture
	glDrawArrays(GL_TRIANGLES, 0, 6);

	// draw on a texture
	glBindFramebuffer(GL_FRAMEBUFFER, m_MaskFBO);
	glViewport(0, 0, m_WinWidth, m_WinHeight);
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glEnable(GL_DEPTH_TEST);

	m_PostAAShader->use();
	glBindVertexArray(m_PostAAVAO);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, m_PostAATexture); // use the now resolved color attachment as the quad's texture
	glDrawArrays(GL_TRIANGLES, 0, 6);

	if (m_SaveFlag)
	{
		SaveFromFBO("mask", m_MaskFBO);
		m_SaveFlag = false;
	}

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

}

void LithoRenderer::DrawInfill()
{
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// bind fbo
	glBindFramebuffer(GL_FRAMEBUFFER, m_AAFBO);
	glViewport(0, 0, m_WinWidth, m_WinHeight);
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glEnable(GL_DEPTH_TEST);

	// draw aa
	m_Shader->use();
	{
		float framePhysicalSize = m_PixelSize * (m_WinWidth / 2);

		glm::mat4 projMat = glm::ortho(-framePhysicalSize + m_CenterX, framePhysicalSize + m_CenterX, -framePhysicalSize + m_CenterY, framePhysicalSize + m_CenterY, -1.f, 1.f);
		glm::mat4 rotMat = glm::rotate(glm::mat4(1.f), m_InfillPatternRotation, glm::vec3(0, 0, 1));
		glm::mat4 mvp = projMat * rotMat;

		m_Shader->setMat4("mvp", mvp);
	}

	/*float lineWidth = m_InfillLineWidth / m_PixelSize;
	std::cout << lineWidth << std::endl;
	glLineWidth(lineWidth);*/

	glBindVertexArray(m_InfillVAO);
	glDrawArrays(GL_QUADS, 0, m_InfillVertices.size());
	glBindVertexArray(0);

	// blit 
	glBindFramebuffer(GL_READ_FRAMEBUFFER, m_AAFBO);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_PostAAFBO);
	glBlitFramebuffer(0, 0, m_WinWidth, m_WinHeight, 0, 0, m_WinWidth, m_WinHeight, GL_COLOR_BUFFER_BIT, GL_NEAREST);

	// bind fbo draw on texture
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);
	glDisable(GL_DEPTH_TEST);

	m_PostAAShader->use();
	glBindVertexArray(m_PostAAVAO);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, m_PostAATexture); // use the now resolved color attachment as the quad's texture
	glDrawArrays(GL_TRIANGLES, 0, 6);


	// draw on fbo
	glBindFramebuffer(GL_FRAMEBUFFER, m_InfillFBO);
	glViewport(0, 0, m_WinWidth, m_WinHeight);
	glClearColor(1.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glEnable(GL_DEPTH_TEST);

	m_PostAAShader->use();
	glBindVertexArray(m_PostAAVAO);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, m_PostAATexture); // use the now resolved color attachment as the quad's texture
	glDrawArrays(GL_TRIANGLES, 0, 6);

	if (m_SaveInfillFalg)
	{
		SaveFromFBO("infill", m_InfillFBO);
		m_SaveInfillFalg = false;
	}

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void LithoRenderer::DrawThickPolygon()
{
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// bind aa fbo
	glBindFramebuffer(GL_FRAMEBUFFER, m_AAFBO);
	glViewport(0, 0, m_WinWidth, m_WinHeight);
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glEnable(GL_DEPTH_TEST);

	// bind shader
	m_Shader->use();
	{
		float framePhysicalSize = m_PixelSize * (m_WinWidth / 2);

		glm::mat4 projMat = glm::ortho(-framePhysicalSize + m_CenterX, framePhysicalSize + m_CenterX, -framePhysicalSize + m_CenterY, framePhysicalSize + m_CenterY, -1.f, 1.f);
		glm::mat4 rotMat = glm::rotate(glm::mat4(1.f), m_InfillPatternRotation, glm::vec3(0, 0, 1));
		glm::mat4 mvp = projMat * rotMat;

		m_Shader->setMat4("mvp", mvp);
	}

	/*float lineWidth = m_InfillLineWidth / m_PixelSize;
	std::cout << lineWidth << std::endl;
	glLineWidth(lineWidth);*/

	// draw triagualated lines
	glBindVertexArray(m_ThickPolygonVAO);
	//glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	glDrawElements(GL_TRIANGLES, m_ThickPolygonQuadsIndices.size(), GL_UNSIGNED_INT, NULL);
	//glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glBindVertexArray(0);

	// blit from aa fbo
	glBindFramebuffer(GL_READ_FRAMEBUFFER, m_AAFBO);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_PostAAFBO);
	glBlitFramebuffer(0, 0, m_WinWidth, m_WinHeight, 0, 0, m_WinWidth, m_WinHeight, GL_COLOR_BUFFER_BIT, GL_NEAREST);

	// unbind fbo to render aa on screen
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);
	glDisable(GL_DEPTH_TEST);

	m_PostAAShader->use();
	glBindVertexArray(m_PostAAVAO);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, m_PostAATexture); // use the now resolved color attachment as the quad's texture
	glDrawArrays(GL_TRIANGLES, 0, 6);

	
	// offscreen
	glBindFramebuffer(GL_FRAMEBUFFER, m_FBOThickPolygon);
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);
	glDisable(GL_DEPTH_TEST);

	m_PostAAShader->use();
	glBindVertexArray(m_PostAAVAO);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, m_PostAATexture); // use the now resolved color attachment as the quad's texture
	glDrawArrays(GL_TRIANGLES, 0, 6);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

}

void LithoRenderer::DrawBlend()
{
	// previous
	DrawThickPolygon();
	DrawInfill();
	DrawTriangulatedPolygonAA();

	// blending
	glViewport(0, 0, m_WinWidth, m_WinHeight);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);
	glDisable(GL_DEPTH_TEST);

	m_BlendShader->use();
	glBindVertexArray(m_BlendVAO);

	m_BlendShader->bindTextureUnit("maskTexture", 0);
	m_BlendShader->bindTextureUnit("infillTexture", 1);
	m_BlendShader->bindTextureUnit("strokeTexture", 2);

	glActiveTexture(GL_TEXTURE0 + 0); glBindTexture(GL_TEXTURE_2D, m_MaskTexture);
	glActiveTexture(GL_TEXTURE0 + 1); glBindTexture(GL_TEXTURE_2D, m_InfillTexture);
	glActiveTexture(GL_TEXTURE0 + 2); glBindTexture(GL_TEXTURE_2D, m_TextureThickPolygon);

	glDrawArrays(GL_TRIANGLES, 0, 6);


}

void LithoRenderer::DrawPolygonWithInfill()
{
	// 1. draw infill 
	//DrawInfill();

	// 2. draw polygon mask
	//DrawPolygonAA();

	// draw thick polygon
	DrawThickPolygon();

	// 3. draw litho
	//DrawBlend();
}

void LithoRenderer::PrepareLineMode()
{
	// shader
	{
		m_ShaderLineMode = new Shader("shader/line.vert", "shader/line.frag");

		m_ShaderLineMode->use();

		glm::mat4 projMat = glm::ortho(-10.f, 400.f, -10.f, 400.f, -1.f, 1.f);

		glm::mat4 mvp = projMat;

		m_ShaderLineMode->setMat4("mvp", mvp);
	}

	// fbo
	PrepareOffScreen(m_TextureLineMode, m_FBOLineMode);

}

void LithoRenderer::DrawLineMode()
{
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(0, 0, m_WinWidth, m_WinHeight);

	
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	m_ShaderLineMode->use();
	{
		float framePhysicalSize = m_PixelSize * (m_WinWidth / 2);

		glm::mat4 projMat = glm::ortho(-framePhysicalSize + m_CenterX, framePhysicalSize + m_CenterX, -framePhysicalSize + m_CenterY, framePhysicalSize + m_CenterY, -1.f, 1.f);
		glm::mat4 rotMat = glm::rotate(glm::mat4(1.f), m_InfillPatternRotation, glm::vec3(0, 0, 1));
		glm::mat4 mvp = projMat * rotMat;

		m_ShaderLineMode->setMat4("mvp", mvp);
	}

	glBindVertexArray(m_VAOLineMode);

	//glDrawArrays(GL_LINE_LOOP, 0, m_PolygonVerticesData.size());
	glDrawArrays(GL_POINTS, 0, m_PolygonVerticesData.size());

	glBindVertexArray(0);

	/*off screen*/
	glBindFramebuffer(GL_FRAMEBUFFER, m_FBOLineMode);
	{
		glViewport(0, 0, m_WinWidth, m_WinHeight);

		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		m_ShaderLineMode->use();
		{
			float framePhysicalSize = m_PixelSize * (m_WinWidth / 2);

			glm::mat4 projMat = glm::ortho(-framePhysicalSize + m_CenterX, framePhysicalSize + m_CenterX, -framePhysicalSize + m_CenterY, framePhysicalSize + m_CenterY, -1.f, 1.f);
			glm::mat4 rotMat = glm::rotate(glm::mat4(1.f), m_InfillPatternRotation, glm::vec3(0, 0, 1));
			glm::mat4 mvp = projMat * rotMat;

			m_ShaderLineMode->setMat4("mvp", mvp);
		}

		glBindVertexArray(m_VAOLineMode);

		//glDrawArrays(GL_LINE_LOOP, 0, m_PolygonVerticesData.size());
		glDrawArrays(GL_POINTS, 0, m_PolygonVerticesData.size());

		glBindVertexArray(0);
	}
	glBindFramebuffer(GL_FRAMEBUFFER, 0);




}


void LithoRenderer::CreateAAFBO()
{
	glGenFramebuffers(1, &m_AAFBO);
	glBindFramebuffer(GL_FRAMEBUFFER, m_AAFBO);

	// texture 
	glGenTextures(1, &m_AATexture);
	glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, m_AATexture);

	glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, m_AASamples, GL_RGB, m_WinWidth, m_WinHeight, GL_TRUE);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D_MULTISAMPLE, m_AATexture, 0);

	// rbo
	unsigned int rbo;
	glGenRenderbuffers(1, &rbo);
	glBindRenderbuffer(GL_RENDERBUFFER, rbo);
	glRenderbufferStorageMultisample(GL_RENDERBUFFER, m_AASamples, GL_DEPTH24_STENCIL8, m_WinWidth, m_WinHeight);
	glBindRenderbuffer(GL_RENDERBUFFER, 0);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbo);


	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
	{
		std::cout << "aa fbo not complete" << std::endl;
		glDeleteFramebuffers(1, &m_AAFBO);
	}

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	//  post fbo
	glGenFramebuffers(1, &m_PostAAFBO);
	glBindFramebuffer(GL_FRAMEBUFFER, m_PostAAFBO);
	// create a color attachment texture
	glGenTextures(1, &m_PostAATexture);
	glBindTexture(GL_TEXTURE_2D, m_PostAATexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, m_WinWidth, m_WinHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_PostAATexture, 0);	// we only need a color buffer

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		std::cout << "ERROR::FRAMEBUFFER:: Intermediate framebuffer is not complete!" << std::endl;
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

}
