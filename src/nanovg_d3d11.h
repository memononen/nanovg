//
// Copyright (c) 2009-2013 Mikko Mononen memon@inside.org
// Port of _D3D2.h to d3d11.h by Chris Maughan
//
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
#ifndef NANOVG_D3D11_H
#define NANOVG_D3D11_H

#include <d3d11.h>

#ifdef __cplusplus
extern "C" {
#endif

#define NVG_ANTIALIAS 1

struct NVGcontext* nvgCreateD3D11(ID3D11Device* pDevice, int atlasw, int atlash, int edgeaa);
void nvgDeleteD3D11(struct NVGcontext* ctx);

#ifdef __cplusplus
}
#endif

#ifdef NANOVG_D3D11_IMPLEMENTATION
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include "nanovg.h"
#include <d3d11_1.h>

#include "D3D11VertexShader.h"
#include "D3D11PixelShaderAA.h"
#include "D3D11PixelShader.h"

// The cpp calling is much simpler.
// For 'c' calling of DX, we need to do pPtr->lpVtbl->Func(pPtr, ...)
// There's probably a better way...
#ifdef __cplusplus
#define D3D_API(p, name, arg1) p->name()
#define D3D_API_1(p, name, arg1) p->name(arg1)
#define D3D_API_2(p, name, arg1, arg2) p->name(arg1, arg2)
#define D3D_API_3(p, name, arg1, arg2, arg3) p->name(arg1, arg2, arg3)
#define D3D_API_4(p, name, arg1, arg2, arg3, arg4) p->name(arg1, arg2, arg3, arg4)
#define D3D_API_5(p, name, arg1, arg2, arg3, arg4, arg5) p->name(arg1, arg2, arg3, arg4, arg5)
#define D3D_API_6(p, name, arg1, arg2, arg3, arg4, arg5, arg6) p->name(arg1, arg2, arg3, arg4, arg5, arg6)
#define SAFE_RELEASE(p) { if ( (p) ) { (p)->Release(); (p) = NULL; } }
#else
#define D3D_API(p, name) p->lpVtbl->name(p)
#define D3D_API_1(p, name, arg1) p->lpVtbl->name(p, arg1)
#define D3D_API_2(p, name, arg1, arg2) p->lpVtbl->name(p, arg1, arg2)
#define D3D_API_3(p, name, arg1, arg2, arg3) p->lpVtbl->name(p, arg1, arg2, arg3)
#define D3D_API_4(p, name, arg1, arg2, arg3, arg4) p->lpVtbl->name(p, arg1, arg2, arg3, arg4)
#define D3D_API_5(p, name, arg1, arg2, arg3, arg4, arg5) p->lpVtbl->name(p, arg1, arg2, arg3, arg4, arg5)
#define D3D_API_6(p, name, arg1, arg2, arg3, arg4, arg5, arg6) p->lpVtbl->name(p, arg1, arg2, arg3, arg4, arg5, arg6)
#define SAFE_RELEASE(p) { if ( (p) ) { (p)->lpVtbl->Release((p)); (p) = NULL; } }
#endif

#pragma pack(push)
#pragma pack(16)
struct PS_CONSTANTS
{
    float scissorMat[16];
    float scissorExt[4];
    float scissorScale[4];
    float paintMat[16];
    float extent[4];
    float radius[4];
    float feather[4];
    float innerCol[4];
    float outerCol[4];
    float strokeMult[4];
    int texType;
    int type;
};
#pragma pack(pop)

struct VS_CONSTANTS
{
    float dummy[16];
    float viewSize[2];
};

enum D3DNVGshaderType {
    NSVG_SHADER_FILLGRAD,
    NSVG_SHADER_FILLIMG,
    NSVG_SHADER_SIMPLE,
    NSVG_SHADER_IMG
};

struct D3DNVGshader {
    ID3D11PixelShader* frag;
    ID3D11VertexShader* vert;
    struct PS_CONSTANTS pc;
    struct VS_CONSTANTS vc;
};

struct D3DNVGtexture {
    int id;
    ID3D11Texture2D* tex;
    ID3D11ShaderResourceView* resourceView;
    int width, height;
    int type;
};

struct D3DNVGBuffer {
    unsigned int MaxBufferEntries;
    unsigned int CurrentBufferEntry;
    ID3D11Buffer* pBuffer;
};

struct D3DNVGcontext {
    ID3D11Device* pDevice;
    ID3D11DeviceContext* pDeviceContext;

    struct D3DNVGshader shader;
    ID3D11Buffer* pVSConstants;
    ID3D11Buffer* pPSConstants;
    
    int ntextures;
    int ctextures;
    int textureId;
    struct D3DNVGtexture* textures;
    ID3D11SamplerState* pSamplerState;

    int edgeAntiAlias;
    
    struct D3DNVGBuffer VertexBuffer;
    struct D3DNVGBuffer ColorBuffer;

    ID3D11Buffer* pFanIndexBuffer;
    ID3D11InputLayout* pLayoutRenderTriangles;
    
    ID3D11BlendState* pBSBlend;
    ID3D11BlendState* pBSNoWrite;
    
    ID3D11RasterizerState* pRSNoCull;
    ID3D11RasterizerState* pRSCull;

    ID3D11DepthStencilState* pDepthStencilDrawShapes;
    ID3D11DepthStencilState* pDepthStencilDrawAA;
    ID3D11DepthStencilState* pDepthStencilFill;
    ID3D11DepthStencilState* pDepthStencilDefault;
};

static void D3Dnvg__vset(struct NVGvertex* vtx, float x, float y, float u, float v)
{
	vtx->x = x;
	vtx->y = y;
	vtx->u = u;
	vtx->v = v;
}

static void D3Dnvg_copyMatrix3to4(float* pDest, const float* pSource)
{
    for (unsigned int i = 0; i < 4; i++)
    {
        memcpy(&pDest[i * 4], &pSource[i * 3], sizeof(float) * 3);
    }
}

static struct D3DNVGtexture* D3Dnvg__allocTexture(struct D3DNVGcontext* D3D)
{
    struct D3DNVGtexture* tex = NULL;
    int i;

    for (i = 0; i < D3D->ntextures; i++) {
        if (D3D->textures[i].id == 0) {
            tex = &D3D->textures[i];
            break;
        }
    }
    if (tex == NULL) {
        if (D3D->ntextures + 1 > D3D->ctextures) {
            D3D->ctextures = (D3D->ctextures == 0) ? 2 : D3D->ctextures * 2;
            D3D->textures = (struct D3DNVGtexture*)realloc(D3D->textures, sizeof(struct D3DNVGtexture)*D3D->ctextures);
            if (D3D->textures == NULL) return NULL;
        }
        tex = &D3D->textures[D3D->ntextures++];
    }

    memset(tex, 0, sizeof(*tex));
    tex->id = ++D3D->textureId;

    return tex;
}

static struct D3DNVGtexture* D3Dnvg__findTexture(struct D3DNVGcontext* D3D, int id)
{
    int i;
    for (i = 0; i < D3D->ntextures; i++)
        if (D3D->textures[i].id == id)
            return &D3D->textures[i];
    return NULL;
}

static int D3Dnvg__deleteTexture(struct D3DNVGcontext* D3D, int id)
{
    int i;
    for (i = 0; i < D3D->ntextures; i++) {
        if (D3D->textures[i].id == id) {
            if (D3D->textures[i].tex != 0)
            {
                SAFE_RELEASE(D3D->textures[i].tex);
                SAFE_RELEASE(D3D->textures[i].resourceView);
            }
            memset(&D3D->textures[i], 0, sizeof(D3D->textures[i]));
            return 1;
        }
    }
    return 0;
}

static int D3Dnvg__checkError(HRESULT hr, const char* str)
{
    if (!SUCCEEDED(hr))
    {
        printf("Error %08x after %s\n", hr, str);
        return 1;
    }
    return 0;
}

static int D3Dnvg__createShader(struct D3DNVGcontext* D3D, struct D3DNVGshader* shader, const void* vshader, unsigned int vshader_size, const void* fshader, unsigned int fshader_size)
{
    ID3D11VertexShader* vert = NULL;
    ID3D11PixelShader* frag = NULL;
    
    memset(shader, 0, sizeof(*shader));

    // Shader byte code is created at compile time from the .hlsl files.
    // No need for error checks; shader errors can be fixed in the IDE.
    HRESULT hr;
    hr = D3D_API_4(D3D->pDevice, CreateVertexShader, vshader, vshader_size, NULL, &vert);
    hr = D3D_API_4(D3D->pDevice, CreatePixelShader, fshader, fshader_size, NULL, &frag);
    
    shader->vert = vert;
    shader->frag = frag;

    return 1;
}

static void D3Dnvg__deleteShader(struct D3DNVGshader* shader)
{
    SAFE_RELEASE(shader->vert);
    SAFE_RELEASE(shader->frag);
}

void D3Dnvg_buildFanIndices(struct D3DNVGcontext* D3D)
{
    D3D11_MAPPED_SUBRESOURCE resource;
    D3D_API_5(D3D->pDeviceContext, Map, (ID3D11Resource*)D3D->pFanIndexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &resource);
    WORD* pIndices = (WORD*)resource.pData;
    
    unsigned int index0 = 0;
    unsigned int index1 = 1;
    unsigned int index2 = 2;
    unsigned int current = 0;
    while (current < (D3D->VertexBuffer.MaxBufferEntries - 3))
    {
        pIndices[current++] = (WORD)index0;
        pIndices[current++] = (WORD)index1++;
        pIndices[current++] = (WORD)index2++;
    }
    D3D_API_2(D3D->pDeviceContext, Unmap, (ID3D11Resource*)D3D->pFanIndexBuffer, 0);
}

struct NVGvertex* D3Dnvg_beginVertexBuffer(struct D3DNVGcontext* D3D, unsigned int maxCount, unsigned int* baseOffset)
{
    D3D11_MAPPED_SUBRESOURCE resource;
    if (maxCount >= D3D->VertexBuffer.MaxBufferEntries)
    {
        D3Dnvg__checkError(E_FAIL, "Vertex buffer too small!");
        return NULL;
    }
    if ((D3D->VertexBuffer.CurrentBufferEntry + maxCount) >= D3D->VertexBuffer.MaxBufferEntries)
    {
        *baseOffset = 0;
        D3D->VertexBuffer.CurrentBufferEntry = maxCount;
        D3D_API_5(D3D->pDeviceContext, Map, (ID3D11Resource*)D3D->VertexBuffer.pBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &resource);
    }
    else
    {
        *baseOffset = D3D->VertexBuffer.CurrentBufferEntry;
        D3D->VertexBuffer.CurrentBufferEntry = *baseOffset + maxCount;
        D3D_API_5(D3D->pDeviceContext, Map, (ID3D11Resource*)D3D->VertexBuffer.pBuffer, 0, D3D11_MAP_WRITE_NO_OVERWRITE, 0, &resource);
    }
    return ((struct NVGvertex*)resource.pData + *baseOffset);
}

void D3Dnvg_endVertexBuffer(struct D3DNVGcontext* D3D)
{
    D3D_API_2(D3D->pDeviceContext, Unmap, (ID3D11Resource*)D3D->VertexBuffer.pBuffer, 0);
}

static void D3Dnvg__copyVerts(struct NVGvertex* pDest, const struct NVGvertex* pSource, unsigned int num)
{
    for (unsigned int i = 0; i < num; i++)
    {
        pDest[i].x = pSource[i].x;
        pDest[i].y = pSource[i].y;
        pDest[i].u = pSource[i].u;
        pDest[i].v = pSource[i].v;
    }
}

static unsigned int D3Dnvg_updateVertexBuffer(ID3D11DeviceContext* pContext, struct D3DNVGBuffer* buffer, const struct NVGvertex* verts, unsigned int nverts)
{
    D3D11_MAPPED_SUBRESOURCE resource;
    if (nverts > buffer->MaxBufferEntries)
    {
        D3Dnvg__checkError(E_FAIL, "Vertex buffer too small!");
        return 0;
    }
    if ((buffer->CurrentBufferEntry + nverts) >= buffer->MaxBufferEntries)
    {
        buffer->CurrentBufferEntry = 0;
        D3D_API_5(pContext, Map, (ID3D11Resource*)buffer->pBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &resource);
    }
    else
    {
        D3D_API_5(pContext, Map, (ID3D11Resource*)buffer->pBuffer, 0, D3D11_MAP_WRITE_NO_OVERWRITE, 0, &resource);
    }

    D3Dnvg__copyVerts((((struct NVGvertex*)resource.pData) + buffer->CurrentBufferEntry), (const struct NVGvertex*)verts, nverts);
    
    D3D_API_2(pContext, Unmap, (ID3D11Resource*)buffer->pBuffer, 0);
    unsigned int retEntry = buffer->CurrentBufferEntry;
    buffer->CurrentBufferEntry += nverts;
    return retEntry;
}

static void D3Dnvg_setBuffers(struct D3DNVGcontext* D3D, unsigned int dynamicOffset, unsigned int staticOffset)
{
    ID3D11Buffer* pBuffers[2];
    pBuffers[0] = D3D->VertexBuffer.pBuffer;
    pBuffers[1] = D3D->ColorBuffer.pBuffer;
    unsigned int strides[2];
    unsigned int offsets[2];
    strides[0] = sizeof(struct NVGvertex);
    strides[1] = 0;
    offsets[0] = dynamicOffset * sizeof(struct NVGvertex);
    offsets[1] = staticOffset * sizeof(struct NVGvertex);

    D3D_API_3(D3D->pDeviceContext, IASetIndexBuffer, D3D->pFanIndexBuffer, DXGI_FORMAT_R16_UINT, 0);
    D3D_API_5(D3D->pDeviceContext, IASetVertexBuffers, 0, 2, pBuffers, strides, offsets);
    D3D_API_1(D3D->pDeviceContext, IASetInputLayout, D3D->pLayoutRenderTriangles);
}

static void D3Dnvg_updateShaders(struct D3DNVGcontext* D3D)
{
    D3D11_MAPPED_SUBRESOURCE MappedResource;

    // Pixel shader constants
    // TODO: Could possibly be more efficient about when this structure is updated.
    D3D_API_5(D3D->pDeviceContext, Map, (ID3D11Resource*)D3D->pPSConstants, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource);
    memcpy(MappedResource.pData, &D3D->shader.pc, sizeof(struct PS_CONSTANTS));
    D3D_API_2(D3D->pDeviceContext, Unmap, (ID3D11Resource*)D3D->pPSConstants, 0);
    D3D_API_3(D3D->pDeviceContext, PSSetConstantBuffers, 0, 1, &D3D->pPSConstants);
}

static int D3Dnvg__renderCreate(void* uptr)
{
    struct D3DNVGcontext* D3D = (struct D3DNVGcontext*)uptr;
    
    if (D3D->edgeAntiAlias) {
        if (D3Dnvg__createShader(D3D, &D3D->shader, g_D3D11VertexShader_Main, sizeof(g_D3D11VertexShader_Main), g_D3D11PixelShaderAA_Main, sizeof(g_D3D11PixelShaderAA_Main)) == 0)
            return 0;
    }
    else {
        if (D3Dnvg__createShader(D3D, &D3D->shader, g_D3D11VertexShader_Main, sizeof(g_D3D11VertexShader_Main), g_D3D11PixelShader_Main, sizeof(g_D3D11PixelShaderAA_Main)) == 0)
            return 0;
    }

    D3D->VertexBuffer.MaxBufferEntries = 10000;
    D3D->VertexBuffer.CurrentBufferEntry = 0;
    D3D->ColorBuffer.MaxBufferEntries = 100;
    D3D->ColorBuffer.CurrentBufferEntry = 0;

    D3D11_BUFFER_DESC bufferDesc;
    bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
    bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    bufferDesc.MiscFlags = 0;

    // Create the vertex buffer.
    bufferDesc.ByteWidth = sizeof(struct NVGvertex)* D3D->VertexBuffer.MaxBufferEntries;
    HRESULT hr = D3D_API_3(D3D->pDevice, CreateBuffer, &bufferDesc, NULL, &D3D->VertexBuffer.pBuffer);
    D3Dnvg__checkError(hr, "Create Vertex Buffer");

    bufferDesc.ByteWidth = sizeof(struct NVGvertex)* D3D->ColorBuffer.MaxBufferEntries;
    hr = D3D_API_3(D3D->pDevice, CreateBuffer, &bufferDesc, NULL, &D3D->ColorBuffer.pBuffer);
    D3Dnvg__checkError(hr, "Create ColorBuffer");

    bufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
    bufferDesc.ByteWidth = sizeof(WORD)* D3D->VertexBuffer.MaxBufferEntries;
    hr = D3D_API_3(D3D->pDevice, CreateBuffer, &bufferDesc, NULL, &D3D->pFanIndexBuffer);
    D3Dnvg__checkError(hr, "Create Vertex Buffer Static");

    D3Dnvg_buildFanIndices(D3D);

    D3D11_INPUT_ELEMENT_DESC LayoutRenderTriangles[] = 
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 8, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };

    hr = D3D_API_5(D3D->pDevice, CreateInputLayout, LayoutRenderTriangles, 3, g_D3D11VertexShader_Main, sizeof(g_D3D11VertexShader_Main), &D3D->pLayoutRenderTriangles);
    D3Dnvg__checkError(hr, "Create Input Layout");

    D3D11_BUFFER_DESC Desc;
    Desc.Usage = D3D11_USAGE_DYNAMIC;
    Desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    Desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    Desc.MiscFlags = 0;
    Desc.ByteWidth = sizeof(struct VS_CONSTANTS);
    if ((Desc.ByteWidth % 16) != 0)
    {
        Desc.ByteWidth += 16 - (Desc.ByteWidth % 16);
    }

    hr = D3D_API_3(D3D->pDevice, CreateBuffer, &Desc, NULL, &D3D->pVSConstants);
    D3Dnvg__checkError(hr, "Create Constant Buffer");

    Desc.ByteWidth = sizeof(struct PS_CONSTANTS);
    if ((Desc.ByteWidth % 16) != 0)
    {
        Desc.ByteWidth += 16 - (Desc.ByteWidth % 16);
    }

    hr = D3D_API_3(D3D->pDevice, CreateBuffer, &Desc, NULL, &D3D->pPSConstants);
    D3Dnvg__checkError(hr, "Create Constant Buffer");

    D3D11_RASTERIZER_DESC rasterDesc;
    ZeroMemory(&rasterDesc, sizeof(rasterDesc));
    rasterDesc.FillMode = D3D11_FILL_SOLID;
    rasterDesc.CullMode = D3D11_CULL_NONE;
    rasterDesc.DepthClipEnable = TRUE;
    rasterDesc.FrontCounterClockwise = TRUE;
    hr = D3D_API_2(D3D->pDevice, CreateRasterizerState, &rasterDesc, &D3D->pRSNoCull);

    rasterDesc.CullMode = D3D11_CULL_BACK;
    hr = D3D_API_2(D3D->pDevice, CreateRasterizerState, &rasterDesc, &D3D->pRSCull);

    D3D11_BLEND_DESC blendDesc;
    ZeroMemory(&blendDesc, sizeof(blendDesc));
    blendDesc.RenderTarget[0].BlendEnable = TRUE;
    blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
    blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
    blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_SRC_ALPHA;
    blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
    blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
    blendDesc.IndependentBlendEnable = FALSE;
    hr = D3D_API_2(D3D->pDevice, CreateBlendState, &blendDesc, &D3D->pBSBlend);

    blendDesc.RenderTarget[0].BlendEnable = FALSE;
    blendDesc.RenderTarget[0].RenderTargetWriteMask = 0;
    hr = D3D_API_2(D3D->pDevice, CreateBlendState, &blendDesc, &D3D->pBSNoWrite);

    // Stencil A Draw shapes
    D3D11_DEPTH_STENCIL_DESC depthStencilDesc;
    ZeroMemory(&depthStencilDesc, sizeof(depthStencilDesc));
    depthStencilDesc.DepthEnable = FALSE;
    depthStencilDesc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;
    depthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
    depthStencilDesc.StencilEnable = TRUE;
    depthStencilDesc.StencilReadMask = D3D11_DEFAULT_STENCIL_READ_MASK;
    depthStencilDesc.StencilWriteMask = D3D11_DEFAULT_STENCIL_WRITE_MASK;

    const D3D11_DEPTH_STENCILOP_DESC frontOp = { D3D11_STENCIL_OP_KEEP, D3D11_STENCIL_OP_KEEP, D3D11_STENCIL_OP_INCR, D3D11_COMPARISON_ALWAYS };
    const D3D11_DEPTH_STENCILOP_DESC backOp = { D3D11_STENCIL_OP_KEEP, D3D11_STENCIL_OP_KEEP, D3D11_STENCIL_OP_DECR, D3D11_COMPARISON_ALWAYS };
    depthStencilDesc.FrontFace = frontOp;
    depthStencilDesc.BackFace = backOp;
    // No color write
    hr = D3D_API_2(D3D->pDevice, CreateDepthStencilState, &depthStencilDesc, &D3D->pDepthStencilDrawShapes);

    // Draw AA
    const D3D11_DEPTH_STENCILOP_DESC aaOp = { D3D11_STENCIL_OP_KEEP, D3D11_STENCIL_OP_KEEP, D3D11_STENCIL_OP_KEEP, D3D11_COMPARISON_EQUAL };
    depthStencilDesc.FrontFace = aaOp;
    depthStencilDesc.BackFace = aaOp;
    
    hr = D3D_API_2(D3D->pDevice, CreateDepthStencilState, &depthStencilDesc, &D3D->pDepthStencilDrawAA);

    // Stencil Fill
    const D3D11_DEPTH_STENCILOP_DESC fillOp = { D3D11_STENCIL_OP_ZERO, D3D11_STENCIL_OP_ZERO, D3D11_STENCIL_OP_ZERO, D3D11_COMPARISON_NOT_EQUAL };
    depthStencilDesc.FrontFace = fillOp;
    depthStencilDesc.BackFace = fillOp;
    hr = D3D_API_2(D3D->pDevice, CreateDepthStencilState, &depthStencilDesc, &D3D->pDepthStencilFill);

    depthStencilDesc.StencilEnable = FALSE;
    hr = D3D_API_2(D3D->pDevice, CreateDepthStencilState, &depthStencilDesc, &D3D->pDepthStencilDefault);

    D3D11_SAMPLER_DESC sampDesc;
    ZeroMemory(&sampDesc, sizeof(sampDesc));
    sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
    sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
    sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
    sampDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
    sampDesc.MinLOD = 0;
    sampDesc.MaxLOD = D3D11_FLOAT32_MAX;
    sampDesc.MipLODBias = 0.0f;//-1.0f;

    D3D_API_2(D3D->pDevice, CreateSamplerState, &sampDesc, &D3D->pSamplerState);
    return 1;
}

static int D3Dnvg__renderCreateTexture(void* uptr, int type, int w, int h, const unsigned char* data)
{
    struct D3DNVGcontext* D3D = (struct D3DNVGcontext*)uptr;
    struct D3DNVGtexture* tex = D3Dnvg__allocTexture(D3D);
    if (tex == NULL)
    {
        return 0;
    }

    tex->width = w;
    tex->height = h;
    tex->type = type;

    int pixelWidthBytes;
    D3D11_TEXTURE2D_DESC texDesc;
    memset(&texDesc, 0, sizeof(texDesc));
    texDesc.ArraySize = 1;
    texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    texDesc.CPUAccessFlags = 0;
    if (type == NVG_TEXTURE_RGBA)
    {
        texDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        pixelWidthBytes = 4;

        // Mip maps
        texDesc.MipLevels = 0;
        texDesc.BindFlags |= D3D11_BIND_RENDER_TARGET;
        texDesc.MiscFlags = D3D11_RESOURCE_MISC_GENERATE_MIPS;
    }
    else
    {
        texDesc.Format = DXGI_FORMAT_R8_UNORM;
        pixelWidthBytes = 1;
        texDesc.MipLevels = 1;
    }
    texDesc.Height = tex->height;
    texDesc.Width = tex->width;
    texDesc.SampleDesc.Count = 1;
    texDesc.SampleDesc.Quality = 0;
    texDesc.Usage = D3D11_USAGE_DEFAULT;

    HRESULT hr = D3D_API_3(D3D->pDevice, CreateTexture2D, &texDesc, NULL, &tex->tex);
    if (FAILED(hr))
    {
        return 0;
    }

    if (data != NULL)
    {
        D3D_API_6(D3D->pDeviceContext, UpdateSubresource, (ID3D11Resource*)tex->tex, 0, NULL, data, tex->width * pixelWidthBytes, (tex->width * tex->height) * pixelWidthBytes);
    }

    D3D11_SHADER_RESOURCE_VIEW_DESC viewDesc;
    viewDesc.Format = texDesc.Format;
    viewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    viewDesc.Texture2D.MipLevels = (UINT)-1;
    viewDesc.Texture2D.MostDetailedMip = 0;
 
    D3D_API_3(D3D->pDevice, CreateShaderResourceView, (ID3D11Resource*)tex->tex, &viewDesc, &tex->resourceView);

    if (data != NULL && texDesc.MipLevels == 0)
    {
        D3D_API_1(D3D->pDeviceContext, GenerateMips, tex->resourceView);
    }

    if (D3Dnvg__checkError(hr, "create tex"))
        return 0;

    return tex->id;
}

static int D3Dnvg__renderDeleteTexture(void* uptr, int image)
{
    struct D3DNVGcontext* D3D = (struct D3DNVGcontext*)uptr;
    return D3Dnvg__deleteTexture(D3D, image);
}

static int D3Dnvg__renderUpdateTexture(void* uptr, int image, int x, int y, int w, int h, const unsigned char* data)
{
    struct D3DNVGcontext* D3D = (struct D3DNVGcontext*)uptr;
    struct D3DNVGtexture* tex = D3Dnvg__findTexture(D3D, image);

    if (tex == NULL)
    {
        return 0;
    }

    D3D11_BOX box;
    box.left = x;
    box.right = (x + w);
    box.top = y;
    box.bottom = (y + h);
    box.front = 0;
    box.back = 1;

    unsigned int pixelWidthBytes;
    if (tex->type == NVG_TEXTURE_RGBA)
    {
        pixelWidthBytes = 4;
    }
    else
    {
        pixelWidthBytes = 1;
    }

    const unsigned char* pData = data + (y * (tex->width * pixelWidthBytes)) + (x * pixelWidthBytes);
    D3D_API_6(D3D->pDeviceContext, UpdateSubresource, (ID3D11Resource*)tex->tex, 0, &box, pData, tex->width, tex->width * tex->height);
 
    return 1;
}

static int D3Dnvg__renderGetTextureSize(void* uptr, int image, int* w, int* h)
{
    struct D3DNVGcontext* D3D = (struct D3DNVGcontext*)uptr;
    struct D3DNVGtexture* tex = D3Dnvg__findTexture(D3D, image);
    if (tex == NULL) return 0;
    *w = tex->width;
    *h = tex->height;
    return 1;
}

static void D3Dnvg__xformIdentity(float* t)
{
    t[0] = 1.0f; t[1] = 0.0f;
    t[2] = 0.0f; t[3] = 1.0f;
    t[4] = 0.0f; t[5] = 0.0f;
}

static void D3Dnvg__xformInverse(float* inv, float* t)
{
    double invdet, det = (double)t[0] * t[3] - (double)t[2] * t[1];
    if (det > -1e-6 && det < 1e-6) {
        D3Dnvg__xformIdentity(t);
        return;
    }
    invdet = 1.0 / det;
    inv[0] = (float)(t[3] * invdet);
    inv[2] = (float)(-t[2] * invdet);
    inv[4] = (float)(((double)t[2] * t[5] - (double)t[3] * t[4]) * invdet);
    inv[1] = (float)(-t[1] * invdet);
    inv[3] = (float)(t[0] * invdet);
    inv[5] = (float)(((double)t[1] * t[4] - (double)t[0] * t[5]) * invdet);
}

static void D3Dnvg__xformToMat3x3(float* m3, float* t)
{
    m3[0] = t[0];
    m3[1] = t[1];
    m3[2] = 0.0f;
    m3[3] = t[2];
    m3[4] = t[3];
    m3[5] = 0.0f;
    m3[6] = t[4];
    m3[7] = t[5];
    m3[8] = 1.0f;
}

static int D3Dnvg__setupPaint(struct D3DNVGcontext* D3D, struct NVGpaint* paint, struct NVGscissor* scissor, float width, float fringe)
{
    struct D3DNVGtexture* tex = NULL;
    float invxform[6], paintMat[9], scissorMat[9];
    float scissorx = 0, scissory = 0;
    float scissorsx = 0, scissorsy = 0;

    D3Dnvg__xformInverse(invxform, paint->xform);
    D3Dnvg__xformToMat3x3(paintMat, invxform);

    if (scissor->extent[0] < 0.5f || scissor->extent[1] < 0.5f) 
    {
        memset(scissorMat, 0, sizeof(scissorMat));
        scissorx = 1.0f;
        scissory = 1.0f;
        scissorsx = 1.0f;
        scissorsy = 1.0f;
    }
    else {
        D3Dnvg__xformInverse(invxform, scissor->xform);
        D3Dnvg__xformToMat3x3(scissorMat, invxform);
        scissorx = scissor->extent[0];
        scissory = scissor->extent[1];
        scissorsx = sqrtf(scissor->xform[0] * scissor->xform[0] + scissor->xform[2] * scissor->xform[2]) / fringe;
        scissorsy = sqrtf(scissor->xform[1] * scissor->xform[1] + scissor->xform[3] * scissor->xform[3]) / fringe;
    }

    if (paint->image != 0) 
    {
        tex = D3Dnvg__findTexture(D3D, paint->image);
        if (tex == NULL)
        {
            return 0;
        }
        
        D3D_API_3(D3D->pDeviceContext, PSSetShaderResources,0, 1, &tex->resourceView);
 
        D3D->shader.pc.type = NSVG_SHADER_FILLIMG;
                
        D3Dnvg_copyMatrix3to4(D3D->shader.pc.scissorMat, scissorMat);

        D3D->shader.pc.scissorExt[0] = scissorx;
        D3D->shader.pc.scissorExt[1] = scissory;

        D3D->shader.pc.scissorScale[0] = scissorsx;
        D3D->shader.pc.scissorScale[1] = scissorsy;

        D3Dnvg_copyMatrix3to4(D3D->shader.pc.paintMat, paintMat);

        D3D->shader.pc.extent[0] = paint->extent[0];
        D3D->shader.pc.extent[1] = paint->extent[1];

        D3D->shader.pc.strokeMult[0] = (width*0.5f + fringe*0.5f) / fringe;

        D3D->shader.pc.texType = (tex->type == NVG_TEXTURE_RGBA ? 0 : 1);
    }
    else 
    {
        D3D->shader.pc.type = NSVG_SHADER_FILLGRAD;

        D3Dnvg_copyMatrix3to4(D3D->shader.pc.scissorMat, scissorMat);

        D3D->shader.pc.scissorExt[0] = scissorx;
        D3D->shader.pc.scissorExt[1] = scissory;

        D3D->shader.pc.scissorScale[0] = scissorsx;
        D3D->shader.pc.scissorScale[1] = scissorsy;

        D3Dnvg_copyMatrix3to4(D3D->shader.pc.paintMat, paintMat);

        D3D->shader.pc.extent[0] = paint->extent[0];
        D3D->shader.pc.extent[1] = paint->extent[1];

        D3D->shader.pc.strokeMult[0] = (width*0.5f + fringe*0.5f) / fringe;

        D3D->shader.pc.radius[0] = paint->radius;
        D3D->shader.pc.feather[0] = paint->feather;

        memcpy(D3D->shader.pc.innerCol, paint->innerColor.rgba, sizeof(float)* 4);
        memcpy(D3D->shader.pc.outerCol, paint->outerColor.rgba, sizeof(float)* 4);
    }

    D3Dnvg_updateShaders(D3D);

    return 1;
}

static void D3Dnvg__renderViewport(void* uptr, int width, int height, int alphaBlend)
{
    struct D3DNVGcontext* D3D = (struct D3DNVGcontext*)uptr;

    NVG_NOTUSED(alphaBlend);
    D3D->shader.vc.viewSize[0] = (float)width;
    D3D->shader.vc.viewSize[1] = (float)height;

    // Vertex parameters only change when viewport size changes
    D3D11_MAPPED_SUBRESOURCE mappedResource;
    D3D_API_5(D3D->pDeviceContext, Map, (ID3D11Resource*)D3D->pVSConstants, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
    memcpy(mappedResource.pData, &D3D->shader.vc, sizeof(struct VS_CONSTANTS));
    D3D_API_2(D3D->pDeviceContext, Unmap, (ID3D11Resource*)D3D->pVSConstants, 0);

    D3D_API_3(D3D->pDeviceContext, VSSetConstantBuffers, 0, 1, &D3D->pVSConstants);

    // Shaders always the same
    D3D_API_3(D3D->pDeviceContext, PSSetShader, D3D->shader.frag, NULL, 0);
    D3D_API_3(D3D->pDeviceContext, VSSetShader, D3D->shader.vert, NULL, 0);

    // Sampler always the same
    D3D_API_3(D3D->pDeviceContext, PSSetSamplers, 0, 1, &D3D->pSamplerState);

    /* TODO
    if (alphaBlend == NVG_PREMULTIPLIED_ALPHA)
        D3DBlendFuncSeparate(D3D_SRC_ALPHA, D3D_ONE_MINUS_SRC_ALPHA, D3D_ONE, D3D_ONE_MINUS_SRC_ALPHA);
    else
        D3DBlendFunc(D3D_SRC_ALPHA, D3D_ONE_MINUS_SRC_ALPHA);
        */
}

static void D3Dnvg__renderFlush(void* uptr, int alphaBlend)
{
    NVG_NOTUSED(uptr);
    NVG_NOTUSED(alphaBlend);
}

static int D3Dnvg__maxVertCount(const struct NVGpath* paths, int npaths)
{
    int i, count = 0;
    for (i = 0; i < npaths; i++) {
        count += paths[i].nfill;
        count += paths[i].nstroke;
    }
    return count;
}


static void D3Dnvg__uploadPaths(struct NVGvertex* pVertex, const struct NVGpath* paths, int npaths)
{
    const struct NVGpath* path;
    int i, n = 0;
    
    for (i = 0; i < npaths; i++) 
    {
        path = &paths[i];
        if (path->nfill > 0) 
        {
            D3Dnvg__copyVerts(pVertex + n, &path->fill[0], path->nfill);
            n += path->nfill;
        }
        if (path->nstroke > 0)
        {
            D3Dnvg__copyVerts(pVertex + n, &path->stroke[0], path->nstroke);
            n += path->nstroke;
        }
    }
}

static void D3Dnvg__renderFill(void* uptr, struct NVGpaint* paint, struct NVGscissor* scissor, float fringe,
    const float* bounds, const struct NVGpath* paths, int npaths)
{
    struct D3DNVGcontext* D3D = (struct D3DNVGcontext*)uptr;
    const struct NVGpath* path;
    int i, n, offset, maxCount;

    if (D3D->shader.vert == NULL ||
        D3D->shader.frag == NULL)
    {
        return;
    }
    
    maxCount = D3Dnvg__maxVertCount(paths, npaths);
    
    unsigned int baseOffset;
    struct NVGvertex* pVertex = D3Dnvg_beginVertexBuffer(D3D, maxCount, &baseOffset);
    
    D3Dnvg__uploadPaths(pVertex, paths, npaths);

    D3Dnvg_endVertexBuffer(D3D);

    D3Dnvg_setBuffers(D3D, baseOffset, 0);
        
    if (npaths == 1 && paths[0].convex) 
    {
        D3Dnvg__setupPaint(D3D, paint, scissor, fringe, fringe);

        D3D_API_1(D3D->pDeviceContext, RSSetState, D3D->pRSNoCull);
        
        n = 0;
        for (i = 0; i < npaths; i++) 
        {
            path = &paths[i];
            if (path->nfill <= 2)
            {
                continue;
            }
            offset = n;
            D3D_API_1(D3D->pDeviceContext, IASetPrimitiveTopology, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
            unsigned int numIndices = ((path->nfill - 2) * 3);
            assert(numIndices < D3D->VertexBuffer.MaxBufferEntries);
            if (numIndices < D3D->VertexBuffer.MaxBufferEntries)
            {
                D3D_API_3(D3D->pDeviceContext, DrawIndexed, numIndices, 0, offset);
            }
            n += path->nfill + path->nstroke;
        }

        D3D_API_1(D3D->pDeviceContext, RSSetState, D3D->pRSCull);
        
        if (D3D->edgeAntiAlias) 
        {
            // Draw fringes
            n = 0;
            for (i = 0; i < npaths; i++) 
            {
                path = &paths[i];
                offset = (n + path->nfill);
                D3D_API_1(D3D->pDeviceContext, IASetPrimitiveTopology, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
                D3D_API_2(D3D->pDeviceContext, Draw, path->nstroke, offset);
                n += path->nfill + path->nstroke;
            }
        }
    }
    else 
    {
        D3D_API_1(D3D->pDeviceContext, RSSetState, D3D->pRSCull);
        D3D_API_3(D3D->pDeviceContext, OMSetBlendState, NULL, NULL, 0xFFFFFFFF);

        // Draw shapes
        D3D_API_2(D3D->pDeviceContext, OMSetDepthStencilState, D3D->pDepthStencilDrawShapes, 0);
        D3D_API_3(D3D->pDeviceContext, OMSetBlendState, D3D->pBSNoWrite, NULL, 0xFFFFFFFF);

        D3D->shader.pc.type = NSVG_SHADER_SIMPLE;

        D3Dnvg_updateShaders(D3D);
        
        D3D_API_1(D3D->pDeviceContext, RSSetState, D3D->pRSNoCull);

        n = 0;
        for (i = 0; i < npaths; i++) 
        {
            path = &paths[i];
            if (path->nfill <= 2)
            {
                continue;
            }
            offset = n;

            D3D_API_1(D3D->pDeviceContext, IASetPrimitiveTopology, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

            unsigned int numIndices = ((path->nfill - 2) * 3);
            assert(numIndices < D3D->VertexBuffer.MaxBufferEntries);
            if (numIndices < D3D->VertexBuffer.MaxBufferEntries)
            {
                D3D_API_3(D3D->pDeviceContext, DrawIndexed, numIndices, 0, offset);
            }

            n += path->nfill + path->nstroke;
        }

        D3D_API_1(D3D->pDeviceContext, RSSetState, D3D->pRSCull);
        
        // Draw aliased off-pixels
        D3D_API_3(D3D->pDeviceContext, OMSetBlendState, D3D->pBSBlend, NULL, 0xFFFFFFFF);

        D3Dnvg__setupPaint(D3D, paint, scissor, fringe, fringe);

        if (D3D->edgeAntiAlias) 
        {
            D3D_API_2(D3D->pDeviceContext, OMSetDepthStencilState, D3D->pDepthStencilDrawAA, 0);

            // Draw fringes
            n = 0;
            for (i = 0; i < npaths; i++) 
            {
                path = &paths[i];
                offset = (n + path->nfill);
                D3D_API_1(D3D->pDeviceContext, IASetPrimitiveTopology, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
                D3D_API_2(D3D->pDeviceContext, Draw, path->nstroke, offset);
                n += path->nfill + path->nstroke;
            }
        }

        // Draw fill
        D3D_API_2(D3D->pDeviceContext, OMSetDepthStencilState, D3D->pDepthStencilFill, 0);
        D3D_API_1(D3D->pDeviceContext, RSSetState, D3D->pRSNoCull);

        struct NVGvertex quad[6];
        D3Dnvg__vset(&quad[0], bounds[0], bounds[3], .5f, 1.0f);
        D3Dnvg__vset(&quad[1], bounds[2], bounds[3], .5f, 1.0f);
        D3Dnvg__vset(&quad[2], bounds[2], bounds[1], .5f, 1.0f);
        D3Dnvg__vset(&quad[3], bounds[0], bounds[3], .5f, 1.0f);
        D3Dnvg__vset(&quad[4], bounds[2], bounds[1], .5f, 1.0f);
        D3Dnvg__vset(&quad[5], bounds[0], bounds[1], .5f, 1.0f);

        unsigned int dynamicOffset = D3Dnvg_updateVertexBuffer(D3D->pDeviceContext, &D3D->VertexBuffer, quad, 6);
        D3Dnvg_setBuffers(D3D, dynamicOffset, 0);

        D3D_API_1(D3D->pDeviceContext, IASetPrimitiveTopology, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        D3D_API_2(D3D->pDeviceContext, Draw, 6, 0);

        D3D_API_2(D3D->pDeviceContext, OMSetDepthStencilState, D3D->pDepthStencilDefault, 0);
    }
    //D3D->pDeviceContext->OMSetBlendState(NULL, NULL, 0xFFFFFFFF);
}

static void D3Dnvg__renderStroke(void* uptr, struct NVGpaint* paint, struct NVGscissor* scissor, float fringe,
    float width, const struct NVGpath* paths, int npaths)
{
    struct D3DNVGcontext* D3D = (struct D3DNVGcontext*)uptr;
    const struct NVGpath* path;
    int i, n, offset, maxCount;

    if (D3D->shader.vert == NULL ||
        D3D->shader.frag == NULL)
    {
        return;
    }

    D3Dnvg__setupPaint(D3D, paint, scissor, width, fringe);
        
    D3D_API_1(D3D->pDeviceContext, RSSetState, D3D->pRSCull);
    
    maxCount = D3Dnvg__maxVertCount(paths, npaths);
    
    unsigned int baseOffset;
    struct NVGvertex* pVertex = D3Dnvg_beginVertexBuffer(D3D, maxCount, &baseOffset);

    D3Dnvg__uploadPaths(pVertex, paths, npaths);

    D3Dnvg_endVertexBuffer(D3D);

    D3Dnvg_setBuffers(D3D, baseOffset, 0);
        
    D3D_API_1(D3D->pDeviceContext, IASetPrimitiveTopology, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
    
    // Draw Strokes
    n = 0;
    for (i = 0; i < npaths; i++) 
    {
        path = &paths[i];
        offset = (n + path->nfill);
        D3D_API_2(D3D->pDeviceContext, Draw, path->nstroke, offset);
        n += path->nfill + path->nstroke;
    }
}

static void D3Dnvg__renderTriangles(void* uptr, struct NVGpaint* paint, struct NVGscissor* scissor,
    const struct NVGvertex* verts, int nverts)
{
    struct D3DNVGcontext* D3D = (struct D3DNVGcontext*)uptr;
    struct D3DNVGtexture* tex = D3Dnvg__findTexture(D3D, paint->image);
    NVG_NOTUSED(scissor);

    if (D3D->shader.vert == NULL ||
        D3D->shader.frag == NULL)
    {
        return;
    }

    D3D->shader.pc.type = NSVG_SHADER_IMG;
    D3D->shader.pc.texType = (tex->type == NVG_TEXTURE_RGBA ? 0 : 1);

    // Update vertex shader constants
    D3Dnvg_updateShaders(D3D);

    if (tex != NULL)
    {
        D3D_API_3(D3D->pDeviceContext, PSSetShaderResources, 0, 1, &tex->resourceView);
    }

    unsigned int buffer0Offset = D3Dnvg_updateVertexBuffer(D3D->pDeviceContext, &D3D->VertexBuffer, verts, nverts);

    unsigned int buffer1Offset = D3Dnvg_updateVertexBuffer(D3D->pDeviceContext, &D3D->ColorBuffer, (struct NVGvertex*)paint->innerColor.rgba, 1);

    D3Dnvg_setBuffers(D3D, buffer0Offset, buffer1Offset);

    D3D_API_1(D3D->pDeviceContext, IASetPrimitiveTopology, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    
    D3D_API_2(D3D->pDeviceContext, Draw, nverts, 0);
}

static void D3Dnvg__renderDelete(void* uptr)
{
    struct D3DNVGcontext* D3D = (struct D3DNVGcontext*)uptr;
    int i;
    if (D3D == NULL) 
    {
        return;
    }

    D3Dnvg__deleteShader(&D3D->shader);

    for (i = 0; i < D3D->ntextures; i++) 
    {
        SAFE_RELEASE(D3D->textures[i].tex);
        SAFE_RELEASE(D3D->textures[i].resourceView);
    }
    SAFE_RELEASE(D3D->pSamplerState);

    SAFE_RELEASE(D3D->VertexBuffer.pBuffer);
    SAFE_RELEASE(D3D->ColorBuffer.pBuffer);

    SAFE_RELEASE(D3D->pVSConstants);
    SAFE_RELEASE(D3D->pPSConstants);

    SAFE_RELEASE(D3D->pFanIndexBuffer);
    SAFE_RELEASE(D3D->pLayoutRenderTriangles);

    SAFE_RELEASE(D3D->pBSBlend);
    SAFE_RELEASE(D3D->pBSNoWrite);
    SAFE_RELEASE(D3D->pRSCull);
    SAFE_RELEASE(D3D->pRSNoCull);
    SAFE_RELEASE(D3D->pDepthStencilDrawShapes);
    SAFE_RELEASE(D3D->pDepthStencilDrawAA);
    SAFE_RELEASE(D3D->pDepthStencilFill);
    SAFE_RELEASE(D3D->pDepthStencilDefault);

    // We took a reference to this
    // Don't delete the device though.
    SAFE_RELEASE(D3D->pDeviceContext);
    
    free(D3D->textures);

    free(D3D);
}


struct NVGcontext* nvgCreateD3D11(ID3D11Device* pDevice, int atlasw, int atlash, int edgeaa)
{
    struct NVGparams params;
    struct NVGcontext* ctx = NULL;
    struct D3DNVGcontext* D3D = (struct D3DNVGcontext*)malloc(sizeof(struct D3DNVGcontext));
    if (D3D == NULL) 
    {
        goto error;
    }
    memset(D3D, 0, sizeof(struct D3DNVGcontext));
    D3D->pDevice = pDevice;
    D3D_API_1(pDevice, GetImmediateContext, &D3D->pDeviceContext);

    memset(&params, 0, sizeof(params));
    params.renderCreate = D3Dnvg__renderCreate;
    params.renderCreateTexture = D3Dnvg__renderCreateTexture;
    params.renderDeleteTexture = D3Dnvg__renderDeleteTexture;
    params.renderUpdateTexture = D3Dnvg__renderUpdateTexture;
    params.renderGetTextureSize = D3Dnvg__renderGetTextureSize;
    params.renderViewport = D3Dnvg__renderViewport;
    params.renderFlush = D3Dnvg__renderFlush;
    params.renderFill = D3Dnvg__renderFill;
    params.renderStroke = D3Dnvg__renderStroke;
    params.renderTriangles = D3Dnvg__renderTriangles;
    params.renderDelete = D3Dnvg__renderDelete;
    params.userPtr = D3D;
    params.atlasWidth = atlasw;
    params.atlasHeight = atlash;
    params.edgeAntiAlias = edgeaa;

    D3D->edgeAntiAlias = edgeaa;

    ctx = nvgCreateInternal(&params);
    if (ctx == NULL) goto error;

    return ctx;

error:
    // 'D3D' is freed by nvgDeleteInternal.
    if (ctx != NULL) nvgDeleteInternal(ctx);
    return NULL;
}

void nvgDeleteD3D11(struct NVGcontext* ctx)
{
    nvgDeleteInternal(ctx);
}

#endif //NANOVG_D3D11_IMPLEMENTATION
#endif //NANOVG_D3D11_H