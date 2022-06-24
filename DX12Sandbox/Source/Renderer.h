#include "pch.h"

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

	bool bRunning = true;

	/********** Window Parameters **********/

	HWND HWindow;

	int WindowWidth = 1280;

	int WindowHeight = 720;

	bool bFullScreen = false;

	/********** EndWindow Parameters **********/

	/********** Direct 3D Variables **********/

	// The number of FramBuffers (3 for tripleBuffering)
	const int FrameBufferCount = 3;

	// The direct3D Device
	ID3D12Device* Device;

	// SwapChain to switch between render targets
	IDXGISwapChain3* SwapChain;

	// Command Queue to contain Command Lists
	ID3D12CommandQueue* CommandQueue;

	// Descriptor Heap to hold Resources
	ID3D12DescriptorHeap* RTVDescriptorHeap;

	// FrameBufferCount Render Targets
	ID3D12Resource* RenderTargets[3];

	// Allocators : FrameBufferCount * NumberOfThreads
	ID3D12CommandAllocator* CommandAllocators[3];

	// The Command list to record commands into then execute to render
	ID3D12GraphicsCommandList* CommandList;

	// PSO containing a pipeline state
	ID3D12PipelineState* PSO;

	// Defines the data that shaders will access
	ID3D12RootSignature* RootSignature;

	// Area that output from the rasterizer will be stretched to
	D3D12_VIEWPORT Viewport;

	// The area to draw in, pixels outside will be culled
	D3D12_RECT ScissorRect;

	//// Default Buffer in GPU memory to send our Vertices
	//ID3D12Resource* VertexBuffer;

	//// A structure containing data to describe our VertexBuffer (pointer, size of the buffer, size of each element)
	//D3D12_VERTEX_BUFFER_VIEW VertexBufferView;

	//// Default Buffer in GPU memory to send our Indices
	//ID3D12Resource* IndexBuffer;

	//// A structure containing data to describe our IndexBuffer 
	//D3D12_INDEX_BUFFER_VIEW IndexBufferView;

	// One fence per allocator to know when the command list is being executed
	ID3D12Fence* Fences[3];

	// Handle to an event for our fence
	HANDLE FenceEvent;

	// Value incremented each frame for each Fence
	UINT64 FenceValues[3];

	// Current RenderTarget
	int FrameIndex;

	// Size of the Render Target Descriptor
	int RTVDescriptorSize;

	/********** End Direct 3D Variables **********/

	class CMesh* Mesh = nullptr;

};