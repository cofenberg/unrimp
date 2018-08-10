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
//[ Header guard                                          ]
//[-------------------------------------------------------]
#pragma once


//[-------------------------------------------------------]
//[ Includes                                              ]
//[-------------------------------------------------------]
#include "RendererRuntime/Public/Core/GetInvalid.h"


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
namespace RendererRuntime
{


	//[-------------------------------------------------------]
	//[ Global definitions                                    ]
	//[-------------------------------------------------------]
	typedef uint32_t MaterialResourceId;	///< POD material resource identifier


	//[-------------------------------------------------------]
	//[ Classes                                               ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Sub-mesh class
	*/
	class SubMesh final
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		inline SubMesh() :
			mMaterialResourceId(getInvalid<MaterialResourceId>()),
			mStartIndexLocation(0),
			mNumberOfIndices(0)
		{
			// Nothing here
		}

		inline SubMesh(MaterialResourceId materialResourceId, uint32_t startIndexLocation, uint32_t numberOfIndices) :
			mMaterialResourceId(materialResourceId),
			mStartIndexLocation(startIndexLocation),
			mNumberOfIndices(numberOfIndices)
		{
			// Nothing here
		}

		inline SubMesh(const SubMesh& subMesh) :
			mMaterialResourceId(subMesh.mMaterialResourceId),
			mStartIndexLocation(subMesh.mStartIndexLocation),
			mNumberOfIndices(subMesh.mNumberOfIndices)
		{
			// Nothing here
		}

		inline ~SubMesh()
		{
			// Nothing here
		}

		inline SubMesh& operator=(const SubMesh& subMesh)
		{
			mMaterialResourceId	= subMesh.mMaterialResourceId;
			mStartIndexLocation = subMesh.mStartIndexLocation;
			mNumberOfIndices	= subMesh.mNumberOfIndices;

			// Done
			return *this;
		}

		inline MaterialResourceId getMaterialResourceId() const
		{
			return mMaterialResourceId;
		}

		inline void setMaterialResourceId(MaterialResourceId materialResourceId)
		{
			mMaterialResourceId = materialResourceId;
		}

		inline uint32_t getStartIndexLocation() const
		{
			return mStartIndexLocation;
		}

		inline void setStartIndexLocation(uint32_t startIndexLocation)
		{
			mStartIndexLocation = startIndexLocation;
		}

		inline uint32_t getNumberOfIndices() const
		{
			return mNumberOfIndices;
		}

		inline void setNumberOfIndices(uint32_t numberOfIndices)
		{
			mNumberOfIndices = numberOfIndices;
		}


	//[-------------------------------------------------------]
	//[ Private data                                          ]
	//[-------------------------------------------------------]
	private:
		MaterialResourceId mMaterialResourceId;	///< Material resource ID, can be set to invalid value
		uint32_t		   mStartIndexLocation;
		uint32_t		   mNumberOfIndices;


	};


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
} // RendererRuntime
