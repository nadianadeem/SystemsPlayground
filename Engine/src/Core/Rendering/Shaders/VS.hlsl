cbuffer TransformCB : register(b0)
{
    row_major float4x4 transform;
};

struct VSInput
{
    float3 pos : POSITION;
    float4 col : COLOR;
};

struct PSInput
{
    float4 pos : SV_POSITION;
    float4 col : COLOR;
};

PSInput main(VSInput input)
{
    PSInput output;
    output.pos = mul(float4(input.pos, 1.0f), transform);
    output.col = input.col;
    return output;
}
