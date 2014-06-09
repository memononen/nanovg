Texture2D    g_texture                    : register(t0);
SamplerState g_sampler                 : register(s0);

struct PS_INPUT
{
    float4 position   : SV_Position;    // vertex position
    float2 ftcoord    : TEXCOORD0;      // float 2 tex coord
    float2 fpos       : TEXCOORD1;      // float 2 position 
    float4 fcolor     : COLOR0;        // vertex diffuse color
};

cbuffer PS_CONSTANTS
{
    float4x4 scissorMat;
    float4 scissorExt;
    float4 scissorScale;
    float4x4 paintMat;
    float4 extent;
    float4 radius;
    float4 feather;
    float4 innerCol;
    float4 outerCol;
    float4 strokeMult;
    int texType;
    int type;
};
 

float sdroundrect(float2 pt, float2 ext, float rad)
{
    float2 ext2 = ext - float2(rad, rad);
    float2 d = abs(pt) - ext2;
    return min(max(d.x, d.y), 0.0) + length(max(d, 0.0)) - rad;
}

// Scissoring
float scissorMask(float2 p)
{
    float2 sc = (abs((mul((float3x3)scissorMat, float3(p.x, p.y, 1.0))).xy) - scissorExt.xy);
        sc = float2(0.5, 0.5) - sc * scissorScale.xy;
    return clamp(sc.x, 0.0, 1.0) * clamp(sc.y, 0.0, 1.0);

    return .0f;
}

// Stroke - from [0..1] to clipped pyramid, where the slope is 1px.
float strokeMask(float2 ftcoord)
{
    return min(1.0, (1.0 - abs(ftcoord.x*2.0 - 1.0))*strokeMult.x) * ftcoord.y;
}

float4 D3D11PixelShader_Main(PS_INPUT input) : SV_TARGET
{
    if (type == 0)
    {
        // Gradient
        float scissor = scissorMask(input.fpos);
        float strokeAlpha = strokeMask(input.ftcoord);

        // Calculate gradient color using box gradient
        float2 pt = (mul((float3x3)paintMat, float3(input.fpos, 1.0))).xy;
            float d = clamp((sdroundrect(pt, extent.xy, radius.x) + feather.x*0.5) / feather.x, 0.0, 1.0);
        float4 color = lerp(innerCol, outerCol, d);

        // Combine alpha
        color.w *= strokeAlpha * scissor;
        return color;
    }
    else if (type == 1)
    {
        // Image
        float scissor = scissorMask(input.fpos);
        float strokeAlpha = strokeMask(input.ftcoord);
        // Calculate color fron texture
        float2 pt = (mul((float3x3)paintMat, float3(input.fpos, 1.0))).xy / extent.xy;
            float4 color = g_texture.Sample(g_sampler, pt);
            color = texType == 0 ? color : float4(1, 1, 1, color.x);

        // Apply color tint and alpha.
        color *= innerCol;

        // Combine alpha
        color.w *= strokeAlpha * scissor;
        return color;
    }
    else if (type == 2)
    {
        // Stencil fill
        return float4(1, 1, 1, 1);
    }
    else if (type == 3)
    {
        // Textured tris
        float4 color = g_texture.Sample(g_sampler, input.ftcoord);
            color = texType == 0 ? color : float4(1, 1, 1, color.x);
        return (color * input.fcolor);
    }
    return float4(1.0, 1.0, 1.0, 1.0);
}
