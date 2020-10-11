#pragma once

#include "GLFunctions.h"
#include <array>

class GLBackdrop
{
	GLFunctions gl;
	GLuint vao;
	GLuint vbo;
	GLuint program;
	GLuint texture;
	int textureWidth;
	int textureHeight;
	unsigned char* textureData;
	std::array<GLfloat, 4 * 6> vertexBufferData;

public:
	GLBackdrop(int width, int height, unsigned char* pixels);
	~GLBackdrop();
	void UpdateTexture();
	void UpdateQuad(int windowWidth, int windowHeight, bool pillarbox);
	void Render(int windowWidth, int windowHeight, bool pillarbox, bool linearFiltering);
};
