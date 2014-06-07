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
        return float4(1, 0, 0, 1);
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
        return float4(0, 1, 0, 1);
        // Image
        float scissor = scissorMask(input.fpos);
        float strokeAlpha = strokeMask(input.ftcoord);
        // Calculate color fron texture
        float2 pt = (mul((float3x3)paintMat, float3(input.fpos, 1.0))).xy / extent.xy;
            float4 color = g_texture.Sample(g_sampler, pt);
            color = texType == 0 ? color : float4(1, 1, 1, color.x);

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
        return float4(0, 0, 1, 1);
        // Textured tris
        float4 color = g_texture.Sample(g_sampler, input.ftcoord);
            color = texType == 0 ? color : float4(1, 1, 1, color.x);
        return float4(1.0, 0.0, 0.0, 1.0);
        //return (color * input.fcolor);
    }
    return float4(1.0, 1.0, 1.0, 1.0);
}

/*
static const char* fillFragShader =
"uniform mat3 scissorMat;\n"
"uniform vec2 scissorExt;\n"
"uniform vec2 scissorScale;\n"
"uniform mat3 paintMat;\n"
"uniform vec2 extent;\n"
"uniform float radius;\n"
"uniform float feather;\n"
"uniform vec4 innerCol;\n"
"uniform vec4 outerCol;\n"
"uniform float strokeMult;\n"
"uniform sampler2D tex;\n"
"uniform int texType;\n"
"uniform int type;\n"
"in vec2 ftcoord;\n"
"in vec4 fcolor;\n"
"in vec2 fpos;\n"
"out vec4 outColor;\n"
"\n"
"float sdroundrect(vec2 pt, vec2 ext, float rad) {\n"
"	vec2 ext2 = ext - vec2(rad,rad);\n"
"	vec2 d = abs(pt) - ext2;\n"
"	return min(max(d.x,d.y),0.0) + length(max(d,0.0)) - rad;\n"
"}\n"
"\n"
"// Scissoring\n"
"float scissorMask(vec2 p) {\n"
"	vec2 sc = (abs((scissorMat * vec3(p,1.0)).xy) - scissorExt);\n"
"	sc = vec2(0.5,0.5) - sc * scissorScale;\n"
"	return clamp(sc.x,0.0,1.0) * clamp(sc.y,0.0,1.0);\n"
"}\n"
"\n"
"void main(void) {\n"
"	if (type == 0) {			// Gradient\n"
"		float scissor = scissorMask(fpos);\n"
"		// Calculate gradient color using box gradient\n"
"		vec2 pt = (paintMat * vec3(fpos,1.0)).xy;\n"
"		float d = clamp((sdroundrect(pt, extent, radius) + feather*0.5) / feather, 0.0, 1.0);\n"
"		vec4 color = mix(innerCol,outerCol,d);\n"
"		// Combine alpha\n"
"		color.w *= scissor;\n"
"		outColor = color;\n"
"	} else if (type == 1) {		// Image\n"
"		float scissor = scissorMask(fpos);\n"
"		// Calculate color fron texture\n"
"		vec2 pt = (paintMat * vec3(fpos,1.0)).xy / extent;\n"
"		vec4 color = texture(tex, pt);\n"
"   	color = texType == 0 ? color : vec4(1,1,1,color.x);\n"
"		// Combine alpha\n"
"		color.w *= scissor;\n"
"		outColor = color;\n"
"	} else if (type == 2) {		// Stencil fill\n"
"		outColor = vec4(1,1,1,1);\n"
"	} else if (type == 3) {		// Textured tris\n"
"		vec4 color = texture(tex, ftcoord);\n"
"   	color = texType == 0 ? color : vec4(1,1,1,color.x);\n"
"		outColor = color * fcolor;\n"
"	}\n"
"}\n";
*/
