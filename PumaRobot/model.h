#pragma once
#include <vector>
#include <istream>

struct VertexPosition
{
	float x, y, z;
};

struct VertexNormal
{
	VertexPosition* positionPtr;
	float nx, ny, nz;
};

struct Face
{
	unsigned int vertexIdx1;
	unsigned int vertexIdx2;
	unsigned int vertexIdx3;
};

struct Edge
{
	VertexPosition* vertexIndexPtr1;
	VertexPosition* vertexIndexPtr2;
	Face faceIndexPtr1;
	Face faceIndexPtr2;
};

class Model
{
private:
	std::istream& LoadPositions(std::istream& in);
	std::istream& LoadNormals(std::istream& in);
	std::istream& LoadFaces(std::istream& in);
	std::istream& LoadEdges(std::istream& in);
public:
	std::vector<VertexPosition> vertexPositions;
	std::vector<VertexNormal> vertexNormals;
	std::vector<Face> faces;
	std::vector<Edge> edges;

	void LoadModelFromFile(const std::string& filePath);
};

