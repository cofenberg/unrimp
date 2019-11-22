/*********************************************************\
 * Copyright (c) 2012-2019 The Unrimp Team
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this software
 * and associated documentation files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or
 * substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING
 * BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
\*********************************************************/


//[-------------------------------------------------------]
//[ Shader start                                          ]
//[-------------------------------------------------------]
#if defined(RHI_DIRECT3D10) || defined(RHI_DIRECT3D11) || defined(RHI_DIRECT3D12)
if (rhi->getNameId() == Rhi::NameId::DIRECT3D10 || rhi->getNameId() == Rhi::NameId::DIRECT3D11 || rhi->getNameId() == Rhi::NameId::DIRECT3D12)
{


//[-------------------------------------------------------]
//[ Vertex shader source code                             ]
//[-------------------------------------------------------]
// One vertex shader invocation per vertex
vertexShaderSourceCode = R"(
struct Vertex
{
	float2 position;
	float2 padding;
};

// Attribute input/output
struct VS_OUTPUT
{
	float4 Position : SV_POSITION;	// Clip space vertex position as output, left/bottom is (-1,-1) and right/top is (1,1)
	float2 TexCoord : TEXCOORD0;	// Normalized texture coordinate as output
};

// Uniforms
tbuffer InputTextureBuffer : register(t0)
{
	float4 inputPositionOffset[3];
};
StructuredBuffer<Vertex> InputStructuredBuffer : register(t1);

// Programs
VS_OUTPUT main(float2 Position : POSITION,	// Clip space vertex position as input, left/bottom is (-1,-1) and right/top is (1,1)
			   uint   VertexId : SV_VERTEXID)
{
	// Pass through the clip space vertex position, left/bottom is (-1,-1) and right/top is (1,1)
	VS_OUTPUT output;
	output.Position = float4(Position + inputPositionOffset[VertexId].xy + InputStructuredBuffer[VertexId].position, 0.5f, 1.0f);
	output.TexCoord = Position.xy;
	return output;
}
)";


//[-------------------------------------------------------]
//[ Fragment shader source code                           ]
//[-------------------------------------------------------]
// One fragment shader invocation per fragment
// "pixel shader" in Direct3D terminology
fragmentShaderSourceCode = R"(
// Uniforms
cbuffer InputUniformBuffer : register(b0)
{
	float4 inputColorUniform;
}
SamplerState SamplerLinear : register(s0);
Texture2D AlbedoMap : register(t1);

// Programs
float4 main(float4 Position : SV_POSITION, float2 TexCoord : TEXCOORD0) : SV_TARGET
{
	// Fetch the texel at the given texture coordinate and return its color
	return AlbedoMap.Sample(SamplerLinear, TexCoord) * inputColorUniform;
}
)";


//[-------------------------------------------------------]
//[ Compute shader source code                            ]
//[-------------------------------------------------------]
computeShaderSourceCode1 = R"(
// Input
Texture2D<float4>	InputTexture2D		 : register(t0);
ByteAddressBuffer	InputVertexBuffer	 : register(t1);
tbuffer				InputIndexBuffer	 : register(t2)
{
	uint inputIndexBuffer[3];
};
cbuffer				InputUniformBuffer	 : register(b0)
{
	float4 inputColorUniform;
}

// Output
RWTexture2D<float4>	OutputTexture2D		 : register(u0);
RWByteAddressBuffer	OutputVertexBuffer   : register(u1);
RWBuffer<uint>		OutputIndexBuffer    : register(u2);

// Programs
[numthreads(16, 16, 1)]
void main(uint3 dispatchThreadId : SV_DispatchThreadID)
{
	// Fetch input texel
	float4 color = InputTexture2D.Load(dispatchThreadId) * inputColorUniform;

	// Modify color
	color.g *= 1.0f - (float(dispatchThreadId.x) / 16.0f);
	color.g *= 1.0f - (float(dispatchThreadId.y) / 16.0f);

	// Output texel
	OutputTexture2D[dispatchThreadId.xy] = color;

	// Output buffer
	if (0 == dispatchThreadId.x && 0 == dispatchThreadId.y && 0 == dispatchThreadId.z)
	{
		// Output vertex buffer values
		// -> Using a structured vertex buffer would be handy inside shader source codes, sadly this isn't possible with Direct3D 11 and will result in the following error:
		//    D3D11 ERROR: ID3D11Device::CreateBuffer: Buffers created with D3D11_RESOURCE_MISC_BUFFER_STRUCTURED cannot specify any of the following listed bind flags.  The following BindFlags bits (0x9) are set: D3D11_BIND_VERTEX_BUFFER (1), D3D11_BIND_INDEX_BUFFER (0), D3D11_BIND_CONSTANT_BUFFER (0), D3D11_BIND_STREAM_OUTPUT (0), D3D11_BIND_RENDER_TARGET (0), or D3D11_BIND_DEPTH_STENCIL (0). [ STATE_CREATION ERROR #68: CREATEBUFFER_INVALIDMISCFLAGS]
		for (int vertexBufferIndex = 0; vertexBufferIndex < 3; ++vertexBufferIndex)
		{
			float2 position = asfloat(InputVertexBuffer.Load2(vertexBufferIndex * 8));
			OutputVertexBuffer.Store2(vertexBufferIndex * 8, asuint(position));
		}

		// Output index buffer values
		for (int indexBufferIndex = 0; indexBufferIndex < 3; ++indexBufferIndex)
		{
			OutputIndexBuffer[indexBufferIndex] = inputIndexBuffer[indexBufferIndex];
		}

		// Output uniform buffer not possible by design
	}
}
)";

computeShaderSourceCode2 = R"(
struct Vertex
{
	float2 position;
	float2 padding;
};

// Input
tbuffer					 InputTextureBuffer    : register(t0)
{
	float4 inputPositionOffset[3];
};
StructuredBuffer<Vertex> InputStructuredBuffer : register(t1);
tbuffer					 InputIndirectBuffer   : register(t2)
{
	uint inputIndirectBuffer[5];
};

// Output
RWBuffer<float4>		   OutputTextureBuffer    : register(u0);
RWStructuredBuffer<Vertex> OutputStructuredBuffer : register(u1);
RWBuffer<uint>			   OutputIndirectBuffer   : register(u2);

// Programs
[numthreads(3, 1, 1)]
void main(uint3 dispatchThreadId : SV_DispatchThreadID)
{
	// Output buffer
	if (0 == dispatchThreadId.x && 0 == dispatchThreadId.y && 0 == dispatchThreadId.z)
	{
		// Output texture buffer values
		for (int textureBufferIndex = 0; textureBufferIndex < 3; ++textureBufferIndex)
		{
			OutputTextureBuffer[textureBufferIndex] = inputPositionOffset[textureBufferIndex];
		}

		// Output structured buffer values
		for (int structuredBufferIndex = 0; structuredBufferIndex < 3; ++structuredBufferIndex)
		{
			OutputStructuredBuffer[structuredBufferIndex] = InputStructuredBuffer[structuredBufferIndex];
		}

		// Output indirect buffer values (draw calls)
		// -> Using a structured indirect buffer would be handy inside shader source codes, sadly this isn't possible with Direct3D 11 and will result in the following error:
		//    "D3D11 ERROR: ID3D11Device::CreateBuffer: A resource cannot created with both D3D11_RESOURCE_MISC_DRAWINDIRECT_ARGS and D3D11_RESOURCE_MISC_BUFFER_STRUCTURED. [ STATE_CREATION ERROR #68: CREATEBUFFER_INVALIDMISCFLAGS]"
	//	OutputIndirectBuffer[0] = inputIndirectBuffer[0];	// Rhi::DrawIndexedArguments::indexCountPerInstance	- Filled by compute shader via atomics counting
		OutputIndirectBuffer[1] = inputIndirectBuffer[1];	// Rhi::DrawIndexedArguments::instanceCount
		OutputIndirectBuffer[2] = inputIndirectBuffer[2];	// Rhi::DrawIndexedArguments::startIndexLocation
		OutputIndirectBuffer[3] = inputIndirectBuffer[3];	// Rhi::DrawIndexedArguments::baseVertexLocation
		OutputIndirectBuffer[4] = inputIndirectBuffer[4];	// Rhi::DrawIndexedArguments::startInstanceLocation
	}

	// Atomics for counting usage example
	// -> Change '[numthreads(3, 1, 1)]' into '[numthreads(2, 1, 1)]' and if the triangle is gone you know the counter reset worked
	if (0 == dispatchThreadId.x)
	{
		// Reset the counter on first invocation
		uint originalValue;
		InterlockedExchange(OutputIndirectBuffer[0], 0, originalValue);
	}
	InterlockedAdd(OutputIndirectBuffer[0], 1);
}
)";


//[-------------------------------------------------------]
//[ Shader end                                            ]
//[-------------------------------------------------------]
}
else
#endif
