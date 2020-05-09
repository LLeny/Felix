#include "WinRenderer.hpp"
#include "renderer.hxx"

#define V_THROW(x) { HRESULT hr_ = (x); if( FAILED( hr_ ) ) { throw std::runtime_error{ "DXError" }; } }


WinRenderer::WinRenderer( HWND hWnd ) : mHWnd{ hWnd }, theWinWidth{}, theWinHeight{}
{
  typedef HRESULT( WINAPI * LPD3D11CREATEDEVICE )( IDXGIAdapter*, D3D_DRIVER_TYPE, HMODULE, UINT32, CONST D3D_FEATURE_LEVEL*, UINT, UINT32, ID3D11Device**, D3D_FEATURE_LEVEL*, ID3D11DeviceContext** );
  static LPD3D11CREATEDEVICE  s_DynamicD3D11CreateDevice = nullptr;
  HMODULE hModD3D11 = ::LoadLibrary( L"d3d11.dll" );
  if ( hModD3D11 == nullptr )
    throw std::runtime_error{ "DXError" };

  s_DynamicD3D11CreateDevice = (LPD3D11CREATEDEVICE)GetProcAddress( hModD3D11, "D3D11CreateDevice" );


  D3D_FEATURE_LEVEL  featureLevelsRequested = D3D_FEATURE_LEVEL_11_0;
  UINT               numFeatureLevelsRequested = 1;
  D3D_FEATURE_LEVEL  featureLevelsSupported;

  HRESULT hr = s_DynamicD3D11CreateDevice( nullptr,  D3D_DRIVER_TYPE_HARDWARE, nullptr,
#ifndef NDEBUG
    D3D11_CREATE_DEVICE_DEBUG,
#else
    0,
#endif
    &featureLevelsRequested, numFeatureLevelsRequested, D3D11_SDK_VERSION, &mD3DDevice, &featureLevelsSupported, &mImmediateContext );

  V_THROW( hr );

  DXGI_SWAP_CHAIN_DESC sd;
  ZeroMemory( &sd, sizeof( sd ) );
  sd.BufferCount = 1;
  sd.BufferDesc.Width = 0;
  sd.BufferDesc.Height = 0;
  sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
  sd.BufferDesc.RefreshRate.Numerator = 60;
  sd.BufferDesc.RefreshRate.Denominator = 1;
  sd.BufferUsage = DXGI_USAGE_UNORDERED_ACCESS;
  sd.OutputWindow = mHWnd;
  sd.SampleDesc.Count = 1;
  sd.SampleDesc.Quality = 0;
  sd.Windowed = TRUE;

  ATL::CComPtr<IDXGIDevice> pDXGIDevice;
  V_THROW( mD3DDevice->QueryInterface( __uuidof( IDXGIDevice ), (void **)&pDXGIDevice ) );

  ATL::CComPtr<IDXGIAdapter> pDXGIAdapter;
  V_THROW( pDXGIDevice->GetParent( __uuidof( IDXGIAdapter ), (void **)&pDXGIAdapter ) );

  ATL::CComPtr<IDXGIFactory> pIDXGIFactory;
  V_THROW( pDXGIAdapter->GetParent( __uuidof( IDXGIFactory ), (void **)&pIDXGIFactory ) );

  ATL::CComPtr<IDXGISwapChain> pSwapChain;
  V_THROW( pIDXGIFactory->CreateSwapChain( mD3DDevice, &sd, &pSwapChain ) );

  mSwapChain = std::move( pSwapChain );

  V_THROW( mD3DDevice->CreateComputeShader( g_Renderer, sizeof g_Renderer, nullptr, &mRendererCS ) );

  D3D11_BUFFER_DESC bd ={};
  bd.ByteWidth = sizeof( CBPosSize );
  bd.Usage = D3D11_USAGE_DEFAULT;
  bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
  V_THROW( mD3DDevice->CreateBuffer( &bd, NULL, &mPosSizeCB ) );

}

void WinRenderer::render( DisplayGenerator::Pixel const * surface )
{
  RECT r;
  ::GetClientRect( mHWnd, &r );

  if ( theWinHeight != ( r.bottom - r.top ) || ( theWinWidth != r.right - r.left ) )
  {
    theWinHeight = r.bottom - r.top;
    theWinWidth = r.right - r.left;

    if ( theWinHeight == 0 || theWinWidth == 0 )
    {
      return;
    }

    mBackBuffer.Release();
    mBackBufferUAV.Release();

    mSwapChain->ResizeBuffers( 0, theWinWidth, theWinHeight, DXGI_FORMAT_UNKNOWN, 0 );

    V_THROW( mSwapChain->GetBuffer( 0, __uuidof( ID3D11Texture2D ), (LPVOID*)&mBackBuffer ) );
    V_THROW( mD3DDevice->CreateUnorderedAccessView( mBackBuffer, nullptr, &mBackBufferUAV ) );

    D3D11_VIEWPORT vp;
    vp.Width = (FLOAT)theWinWidth;
    vp.Height = (FLOAT)theWinHeight;
    vp.MinDepth = 0.0f;
    vp.MaxDepth = 1.0f;
    vp.TopLeftX = 0;
    vp.TopLeftY = 0;
    mImmediateContext->RSSetViewports( 1, &vp );
  }

  CBPosSize cbPosSize{ 0, 0, 160, 102 };
  mImmediateContext->UpdateSubresource( mPosSizeCB, 0, NULL, &cbPosSize, 0, 0 );
  mImmediateContext->CSSetUnorderedAccessViews( 0, 1, &mBackBufferUAV.p, nullptr );
  mImmediateContext->CSSetShader( mRendererCS, nullptr, 0 );
  UINT v[4] ={};
  mImmediateContext->ClearUnorderedAccessViewUint( mBackBufferUAV, v );
  mImmediateContext->Dispatch( 10, 102, 1 );
  mSwapChain->Present( 0, 0 );
}

