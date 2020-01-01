/*********************************************************\
 * Copyright (c) 2012-2020 The Unrimp Team
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
#include "RendererToolkit/Private/Helper/AssimpLogStream.h"

#include <stdexcept>


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
namespace RendererToolkit
{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	AssimpLogStream::AssimpLogStream()
	{
		Assimp::DefaultLogger::create("", Assimp::Logger::NORMAL, aiDefaultLogStream_DEBUGGER);
		Assimp::DefaultLogger::get()->attachStream(this, Assimp::DefaultLogger::Err);
	}

	AssimpLogStream::~AssimpLogStream()
	{
		Assimp::DefaultLogger::get()->detatchStream(this, Assimp::DefaultLogger::Err);
		Assimp::DefaultLogger::kill();
	}


	//[-------------------------------------------------------]
	//[ Public virtual Assimp::LogStream methods              ]
	//[-------------------------------------------------------]
	void AssimpLogStream::write(const char* message)
	{
		// Ignore the following errors
		// -> "Failed to compute tangents; need UV data in channel0" since some sub-meshes might have no texture coordinates, worth a hint but no error
		// -> "OBJ: unexpected illumination model (0-2 recognized)" since the illumination model information is unused anyway
		// -> "Skipping one or more lines with the same contents" since the default setting of Assimp is to not repeat error messages but to emit such a message instead
		if (nullptr == strstr(message, "Failed to compute tangents; need UV data in channel0") &&
			nullptr == strstr(message, "OBJ: unexpected illumination model (0-2 recognized)") &&
			nullptr == strstr(message, "Skipping one or more lines with the same contents") &&
			nullptr == strstr(message, "FindInvalidDataProcess fails on mesh uvcoords: All vectors are identical") &&
			nullptr == strstr(message, "FindInvalidDataProcess fails on mesh normals: Found zero-length vector") &&
			nullptr == strstr(message, "This algorithm works on triangle meshes only"))
		{
			mLastErrorMessage = message;
			throw std::runtime_error(message);
		}
	}


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
} // RendererToolkit
