#include "D3D11Manager.h"
#include "shader.h"
#include <d3d11.h>
#include <dxgi1_4.h>
#include <cassert>
#include <Windows.h>
#include "d3d11on12.h"


class RenderTarget
{
    Microsoft::WRL::ComPtr<ID3D11RenderTargetView> m_pRenderTargetViews[2];
	D3D11_TEXTURE2D_DESC m_colorDesc;

	Microsoft::WRL::ComPtr<ID3D11Texture2D> m_depthStencil;
    Microsoft::WRL::ComPtr<ID3D11DepthStencilView> m_depthStencilView;

public:
    bool IsInitialized() const { return m_pRenderTargetViews[0] ? true : false; }

    bool Initialize(const Microsoft::WRL::ComPtr<ID3D11Device>& pDevice, const Microsoft::WRL::ComPtr<ID3D11Texture2D> *ppTexture, uint32_t total_count)
    {
        for (uint32_t i = 0; i < total_count; ++i)
        {
            ppTexture[i]->GetDesc(&m_colorDesc);

            HRESULT hr = pDevice->CreateRenderTargetView(ppTexture[i].Get(), NULL, &m_pRenderTargetViews[i]);
            if (FAILED(hr)) {
                return false;
            }
        }

        D3D11_TEXTURE2D_DESC tdesc;
        ppTexture[0]->GetDesc(&tdesc);

        D3D11_TEXTURE2D_DESC depthDesc;
        ZeroMemory(&depthDesc, sizeof(depthDesc));
        depthDesc.Width = tdesc.Width;
        depthDesc.Height = tdesc.Height;
        depthDesc.MipLevels = 1;
        depthDesc.ArraySize = 1;
        depthDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
        depthDesc.SampleDesc.Count = 1;
        depthDesc.SampleDesc.Quality = 0;
        depthDesc.Usage = D3D11_USAGE_DEFAULT;
        depthDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
        depthDesc.CPUAccessFlags = 0;
        depthDesc.MiscFlags = 0;
        auto hr = pDevice->CreateTexture2D(&depthDesc, NULL, &m_depthStencil);
        if (FAILED(hr)) {
            return false;
        }

        D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc;
        ZeroMemory(&dsvDesc, sizeof(dsvDesc));
        dsvDesc.Format = depthDesc.Format;
        dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DMS;
        dsvDesc.Texture2D.MipSlice = 0;
        hr = pDevice->CreateDepthStencilView(m_depthStencil.Get(), &dsvDesc, &m_depthStencilView);
        if (FAILED(hr)) {
            return false;
        }

        return true;
    }

    void SetAndClear(const Microsoft::WRL::ComPtr<ID3D11DeviceContext>& pDeviceContext, UINT fb_index)
    {
        // Output-Merger stage
        pDeviceContext->OMSetRenderTargets(1, m_pRenderTargetViews[fb_index].GetAddressOf(), m_depthStencilView.Get());

        if (m_pRenderTargetViews[fb_index]) {
            // clear
            float clearColor[] = { 0.2f, 0.2f, 0.4f, 1.0f };
            pDeviceContext->ClearRenderTargetView(m_pRenderTargetViews[fb_index].Get(), clearColor);
            float clearDepth = 1.0f;
            pDeviceContext->ClearDepthStencilView(m_depthStencilView.Get()
                , D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, clearDepth, 0);

            // Rasterizer stage
            D3D11_VIEWPORT vp;
            vp.Width = static_cast<float>(m_colorDesc.Width);
            vp.Height = static_cast<float>(m_colorDesc.Height);
            vp.MinDepth = 0.0f;
            vp.MaxDepth = 1.0f;
            vp.TopLeftX = 0;
            vp.TopLeftY = 0;
            pDeviceContext->RSSetViewports(1, &vp);
        }
    }
};



//////////////////////////////////////////////////////////////////////////////
// D3D11Manager
//////////////////////////////////////////////////////////////////////////////
D3D11Manager::D3D11Manager()
    : m_renderTarget(new RenderTarget)
    , m_shader(new Shader)
{
}

D3D11Manager::~D3D11Manager()
{
}

bool D3D11Manager::Initialize(HWND hWnd
    , const std::string& shaderSource, const std::wstring& textureFile)
{
    using namespace Microsoft::WRL;

    ComPtr<IDXGIFactory4> factory;
    CreateDXGIFactory2(0, IID_PPV_ARGS(&factory));

    ComPtr<IDXGIAdapter1> hwadapter;
    for (uint32_t i = 0; DXGI_ERROR_NOT_FOUND != factory.Get()->EnumAdapters1(i, &hwadapter); ++i)
    {
        DXGI_ADAPTER_DESC1 desc;
        hwadapter->GetDesc1(&desc);
        if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
        {
            continue;
        }

        // try to create device, but do not create it actually
        auto hr = D3D12CreateDevice(hwadapter.Get(), D3D_FEATURE_LEVEL_11_0, _uuidof(ID3D12Device), nullptr);
        if (hr >= 0)
        {
            break;
        }
    }

    auto hr0 = D3D12CreateDevice(hwadapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&m_pD3D12Device));
    if (hr0 < 0) assert(0);

    D3D12_COMMAND_QUEUE_DESC queue_desc = {};
    queue_desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    queue_desc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    hr0 = m_pD3D12Device->CreateCommandQueue(&queue_desc, IID_PPV_ARGS(&m_pD3D12CommandQueue));

    DXGI_SWAP_CHAIN_DESC1 scDesc;
    ZeroMemory(&scDesc, sizeof(scDesc));
    scDesc.BufferCount = 2;
    scDesc.Width = 320;
    scDesc.Height = 320;
    scDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM; // cannot use _SRGB here
    scDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    scDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    scDesc.SampleDesc.Count = 1;
    scDesc.SampleDesc.Quality = 0;
    ComPtr<IDXGISwapChain1> swapChain;
    hr0 = factory->CreateSwapChainForHwnd(
        m_pD3D12CommandQueue.Get()
        , hWnd
        , &scDesc
        , nullptr
        , nullptr
        , &swapChain
    );
    swapChain.As(&m_pSwapChain);

    hr0 = D3D11On12CreateDevice(
        m_pD3D12Device.Get()
        , D3D11_CREATE_DEVICE_BGRA_SUPPORT
        , nullptr
        , 0
        , reinterpret_cast<IUnknown**>(m_pD3D12CommandQueue.GetAddressOf())
        , 1
        , 0
        , &m_pDevice
        , &m_pDeviceContext
        , nullptr
    );

    hr0 = m_pDevice.As(&m_pD3D11On12Device);

    D3D_DRIVER_TYPE dtype = D3D_DRIVER_TYPE_HARDWARE;
    UINT flags = 0;
    D3D_FEATURE_LEVEL featureLevels[] = {
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_1,
        D3D_FEATURE_LEVEL_10_0,
        D3D_FEATURE_LEVEL_9_3,
        D3D_FEATURE_LEVEL_9_2,
        D3D_FEATURE_LEVEL_9_1,
    };
    UINT numFeatureLevels = sizeof(featureLevels) / sizeof(D3D_FEATURE_LEVEL);
    UINT sdkVersion = D3D11_SDK_VERSION;
    D3D_FEATURE_LEVEL validFeatureLevel;
    /*


    // Here we shall try to replace the D3D11Device to D3D11On12Device
    // the main question is how to solve the swap-chain.
    HRESULT hr = D3D11CreateDeviceAndSwapChain(
            NULL, // adapter
            dtype,
            NULL,
            flags,
            featureLevels,
            numFeatureLevels,
            sdkVersion,
            //&scDesc,
            &m_pSwapChain,
            &m_pDevice,
            &validFeatureLevel,
            &m_pDeviceContext);

    if (FAILED(hr)){
        return false;
    }
            */

    // shader 
    if(!m_shader->Initialize(m_pDevice, shaderSource, textureFile)){
        return false;
    }

    return true;
}

void D3D11Manager::Resize(int w, int h)
{
	if (!m_pDeviceContext){
		return;
	}
    // clear render target
    m_renderTarget=std::make_shared<RenderTarget>();

    auto fb_index = m_pSwapChain->GetCurrentBackBufferIndex();
    m_renderTarget->SetAndClear(m_pDeviceContext, fb_index);
    // resize swapchain
    DXGI_SWAP_CHAIN_DESC desc;
    m_pSwapChain->GetDesc(&desc);
    m_pSwapChain->ResizeBuffers(desc.BufferCount,
            0, 0,	// reference ClientRect
            desc.BufferDesc.Format,
            0 // flags
            );
}

void D3D11Manager::Render()
{
    using namespace Microsoft::WRL;
    if(!m_renderTarget->IsInitialized()){

        // retrieve the RenderTarget from D3D12 device
        ComPtr<ID3D12Resource> renderTargets[2];
        m_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&renderTargets[0]));
        m_pSwapChain->GetBuffer(1, IID_PPV_ARGS(&renderTargets[1]));

        ComPtr<ID3D11Resource> wrappedBackBuffers[2];

        D3D11_RESOURCE_FLAGS d3d11flags = { D3D11_BIND_RENDER_TARGET };
        auto hr0 = m_pD3D11On12Device->CreateWrappedResource(
            renderTargets[0].Get()
            , &d3d11flags
            , D3D12_RESOURCE_STATE_RENDER_TARGET
            , D3D12_RESOURCE_STATE_PRESENT
            , IID_PPV_ARGS(&wrappedBackBuffers[0])
        );
        auto hr1 = m_pD3D11On12Device->CreateWrappedResource(
            renderTargets[1].Get()
            , &d3d11flags
            , D3D12_RESOURCE_STATE_RENDER_TARGET
            , D3D12_RESOURCE_STATE_PRESENT
            , IID_PPV_ARGS(&wrappedBackBuffers[1])
        );

        Microsoft::WRL::ComPtr<ID3D11Texture2D> pBackBuffer;

        // save them to back buffer
        auto hr2 = wrappedBackBuffers[0].As(&m_pBackBuffers[0]);
        auto hr3 = wrappedBackBuffers[1].As(&m_pBackBuffers[1]);

        //auto hr = m_pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), &pBackBuffer);
		// Create RTV
        if(!m_renderTarget->Initialize(m_pDevice, m_pBackBuffers, 2)){
            return;
        }
    }

    auto fb_index = m_pSwapChain->GetCurrentBackBufferIndex();
	m_renderTarget->SetAndClear(m_pDeviceContext, fb_index);

	// draw
	m_shader->Animation();
	m_shader->Draw(m_pDeviceContext);

    m_pDeviceContext->Flush();

    // output
    m_pSwapChain->Present(NULL, NULL);
}
