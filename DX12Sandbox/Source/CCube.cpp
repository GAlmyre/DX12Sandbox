#include "pch.h"
#include "CCube.h"

CCube::CCube()
{
	Vertices.clear();
	// Front
	Vertices.emplace_back(DirectX::XMFLOAT3(-0.5f,	0.5f,		-0.5f),	DirectX::XMFLOAT2(0.0f, 0.0f));
	Vertices.emplace_back(DirectX::XMFLOAT3(0.5f,	-0.5f,	-0.5f),		DirectX::XMFLOAT2(1.0f, 1.0f));
	Vertices.emplace_back(DirectX::XMFLOAT3(-0.5f,	-0.5f,	-0.5f),		DirectX::XMFLOAT2(0.0f, 1.0f));
	Vertices.emplace_back(DirectX::XMFLOAT3(0.5f,	0.5f,		-0.5f),	DirectX::XMFLOAT2(1.0f, 0.0f));

	// Right
	Vertices.emplace_back(Vertex(DirectX::XMFLOAT3(0.5f,	-0.5f,	-0.5f), DirectX::XMFLOAT2(0.0f, 1.0f)));
	Vertices.emplace_back(Vertex(DirectX::XMFLOAT3(0.5f, 0.5f, 0.5f), DirectX::XMFLOAT2(1.0f, 0.0f)));
	Vertices.emplace_back(Vertex(DirectX::XMFLOAT3(0.5f, -0.5f, 0.5f), DirectX::XMFLOAT2(1.0f, 1.0f)));
	Vertices.emplace_back(Vertex(DirectX::XMFLOAT3(0.5f, 0.5f, -0.5f), DirectX::XMFLOAT2(0.0f, 0.0f)));

	// Left
	Vertices.emplace_back(Vertex(DirectX::XMFLOAT3(-0.5f,	0.5f,		0.5f), DirectX::XMFLOAT2(0.0f, 0.0f)));
	Vertices.emplace_back(Vertex(DirectX::XMFLOAT3(-0.5f, -0.5f, -0.5f), DirectX::XMFLOAT2(1.0f, 1.0f)));
	Vertices.emplace_back(Vertex(DirectX::XMFLOAT3(-0.5f, -0.5f, 0.5f), DirectX::XMFLOAT2(0.0f, 1.0f)));
	Vertices.emplace_back(Vertex(DirectX::XMFLOAT3(-0.5f, 0.5f, -0.5f), DirectX::XMFLOAT2(1.0f, 0.0f)));

	// Back
	Vertices.emplace_back(Vertex(DirectX::XMFLOAT3(0.5f,	0.5f,		0.5f), DirectX::XMFLOAT2(0.0f, 0.0f)));
	Vertices.emplace_back(Vertex(DirectX::XMFLOAT3(-0.5f, -0.5f, 0.5f), DirectX::XMFLOAT2(1.0f, 1.0f)));
	Vertices.emplace_back(Vertex(DirectX::XMFLOAT3(0.5f, -0.5f, 0.5f), DirectX::XMFLOAT2(0.0f, 1.0f)));
	Vertices.emplace_back(Vertex(DirectX::XMFLOAT3(-0.5f, 0.5f, 0.5f), DirectX::XMFLOAT2(1.0f, 0.0f)));
	// Top
	Vertices.emplace_back(Vertex(DirectX::XMFLOAT3(-0.5f,	0.5f,		-0.5f), DirectX::XMFLOAT2(0.0f, 1.0f)));
	Vertices.emplace_back(Vertex(DirectX::XMFLOAT3(0.5f, 0.5f, 0.5f), DirectX::XMFLOAT2(1.0f, 0.0f)));
	Vertices.emplace_back(Vertex(DirectX::XMFLOAT3(0.5f, 0.5f, -0.5f), DirectX::XMFLOAT2(1.0f, 1.0f)));
	Vertices.emplace_back(Vertex(DirectX::XMFLOAT3(-0.5f, 0.5f, 0.5f), DirectX::XMFLOAT2(0.0f, 0.0f)));

	// Bottom
	Vertices.emplace_back(Vertex(DirectX::XMFLOAT3(0.5f,	-0.5f,	0.5f), DirectX::XMFLOAT2(0.0f, 0.0f)));
	Vertices.emplace_back(Vertex(DirectX::XMFLOAT3(-0.5f, -0.5f, -0.5f), DirectX::XMFLOAT2(1.0f, 1.0f)));
	Vertices.emplace_back(Vertex(DirectX::XMFLOAT3(0.5f, -0.5f, -0.5f), DirectX::XMFLOAT2(0.0f, 1.0f)));
	Vertices.emplace_back(Vertex(DirectX::XMFLOAT3(-0.5f, -0.5f, 0.5f), DirectX::XMFLOAT2(1.0f, 0.0f)));

	Indices =
	{
		// front face
		0, 1, 2, // first triangle
		0, 3, 1, // second triangle

		// left face
		4, 5, 6, // first triangle
		4, 7, 5, // second triangle

		// right face
		8, 9, 10, // first triangle
		8, 11, 9, // second triangle

		// back face
		12, 13, 14, // first triangle
		12, 15, 13, // second triangle

		// top face
		16, 17, 18, // first triangle
		16, 19, 17, // second triangle

		// bottom face
		20, 21, 22, // first triangle
		20, 23, 21, // second triangle
	};
}