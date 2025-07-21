#include "pch.h"
#include "imgui_impl_dx9.h"

// DirectX data
struct ImGui_ImplDX9_Data
{
    D3DDevice *pd3dDevice;
    D3DVertexDeclaration *pVD;
    D3DVertexBuffer *pVB;
    D3DIndexBuffer *pIB;
    D3DVertexShader *pVS;
    D3DPixelShader *pPS;
    D3DTexture *FontTexture;
    int VertexBufferSize;
    int IndexBufferSize;

    ImGui_ImplDX9_Data()
    {
        memset(this, 0, sizeof(*this));
        VertexBufferSize = 5000;
        IndexBufferSize = 10000;
    }
};

struct CUSTOMVERTEX
{
    float pos[3];
    D3DCOLOR col;
    float uv[2];
};

#ifdef IMGUI_USE_BGRA_PACKED_COLOR
    #define IMGUI_COL_TO_DX9_ARGB(_COL) (_COL)
#else
    #define IMGUI_COL_TO_DX9_ARGB(_COL) (((_COL) & 0xFF00FF00) | (((_COL) & 0xFF0000) >> 16) | (((_COL) & 0xFF) << 16))
#endif

const char *g_VertexShaderProgram =
    "float4x4 matWVP : register(c0);"
    ""
    "struct VS_IN"
    "{"
    "    float4 ObjPos : POSITION;"
    "    float4 Color : COLOR;"
    "    float2 UV : TEXCOORD0;"
    "};"
    ""
    "struct VS_OUT"
    "{"
    "    float4 ProjPos : POSITION;"
    "    float4 Color : COLOR;"
    "    float2 UV : TEXCOORD0;"
    "};"
    ""
    "VS_OUT main(VS_IN In)"
    "{"
    "    VS_OUT Out;"
    "    Out.ProjPos = mul(matWVP, In.ObjPos);"
    "    Out.Color = In.Color;"
    "    Out.UV = In.UV;"
    "    return Out;"
    "}";

const char *g_PixelShaderProgram =
    "struct PS_IN"
    "{"
    "    float4 ProjPos : POSITION;"
    "    float4 Color : COLOR;"
    "    float2 UV : TEXCOORD0;"
    "};"
    ""
    "sampler TextureSampler : register(s0);"
    "Texture2D Texture : register(t0);"
    ""
    "float4 main(PS_IN In) : COLOR"
    "{"
    "    float4 TextureColor = Texture.Sample(TextureSampler, In.UV);"
    "    return In.Color * TextureColor;"
    "}";

static ImGui_ImplDX9_Data *ImGui_ImplDX9_GetBackendData()
{
    return ImGui::GetCurrentContext() ? (ImGui_ImplDX9_Data *)ImGui::GetIO().BackendRendererUserData : NULL;
}

static bool ImGui_ImplDX9_LoadShaders()
{
    ImGui_ImplDX9_Data *bd = ImGui_ImplDX9_GetBackendData();

    // Buffers to hold compiled shaders and possible error messages
    ID3DXBuffer *pShaderCode = NULL;
    ID3DXBuffer *pErrorMsg = NULL;

    // Compile vertex shader
    HRESULT hr = D3DXCompileShader(g_VertexShaderProgram, strlen(g_VertexShaderProgram), NULL, NULL, "main", "vs_2_0", 0, &pShaderCode, &pErrorMsg, NULL);
    if (FAILED(hr))
    {
        OutputDebugStringA("Couldn't compile vertex shader.\n");
        OutputDebugStringA(pErrorMsg ? (char *)pErrorMsg->GetBufferPointer() : "");
        return false;
    }

    // Create vertex shader
    bd->pd3dDevice->CreateVertexShader((DWORD *)pShaderCode->GetBufferPointer(), &bd->pVS);

    // Shader code is no longer required
    pShaderCode->Release();
    pShaderCode = NULL;

    // Compile pixel shader
    hr = D3DXCompileShader(g_PixelShaderProgram, strlen(g_PixelShaderProgram), NULL, NULL, "main", "ps_2_0", 0, &pShaderCode, &pErrorMsg, NULL);
    if (FAILED(hr))
    {
        OutputDebugStringA("Couldn't compile pixel shader.\n");
        OutputDebugStringA(pErrorMsg ? (char *)pErrorMsg->GetBufferPointer() : "");
        return false;
    }

    // Create pixel shader
    bd->pd3dDevice->CreatePixelShader((DWORD *)pShaderCode->GetBufferPointer(), &bd->pPS);

    // Shader code no longer required.
    pShaderCode->Release();
    pShaderCode = NULL;

    return true;
}

static bool ImGui_ImplDX9_CreateVertexElements()
{
    ImGui_ImplDX9_Data *bd = ImGui_ImplDX9_GetBackendData();

    // Define the vertex elements
    static const D3DVERTEXELEMENT9 VertexElements[] = {
        { 0, 0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0 },
        { 0, sizeof(float) * 3, D3DDECLTYPE_D3DCOLOR, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_COLOR, 0 },
        { 0, sizeof(float) * 3 + sizeof(D3DCOLOR), D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0 },
        D3DDECL_END()
    };

    // Create a vertex declaration from the element descriptions
    bd->pd3dDevice->CreateVertexDeclaration(VertexElements, &bd->pVD);

    return true;
}

bool ImGui_ImplDX9_Init(IDirect3DDevice9 *device)
{
    ImGuiIO &io = ImGui::GetIO();
    IM_ASSERT(io.BackendRendererUserData == NULL && "Already initialized a renderer backend!");

    // Setup backend capabilities flags
    ImGui_ImplDX9_Data *bd = IM_NEW(ImGui_ImplDX9_Data)();
    io.BackendRendererUserData = (void *)bd;
    io.BackendRendererName = "imgui_impl_dx9";
    io.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset; // We can honor the ImDrawCmd::VtxOffset field, allowing for large meshes.

    bd->pd3dDevice = device;
    bd->pd3dDevice->AddRef();

    // Load shaders
    if (!ImGui_ImplDX9_LoadShaders())
        return false;

    // Setup vertex layout
    if (!ImGui_ImplDX9_CreateVertexElements())
        return false;

    return true;
}

static bool ImGui_ImplDX9_CreateFontsTexture()
{
    // Build texture atlas
    ImGuiIO &io = ImGui::GetIO();
    ImGui_ImplDX9_Data *bd = ImGui_ImplDX9_GetBackendData();
    unsigned char *pixels;
    int width, height, bytes_per_pixel;
    io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height, &bytes_per_pixel);

    // Convert RGBA32 to BGRA32 (because RGBA32 is not well supported by DX9 devices)
#ifndef IMGUI_USE_BGRA_PACKED_COLOR
    if (io.Fonts->TexPixelsUseColors)
    {
        ImU32 *dst_start = (ImU32 *)ImGui::MemAlloc((size_t)width * height * bytes_per_pixel);
        for (ImU32 *src = (ImU32 *)pixels, *dst = dst_start, *dst_end = dst_start + (size_t)width * height; dst < dst_end; src++, dst++)
            *dst = IMGUI_COL_TO_DX9_ARGB(*src);
        pixels = (unsigned char *)dst_start;
    }
#endif

    // Upload texture to graphics system
    bd->FontTexture = NULL;
    if (bd->pd3dDevice->CreateTexture(width, height, 1, 0, D3DFMT_LIN_A8R8G8B8, D3DPOOL_DEFAULT, &bd->FontTexture, NULL) < 0)
        return false;
    D3DLOCKED_RECT tex_locked_rect;
    if (bd->FontTexture->LockRect(0, &tex_locked_rect, NULL, 0) != D3D_OK)
        return false;
    for (int y = 0; y < height; y++)
        memcpy((unsigned char *)tex_locked_rect.pBits + (size_t)tex_locked_rect.Pitch * y, pixels + (size_t)width * bytes_per_pixel * y, (size_t)width * bytes_per_pixel);
    bd->FontTexture->UnlockRect(0);

    // Store our identifier
    io.Fonts->SetTexID((ImTextureID)bd->FontTexture);

#ifndef IMGUI_USE_BGRA_PACKED_COLOR
    if (io.Fonts->TexPixelsUseColors)
        ImGui::MemFree(pixels);
#endif

    return true;
}

static bool ImGui_ImplDX9_CreateDeviceObjects()
{
    ImGui_ImplDX9_Data *bd = ImGui_ImplDX9_GetBackendData();
    if (!bd || !bd->pd3dDevice)
        return false;
    if (!ImGui_ImplDX9_CreateFontsTexture())
        return false;
    return true;
}

void ImGui_ImplDX9_NewFrame()
{
    ImGui_ImplDX9_Data *bd = ImGui_ImplDX9_GetBackendData();
    IM_ASSERT(bd != NULL && "Did you call ImGui_ImplDX9_Init()?");

    if (!bd->FontTexture)
        ImGui_ImplDX9_CreateDeviceObjects();
}

static void ImGui_ImplDX9_SetupRenderState(ImDrawData *draw_data)
{
    ImGui_ImplDX9_Data *bd = ImGui_ImplDX9_GetBackendData();

    // Setup render state: alpha-blending, no face culling, no depth testing
    bd->pd3dDevice->SetRenderState(D3DRS_FILLMODE, D3DFILL_SOLID);
    bd->pd3dDevice->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);
    bd->pd3dDevice->SetRenderState(D3DRS_ALPHATESTENABLE, FALSE);
    bd->pd3dDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
    bd->pd3dDevice->SetRenderState(D3DRS_ZENABLE, FALSE);
    bd->pd3dDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
    bd->pd3dDevice->SetRenderState(D3DRS_BLENDOP, D3DBLENDOP_ADD);
    bd->pd3dDevice->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
    bd->pd3dDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
    bd->pd3dDevice->SetRenderState(D3DRS_SEPARATEALPHABLENDENABLE, TRUE);
    bd->pd3dDevice->SetRenderState(D3DRS_SRCBLENDALPHA, D3DBLEND_ONE);
    bd->pd3dDevice->SetRenderState(D3DRS_DESTBLENDALPHA, D3DBLEND_INVSRCALPHA);
    bd->pd3dDevice->SetRenderState(D3DRS_SCISSORTESTENABLE, TRUE);
    bd->pd3dDevice->SetRenderState(D3DRS_STENCILENABLE, FALSE);
    bd->pd3dDevice->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
    bd->pd3dDevice->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);

    float L = draw_data->DisplayPos.x + 0.5f;
    float R = draw_data->DisplayPos.x + draw_data->DisplaySize.x + 0.5f;
    float T = draw_data->DisplayPos.y + 0.5f;
    float B = draw_data->DisplayPos.y + draw_data->DisplaySize.y + 0.5f;
    XMMATRIX mat_world = XMMatrixIdentity();
    XMMATRIX mat_view = XMMatrixIdentity();
    XMMATRIX mat_proj = XMMatrixOrthographicOffCenterLH(L, R, B, T, -1.0f, 1.0f);
    XMMATRIX mat_wvp = mat_world * mat_view * mat_proj;

    bd->pd3dDevice->SetVertexShader(bd->pVS);
    bd->pd3dDevice->SetPixelShader(bd->pPS);
    bd->pd3dDevice->SetVertexShaderConstantF(0, (float *)&mat_wvp, 4);
    bd->pd3dDevice->SetVertexDeclaration(bd->pVD);
}

// Render function
void ImGui_ImplDX9_RenderDrawData(ImDrawData *draw_data)
{
    // Create and grow buffers if needed
    ImGui_ImplDX9_Data *bd = ImGui_ImplDX9_GetBackendData();
    if (!bd->pVB || bd->VertexBufferSize < draw_data->TotalVtxCount)
    {
        if (bd->pVB)
        {
            bd->pVB->Release();
            bd->pVB = NULL;
        }
        bd->VertexBufferSize = draw_data->TotalVtxCount + 5000;
        if (bd->pd3dDevice->CreateVertexBuffer(bd->VertexBufferSize * sizeof(CUSTOMVERTEX), D3DUSAGE_WRITEONLY, 0, D3DPOOL_DEFAULT, &bd->pVB, NULL) < 0)
            return;
    }
    if (!bd->pIB || bd->IndexBufferSize < draw_data->TotalIdxCount)
    {
        if (bd->pIB)
        {
            bd->pIB->Release();
            bd->pIB = NULL;
        }
        bd->IndexBufferSize = draw_data->TotalIdxCount + 10000;
        if (bd->pd3dDevice->CreateIndexBuffer(bd->IndexBufferSize * sizeof(ImDrawIdx), D3DUSAGE_WRITEONLY, sizeof(ImDrawIdx) == 2 ? D3DFMT_INDEX16 : D3DFMT_INDEX32, D3DPOOL_DEFAULT, &bd->pIB, NULL) < 0)
            return;
    }

    // Backup the DX9 state
    IDirect3DStateBlock9 *d3d9_state_block = NULL;
    if (bd->pd3dDevice->CreateStateBlock(D3DSBT_ALL, &d3d9_state_block) < 0)
        return;
    if (d3d9_state_block->Capture() < 0)
    {
        d3d9_state_block->Release();
        return;
    }

    // Allocate buffers
    CUSTOMVERTEX *vtx_dst;
    ImDrawIdx *idx_dst;
    if (bd->pVB->Lock(0, (UINT)(draw_data->TotalVtxCount * sizeof(CUSTOMVERTEX)), (void **)&vtx_dst, 0) < 0)
    {
        d3d9_state_block->Release();
        return;
    }
    if (bd->pIB->Lock(0, (UINT)(draw_data->TotalIdxCount * sizeof(ImDrawIdx)), (void **)&idx_dst, 0) < 0)
    {
        bd->pVB->Unlock();
        d3d9_state_block->Release();
        return;
    }

    // Copy and convert all vertices into a single contiguous buffer, convert colors to DX9 default format.
    // FIXME-OPT: This is a minor waste of resource, the ideal is to use imconfig.h and
    //  1) to avoid repacking colors:   #define IMGUI_USE_BGRA_PACKED_COLOR
    //  2) to avoid repacking vertices: #define IMGUI_OVERRIDE_DRAWVERT_STRUCT_LAYOUT struct ImDrawVert { ImVec2 pos; float z; ImU32 col; ImVec2 uv; }
    for (int n = 0; n < draw_data->CmdListsCount; n++)
    {
        const ImDrawList *cmd_list = draw_data->CmdLists[n];
        const ImDrawVert *vtx_src = cmd_list->VtxBuffer.Data;
        for (int i = 0; i < cmd_list->VtxBuffer.Size; i++)
        {
            vtx_dst->pos[0] = vtx_src->pos.x;
            vtx_dst->pos[1] = vtx_src->pos.y;
            vtx_dst->pos[2] = 0.0f;
            vtx_dst->col = IMGUI_COL_TO_DX9_ARGB(vtx_src->col);
            vtx_dst->uv[0] = vtx_src->uv.x;
            vtx_dst->uv[1] = vtx_src->uv.y;
            vtx_dst++;
            vtx_src++;
        }
        memcpy(idx_dst, cmd_list->IdxBuffer.Data, cmd_list->IdxBuffer.Size * sizeof(ImDrawIdx));
        idx_dst += cmd_list->IdxBuffer.Size;
    }
    bd->pVB->Unlock();
    bd->pIB->Unlock();
    bd->pd3dDevice->SetStreamSource(0, bd->pVB, 0, sizeof(CUSTOMVERTEX));
    bd->pd3dDevice->SetIndices(bd->pIB);

    // Setup desired DX state
    ImGui_ImplDX9_SetupRenderState(draw_data);

    // Render command lists
    // (Because we merged all buffers into a single one, we maintain our own offset into them)
    int global_vtx_offset = 0;
    int global_idx_offset = 0;
    ImVec2 clip_off = draw_data->DisplayPos;
    for (int n = 0; n < draw_data->CmdListsCount; n++)
    {
        const ImDrawList *cmd_list = draw_data->CmdLists[n];
        for (int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.Size; cmd_i++)
        {
            const ImDrawCmd *pcmd = &cmd_list->CmdBuffer[cmd_i];
            if (pcmd->UserCallback != NULL)
            {
                // User callback, registered via ImDrawList::AddCallback()
                // (ImDrawCallback_ResetRenderState is a special callback value used by the user to request the renderer to reset render state.)
                if (pcmd->UserCallback == ImDrawCallback_ResetRenderState)
                    ImGui_ImplDX9_SetupRenderState(draw_data);
                else
                    pcmd->UserCallback(cmd_list, pcmd);
            }
            else
            {
                // Project scissor/clipping rectangles into framebuffer space
                ImVec2 clip_min(pcmd->ClipRect.x - clip_off.x, pcmd->ClipRect.y - clip_off.y);
                ImVec2 clip_max(pcmd->ClipRect.z - clip_off.x, pcmd->ClipRect.w - clip_off.y);
                if (clip_max.x <= clip_min.x || clip_max.y <= clip_min.y)
                    continue;

                // Apply Scissor/clipping rectangle, Bind texture, Draw
                const RECT r = { (LONG)clip_min.x, (LONG)clip_min.y, (LONG)clip_max.x, (LONG)clip_max.y };
                const LPDIRECT3DTEXTURE9 texture = (LPDIRECT3DTEXTURE9)pcmd->GetTexID();
                bd->pd3dDevice->SetTexture(0, texture);
                bd->pd3dDevice->SetScissorRect(&r);
                bd->pd3dDevice->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, pcmd->VtxOffset + global_vtx_offset, 0, (UINT)cmd_list->VtxBuffer.Size, pcmd->IdxOffset + global_idx_offset, pcmd->ElemCount / 3);
            }
        }
        global_idx_offset += cmd_list->IdxBuffer.Size;
        global_vtx_offset += cmd_list->VtxBuffer.Size;
    }

    // Restore the DX9 state
    d3d9_state_block->Apply();
    d3d9_state_block->Release();
}
