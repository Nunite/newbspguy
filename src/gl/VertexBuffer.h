#pragma once
#include <GL/glew.h>
#include <vector>
#include "ShaderProgram.h"
#include "util.h"

class VertexBuffer
{
public:
	size_t numVerts;
	int primitive;
	bool ownData; // set to true if buffer should delete data on destruction
	ShaderProgram* shaderProgram; // for getting handles to vertex attributes

	// Specify which common attributes to use. They will be located in the
	// shader program. If passing data, note that data is not copied, but referenced
	VertexBuffer(ShaderProgram* shaderProgram, void* dat, size_t numVerts, int primitive = 0);
	~VertexBuffer();

	// Note: Data is not copied into the class - don't delete your data.
	//       Data will be deleted when the buffer is destroyed.
	void setData(void* data, size_t numVerts);

	// get data (if deleted, then fill it from subbuffer)
	unsigned char* get_data();

	void upload(bool hideErrors = true, bool forceReupload = true);
	void deleteBuffer();

	void drawRange(int primitive, size_t start, size_t end, bool hideErrors = true);
	void draw(int primitive);
	void drawFull();

	bool uploaded;
	size_t frameId;
private:
	unsigned char* data;
	GLuint vboId;
	GLuint vaoId;
	// add attributes according to the attribute flags
	void addAttributes(int attFlags);
};