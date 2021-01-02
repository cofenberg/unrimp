/*********************************************************\
 * Copyright (c) 2012-2021 The Unrimp Team
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
//[ Header guard                                          ]
//[-------------------------------------------------------]
#pragma once


//[-------------------------------------------------------]
//[ Includes                                              ]
//[-------------------------------------------------------]
#include "Renderer/Public/Resource/IResource.h"
#include "Renderer/Public/Resource/ShaderBlueprint/Cache/ShaderProperties.h"

// Disable warnings in external headers, we can't fix them
PRAGMA_WARNING_PUSH
	PRAGMA_WARNING_DISABLE_MSVC(4365)	// warning C4365: 'argument': conversion from 'long' to 'unsigned int', signed/unsigned mismatch
	PRAGMA_WARNING_DISABLE_MSVC(5026)	// warning C5026: 'std::_Generic_error_category': move constructor was implicitly defined as deleted
	PRAGMA_WARNING_DISABLE_MSVC(5027)	// warning C5027: 'std::_Generic_error_category': move assignment operator was implicitly defined as deleted
	PRAGMA_WARNING_DISABLE_MSVC(4571)	// warning C4571: Informational: catch(...) semantics changed since Visual C++ 7.1; structured exceptions (SEH) are no longer caught
	PRAGMA_WARNING_DISABLE_MSVC(4625)	// warning C4625: 'std::codecvt_base': copy constructor was implicitly defined as deleted
	PRAGMA_WARNING_DISABLE_MSVC(4626)	// warning C4626: 'std::codecvt<char16_t,char,_Mbstatet>': assignment operator was implicitly defined as deleted
	PRAGMA_WARNING_DISABLE_MSVC(4668)	// warning C4668: '_M_HYBRID_X86_ARM64' is not defined as a preprocessor macro, replacing with '0' for '#if/#elif'
	PRAGMA_WARNING_DISABLE_MSVC(4774)	// warning C4774: 'sprintf_s' : format string expected in argument 3 is not a string literal
	#include <string>
PRAGMA_WARNING_POP


//[-------------------------------------------------------]
//[ Forward declarations                                  ]
//[-------------------------------------------------------]
namespace Renderer
{
	template <class ELEMENT_TYPE, typename ID_TYPE, uint32_t MAXIMUM_NUMBER_OF_ELEMENTS> class PackedElementManager;
	template <class TYPE, class LOADER_TYPE, typename ID_TYPE, uint32_t MAXIMUM_NUMBER_OF_ELEMENTS> class ResourceManagerTemplate;
}


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
namespace Renderer
{


	//[-------------------------------------------------------]
	//[ Global definitions                                    ]
	//[-------------------------------------------------------]
	typedef uint32_t ShaderPieceResourceId;		///< POD shader piece resource identifier
	typedef uint32_t ShaderBlueprintResourceId;	///< POD shader blueprint resource identifier


	//[-------------------------------------------------------]
	//[ Classes                                               ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Shader blueprint resource
	*/
	class ShaderBlueprintResource final : public IResource
	{


	//[-------------------------------------------------------]
	//[ Friends                                               ]
	//[-------------------------------------------------------]
		friend class ShaderBlueprintResourceLoader;
		friend PackedElementManager<ShaderBlueprintResource, ShaderBlueprintResourceId, 64>;									// Type definition of template class
		friend ResourceManagerTemplate<ShaderBlueprintResource, ShaderBlueprintResourceLoader, ShaderBlueprintResourceId, 64>;	// Type definition of template class


	//[-------------------------------------------------------]
	//[ Public definitions                                    ]
	//[-------------------------------------------------------]
	public:
		typedef std::vector<ShaderPieceResourceId> IncludeShaderPieceResourceIds;


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Return the IDs of the shader piece resources to include
		*
		*  @return
		*    The IDs of the shader piece resources to include
		*/
		[[nodiscard]] inline const IncludeShaderPieceResourceIds& getIncludeShaderPieceResourceIds() const
		{
			return mIncludeShaderPieceResourceIds;
		}

		/**
		*  @brief
		*    Return the referenced shader properties
		*
		*  @return
		*    The referenced shader properties
		*/
		[[nodiscard]] inline const ShaderProperties& getReferencedShaderProperties() const
		{
			return mReferencedShaderProperties;
		}

		/**
		*  @brief
		*    Return the shader source code
		*
		*  @return
		*    The shader ASCII source code
		*/
		[[nodiscard]] inline const std::string& getShaderSourceCode() const
		{
			return mShaderSourceCode;
		}


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		inline ShaderBlueprintResource()
		{
			// Nothing here
		}

		inline virtual ~ShaderBlueprintResource() override
		{
			// Sanity checks
			ASSERT(mIncludeShaderPieceResourceIds.empty(), "Invalid include shader piece resource IDs")
			ASSERT(mReferencedShaderProperties.getSortedPropertyVector().empty(), "Invalid referenced shader properties")
			ASSERT(mShaderSourceCode.empty(), "Invalid shader source code")
		}

		explicit ShaderBlueprintResource(const ShaderBlueprintResource&) = delete;
		ShaderBlueprintResource& operator=(const ShaderBlueprintResource&) = delete;

		//[-------------------------------------------------------]
		//[ "Renderer::PackedElementManager" management           ]
		//[-------------------------------------------------------]
		inline void initializeElement(ShaderBlueprintResourceId shaderBlueprintResourceId)
		{
			// Sanity checks
			ASSERT(mIncludeShaderPieceResourceIds.empty(), "Invalid include shader piece resource IDs")
			ASSERT(mReferencedShaderProperties.getSortedPropertyVector().empty(), "Invalid referenced shader properties")
			ASSERT(mShaderSourceCode.empty(), "Invalid shader source code")

			// Call base implementation
			IResource::initializeElement(shaderBlueprintResourceId);
		}

		inline void deinitializeElement()
		{
			// Reset everything
			mIncludeShaderPieceResourceIds.clear();
			mReferencedShaderProperties.clear();
			mShaderSourceCode.clear();

			// Call base implementation
			IResource::deinitializeElement();
		}


	//[-------------------------------------------------------]
	//[ Private data                                          ]
	//[-------------------------------------------------------]
	private:
		IncludeShaderPieceResourceIds mIncludeShaderPieceResourceIds;
		ShaderProperties			  mReferencedShaderProperties;	// Directly use "Renderer::ShaderProperties" to keep things simple, although we don't need a shader property value
		std::string					  mShaderSourceCode;


	};


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
} // Renderer
