#pragma once

#include <DirectXMath.h>
#include <vector>
#include "pch.h"
#include "Actor.h"

struct Vertex
{
	Vertex(XMFLOAT3 InPos, XMFLOAT2 InTexCoord)
	{ 
		Pos = InPos; 
		TexCoord = InTexCoord;
	}

	XMFLOAT3 Pos;
	//XMFLOAT4 Color;
	XMFLOAT2 TexCoord;
};

class CMesh : public Actor
{
public:

	// Constructor
	CMesh();

	~CMesh();

	void Init(ID3D12Device* Device, ID3D12GraphicsCommandList* CommandList);

	int GetVertexBufferSize() const;	

	int GetIndexBufferSize() const;

	void Draw(ID3D12GraphicsCommandList* CommandList);

	// The list of vertices
	std::vector<Vertex> Vertices;

	// The list of indices
	std::vector<unsigned int> Indices;

	/*	DX12 stuff*/

	// Default Buffer in GPU memory to send our Vertices
	ID3D12Resource* VertexBuffer;

	// A structure containing data to describe our VertexBuffer (pointer, size of the buffer, size of each element)
	D3D12_VERTEX_BUFFER_VIEW VertexBufferView;

	// Default Buffer in GPU memory to send our Indices
	ID3D12Resource* IndexBuffer;

	// A structure containing data to describe our IndexBuffer 
	D3D12_INDEX_BUFFER_VIEW IndexBufferView;

	/* End DX12 stuff */
};