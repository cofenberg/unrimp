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
#include "RendererRuntime/PrecompiledHeader.h"
#include "RendererRuntime/Resource/ShaderBlueprint/Cache/Preprocessor/Preprocessor.h"
#define MOJOSHADER_NO_VERSION_INCLUDE
#include "RendererRuntime/Resource/ShaderBlueprint/Cache/Preprocessor/mojoshader.h"
#include "RendererRuntime/IRendererRuntime.h"
#include "RendererRuntime/Context.h"

#include <cstring>	// For "strlen()"


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
namespace RendererRuntime
{


	//[-------------------------------------------------------]
	//[ Public static methods                                 ]
	//[-------------------------------------------------------]
	void Preprocessor::preprocess(const IRendererRuntime& rendererRuntime, std::string& source, std::string& result)
	{
		// TODO(co) The usage of MojoShader just as preprocessor is overkill. Find a simpler but still efficient solution. Switch to "mcpp -- a portable C preprocessor" ( http://mcpp.sourceforge.net/ ) ?

		// Disable warnings in external headers, we can't fix them
		PRAGMA_WARNING_PUSH
			PRAGMA_WARNING_DISABLE_MSVC(5039)	// warning C5039: 'MOJOSHADER_preprocess': pointer or reference to potentially throwing function passed to extern C function under -EHc. Undefined behavior may occur if this function throws an exception.

			// Preprocess
			const MOJOSHADER_preprocessData* preprocessData = MOJOSHADER_preprocess(nullptr, source.c_str(), static_cast<unsigned int>(source.length()), 0, 0, 0, 0, 0, 0, 0, 0, 0);
		PRAGMA_WARNING_POP

		// Evaluate the result
		if (preprocessData->error_count > 0)
		{
			for (int i = 0; i < preprocessData->error_count; ++i)
			{
				RENDERER_LOG(rendererRuntime.getContext(), CRITICAL, "Renderer runtime shader preprocessor %s:%d: Error: %s", preprocessData->errors[i].filename ? preprocessData->errors[i].filename : "???", preprocessData->errors[i].error_position, preprocessData->errors[i].error)
			}
		}
		else
		{
			result.assign(preprocessData->output, static_cast<size_t>(preprocessData->output_len));
		}
		MOJOSHADER_freePreprocessData(preprocessData);
	}


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
} // RendererRuntime
