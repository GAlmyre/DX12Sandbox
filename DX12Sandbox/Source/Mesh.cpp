#include "Mesh.h"
#include "pch.h"

//using namespace DirectX;

CMesh::CMesh()
{
	// First quad
	Vertices.push_back(Vertex(DirectX::XMFLOAT3(-0.5f, 0.5f, 0.5f), DirectX::XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f)));
	Vertices.push_back(Vertex(DirectX::XMFLOAT3(0.5f, -0.5f, 0.5f), DirectX::XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f)));
	Vertices.push_back(Vertex(DirectX::XMFLOAT3(-0.5f, -0.5f, 0.5f), DirectX::XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f)));
	Vertices.push_back(Vertex(DirectX::XMFLOAT3(0.5f, 0.5f,	0.5f), DirectX::XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f)));

	// Second quad
	Vertices.push_back(Vertex(DirectX::XMFLOAT3(-0.75f, 0.75f, 0.7f),	DirectX::XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f)));
	Vertices.push_back(Vertex(DirectX::XMFLOAT3(0.0f, 0.0f, 0.7f),	DirectX::XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f)));
	Vertices.push_back(Vertex(DirectX::XMFLOAT3(-0.75f, 0.0f, 0.7f),	DirectX::XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f)));
	Vertices.push_back(Vertex(DirectX::XMFLOAT3(0.0f, 0.75f, 0.7f),	DirectX::XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f)));

	Indices = 
	{	
		0, 1, 2, // First Triangle
		0, 3, 1  // Second Triangle
	};
}

void CMesh::Init(ID3D12Device* Device, ID3D12GraphicsCommandList* CommandList)
{
	int VertexBufferSize = GetVertexBufferSize();

	CD3DX12_HEAP_PROPERTIES HeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	CD3DX12_RESOURCE_DESC BufferDesc = CD3DX12_RESOURCE_DESC::Buffer(VertexBufferSize);
	Device->CreateCommittedResource(
		&HeapProperties,
		D3D12_HEAP_FLAG_NONE,
		&BufferDesc,
		D3D12_RESOURCE_STATE_COPY_DEST,
		nullptr,
		IID_PPV_ARGS(&VertexBuffer)
	);
	VertexBuffer->SetName(L"Vertex Buffer Resource Heap");

	ID3D12Resource* VBufferUploadHeap;
	HeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	Device->CreateCommittedResource(
		&HeapProperties,
		D3D12_HEAP_FLAG_NONE,
		&BufferDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&VBufferUploadHeap)
	);
	VBufferUploadHeap->SetName(L"Vertex Buffer Upload Resource Heap");

	D3D12_SUBRESOURCE_DATA VertexData = {};
	VertexData.pData = this->Vertices.data();
	VertexData.RowPitch = VertexBufferSize;
	VertexData.SlicePitch = VertexBufferSize;

	UpdateSubresources(CommandList, VertexBuffer, VBufferUploadHeap, 0, 0, 1, &VertexData);
	CD3DX12_RESOURCE_BARRIER Barrier = CD3DX12_RESOURCE_BARRIER::Transition(VertexBuffer, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
	CommandList->ResourceBarrier(1, &Barrier);

	// Create the IndexBuffer
	int IndexBufferSize = GetIndexBufferSize();

	HeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	BufferDesc = CD3DX12_RESOURCE_DESC::Buffer(IndexBufferSize);
	Device->CreateCommittedResource(
		&HeapProperties,
		D3D12_HEAP_FLAG_NONE,
		&BufferDesc,
		D3D12_RESOURCE_STATE_COPY_DEST,
		nullptr,
		IID_PPV_ARGS(&IndexBuffer)
	);
	IndexBuffer->SetName(L"Index Buffer Resource Heap");

	ID3D12Resource* IBufferUploadHeap;
	HeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	Device->CreateCommittedResource(
		&HeapProperties,
		D3D12_HEAP_FLAG_NONE,
		&BufferDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&IBufferUploadHeap)
	);
	IBufferUploadHeap->SetName(L"Vertex Buffer Upload Resource Heap");

	D3D12_SUBRESOURCE_DATA IndexData = {};
	IndexData.pData = Indices.data();
	IndexData.RowPitch = IndexBufferSize;
	IndexData.SlicePitch = IndexBufferSize;

	UpdateSubresources(CommandList, IndexBuffer, IBufferUploadHeap, 0, 0, 1, &IndexData);
	Barrier = CD3DX12_RESOURCE_BARRIER::Transition(IndexBuffer, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
	CommandList->ResourceBarrier(1, &Barrier);

	//VertexBufferView.BufferLocation = VertexBuffer->GetGPUVirtualAddress();
	//VertexBufferView.StrideInBytes = sizeof(Vertex);
	//VertexBufferView.SizeInBytes = GetVertexBufferSize();

	//IndexBufferView.BufferLocation = IndexBuffer->GetGPUVirtualAddress();
	//IndexBufferView.Format = DXGI_FORMAT_R32_UINT;
	//IndexBufferView.SizeInBytes = GetIndexBufferSize();

}

int CMesh::GetVertexBufferSize() const
{
	return sizeof(Vertex) * this->Vertices.size();
}

int CMesh::GetIndexBufferSize() const
{
	return sizeof(Vertex) * Indices.size();
}

void CMesh::Draw(ID3D12GraphicsCommandList* CommandList)
{
	// Set Vertex Buffer
	CommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	CommandList->IASetVertexBuffers(0 ,1, &VertexBufferView);
		
	// Set Index Buffer
	CommandList->IASetIndexBuffer(&IndexBufferView);
	CommandList->DrawIndexedInstanced(6, 1, 0, 0, 0);
	CommandList->DrawIndexedInstanced(6, 1, 0, 4, 0);
}

CMesh::~CMesh()
{
	SAFE_RELEASE(VertexBuffer);
	SAFE_RELEASE(IndexBuffer);
}
