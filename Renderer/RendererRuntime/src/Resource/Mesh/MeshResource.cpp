/*********************************************************\
 * Copyright (c) 2012-2018 The Unrimp Team
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
//[ Includes                                              ]
//[-------------------------------------------------------]
#include "RendererRuntime/Resource/Mesh/MeshResource.h"

// Disable warnings in external headers, we can't fix them
PRAGMA_WARNING_PUSH
	PRAGMA_WARNING_DISABLE_MSVC(4464)	// warning C4464: relative include path contains '..'
	#include <glm/detail/setup.hpp>
PRAGMA_WARNING_POP


//[-------------------------------------------------------]
//[ Anonymous detail namespace                            ]
//[-------------------------------------------------------]
namespace
{
	namespace detail
	{


		//[-------------------------------------------------------]
		//[ Global variables                                      ]
		//[-------------------------------------------------------]
		// Vertex input layout
		static constexpr Renderer::VertexAttribute StaticVertexAttributesLayout[] =
		{
			{ // Attribute 0
				// Data destination
				Renderer::VertexAttributeFormat::FLOAT_3,	// vertexAttributeFormat (Renderer::VertexAttributeFormat)
				"Position",									// name[32] (char)
				"POSITION",									// semanticName[32] (char)
				0,											// semanticIndex (uint32_t)
				// Data source
				0,											// inputSlot (uint32_t)
				0,											// alignedByteOffset (uint32_t)
				sizeof(float) * 5 + sizeof(short) * 4,		// strideInBytes (uint32_t)
				0											// instancesPerElement (uint32_t)
			},
			{ // Attribute 1
				// Data destination
				Renderer::VertexAttributeFormat::FLOAT_2,	// vertexAttributeFormat (Renderer::VertexAttributeFormat)
				"TexCoord",									// name[32] (char)
				"TEXCOORD",									// semanticName[32] (char)
				0,											// semanticIndex (uint32_t)
				// Data source
				0,											// inputSlot (uint32_t)
				sizeof(float) * 3,							// alignedByteOffset (uint32_t)
				sizeof(float) * 5 + sizeof(short) * 4,		// strideInBytes (uint32_t)
				0											// instancesPerElement (uint32_t)
			},
			{ // Attribute 2
				// Data destination
				Renderer::VertexAttributeFormat::SHORT_4,	// vertexAttributeFormat (Renderer::VertexAttributeFormat)
				"QTangent",									// name[32] (char)
				"TEXCOORD",									// semanticName[32] (char)
				1,											// semanticIndex (uint32_t)
				// Data source
				0,											// inputSlot (uint32_t)
				sizeof(float) * 5,							// alignedByteOffset (uint32_t)
				sizeof(float) * 5 + sizeof(short) * 4,		// strideInBytes (uint32_t)
				0											// instancesPerElement (uint32_t)
			},
			{ // Attribute 3, see "17/11/2012 Surviving without gl_DrawID" - https://www.g-truc.net/post-0518.html
				// Data destination
				Renderer::VertexAttributeFormat::UINT_1,	// vertexAttributeFormat (Renderer::VertexAttributeFormat)
				"drawId",									// name[32] (char)
				"DRAWID",									// semanticName[32] (char)
				0,											// semanticIndex (uint32_t)
				// Data source
				1,											// inputSlot (uint32_t)
				0,											// alignedByteOffset (uint32_t)
				sizeof(uint32_t),							// strideInBytes (uint32_t)
				1											// instancesPerElement (uint32_t)
			}
		};
		static constexpr Renderer::VertexAttribute SkinnedVertexAttributesLayout[] =
		{
			{ // Attribute 0
				// Data destination
				Renderer::VertexAttributeFormat::FLOAT_3,						// vertexAttributeFormat (Renderer::VertexAttributeFormat)
				"Position",														// name[32] (char)
				"POSITION",														// semanticName[32] (char)
				0,																// semanticIndex (uint32_t)
				// Data source
				0,																// inputSlot (uint32_t)
				0,																// alignedByteOffset (uint32_t)
				sizeof(float) * 5 + sizeof(short) * 4 + sizeof(uint8_t) * 8,	// strideInBytes (uint32_t)
				0																// instancesPerElement (uint32_t)
			},
			{ // Attribute 1
				// Data destination
				Renderer::VertexAttributeFormat::FLOAT_2,						// vertexAttributeFormat (Renderer::VertexAttributeFormat)
				"TexCoord",														// name[32] (char)
				"TEXCOORD",														// semanticName[32] (char)
				0,																// semanticIndex (uint32_t)
				// Data source
				0,																// inputSlot (uint32_t)
				sizeof(float) * 3,												// alignedByteOffset (uint32_t)
				sizeof(float) * 5 + sizeof(short) * 4 + sizeof(uint8_t) * 8,	// strideInBytes (uint32_t)
				0																// instancesPerElement (uint32_t)
			},
			{ // Attribute 2
				// Data destination
				Renderer::VertexAttributeFormat::SHORT_4,						// vertexAttributeFormat (Renderer::VertexAttributeFormat)
				"QTangent",														// name[32] (char)
				"TEXCOORD",														// semanticName[32] (char)
				1,																// semanticIndex (uint32_t)
				// Data source
				0,																// inputSlot (uint32_t)
				sizeof(float) * 5,												// alignedByteOffset (uint32_t)
				sizeof(float) * 5 + sizeof(short) * 4 + sizeof(uint8_t) * 8,	// strideInBytes (uint32_t)
				0																// instancesPerElement (uint32_t)
			},
			{ // Attribute 3, see "17/11/2012 Surviving without gl_DrawID" - https://www.g-truc.net/post-0518.html
				// Data destination
				Renderer::VertexAttributeFormat::UINT_1,	// vertexAttributeFormat (Renderer::VertexAttributeFormat)
				"drawId",									// name[32] (char)
				"DRAWID",									// semanticName[32] (char)
				0,											// semanticIndex (uint32_t)
				// Data source
				1,											// inputSlot (uint32_t)
				0,											// alignedByteOffset (uint32_t)
				sizeof(uint32_t),							// strideInBytes (uint32_t)
				1											// instancesPerElement (uint32_t)
			},
			{ // Attribute 4
				// Data destination
				Renderer::VertexAttributeFormat::R8G8B8A8_UINT,					// vertexAttributeFormat (Renderer::VertexAttributeFormat)
				"BlendIndices",													// name[32] (char)
				"BLENDINDICES",													// semanticName[32] (char)
				0,																// semanticIndex (uint32_t)
				// Data source
				0,																// inputSlot (uint32_t)
				sizeof(float) * 5 + sizeof(short) * 4,							// alignedByteOffset (uint32_t)
				sizeof(float) * 5 + sizeof(short) * 4 + sizeof(uint8_t) * 8,	// strideInBytes (uint32_t)
				0																// instancesPerElement (uint32_t)
			},
			{ // Attribute 5
				// Data destination
				Renderer::VertexAttributeFormat::R8G8B8A8_UINT,					// vertexAttributeFormat (Renderer::VertexAttributeFormat)
				"BlendWeights",													// name[32] (char)
				"BLENDWEIGHT",													// semanticName[32] (char)
				0,																// semanticIndex (uint32_t)
				// Data source
				0,																// inputSlot (uint32_t)
				sizeof(float) * 5 + sizeof(short) * 4 + sizeof(uint8_t) * 4,	// alignedByteOffset (uint32_t)
				sizeof(float) * 5 + sizeof(short) * 4 + sizeof(uint8_t) * 8,	// strideInBytes (uint32_t)
				0																// instancesPerElement (uint32_t)
			}
		};


//[-------------------------------------------------------]
//[ Anonymous detail namespace                            ]
//[-------------------------------------------------------]
	} // detail
}


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
namespace RendererRuntime
{


	//[-------------------------------------------------------]
	//[ Public definitions                                    ]
	//[-------------------------------------------------------]
	const Renderer::VertexAttributes MeshResource::VERTEX_ATTRIBUTES(static_cast<uint32_t>(glm::countof(::detail::StaticVertexAttributesLayout)), ::detail::StaticVertexAttributesLayout);
	const Renderer::VertexAttributes MeshResource::SKINNED_VERTEX_ATTRIBUTES(static_cast<uint32_t>(glm::countof(::detail::SkinnedVertexAttributesLayout)), ::detail::SkinnedVertexAttributesLayout);


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
} // RendererRuntime
