//
// Copyright (c) 2013 Mikko Mononen memon@inside.org
// D3D Demo port cmaughan.
// This software is provided 'as-is', without any express or implied
// warranty.  In no event will the authors be held liable for any damages
// arising from the use of this software.
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it
// freely, subject to the following restrictions:
// 1. The origin of this software must not be misrepresented; you must not
//    claim that you wrote the original software. If you use this software
//    in a product, an acknowledgment in the product documentation would be
//    appreciated but is not required.
// 2. Altered source versions must be plainly marked as such, and must not be
//    misrepresented as being the original software.
// 3. This notice may not be removed or altered from any source distribution.
//

#include <stdio.h>
#include "nanovg.h"
#define NANOVG_D3D11_IMPLEMENTATION
#define COBJMACROS // This example is in c, so we use the COM macros
#define INITGUID
#include "nanovg_d3d11.h"
#include "demo.h"
#include "perf.h"
#include <Windows.h>
#include <windowsx.h>

int blowup = 0;
int screenshot = 0;
int premult = 0;
int xm = 0;
int ym = 0;
int xWin = 0;
int yWin = 0;

struct NVGcontext* vg = NULL;
struct GPUtimer gpuTimer;
struct PerfGraph fps, cpuGraph, gpuGraph;
double prevt = 0, cpuTime = 0;
struct DemoData data;

// Timer
double cpuTimerResolution;
unsigned __int64  cpuTimerBase;


ID3D11Device* pDevice;
ID3D11DeviceContext* pDeviceContext;
IDXGISwapChain* pSwapChain;
DXGI_SWAP_CHAIN_DESC swapDesc;
ID3D11RenderTargetView* pRenderTargetView;
ID3D11Texture2D* pDepthStencil;
ID3D11DepthStencilView* pDepthStencilView;
D3D_FEATURE_LEVEL FeatureLevel;

// Global Variables:
HINSTANCE hInst;
HWND hWndMain = 0;

// Forward declarations of functions included in this code module:
ATOM MyRegisterClass(HINSTANCE hInstance);
BOOL InitInstance(HINSTANCE, int);
BOOL InitializeDX(unsigned int x, unsigned int y);
void UnInitializeDX();
HRESULT ResizeWindow(unsigned int x, unsigned int y);

void initCPUTimer(void);
double getCPUTime(void);

const char* pszWindowClass = "WindowClass";


// Do the drawing
void Draw()
{
    int n;
    int i;
    float gpuTimes[3];
    double dt;
    double t;
    float clearColor[4];
    D3D11_VIEWPORT viewport;
    float pxRatio;

    if (!pDeviceContext)
    {
        return;
    }

    t = getCPUTime();
    dt = t - prevt;
	prevt = t;
    
    if (premult)
    {
        clearColor[0] = 0.0f;
        clearColor[1] = 0.0f;
        clearColor[2] = 0.0f;
        clearColor[3] = 0.0f;
    }
	else
    {
                    
        clearColor[0] = 0.3f;
        clearColor[1] = 0.3f;
        clearColor[2] = 0.32f;
        clearColor[3] = 1.0f;
    }

    ID3D11DeviceContext_OMSetRenderTargets(pDeviceContext, 1, &pRenderTargetView, pDepthStencilView);
          
    viewport.Height = (float)yWin;
    viewport.Width = (float)xWin;
    viewport.MaxDepth = 1.0f;
    viewport.MinDepth = 0.0f;
    viewport.TopLeftX = 0.0f;
    viewport.TopLeftY = 0.0f;
    ID3D11DeviceContext_RSSetViewports(pDeviceContext, 1, &viewport);

    ID3D11DeviceContext_ClearRenderTargetView(pDeviceContext, pRenderTargetView, clearColor);
    ID3D11DeviceContext_ClearDepthStencilView(pDeviceContext, pDepthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 0.0f, (UINT8)0);

    pxRatio = (float)xWin / (float)yWin;
    nvgBeginFrame(vg, xWin, yWin, pxRatio, premult ? NVG_PREMULTIPLIED_ALPHA : NVG_STRAIGHT_ALPHA);

    renderDemo(vg, (float)xm, (float)ym, (float)xWin, (float)yWin, (float)t, blowup, &data);

    renderGraph(vg, 5,5, &fps);
	renderGraph(vg, 5+200+5,5, &cpuGraph);
	if (gpuTimer.supported)
	    renderGraph(vg, 5+200+5+200+5,5, &gpuGraph);

	nvgEndFrame(vg);

    // Measure the CPU time taken excluding swap buffers (as the swap may wait for GPU)
	cpuTime = getCPUTime() - t;

	updateGraph(&fps, (float)dt);
	updateGraph(&cpuGraph, (float)cpuTime);

	// We may get multiple results.
	n = stopGPUTimer(&gpuTimer, gpuTimes, 3);
	for (i = 0; i < n; i++)
        updateGraph(&gpuGraph, gpuTimes[i]);

	if (screenshot)
    {
	    screenshot = 0;
        // Screenshot not yet supported
		//saveScreenShot(fbWidth, fbHeight, premult, "dump.png");
	}

    // Don't wait for VBlank
    IDXGISwapChain_Present(pSwapChain, 0, 0);
}

// Window message loop
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
        // Keyboard handling
        case WM_KEYDOWN:
        {
            if (GetKeyState(VK_ESCAPE))
            {
                CloseWindow(hWnd);
            }
            else if (GetKeyState(VK_SPACE))
            {
                blowup = !blowup;
            }
            else if (wParam == 'P')
            {
                premult = !premult;
            }
            else if (wParam == 'S')
            {
                screenshot = 1;
            }
        }
        break;
	
        // Mouse pos
        case WM_MOUSEMOVE:
        {     
            xm = GET_X_LPARAM(lParam);
            ym = GET_Y_LPARAM(lParam);
        }
        break;
    
        // Painting
	    case WM_PAINT:
        { 
            Draw(hWnd);
            ValidateRect(hWnd, NULL);
        }
	    break;

        // Sizing
        case WM_SIZE:
        {
            ResizeWindow(LOWORD(lParam), HIWORD(lParam));
        }
	    break;

	    case WM_ERASEBKGND:
        {
            // No need to erase background
		    return 1;
        }
	    break;

	    case WM_DESTROY:
        {
            UnInitializeDX();

            // Quit the app
            PostQuitMessage(0);
        }
	    break;

	    default:
        {
		    return DefWindowProc(hWnd, message, wParam, lParam);
	    }

    }
	return 0;
}

ATOM MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEX wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);
   
	wcex.style			= CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
	wcex.lpfnWndProc	= WndProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= hInstance;
	wcex.hIcon			= NULL;
	wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground	= (HBRUSH)(COLOR_WINDOW+1);
	wcex.lpszMenuName	= "";
	wcex.lpszClassName	= pszWindowClass;
	wcex.hIconSm		= NULL;

	return RegisterClassEx(&wcex);
}

BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
    RECT rcWin;
    
    hInst = hInstance; // Store instance handle in our global variable

    rcWin.left = 0;
    rcWin.right = 1000;
    rcWin.top = 0;
    rcWin.bottom = 600;
  
    AdjustWindowRectEx(&rcWin, WS_OVERLAPPEDWINDOW, FALSE, 0);
    
    rcWin.right += -rcWin.left;
    rcWin.bottom += -rcWin.top;
  
    hWndMain = CreateWindowEx(0, pszWindowClass, "Nanovg", WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, (int)rcWin.right, (int)rcWin.bottom, NULL, NULL, hInstance, NULL);

    if (!hWndMain)
    {
        return FALSE;
    }

    if (FAILED(InitializeDX(rcWin.right, rcWin.bottom)))
    {
        printf("Could not init DX\n");
        return FALSE;
    }
    
#ifdef DEMO_MSAA
	vg = nvgCreateD3D11(pDevice, 512, 512, 0);
#else
	vg = nvgCreateD3D11(pDevice, 512, 512, NVG_ANTIALIAS);
#endif
	if (vg == NULL) 
    {
		printf("Could not init nanovg.\n");
		return FALSE;
	}

	if (loadDemoData(vg, &data) == -1)
    {
		return FALSE;
    }

	initGPUTimer(&gpuTimer);
    initCPUTimer();

    initGraph(&fps, GRAPH_RENDER_FPS, "Frame Time");
	initGraph(&cpuGraph, GRAPH_RENDER_MS, "CPU Time");
	initGraph(&gpuGraph, GRAPH_RENDER_MS, "GPU Time");

    InvalidateRect(hWndMain, NULL, TRUE);
    ShowWindow(hWndMain, nCmdShow);
    UpdateWindow(hWndMain);

    startGPUTimer(&gpuTimer);

    return TRUE;
}

int main()
{
    MSG msg;
    
    hInst = GetModuleHandle(NULL);
	MyRegisterClass(hInst);

	// Perform application initialization:
	if (!InitInstance (hInst, SW_SHOW))
	{
		return FALSE;
	}

    ZeroMemory(&msg, sizeof(msg));
	
    // Main message loop:
    while (msg.message != WM_QUIT)
    {
        if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        else
        {
            Draw();
        }
    }
    
    freeDemoData(vg, &data);

	nvgDeleteD3D11(vg);

	printf("Average Frame Time: %.2f ms\n", getGraphAverage(&fps) * 1000.0f);
	printf("          CPU Time: %.2f ms\n", getGraphAverage(&cpuGraph) * 1000.0f);
	printf("          GPU Time: %.2f ms\n", getGraphAverage(&gpuGraph) * 1000.0f);

	return (int) msg.wParam;
}


// Frees everything
void UnInitializeDX()
{ 
    // Detach RTs
    if (pDeviceContext)
    {
        ID3D11RenderTargetView *viewList[1] = { NULL };
        ID3D11DeviceContext_OMSetRenderTargets(pDeviceContext, 1, viewList, NULL);
    }

    SAFE_RELEASE(pDeviceContext);
    SAFE_RELEASE(pDevice);
    SAFE_RELEASE(pSwapChain);
    SAFE_RELEASE(pRenderTargetView);
    SAFE_RELEASE(pDepthStencil);
    SAFE_RELEASE(pDepthStencilView);
}

// Setup the device and the rendering targets
BOOL InitializeDX(unsigned int x, unsigned int y)
{
    HRESULT hr = S_OK;
    IDXGIDevice *pDXGIDevice = NULL;
    IDXGIAdapter *pAdapter = NULL;
    IDXGIFactory *pDXGIFactory = NULL;
    UINT deviceFlags = 0;
    UINT driver = 0;

    static const D3D_DRIVER_TYPE driverAttempts[] =
    {
        D3D_DRIVER_TYPE_HARDWARE,
        D3D_DRIVER_TYPE_WARP,
        D3D_DRIVER_TYPE_REFERENCE,
    };

    static const D3D_FEATURE_LEVEL levelAttempts[] =
    {
        D3D_FEATURE_LEVEL_11_0,  // Direct3D 11.0 SM 5
        D3D_FEATURE_LEVEL_10_1,  // Direct3D 10.1 SM 4
        D3D_FEATURE_LEVEL_10_0,  // Direct3D 10.0 SM 4
        D3D_FEATURE_LEVEL_9_3,   // Direct3D 9.3  SM 3
        D3D_FEATURE_LEVEL_9_2,   // Direct3D 9.2  SM 2
        D3D_FEATURE_LEVEL_9_1,   // Direct3D 9.1  SM 2
    };

    for (driver = 0; driver < ARRAYSIZE(driverAttempts); driver++)
    {
        hr = D3D11CreateDevice(
            NULL,
            driverAttempts[driver],
            NULL,
            deviceFlags,
            levelAttempts,
            ARRAYSIZE(levelAttempts),
            D3D11_SDK_VERSION,
            &pDevice,
            &FeatureLevel,
            &pDeviceContext
            );

        if (SUCCEEDED(hr))
        {
            break;
        }

    }

    
    if (SUCCEEDED(hr))
    {
        hr = ID3D11Device_QueryInterface(pDevice, &IID_IDXGIDevice, &pDXGIDevice);
    }
    if (SUCCEEDED(hr))
    {
        hr = IDXGIDevice_GetAdapter(pDXGIDevice, &pAdapter);
    }
    if (SUCCEEDED(hr))
    {
        hr = IDXGIAdapter_GetParent(pAdapter, &IID_IDXGIFactory, (void**)&pDXGIFactory);
    }
    if (SUCCEEDED(hr))
    {
        ZeroMemory(&swapDesc, sizeof(swapDesc));

        swapDesc.SampleDesc.Count = 1;        //The Number of Multisamples per Level
        swapDesc.SampleDesc.Quality = 0;      //between 0(lowest Quality) and one lesser than pDevice->CheckMultisampleQualityLevels
      
        // Enable if you want to use multisample AA for the rendertarget
#ifdef DEMO_MSAA
        for (UINT i = 1; i <= D3D11_MAX_MULTISAMPLE_SAMPLE_COUNT; i++)
        {
            UINT Quality;
            if SUCCEEDED(ID3D11Device_CheckMultisampleQualityLevels(pDevice, DXGI_FORMAT_B8G8R8A8_UNORM, i, &Quality))
            {
                if (Quality > 0)
                {
                    DXGI_SAMPLE_DESC Desc;
                    Desc.Count = i;
                    Desc.Quality = Quality - 1;
                    swapDesc.SampleDesc = Desc;
                }
            }
        }
#endif

        swapDesc.BufferDesc.Width = x;
        swapDesc.BufferDesc.Height = y;
        swapDesc.BufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
        swapDesc.BufferDesc.RefreshRate.Numerator = 60;
        swapDesc.BufferDesc.RefreshRate.Denominator = 1;
        swapDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        swapDesc.BufferCount = 1;
        swapDesc.OutputWindow = hWndMain;
        swapDesc.Windowed = TRUE;
        hr = IDXGIFactory_CreateSwapChain(pDXGIFactory, (IUnknown*)pDevice, &swapDesc, &pSwapChain);

#ifdef DEMO_MSAA
        // Fallback to single sample
        if (FAILED(hr))
        {
            swapDesc.SampleDesc.Count = 1;
            hr = IDXGIFactory_CreateSwapChain(pDXGIFactory, (IUnknown*)pDevice, &swapDesc, &pSwapChain);
        }
#endif
    }

    SAFE_RELEASE(pDXGIDevice);
    SAFE_RELEASE(pAdapter);
    SAFE_RELEASE(pDXGIFactory);

    if (SUCCEEDED(hr))
    {
        hr = ResizeWindow(x, y);
        if (FAILED(hr))
        {
            return FALSE;
        }
    }
    else
    {
        // Fail
        UnInitializeDX();
        return FALSE;
    }

    return TRUE;
}

HRESULT ResizeWindow(unsigned int x, unsigned int y)
{
    D3D11_RENDER_TARGET_VIEW_DESC renderDesc;
    ID3D11RenderTargetView *viewList[1] = { NULL };
    HRESULT hr = S_OK;
    ID3D11Resource *pBackBufferResource = NULL;
    D3D11_TEXTURE2D_DESC texDesc;
    D3D11_DEPTH_STENCIL_VIEW_DESC depthViewDesc;

    xWin = x;
    yWin = y;
   
    if (!pDevice || !pDeviceContext)
        return E_FAIL;

    //pDeviceContext->ClearState();
    ID3D11DeviceContext_OMSetRenderTargets(pDeviceContext, 1, viewList, NULL);

    // Ensure that nobody is holding onto one of the old resources
    SAFE_RELEASE(pRenderTargetView);
    SAFE_RELEASE(pDepthStencilView);

    // Resize render target buffers
    hr = IDXGISwapChain_ResizeBuffers(pSwapChain, 1, x, y, DXGI_FORMAT_B8G8R8A8_UNORM, 0);
    if (FAILED(hr))
    {
        return hr;
    }
   
    // Create the render target view and set it on the device
    hr = IDXGISwapChain_GetBuffer(pSwapChain,
        0,
        &IID_ID3D11Texture2D,
        (void**)&pBackBufferResource
        );
    if (FAILED(hr))
    {
        return hr;
    }

    renderDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    renderDesc.ViewDimension = (swapDesc.SampleDesc.Count>1) ? D3D11_RTV_DIMENSION_TEXTURE2DMS : D3D11_RTV_DIMENSION_TEXTURE2D;
    renderDesc.Texture2D.MipSlice = 0;

    hr = ID3D11Device_CreateRenderTargetView(pDevice, pBackBufferResource, &renderDesc, &pRenderTargetView);
    SAFE_RELEASE(pBackBufferResource);
    if (FAILED(hr))
    {
        return hr;
    }

    texDesc.ArraySize = 1;
    texDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
    texDesc.CPUAccessFlags = 0;
    texDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    texDesc.Height = (UINT)y;
    texDesc.Width = (UINT)x;
    texDesc.MipLevels = 1;
    texDesc.MiscFlags = 0;
    texDesc.SampleDesc.Count = swapDesc.SampleDesc.Count;
    texDesc.SampleDesc.Quality = swapDesc.SampleDesc.Quality;
    texDesc.Usage = D3D11_USAGE_DEFAULT;

    SAFE_RELEASE(pDepthStencil);
    hr = ID3D11Device_CreateTexture2D(pDevice, &texDesc, NULL, &pDepthStencil);
    if (FAILED(hr))
    {
        return hr;
    }

    depthViewDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    depthViewDesc.ViewDimension = (swapDesc.SampleDesc.Count>1) ? D3D11_DSV_DIMENSION_TEXTURE2DMS : D3D11_DSV_DIMENSION_TEXTURE2D;
    depthViewDesc.Flags = 0;
    depthViewDesc.Texture2D.MipSlice = 0;

    hr = ID3D11Device_CreateDepthStencilView(pDevice, (ID3D11Resource*)pDepthStencil, &depthViewDesc, &pDepthStencilView);
    return hr;
}

    
// High perf counter.
static unsigned __int64 getRawTime(void)
{
    unsigned __int64 time;
    QueryPerformanceCounter((LARGE_INTEGER*)&time);
    return time;
}

// Initialise timer
void initCPUTimer(void)
{
    unsigned __int64 frequency;

    if (QueryPerformanceFrequency((LARGE_INTEGER*)&frequency))
    {
        cpuTimerResolution = 1.0 / (double)frequency;
    }

    cpuTimerBase = getRawTime();
}

// GetTime
double getCPUTime(void)
{
    return (double)(getRawTime() - cpuTimerBase) *
        cpuTimerResolution;
}
	
