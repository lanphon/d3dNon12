#include <windows.h>
#include <d3d9.h>
#include <d3dx9.h>
#include "resource.h"

HWND g_MainWindowHandle = 0;
IDirect3D9* g_pD3D = NULL;
IDirect3DDevice9* g_pD3DDevice = NULL;

UINT g_WindowWidth = 1280;
UINT g_WindowHeight = 720;

float g_fRotateX = D3DX_PI / 4.0f;
float g_fRotateY = 0.0f;
float g_fRotateZ = 0.0f;

// Some buffers to render a cube
IDirect3DVertexBuffer9* g_CubeVertexBuffer = NULL;
IDirect3DIndexBuffer9*  g_CubeIndexBuffer = NULL;

// Vertex definition for our vertex data
struct VertexXYZColor
{
    float x, y, z;  // Vertex position.
    D3DCOLOR color;
    static const DWORD VertexFormat = D3DFVF_XYZ|D3DFVF_DIFFUSE; // Flexible vertex format definition for this vertex type
};

const D3DXCOLOR WHITE   ( D3DCOLOR_XRGB( 255, 255, 255 ) );
const D3DXCOLOR BLACK   ( D3DCOLOR_XRGB( 0, 0, 0 ) );
const D3DXCOLOR RED     ( D3DCOLOR_XRGB( 255, 0, 0 ) );
const D3DXCOLOR GREEN   ( D3DCOLOR_XRGB( 0, 255, 0 ) );
const D3DXCOLOR BLUE    ( D3DCOLOR_XRGB( 0, 0, 255 ) );
const D3DXCOLOR YELLOW  ( D3DCOLOR_XRGB( 255, 255, 0 ) );
const D3DXCOLOR CYAN    ( D3DCOLOR_XRGB( 0, 255, 255 ) );
const D3DXCOLOR MAGENTA ( D3DCOLOR_XRGB( 255, 0, 255 ) );

// Vertices of a unit cube
VertexXYZColor g_CubeVertexData[8] = {
    { -1.0f, -1.0f, -1.0f, (D3DCOLOR)BLACK },
    { -1.0f,  1.0f, -1.0f, (D3DCOLOR)GREEN },
    {  1.0f,  1.0f, -1.0f, (D3DCOLOR)YELLOW },
    {  1.0f, -1.0f, -1.0f, (D3DCOLOR)RED },
    { -1.0f, -1.0f,  1.0f, (D3DCOLOR)BLUE },
    { -1.0f,  1.0f,  1.0f, (D3DCOLOR)CYAN },
    {  1.0f,  1.0f,  1.0f, (D3DCOLOR)WHITE },
    {  1.0f, -1.0f,  1.0f, (D3DCOLOR)MAGENTA },
};

WORD g_CubeIndexData[36] = 
{
    0, 1, 2,
    0, 2, 3,
    4, 6, 5,
    4, 7, 6,
    4, 5, 1,
    4, 1, 0,
    3, 2, 6,
    3, 6, 7,
    1, 5, 6,
    1, 6, 2, 
    4, 0, 3,
    4, 3, 7,
};

// Initialize the windows application.
bool InitWindowsApp( HINSTANCE hInstance, int show );
// Initialize DirectX
bool InitDirectX( HINSTANCE hInstance, int width, int height, bool bWindowed, D3DDEVTYPE deviceType, IDirect3DDevice9** device );

// Setup the application resources
bool Setup();
// Handler for the windows message loop.
int Run();
// Update our game logic
void Update( float deltaTime );
// Render our scene
void Render();
// Release resources
void Cleanup();

// The windows procedure.  This method is used to handle events
// that our window receives.
LRESULT CALLBACK WndProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam );

// The main entry point for windows applications.
int WINAPI WinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR lpCmdLine, int nShowCmd )
{
    // Create and initialize the windows application.
    if ( !InitWindowsApp( hInstance, nShowCmd ) )
    {
        MessageBox(0, TEXT("Application Initialization Failed"), TEXT("ERROR"), MB_OK );
        return 0;
    }

    if ( !InitDirectX( hInstance, g_WindowWidth, g_WindowHeight, true, D3DDEVTYPE_HAL, &g_pD3DDevice ) )
    {
        MessageBox( 0, TEXT("Failed to initilize DirectX"), TEXT("ERROR"), MB_OK );
    }

    if ( !Setup() )
    {
        MessageBox( 0, TEXT("Failed to setup application resources"), TEXT("ERROR"), MB_OK );
    }

    int retCode = Run();

    Cleanup();

    return retCode;
}

bool InitWindowsApp( HINSTANCE hInstance, int show )
{
    // Create a window description
    WNDCLASSEX wc;

    wc.cbSize       = sizeof(WNDCLASSEX);
    wc.style        = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
    wc.lpfnWndProc  = WndProc;  // Register the callback function for the window procedure
    wc.cbClsExtra   = 0;
    wc.cbWndExtra   = 0;
    wc.hInstance    = hInstance;
    wc.hIcon        = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_MAIN_ICON));
    wc.hCursor      = LoadCursor( 0, IDC_ARROW );
    wc.hbrBackground= static_cast<HBRUSH>( GetStockObject(WHITE_BRUSH) );
    wc.lpszMenuName = NULL;
    wc.lpszClassName= TEXT("DirectX_Template");
    wc.hIconSm      = NULL;

    if ( !RegisterClassEx(&wc) )
    {
        MessageBox( 0, TEXT("Failed to register window class."), NULL, 0 );
        return false;
    }

    // Create a new window using the class description we just registered.
    g_MainWindowHandle = CreateWindowEx( 
        WS_EX_OVERLAPPEDWINDOW,     // DWORD dwExStyle
        TEXT("DirectX_Template"),   // LPCWSTR lpClassName
        TEXT("DirectX Template"),   // LPCWSTR lpWindowName
        WS_OVERLAPPEDWINDOW,        // DWORD dwStyle
        CW_USEDEFAULT,              // int X
        CW_USEDEFAULT,              // int Y
        g_WindowWidth,              // int nWidth
        g_WindowHeight,             // int nHeight
        NULL,                       // HWND hWndParent
        NULL,                       // HMENU hMenu
        hInstance,                  // HINSTANCE hInstance
        NULL                        // LPVOID lpParam
        );

    if ( g_MainWindowHandle == 0 )
    {
        MessageBox( 0, TEXT("Failed to create main window"), NULL, 0 );
        return false;
    }

    // And show and update the window we just created
    ShowWindow( g_MainWindowHandle, show );
    UpdateWindow( g_MainWindowHandle );

    return true;
}

// This method encapsulates the windows message loop.
int Run()
{
    MSG msg;
    ZeroMemory( &msg, sizeof(MSG) );

    static float previousTime = (float)timeGetTime();
    static const float targetFramerate = 30.0f;
    static const float maxTimeStep = 1.0f / targetFramerate; 

    // The message loop will run until the WM_QUIT message is received.
    while ( true )
    {
        if ( PeekMessage( &msg, 0, 0, 0, PM_REMOVE ) )
        {
            if ( msg.message == WM_QUIT ) break;

            // Translate the message and dispatch it to the appropriate 
            // window procedure.
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        else
        {
            float currentTime = (float)timeGetTime();
            // Calculate the delta time (in seconds)
            float deltaTime = ( currentTime - previousTime ) / 1000.0f;
            previousTime = currentTime;

            // Cap the delta (useful for debugging)
            deltaTime = min( deltaTime, maxTimeStep );

            // If there are no messages to handle on the message queue, 
            // update render our game.
            Update( deltaTime );
        }
    }

    return msg.wParam;
}

LRESULT CALLBACK WndProc( HWND windowHandle, UINT msg, WPARAM wParam, LPARAM lParam )
{
    switch ( msg )
    {
    case WM_PAINT:
        {
            Render(); // Redraw the window
            ValidateRect( windowHandle, NULL );
            return 0;
        }
        break;
    case WM_KEYDOWN:        // A key was pressed on the keyboard
        {
            switch ( wParam )
            {
            case VK_ESCAPE:
                {
                    DestroyWindow( g_MainWindowHandle );
                }
                break;
            case 'f':
            case 'F':
                {
                    // Switch to flat shading
                    if ( g_pD3DDevice != NULL ) g_pD3DDevice->SetRenderState( D3DRS_SHADEMODE, D3DSHADE_FLAT );
                }
                break;
            case 'g':
            case 'G':
                {
                    // Switch to Gouraud shading
                    if ( g_pD3DDevice != NULL ) g_pD3DDevice->SetRenderState( D3DRS_SHADEMODE, D3DSHADE_GOURAUD );
                }
                break;
            case 'p':
            case 'P':
                {
                    // Switch to Phong shading
                    if ( g_pD3DDevice != NULL ) g_pD3DDevice->SetRenderState( D3DRS_SHADEMODE, D3DSHADE_PHONG );
                }
                break;
            }
            return 0;
        }
        break;
    case WM_DESTROY:
        {
            PostQuitMessage( 0 );
            return 0;
        }
        break;
    }

    // Forward unhandled messages to the default window procedure
    return DefWindowProc( windowHandle, msg, wParam, lParam );
}

bool InitDirectX( HINSTANCE hInstance, int width, int height, bool bWindowed, D3DDEVTYPE deviceType, IDirect3DDevice9** device )
{
    // Create a Direct3D interface object
    g_pD3D = Direct3DCreate9(D3D_SDK_VERSION);

    if ( g_pD3D == NULL )
    {
        MessageBox( 0, TEXT("Failed to create Direct3D interface object."), TEXT("Error"), MB_OK );
        return false;
    }
    
    // Check for hardware vertex processing
    D3DCAPS9 deviceCaps;
    g_pD3D->GetDeviceCaps( D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, &deviceCaps );

    int vertexProcessing = 0;
    if ( ( deviceCaps.DevCaps & D3DDEVCAPS_HWTRANSFORMANDLIGHT ) != 0 )
    {
        vertexProcessing = D3DCREATE_HARDWARE_VERTEXPROCESSING;
    }
    else
    {   // Hardware vertex processing not supported, fallback to software vertex processing.
        vertexProcessing = D3DCREATE_SOFTWARE_VERTEXPROCESSING;
    }

    D3DPRESENT_PARAMETERS d3dpp;
    ZeroMemory( &d3dpp, sizeof(D3DPRESENT_PARAMETERS) );

    d3dpp.BackBufferWidth               = width;
    d3dpp.BackBufferHeight              = height;
    d3dpp.BackBufferFormat              = D3DFMT_A8R8G8B8; // D3DFMT_UNKNOWN;
    d3dpp.BackBufferCount               = 1;
    d3dpp.MultiSampleType               = D3DMULTISAMPLE_NONE;
    d3dpp.MultiSampleQuality            = 0;
    d3dpp.SwapEffect                    = D3DSWAPEFFECT_DISCARD;
    d3dpp.hDeviceWindow                 = g_MainWindowHandle;
    d3dpp.Windowed                      = bWindowed;
    d3dpp.EnableAutoDepthStencil        = true;
    d3dpp.AutoDepthStencilFormat        = D3DFMT_D24S8;
    d3dpp.Flags                         = 0;
    d3dpp.FullScreen_RefreshRateInHz    = D3DPRESENT_RATE_DEFAULT;
    d3dpp.PresentationInterval          = D3DPRESENT_INTERVAL_IMMEDIATE;

    if ( FAILED( g_pD3D->CreateDevice( D3DADAPTER_DEFAULT, deviceType, g_MainWindowHandle, vertexProcessing, &d3dpp, device ) ) )
    {
        MessageBox( 0, TEXT("Failed to create Direct3D Device"), TEXT("Error"), MB_OK );
        return false;
    }

    return true;
}

bool Setup()
{
    if ( g_pD3DDevice == NULL )
    {
        MessageBox( 0, TEXT("NULL reference to D3DDevice"), TEXT("Error"), MB_OK );
        return false;
    }

    // Create the vertex buffer and index buffer for our cube
    g_pD3DDevice->CreateVertexBuffer( 8 * sizeof(VertexXYZColor), D3DUSAGE_WRITEONLY, VertexXYZColor::VertexFormat, D3DPOOL_MANAGED, &g_CubeVertexBuffer, NULL );
    g_pD3DDevice->CreateIndexBuffer( 36 * sizeof(WORD), D3DUSAGE_WRITEONLY, D3DFMT_INDEX16, D3DPOOL_MANAGED, &g_CubeIndexBuffer, NULL );

    // Fill the buffers with the cube data
    VertexXYZColor* vertices = NULL;
    g_CubeVertexBuffer->Lock( 0, 0, (void**)&vertices, 0 );
    memcpy_s(vertices, 8 * sizeof(VertexXYZColor), g_CubeVertexData, 8 * sizeof(VertexXYZColor) );
    g_CubeVertexBuffer->Unlock();

    // Fill the index buffer with the cube's index data
    WORD* indices = NULL;
    g_CubeIndexBuffer->Lock( 0, 0, (void**)&indices, 0 );
    memcpy_s(indices, 36 * sizeof(WORD), g_CubeIndexData, 36 * sizeof(WORD) );
    g_CubeIndexBuffer->Unlock();

    // Setup the view matrix
    D3DXVECTOR3 cameraPosition( 0.0f, 0.0f, -5.0f );
    D3DXVECTOR3 cameraTarget( 0.0f, 0.0f, 0.0f );
    D3DXVECTOR3 cameraUp( 0.0f, 1.0f, 0.0f );
    D3DXMATRIX viewMatrix;
    D3DXMatrixLookAtLH( &viewMatrix, &cameraPosition, &cameraTarget, &cameraUp );

    g_pD3DDevice->SetTransform( D3DTS_VIEW, &viewMatrix );

    // Setup the projection matrix
    D3DXMATRIX projectionMatrix;
    D3DXMatrixPerspectiveFovLH( &projectionMatrix, D3DX_PI / 4, (float)g_WindowWidth / (float)g_WindowHeight, 0.1f, 100.0f );

    g_pD3DDevice->SetTransform( D3DTS_PROJECTION, &projectionMatrix );

    // Disable lighting
    g_pD3DDevice->SetRenderState( D3DRS_LIGHTING, false );

    return true;
}

void Cleanup()
{
    // Release our resources
    if ( g_CubeVertexBuffer != NULL )
    {
        g_CubeVertexBuffer->Release();
        g_CubeVertexBuffer = NULL;
    }

    if ( g_CubeIndexBuffer != NULL )
    {
        g_CubeIndexBuffer->Release();
        g_CubeIndexBuffer = NULL;
    }

    // Release the Direct3D device
    if ( g_pD3DDevice != NULL )
    {
        g_pD3DDevice->Release();
        g_pD3DDevice = NULL;
    }

    // Release the Direct3D interface object
    if ( g_pD3D != NULL )
    {
        g_pD3D->Release();
        g_pD3D = NULL;
    }
}

void Update( float deltaTime ) 
{
    // Rate of rotation in units/second
    const float fRotationRateY = D3DX_PI / 4.0f;
    const float fRotationRateZ = D3DX_PI / 2.0f;

    g_fRotateY +=  fRotationRateY * deltaTime;
    g_fRotateZ += fRotationRateZ * deltaTime;

    // Clamp to the allowed range
    g_fRotateY = fmodf( g_fRotateY, D3DX_PI * 2.0f );
    g_fRotateZ = fmodf( g_fRotateZ, D3DX_PI * 2.0f );

    // Redraw our window
    RedrawWindow( g_MainWindowHandle, NULL, NULL, RDW_INTERNALPAINT );
}

void Render()
{
    if ( g_pD3DDevice == NULL)
    {
        return;
    }

    // Setup our world matrix based on the rotation parameters
    D3DXMATRIX rotateX, rotateY, rotateZ;
    D3DXMatrixRotationX( &rotateX, g_fRotateX );
    D3DXMatrixRotationY( &rotateY, g_fRotateY );
    D3DXMatrixRotationZ( &rotateZ, g_fRotateZ );

    D3DXMATRIX worldMatrix = rotateX * rotateY * rotateZ;
    g_pD3DDevice->SetTransform( D3DTS_WORLD, &worldMatrix );

    // Render the scene
    g_pD3DDevice->Clear( 0, NULL, D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER, D3DCOLOR_XRGB(55,55,55), 1.0f, 0 );
    g_pD3DDevice->BeginScene();

    g_pD3DDevice->SetStreamSource( 0, g_CubeVertexBuffer, 0, sizeof(VertexXYZColor) );
    g_pD3DDevice->SetIndices( g_CubeIndexBuffer );
    g_pD3DDevice->SetFVF( VertexXYZColor::VertexFormat );
    g_pD3DDevice->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, 0, 8, 0, 12 );

    g_pD3DDevice->EndScene();
    g_pD3DDevice->Present( NULL, NULL, NULL, NULL );
}
