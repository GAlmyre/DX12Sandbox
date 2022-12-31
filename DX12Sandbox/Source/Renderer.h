#pragma once
#include "pch.h"
#include <DirectXMath.h>
#include <wincodec.h>

#define FRAMEBUFFER_COUNT 3

struct ConstantBufferPerObject
{
	DirectX::XMFLOAT4X4 WorldViewProj;
};

class CRenderer
{
public:

	CRenderer();

	// Initialize DirectX12
	bool InitD3D();

	// Update the App's logic
	void Update();

	// Update the D3D Pipeline (command lists)
	void UpdatePipeline();

	// Execute the command list
	void Render();

	// Release Com objects and release memory
	void Cleanup();

	// Wait until the GPU is finished with a command list
	void WaitForPreviousFrame();

	DXGI_FORMAT GetDXGIFormatFromWICFormat(WICPixelFormatGUID& WicFormatGUID);
	WICPixelFormatGUID GetConvertToWICFormat(WICPixelFormatGUID& WicFormatGUID);
	int GetDXGIFormatBitsPerPixel(DXGI_FORMAT& DxgiFormat);
	// Load an image in the form of a bitmap
	int LoadImageDataFromFile(BYTE** ImageData, D3D12_RESOURCE_DESC& ResourceDescription, LPCWSTR Filename, int& BytesPerRow);

	bool bRunning = true;

	/********** Window Parameters **********/

	HWND HWindow;

	int WindowWidth = 1280;

	int WindowHeight = 720;

	bool bFullScreen = false;

	/********** EndWindow Parameters **********/

	/********** Direct 3D Variables **********/

	// The number of FramBuffers (3 for tripleBuffering)
	const int FrameBufferCount = FRAMEBUFFER_COUNT;

	// The direct3D Device
	ID3D12Device* Device;

	// SwapChain to switch between render targets
	IDXGISwapChain3* SwapChain;

	// Command Queue to contain Command Lists
	ID3D12CommandQueue* CommandQueue;

	// Descriptor Heap to hold Resources
	ID3D12DescriptorHeap* RTVDescriptorHeap;

	// FrameBufferCount Render Targets
	ID3D12Resource* RenderTargets[FRAMEBUFFER_COUNT];

	// Allocators : FrameBufferCount * NumberOfThreads
	ID3D12CommandAllocator* CommandAllocators[FRAMEBUFFER_COUNT];

	// The Command list to record commands into then execute to render
	ID3D12GraphicsCommandList* CommandList;

	// PSO containing a pipeline state
	ID3D12PipelineState* PSO;

	// Depth/Stencil
	ID3D12Resource* DepthStencilBuffer; // This is the memory for our depth buffer
	ID3D12DescriptorHeap* DepthStencilDescriptorHeap; // This is a heap for our depth/stencil buffer descriptor

	// Defines the data that shaders will access
	ID3D12RootSignature* RootSignature;

	// Area that output from the rasterizer will be stretched to
	D3D12_VIEWPORT Viewport;

	// The area to draw in, pixels outside will be culled
	D3D12_RECT ScissorRect;

	// One fence per allocator to know when the command list is being executed
	ID3D12Fence* Fences[FRAMEBUFFER_COUNT];

	// Handle to an event for our fence
	HANDLE FenceEvent;

	// Value incremented each frame for each Fence
	UINT64 FenceValues[FRAMEBUFFER_COUNT];

	// Current RenderTarget
	int FrameIndex;

	// Size of the Render Target Descriptor
	int RTVDescriptorSize;

	// *** Constant Buffer *** //
	// The heap to store the descriptor of our constant buffer
	int ConstantBufferPerObjectAlignedSize = (sizeof(ConstantBufferPerObject) + 255) & ~255;

	ConstantBufferPerObject ConstantBuffer;

	// The memory in GPU where our constant buffer will be
	ID3D12Resource* ConstantBufferUploadHeaps[FRAMEBUFFER_COUNT];

	// A pointer to the memory location of our constant buffer
	UINT8* ConstantBufferGPUAdress[FRAMEBUFFER_COUNT];

	/********** End Direct 3D Variables **********/

	class CMesh* Mesh = nullptr;

	// Camera 
	class Camera* SceneCamera = nullptr;

	/* TEXTURE */
	ID3D12Resource* TextureBuffer;

	ID3D12DescriptorHeap* MainDescriptorHeap;

	ID3D12Resource* TextureBufferUploadHeap;
};