#include "pch.h"
#include "Renderer.h"
#include "Mesh.h"
#include "CCube.h"
#include "Camera.h"
#include <shlobj.h>
#include <strsafe.h>
#include <wincodec.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define D3DCOMPILE_DEBUG 1

static std::wstring GetLatestWinPixGpuCapturerPath()
{
	LPWSTR ProgramFilesPath = nullptr;
	SHGetKnownFolderPath(FOLDERID_ProgramFiles, KF_FLAG_DEFAULT, NULL, &ProgramFilesPath);

	std::wstring PixSearchPath = ProgramFilesPath + std::wstring(L"\\Microsoft PIX\\*");

	WIN32_FIND_DATA FindData;
	bool bFoundPixInstallation = false;
	wchar_t NewestVersionFound[MAX_PATH];

	HANDLE hFind = FindFirstFile(PixSearchPath.c_str(), &FindData);
	if (hFind != INVALID_HANDLE_VALUE)
	{
		do
		{
			if (((FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY) &&
				(FindData.cFileName[0] != '.'))
			{
				if (!bFoundPixInstallation || wcscmp(NewestVersionFound, FindData.cFileName) <= 0)
				{
					bFoundPixInstallation = true;
					StringCchCopy(NewestVersionFound, _countof(NewestVersionFound), FindData.cFileName);
				}
			}
		} while (FindNextFile(hFind, &FindData) != 0);
	}

	FindClose(hFind);

	if (!bFoundPixInstallation)
	{
		MessageBox(0, L"Error: no PIX installation found", 0, 0);
	}

	wchar_t Output[MAX_PATH];
	StringCchCopy(Output, PixSearchPath.length(), PixSearchPath.data());
	StringCchCat(Output, MAX_PATH, &NewestVersionFound[0]);
	StringCchCat(Output, MAX_PATH, L"\\WinPixGpuCapturer.dll");

	return &Output[0];
}

CRenderer::CRenderer()
{
}

bool CRenderer::InitD3D()
{
	HRESULT Hr;

	// ----- Create the Device by going through the Graphics cards (adapters) and selecting one that has the required feature level -----
	IDXGIFactory4* Factory;
	Hr = CreateDXGIFactory1(IID_PPV_ARGS(&Factory));
	if (FAILED(Hr))
	{
		MessageBox(0, L"Couldn't create a factory", 0, 0);
		return false;
	}

	// Check to see if a copy of WinPixGpuCapturer.dll has already been injected into the application.
	// This may happen if the application is launched through the PIX UI. 
	if (GetModuleHandle(L"WinPixGpuCapturer.dll") == 0)
	{
		LoadLibrary(GetLatestWinPixGpuCapturerPath().c_str());
	}

	IDXGIAdapter1* Adapter;
	int AdapterIndex = 0;
	bool bAdapterFound = false;

	while (Factory->EnumAdapters1(AdapterIndex, &Adapter) != DXGI_ERROR_NOT_FOUND)
	{
		DXGI_ADAPTER_DESC1 AdapterDesc;
		Adapter->GetDesc1(&AdapterDesc);

		if (AdapterDesc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
		{
			// This is a software device, not an actual graphics card
			AdapterIndex++;
			continue;
		}

		Hr = D3D12CreateDevice(Adapter, D3D_FEATURE_LEVEL_11_0, _uuidof(ID3D12Device), nullptr);
		if (SUCCEEDED(Hr))
		{
			bAdapterFound = true;
			break;
		}

		AdapterIndex++;
	}

	if (!bAdapterFound)
	{
		MessageBox(0, L"Didn't find an adapter", 0, 0);
		return false;
	}
	OutputDebugString(L"Found an adapter\n");

	Microsoft::WRL::ComPtr<ID3D12Debug> debugController;
	if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
	{
		debugController->EnableDebugLayer();
	}

	// Create the Device
	Hr = D3D12CreateDevice(Adapter, D3D_FEATURE_LEVEL_11_0,	IID_PPV_ARGS(&Device));
	if (FAILED(Hr))
	{
		MessageBox(0, L"Couldn't create the device", 0, 0);
		return false;
	}
	OutputDebugString(L"Device Created\n");

	// Create the RTV Command Queue 
	D3D12_COMMAND_QUEUE_DESC CommandQueueDesc = {};
	Hr = Device->CreateCommandQueue(&CommandQueueDesc, IID_PPV_ARGS(&CommandQueue));
	if (FAILED(Hr))
	{
		MessageBox(0, L"Couldn't create the Command Queue", 0, 0);
		return false;
	}
	OutputDebugString(L"Command Queue Created\n");

	// Create the SwapChain
	DXGI_MODE_DESC BackBufferDesc = {};
	BackBufferDesc.Width = WindowWidth;
	BackBufferDesc.Height = WindowHeight;
	BackBufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;

	DXGI_SAMPLE_DESC SampleDesc = {};
	SampleDesc.Count = 1;

	DXGI_SWAP_CHAIN_DESC SwapChainDesc = {};
	SwapChainDesc.BufferCount = FrameBufferCount;
	SwapChainDesc.BufferDesc = BackBufferDesc;
	SwapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	SwapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	SwapChainDesc.OutputWindow = HWindow;
	SwapChainDesc.SampleDesc = SampleDesc;
	SwapChainDesc.Windowed = !bFullScreen;

	IDXGISwapChain* TempSwapChain;
	Factory->CreateSwapChain(CommandQueue, &SwapChainDesc, &TempSwapChain);

	SwapChain = static_cast<IDXGISwapChain3*>(TempSwapChain);

	FrameIndex = SwapChain->GetCurrentBackBufferIndex();
	OutputDebugString(L"Swap Chain Created\n");

	// Create the Descriptor Heap for the Back Buffers (Render Targets)
	D3D12_DESCRIPTOR_HEAP_DESC RTVHeapDesc = {};
	RTVHeapDesc.NumDescriptors = FrameBufferCount;
	RTVHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	RTVHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE; // Not shader visible

	Hr = Device->CreateDescriptorHeap(&RTVHeapDesc, IID_PPV_ARGS(&RTVDescriptorHeap));
	if (FAILED(Hr))
	{
		MessageBox(0, L"Couldn't create the RTV Descriptor Heap", 0, 0);
		return false;
	}

	RTVDescriptorSize = Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	CD3DX12_CPU_DESCRIPTOR_HANDLE RTVHandle(RTVDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

	for (int i = 0; i < FrameBufferCount; ++i)
	{
		Hr = SwapChain->GetBuffer(i, IID_PPV_ARGS(&RenderTargets[i]));
		if (FAILED(Hr))
		{
			MessageBox(0, L"Couldn't Get the Buffer", 0, 0);
			return false;
		}
		Device->CreateRenderTargetView(RenderTargets[i], nullptr, RTVHandle);
		RTVHandle.Offset(1, RTVDescriptorSize);
	}
	OutputDebugString(L"RTV Handles Created\n");

	// Create the Command Allocators
	for (int i = 0; i < FrameBufferCount; ++i)
	{
		Hr = Device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&CommandAllocators[i]));
		if (FAILED(Hr))
		{
			MessageBox(0, L"Couldn't Create the Command Allocator", 0, 0);
			return false;
		}
	}
	OutputDebugString(L"Command Allocators Created\n");

	// Create the Command list with the first allocator
	Hr = Device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, CommandAllocators[0], NULL, IID_PPV_ARGS(&CommandList));
	if (FAILED(Hr))
	{
		MessageBox(0, L"Couldn't Create the Command List", 0, 0);
		return false;
	}
	//CommandList->Close();
	OutputDebugString(L"Command List Created\n");

	// Create the Fence and Fence Event
	for (int i = 0; i < FrameBufferCount; ++i)
	{
		Hr = Device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&Fences[i]));
		if (FAILED(Hr))
		{
			MessageBox(0, L"Couldn't Create the Fence", 0, 0);
			return false;
		}
		FenceValues[i] = 0;
	}

	FenceEvent = CreateEvent(nullptr, false, false, nullptr);
	if (!FenceEvent)
	{
		MessageBox(0, L"Couldn't Create the Fence Event", 0, 0);
		return false;
	}

	OutputDebugString(L"Fences Created\n");

	// Create the Root Signature
	D3D12_ROOT_DESCRIPTOR RootDescriptor;
	RootDescriptor.RegisterSpace = 0;
	RootDescriptor.ShaderRegister = 0;

	// Create the descriptor Range
	D3D12_DESCRIPTOR_RANGE DescriptorTableRanges[1];
	DescriptorTableRanges[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	DescriptorTableRanges[0].NumDescriptors = 1;
	DescriptorTableRanges[0].BaseShaderRegister = 0;
	DescriptorTableRanges[0].RegisterSpace = 0;
	DescriptorTableRanges[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	// Create the descriptor Table
	D3D12_ROOT_DESCRIPTOR_TABLE DescriptorTable;
	DescriptorTable.NumDescriptorRanges = _countof(DescriptorTableRanges);
	DescriptorTable.pDescriptorRanges = &DescriptorTableRanges[0];

	D3D12_ROOT_PARAMETER RootParameters[2];
	RootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	RootParameters[0].Descriptor = RootDescriptor;
	RootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;

	RootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	RootParameters[1].DescriptorTable = DescriptorTable;
	RootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	// Static Sampler
	D3D12_STATIC_SAMPLER_DESC Sampler = {
		D3D12_FILTER_COMPARISON_MIN_MAG_MIP_POINT, D3D12_TEXTURE_ADDRESS_MODE_BORDER, D3D12_TEXTURE_ADDRESS_MODE_BORDER,
		D3D12_TEXTURE_ADDRESS_MODE_BORDER, 0, 0, D3D12_COMPARISON_FUNC_NEVER,
		D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK, 0.0f, D3D12_FLOAT32_MAX, 0, 0, D3D12_SHADER_VISIBILITY_PIXEL
	};

	CD3DX12_ROOT_SIGNATURE_DESC RootSignatureDescriptor;
	RootSignatureDescriptor.Init(_countof(RootParameters), RootParameters, 1, &Sampler,
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS);

	ID3DBlob* Signature = nullptr;
	Hr = D3D12SerializeRootSignature(&RootSignatureDescriptor, D3D_ROOT_SIGNATURE_VERSION_1, &Signature, nullptr);
	if (FAILED(Hr))
	{
		return false;
	}

	Hr = Device->CreateRootSignature(0, Signature->GetBufferPointer(), Signature->GetBufferSize(), IID_PPV_ARGS(&RootSignature));
	if (FAILED(Hr))
	{
		return false;
	}

	// Create the Vertex and Pixel Shader
	ID3DBlob* VertexShader;
	ID3DBlob* ErrorBuffer;
	Hr = D3DCompileFromFile(L"Shaders/VertexShader.hlsl", nullptr, nullptr, "main", "vs_5_0", D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION, 0, &VertexShader, &ErrorBuffer);
	if (FAILED(Hr))
	{
		OutputDebugStringA((char*)ErrorBuffer->GetBufferPointer());
		return false;
	}

	D3D12_SHADER_BYTECODE VertexShaderBytecode = {};
	VertexShaderBytecode.BytecodeLength = VertexShader->GetBufferSize();
	VertexShaderBytecode.pShaderBytecode = VertexShader->GetBufferPointer();

	ID3DBlob* PixelShader;
	Hr = D3DCompileFromFile(L"Shaders/PixelShader.hlsl", nullptr, nullptr, "main", "ps_5_0", D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION, 0, &PixelShader, &ErrorBuffer);
	if (FAILED(Hr))
	{
		OutputDebugStringA((char*)ErrorBuffer->GetBufferPointer());
		return false;
	}

	D3D12_SHADER_BYTECODE PixelShaderBytecode = {};
	PixelShaderBytecode.BytecodeLength = PixelShader->GetBufferSize();
	PixelShaderBytecode.pShaderBytecode = PixelShader->GetBufferPointer();

	// Create an input layout
	D3D12_INPUT_ELEMENT_DESC InputLayout[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};

	D3D12_INPUT_LAYOUT_DESC InputLayoutDesc = {};
	InputLayoutDesc.NumElements = sizeof(InputLayout) / sizeof(D3D12_INPUT_ELEMENT_DESC);
	InputLayoutDesc.pInputElementDescs = InputLayout;

	// Create a PSO
	D3D12_GRAPHICS_PIPELINE_STATE_DESC PSODesc = {};
	PSODesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT); // default depth/stencil state
	PSODesc.InputLayout = InputLayoutDesc;
	PSODesc.pRootSignature = RootSignature;
	PSODesc.VS = VertexShaderBytecode;
	PSODesc.PS = PixelShaderBytecode;
	PSODesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	PSODesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	PSODesc.SampleDesc = SampleDesc;
	PSODesc.SampleMask = 0xffffffff;
	PSODesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	PSODesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	PSODesc.NumRenderTargets = 1;

	Hr = Device->CreateGraphicsPipelineState(&PSODesc, IID_PPV_ARGS(&PSO));
	if (FAILED(Hr))
	{
		return false;
	}

	// Create the meshes and the camera for the scene
	CCube* Cube = new CCube;
	Mesh = Cube;
	Mesh->Init(Device, CommandList);

	SceneCamera = new Camera;

	// Depth / Stencil 

	// Create the Depth/Stencil descriptor heap
	D3D12_DESCRIPTOR_HEAP_DESC DepthStencilViewHeapDesc = {};
	DepthStencilViewHeapDesc.NumDescriptors = 1;
	DepthStencilViewHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	DepthStencilViewHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	Hr = Device->CreateDescriptorHeap(&DepthStencilViewHeapDesc, IID_PPV_ARGS(&DepthStencilDescriptorHeap));
	if (FAILED(Hr))
	{
		return false;
	}

	D3D12_DEPTH_STENCIL_VIEW_DESC DepthStencilDesc = {};
	DepthStencilDesc.Format = DXGI_FORMAT_D32_FLOAT;
	DepthStencilDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	DepthStencilDesc.Flags = D3D12_DSV_FLAG_NONE;

	// the value we want when the depth/stencil is clear
	D3D12_CLEAR_VALUE DepthClearValue = {};
	DepthClearValue.Format = DXGI_FORMAT_D32_FLOAT;
	DepthClearValue.DepthStencil.Depth = 1.0f;
	DepthClearValue.DepthStencil.Stencil = 0;

	CD3DX12_HEAP_PROPERTIES HeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	CD3DX12_RESOURCE_DESC Texture = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_D32_FLOAT, WindowWidth, WindowHeight, 1, 0, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);
	Device->CreateCommittedResource(
		&HeapProperties,
		D3D12_HEAP_FLAG_NONE,
		&Texture,
		D3D12_RESOURCE_STATE_DEPTH_WRITE,
		&DepthClearValue,
		IID_PPV_ARGS(&DepthStencilBuffer)
	);
	Hr = Device->CreateDescriptorHeap(&DepthStencilViewHeapDesc, IID_PPV_ARGS(&DepthStencilDescriptorHeap));
	if (FAILED(Hr))
	{
		return false;
	}
	DepthStencilDescriptorHeap->SetName(L"Depth/Stencil Resource Heap");

	Device->CreateDepthStencilView(DepthStencilBuffer, &DepthStencilDesc, DepthStencilDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

	for (int i = 0; i < FRAMEBUFFER_COUNT; i++)
	{
		CD3DX12_HEAP_PROPERTIES BufferHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
		CD3DX12_RESOURCE_DESC Buffer = CD3DX12_RESOURCE_DESC::Buffer(1024 * 64);
		Hr = Device->CreateCommittedResource(&BufferHeapProperties, D3D12_HEAP_FLAG_NONE,
		                                     &Buffer,
		                                     D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
		                                     IID_PPV_ARGS(&ConstantBufferUploadHeaps[i]));
		ZeroMemory(&ConstantBuffer, sizeof(ConstantBuffer));
		CD3DX12_RANGE ReadRange(0, 0);

		Hr = ConstantBufferUploadHeaps[i]->Map(0, &ReadRange, reinterpret_cast<void**>(&ConstantBufferGPUAdress[i]));

		memcpy(ConstantBufferGPUAdress[i], &ConstantBuffer, sizeof(ConstantBuffer));
	}

#pragma region Texture

	// Load the image from a file
	D3D12_RESOURCE_DESC TextureDescriptor = {};
	int ImageBytesPerRow = 0;
	BYTE* ImageData;
	int ImageSize = LoadImageDataFromFile(&ImageData, TextureDescriptor, L"Texture.jpg", ImageBytesPerRow);

	if (ImageSize <= 0)
	{
        OutputDebugString(L"ImageSize is not valid\n");
        return false;
	}

	CD3DX12_HEAP_PROPERTIES TextureHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	Hr = Device->CreateCommittedResource(&TextureHeapProperties, D3D12_HEAP_FLAG_NONE, &TextureDescriptor,
	                                     D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&TextureBuffer));
	if (FAILED(Hr))
	{
		return false;
	}
	TextureBuffer->SetName(L"Texture Buffer resource Heap");

	// Upload heap to upload the texture
	UINT64 TextureUploadBufferSize;
	Device->GetCopyableFootprints(&TextureDescriptor, 0, 1, 0, nullptr, nullptr, nullptr, &TextureUploadBufferSize);

	CD3DX12_HEAP_PROPERTIES TextureUploadHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	CD3DX12_RESOURCE_DESC TextureUploadHeapResourceDescriptor = CD3DX12_RESOURCE_DESC::Buffer(TextureUploadBufferSize);
	Hr = Device->CreateCommittedResource(&TextureUploadHeapProperties, D3D12_HEAP_FLAG_NONE,
	                                     &TextureUploadHeapResourceDescriptor, D3D12_RESOURCE_STATE_GENERIC_READ,
	                                     nullptr, IID_PPV_ARGS(&TextureBufferUploadHeap));
	if (FAILED(Hr))
	{
		return false;
	}
	TextureBufferUploadHeap->SetName(L"Texture Buffer Upload Resource Heap");

	// Store the texture in an upload heap
	D3D12_SUBRESOURCE_DATA TextureData = {};
	TextureData.pData = &ImageData[0];
	TextureData.RowPitch = ImageBytesPerRow;
	TextureData.SlicePitch = ImageBytesPerRow * TextureDescriptor.Height;

    UpdateSubresources(CommandList, TextureBuffer, TextureBufferUploadHeap, 0, 0, 1, &TextureData);
	CD3DX12_RESOURCE_BARRIER Barrier = CD3DX12_RESOURCE_BARRIER::Transition(TextureBuffer, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    CommandList->ResourceBarrier(1, &Barrier);

    D3D12_DESCRIPTOR_HEAP_DESC HeapDescriptor = {};
    HeapDescriptor.NumDescriptors = 1;
    HeapDescriptor.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    HeapDescriptor.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    Hr = Device->CreateDescriptorHeap(&HeapDescriptor, IID_PPV_ARGS(&MainDescriptorHeap));
    if (FAILED(Hr))
    {
        return false;
    }

    // Create the shader resource view
	D3D12_SHADER_RESOURCE_VIEW_DESC SrvDesc = {};
	SrvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	SrvDesc.Format = TextureDescriptor.Format;
	SrvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	SrvDesc.Texture2D.MipLevels = 1;
	Device->CreateShaderResourceView(TextureBuffer, &SrvDesc, MainDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

#pragma endregion Texture

	CommandList->Close();

	ID3D12CommandList* ppCommandLists[] = { CommandList };
	CommandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

	FenceValues[FrameIndex]++;
	Hr = CommandQueue->Signal(Fences[FrameIndex], FenceValues[FrameIndex]);
	if (FAILED(Hr))
	{
		return false;
	}

	// We can delete the image data now that we are done using it
	delete ImageData;

	Mesh->VertexBufferView.BufferLocation = Mesh->VertexBuffer->GetGPUVirtualAddress();
	Mesh->VertexBufferView.StrideInBytes = sizeof(Vertex);
	Mesh->VertexBufferView.SizeInBytes = Mesh->GetVertexBufferSize();

	Mesh->IndexBufferView.BufferLocation = Mesh->IndexBuffer->GetGPUVirtualAddress();
	Mesh->IndexBufferView.Format = DXGI_FORMAT_R32_UINT;
	Mesh->IndexBufferView.SizeInBytes = Mesh->GetIndexBufferSize();

	// Fill out the Viewport
	Viewport.TopLeftX = 0;
	Viewport.TopLeftY = 0;
	Viewport.Width = WindowWidth;
	Viewport.Height = WindowHeight;
	Viewport.MinDepth = 0.0f;
	Viewport.MaxDepth = 1.0f;

	// Fill out a scissor rect
	ScissorRect.left = 0;
	ScissorRect.top = 0;
	ScissorRect.right = WindowWidth;
	ScissorRect.bottom = WindowHeight;

	return true;
}

void CRenderer::Update()
{
	// Rotate the mesh
	DirectX::XMFLOAT3 Rotation = Mesh->GetRotation();
	Rotation.y += 0.01f;
	Rotation.x += 0.001f;
	Mesh->SetRotation(Rotation);

	// update constant buffer
	DirectX::XMMATRIX ViewMatrix = DirectX::XMLoadFloat4x4(&SceneCamera->ViewMatrix);
	DirectX::XMMATRIX ProjMatrix = DirectX::XMLoadFloat4x4(&SceneCamera->ProjectionMatrix);
	XMFLOAT4X4 WorldMat = Mesh->GetWorldMatrix();
	DirectX::XMMATRIX WVPMatrix = DirectX::XMLoadFloat4x4(&WorldMat) * ViewMatrix * ProjMatrix;
	DirectX::XMMATRIX Transposed = DirectX::XMMatrixTranspose(WVPMatrix);
	DirectX::XMStoreFloat4x4(&ConstantBuffer.WorldViewProj, Transposed);

	memcpy(ConstantBufferGPUAdress[FrameIndex], &ConstantBuffer, sizeof(ConstantBuffer));

}

void CRenderer::UpdatePipeline()
{
	WaitForPreviousFrame();
	HRESULT Hr = CommandAllocators[FrameIndex]->Reset();
	if (FAILED(Hr))
	{
		MessageBox(nullptr, L"Couldn't Reset the Allocator", nullptr, 0);
		bRunning = false;
	}

	Hr = CommandList->Reset(CommandAllocators[FrameIndex], PSO);
	if (FAILED(Hr))
	{
		MessageBox(nullptr, L"Couldn't Reset the Command List", nullptr, 0);
		bRunning = false;
	}

	// Start recording commands here

	// We create a resource Barrier to transition from present to render target state and we get a handle to the current RTV
	CD3DX12_RESOURCE_BARRIER Barrier = CD3DX12_RESOURCE_BARRIER::Transition(RenderTargets[FrameIndex], D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
	CommandList->ResourceBarrier(1, &Barrier);

	const CD3DX12_CPU_DESCRIPTOR_HANDLE RTVHandle(RTVDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), FrameIndex, RTVDescriptorSize);
	const CD3DX12_CPU_DESCRIPTOR_HANDLE DepthStencilHandle(DepthStencilDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

	CommandList->OMSetRenderTargets(1, &RTVHandle, false, &DepthStencilHandle);

	// Clear the render target to the desired color
	const float ClearColor[] = { 0.0f, 0.2f, 0.4f, 1.0f };
	CommandList->ClearRenderTargetView(RTVHandle, ClearColor, 0, nullptr);

	// Clear the depth buffer
	CommandList->ClearDepthStencilView(DepthStencilDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

	// Set the root signature
	CommandList->SetGraphicsRootSignature(RootSignature);

	// Set the descriptor heap
	ID3D12DescriptorHeap* DescriptorHeaps[] = { MainDescriptorHeap };
	CommandList->SetDescriptorHeaps(_countof(DescriptorHeaps), DescriptorHeaps);

	// Set the graphics root descriptor table
	CommandList->SetGraphicsRootDescriptorTable(1, MainDescriptorHeap->GetGPUDescriptorHandleForHeapStart());

	CommandList->RSSetViewports(1, &Viewport);
	CommandList->RSSetScissorRects(1, &ScissorRect);

	CommandList->SetGraphicsRootConstantBufferView(0, ConstantBufferUploadHeaps[FrameIndex]->GetGPUVirtualAddress());

	Mesh->Draw(CommandList);

	// Transition back to present
	Barrier = CD3DX12_RESOURCE_BARRIER::Transition(RenderTargets[FrameIndex], D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
	CommandList->ResourceBarrier(1, &Barrier);

	Hr = CommandList->Close();
	if (FAILED(Hr))
	{
		MessageBox(nullptr, L"Couldn't close the Command List", 0, 0);
		bRunning = false;
	}
}

void CRenderer::Render() 
{
	UpdatePipeline();

	ID3D12CommandList* ppCommandLists[] = { CommandList };
	CommandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

	HRESULT Hr = CommandQueue->Signal(Fences[FrameIndex], FenceValues[FrameIndex]);
	if (FAILED(Hr))
	{
		bRunning = false;
	}

	Hr = SwapChain->Present(0, 0);
	if (FAILED(Hr))
	{
		bRunning = false;
	}
}

void CRenderer::Cleanup()
{
	// Wait for everybody to finish
	for (int i = 0;  i < FrameBufferCount;  ++i)
	{
		FrameIndex = i;
		WaitForPreviousFrame();
	}

	// Get SwapChain out of fullscreen before exiting
	BOOL bFS = false;
	if (SwapChain->GetFullscreenState(&bFS, nullptr))
	{
		SwapChain->SetFullscreenState(false, nullptr);
	}

	SAFE_RELEASE(Device);
	SAFE_RELEASE(SwapChain);
	SAFE_RELEASE(CommandQueue);
	SAFE_RELEASE(RTVDescriptorHeap);
	SAFE_RELEASE(CommandList);
	SAFE_RELEASE(PSO);
	SAFE_RELEASE(RootSignature);
	SAFE_RELEASE(DepthStencilBuffer);
	SAFE_RELEASE(DepthStencilDescriptorHeap);

	for (int i = 0; i < FrameBufferCount; ++i)
	{
		SAFE_RELEASE(RenderTargets[i]);
		SAFE_RELEASE(CommandAllocators[i]);
		SAFE_RELEASE(Fences[i]);
		SAFE_RELEASE(ConstantBufferUploadHeaps[i]);
	}

	delete Mesh;
	delete SceneCamera;
}

void CRenderer::WaitForPreviousFrame()
{
	HRESULT Hr;

	FrameIndex = SwapChain->GetCurrentBackBufferIndex();
	
	if (Fences[FrameIndex]->GetCompletedValue() < FenceValues[FrameIndex])
	{
		Hr = Fences[FrameIndex]->SetEventOnCompletion(FenceValues[FrameIndex], FenceEvent);
		if (FAILED(Hr))
		{
			bRunning = false;
		}

		WaitForSingleObject(FenceEvent, INFINITE);
	}

	FenceValues[FrameIndex]++;
}

// get the dxgi format equivilent of a wic format
DXGI_FORMAT CRenderer::GetDXGIFormatFromWICFormat(WICPixelFormatGUID& WicFormatGUID)
{
    if (WicFormatGUID == GUID_WICPixelFormat128bppRGBAFloat) return DXGI_FORMAT_R32G32B32A32_FLOAT;
    if (WicFormatGUID == GUID_WICPixelFormat64bppRGBAHalf) return DXGI_FORMAT_R16G16B16A16_FLOAT;
    if (WicFormatGUID == GUID_WICPixelFormat64bppRGBA) return DXGI_FORMAT_R16G16B16A16_UNORM;
    if (WicFormatGUID == GUID_WICPixelFormat32bppRGBA) return DXGI_FORMAT_R8G8B8A8_UNORM;
    if (WicFormatGUID == GUID_WICPixelFormat32bppBGRA) return DXGI_FORMAT_B8G8R8A8_UNORM;
    if (WicFormatGUID == GUID_WICPixelFormat32bppBGR) return DXGI_FORMAT_B8G8R8X8_UNORM;
    if (WicFormatGUID == GUID_WICPixelFormat32bppRGBA1010102XR) return DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM;

    if (WicFormatGUID == GUID_WICPixelFormat32bppRGBA1010102) return DXGI_FORMAT_R10G10B10A2_UNORM;
    if (WicFormatGUID == GUID_WICPixelFormat16bppBGRA5551) return DXGI_FORMAT_B5G5R5A1_UNORM;
    if (WicFormatGUID == GUID_WICPixelFormat16bppBGR565) return DXGI_FORMAT_B5G6R5_UNORM;
    if (WicFormatGUID == GUID_WICPixelFormat32bppGrayFloat) return DXGI_FORMAT_R32_FLOAT;
    if (WicFormatGUID == GUID_WICPixelFormat16bppGrayHalf) return DXGI_FORMAT_R16_FLOAT;
    if (WicFormatGUID == GUID_WICPixelFormat16bppGray) return DXGI_FORMAT_R16_UNORM;
    if (WicFormatGUID == GUID_WICPixelFormat8bppGray) return DXGI_FORMAT_R8_UNORM;
    if (WicFormatGUID == GUID_WICPixelFormat8bppAlpha) return DXGI_FORMAT_A8_UNORM;

    return DXGI_FORMAT_UNKNOWN;
}

// get a dxgi compatible wic format from another wic format
WICPixelFormatGUID CRenderer::GetConvertToWICFormat(WICPixelFormatGUID& WicFormatGUID)
{
    if (WicFormatGUID == GUID_WICPixelFormatBlackWhite) return GUID_WICPixelFormat8bppGray;
    if (WicFormatGUID == GUID_WICPixelFormat1bppIndexed) return GUID_WICPixelFormat32bppRGBA;
    if (WicFormatGUID == GUID_WICPixelFormat2bppIndexed) return GUID_WICPixelFormat32bppRGBA;
    if (WicFormatGUID == GUID_WICPixelFormat4bppIndexed) return GUID_WICPixelFormat32bppRGBA;
    if (WicFormatGUID == GUID_WICPixelFormat8bppIndexed) return GUID_WICPixelFormat32bppRGBA;
    if (WicFormatGUID == GUID_WICPixelFormat2bppGray) return GUID_WICPixelFormat8bppGray;
    if (WicFormatGUID == GUID_WICPixelFormat4bppGray) return GUID_WICPixelFormat8bppGray;
    if (WicFormatGUID == GUID_WICPixelFormat16bppGrayFixedPoint) return GUID_WICPixelFormat16bppGrayHalf;
    if (WicFormatGUID == GUID_WICPixelFormat32bppGrayFixedPoint) return GUID_WICPixelFormat32bppGrayFloat;
    if (WicFormatGUID == GUID_WICPixelFormat16bppBGR555) return GUID_WICPixelFormat16bppBGRA5551;
    if (WicFormatGUID == GUID_WICPixelFormat32bppBGR101010) return GUID_WICPixelFormat32bppRGBA1010102;
    if (WicFormatGUID == GUID_WICPixelFormat24bppBGR) return GUID_WICPixelFormat32bppRGBA;
    if (WicFormatGUID == GUID_WICPixelFormat24bppRGB) return GUID_WICPixelFormat32bppRGBA;
    if (WicFormatGUID == GUID_WICPixelFormat32bppPBGRA) return GUID_WICPixelFormat32bppRGBA;
    if (WicFormatGUID == GUID_WICPixelFormat32bppPRGBA) return GUID_WICPixelFormat32bppRGBA;
    if (WicFormatGUID == GUID_WICPixelFormat48bppRGB) return GUID_WICPixelFormat64bppRGBA;
    if (WicFormatGUID == GUID_WICPixelFormat48bppBGR) return GUID_WICPixelFormat64bppRGBA;
    if (WicFormatGUID == GUID_WICPixelFormat64bppBGRA) return GUID_WICPixelFormat64bppRGBA;
    if (WicFormatGUID == GUID_WICPixelFormat64bppPRGBA) return GUID_WICPixelFormat64bppRGBA;
    if (WicFormatGUID == GUID_WICPixelFormat64bppPBGRA) return GUID_WICPixelFormat64bppRGBA;
    if (WicFormatGUID == GUID_WICPixelFormat48bppRGBFixedPoint) return GUID_WICPixelFormat64bppRGBAHalf;
    if (WicFormatGUID == GUID_WICPixelFormat48bppBGRFixedPoint) return GUID_WICPixelFormat64bppRGBAHalf;
    if (WicFormatGUID == GUID_WICPixelFormat64bppRGBAFixedPoint) return GUID_WICPixelFormat64bppRGBAHalf;
    if (WicFormatGUID == GUID_WICPixelFormat64bppBGRAFixedPoint) return GUID_WICPixelFormat64bppRGBAHalf;
    if (WicFormatGUID == GUID_WICPixelFormat64bppRGBFixedPoint) return GUID_WICPixelFormat64bppRGBAHalf;
    if (WicFormatGUID == GUID_WICPixelFormat64bppRGBHalf) return GUID_WICPixelFormat64bppRGBAHalf;
    if (WicFormatGUID == GUID_WICPixelFormat48bppRGBHalf) return GUID_WICPixelFormat64bppRGBAHalf;
    if (WicFormatGUID == GUID_WICPixelFormat128bppPRGBAFloat) return GUID_WICPixelFormat128bppRGBAFloat;
    if (WicFormatGUID == GUID_WICPixelFormat128bppRGBFloat) return GUID_WICPixelFormat128bppRGBAFloat;
    if (WicFormatGUID == GUID_WICPixelFormat128bppRGBAFixedPoint) return GUID_WICPixelFormat128bppRGBAFloat;
    if (WicFormatGUID == GUID_WICPixelFormat128bppRGBFixedPoint) return GUID_WICPixelFormat128bppRGBAFloat;
    if (WicFormatGUID == GUID_WICPixelFormat32bppRGBE) return GUID_WICPixelFormat128bppRGBAFloat;
    if (WicFormatGUID == GUID_WICPixelFormat32bppCMYK) return GUID_WICPixelFormat32bppRGBA;
    if (WicFormatGUID == GUID_WICPixelFormat64bppCMYK) return GUID_WICPixelFormat64bppRGBA;
    if (WicFormatGUID == GUID_WICPixelFormat40bppCMYKAlpha) return GUID_WICPixelFormat64bppRGBA;
    if (WicFormatGUID == GUID_WICPixelFormat80bppCMYKAlpha) return GUID_WICPixelFormat64bppRGBA;

#if (_WIN32_WINNT >= _WIN32_WINNT_WIN8) || defined(_WIN7_PLATFORM_UPDATE)
    if (WicFormatGUID == GUID_WICPixelFormat32bppRGB) return GUID_WICPixelFormat32bppRGBA;
    if (WicFormatGUID == GUID_WICPixelFormat64bppRGB) return GUID_WICPixelFormat64bppRGBA;
    if (WicFormatGUID == GUID_WICPixelFormat64bppPRGBAHalf) return GUID_WICPixelFormat64bppRGBAHalf;
#endif

    return GUID_WICPixelFormatDontCare;
}

// get the number of bits per pixel for a dxgi format
int CRenderer::GetDXGIFormatBitsPerPixel(DXGI_FORMAT& DxgiFormat)
{
    if (DxgiFormat == DXGI_FORMAT_R32G32B32A32_FLOAT) return 128;
    if (DxgiFormat == DXGI_FORMAT_R16G16B16A16_FLOAT) return 64;
    if (DxgiFormat == DXGI_FORMAT_R16G16B16A16_UNORM) return 64;
    if (DxgiFormat == DXGI_FORMAT_R8G8B8A8_UNORM) return 32;
    if (DxgiFormat == DXGI_FORMAT_B8G8R8A8_UNORM) return 32;
    if (DxgiFormat == DXGI_FORMAT_B8G8R8X8_UNORM) return 32;
    if (DxgiFormat == DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM) return 32;

    if (DxgiFormat == DXGI_FORMAT_R10G10B10A2_UNORM) return 32;
    if (DxgiFormat == DXGI_FORMAT_B5G5R5A1_UNORM) return 16;
    if (DxgiFormat == DXGI_FORMAT_B5G6R5_UNORM) return 16;
    if (DxgiFormat == DXGI_FORMAT_R32_FLOAT) return 32;
    if (DxgiFormat == DXGI_FORMAT_R16_FLOAT) return 16;
    if (DxgiFormat == DXGI_FORMAT_R16_UNORM) return 16;
    if (DxgiFormat == DXGI_FORMAT_R8_UNORM) return 8;
    if (DxgiFormat == DXGI_FORMAT_A8_UNORM) return 8;
	return 0;
}

// load and decode image from file
int CRenderer::LoadImageDataFromFile(BYTE** ImageData, D3D12_RESOURCE_DESC& ResourceDescription, LPCWSTR Filename, int& BytesPerRow)
{
    HRESULT Hr;

    // we only need one instance of the imaging factory to create decoders and frames
    static IWICImagingFactory* WicFactory;

    // reset decoder, frame and converter since these will be different for each image we load
    IWICBitmapDecoder* WicDecoder = NULL;
    IWICBitmapFrameDecode* WicFrame = NULL;
    IWICFormatConverter* WicConverter = NULL;

    bool bImageConverted = false;

    if (WicFactory == NULL)
    {
        // Initialize the COM library
        CoInitialize(NULL);

        // create the WIC factory
        Hr = CoCreateInstance(
            CLSID_WICImagingFactory,
            NULL,
            CLSCTX_INPROC_SERVER,
            IID_PPV_ARGS(&WicFactory)
        );
        if (FAILED(Hr)) return 0;
    }

    // load a decoder for the image
    Hr = WicFactory->CreateDecoderFromFilename(
        Filename,                        // Image we want to load in
        NULL,                            // This is a vendor ID, we do not prefer a specific one so set to null
        GENERIC_READ,                    // We want to read from this file
        WICDecodeMetadataCacheOnLoad,    // We will cache the metadata right away, rather than when needed, which might be unknown
        &WicDecoder                      // the wic decoder to be created
    );
    if (FAILED(Hr)) return 0;

    // get image from decoder (this will decode the "frame")
    Hr = WicDecoder->GetFrame(0, &WicFrame);
    if (FAILED(Hr)) return 0;

    // get wic pixel format of image
    WICPixelFormatGUID PixelFormat;
    Hr = WicFrame->GetPixelFormat(&PixelFormat);
    if (FAILED(Hr)) return 0;

    // get size of image
    UINT TextureWidth, TextureHeight;
    Hr = WicFrame->GetSize(&TextureWidth, &TextureHeight);
    if (FAILED(Hr)) return 0;

    // we are not handling sRGB types in this tutorial, so if you need that support, you'll have to figure
    // out how to implement the support yourself

    // convert wic pixel format to dxgi pixel format
    DXGI_FORMAT DxgiFormat = GetDXGIFormatFromWICFormat(PixelFormat);

    // if the format of the image is not a supported dxgi format, try to convert it
    if (DxgiFormat == DXGI_FORMAT_UNKNOWN)
    {
        // get a dxgi compatible wic format from the current image format
        WICPixelFormatGUID ConvertToPixelFormat = GetConvertToWICFormat(PixelFormat);

        // return if no dxgi compatible format was found
        if (ConvertToPixelFormat == GUID_WICPixelFormatDontCare) return 0;

        // set the dxgi format
        DxgiFormat = GetDXGIFormatFromWICFormat(ConvertToPixelFormat);

        // create the format converter
        Hr = WicFactory->CreateFormatConverter(&WicConverter);
        if (FAILED(Hr)) return 0;

        // make sure we can convert to the dxgi compatible format
        BOOL bCanConvert = FALSE;
        Hr = WicConverter->CanConvert(PixelFormat, ConvertToPixelFormat, &bCanConvert);
        if (FAILED(Hr) || !bCanConvert) return 0;

        // do the conversion (wicConverter will contain the converted image)
        Hr = WicConverter->Initialize(WicFrame, ConvertToPixelFormat, WICBitmapDitherTypeErrorDiffusion, 0, 0, WICBitmapPaletteTypeCustom);
        if (FAILED(Hr)) return 0;

        // this is so we know to get the image data from the wicConverter (otherwise we will get from wicFrame)
        bImageConverted = true;
    }

    int BitsPerPixel = GetDXGIFormatBitsPerPixel(DxgiFormat); // number of bits per pixel
    BytesPerRow = (TextureWidth * BitsPerPixel) / 8; // number of bytes in each row of the image data
    int ImageSize = BytesPerRow * TextureHeight; // total image size in bytes

    // allocate enough memory for the raw image data, and set imageData to point to that memory
    *ImageData = (BYTE*)malloc(ImageSize);

    // copy (decoded) raw image data into the newly allocated memory (imageData)
    if (bImageConverted)
    {
        // if image format needed to be converted, the wic converter will contain the converted image
        Hr = WicConverter->CopyPixels(0, BytesPerRow, ImageSize, *ImageData);
        if (FAILED(Hr)) return 0;
    }
    else
    {
        // no need to convert, just copy data from the wic frame
        Hr = WicFrame->CopyPixels(0, BytesPerRow, ImageSize, *ImageData);
        if (FAILED(Hr)) return 0;
    }

    // now describe the texture with the information we have obtained from the image
    ResourceDescription = {};
    ResourceDescription.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    ResourceDescription.Alignment = 0; // may be 0, 4KB, 64KB, or 4MB. 0 will let runtime decide between 64KB and 4MB (4MB for multi-sampled textures)
    ResourceDescription.Width = TextureWidth; // width of the texture
    ResourceDescription.Height = TextureHeight; // height of the texture
    ResourceDescription.DepthOrArraySize = 1; // if 3d image, depth of 3d image. Otherwise an array of 1D or 2D textures (we only have one image, so we set 1)
    ResourceDescription.MipLevels = 1; // Number of mipmaps. We are not generating mipmaps for this texture, so we have only one level
    ResourceDescription.Format = DxgiFormat; // This is the dxgi format of the image (format of the pixels)
    ResourceDescription.SampleDesc.Count = 1; // This is the number of samples per pixel, we just want 1 sample
    ResourceDescription.SampleDesc.Quality = 0; // The quality level of the samples. Higher is better quality, but worse performance
    ResourceDescription.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN; // The arrangement of the pixels. Setting to unknown lets the driver choose the most efficient one
    ResourceDescription.Flags = D3D12_RESOURCE_FLAG_NONE; // no flags

    // return the size of the image. remember to delete the image once your done with it (in this tutorial once its uploaded to the gpu)
    return ImageSize;
}
