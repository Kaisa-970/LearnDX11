//--------------------------------------------------------------------------------------
// File: Tutorial01.cpp
//
// This application demonstrates creating a Direct3D 11 device
//
// http://msdn.microsoft.com/en-us/library/windows/apps/ff729718.aspx
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License (MIT).
//--------------------------------------------------------------------------------------
#include <windows.h>
#include <d3d11_1.h>
#include <directxmath.h>
#include <directxcolors.h>
#include <d3dcompiler.h>

//#pragma comment(lib,"winmm.lib")

using namespace DirectX;

#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 600
#define WINDOW_TITLE L"【致我们永不熄灭的游戏开发梦想】程序核心框架"

#define SAFE_RELEASE(p) {if(p){p->Release(); p = NULL;}}

//--------------------------------------------------------------------------------------
// Forward declarations
//--------------------------------------------------------------------------------------
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);

HRESULT Direct3D_Init(HWND hwnd);
HRESULT Objects_Init(HWND hwnd);
VOID Direct3D_Render(HWND hwnd);
VOID Direct3D_CleanUp();


IDXGISwapChain* g_pSwapChain;
IDXGISwapChain1* g_pSwapChain1;
ID3D11Device* g_pd3dDevice;
ID3D11Device1* g_pd3dDevice1;
D3D_FEATURE_LEVEL FeatureLevel;
ID3D11DeviceContext* g_pImmediateContext;
ID3D11DeviceContext1* g_pImmediateContext1;
ID3D11RenderTargetView* g_pRengerTargetView = nullptr;
ID3D11VertexShader* g_pVertexShader = nullptr;
ID3D11InputLayout* g_pVSLayout = nullptr;
ID3D11Buffer* g_pVertexBuffer;
ID3D11PixelShader* g_pPixelShader = nullptr;
ID3D11Buffer* g_pIndexBuffer;
D3D_FEATURE_LEVEL g_FeatureLevel;
D3D_DRIVER_TYPE g_driverType;

struct SimpleVertex
{
    XMFLOAT3 Pos;
    XMFLOAT4 Color;
};

//--------------------------------------------------------------------------------------
// Entry point to the program. Initializes everything and goes into a message processing 
// loop. Idle time is used to render the scene.
//--------------------------------------------------------------------------------------
int WINAPI wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int nCmdShow)
{
    //UNREFERENCED_PARAMETER(hPrevInstance);
    //UNREFERENCED_PARAMETER(lpCmdLine);

    // 窗口类的设计
    WNDCLASSEX wndClass = { 0 };
    wndClass.cbSize = sizeof(WNDCLASSEX);
    wndClass.style = CS_HREDRAW | CS_VREDRAW;
    wndClass.lpfnWndProc = WndProc;
    wndClass.cbClsExtra = 0;
    wndClass.cbWndExtra = 0;
    wndClass.hInstance = hInstance;
    wndClass.hIcon = (HICON)::LoadImage(hInstance,L"tico.ico",IMAGE_ICON,0,
        0,LR_DEFAULTSIZE | LR_LOADFROMFILE);
    wndClass.hCursor = LoadCursor(NULL, IDC_ARROW);
    wndClass.hbrBackground = (HBRUSH)GetStockObject(GRAY_BRUSH);
    wndClass.lpszMenuName = NULL;
    wndClass.lpszClassName = L"ForTheDreamOfGameDevelop";

    // 窗口类的注册
    if (!RegisterClassEx(&wndClass)) {
        return -1;
    }

    // 创建窗口
    auto hWnd = CreateWindow(L"ForTheDreamOfGameDevelop", WINDOW_TITLE, WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, WINDOW_WIDTH, WINDOW_HEIGHT, NULL, NULL, hInstance, NULL);

    PlaySound(L"bgm.wav", NULL, SND_FILENAME | SND_ASYNC | SND_LOOP);

    //MessageBox(hWnd, L"Play!", L"提示", 0);

    //MoveWindow(hWnd, 250, 80, WINDOW_WIDTH, WINDOW_HEIGHT, true); //移动窗口
    ShowWindow(hWnd, nCmdShow); //显示窗口
    //UpdateWindow(hWnd);  //更新窗口

    // 设备初始化
    if (FAILED(Direct3D_Init(hWnd))) {
        Direct3D_CleanUp();
        return 0;
    }

    // 消息循环
    MSG msg = { 0 };

    //// GetMessage有消息才会返回，没消息就阻塞，显然不符合游戏场景
    //while (GetMessage(&msg, NULL, 0, 0)) {
    //    TranslateMessage(&msg);
    //    DispatchMessage(&msg);
    //}

    while (msg.message != WM_QUIT)
    {
        if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) //判断是否有消息
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        else {
            
            //渲染更新

            Direct3D_Render(hWnd);
        }
    }

    UnregisterClass(L"ForTheDreamOfGameDevelop", wndClass.hInstance);
    
    return 0;
}


//--------------------------------------------------------------------------------------
// Called every time the application receives a message
//--------------------------------------------------------------------------------------
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    PAINTSTRUCT ps;
    HDC hdc;
    switch (message)
    {
    case WM_PAINT:
        hdc = BeginPaint(hWnd,&ps) ;
        //Direct3D_Render(hWnd);
        EndPaint(hWnd, &ps);
        //ValidateRect(hWnd, NULL); //更新客户区显示
        break;

    case WM_KEYDOWN:
        if (wParam == VK_ESCAPE) {  //按下ESC销毁窗口并发送WM_DESTROY消息
            DestroyWindow(hWnd);
        }
        break;

    case WM_DESTROY:
        Direct3D_CleanUp();
        PostQuitMessage(0); //向系统表明有个线程有终止请求
        break;

        // Note that this tutorial does not handle resizing (WM_SIZE) requests,
        // so we created the window without the resize border.

    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }

    return 0;
}

HRESULT Direct3D_Init(HWND hwnd) {
    //if (!(S_OK == Objects_Init(hwnd))) {
    //    return E_FAIL;
    //}

    RECT rc;
    GetClientRect(hwnd, &rc);
    UINT width = rc.right - rc.left;
    UINT height = rc.bottom - rc.top;

    DXGI_SWAP_CHAIN_DESC sd = {};
    //ZeroMemory(&sd, sizeof(sd));
    sd.BufferCount = 1;
    sd.BufferDesc.Width = width;
    sd.BufferDesc.Height = height;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hwnd;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.Windowed = true;
    //sd.Flags = 0;

    D3D_FEATURE_LEVEL featureLevels[] =
    {
        D3D_FEATURE_LEVEL_11_1,
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_1,
        D3D_FEATURE_LEVEL_10_0,
    };

    HRESULT hr = S_OK;

    UINT createDeviceFlags = 0;
#ifdef _DEBUG
    createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif


    if (FAILED(hr = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE,
        nullptr, createDeviceFlags, featureLevels, ARRAYSIZE(featureLevels), D3D11_SDK_VERSION, &sd,
        &g_pSwapChain, &g_pd3dDevice, &g_FeatureLevel, &g_pImmediateContext))) {

        return hr;
    }

    

    //*****Turial
    //D3D_DRIVER_TYPE driverTypes[] =
    //{
    //    D3D_DRIVER_TYPE_HARDWARE,
    //    D3D_DRIVER_TYPE_WARP,
    //    D3D_DRIVER_TYPE_REFERENCE,
    //};
    //UINT numDriverTypes = ARRAYSIZE(driverTypes);

    //UINT numFeatureLevels = ARRAYSIZE(featureLevels);

    //for (UINT driverTypeIndex = 0; driverTypeIndex < numDriverTypes; driverTypeIndex++)
    //{
    //    g_driverType = driverTypes[driverTypeIndex];
    //    hr = D3D11CreateDevice(nullptr, g_driverType, nullptr, createDeviceFlags, featureLevels, numFeatureLevels,
    //        D3D11_SDK_VERSION, &g_pd3dDevice, &g_FeatureLevel, &g_pImmediateContext);

    //    if (hr == E_INVALIDARG)
    //    {
    //        // DirectX 11.0 platforms will not recognize D3D_FEATURE_LEVEL_11_1 so we need to retry without it
    //        hr = D3D11CreateDevice(nullptr, g_driverType, nullptr, createDeviceFlags, &featureLevels[1], numFeatureLevels - 1,
    //            D3D11_SDK_VERSION, &g_pd3dDevice, &g_FeatureLevel, &g_pImmediateContext);
    //    }

    //    if (SUCCEEDED(hr))
    //        break;
    //}
    //if (FAILED(hr))
    //    return hr;

    //IDXGIFactory1* dxgiFactory = nullptr;
    //{
    //    IDXGIDevice* dxgiDevice = nullptr;
    //    hr = g_pd3dDevice->QueryInterface(__uuidof(IDXGIDevice), reinterpret_cast<void**>(&dxgiDevice));
    //    if (SUCCEEDED(hr))
    //    {
    //        IDXGIAdapter* adapter = nullptr;
    //        hr = dxgiDevice->GetAdapter(&adapter);
    //        if (SUCCEEDED(hr))
    //        {
    //            hr = adapter->GetParent(__uuidof(IDXGIFactory1), reinterpret_cast<void**>(&dxgiFactory));
    //            adapter->Release();
    //        }
    //        dxgiDevice->Release();
    //    }
    //}
    //if (FAILED(hr))
    //    return hr;

    //// Create swap chain
    //IDXGIFactory2* dxgiFactory2 = nullptr;
    //hr = dxgiFactory->QueryInterface(__uuidof(IDXGIFactory2), reinterpret_cast<void**>(&dxgiFactory2));
    //if (dxgiFactory2)
    //{
    //    // DirectX 11.1 or later
    //    hr = g_pd3dDevice->QueryInterface(__uuidof(ID3D11Device1), reinterpret_cast<void**>(&g_pd3dDevice1));
    //    if (SUCCEEDED(hr))
    //    {
    //        (void)g_pImmediateContext->QueryInterface(__uuidof(ID3D11DeviceContext1), reinterpret_cast<void**>(&g_pImmediateContext1));
    //    }

    //    DXGI_SWAP_CHAIN_DESC1 sd = {};
    //    sd.Width = width;
    //    sd.Height = height;
    //    sd.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    //    sd.SampleDesc.Count = 1;
    //    sd.SampleDesc.Quality = 0;
    //    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    //    sd.BufferCount = 1;

    //    hr = dxgiFactory2->CreateSwapChainForHwnd(g_pd3dDevice, hwnd, &sd, nullptr, nullptr, &g_pSwapChain1);
    //    if (SUCCEEDED(hr))
    //    {
    //        hr = g_pSwapChain1->QueryInterface(__uuidof(IDXGISwapChain), reinterpret_cast<void**>(&g_pSwapChain));
    //    }

    //    dxgiFactory2->Release();
    //}
    //else
    //{
    //    // DirectX 11.0 systems
    //    DXGI_SWAP_CHAIN_DESC sd = {};
    //    sd.BufferCount = 1;
    //    sd.BufferDesc.Width = width;
    //    sd.BufferDesc.Height = height;
    //    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    //    sd.BufferDesc.RefreshRate.Numerator = 60;
    //    sd.BufferDesc.RefreshRate.Denominator = 1;
    //    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    //    sd.OutputWindow = hwnd;
    //    sd.SampleDesc.Count = 1;
    //    sd.SampleDesc.Quality = 0;
    //    sd.Windowed = TRUE;

    //    hr = dxgiFactory->CreateSwapChain(g_pd3dDevice, &sd, &g_pSwapChain);
    //}

    //// Note this tutorial doesn't handle full-screen swapchains so we block the ALT+ENTER shortcut
    //dxgiFactory->MakeWindowAssociation(hwnd, DXGI_MWA_NO_ALT_ENTER);

    //dxgiFactory->Release();
    //*****




    ID3D11Texture2D* pBackBuffer = nullptr;
    hr = g_pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&pBackBuffer));
    if (FAILED(hr)) {
        MessageBox(hwnd, L"GetBuffer failed!", 0, 0);
    }
    hr = g_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &g_pRengerTargetView);
    pBackBuffer->Release();
    if (FAILED(hr)) {
        MessageBox(hwnd, L"CreateRT failed!", 0, 0);
    }

    // 将渲染目标视图绑定到渲染管线(重要！！！)
    g_pImmediateContext->OMSetRenderTargets(1, &g_pRengerTargetView, NULL);

    D3D11_VIEWPORT vp;
    vp.Width = (FLOAT)width;
    vp.Height = (FLOAT)height;
    vp.MinDepth = 0.0f;
    vp.MaxDepth = 1.0f;
    vp.TopLeftX = 0;
    vp.TopLeftY = 0;
    g_pImmediateContext->RSSetViewports(1, &vp);

    SimpleVertex vertices[] = {
        {XMFLOAT3(0,0.5,0.5),XMFLOAT4(1,0,0,1)},
        {XMFLOAT3(0.5,-0.5,0.5),XMFLOAT4(1,0,0,1)},
        {XMFLOAT3(-0.5,-0.5,0.5),XMFLOAT4(1,0,0,1)}
    };

    ID3DBlob* pVSBlob = nullptr;
    ID3DBlob* pErrorBlob = nullptr;
    hr = D3DCompileFromFile(L"shader.fxh", nullptr, nullptr, "VS",
        "vs_4_0", 0, 0, &pVSBlob, &pErrorBlob);

    if (FAILED(hr)) {
        if (pErrorBlob) {
            OutputDebugStringA(reinterpret_cast<const char*>(
                pErrorBlob->GetBufferPointer()));
            pErrorBlob->Release();
        }
        return hr;
    }
    if (pErrorBlob) {
        pErrorBlob->Release();
    }

    hr = g_pd3dDevice->CreateVertexShader(pVSBlob->GetBufferPointer(),
        pVSBlob->GetBufferSize(), nullptr, &g_pVertexShader);
    if (FAILED(hr)) {
        pVSBlob->Release();
        return hr;
    }

    D3D11_INPUT_ELEMENT_DESC layout[] = {
        {"POSITION",0,DXGI_FORMAT_R32G32B32_FLOAT,0,0,D3D11_INPUT_PER_VERTEX_DATA ,0},
        {"COLOR",0,DXGI_FORMAT_R32G32B32A32_FLOAT,0,12,D3D11_INPUT_PER_VERTEX_DATA ,0}
    };


    hr = g_pd3dDevice->CreateInputLayout(layout, ARRAYSIZE(layout),
        pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), &g_pVSLayout);
    pVSBlob->Release();
    if (FAILED(hr)) {
        return hr;
    }
    g_pImmediateContext->IASetInputLayout(g_pVSLayout);

    ID3DBlob* pPSBlob = nullptr;
    pErrorBlob = nullptr;
    hr = D3DCompileFromFile(L"shader.fxh", nullptr, nullptr, "PS",
        "ps_4_0", 0, 0, &pPSBlob, &pErrorBlob);

    if (FAILED(hr)) {
        if (pErrorBlob) {
            OutputDebugStringA(reinterpret_cast<const char*>(
                pErrorBlob->GetBufferPointer()));
            pErrorBlob->Release();
        }
        return hr;
    }
    if (pErrorBlob) {
        pErrorBlob->Release();
    }

    hr = g_pd3dDevice->CreatePixelShader(pPSBlob->GetBufferPointer(),
        pPSBlob->GetBufferSize(), nullptr, &g_pPixelShader);
    pPSBlob->Release();
    if (FAILED(hr)) {
        return hr;
    }


    D3D11_BUFFER_DESC bd = {};
    bd.Usage = D3D11_USAGE_DEFAULT;
    bd.ByteWidth = sizeof(SimpleVertex) * 3;
    bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    bd.CPUAccessFlags = 0;

    D3D11_SUBRESOURCE_DATA InitData = {};
    InitData.pSysMem = vertices;

    hr = g_pd3dDevice->CreateBuffer(&bd, &InitData, &g_pVertexBuffer);
    if (FAILED(hr)) {
        return hr;
    }
    UINT stride = sizeof(SimpleVertex);
    UINT offset = 0;
    g_pImmediateContext->IASetVertexBuffers(0, 1, &g_pVertexBuffer, &stride, &offset);


    g_pImmediateContext->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    return S_OK;
}

HRESULT Objects_Init(HWND hwnd) {
    return S_OK;
}

VOID Direct3D_Render(HWND hwnd) {

    g_pImmediateContext->ClearRenderTargetView(g_pRengerTargetView, Colors::Black);
    
    /*g_pImmediateContext->VSSetConstantBuffers(0, 1, &g_pVertexBuffer);*/

    g_pImmediateContext->VSSetShader(g_pVertexShader,nullptr,0);
    //g_pImmediateContext->VSSetConstantBuffers(0, 1, &g_pVertexBuffer);
    g_pImmediateContext->PSSetShader(g_pPixelShader, nullptr, 0);
    g_pImmediateContext->Draw(3, 0);
    g_pSwapChain->Present(0, 0);
}

VOID Direct3D_CleanUp() {
    if (g_pd3dDevice != nullptr) {
        g_pd3dDevice->Release();
    }

    if (g_pSwapChain != nullptr) {
        g_pSwapChain->Release();
    }

    if (g_pImmediateContext != nullptr) {
        g_pImmediateContext->Release();
    }

    if (g_pRengerTargetView != nullptr) {
        g_pRengerTargetView->Release();
    }
    if (g_pVertexShader != nullptr) {
        g_pVertexShader->Release();
    }
    if (g_pVertexBuffer != nullptr) {
        g_pVertexBuffer->Release();
    }
    if (g_pVSLayout != nullptr) {
        g_pVSLayout->Release();
    }
    if (g_pPixelShader != nullptr) {
        g_pPixelShader->Release();
    }
}
