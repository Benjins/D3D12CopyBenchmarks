
#include <stdio.h>

#include <stdint.h>

#include <vector>
#include <thread>
#include <limits>
#include <mutex>

#include <Windows.h>

#include <assert.h>

#include <d3d12.h>

#include <d3dcompiler.h>

#include <dxgi1_2.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

struct CD3DX12_RESOURCE_DESC : public D3D12_RESOURCE_DESC
{
	CD3DX12_RESOURCE_DESC() = default;
	explicit CD3DX12_RESOURCE_DESC(const D3D12_RESOURCE_DESC& o) :
		D3D12_RESOURCE_DESC(o)
	{}
	CD3DX12_RESOURCE_DESC(
		D3D12_RESOURCE_DIMENSION dimension,
		UINT64 alignment,
		UINT64 width,
		UINT height,
		UINT16 depthOrArraySize,
		UINT16 mipLevels,
		DXGI_FORMAT format,
		UINT sampleCount,
		UINT sampleQuality,
		D3D12_TEXTURE_LAYOUT layout,
		D3D12_RESOURCE_FLAGS flags)
	{
		Dimension = dimension;
		Alignment = alignment;
		Width = width;
		Height = height;
		DepthOrArraySize = depthOrArraySize;
		MipLevels = mipLevels;
		Format = format;
		SampleDesc.Count = sampleCount;
		SampleDesc.Quality = sampleQuality;
		Layout = layout;
		Flags = flags;
	}
	static inline CD3DX12_RESOURCE_DESC Buffer(
		const D3D12_RESOURCE_ALLOCATION_INFO& resAllocInfo,
		D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE)
	{
		return CD3DX12_RESOURCE_DESC(D3D12_RESOURCE_DIMENSION_BUFFER, resAllocInfo.Alignment, resAllocInfo.SizeInBytes,
			1, 1, 1, DXGI_FORMAT_UNKNOWN, 1, 0, D3D12_TEXTURE_LAYOUT_ROW_MAJOR, flags);
	}
	static inline CD3DX12_RESOURCE_DESC Buffer(
		UINT64 width,
		D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE,
		UINT64 alignment = 0)
	{
		return CD3DX12_RESOURCE_DESC(D3D12_RESOURCE_DIMENSION_BUFFER, alignment, width, 1, 1, 1,
			DXGI_FORMAT_UNKNOWN, 1, 0, D3D12_TEXTURE_LAYOUT_ROW_MAJOR, flags);
	}
	static inline CD3DX12_RESOURCE_DESC Tex1D(
		DXGI_FORMAT format,
		UINT64 width,
		UINT16 arraySize = 1,
		UINT16 mipLevels = 0,
		D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE,
		D3D12_TEXTURE_LAYOUT layout = D3D12_TEXTURE_LAYOUT_UNKNOWN,
		UINT64 alignment = 0)
	{
		return CD3DX12_RESOURCE_DESC(D3D12_RESOURCE_DIMENSION_TEXTURE1D, alignment, width, 1, arraySize,
			mipLevels, format, 1, 0, layout, flags);
	}
	static inline CD3DX12_RESOURCE_DESC Tex2D(
		DXGI_FORMAT format,
		UINT64 width,
		UINT height,
		UINT16 arraySize = 1,
		UINT16 mipLevels = 0,
		UINT sampleCount = 1,
		UINT sampleQuality = 0,
		D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE,
		D3D12_TEXTURE_LAYOUT layout = D3D12_TEXTURE_LAYOUT_UNKNOWN,
		UINT64 alignment = 0)
	{
		return CD3DX12_RESOURCE_DESC(D3D12_RESOURCE_DIMENSION_TEXTURE2D, alignment, width, height, arraySize,
			mipLevels, format, sampleCount, sampleQuality, layout, flags);
	}
	static inline CD3DX12_RESOURCE_DESC Tex3D(
		DXGI_FORMAT format,
		UINT64 width,
		UINT height,
		UINT16 depth,
		UINT16 mipLevels = 0,
		D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE,
		D3D12_TEXTURE_LAYOUT layout = D3D12_TEXTURE_LAYOUT_UNKNOWN,
		UINT64 alignment = 0)
	{
		return CD3DX12_RESOURCE_DESC(D3D12_RESOURCE_DIMENSION_TEXTURE3D, alignment, width, height, depth,
			mipLevels, format, 1, 0, layout, flags);
	}
	inline UINT16 Depth() const
	{
		return (Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE3D ? DepthOrArraySize : 1);
	}
	inline UINT16 ArraySize() const
	{
		return (Dimension != D3D12_RESOURCE_DIMENSION_TEXTURE3D ? DepthOrArraySize : 1);
	}
};


typedef int32_t int32;

#define ASSERT assert


#pragma comment(lib, "DXGI.lib")
#pragma comment(lib, "D3D12.lib")
#pragma comment(lib, "d3dcompiler.lib")

const char* VertexShaderCode =
"struct PSInput { float4 pos : SV_POSITION; };\n"
"PSInput VSMain(float4 position : POSITION) {\n"
"	PSInput res;\n"
"	res.pos = position;\n"
"	return res;"
"}";


const char* PixelShaderCode =
"struct PSInput { float4 pos : SV_POSITION; };\n"
"SamplerState TexSampler;\n"
"Texture2D inputTexture;\n"
"float4 PSMain(PSInput input) : SV_TARGET {\n"
"	return inputTexture.SampleLevel(TexSampler, ((input.pos.xy) / 1024.0), 0);\n"
"}";


#define LOG(msg, ...) do { char OtherStuff[1024] = {}; \
		snprintf(OtherStuff, sizeof(OtherStuff), msg "\n", ## __VA_ARGS__); \
		OutputDebugStringA(OtherStuff); \
	} while(0)

D3D12_RASTERIZER_DESC GetDefaultRasterizerDesc() {
	D3D12_RASTERIZER_DESC Desc = {};
	Desc.FillMode = D3D12_FILL_MODE_SOLID;
	Desc.CullMode = D3D12_CULL_MODE_NONE;
	Desc.FrontCounterClockwise = FALSE;
	Desc.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
	Desc.DepthBiasClamp = 0;
	Desc.SlopeScaledDepthBias = 0;
	Desc.DepthClipEnable = TRUE;
	Desc.MultisampleEnable = FALSE;
	Desc.AntialiasedLineEnable = FALSE;
	Desc.ForcedSampleCount = 0;
	Desc.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

	return Desc;
}

D3D12_BLEND_DESC GetDefaultBlendStateDesc() {
	D3D12_BLEND_DESC Desc = {};
	Desc.AlphaToCoverageEnable = FALSE;
	Desc.IndependentBlendEnable = FALSE;

	const D3D12_RENDER_TARGET_BLEND_DESC DefaultRenderTargetBlendDesc =
	{
		FALSE,FALSE,
		D3D12_BLEND_ONE, D3D12_BLEND_ZERO, D3D12_BLEND_OP_ADD,
		D3D12_BLEND_ONE, D3D12_BLEND_ZERO, D3D12_BLEND_OP_ADD,
		D3D12_LOGIC_OP_NOOP,
		D3D12_COLOR_WRITE_ENABLE_ALL,
	};

	for (int i = 0; i < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT; ++i) {
		Desc.RenderTarget[i] = DefaultRenderTargetBlendDesc;
	}

	return Desc;
}

ID3D12Resource* AllocateTexture(ID3D12Device* Device, int Width, int Height, DXGI_FORMAT Format, D3D12_RESOURCE_FLAGS ResourcceFlags, D3D12_RESOURCE_STATES StartingState)
{
	ID3D12Resource* Resource = nullptr;

	D3D12_RESOURCE_DESC BackBufferDesc = {};
	BackBufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	BackBufferDesc.Width = Width;
	BackBufferDesc.Height = Height;
	BackBufferDesc.DepthOrArraySize = 1;
	BackBufferDesc.SampleDesc.Count = 1;
	BackBufferDesc.Flags = ResourcceFlags;
	BackBufferDesc.Format = Format;

	D3D12_HEAP_PROPERTIES Props = {};
	Props.Type = D3D12_HEAP_TYPE_DEFAULT;

	HRESULT hr = Device->CreateCommittedResource(&Props, D3D12_HEAP_FLAG_NONE, &BackBufferDesc, StartingState, nullptr, IID_PPV_ARGS(&Resource));
	ASSERT(SUCCEEDED(hr));

	return Resource;
}

ID3D12Resource* AllocateUploadTexture(ID3D12Device* Device, int BufferSize)
{
	D3D12_RESOURCE_DESC Desc = CD3DX12_RESOURCE_DESC::Buffer(BufferSize);

	ID3D12Resource* Resource = nullptr;

	D3D12_HEAP_PROPERTIES Props = {};
	Props.Type = D3D12_HEAP_TYPE_UPLOAD;

	HRESULT hr = Device->CreateCommittedResource(&Props, D3D12_HEAP_FLAG_NONE, &Desc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&Resource));
	ASSERT(SUCCEEDED(hr));

	return Resource;
}

ID3D12Resource* AllocateReadbackTexture(ID3D12Device* Device, int BufferSize)
{
	D3D12_RESOURCE_DESC Desc = CD3DX12_RESOURCE_DESC::Buffer(BufferSize);

	ID3D12Resource* Resource = nullptr;

	D3D12_HEAP_PROPERTIES Props = {};
	Props.Type = D3D12_HEAP_TYPE_READBACK;

	HRESULT hr = Device->CreateCommittedResource(&Props, D3D12_HEAP_FLAG_NONE, &Desc, D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&Resource));
	ASSERT(SUCCEEDED(hr));

	return Resource;
}

ID3D12RootSignature* CreatePixelRootSig(ID3D12Device* Device)
{
	D3D12_ROOT_SIGNATURE_DESC RootSigDesc = {};
	RootSigDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;


	D3D12_DESCRIPTOR_RANGE DescriptorRange = {};
	DescriptorRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	DescriptorRange.BaseShaderRegister = 0;
	DescriptorRange.NumDescriptors = 1;
	DescriptorRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	D3D12_ROOT_PARAMETER RootParam = {};
	RootParam.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	RootParam.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	RootParam.DescriptorTable.NumDescriptorRanges = 1;
	RootParam.DescriptorTable.pDescriptorRanges = &DescriptorRange;

	RootSigDesc.NumParameters = 1;
	RootSigDesc.pParameters = &RootParam;

	D3D12_STATIC_SAMPLER_DESC SamplerDesc = {};

	SamplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
	SamplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	SamplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	SamplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	SamplerDesc.MipLODBias = 0;
	SamplerDesc.MaxAnisotropy = 0;
	SamplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
	SamplerDesc.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
	SamplerDesc.MinLOD = 0.0f;
	SamplerDesc.MaxLOD = D3D12_FLOAT32_MAX;
	SamplerDesc.ShaderRegister = 0;
	SamplerDesc.RegisterSpace = 0;

	RootSigDesc.NumStaticSamplers = 1;
	RootSigDesc.pStaticSamplers = &SamplerDesc;

	ID3DBlob* RootSigBlob = nullptr;
	ID3DBlob* RootSigErrorBlob = nullptr;

	HRESULT hr = D3D12SerializeRootSignature(&RootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1, &RootSigBlob, &RootSigErrorBlob);

	if (!SUCCEEDED(hr))
	{
		const char* ErrStr = (const char*)RootSigErrorBlob->GetBufferPointer();
		int32 ErrStrLen = RootSigErrorBlob->GetBufferSize();
		LOG("Root Sig Err: '%.*s'", ErrStrLen, ErrStr);
	}

	ASSERT(SUCCEEDED(hr));

	ID3D12RootSignature* RootSig = nullptr;
	hr = Device->CreateRootSignature(0, RootSigBlob->GetBufferPointer(), RootSigBlob->GetBufferSize(), IID_PPV_ARGS(&RootSig));

	ASSERT(SUCCEEDED(hr));

	return RootSig;
}

ID3D12PipelineState* CreatePixelPSO(ID3D12Device* Device, ID3D12RootSignature* RootSig, DXGI_FORMAT OutputFormat, ID3DBlob* VSByteCode, ID3DBlob* PSByteCode)
{
	ID3D12PipelineState* PSO = nullptr;
	D3D12_INPUT_ELEMENT_DESC inputElementDescs[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	};

	D3D12_SHADER_BYTECODE VertexShaderByteCode;
	VertexShaderByteCode.pShaderBytecode = VSByteCode->GetBufferPointer();
	VertexShaderByteCode.BytecodeLength = VSByteCode->GetBufferSize();

	D3D12_SHADER_BYTECODE PixelShaderByteCode;
	PixelShaderByteCode.pShaderBytecode = PSByteCode->GetBufferPointer();
	PixelShaderByteCode.BytecodeLength = PSByteCode->GetBufferSize();

	D3D12_GRAPHICS_PIPELINE_STATE_DESC PSODesc = {};
	PSODesc.InputLayout = { inputElementDescs, (sizeof(inputElementDescs) / sizeof(inputElementDescs[0])) };
	PSODesc.pRootSignature = RootSig;
	PSODesc.VS = VertexShaderByteCode;
	PSODesc.PS = PixelShaderByteCode;
	PSODesc.RasterizerState = GetDefaultRasterizerDesc();
	PSODesc.BlendState = GetDefaultBlendStateDesc();
	PSODesc.DepthStencilState.DepthEnable = FALSE;
	PSODesc.DepthStencilState.StencilEnable = FALSE;
	PSODesc.SampleMask = UINT_MAX;
	PSODesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	PSODesc.NumRenderTargets = 1;
	PSODesc.RTVFormats[0] = DXGI_FORMAT_B8G8R8A8_UNORM;
	PSODesc.SampleDesc.Count = 1;

	Device->CreateGraphicsPipelineState(&PSODesc, IID_PPV_ARGS(&PSO));

	return PSO;
}

ID3D12Resource* AllocateVertexBuffer(ID3D12Device* Device, int BufferSize)
{
	D3D12_RESOURCE_DESC VertResourceDesc = CD3DX12_RESOURCE_DESC::Buffer(BufferSize);

	ID3D12Resource* VertexBufferRes = nullptr;

	D3D12_HEAP_PROPERTIES Props = {};
	Props.Type = D3D12_HEAP_TYPE_UPLOAD;
	const D3D12_RESOURCE_STATES InitialState = D3D12_RESOURCE_STATE_GENERIC_READ;
	HRESULT hr = Device->CreateCommittedResource(&Props, D3D12_HEAP_FLAG_NONE, &VertResourceDesc, InitialState, nullptr, IID_PPV_ARGS(&VertexBufferRes));
	ASSERT(SUCCEEDED(hr));

	void* pVertData = nullptr;
	D3D12_RANGE readRange = {};
	hr = VertexBufferRes->Map(0, &readRange, &pVertData);
	ASSERT(SUCCEEDED(hr));

	{
		float* pFloatData = (float*)pVertData;
		pFloatData[0] = -1.0f;
		pFloatData[1] = -1.0f;
		pFloatData[2] = 0.0f;
		pFloatData[3] = 1.0f;

		pFloatData[4] = 1.0f;
		pFloatData[5] = -1.0f;
		pFloatData[6] = 0.0f;
		pFloatData[7] = 1.0f;

		pFloatData[8] = -1.0f;
		pFloatData[9] = 1.0f;
		pFloatData[10] = 0.0f;
		pFloatData[11] = 1.0f;

		pFloatData[12] = 1.0f;
		pFloatData[13] = 1.0f;
		pFloatData[14] = 0.0f;
		pFloatData[15] = 1.0f;
	}

	VertexBufferRes->Unmap(0, nullptr);

	return VertexBufferRes;
}

void SetTextureUploadRandomBytes(ID3D12Resource* TextureUploadResource, int BufferSize, int Width, int Height, int Pitch)
{
	void* pTexturePixelData = nullptr;
	HRESULT hr = TextureUploadResource->Map(0, nullptr, &pTexturePixelData);
	ASSERT(SUCCEEDED(hr));

	for (int i = 0; i < BufferSize; i++)
	{
		((uint8_t*)pTexturePixelData)[i] = (uint8_t)(rand() % 256);
	}

	stbi_write_png("random_bytes_in.png", Width, Height, 4, pTexturePixelData, Pitch);

	TextureUploadResource->Unmap(0, nullptr);
}

void UploadTextureResource(ID3D12GraphicsCommandList* CommandList, ID3D12Resource* TextureUploadResource, ID3D12Resource* TextureResource, int32 Width, int32 Height, int32 Pitch)
{
	D3D12_TEXTURE_COPY_LOCATION CopyLocSrc = {}, CopyLocDst = {};
	CopyLocSrc.pResource = TextureUploadResource;
	CopyLocSrc.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
	CopyLocSrc.PlacedFootprint.Offset = 0;
	CopyLocSrc.PlacedFootprint.Footprint.Width = Width;
	CopyLocSrc.PlacedFootprint.Footprint.Height = Height;
	CopyLocSrc.PlacedFootprint.Footprint.Depth = 1;
	CopyLocSrc.PlacedFootprint.Footprint.RowPitch = Pitch;
	CopyLocSrc.PlacedFootprint.Footprint.Format = DXGI_FORMAT_B8G8R8A8_UNORM;

	CopyLocDst.pResource = TextureResource;
	CopyLocDst.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
	CopyLocDst.SubresourceIndex = 0;

	{
		D3D12_RESOURCE_BARRIER Barrier = {};
		Barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		Barrier.Transition.pResource = TextureResource;
		Barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		Barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
		Barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_DEST;

		CommandList->ResourceBarrier(1, &Barrier);
	}

	CommandList->CopyTextureRegion(&CopyLocDst, 0, 0, 0, &CopyLocSrc, nullptr);

	{
		D3D12_RESOURCE_BARRIER Barrier = {};
		Barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		Barrier.Transition.pResource = TextureResource;
		Barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		Barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
		Barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;

		CommandList->ResourceBarrier(1, &Barrier);
	}
}

void CopyRenderTargetDataToReadback(ID3D12GraphicsCommandList* CommandList, ID3D12Resource* DestResource, ID3D12Resource* ReadbackRT, int RTWidth, int RTHeight, int Pitch)
{
	D3D12_TEXTURE_COPY_LOCATION CopyLocSrc = {}, CopyLocDst = {};
	CopyLocDst.pResource = ReadbackRT;
	CopyLocDst.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
	CopyLocDst.PlacedFootprint.Offset = 0;
	CopyLocDst.PlacedFootprint.Footprint.Width = RTWidth;
	CopyLocDst.PlacedFootprint.Footprint.Height = RTHeight;
	CopyLocDst.PlacedFootprint.Footprint.Depth = 1;
	CopyLocDst.PlacedFootprint.Footprint.RowPitch = Pitch;
	CopyLocDst.PlacedFootprint.Footprint.Format = DXGI_FORMAT_B8G8R8A8_UNORM;

	CopyLocSrc.pResource = DestResource;
	CopyLocSrc.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
	CopyLocSrc.SubresourceIndex = 0;

	// Transition dest from render target to copy source
	{
		D3D12_RESOURCE_BARRIER Barrier = {};
		Barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		Barrier.Transition.pResource = DestResource;
		Barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		Barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
		Barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_SOURCE;

		CommandList->ResourceBarrier(1, &Barrier);
	}

	CommandList->CopyTextureRegion(&CopyLocDst, 0, 0, 0, &CopyLocSrc, nullptr);

	// Transition dest back to render target
	{
		D3D12_RESOURCE_BARRIER Barrier = {};
		Barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		Barrier.Transition.pResource = DestResource;
		Barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		Barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_SOURCE;
		Barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;

		CommandList->ResourceBarrier(1, &Barrier);
	}
}

void WriteReadbackToFile(ID3D12Resource* RTReadback, int RTWidth, int RTHeight)
{
	void* pPixelData = nullptr;
	HRESULT hr = RTReadback->Map(0, nullptr, &pPixelData);
	ASSERT(SUCCEEDED(hr));

	stbi_write_png("random_bytes_out.png", RTWidth, RTHeight, 4, pPixelData, 0);

	RTReadback->Unmap(0, nullptr);
}

ID3D12DescriptorHeap* GetSRVHeapForTexture(ID3D12Device* Device, ID3D12Resource* Texture)
{
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = 1;

	ID3D12DescriptorHeap* TextureSRVHeap = nullptr;

	// TODO: Descriptor heap needs to go somewhere, maybe on texture?
	D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
	srvHeapDesc.NumDescriptors = 1;
	srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	HRESULT hr = Device->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&TextureSRVHeap));

	Device->CreateShaderResourceView(Texture, &srvDesc, TextureSRVHeap->GetCPUDescriptorHandleForHeapStart());

	return TextureSRVHeap;
}

struct GPUTimer
{
	ID3D12QueryHeap* QueryHeap = nullptr;

	ID3D12Resource* TimestampReadback = nullptr;

	void Init(ID3D12Device* Device)
	{
		D3D12_QUERY_HEAP_DESC QueryHeapDesc = {};
		QueryHeapDesc.Count = 1024;
		QueryHeapDesc.Type = D3D12_QUERY_HEAP_TYPE_TIMESTAMP;
		HRESULT hr = Device->CreateQueryHeap(&QueryHeapDesc, IID_PPV_ARGS(&QueryHeap));
		ASSERT(SUCCEEDED(hr));

		D3D12_RESOURCE_DESC Desc = CD3DX12_RESOURCE_DESC::Buffer(QueryHeapDesc.Count * sizeof(uint64_t));

		D3D12_HEAP_PROPERTIES Props = {};
		Props.Type = D3D12_HEAP_TYPE_READBACK;

		hr = Device->CreateCommittedResource(&Props, D3D12_HEAP_FLAG_NONE, &Desc, D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&TimestampReadback));
		ASSERT(SUCCEEDED(hr));
	}

	void StartTiming(ID3D12GraphicsCommandList* CommandList)
	{
		CommandList->EndQuery(QueryHeap, D3D12_QUERY_TYPE_TIMESTAMP, 0);
	}

	void EndTiming(ID3D12GraphicsCommandList* CommandList)
	{
		CommandList->EndQuery(QueryHeap, D3D12_QUERY_TYPE_TIMESTAMP, 1);
		CommandList->ResolveQueryData(QueryHeap, D3D12_QUERY_TYPE_TIMESTAMP, 0, 2, TimestampReadback, 0);
	}

	void GetTiming(uint64_t* OutStart, uint64_t* OutEnd)
	{
		void* pPixelData = nullptr;
		HRESULT hr = TimestampReadback->Map(0, nullptr, &pPixelData);
		ASSERT(SUCCEEDED(hr));

		*OutStart = ((uint64_t*)pPixelData)[0];
		*OutEnd = ((uint64_t*)pPixelData)[1];

		TimestampReadback->Unmap(0, nullptr);
	}
};

int main(int argc, char** argv) {

	ID3D12Debug1* D3D12DebugLayer = nullptr;
	D3D12GetDebugInterface(IID_PPV_ARGS(&D3D12DebugLayer));
	D3D12DebugLayer->EnableDebugLayer();


	IDXGIFactory2* DXGIFactory = nullptr;

	HRESULT hr = CreateDXGIFactory(IID_PPV_ARGS(&DXGIFactory));
	ASSERT(SUCCEEDED(hr));

	IDXGIAdapter* ChosenAdapter = nullptr;

	{
		IDXGIAdapter* Adapter = nullptr;
		for (int AdapterIndex = 0; true; AdapterIndex++) {
			hr = DXGIFactory->EnumAdapters(AdapterIndex, &Adapter);
			if (!SUCCEEDED(hr)) {
				break;
			}

			DXGI_ADAPTER_DESC AdapterDesc = {};
			Adapter->GetDesc(&AdapterDesc);

			// Avoid the WARP adapter or Intel (which will likely be integrated)
			if (AdapterDesc.VendorId != 0x1414 && AdapterDesc.VendorId != 0x8686) {
				ChosenAdapter = Adapter;
			}

			OutputDebugStringW(L"\nAdapter: ");
			OutputDebugStringW(AdapterDesc.Description);

			LOG("\nvendor = %X device = %X\nDedicated vid mem: %lld  Dedicated system mem: %lld  Shared system mem: %lld\n",
				AdapterDesc.VendorId, AdapterDesc.DeviceId,
				AdapterDesc.DedicatedVideoMemory, AdapterDesc.DedicatedSystemMemory, AdapterDesc.SharedSystemMemory);
		}
	}

	ASSERT(ChosenAdapter != nullptr);

	{
		DXGI_ADAPTER_DESC ChosenAdapterDesc = {};
		ChosenAdapter->GetDesc(&ChosenAdapterDesc);

		OutputDebugStringW(L"\nChosen Adapter: ");
		OutputDebugStringW(ChosenAdapterDesc.Description);
		OutputDebugStringW(L"\n");
	}

	ID3D12Device* Device = nullptr;
	hr = SUCCEEDED(D3D12CreateDevice(ChosenAdapter, D3D_FEATURE_LEVEL_12_1, IID_PPV_ARGS(&Device)));
	ASSERT(hr);


	// NOTE: Requires Developer Mode
	Device->SetStablePowerState(true);


	ID3DBlob* VSByteCode = nullptr;
	ID3DBlob* PSByteCode = nullptr;
	ID3DBlob* ErrorMsg = nullptr;
	UINT CompilerFlags = 0;
	hr = D3DCompile(VertexShaderCode, strlen(VertexShaderCode), "<VS_SOURCE>", nullptr, nullptr, "VSMain", "vs_5_0", CompilerFlags, 0, &VSByteCode, &ErrorMsg);
	if (ErrorMsg)
	{
		const char* ErrMsgStr = (const char*)ErrorMsg->GetBufferPointer();
		OutputDebugStringA(ErrMsgStr);
	}
	ASSERT(SUCCEEDED(hr));

	hr = D3DCompile(PixelShaderCode, strlen(PixelShaderCode), "<PS_SOURCE>", nullptr, nullptr, "PSMain", "ps_5_0", CompilerFlags, 0, &PSByteCode, &ErrorMsg);
	if (ErrorMsg)
	{
		const char* ErrMsgStr = (const char*)ErrorMsg->GetBufferPointer();
		OutputDebugStringA(ErrMsgStr);
	}
	ASSERT(SUCCEEDED(hr));


	ID3D12RootSignature* PixelRootSig = CreatePixelRootSig(Device);
	ID3D12PipelineState* PixelPSO = CreatePixelPSO(Device, PixelRootSig, DXGI_FORMAT_B8G8R8A8_UNORM, VSByteCode, PSByteCode);

	ID3D12CommandQueue* CommandQueue = nullptr;

	D3D12_COMMAND_QUEUE_DESC CmdQueueDesc = {};
	CmdQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	hr = Device->CreateCommandQueue(&CmdQueueDesc, IID_PPV_ARGS(&CommandQueue));
	ASSERT(SUCCEEDED(hr));

	uint64_t TimestampFreq = 0;
	CommandQueue->GetTimestampFrequency(&TimestampFreq);

	{
		ID3D12CommandAllocator* CommandAllocator = nullptr;
		hr = Device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&CommandAllocator));
		ASSERT(SUCCEEDED(hr));

		ID3D12GraphicsCommandList* CommandList = nullptr;
		hr = Device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, CommandAllocator, 0, IID_PPV_ARGS(&CommandList));
		ASSERT(SUCCEEDED(hr));

		const int32 RTWidth = 1024;
		const int32 RTHeight = 1024;

		ID3D12Fence* ExecFence = nullptr;
		ASSERT(SUCCEEDED(Device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&ExecFence))));
		uint64_t NextValueToSignal = 1;

		GPUTimer Timer;

		Timer.Init(Device);

		// Pixel Shader Copy
		{
			ID3D12Resource* DestResource = AllocateTexture(Device, RTWidth, RTHeight, DXGI_FORMAT_B8G8R8A8_UNORM, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET, D3D12_RESOURCE_STATE_RENDER_TARGET);
			ID3D12Resource* SrcResource = AllocateTexture(Device, RTWidth, RTHeight, DXGI_FORMAT_B8G8R8A8_UNORM, D3D12_RESOURCE_FLAG_NONE, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

			int bpp = 4;
			int TexBufferSize = RTWidth * RTHeight * bpp;
			ID3D12Resource* UploadSource = AllocateUploadTexture(Device, TexBufferSize);

			ID3D12Resource* ReadbackRT = AllocateReadbackTexture(Device, TexBufferSize);

			SetTextureUploadRandomBytes(UploadSource, TexBufferSize, RTWidth, RTHeight, RTWidth * bpp);
			UploadTextureResource(CommandList, UploadSource, SrcResource, RTWidth, RTHeight, RTWidth * bpp);

			D3D12_DESCRIPTOR_HEAP_DESC DescriptorHeapDesc = {};
			DescriptorHeapDesc.NumDescriptors = 1;
			DescriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;

			ID3D12DescriptorHeap* DescriptorHeap = nullptr;
			Device->CreateDescriptorHeap(&DescriptorHeapDesc, IID_PPV_ARGS(&DescriptorHeap));

			D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = DescriptorHeap->GetCPUDescriptorHandleForHeapStart();
			Device->CreateRenderTargetView(DestResource, nullptr, rtvHandle);


			ID3D12DescriptorHeap* SrcSRV = GetSRVHeapForTexture(Device, SrcResource);

			// TODO: Copy vertex buffer to GPU
			int32 VertexCount = 4;
			int32 VertexBufferSize = VertexCount * 16;
			ID3D12Resource* VertexBufferRes = AllocateVertexBuffer(Device, VertexBufferSize);

			D3D12_VIEWPORT Viewport = {};
			Viewport.MinDepth = 0;
			Viewport.MaxDepth = 1;
			Viewport.TopLeftX = 0;
			Viewport.TopLeftY = 0;
			Viewport.Width = RTWidth;
			Viewport.Height = RTHeight;

			D3D12_RECT ScissorRect = {};
			ScissorRect.left = 0;
			ScissorRect.right = RTWidth;
			ScissorRect.top = 0;
			ScissorRect.bottom = RTHeight;

			// CommandList State

			for (int iter = 0; iter < 1024; iter++)
			{
				CommandList->SetPipelineState(PixelPSO);
				CommandList->SetGraphicsRootSignature(PixelRootSig);

				CommandList->RSSetViewports(1, &Viewport);
				CommandList->RSSetScissorRects(1, &ScissorRect);
				CommandList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);

				CommandList->SetDescriptorHeaps(1, &SrcSRV);
				CommandList->SetGraphicsRootDescriptorTable(0, SrcSRV->GetGPUDescriptorHandleForHeapStart());

				{
					D3D12_VERTEX_BUFFER_VIEW vtbView = {};
					vtbView.BufferLocation = VertexBufferRes->GetGPUVirtualAddress();
					vtbView.SizeInBytes = VertexBufferSize;
					vtbView.StrideInBytes = 16;

					CommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
					CommandList->IASetVertexBuffers(0, 1, &vtbView);
				}

				Timer.StartTiming(CommandList);

				CommandList->DrawInstanced(VertexCount, 1, 0, 0);

				Timer.EndTiming(CommandList);

				// Copy render target data to readback texture
				//CopyRenderTargetDataToReadback(CommandList, DestResource, ReadbackRT, RTWidth, RTHeight, RTWidth * bpp);

				CommandList->Close();

				ID3D12CommandList* CommandLists[] = { CommandList };
				CommandQueue->ExecuteCommandLists(1, CommandLists);

				CommandQueue->Signal(ExecFence, NextValueToSignal);

				HANDLE hEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
				ExecFence->SetEventOnCompletion(NextValueToSignal, hEvent);
				WaitForSingleObject(hEvent, INFINITE);
				CloseHandle(hEvent);

				NextValueToSignal++;

				//WriteReadbackToFile(ReadbackRT, RTWidth, RTHeight);

				uint64_t StartTS = 0;
				uint64_t EndTS = 0;
				Timer.GetTiming(&StartTS, &EndTS);

				uint64_t TotalTS = EndTS - StartTS;
				double TotalUsec = ((double)TotalTS) / TimestampFreq * (1000.0 * 1000.0);
				LOG("Took %5.1f usec (%llu ticks) for pixel shader copy (%4d x %4d)", TotalUsec, TotalTS, RTWidth, RTHeight);

				CommandList->Reset(CommandAllocator, nullptr);
			}
		}

		// Since executing a command list will be asynchronous on another thread,
		// wait a second so that the async work can hit the bug
		//Sleep(1000);
	}


	return 0;
}