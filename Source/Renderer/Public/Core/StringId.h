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
#ifdef _WIN32
	// Disable warnings in external headers, we can't fix them
	__pragma(warning(push))
		__pragma(warning(disable: 4574)) // warning C4574: '_HAS_ITERATOR_DEBUGGING' is defined to be '0': did you mean to use '#if _HAS_ITERATOR_DEBUGGING'?
		__pragma(warning(disable: 4668)) // warning C4668: '_M_HYBRID_X86_ARM64' is not defined as a preprocessor macro, replacing with '0' for '#if/#elif'
		#ifdef RHI_DEBUG
			#include <cassert>
			#define ASSERT(expression, message) assert((expression) && message);	// TODO(co) "RHI_ASSERT()" should be used everywhere
		#else
			#define ASSERT(expression, message)	// TODO(co) "RHI_ASSERT()" should be used everywhere
		#endif
		#include <inttypes.h>	// For uint32_t, uint64_t etc.
		#include <type_traits>	// For "std::integral_constant"
	__pragma(warning(pop))
#else
	#ifdef RHI_DEBUG
		#include <cassert>
		#define ASSERT(expression, message) assert((expression) && message);	// TODO(co) "RHI_ASSERT()" should be used everywhere
	#else
		#define ASSERT(expression, message)	// TODO(co) "RHI_ASSERT()" should be used everywhere
	#endif
	#include <inttypes.h>	// For uint32_t, uint64_t etc.
	#include <type_traits>	// For "std::integral_constant"
#endif


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
namespace Renderer
{


	//[-------------------------------------------------------]
	//[ Classes                                               ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Compile time string ID class generating an identifier by hashing a given string
	*
	*  @remarks
	*    The following example shows how to use the string ID class:
	*    @code
	*    uint32_t id = STRING_ID("Example/Mesh/Default/Squirrel");	// Result will be 2906231359
	*    @endcode
	*
	*  @note
	*    - Is using compile-time string hashing as described within the Gamasutra article "In-Depth: Quasi Compile-Time String Hashing"
	*      by Stefan Reinalter ( http://www.gamasutra.com/view/news/38198/InDepth_Quasi_CompileTime_String_Hashing.php#.UG1MpVFQbq4 )
	*    - As hash function FNV-1a is used ( http://isthe.com/chongo/tech/comp/fnv/ )
	*/
	class StringId final
	{


	//[-------------------------------------------------------]
	//[ Public definitions                                    ]
	//[-------------------------------------------------------]
	public:
		static constexpr uint32_t FNV1a_INITIAL_HASH_32  = 0xcbf29ce4u;
		static constexpr uint32_t FNV1a_MAGIC_PRIME_32	 = 0x1000193u;
		static constexpr uint32_t MAXIMUM_UINT32_T_VALUE = 4294967295u;	///< We don't want to include "<limits>" in this lightweight core header just to be able to use "std::numeric_limits<uint32_t>::max()"

		/**
		*  @brief
		*    "const char*"-wrapper enabling the compiler to distinguish between static and dynamic strings
		*/
		struct ConstCharWrapper final
		{
			/**
			*  @brief
			*    "const char*"-wrapper constructor
			*
			*  @param[in] string
			*    Null terminated ASCII string to calculate the hash value for, must be valid
			*
			*  @note
			*    - Not explicit by intent
			*/
			inline ConstCharWrapper(const char* string) :
				mString(string)
			{
				// Nothing here
			}

			const char* mString;	///< Null terminated ASCII string to calculate the hash value for, must be valid
		};


	//[-------------------------------------------------------]
	//[ Public static methods                                 ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Calculate the hash value of the given string at compile time
		*
		*  @param[in] string
		*    Null terminated ASCII string to calculate the hash value for, must be valid
		*  @param[in] value
		*    Initial hash
		*
		*  @return
		*    The hash value of the given string
		*/
		[[nodiscard]] static constexpr inline uint32_t compileTimeFNV(const char* string, const uint32_t value = FNV1a_INITIAL_HASH_32) noexcept
		{
			// 32-bit FNV-1a implementation basing on http://www.isthe.com/chongo/tech/comp/fnv/
			return ('\0' == string[0]) ? value : compileTimeFNV(&string[1], (value ^ static_cast<uint32_t>(string[0])) * FNV1a_MAGIC_PRIME_32);
		}

		/**
		*  @brief
		*    Calculate the hash value of the given string
		*
		*  @param[in] string
		*    Null terminated ASCII string to calculate the hash value for, must be valid
		*
		*  @return
		*    The hash value of the given string
		*/
		[[nodiscard]] static inline uint32_t calculateFNV(const char* string)
		{
			// Sanity check
			ASSERT(nullptr != string, "The string must be valid to be able to calculate a FNV1a32 hash")

			// 32-bit FNV-1a implementation basing on http://www.isthe.com/chongo/tech/comp/fnv/
			uint32_t hash = FNV1a_INITIAL_HASH_32;
			for ( ; '\0' != *string; ++string)
			{
				hash = (hash ^ *string) * FNV1a_MAGIC_PRIME_32;
			}
			return hash;
		}


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Default constructor
		*/
		inline StringId() :
			mId(MAXIMUM_UINT32_T_VALUE)
		{
			// Nothing here
		}

		/**
		*  @brief
		*    Constructor for calculating the hash value of a given static string
		*
		*  @param[in] string
		*    Static string to calculate the hash value for, must be valid
		*/
		template <uint32_t N>
		inline StringId(const char (&string)[N]) noexcept :
			mId(compileTimeFNV(string))
		{
			// It's a trap!
			#ifdef _WIN32
				static_assert(false, "Use the \"STRING_ID()\" macro to mark compile string IDs");
			#endif
		}

		/**
		*  @brief
		*    Constructor for calculating the hash value of a given dynamic string
		*
		*  @param[in] string
		*    Dynamic string to calculate the hash value for, must be valid
		*/
		inline explicit StringId(const ConstCharWrapper& string) :
			mId(calculateFNV(string.mString))
		{
			// Nothing here
		}

		/**
		*  @brief
		*    Constructor for directly setting an uint32_t value as string ID
		*
		*  @param[in] id
		*    ID value to set
		*
		*  @note
		*    - By intent not explicit for best possible usability
		*/
		inline StringId(uint32_t id) :
			mId(id)
		{
			// Nothing do to in here
		}

		/**
		*  @brief
		*    Copy constructor
		*
		*  @param[in] stringId
		*    String ID instance to copy from
		*
		*  @remarks
		*    This constructor is by intent not explicit because otherwise GCC 4.9 throws an error when "Renderer::StringId" is used
		*    e.g. as a function parameter and the function is called with a string literal. Example:
		*    "
		*      typedef StringId AssetId;
		*      void function(StringId assetId) {}
		*      void functionAssetId(AssetId assetId) {}
		*    "
		*    Function call results:
		*    - function("BlaBlub"); <-- GCC 4.9 error: no matching function for call to "Renderer::StringId::StringId(Renderer::StringId)"
		*    - functionAssetId("BlaBlub"); <-- GCC 4.9 error: no matching function for call to "Renderer::StringId::StringId(AssetId)"
		*/
		inline StringId(const StringId& stringId) :
			mId(stringId.mId)
		{
			// Nothing here
		}

		/**
		*  @brief
		*    Return the generated ID
		*
		*  @return
		*    The generated FNV-1a hash value used as identifier
		*/
		[[nodiscard]] inline uint32_t getId() const
		{
			return mId;
		}

		/**
		*  @brief
		*    Return the generated ID
		*
		*  @return
		*    The generated FNV-1a hash value used as identifier
		*/
		[[nodiscard]] inline operator uint32_t() const
		{
			return mId;
		}


	//[-------------------------------------------------------]
	//[ Private data                                          ]
	//[-------------------------------------------------------]
	private:
		uint32_t mId;	///< The generated FNV-1a hash value which is used as identifier


	};


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
} // Renderer


//[-------------------------------------------------------]
//[ Macros & definitions                                  ]
//[-------------------------------------------------------]
#ifdef _MSC_VER
	// Disable warnings in external headers, we can't fix them: warning C4307: '*': integral constant overflow
	/**
	*  @brief
	*    Compile time string ID macro
	*/
	#define STRING_ID(string) \
		__pragma(warning(push)) \
			__pragma(warning(disable:4307)) \
			std::integral_constant<uint32_t, Renderer::StringId::compileTimeFNV(string)>::value \
		__pragma(warning(pop))
#else
	/**
	*  @brief
	*    Compile time string ID macro
	*/
	#define STRING_ID(string) std::integral_constant<uint32_t, Renderer::StringId::compileTimeFNV(string)>::value
#endif

/**
*  @brief
*    Compile time asset ID macro; use this alias instead of "STRING_ID()" to be able to easily search for asset references
*/
#define ASSET_ID(string) STRING_ID(string)
