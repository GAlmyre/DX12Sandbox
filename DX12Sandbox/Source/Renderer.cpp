#include "pch.h"
#include "Renderer.h"
#include "Mesh.h"
#include "CCube.h"
#include "Camera.h"
#include <shlobj.h>
#include <strsafe.h>


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

	D3D12_ROOT_PARAMETER RootParameters[1];
	RootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	RootParameters[0].Descriptor = RootDescriptor;
	RootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;

	CD3DX12_ROOT_SIGNATURE_DESC RootSignatureDescriptor;
	RootSignatureDescriptor.Init(_countof(RootParameters), RootParameters, 0, nullptr,
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS);

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
		{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
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

	CommandList->Close();

	ID3D12CommandList* ppCommandLists[] = { CommandList };
	CommandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

	FenceValues[FrameIndex]++;
	Hr = CommandQueue->Signal(Fences[FrameIndex], FenceValues[FrameIndex]);
	if (FAILED(Hr))
	{
		return false;
	}

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

#pragma region MeshPosition
	// Cube position
	MeshPosition = DirectX::XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f);
	DirectX::XMVECTOR MeshPos = XMLoadFloat4(&MeshPosition);

	DirectX::XMMATRIX TranslationMatrix = DirectX::XMMatrixTranslationFromVector(MeshPos);
	XMStoreFloat4x4(&MeshRotMat, DirectX::XMMatrixIdentity());
	XMStoreFloat4x4(&MeshWorldMat, TranslationMatrix);

#pragma endregion

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
