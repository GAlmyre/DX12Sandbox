#include <DirectXMath.h>
#include <vector>
#include "pch.h"

struct Vertex
{
	Vertex(DirectX::XMFLOAT3 InPos, DirectX::XMFLOAT4 InColor) 
	{ 
		Pos = InPos; 
		Color = InColor;	
	}

	DirectX::XMFLOAT3 Pos;
	DirectX::XMFLOAT4 Color; 
};

class CMesh
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