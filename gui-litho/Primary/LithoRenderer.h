#pragma once
#define GLM_FORCE_RADIANS
#include <GL/glew.h>
#include <gl/GL.h>
#include "shader.h"
#include <array>
#include <algorithm>
#include "mapbox/earcut.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <string>

#define _USE_MATH_DEFINES
#include <math.h>

#include "LithoModel.h"

enum LithoUnits
{
	Milli,Micron,Nano
};



class LithoRenderer
{
	using Coord = float;
	using N = unsigned int;
	using Point = std::array<Coord, 2>;

public:

	LithoRenderer() {}
	
	void Init();

	void RemakePolygonVAO(LithoModel& model,int layer,int polygon);

	void RemakeInfillVAO(float lineWidth, float lineGap,int lineNum);

	glm::vec2 GetCenter();

	void SetPixelSize(float pixelSize);
	void SetPatternRotation(float rotDeg);
	void SetLineWidth(float lineWidth);
	void ShiftHorizontal(int pixelNumber);
	void ShiftVertical(int pixelNumber);

	void MoveByPixel(int x,int y,bool release);
	

	void SaveOnceMask();
	void SaveOnceInfill();

	void RenderSpecificMode(int mode);

	void DrawLine();

	void DrawPolygon();

	void DrawTriangulatedPolygonAA(); // anti-aliasing draw call

	void DrawInfill();

	void DrawThickPolygon();

	void DrawBlend();

	void DrawPolygonWithInfill();

private:// draw mode 
	void PrepareLineMode();
	void DrawLineMode();

private:
	float m_PixelSize = 0.15; // 


	int m_WinWidth = 1000;
	int m_WinHeight = 1000;
	int m_AASamples = 16;

	float m_CenterX = 0;
	float m_CenterY = 0;
	float m_CenterXLast = 0;
	float m_CenterYLast = 0;


	float m_InfillSize = 2.0f;
	float m_InfillLineWidth = 0.02f;
	float m_InfillLineGap = 0.08f;

	float m_ThickPolygonWidth = 0.02f;

	float m_InfillPatternRotation = 0;

	

	bool m_SaveFlag = false;
	bool m_SaveInfillFalg = false;



	void CreateAAFBO();
	void MakeTriangulatedVAO();
	void MakePostAAVAO();
	void MakeInfillVAO();
	void MakeLineModeVAO();// just polygon points nofancy
	void MakeThickPolygonVAO();// for render thick line of a polygon, 
	void MakeAAShader();
	void MakePostAAShader();
	void MakeBlendShader();
	void MakeBlendVAO();



	void MakeMaskFBO();
	void MakeInfillFBO();
	void MakeBlendFBO();


	void SaveFromFBO(std::string name,GLuint fbo);
	void CalculateThickLineQuad(glm::vec2 & curr, glm::vec2 & last, glm::vec2 & next, glm::vec2 & interFirst, glm::vec2 & interSecond,float lineWidth);

	void PrepareOffScreen(GLuint& textureId, GLuint& fboId);

	std::vector<std::vector<Point>> m_Polygon;
	
	std::vector<float> m_PolygonDumb;
	std::vector<N> m_Indices;
	std::vector<unsigned int> m_IndicesDumb;

	std::vector<float> m_InfillVertices;
	std::vector<glm::vec2> m_PolygonVerticesData;
	std::vector<glm::vec2> m_ThickPolygonQuads;
	std::vector<unsigned int> m_ThickPolygonQuadsIndices;

	GLuint m_TriangulatedVAO;
	GLuint m_PostAAVAO;
	GLuint m_InfillVAO;
	GLuint m_BlendVAO;
	GLuint m_BlendFBO;
	GLuint m_ThickPolygonVAO;
	GLuint m_VAOLineMode;
	GLuint m_TriangulatedEBO;
	GLuint m_AAFBO;
	GLuint m_MaskFBO;
	GLuint m_MaskTexture;
	GLuint m_InfillFBO;
	GLuint m_InfillTexture;
	GLuint m_AATexture;
	GLuint m_PostAAFBO;
	GLuint m_PostAATexture;
	Shader* m_Shader;
	Shader* m_ShaderLineMode;
	Shader* m_PostAAShader;
	Shader* m_BlendShader;

	GLuint m_TextureLineMode;
	GLuint m_FBOLineMode;

	GLuint m_TextureThickPolygon;
	GLuint m_FBOThickPolygon;

	


	glm::mat4 m_MVP;




};

