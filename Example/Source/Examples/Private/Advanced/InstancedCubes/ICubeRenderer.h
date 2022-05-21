/*********************************************************\
 * Copyright (c) 2012-2022 The Unrimp Team
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
#include "Examples/Private/Framework/PlatformTypes.h"


//[-------------------------------------------------------]
//[ Forward declarations                                  ]
//[-------------------------------------------------------]
namespace Rhi
{
	class CommandBuffer;
}


//[-------------------------------------------------------]
//[ Classes                                               ]
//[-------------------------------------------------------]
/**
*  @brief
*    Abstract cube renderer interface
*/
class ICubeRenderer
{


//[-------------------------------------------------------]
//[ Public methods                                        ]
//[-------------------------------------------------------]
public:
	/**
	*  @brief
	*    Destructor
	*/
	inline virtual ~ICubeRenderer()
	{
		// Nothing here
	}


//[-------------------------------------------------------]
//[ Public virtual ICubeRenderer methods                  ]
//[-------------------------------------------------------]
public:
	/**
	*  @brief
	*    Set the number of cubes
	*
	*  @param[in] numberOfCubes
	*    Number of cubes
	*/
	virtual void setNumberOfCubes(uint32_t numberOfCubes) = 0;

	/**
	*  @brief
	*    Draw the cubes by filling a given command buffer
	*
	*  @param[in] globalTimer
	*    Global timer
	*  @param[in] globalScale
	*    Global scale
	*  @param[in] lightPositionX
	*    X component of the light position
	*  @param[in] lightPositionY
	*    Y component of the light position
	*  @param[in] lightPositionZ
	*    Z component of the light position
	*  @param[out] commandBuffer
	*    RHI command buffer to fill
	*/
	virtual void fillCommandBuffer(float globalTimer, float globalScale, float lightPositionX, float lightPositionY, float lightPositionZ, Rhi::CommandBuffer& commandBuffer) = 0;


//[-------------------------------------------------------]
//[ Protected methods                                     ]
//[-------------------------------------------------------]
protected:
	/**
	*  @brief
	*    Default constructor
	*/
	inline ICubeRenderer()
	{
		// Nothing here
	}

	explicit ICubeRenderer(const ICubeRenderer& source) = delete;
	ICubeRenderer& operator =(const ICubeRenderer& source) = delete;


//[-------------------------------------------------------]
//[ Protected static data                                 ]
//[-------------------------------------------------------]
protected:
	static constexpr uint32_t MAXIMUM_NUMBER_OF_TEXTURES = 8;	///< Maximum number of textures


};
