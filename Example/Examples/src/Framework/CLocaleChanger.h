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
#include "Framework/PlatformTypes.h"

// Disable warnings in external headers, we can't fix them
PRAGMA_WARNING_PUSH
	PRAGMA_WARNING_DISABLE_MSVC(4365)	// warning C4365: 'argument': conversion from 'long' to 'unsigned int', signed/unsigned mismatch
	PRAGMA_WARNING_DISABLE_MSVC(5026)	// warning C5026: 'std::_Generic_error_category': move constructor was implicitly defined as deleted
	PRAGMA_WARNING_DISABLE_MSVC(5027)	// warning C5027: 'std::_Generic_error_category': move assignment operator was implicitly defined as deleted
	PRAGMA_WARNING_DISABLE_MSVC(4571)	// warning C4571: Informational: catch(...) semantics changed since Visual C++ 7.1; structured exceptions (SEH) are no longer caught
	PRAGMA_WARNING_DISABLE_MSVC(4625)	// warning C4625: 'std::codecvt_base': copy constructor was implicitly defined as deleted
	PRAGMA_WARNING_DISABLE_MSVC(4626)	// warning C4626: 'std::codecvt<char16_t,char,_Mbstatet>': assignment operator was implicitly defined as deleted
	PRAGMA_WARNING_DISABLE_MSVC(4774)	// warning C4774: 'sprintf_s' : format string expected in argument 3 is not a string literal
	#include <string>
PRAGMA_WARNING_POP
#include <clocale>


//[-------------------------------------------------------]
//[ Classes                                               ]
//[-------------------------------------------------------]
/**
*  @brief
*    Changes the current locale to 'C' locale temporarily using RAII (Resource Acquisition Is Initialization)
*
* @remarks
*    When this instance gets destroyed the old locale setting is restored
*    => Not possible to forget the restore anymore ;)
*/
class CLocaleChanger final
{


//[-------------------------------------------------------]
//[ Public functions                                      ]
//[-------------------------------------------------------]
public:
	/**
	*  @brief
	*    Constructor
	*/
	CLocaleChanger()
	{
		// Get the currently set locale, if it's a null pointer or already "C" just do nothing
		const char* currentLocale = setlocale(LC_ALL, nullptr);
		if (currentLocale && 'C' != currentLocale[0])
		{
			// Duplicate the string
			m_savedLocale = currentLocale;

			// Set the locale back to the default
			setlocale(LC_ALL, "C");
		}
	}

	/**
	*  @brief
	*    Destructor
	*/
	~CLocaleChanger()
	{
		if (m_savedLocale.length() > 0)
		{
			// Be polite and restore the previously set locale
			setlocale(LC_ALL, m_savedLocale.c_str());
		}
	}


//[-------------------------------------------------------]
//[ Private functions                                     ]
//[-------------------------------------------------------]
private:
	explicit CLocaleChanger(const CLocaleChanger&) = delete;
	CLocaleChanger& operator =(const CLocaleChanger&) = delete;


//[-------------------------------------------------------]
//[ Private data                                          ]
//[-------------------------------------------------------]
private:
	std::string m_savedLocale;


};
