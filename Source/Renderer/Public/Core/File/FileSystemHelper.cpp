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
//[ Includes                                              ]
//[-------------------------------------------------------]
#include "Renderer/Public/Core/File/FileSystemHelper.h"


//[-------------------------------------------------------]
//[ Anonymous detail namespace                            ]
//[-------------------------------------------------------]
namespace
{
	namespace detail
	{


		//[-------------------------------------------------------]
		//[ Global functions                                      ]
		//[-------------------------------------------------------]
		#ifdef LINUX
			// Copied from boost: https://github.com/boostorg/filesystem/blob/a682eaa476cf0b4e992884d32dd2ddcfb0b6b1aa/src/path.cpp
			// Implement lexically_normal used when std::filesystem::path implementation of the c++ runtime doesn't provide this as member method
			// std::filesystem::path::lexically_normal is part of the filesystem TS which is part of C++17
			[[nodiscard]] const std_filesystem::path&  dot_path()
			{
				#ifdef _WIN32
					static const std_filesystem::path dot_pth(L".");
				#else
					static const std_filesystem::path dot_pth(".");
				#endif
				return dot_pth;
			}
		
			#ifdef _WIN32
				const std_filesystem::path::value_type      separator = L'/';
				const std_filesystem::path::value_type      preferred_separator = L'\\';
				const std_filesystem::path::value_type      dot = L'.';
				const std_filesystem::path::value_type      colon = L':';
			#else 
				const std_filesystem::path::value_type      separator = '/';
				const std_filesystem::path::value_type      preferred_separator = '/';
				const std_filesystem::path::value_type      dot = '.';
			#endif
		
			[[nodiscard]] std_filesystem::path lexically_normal(const std_filesystem::path& path)
			{
				if (path.empty())
					return path;

				std_filesystem::path temp;
				std_filesystem::path::iterator start(path.begin());
				std_filesystem::path::iterator last(path.end());
				std_filesystem::path::iterator stop(last--);
				for (std_filesystem::path::iterator itr(start); itr != stop; ++itr)
				{
					// ignore "." except at start and last
					if (itr->native().size() == 1
						&& (itr->native())[0] == detail::dot
						&& itr != start
						&& itr != last) continue;

					// ignore a name and following ".."
					if (!temp.empty()
						&& itr->native().size() == 2
						&& (itr->native())[0] == detail::dot
						&& (itr->native())[1] == detail::dot) // dot dot
					{
						std_filesystem::path::string_type lf(temp.filename().native());  
						if (lf.size() > 0  
						&& (lf.size() != 1
							|| (lf[0] != detail::dot
							&& lf[0] != detail::separator))
						&& (lf.size() != 2 
							|| (lf[0] != detail::dot
							&& lf[1] != detail::dot
						#ifdef _WIN32
							&& lf[1] != ::detail::colon
						#endif
							)
							)
						)
						{
							temp.remove_filename();
							//// if not root directory, must also remove "/" if any
							//if (temp.native().size() > 0
							//  && temp.native()[temp.native().size()-1]
							//    == separator)
							//{
							//  string_type::size_type rds(
							//    root_directory_start(temp.native(), temp.native().size()));
							//  if (rds == string_type::npos
							//    || rds != temp.native().size()-1) 
							//  {
							//    temp.m_pathname.erase(temp.native().size()-1);
							//  }
							//}

							std_filesystem::path::iterator next(itr);
							if (temp.empty() && ++next != stop
							&& next == last && *last == detail::dot_path())
							{
								temp /= detail::dot_path();
							}
							continue;
						}
					}

					temp /= *itr;
				};

				if (temp.empty())
					temp /= detail::dot_path();
				return temp;
			}
		#endif


//[-------------------------------------------------------]
//[ Anonymous detail namespace                            ]
//[-------------------------------------------------------]
	} // detail
}


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
namespace Renderer
{


	//[-------------------------------------------------------]
	//[ Public static methods                                 ]
	//[-------------------------------------------------------]
	std_filesystem::path FileSystemHelper::lexicallyNormal(const std_filesystem::path& path)
	{
		#ifdef LINUX
			return ::detail::lexically_normal(path);
		#else
			return path.lexically_normal();
		#endif
	}


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
} // Renderer
