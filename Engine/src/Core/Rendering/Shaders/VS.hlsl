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

cbuffer MVP : register(b0)
{
    float4x4 uMVP;
};

VSOutput main(VSInput input)
{
    VSOutput o;
    o.pos = mul(uMVP, float4(input.pos, 1.0f));
    o.color = input.color; // this is fine
    return o;
}
