cbuffer Transform : register(b0)
{
    float4x4 uMVP;
};

struct VSInput
{
    float3 pos : POSITION;
    float4 color : COLOR;
};

struct VSOutput
{
    float4 pos : SV_POSITION;
    float4 color : COLOR;
};

VSOutput main(VSInput input)
{
    VSOutput o;
    o.pos = mul(uMVP, float4(input.pos, 1.0f));
    o.color = input.color;
    return o;
}
