#include <string>
#include <memory>
#include <wrl/client.h>

#include <d3d11on12.h>
#include <d3d12.h>


class D3D11Manager
{
    Microsoft::WRL::ComPtr<struct IDXGISwapChain3> m_pSwapChain; // use this swap-chain type

    Microsoft::WRL::ComPtr<struct ID3D11Device> m_pDevice;

    Microsoft::WRL::ComPtr<ID3D12Device> m_pD3D12Device;
    Microsoft::WRL::ComPtr<ID3D12CommandQueue> m_pD3D12CommandQueue;
    Microsoft::WRL::ComPtr<ID3D11On12Device> m_pD3D11On12Device;

    Microsoft::WRL::ComPtr<struct ID3D11DeviceContext> m_pDeviceContext;

    Microsoft::WRL::ComPtr<ID3D11Texture2D> m_pBackBuffers[2]; // the wrapped back buffer number, here we hard code it to double-buffer

    std::shared_ptr<class RenderTarget> m_renderTarget;
    std::shared_ptr<class Shader> m_shader;

public:
    D3D11Manager();
    ~D3D11Manager();
    bool Initialize(HWND hWnd
            , const std::string &shaderSource, const std::wstring &textureFile);
    void Resize(int w, int h);
    void Render();
};

