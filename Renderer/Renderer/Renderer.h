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


/**
*  @brief
*    Amalgamated/unity build renderer interfaces
*
*  @remarks
*    == Description ==
*    Please note that this is a 100% interface project resulting in no binary at all. The one and only goal of this project is to offer unified renderer interfaces for multiple graphics APIs like OpenGL or Direct3D. Features like resource loading, font rendering or even rendering of complex scenes is out of the scope of this project.
*
*    In order to make it easier to use the renderer, the optional comfort header "Renderer.h" puts everything together within a single header without any additional header inclusion. When writing interfaces for other languages like C# or Pascal, you might want to port this header instead of the more complex individual headers.
*
*    == Dependencies ==
*    None.
*
*    == Preprocessor Definitions ==
*    - Set "_WIN32" as preprocessor definition when building for Microsoft Windows
*    - Set "LINUX" as preprocessor definition when building for Linux or similar platforms
*        - For Linux or similar platforms: Set "HAVE_VISIBILITY_ATTR" as preprocessor definition to use the visibility attribute (the used compiler must support it)
*    - Set "__ANDROID__" as preprocessor definition when building for Android
*    - Set "X64_ARCHITECTURE" as preprocessor definition when building for x64 instead of x86
*    - Set "RENDERER_STATISTICS" as preprocessor definition in order to enable the gathering of statistics (tiny binary size and tiny negative performance impact)
*    - Set "RENDERER_DEBUG" as preprocessor definition in order to enable e.g. Direct3D 9 PIX functions (D3DPERF_* functions, also works directly within VisualStudio 2012 out-of-the-box) debug features (disabling support just reduces the binary size slightly but makes debugging more difficult)
*/


//[-------------------------------------------------------]
//[ Header guard                                          ]
//[-------------------------------------------------------]
#pragma once


//[-------------------------------------------------------]
//[ C++ compiler keywords                                 ]
//[-------------------------------------------------------]
#ifdef _WIN32
	#ifndef NULL_HANDLE
		#define NULL_HANDLE 0
	#endif

	// To export functions
	#define GENERIC_FUNCTION_EXPORT extern "C" __declspec(dllexport)

	/**
	*  @brief
	*    Force the compiler to inline something
	*
	*  @note
	*    - Do only use this when you really have to, usually it's best to let the compiler decide by using the standard keyword "inline"
	*/
	#define FORCEINLINE __forceinline

	/**
	*  @brief
	*    Restrict alias
	*/
	#define RESTRICT __restrict
	#define RESTRICT_RETURN __restrict

	/**
	*  @brief
	*    No operation macro ("_asm nop"/"__nop()")
	*/
	#define NOP __nop()

	/**
	*  @brief
	*    C++17 attribute specifier sequence "[[maybe_unused]]"
	*/
	#define MAYBE_UNUSED [[maybe_unused]]

	/**
	*  @brief
	*    Debug break operation macro
	*/
	#define DEBUG_BREAK __debugbreak()

	/**
	*  @brief
	*    Platform specific "#pragma warning(push)" (MS Windows Visual Studio)
	*/
	#define PRAGMA_WARNING_PUSH __pragma(warning(push))

	/**
	*  @brief
	*    Platform specific "#pragma warning(pop)" (MS Windows Visual Studio)
	*/
	#define PRAGMA_WARNING_POP __pragma(warning(pop))

	/**
	*  @brief
	*    Platform specific "#pragma warning(disable: <x>)" (MS Windows Visual Studio)
	*/
	#define PRAGMA_WARNING_DISABLE_MSVC(id) __pragma(warning(disable: id))

	/**
	*  @brief
	*    Platform specific "#pragma clang diagnostic ignored <x>" (Clang)
	*/
	#define PRAGMA_WARNING_DISABLE_CLANG(id)

	/**
	*  @brief
	*    Platform specific "#pragma GCC diagnostic ignored <x>" (GCC)
	*/
	#define PRAGMA_WARNING_DISABLE_GCC(id)
#elif LINUX
	#ifndef NULL_HANDLE
		#define NULL_HANDLE 0
	#endif

	// To export functions
	#if defined(HAVE_VISIBILITY_ATTR)
		#define GENERIC_FUNCTION_EXPORT extern "C" __attribute__ ((visibility("default")))
	#else
		#define GENERIC_FUNCTION_EXPORT extern "C"
	#endif

	/**
	*  @brief
	*    Force the compiler to inline something
	*
	*  @note
	*    - Do only use this when you really have to, usually it's best to let the compiler decide by using the standard keyword "inline"
	*/
	#define FORCEINLINE __attribute__((always_inline))

	/**
	*  @brief
	*    Restrict alias
	*/
	#define RESTRICT __restrict__
	#define RESTRICT_RETURN

	/**
	*  @brief
	*    No operation macro ("_asm nop"/__nop())
	*/
	#define NOP asm ("nop");

	/**
	*  @brief
	*    C++17 attribute specifier sequence "[[maybe_unused]]"
	*/
	#define MAYBE_UNUSED

	/**
	*  @brief
	*    Debug break operation macro
	*/
	#define DEBUG_BREAK __builtin_trap()

	#ifdef __clang__
		/**
		*  @brief
		*    Platform specific "#pragma clang diagnostic push" (Clang)
		*/
		#define PRAGMA_WARNING_PUSH _Pragma("clang diagnostic push")

		/**
		*  @brief
		*    Platform specific "#pragma clang diagnostic pop" (Clang)
		*/
		#define PRAGMA_WARNING_POP _Pragma("clang diagnostic pop")

		/**
		*  @brief
		*    Platform specific "#pragma warning(disable: <x>)" (MS Windows Visual Studio)
		*/
		#define PRAGMA_WARNING_DISABLE_MSVC(id)

		/**
		*  @brief
		*    Platform specific "#pragma GCC diagnostic ignored <x>" (GCC)
		*/
		#define PRAGMA_WARNING_DISABLE_GCC(id)

		/**
		*  @brief
		*    Platform specific "#pragma clang diagnostic ignored <x>" (Clang)
		*/
		// We need stringify because _Pragma expects an string literal
		#define PRAGMA_STRINGIFY(a) #a
		#define PRAGMA_WARNING_DISABLE_CLANG(id) _Pragma(PRAGMA_STRINGIFY(clang diagnostic ignored id) )
	#elif __GNUC__
		// gcc
		/**
		*  @brief
		*    Platform specific "#pragma GCC diagnostic push" (GCC)
		*/
		#define PRAGMA_WARNING_PUSH _Pragma("GCC diagnostic push")

		/**
		*  @brief
		*    Platform specific "#pragma warning(pop)" (GCC)
		*/
		#define PRAGMA_WARNING_POP _Pragma("GCC diagnostic pop")

		/**
		*  @brief
		*    Platform specific "#pragma warning(disable: <x>)" (MS Windows Visual Studio)
		*/
		#define PRAGMA_WARNING_DISABLE_MSVC(id)

		/**
		*  @brief
		*    Platform specific "#pragma GCC diagnostic ignored <x>" (GCC)
		*/
		// We need stringify because _Pragma expects an string literal
		#define PRAGMA_STRINGIFY(a) #a
		#define PRAGMA_WARNING_DISABLE_GCC(id) _Pragma(PRAGMA_STRINGIFY(GCC diagnostic ignored id) )

		/**
		*  @brief
		*    Platform specific "#pragma clang diagnostic ignored <x>" (Clang)
		*/
		#define PRAGMA_WARNING_DISABLE_CLANG(id)
	#else
		#error "Unsupported compiler"
	#endif

	#define stricmp(a, b)	strcasecmp(a, b)
#else
	#error "Unsupported platform"
#endif




//[-------------------------------------------------------]
//[ Includes                                              ]
//[-------------------------------------------------------]
// Disable warnings in external headers, we can't fix them
PRAGMA_WARNING_PUSH
	PRAGMA_WARNING_DISABLE_MSVC(4365)	// warning C4365: 'return': conversion from 'int' to 'std::char_traits<wchar_t>::int_type', signed/unsigned mismatch
	PRAGMA_WARNING_DISABLE_MSVC(4574)	// warning C4574: '_HAS_ITERATOR_DEBUGGING' is defined to be '0': did you mean to use '#if _HAS_ITERATOR_DEBUGGING'?
	PRAGMA_WARNING_DISABLE_MSVC(4668)	// warning C4668: '_M_HYBRID_X86_ARM64' is not defined as a preprocessor macro, replacing with '0' for '#if/#elif'
	PRAGMA_WARNING_DISABLE_MSVC(4987)	// warning C4987: nonstandard extension used: 'throw (...)'
	#include <new>			// For placement new
	#include <cmath>
	#include <string.h>		// For "strcpy()"
	#include <inttypes.h>	// For uint32_t, uint64_t etc.
PRAGMA_WARNING_POP
#ifdef RENDERER_DEBUG
	#include <cassert>
	#define ASSERT assert	// TODO(co) "RENDERER_ASSERT()" should be used everywhere
#else
	#define ASSERT			// TODO(co) "RENDERER_ASSERT()" should be used everywhere
#endif
#ifdef RENDERER_STATISTICS
	// Disable warnings in external headers, we can't fix them
	PRAGMA_WARNING_PUSH
		PRAGMA_WARNING_DISABLE_MSVC(4365)	// warning C4365: 'return': conversion from 'int' to 'std::char_traits<wchar_t>::int_type', signed/unsigned mismatch
		PRAGMA_WARNING_DISABLE_MSVC(4623)	// warning C4623: 'std::_List_node<_Ty,std::_Default_allocator_traits<_Alloc>::void_pointer>': default constructor was implicitly defined as deleted
		PRAGMA_WARNING_DISABLE_MSVC(4625)	// warning C4625: 'std::_Ptr_base<_Ty>': copy constructor was implicitly defined as deleted
		PRAGMA_WARNING_DISABLE_MSVC(4626)	// warning C4626: 'std::_Compressed_pair<glslang::pool_allocator<char>,std::_String_val<std::_Simple_types<_Ty>>,false>': assignment operator was implicitly defined as deleted
		PRAGMA_WARNING_DISABLE_MSVC(4668)	// warning C4668: '_M_HYBRID_X86_ARM64' is not defined as a preprocessor macro, replacing with '0' for '#if/#elif'
		PRAGMA_WARNING_DISABLE_MSVC(5027)	// warning C5027: 'std::_Compressed_pair<glslang::pool_allocator<char>,std::_String_val<std::_Simple_types<_Ty>>,false>': move assignment operator was implicitly defined as deleted
		#include <atomic>	// For "std::atomic<>"
	PRAGMA_WARNING_POP
#endif
#ifdef _WIN32
	#include <intrin.h>	// For "__nop()"
#endif
#ifdef LINUX
	// Copied from "Xlib.h"
	struct _XDisplay;

	// Copied from "wayland-client.h"
	struct wl_display;
	struct wl_surface;
#endif




//[-------------------------------------------------------]
//[ Forward declarations                                  ]
//[-------------------------------------------------------]
namespace Renderer
{
	class ILog;
	class IAssert;
	class Context;
	class IRenderer;
	class IAllocator;
	class IShaderLanguage;
	class IResource;
		class IRootSignature;
		class IResourceGroup;
		class IProgram;
		class IVertexArray;
		class IRenderPass;
		class IRenderTarget;
			class IRenderWindow;
			class ISwapChain;
			class IFramebuffer;
			struct FramebufferAttachment;
		class CommandBuffer;
		class IBufferManager;
		class IBuffer;
			class IIndexBuffer;
			class IVertexBuffer;
			class IUniformBuffer;
			class ITextureBuffer;
			class IIndirectBuffer;
		class ITextureManager;
		class ITexture;
			class ITexture1D;
			class ITexture2D;
			class ITexture2DArray;
			class ITexture3D;
			class ITextureCube;
		class IState;
			class IPipelineState;
			class ISamplerState;
		class IShader;
			class IVertexShader;
			class ITessellationControlShader;
			class ITessellationEvaluationShader;
			class IGeometryShader;
			class IFragmentShader;
}


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
namespace Renderer
{




	//[-------------------------------------------------------]
	//[ Renderer/PlatformTypes.h                              ]
	//[-------------------------------------------------------]
	#ifdef _WIN32
		#ifdef X64_ARCHITECTURE
			typedef unsigned __int64 handle;	// Replacement for nasty Microsoft Windows stuff leading to header chaos
		#else
			typedef unsigned __int32 handle;	// Replacement for nasty Microsoft Windows stuff leading to header chaos
		#endif
	#elif LINUX
		#ifdef X64_ARCHITECTURE
			typedef uint64_t handle;
		#else
			typedef uint32_t handle;
		#endif
	#else
		#error "Unsupported platform"
	#endif




	//[-------------------------------------------------------]
	//[ Renderer/Context.h                                    ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Context class encapsulating all embedding related wirings
	*/
	class Context
	{

	// Public definitions
	public:
		enum class ContextType
		{
			WINDOWS,
			X11,
			WAYLAND
		};

	// Public methods
	public:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] log
		*    Log instance to use, the log instance must stay valid as long as the renderer instance exists
		*  @param[in] assert
		*    Assert instance to use, the assert instance must stay valid as long as the renderer instance exists
		*  @param[in] allocator
		*    Allocator instance to use, the allocator instance must stay valid as long as the renderer instance exists
		*  @param[in] nativeWindowHandle
		*    Native window handle
		*  @param[in] useExternalContext
		*    Indicates if an external renderer context is used; in this case the renderer itself has nothing to do with the creation/managing of an renderer context
		*  @param[in] contextType
		*    The type of the context
		*/
		inline Context(ILog& log, IAssert& assert, IAllocator& allocator, handle nativeWindowHandle = 0, bool useExternalContext = false, ContextType contextType = Context::ContextType::WINDOWS) :
			mLog(log),
			mAssert(assert),
			mAllocator(allocator),
			mNativeWindowHandle(nativeWindowHandle),
			mUseExternalContext(useExternalContext),
			mContextType(contextType),
			mRendererApiSharedLibrary(nullptr)
		{}

		/**
		*  @brief
		*    Destructor
		*/
		inline virtual ~Context()
		{}

		/**
		*  @brief
		*    Return the log instance
		*
		*  @return
		*    The log instance
		*/
		inline ILog& getLog() const
		{
			return mLog;
		}

		/**
		*  @brief
		*    Return the assert instance
		*
		*  @return
		*    The assert instance
		*/
		inline IAssert& getAssert() const
		{
			return mAssert;
		}

		/**
		*  @brief
		*    Return the allocator instance
		*
		*  @return
		*    The allocator instance
		*/
		inline IAllocator& getAllocator() const
		{
			return mAllocator;
		}

		/**
		*  @brief
		*    Return the native window handle
		*
		*  @return
		*    The native window handle
		*/
		inline handle getNativeWindowHandle() const
		{
			return mNativeWindowHandle;
		}

		/**
		*  @brief
		*    Return whether or not an external context is used
		*
		*  @return
		*    "true" if an external context is used, else "false"
		*/
		inline bool isUsingExternalContext() const
		{
			return mUseExternalContext;
		}

		/**
		*  @brief
		*    Return the type of the context
		*
		*  @return
		*    The context type
		*/
		inline ContextType getType() const
		{
			return mContextType;
		}

		/**
		*  @brief
		*    Return a handle to the renderer API shared library
		*
		*  @return
		*    The handle to the renderer API shared library
		*/
		inline void* getRendererApiSharedLibrary() const
		{
			return mRendererApiSharedLibrary;
		}

		/**
		*  @brief
		*    Set the handle for the renderer API shared library to use instead of let it load by the renderer instance
		*
		*  @param[in] rendererApiSharedLibrary
		*    A handle to the renderer API shared library; the renderer will use this handle instead of loading the renderer API shared library itself
		*/
		inline void setRendererApiSharedLibrary(void* rendererApiSharedLibrary)
		{
			mRendererApiSharedLibrary = rendererApiSharedLibrary;
		}

	// Private methods
	private:
		explicit Context(const Context&) = delete;
		Context& operator=(const Context&) = delete;

	// Private data
	private:
		ILog&		mLog;
		IAssert&	mAssert;
		IAllocator&	mAllocator;
		handle		mNativeWindowHandle;
		bool		mUseExternalContext;
		ContextType	mContextType;
		void*		mRendererApiSharedLibrary;	///< A handle to the renderer API shared library (e.g. obtained via "dlopen()" and co)

	};

	#ifdef LINUX
		/**
		*  @brief
		*    X11 version of the context class
		*/
		class X11Context final : public Context
		{

		// Public methods
		public:
			/**
			*  @brief
			*    Constructor
			*
			*  @param[in] log
			*    Log instance to use, the log instance must stay valid as long as the renderer instance exists
			*  @param[in] assert
			*    Assert instance to use, the assert instance must stay valid as long as the renderer instance exists
			*  @param[in] allocator
			*    Allocator instance to use, the allocator instance must stay valid as long as the renderer instance exists
			*  @param[in] display
			*    The X11 display connection
			*  @param[in] nativeWindowHandle
			*    Native window handle
			*  @param[in] useExternalContext
			*    Indicates if an external renderer context is used; in this case the renderer itself has nothing to do with the creation/managing of an renderer context
			*/
			inline X11Context(ILog& log, IAssert& assert, IAllocator& allocator, _XDisplay* display, handle nativeWindowHandle = 0, bool useExternalContext = false) :
				Context(log, assert, allocator, nativeWindowHandle, useExternalContext, Context::ContextType::X11),
				mDisplay(display)
			{}

			/**
			*  @brief
			*    Return the x11 display connection
			*
			*  @return
			*    The x11 display connection
			*/
			inline _XDisplay* getDisplay() const
			{
				return mDisplay;
			}


		// Private data
		private:
			_XDisplay* mDisplay;

		};

		/**
		*  @brief
		*    Wayland version of the context class
		*/
		class WaylandContext final : public Context
		{

		// Public methods
		public:
			/**
			*  @brief
			*    Constructor
			*
			*  @param[in] log
			*    Log instance to use, the log instance must stay valid as long as the renderer instance exists
			*  @param[in] assert
			*    Assert instance to use, the assert instance must stay valid as long as the renderer instance exists
			*  @param[in] allocator
			*    Allocator instance to use, the allocator instance must stay valid as long as the renderer instance exists
			*  @param[in] display
			*    The Wayland display connection
			*  @param[in] surface
			*    The Wayland surface
			*  @param[in] useExternalContext
			*    Indicates if an external renderer context is used; in this case the renderer itself has nothing to do with the creation/managing of an renderer context
			*/
			inline WaylandContext(ILog& log, IAssert& assert, IAllocator& allocator, wl_display* display, wl_surface* surface = 0, bool useExternalContext = false) :
				Context(log, assert, allocator, 1, useExternalContext, Context::ContextType::WAYLAND),	// Under Wayland the surface (aka window) handle is not an integer, but the renderer implementation expects an integer as window handle so we give here an value != 0 so that a swap chain is created
				mDisplay(display),
				mSurface(surface)
			{}

			/**
			*  @brief
			*    Return the Wayland display connection
			*
			*  @return
			*    The Wayland display connection
			*/
			inline wl_display* getDisplay() const
			{
				return mDisplay;
			}

			/**
			*  @brief
			*    Return the Wayland surface
			*
			*  @return
			*    The Wayland surface
			*/
			inline wl_surface* getSurface() const
			{
				return mSurface;
			}

		// Private data
		private:
			wl_display* mDisplay;
			wl_surface* mSurface;

		};
	#endif




	//[-------------------------------------------------------]
	//[ Renderer/ILog.h                                       ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Abstract log interface
	*
	*  @note
	*    - The implementation must be multi-threading safe since the renderer is allowed to internally use multiple threads
	*/
	class ILog
	{

	// Public definitions
	public:
		/**
		*  @brief
		*    Log message type
		*/
		enum class Type
		{
			TRACE,					///< Trace
			DEBUG,					///< Debug
			INFORMATION,			///< Information
			WARNING,				///< General warning
			PERFORMANCE_WARNING,	///< Performance related warning
			COMPATIBILITY_WARNING,	///< Compatibility related warning
			CRITICAL				///< Critical
		};

	// Public virtual Renderer::ILog methods
	public:
		/**
		*  @brief
		*    Print a formatted log message
		*
		*  @param[in] type
		*    Log message type
		*  @param[in] attachment
		*    Optional attachment (for example build shader source code), can be a null pointer
		*  @param[in] file
		*    File as ASCII string
		*  @param[in] line
		*    Line number
		*  @param[in] format
		*    "snprintf"-style formatted UTF-8 log message
		*
		*  @return
		*    "true" to request debug break, else "false"
		*/
		virtual bool print(Type type, const char* attachment, const char* file, uint32_t line, const char* format, ...) = 0;

	// Protected methods
	protected:
		inline ILog()
		{}

		inline virtual ~ILog()
		{}

		explicit ILog(const ILog&) = delete;
		ILog& operator=(const ILog&) = delete;

	};

	// Macros & definitions
	/**
	*  @brief
	*    Ease-of-use log macro
	*
	*  @param[in] context
	*    Renderer context to ask for the log interface
	*  @param[in] type
	*    Log message type
	*  @param[in] format
	*    "snprintf"-style formatted UTF-8 log message
	*
	*  @note
	*    - Example: RENDERER_LOG(mContext, DEBUG, "Direct3D 11 renderer backend startup")
	*    - See http://cnicholson.net/2009/02/stupid-c-tricks-adventures-in-assert/ - "2.  Wrap your macros in do { … } while(0)." for background information about the do-while wrap
	*/
	#define RENDERER_LOG(context, type, format, ...) \
		do \
		{ \
			if ((context).getLog().print(Renderer::ILog::Type::type, nullptr, __FILE__, static_cast<uint32_t>(__LINE__), format, ##__VA_ARGS__)) \
			{ \
				DEBUG_BREAK; \
			} \
		} while (0);




	//[-------------------------------------------------------]
	//[ Renderer/IAssert.h                                    ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Abstract assert interface
	*
	*  @note
	*    - The implementation must be multi-threading safe since the renderer is allowed to internally use multiple threads
	*/
	class IAssert
	{

	// Public virtual Renderer::IAssert methods
	public:
		/**
		*  @brief
		*    Handle assert
		*
		*  @param[in] expression
		*    Expression as ASCII string
		*  @param[in] file
		*    File as ASCII string
		*  @param[in] line
		*    Line number
		*  @param[in] format
		*    "snprintf"-style formatted UTF-8 assert message
		*
		*  @return
		*    "true" to request debug break, else "false"
		*/
		virtual bool handleAssert(const char* expression, const char* file, uint32_t line, const char* format, ...) = 0;

	// Protected methods
	protected:
		inline IAssert()
		{}

		inline virtual ~IAssert()
		{}

		explicit IAssert(const IAssert&) = delete;
		IAssert& operator=(const IAssert&) = delete;

	};

	// Macros & definitions
	/**
	*  @brief
	*    Ease-of-use assert macro
	*
	*  @param[in] context
	*    Renderer context to ask for the assert interface
	*  @param[in] expression
	*    Expression which must be true, else the assert triggers
	*  @param[in] format
	*    "snprintf"-style formatted UTF-8 assert message
	*
	*  @note
	*    - Example: RENDERER_ASSERT(mContext, isInitialized, "Direct3D 11 renderer backend assert failed")
	*    - See http://cnicholson.net/2009/02/stupid-c-tricks-adventures-in-assert/ - "2.  Wrap your macros in do { … } while(0)." for background information about the do-while wrap
	*/
	#ifdef RENDERER_DEBUG
		#define RENDERER_ASSERT(context, expression, format, ...) \
			do \
			{ \
				if (!(expression) && (context).getAssert().handleAssert(#expression, __FILE__, static_cast<uint32_t>(__LINE__), format, ##__VA_ARGS__)) \
				{ \
					DEBUG_BREAK; \
				} \
			} while (0);
	#else
		#define RENDERER_ASSERT(context, expression, format, ...)
	#endif




	//[-------------------------------------------------------]
	//[ Renderer/IAllocator.h                                 ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Abstract memory allocator interface
	*
	*  @note
	*    - The implementation must be multi-threading safe since the renderer is allowed to internally use multiple threads
	*    - The interface design is basing on "Nicholas Frechette's Blog Raw bits" - "A memory allocator interface" - http://nfrechette.github.io/2015/05/11/memory_allocator_interface/
	*/
	class IAllocator
	{

	// Public static methods
	public:
		template<typename Type>
		static inline Type* constructN(Type* basePointer, size_t count)
		{
			for (size_t i = 0; i < count; ++i)
			{
				new ((void*)(basePointer + i)) Type();
			}
			return basePointer;
		}

	// Public methods
	public:
		/**
		*  @brief
		*    Reallocate
		*
		*  @param[in] oldPointer
		*    Old pointer, can be a null pointer
		*  @param[in] oldNumberOfBytes
		*    Old number of bytes, must be zero of the old pointer is a null pointer, else can be zero if this information isn't available
		*  @param[in] newNumberOfBytes
		*    New number of bytes
		*  @param[in] alignment
		*    Alignment
		*/
		inline void* reallocate(void* oldPointer, size_t oldNumberOfBytes, size_t newNumberOfBytes, size_t alignment)
		{
			ASSERT(mReallocateFuntion);
			ASSERT(nullptr != oldPointer || 0 == oldNumberOfBytes);
			return (*mReallocateFuntion)(*this, oldPointer, oldNumberOfBytes, newNumberOfBytes, alignment);
		}

	// Protected definitions
	protected:
		typedef void* (*ReallocateFuntion)(IAllocator&, void*, size_t, size_t, size_t);

	// Protected methods
	protected:
		inline explicit IAllocator(ReallocateFuntion reallocateFuntion) :
			mReallocateFuntion(reallocateFuntion)
		{
			ASSERT(mReallocateFuntion);
		}

		inline virtual ~IAllocator()
		{}

		explicit IAllocator(const IAllocator&) = delete;
		IAllocator& operator=(const IAllocator&) = delete;

	// Private methods
	private:
		ReallocateFuntion mReallocateFuntion;

	};

	// Macros & definitions

	// Malloc and free
	#define RENDERER_MALLOC(context, newNumberOfBytes) (context).getAllocator().reallocate(nullptr, 0, newNumberOfBytes, 1)
	#define RENDERER_MALLOC_TYPED(context, type, newNumberOfElements) reinterpret_cast<type*>((context).getAllocator().reallocate(nullptr, 0, sizeof(type) * (newNumberOfElements), 1))
	#define RENDERER_FREE(context, oldPointer) (context).getAllocator().reallocate(oldPointer, 0, 0, 1)

	// New and delete
	// - Using placement new and explicit destructor call
	// - See http://cnicholson.net/2009/02/stupid-c-tricks-adventures-in-assert/ - "2.  Wrap your macros in do { … } while(0)." for background information about the do-while wrap
	#define RENDERER_NEW(context, type) new ((context).getAllocator().reallocate(nullptr, 0, sizeof(type), 1)) type
	#define RENDERER_DELETE(context, type, oldPointer) \
		do \
		{ \
			if (nullptr != oldPointer) \
			{ \
				oldPointer->~type(); \
				(context).getAllocator().reallocate(oldPointer, 0, 0, 1); \
			} \
		} while (0)

	// New and delete of arrays
	// - Using placement new and explicit destructor call, not using the array version since it's using an undocumented additional amount of memory
	// - See http://cnicholson.net/2009/02/stupid-c-tricks-adventures-in-assert/ - "2.  Wrap your macros in do { … } while(0)." for background information about the do-while wrap
	#define RENDERER_NEW_ARRAY(context, type, count) Renderer::IAllocator::constructN(static_cast<type*>(((context).getAllocator().reallocate(nullptr, 0, sizeof(type) * (count), 1))), count)
	#define RENDERER_DELETE_ARRAY(context, type, oldPointer, count) \
		do \
		{ \
			if (nullptr != oldPointer) \
			{ \
				for (size_t allocatorArrayIndex = 0; allocatorArrayIndex < count; ++allocatorArrayIndex) \
				{ \
					(oldPointer)[allocatorArrayIndex].~type(); \
				} \
				(context).getAllocator().reallocate(oldPointer, 0, 0, 1); \
			} \
		} while (0)




	//[-------------------------------------------------------]
	//[ Renderer/RendererTypes.h                              ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Holds information about an window into which the rendering should be done
	*
	*  @note
	*    - One of those members must be valid
	*/
	struct WindowHandle final
	{
		handle			nativeWindowHandle;	///< The native window handle
		IRenderWindow*	renderWindow;		///< A pointer to an "Renderer::IRenderWindow"-instance, can be a null pointer
		#ifdef LINUX
			wl_surface*	waylandSurface;		///< A Wayland surface can't be put into a handle type, so we store a pointer to the Wayland surface here
		#else
			void*		unused;				///< For writing e.g. "Renderer::WindowHandle{nativeWindowHandle, nullptr, nullptr}" for all platforms // TODO(co) We might want to implement another solution like "WindowHandle::fromNativeWindowHandle()", "WindowHandle::fromRenderWindow()", "WindowHandle::fromWaylandSurface()", while there we could add a type and then using a data-union
		#endif
	};

	/**
	*  @brief
	*    Map types
	*
	*  @note
	*    - These constants directly map to Direct3D 10 & 11 constants, do not change them
	*/
	// TODO(co) Comments
	enum class MapType
	{
		READ			   = 1,	///<
		WRITE			   = 2,	///<
		READ_WRITE		   = 3,	///<
		WRITE_DISCARD	   = 4,	///<
		WRITE_NO_OVERWRITE = 5	///<
	};

	/**
	*  @brief
	*    Map flags
	*
	*  @note
	*    - These constants directly map to Direct3D 11 constants, do not change them
	*/
	struct MapFlag final
	{
		enum Enum
		{
			DO_NOT_WAIT = 0x100000L	///< In case the resource is currently used when "Renderer::IRenderer::map()" is called, let the method return with an error, cannot be used with "Renderer::MapType::WRITE_DISCARD" or "Renderer::MapType::WRITE_NO_OVERWRITE"
		};
	};

	/**
	*  @brief
	*    Clear flags
	*/
	struct ClearFlag final
	{
		enum Enum
		{
			COLOR       = 1 << 0,		///< Clear color buffer
			DEPTH       = 1 << 1,		///< Clear depth buffer
			STENCIL     = 1 << 2,		///< Clear stencil buffer
			COLOR_DEPTH = COLOR | DEPTH	///< Clear color and depth buffer
		};
	};

	/**
	*  @brief
	*    Comparison function
	*
	*  @note
	*    - Original Direct3D comments from http://msdn.microsoft.com/en-us/library/windows/desktop/bb204902%28v=vs.85%29.aspx are used in here
	*    - These constants directly map to Direct3D 10 & 11 & 12 constants, do not change them
	*
	*  @see
	*    - "D3D12_COMPARISON_FUNC"-documentation for details
	*/
	enum class ComparisonFunc
	{
		NEVER		  = 1,	///< Never pass the comparison
		LESS		  = 2,	///< If the source data is less than the destination data, the comparison passes
		EQUAL		  = 3,	///< If the source data is equal to the destination data, the comparison passes
		LESS_EQUAL	  = 4,	///< If the source data is less than or equal to the destination data, the comparison passes
		GREATER		  = 5,	///< If the source data is greater than the destination data, the comparison passes
		NOT_EQUAL	  = 6,	///< If the source data is not equal to the destination data, the comparison passes
		GREATER_EQUAL = 7,	///< If the source data is greater than or equal to the destination data, the comparison passes
		ALWAYS		  = 8	///< Always pass the comparison
	};

	/**
	*  @brief
	*    Color write enable flags
	*
	*  @note
	*    - These constants directly map to Direct3D 10 & 11 constants, do not change them
	*/
	// TODO(co) Renderer::ColorWriteEnableFlag, document
	// TODO(co) A flags-class would be nice to avoid invalid flags
	struct ColorWriteEnableFlag final
	{
		enum Enum
		{
			RED   = 1,
			GREEN = 2,
			BLUE  = 4,
			ALPHA = 8,
			ALL   = (((RED | GREEN) | BLUE) | ALPHA)
		};
	};

	/**
	*  @brief
	*    Mapped subresource
	*
	*  @note
	*    - This structure directly maps to Direct3D 11, do not change it
	*/
	// TODO(co) Comments
	struct MappedSubresource final
	{
		void*	 data;
		uint32_t rowPitch;
		uint32_t depthPitch;
	};

	/**
	*  @brief
	*    Viewport
	*
	*  @note
	*    - This structure directly maps to Direct3D 11 & 12 as well as Vulkan, do not change it
	*
	*  @see
	*    - "D3D12_VIEWPORT" or "VkViewport" documentation for details
	*/
	struct Viewport final
	{
		float topLeftX;	///< Top left x start position
		float topLeftY;	///< Top left y start position
		float width;	///< Viewport width
		float height;	///< Viewport height
		float minDepth;	///< Minimum depth value, usually 0.0f, between [0, 1]
		float maxDepth;	///< Maximum depth value, usually 1.0f, between [0, 1]
	};

	/**
	*  @brief
	*    Scissor rectangle
	*
	*  @note
	*    - This structure directly maps to Direct3D 9 & 10 & 11 & 12, do not change it
	*
	*  @see
	*    - "D3D12_RECT"-documentation for details
	*/
	struct ScissorRectangle final
	{
		long topLeftX;		///< Top left x-coordinate of the scissor rectangle
		long topLeftY;		///< Top left y-coordinate of the scissor rectangle
		long bottomRightX;	///< Bottom right x-coordinate of the scissor rectangle
		long bottomRightY;	///< Bottom right y-coordinate of the scissor rectangle
	};




	//[-------------------------------------------------------]
	//[ Renderer/ResourceType.h                               ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Resource type
	*/
	enum class ResourceType
	{
		ROOT_SIGNATURE				   = 0,		///< Root signature
		RESOURCE_GROUP				   = 1,		///< Resource group
		PROGRAM						   = 2,		///< Program, "Renderer::IShader"-related
		VERTEX_ARRAY				   = 3,		///< Vertex array object (VAO, input-assembler (IA) stage), "Renderer::IBuffer"-related
		RENDER_PASS					   = 4,		///< Render pass
		// IRenderTarget
		SWAP_CHAIN					   = 5,		///< Swap chain
		FRAMEBUFFER					   = 6,		///< Framebuffer object (FBO)
		// IBuffer
		INDEX_BUFFER				   = 7,		///< Index buffer object (IBO, input-assembler (IA) stage)
		VERTEX_BUFFER				   = 8,		///< Vertex buffer object (VBO, input-assembler (IA) stage)
		UNIFORM_BUFFER				   = 9,		///< Uniform buffer object (UBO, "constant buffer" in Direct3D terminology)
		TEXTURE_BUFFER				   = 10,	///< Texture buffer object (TBO)
		INDIRECT_BUFFER				   = 11,	///< Indirect buffer object
		// ITexture
		TEXTURE_1D					   = 12,	///< Texture 1D
		TEXTURE_2D					   = 13,	///< Texture 2D
		TEXTURE_2D_ARRAY			   = 14,	///< Texture 2D array
		TEXTURE_3D					   = 15,	///< Texture 3D
		TEXTURE_CUBE				   = 16,	///< Texture cube
		// IState
		PIPELINE_STATE				   = 17,	///< Pipeline state (PSO)
		SAMPLER_STATE				   = 18,	///< Sampler state
		// IShader
		VERTEX_SHADER				   = 19,	///< Vertex shader (VS)
		TESSELLATION_CONTROL_SHADER	   = 20,	///< Tessellation control shader (TCS, "hull shader" in Direct3D terminology)
		TESSELLATION_EVALUATION_SHADER = 21,	///< Tessellation evaluation shader (TES, "domain shader" in Direct3D terminology)
		GEOMETRY_SHADER				   = 22,	///< Geometry shader (GS)
		FRAGMENT_SHADER				   = 23		///< Fragment shader (FS, "pixel shader" in Direct3D terminology)
	};




	//[-------------------------------------------------------]
	//[ Renderer/State/SamplerStateTypes.h                    ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Filter mode
	*
	*  @note
	*    - Original Direct3D comments from http://msdn.microsoft.com/en-us/library/windows/desktop/bb205060%28v=vs.85%29.aspx are used in here
	*    - These constants directly map to Direct3D 10 & 11 & 12 constants, do not change them
	*    - "Point" = "nearest" in OpenGL terminology
	*
	*  @see
	*    - "D3D12_FILTER"-documentation for details
	*/
	enum class FilterMode
	{
		MIN_MAG_MIP_POINT						   = 0,		///< Use point sampling for minification, magnification, and mip-level sampling.
		MIN_MAG_POINT_MIP_LINEAR				   = 0x1,	///< Use point sampling for minification and magnification; use linear interpolation for mip-level sampling.
		MIN_POINT_MAG_LINEAR_MIP_POINT			   = 0x4,	///< Use point sampling for minification; use linear interpolation for magnification; use point sampling for mip-level sampling.
		MIN_POINT_MAG_MIP_LINEAR				   = 0x5,	///< Use point sampling for minification; use linear interpolation for magnification and mip-level sampling.
		MIN_LINEAR_MAG_MIP_POINT				   = 0x10,	///< Use linear interpolation for minification; use point sampling for magnification and mip-level sampling.
		MIN_LINEAR_MAG_POINT_MIP_LINEAR			   = 0x11,	///< Use linear interpolation for minification; use point sampling for magnification; use linear interpolation for mip-level sampling.
		MIN_MAG_LINEAR_MIP_POINT				   = 0x14,	///< Use linear interpolation for minification and magnification; use point sampling for mip-level sampling.
		MIN_MAG_MIP_LINEAR						   = 0x15,	///< Use linear interpolation for minification, magnification, and mip-level sampling.
		ANISOTROPIC								   = 0x55,	///< Use anisotropic interpolation for minification, magnification, and mip-level sampling.
		COMPARISON_MIN_MAG_MIP_POINT			   = 0x80,	///< Use point sampling for minification, magnification, and mip-level sampling. Compare the result to the comparison value.
		COMPARISON_MIN_MAG_POINT_MIP_LINEAR		   = 0x81,	///< Use point sampling for minification and magnification; use linear interpolation for mip-level sampling. Compare the result to the comparison value.
		COMPARISON_MIN_POINT_MAG_LINEAR_MIP_POINT  = 0x84,	///< Use point sampling for minification; use linear interpolation for magnification; use point sampling for mip-level sampling. Compare the result to the comparison value.
		COMPARISON_MIN_POINT_MAG_MIP_LINEAR		   = 0x85,	///< Use point sampling for minification; use linear interpolation for magnification and mip-level sampling. Compare the result to the comparison value.
		COMPARISON_MIN_LINEAR_MAG_MIP_POINT		   = 0x90,	///< Use linear interpolation for minification; use point sampling for magnification and mip-level sampling. Compare the result to the comparison value.
		COMPARISON_MIN_LINEAR_MAG_POINT_MIP_LINEAR = 0x91,	///< Use linear interpolation for minification; use point sampling for magnification; use linear interpolation for mip-level sampling. Compare the result to the comparison value.
		COMPARISON_MIN_MAG_LINEAR_MIP_POINT		   = 0x94,	///< Use linear interpolation for minification and magnification; use point sampling for mip-level sampling. Compare the result to the comparison value.
		COMPARISON_MIN_MAG_MIP_LINEAR			   = 0x95,	///< Use linear interpolation for minification, magnification, and mip-level sampling. Compare the result to the comparison value.
		COMPARISON_ANISOTROPIC					   = 0xd5,	///< Use anisotropic interpolation for minification, magnification, and mip-level sampling. Compare the result to the comparison value.
		UNKNOWN									   = 0xd6	///< Unknown invalid setting
	};

	/**
	*  @brief
	*    Texture address mode
	*
	*  @note
	*    - Original Direct3D comments from http://msdn.microsoft.com/en-us/library/windows/desktop/bb172483%28v=vs.85%29.aspx are used in here
	*    - These constants directly map to Direct3D 10 & 11 & 12 constants, do not change them
	*
	*  @see
	*    - "D3D12_TEXTURE_ADDRESS_MODE"-documentation for details
	*/
	enum class TextureAddressMode
	{
		WRAP		= 1,	///< Tile the texture at every integer junction. For example, for u values between 0 and 3, the texture is repeated three times.
		MIRROR		= 2,	///< Flip the texture at every integer junction. For u values between 0 and 1, for example, the texture is addressed normally; between 1 and 2, the texture is flipped (mirrored); between 2 and 3, the texture is normal again; and so on.
		CLAMP		= 3,	///< Texture coordinates outside the range [0.0, 1.0] are set to the texture color at 0.0 or 1.0, respectively.
		BORDER		= 4,	///< Texture coordinates outside the range [0.0, 1.0] are set to the border color specified in "SamplerState::borderColor".
		MIRROR_ONCE	= 5		///< Similar to "MIRROR" and "CLAMP". Takes the absolute value of the texture coordinate (thus, mirroring around 0), and then clamps to the maximum value.
	};

	/**
	*  @brief
	*    Sampler state
	*
	*  @remarks
	*    == About mipmapping ==
	*    The texture filter mode does not support explicitly disabling mipmapping. In case our texture does not have
	*    any mipmaps, set "Renderer::SamplerState::maxLOD" to zero in order to ensure a correct behaviour across the
	*    difference graphics APIs. When not doing this you usually have no issues when using OpenGL, OpenGL ES 3, Direct 10,
	*    Direct3D 11 or Direct3D 9 with the "ps_2_0"-profile, but when using Direct3D 9 with the "ps_3_0"-profile you might
	*    get into trouble due to another internal graphics API behaviour.
	*
	*  @note
	*    - This sampler state maps directly to Direct3D 10 & 11, do not change it
	*    - If you want to know how the default values were chosen, have a look into the "Renderer::ISamplerState::getDefaultSamplerState()"-implementation
	*
	*  @see
	*    - "D3D12_SAMPLER_DESC"-documentation for details
	*/
	struct SamplerState final
	{
		FilterMode		   filter;			///< Default: "Renderer::FilterMode::MIN_MAG_MIP_LINEAR"
		TextureAddressMode addressU;		///< (also known as "S"), Default: "Renderer::TextureAddressMode::CLAMP"
		TextureAddressMode addressV;		///< (also known as "T"), Default: "Renderer::TextureAddressMode::CLAMP"
		TextureAddressMode addressW;		///< (also known as "R"), Default: "Renderer::TextureAddressMode::CLAMP"
		float			   mipLODBias;		///< Default: "0.0f"
		uint32_t		   maxAnisotropy;	///< Default: "16"
		ComparisonFunc	   comparisonFunc;	///< Default: "Renderer::ComparisonFunc::NEVER"
		float			   borderColor[4];	///< Default: 0.0f, 0.0f, 0.0f, 0.0f
		float			   minLOD;			///< Default: -3.402823466e+38f (-FLT_MAX)
		float			   maxLOD;			///< Default: 3.402823466e+38f (FLT_MAX)
	};




	//[-------------------------------------------------------]
	//[ Renderer/RootSignatureTypes.h                         ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Descriptor range type
	*
	*  @note
	*    - These constants directly map to Direct3D 12 constants, do not change them
	*
	*  @see
	*    - "D3D12_DESCRIPTOR_RANGE_TYPE"-documentation for details
	*    - "UBV" = "CBV"; we're using the OpenGL/Vulkan terminology of "uniform buffer" instead of "constant buffer" as DirectX does
	*/
	enum class DescriptorRangeType
	{
		SRV					  = 0,
		UAV					  = SRV + 1,
		UBV					  = UAV + 1,
		SAMPLER				  = UBV + 1,
		NUMBER_OF_RANGE_TYPES = SAMPLER + 1
	};

	/**
	*  @brief
	*    Shader visibility
	*
	*  @note
	*    - These constants directly map to Direct3D 12 constants, do not change them
	*
	*  @see
	*    - "D3D12_SHADER_VISIBILITY"-documentation for details
	*/
	enum class ShaderVisibility
	{
		ALL                     = 0,
		VERTEX                  = 1,
		TESSELLATION_CONTROL    = 2,
		TESSELLATION_EVALUATION = 3,
		GEOMETRY                = 4,
		FRAGMENT                = 5
	};

	/**
	*  @brief
	*    Descriptor range
	*
	*  @note
	*    - "Renderer::DescriptorRange" is not identical to "D3D12_DESCRIPTOR_RANGE" because it had to be extended by information required by OpenGL
	*
	*  @see
	*    - "D3D12_DESCRIPTOR_RANGE"-documentation for details
	*/
	struct DescriptorRange
	{
		DescriptorRangeType rangeType;
		uint32_t			numberOfDescriptors;
		uint32_t			baseShaderRegister;						///< When using explicit binding locations
		uint32_t			registerSpace;
		uint32_t			offsetInDescriptorsFromTableStart;

		// The rest is not part of "D3D12_DESCRIPTOR_RANGE" and was added to support OpenGL and Direct3D 9 as well
		static constexpr uint32_t NAME_LENGTH = 32;
		char					  baseShaderRegisterName[NAME_LENGTH];	///< When not using explicit binding locations (OpenGL ES 3, legacy GLSL profiles)
		ShaderVisibility		  shaderVisibility;
	};
	struct DescriptorRangeBuilder final : public DescriptorRange
	{
		static constexpr uint32_t OFFSET_APPEND = 0xffffffff;
		static inline void initialize(
			DescriptorRange& range,
			DescriptorRangeType _rangeType,
			uint32_t _numberOfDescriptors,
			uint32_t _baseShaderRegister,
			const char _baseShaderRegisterName[NAME_LENGTH],
			ShaderVisibility _shaderVisibility,
			uint32_t _registerSpace = 0,
			uint32_t _offsetInDescriptorsFromTableStart = OFFSET_APPEND)
		{
			range.rangeType = _rangeType;
			range.numberOfDescriptors = _numberOfDescriptors;
			range.baseShaderRegister = _baseShaderRegister;
			range.registerSpace = _registerSpace;
			range.offsetInDescriptorsFromTableStart = _offsetInDescriptorsFromTableStart;
			strcpy(range.baseShaderRegisterName, _baseShaderRegisterName);
			range.shaderVisibility = _shaderVisibility;
		}
		inline DescriptorRangeBuilder()
		{}
		inline explicit DescriptorRangeBuilder(const DescriptorRangeBuilder&)
		{}
		inline DescriptorRangeBuilder(
			DescriptorRangeType _rangeType,
			uint32_t _numberOfDescriptors,
			uint32_t _baseShaderRegister,
			const char _baseShaderRegisterName[NAME_LENGTH],
			ShaderVisibility _shaderVisibility,
			uint32_t _registerSpace = 0,
			uint32_t _offsetInDescriptorsFromTableStart = OFFSET_APPEND)
		{
			initialize(_rangeType, _numberOfDescriptors, _baseShaderRegister, _baseShaderRegisterName, _shaderVisibility, _registerSpace, _offsetInDescriptorsFromTableStart);
		}
		inline void initializeSampler(
			uint32_t _numberOfDescriptors,
			uint32_t _baseShaderRegister,
			ShaderVisibility _shaderVisibility,
			uint32_t _registerSpace = 0,
			uint32_t _offsetInDescriptorsFromTableStart = OFFSET_APPEND)
		{
			initialize(*this, DescriptorRangeType::SAMPLER, _numberOfDescriptors, _baseShaderRegister, "", _shaderVisibility, _registerSpace, _offsetInDescriptorsFromTableStart);
		}
		inline void initialize(
			DescriptorRangeType _rangeType,
			uint32_t _numberOfDescriptors,
			uint32_t _baseShaderRegister,
			const char _baseShaderRegisterName[NAME_LENGTH],
			ShaderVisibility _shaderVisibility,
			uint32_t _registerSpace = 0,
			uint32_t _offsetInDescriptorsFromTableStart = OFFSET_APPEND)
		{
			initialize(*this, _rangeType, _numberOfDescriptors, _baseShaderRegister, _baseShaderRegisterName, _shaderVisibility, _registerSpace, _offsetInDescriptorsFromTableStart);
		}
	};

	/**
	*  @brief
	*    Root descriptor table
	*
	*  @note
	*    - This structure directly maps to Direct3D 12 structure, do not change it
	*    - Samplers are not allowed in the same descriptor table as UBV/UAV/SRVs
	*
	*  @see
	*    - "D3D12_ROOT_DESCRIPTOR_TABLE"-documentation for details
	*/
	struct RootDescriptorTable
	{
		uint32_t numberOfDescriptorRanges;
		uint64_t descriptorRanges;			///< Can't use "const DescriptorRange*" because we need to have something platform neutral we can easily serialize without getting too fine granular
	};
	struct RootDescriptorTableBuilder final : public RootDescriptorTable
	{
		static inline void initialize(
			RootDescriptorTable& rootDescriptorTable,
			uint32_t _numberOfDescriptorRanges,
			const DescriptorRange* _descriptorRanges)
		{
			rootDescriptorTable.numberOfDescriptorRanges = _numberOfDescriptorRanges;
			rootDescriptorTable.descriptorRanges = reinterpret_cast<uintptr_t>(_descriptorRanges);
		}
		inline RootDescriptorTableBuilder()
		{}
		inline explicit RootDescriptorTableBuilder(const RootDescriptorTableBuilder&)
		{}
		inline RootDescriptorTableBuilder(
			uint32_t _numberOfDescriptorRanges,
			const DescriptorRange* _descriptorRanges)
		{
			initialize(_numberOfDescriptorRanges, _descriptorRanges);
		}
		inline void initialize(
			uint32_t _numberOfDescriptorRanges,
			const DescriptorRange* _descriptorRanges)
		{
			initialize(*this, _numberOfDescriptorRanges, _descriptorRanges);
		}
	};

	/**
	*  @brief
	*    Root parameter type
	*
	*  @note
	*    - These constants directly map to Direct3D 12 constants, do not change them
	*
	*  @see
	*    - "D3D12_ROOT_PARAMETER_TYPE"-documentation for details
	*/
	enum class RootParameterType
	{
		DESCRIPTOR_TABLE = 0,
		CONSTANTS_32BIT  = DESCRIPTOR_TABLE + 1,
		UBV              = CONSTANTS_32BIT + 1,
		SRV              = UBV + 1,
		UAV              = SRV + 1
	};

	/**
	*  @brief
	*    Root constants
	*
	*  @note
	*    - This structure directly maps to Direct3D 12 structure, do not change it
	*
	*  @see
	*    - "D3D12_ROOT_CONSTANTS"-documentation for details
	*/
	struct RootConstants
	{
		uint32_t shaderRegister;
		uint32_t registerSpace;
		uint32_t numberOf32BitValues;
	};
	struct RootConstantsBuilder final : public RootConstants
	{
		static inline void initialize(
			RootConstants& rootConstants,
			uint32_t _numberOf32BitValues,
			uint32_t _shaderRegister,
			uint32_t _registerSpace = 0)
		{
			rootConstants.numberOf32BitValues = _numberOf32BitValues;
			rootConstants.shaderRegister = _shaderRegister;
			rootConstants.registerSpace = _registerSpace;
		}
		inline RootConstantsBuilder()
		{}
		inline explicit RootConstantsBuilder(const RootConstantsBuilder&)
		{}
		inline RootConstantsBuilder(
			uint32_t _numberOf32BitValues,
			uint32_t _shaderRegister,
			uint32_t _registerSpace = 0)
		{
			initialize(_numberOf32BitValues, _shaderRegister, _registerSpace);
		}
		inline void initialize(
			uint32_t _numberOf32BitValues,
			uint32_t _shaderRegister,
			uint32_t _registerSpace = 0)
		{
			initialize(*this, _numberOf32BitValues, _shaderRegister, _registerSpace);
		}
	};

	/**
	*  @brief
	*    Root descriptor
	*
	*  @note
	*    - This structure directly maps to Direct3D 12 structure, do not change it
	*
	*  @see
	*    - "D3D12_ROOT_DESCRIPTOR"-documentation for details
	*/
	struct RootDescriptor
	{
		uint32_t shaderRegister;
		uint32_t registerSpace;
	};
	struct RootDescriptorBuilder final : public RootDescriptor
	{
		static inline void initialize(RootDescriptor& table, uint32_t _shaderRegister, uint32_t _registerSpace = 0)
		{
			table.shaderRegister = _shaderRegister;
			table.registerSpace = _registerSpace;
		}
		inline RootDescriptorBuilder()
		{}
		inline explicit RootDescriptorBuilder(const RootDescriptorBuilder&)
		{}
		inline RootDescriptorBuilder(
			uint32_t _shaderRegister,
			uint32_t _registerSpace = 0)
		{
			initialize(_shaderRegister, _registerSpace);
		}
		inline void initialize(
			uint32_t _shaderRegister,
			uint32_t _registerSpace = 0)
		{
			initialize(*this, _shaderRegister, _registerSpace);
		}
	};

	/**
	*  @brief
	*    Root parameter
	*
	*  @note
	*    - "Renderer::RootParameter" is not identical to "D3D12_ROOT_PARAMETER", the shader visibility is defined per descriptor since Vulkan needs it this way
	*
	*  @see
	*    - "D3D12_ROOT_PARAMETER"-documentation for details
	*/
	struct RootParameter
	{
		RootParameterType		parameterType;
		union
		{
			RootDescriptorTable	descriptorTable;
			RootConstants		constants;
			RootDescriptor		descriptor;
		};
	};
	struct RootParameterData final
	{
		RootParameterType	parameterType;
		uint32_t			numberOfDescriptorRanges;
	};
	struct RootParameterBuilder final : public RootParameter
	{
		static inline void initializeAsDescriptorTable(
			RootParameter& rootParam,
			uint32_t numberOfDescriptorRanges,
			const DescriptorRange* descriptorRanges)
		{
			rootParam.parameterType = RootParameterType::DESCRIPTOR_TABLE;
			RootDescriptorTableBuilder::initialize(rootParam.descriptorTable, numberOfDescriptorRanges, descriptorRanges);
		}
		static inline void initializeAsConstants(
			RootParameter& rootParam,
			uint32_t numberOf32BitValues,
			uint32_t shaderRegister,
			uint32_t registerSpace = 0)
		{
			rootParam.parameterType = RootParameterType::CONSTANTS_32BIT;
			RootConstantsBuilder::initialize(rootParam.constants, numberOf32BitValues, shaderRegister, registerSpace);
		}
		static inline void initializeAsConstantBufferView(
			RootParameter& rootParam,
			uint32_t shaderRegister,
			uint32_t registerSpace = 0)
		{
			rootParam.parameterType = RootParameterType::UBV;
			RootDescriptorBuilder::initialize(rootParam.descriptor, shaderRegister, registerSpace);
		}
		static inline void initializeAsShaderResourceView(
			RootParameter& rootParam,
			uint32_t shaderRegister,
			uint32_t registerSpace = 0)
		{
			rootParam.parameterType = RootParameterType::SRV;
			RootDescriptorBuilder::initialize(rootParam.descriptor, shaderRegister, registerSpace);
		}
		static inline void initializeAsUnorderedAccessView(
			RootParameter& rootParam,
			uint32_t shaderRegister,
			uint32_t registerSpace = 0)
		{
			rootParam.parameterType = RootParameterType::UAV;
			RootDescriptorBuilder::initialize(rootParam.descriptor, shaderRegister, registerSpace);
		}
		inline RootParameterBuilder()
		{}
		inline explicit RootParameterBuilder(const RootParameterBuilder&)
		{}
		inline void initializeAsDescriptorTable(
			uint32_t numberOfDescriptorRanges,
			const DescriptorRange* descriptorRanges)
		{
			initializeAsDescriptorTable(*this, numberOfDescriptorRanges, descriptorRanges);
		}
		inline void initializeAsConstants(
			uint32_t numberOf32BitValues,
			uint32_t shaderRegister,
			uint32_t registerSpace = 0)
		{
			initializeAsConstants(*this, numberOf32BitValues, shaderRegister, registerSpace);
		}
		inline void initializeAsConstantBufferView(
			uint32_t shaderRegister,
			uint32_t registerSpace = 0)
		{
			initializeAsConstantBufferView(*this, shaderRegister, registerSpace);
		}
		inline void initializeAsShaderResourceView(
			uint32_t shaderRegister,
			uint32_t registerSpace = 0)
		{
			initializeAsShaderResourceView(*this, shaderRegister, registerSpace);
		}
		inline void initializeAsUnorderedAccessView(
			uint32_t shaderRegister,
			uint32_t registerSpace = 0)
		{
			initializeAsUnorderedAccessView(*this, shaderRegister, registerSpace);
		}
	};

	/**
	*  @brief
	*    Root signature flags
	*
	*  @note
	*    - These constants directly map to Direct3D 12 constants, do not change them
	*
	*  @see
	*    - "D3D12_ROOT_SIGNATURE_FLAGS"-documentation for details
	*/
	struct RootSignatureFlags final
	{
		enum Enum
		{
			NONE                                            = 0,
			ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT              = 0x1,
			DENY_VERTEX_SHADER_ROOT_ACCESS                  = 0x2,
			DENY_TESSELLATION_CONTROL_SHADER_ROOT_ACCESS    = 0x4,
			DENY_TESSELLATION_EVALUATION_SHADER_ROOT_ACCESS = 0x8,
			DENY_GEOMETRY_SHADER_ROOT_ACCESS                = 0x10,
			DENY_FRAGMENT_SHADER_ROOT_ACCESS                = 0x20,
			ALLOW_STREAM_OUTPUT                             = 0x40
		};
	};

	/**
	*  @brief
	*    Static border color
	*
	*  @note
	*    - These constants directly map to Direct3D 12 constants, do not change them
	*
	*  @see
	*    - "D3D12_STATIC_BORDER_COLOR"-documentation for details
	*/
	enum class StaticBorderColor
	{
		TRANSPARENT_BLACK = 0,
		OPAQUE_BLACK      = TRANSPARENT_BLACK + 1,
		OPAQUE_WHITE      = OPAQUE_BLACK + 1
	};

	/**
	*  @brief
	*    Static sampler
	*
	*  @note
	*    - This structure directly maps to Direct3D 12 structure, do not change it
	*
	*  @see
	*    - "D3D12_STATIC_SAMPLER_DESC"-documentation for details
	*/
	struct StaticSampler final
	{
		FilterMode			filter;
		TextureAddressMode	addressU;
		TextureAddressMode	addressV;
		TextureAddressMode	addressW;
		float				mipLodBias;
		uint32_t			maxAnisotropy;
		ComparisonFunc		comparisonFunc;
		StaticBorderColor	borderColor;
		float				minLod;
		float				maxLod;
		uint32_t			shaderRegister;
		uint32_t			registerSpace;
		ShaderVisibility	shaderVisibility;
	};

	/**
	*  @brief
	*    Root signature
	*
	*  @note
	*    - "Renderer::RootSignature" is not totally identical to "D3D12_ROOT_SIGNATURE_DESC" because it had to be extended by information required by OpenGL, so can't cast to Direct3D 12 structure
	*    - In order to be renderer API independent, do always define and set samplers first
	*    - "Renderer::DescriptorRange": In order to be renderer API independent, do always provide "baseShaderRegisterName" for "Renderer::DescriptorRangeType::SRV" range types
	*
	*  @see
	*    - "D3D12_ROOT_SIGNATURE_DESC"-documentation for details
	*/
	struct RootSignature
	{
		uint32_t				 numberOfParameters;
		const RootParameter*	 parameters;
		uint32_t				 numberOfStaticSamplers;
		const StaticSampler*	 staticSamplers;
		RootSignatureFlags::Enum flags;
	};
	struct RootSignatureBuilder final : public RootSignature
	{
		static inline void initialize(
			RootSignature& rootSignature,
			uint32_t _numberOfParameters,
			const RootParameter* _parameters,
			uint32_t _numberOfStaticSamplers = 0,
			const StaticSampler* _staticSamplers = nullptr,
			RootSignatureFlags::Enum _flags = RootSignatureFlags::NONE)
		{
			rootSignature.numberOfParameters = _numberOfParameters;
			rootSignature.parameters = _parameters;
			rootSignature.numberOfStaticSamplers = _numberOfStaticSamplers;
			rootSignature.staticSamplers = _staticSamplers;
			rootSignature.flags = _flags;
		}
		inline RootSignatureBuilder()
		{}
		inline explicit RootSignatureBuilder(const RootSignatureBuilder&)
		{}
		inline RootSignatureBuilder(
			uint32_t _numberOfParameters,
			const RootParameter* _parameters,
			uint32_t _numberOfStaticSamplers = 0,
			const StaticSampler* _staticSamplers = nullptr,
			RootSignatureFlags::Enum _flags = RootSignatureFlags::NONE)
		{
			initialize(_numberOfParameters, _parameters, _numberOfStaticSamplers, _staticSamplers, _flags);
		}
		inline void initialize(
			uint32_t _numberOfParameters,
			const RootParameter* _parameters,
			uint32_t _numberOfStaticSamplers = 0,
			const StaticSampler* _staticSamplers = nullptr,
			RootSignatureFlags::Enum _flags = RootSignatureFlags::NONE)
		{
			initialize(*this, _numberOfParameters, _parameters, _numberOfStaticSamplers, _staticSamplers, _flags);
		}
	};




	//[-------------------------------------------------------]
	//[ Renderer/Texture/TextureTypes.h                       ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Texture format
	*/
	struct TextureFormat final
	{
		enum Enum
		{
			R8				  = 0,	///< 8-bit pixel format, all bits red
			R8G8B8			  = 1,	///< 24-bit pixel format, 8 bits for red, green and blue
			R8G8B8A8		  = 2,	///< 32-bit pixel format, 8 bits for red, green, blue and alpha
			R8G8B8A8_SRGB	  = 3,	///< 32-bit pixel format, 8 bits for red, green, blue and alpha; sRGB = RGB hardware gamma correction, the alpha channel always remains linear
			B8G8R8A8		  = 4,	///< 32-bit pixel format, 8 bits for red, green, blue and alpha
			R11G11B10F		  = 5,	///< 32-bit float format using 11 bits the red and green channel, 10 bits the blue channel; red and green channels have a 6 bits mantissa and a 5 bits exponent and blue has a 5 bits mantissa and 5 bits exponent, see e.g. https://www.khronos.org/opengl/wiki/Small_Float_Formats#R11F_G11F_B10F -> "Small Float Formats" -> "Numeric limits and precision" and "Small float formats – R11G11B10F precision" by Bart Wronski ( https://bartwronski.com/2017/04/02/small-float-formats-r11g11b10f-precision/ )
			R16G16B16A16F	  = 6,	///< 64-bit float format using 16 bits for the each channel (red, green, blue, alpha)
			R32G32B32A32F	  = 7,	///< 128-bit float format using 32 bits for the each channel (red, green, blue, alpha)
			BC1				  = 8,	///< DXT1 compression (known as BC1 in DirectX 10, RGB compression: 8:1, 8 bytes per block)
			BC1_SRGB		  = 9,	///< DXT1 compression (known as BC1 in DirectX 10, RGB compression: 8:1, 8 bytes per block); sRGB = RGB hardware gamma correction, the alpha channel always remains linear
			BC2				  = 10,	///< DXT3 compression (known as BC2 in DirectX 10, RGBA compression: 4:1, 16 bytes per block)
			BC2_SRGB		  = 11,	///< DXT3 compression (known as BC2 in DirectX 10, RGBA compression: 4:1, 16 bytes per block); sRGB = RGB hardware gamma correction, the alpha channel always remains linear
			BC3				  = 12,	///< DXT5 compression (known as BC3 in DirectX 10, RGBA compression: 4:1, 16 bytes per block)
			BC3_SRGB		  = 13,	///< DXT5 compression (known as BC3 in DirectX 10, RGBA compression: 4:1, 16 bytes per block); sRGB = RGB hardware gamma correction, the alpha channel always remains linear
			BC4				  = 14,	///< 1 component texture compression (also known as 3DC+/ATI1N, known as BC4 in DirectX 10, 8 bytes per block)
			BC5				  = 15,	///< 2 component texture compression (luminance & alpha compression 4:1 -> normal map compression, also known as 3DC/ATI2N, known as BC5 in DirectX 10, 16 bytes per block)
			ETC1			  = 16,	///< 3 component texture compression meant for mobile devices
			R16_UNORM		  = 17,	///< 16-bit unsigned-normalized-integer format that supports 16 bits for the red channel
			R32_UINT		  = 18,	///< 32-bit unsigned integer format
			R32_FLOAT		  = 19,	///< 32-bit float format
			D32_FLOAT		  = 20,	///< 32-bit float depth format
			R16G16_SNORM	  = 21,	///< A two-component, 32-bit signed-normalized-integer format that supports 16 bits for the red channel and 16 bits for the green channel
			R16G16_FLOAT	  = 22,	///< A two-component, 32-bit floating-point format that supports 16 bits for the red channel and 16 bits for the green channel
			UNKNOWN			  = 23,	///< Unknown
			NUMBER_OF_FORMATS = 24	///< Number of texture formats
		};

		/**
		*  @brief
		*    Return whether or not the given "Renderer::TextureFormat" is a compressed format
		*
		*  @param[in] textureFormat
		*    "Renderer::TextureFormat" to check
		*
		*  @return
		*    "true" if the given "Renderer::TextureFormat" is a compressed format, else "false"
		*/
		static inline bool isCompressed(Enum textureFormat)
		{
			static constexpr bool MAPPING[] =
			{
				false,	// Renderer::TextureFormat::R8            - 8-bit pixel format, all bits red
				false,	// Renderer::TextureFormat::R8G8B8        - 24-bit pixel format, 8 bits for red, green and blue
				false,	// Renderer::TextureFormat::R8G8B8A8      - 32-bit pixel format, 8 bits for red, green, blue and alpha
				false,	// Renderer::TextureFormat::R8G8B8A8_SRGB - 32-bit pixel format, 8 bits for red, green, blue and alpha; sRGB = RGB hardware gamma correction, the alpha channel always remains linear
				false,	// Renderer::TextureFormat::B8G8R8A8      - 32-bit pixel format, 8 bits for red, green, blue and alpha
				false,	// Renderer::TextureFormat::R11G11B10F    - 32-bit float format using 11 bits the red and green channel, 10 bits the blue channel; red and green channels have a 6 bits mantissa and a 5 bits exponent and blue has a 5 bits mantissa and 5 bits exponent
				false,	// Renderer::TextureFormat::R16G16B16A16F - 64-bit float format using 16 bits for the each channel (red, green, blue, alpha)
				false,	// Renderer::TextureFormat::R32G32B32A32F - 128-bit float format using 32 bits for the each channel (red, green, blue, alpha)
				true,	// Renderer::TextureFormat::BC1           - DXT1 compression (known as BC1 in DirectX 10, RGB compression: 8:1, 8 bytes per block) - when being uncompressed
				true,	// Renderer::TextureFormat::BC1_SRGB      - DXT1 compression (known as BC1 in DirectX 10, RGB compression: 8:1, 8 bytes per block) - when being uncompressed; sRGB = RGB hardware gamma correction, the alpha channel always remains linear
				true,	// Renderer::TextureFormat::BC2           - DXT3 compression (known as BC2 in DirectX 10, RGBA compression: 4:1, 16 bytes per block) - when being uncompressed
				true,	// Renderer::TextureFormat::BC2_SRGB      - DXT3 compression (known as BC2 in DirectX 10, RGBA compression: 4:1, 16 bytes per block) - when being uncompressed; sRGB = RGB hardware gamma correction, the alpha channel always remains linear
				true,	// Renderer::TextureFormat::BC3           - DXT5 compression (known as BC3 in DirectX 10, RGBA compression: 4:1, 16 bytes per block) - when being uncompressed
				true,	// Renderer::TextureFormat::BC3_SRGB      - DXT5 compression (known as BC3 in DirectX 10, RGBA compression: 4:1, 16 bytes per block) - when being uncompressed; sRGB = RGB hardware gamma correction, the alpha channel always remains linear
				true,	// Renderer::TextureFormat::BC4           - 1 component texture compression (also known as 3DC+/ATI1N, known as BC4 in DirectX 10, 8 bytes per block) - when being uncompressed
				true,	// Renderer::TextureFormat::BC5           - 2 component texture compression (luminance & alpha compression 4:1 -> normal map compression, also known as 3DC/ATI2N, known as BC5 in DirectX 10, 16 bytes per block) - when being uncompressed
				true,	// Renderer::TextureFormat::ETC1          - 3 component texture compression meant for mobile devices
				false,	// Renderer::TextureFormat::R16_UNORM     - 16-bit unsigned-normalized-integer format that supports 16 bits for the red channel
				false,	// Renderer::TextureFormat::R32_UINT      - 32-bit unsigned integer format
				false,	// Renderer::TextureFormat::R32_FLOAT     - 32-bit float format
				false,	// Renderer::TextureFormat::D32_FLOAT     - 32-bit float depth format
				false,	// Renderer::TextureFormat::R16G16_SNORM  - A two-component, 32-bit signed-normalized-integer format that supports 16 bits for the red channel and 16 bits for the green channel
				false,	// Renderer::TextureFormat::R16G16_FLOAT  - A two-component, 32-bit floating-point format that supports 16 bits for the red channel and 16 bits for the green channel
				false	// Renderer::TextureFormat::UNKNOWN       - Unknown
			};
			return MAPPING[textureFormat];
		}

		/**
		*  @brief
		*    Return whether or not the given "Renderer::TextureFormat" is a depth format
		*
		*  @param[in] textureFormat
		*    "Renderer::TextureFormat" to check
		*
		*  @return
		*    "true" if the given "Renderer::TextureFormat" is a depth format, else "false"
		*/
		static inline bool isDepth(Enum textureFormat)
		{
			static constexpr bool MAPPING[] =
			{
				false,	// Renderer::TextureFormat::R8            - 8-bit pixel format, all bits red
				false,	// Renderer::TextureFormat::R8G8B8        - 24-bit pixel format, 8 bits for red, green and blue
				false,	// Renderer::TextureFormat::R8G8B8A8      - 32-bit pixel format, 8 bits for red, green, blue and alpha
				false,	// Renderer::TextureFormat::R8G8B8A8_SRGB - 32-bit pixel format, 8 bits for red, green, blue and alpha; sRGB = RGB hardware gamma correction, the alpha channel always remains linear
				false,	// Renderer::TextureFormat::B8G8R8A8      - 32-bit pixel format, 8 bits for red, green, blue and alpha
				false,	// Renderer::TextureFormat::R11G11B10F    - 32-bit float format using 11 bits the red and green channel, 10 bits the blue channel; red and green channels have a 6 bits mantissa and a 5 bits exponent and blue has a 5 bits mantissa and 5 bits exponent
				false,	// Renderer::TextureFormat::R16G16B16A16F - 64-bit float format using 16 bits for the each channel (red, green, blue, alpha)
				false,	// Renderer::TextureFormat::R32G32B32A32F - 128-bit float format using 32 bits for the each channel (red, green, blue, alpha)
				false,	// Renderer::TextureFormat::BC1           - DXT1 compression (known as BC1 in DirectX 10, RGB compression: 8:1, 8 bytes per block) - when being uncompressed
				false,	// Renderer::TextureFormat::BC1_SRGB      - DXT1 compression (known as BC1 in DirectX 10, RGB compression: 8:1, 8 bytes per block) - when being uncompressed; sRGB = RGB hardware gamma correction, the alpha channel always remains linear
				false,	// Renderer::TextureFormat::BC2           - DXT3 compression (known as BC2 in DirectX 10, RGBA compression: 4:1, 16 bytes per block) - when being uncompressed
				false,	// Renderer::TextureFormat::BC2_SRGB      - DXT3 compression (known as BC2 in DirectX 10, RGBA compression: 4:1, 16 bytes per block) - when being uncompressed; sRGB = RGB hardware gamma correction, the alpha channel always remains linear
				false,	// Renderer::TextureFormat::BC3           - DXT5 compression (known as BC3 in DirectX 10, RGBA compression: 4:1, 16 bytes per block) - when being uncompressed
				false,	// Renderer::TextureFormat::BC3_SRGB      - DXT5 compression (known as BC3 in DirectX 10, RGBA compression: 4:1, 16 bytes per block) - when being uncompressed; sRGB = RGB hardware gamma correction, the alpha channel always remains linear
				false,	// Renderer::TextureFormat::BC4           - 1 component texture compression (also known as 3DC+/ATI1N, known as BC4 in DirectX 10, 8 bytes per block) - when being uncompressed
				false,	// Renderer::TextureFormat::BC5           - 2 component texture compression (luminance & alpha compression 4:1 -> normal map compression, also known as 3DC/ATI2N, known as BC5 in DirectX 10, 16 bytes per block) - when being uncompressed
				false,	// Renderer::TextureFormat::ETC1          - 3 component texture compression meant for mobile devices
				false,	// Renderer::TextureFormat::R16_UNORM     - 16-bit unsigned-normalized-integer format that supports 16 bits for the red channel
				false,	// Renderer::TextureFormat::R32_UINT      - 32-bit unsigned integer format
				false,	// Renderer::TextureFormat::R32_FLOAT     - 32-bit float format
				true,	// Renderer::TextureFormat::D32_FLOAT     - 32-bit float depth format
				false,	// Renderer::TextureFormat::R16G16_SNORM  - A two-component, 32-bit signed-normalized-integer format that supports 16 bits for the red channel and 16 bits for the green channel
				false,	// Renderer::TextureFormat::R16G16_FLOAT  - A two-component, 32-bit floating-point format that supports 16 bits for the red channel and 16 bits for the green channel
				false	// Renderer::TextureFormat::UNKNOWN       - Unknown
			};
			return MAPPING[textureFormat];
		}

		/**
		*  @brief
		*    "Renderer::TextureFormat" to number of bytes per element
		*
		*  @param[in] textureFormat
		*    "Renderer::TextureFormat" to map
		*
		*  @return
		*    Number of bytes per element
		*/
		static inline uint32_t getNumberOfBytesPerElement(Enum textureFormat)
		{
			static constexpr uint32_t MAPPING[] =
			{
				sizeof(uint8_t),		// Renderer::TextureFormat::R8            - 8-bit pixel format, all bits red
				sizeof(uint8_t) * 3,	// Renderer::TextureFormat::R8G8B8        - 24-bit pixel format, 8 bits for red, green and blue
				sizeof(uint8_t) * 4,	// Renderer::TextureFormat::R8G8B8A8      - 32-bit pixel format, 8 bits for red, green, blue and alpha
				sizeof(uint8_t) * 4,	// Renderer::TextureFormat::R8G8B8A8_SRGB - 32-bit pixel format, 8 bits for red, green, blue and alpha; sRGB = RGB hardware gamma correction, the alpha channel always remains linear
				sizeof(uint8_t) * 4,	// Renderer::TextureFormat::B8G8R8A8      - 32-bit pixel format, 8 bits for red, green, blue and alpha
				sizeof(float),			// Renderer::TextureFormat::R11G11B10F    - 32-bit float format using 11 bits the red and green channel, 10 bits the blue channel; red and green channels have a 6 bits mantissa and a 5 bits exponent and blue has a 5 bits mantissa and 5 bits exponent
				sizeof(float) * 2,		// Renderer::TextureFormat::R16G16B16A16F - 64-bit float format using 16 bits for the each channel (red, green, blue, alpha)
				sizeof(float) * 4,		// Renderer::TextureFormat::R32G32B32A32F - 128-bit float format using 32 bits for the each channel (red, green, blue, alpha)
				sizeof(uint8_t) * 3,	// Renderer::TextureFormat::BC1           - DXT1 compression (known as BC1 in DirectX 10, RGB compression: 8:1, 8 bytes per block) - when being uncompressed
				sizeof(uint8_t) * 3,	// Renderer::TextureFormat::BC1_SRGB      - DXT1 compression (known as BC1 in DirectX 10, RGB compression: 8:1, 8 bytes per block) - when being uncompressed; sRGB = RGB hardware gamma correction, the alpha channel always remains linear
				sizeof(uint8_t) * 4,	// Renderer::TextureFormat::BC2           - DXT3 compression (known as BC2 in DirectX 10, RGBA compression: 4:1, 16 bytes per block) - when being uncompressed
				sizeof(uint8_t) * 4,	// Renderer::TextureFormat::BC2_SRGB      - DXT3 compression (known as BC2 in DirectX 10, RGBA compression: 4:1, 16 bytes per block) - when being uncompressed; sRGB = RGB hardware gamma correction, the alpha channel always remains linear
				sizeof(uint8_t) * 4,	// Renderer::TextureFormat::BC3           - DXT5 compression (known as BC3 in DirectX 10, RGBA compression: 4:1, 16 bytes per block) - when being uncompressed
				sizeof(uint8_t) * 4,	// Renderer::TextureFormat::BC3_SRGB      - DXT5 compression (known as BC3 in DirectX 10, RGBA compression: 4:1, 16 bytes per block) - when being uncompressed; sRGB = RGB hardware gamma correction, the alpha channel always remains linear
				sizeof(uint8_t) * 1,	// Renderer::TextureFormat::BC4           - 1 component texture compression (also known as 3DC+/ATI1N, known as BC4 in DirectX 10, 8 bytes per block) - when being uncompressed
				sizeof(uint8_t) * 2,	// Renderer::TextureFormat::BC5           - 2 component texture compression (luminance & alpha compression 4:1 -> normal map compression, also known as 3DC/ATI2N, known as BC5 in DirectX 10, 16 bytes per block) - when being uncompressed
				sizeof(uint8_t) * 3,	// Renderer::TextureFormat::ETC1          - 3 component texture compression meant for mobile devices - when being uncompressed
				sizeof(uint16_t),		// Renderer::TextureFormat::R16_UNORM     - 16-bit unsigned-normalized-integer format that supports 16 bits for the red channel
				sizeof(uint32_t),		// Renderer::TextureFormat::R32_UINT      - 32-bit unsigned integer format
				sizeof(float),			// Renderer::TextureFormat::R32_FLOAT     - 32-bit float format
				sizeof(float),			// Renderer::TextureFormat::D32_FLOAT     - 32-bit float depth format
				sizeof(uint32_t),		// Renderer::TextureFormat::R16G16_SNORM  - A two-component, 32-bit signed-normalized-integer format that supports 16 bits for the red channel and 16 bits for the green channel
				sizeof(float),			// Renderer::TextureFormat::R16G16_FLOAT  - A two-component, 32-bit floating-point format that supports 16 bits for the red channel and 16 bits for the green channel
				0						// Renderer::TextureFormat::UNKNOWN       - Unknown
			};
			return MAPPING[textureFormat];
		}

		/**
		*  @brief
		*    "Renderer::TextureFormat" to number of bytes per row
		*
		*  @param[in] textureFormat
		*    "Renderer::TextureFormat" to map
		*  @param[in] width
		*    Row width
		*
		*  @return
		*    Number of bytes per row
		*/
		static inline uint32_t getNumberOfBytesPerRow(Enum textureFormat, uint32_t width)
		{
			switch (textureFormat)
			{
				// 8-bit pixel format, all bits red
				case R8:
					return width;

				// 24-bit pixel format, 8 bits for red, green and blue
				case R8G8B8:
					return 3 * width;

				// 32-bit pixel format, 8 bits for red, green, blue and alpha
				case R8G8B8A8:
				case R8G8B8A8_SRGB:
				case B8G8R8A8:
					return 4 * width;

				// 32-bit float format using 11 bits the red and green channel, 10 bits the blue channel; red and green channels have a 6 bits mantissa and a 5 bits exponent and blue has a 5 bits mantissa and 5 bits exponent
				case R11G11B10F:
					return 4 * width;

				// 64-bit float format using 16 bits for the each channel (red, green, blue, alpha)
				case R16G16B16A16F:
					return 8 * width;

				// 128-bit float format using 32 bits for the each channel (red, green, blue, alpha)
				case R32G32B32A32F:
					return 16 * width;

				// DXT1 compression (known as BC1 in DirectX 10, RGB compression: 8:1, 8 bytes per block)
				case BC1:
				case BC1_SRGB:
					return ((width + 3) >> 2) * 8;

				// DXT3 compression (known as BC2 in DirectX 10, RGBA compression: 4:1, 16 bytes per block)
				case BC2:
				case BC2_SRGB:
					return ((width + 3) >> 2) * 16;

				// DXT5 compression (known as BC3 in DirectX 10, RGBA compression: 4:1, 16 bytes per block)
				case BC3:
				case BC3_SRGB:
					return ((width + 3) >> 2) * 16;

				// 1 component texture compression (also known as 3DC+/ATI1N, known as BC4 in DirectX 10, 8 bytes per block)
				case BC4:
					return ((width + 3) >> 2) * 8;

				// 2 component texture compression (luminance & alpha compression 4:1 -> normal map compression, also known as 3DC/ATI2N, known as BC5 in DirectX 10, 16 bytes per block)
				case BC5:
					return ((width + 3) >> 2) * 16;

				// 3 component texture compression meant for mobile devices
				case ETC1:
					return (width >> 1);

				// 16-bit unsigned-normalized-integer format that supports 16 bits for the red channel
				case R16_UNORM:
					return sizeof(uint16_t) * width;

				// 32-bit unsigned integer format
				case R32_UINT:
					return sizeof(uint32_t) * width;

				// 32-bit float red/depth format
				case R32_FLOAT:
				case D32_FLOAT:
					return sizeof(float) * width;

				// A two-component, 32-bit signed-normalized-integer format that supports 16 bits for the red channel and 16 bits for the green channel
				case R16G16_SNORM:
					return sizeof(uint32_t) * width;

				// A two-component, 32-bit floating-point format that supports 16 bits for the red channel and 16 bits for the green channel
				case R16G16_FLOAT:
					return sizeof(float) * width;

				// Unknown
				case UNKNOWN:
				case NUMBER_OF_FORMATS:
					return 0;

				default:
					return 0;
			}
		}

		/**
		*  @brief
		*    "Renderer::TextureFormat" to number of bytes per slice
		*
		*  @param[in] textureFormat
		*    "Renderer::TextureFormat" to map
		*  @param[in] width
		*    Slice width
		*  @param[in] height
		*    Slice height
		*
		*  @return
		*    Number of bytes per slice
		*/
		static inline uint32_t getNumberOfBytesPerSlice(Enum textureFormat, uint32_t width, uint32_t height)
		{
			switch (textureFormat)
			{
				// 8-bit pixel format, all bits red
				case R8:
					return width * height;

				// 24-bit pixel format, 8 bits for red, green and blue
				case R8G8B8:
					return 3 * width * height;

				// 32-bit pixel format, 8 bits for red, green, blue and alpha
				case R8G8B8A8:
				case R8G8B8A8_SRGB:
				case B8G8R8A8:
					return 4 * width * height;

				// 32-bit float format using 11 bits the red and green channel, 10 bits the blue channel; red and green channels have a 6 bits mantissa and a 5 bits exponent and blue has a 5 bits mantissa and 5 bits exponent
				case R11G11B10F:
					return 4 * width * height;

				// 64-bit float format using 16 bits for the each channel (red, green, blue, alpha)
				case R16G16B16A16F:
					return 8 * width * height;

				// 128-bit float format using 32 bits for the each channel (red, green, blue, alpha)
				case R32G32B32A32F:
					return 16 * width * height;

				// DXT1 compression (known as BC1 in DirectX 10, RGB compression: 8:1, 8 bytes per block)
				case BC1:
				case BC1_SRGB:
					return ((width + 3) >> 2) * ((height + 3) >> 2) * 8;

				// DXT3 compression (known as BC2 in DirectX 10, RGBA compression: 4:1, 16 bytes per block)
				case BC2:
				case BC2_SRGB:
					return ((width + 3) >> 2) * ((height + 3) >> 2) * 16;

				// DXT5 compression (known as BC3 in DirectX 10, RGBA compression: 4:1, 16 bytes per block)
				case BC3:
				case BC3_SRGB:
					return ((width + 3) >> 2) * ((height + 3) >> 2) * 16;

				// 1 component texture compression (also known as 3DC+/ATI1N, known as BC4 in DirectX 10, 8 bytes per block)
				case BC4:
					return ((width + 3) >> 2) * ((height + 3) >> 2) * 8;

				// 2 component texture compression (luminance & alpha compression 4:1 -> normal map compression, also known as 3DC/ATI2N, known as BC5 in DirectX 10, 16 bytes per block)
				case BC5:
					return ((width + 3) >> 2) * ((height + 3) >> 2) * 16;

				// 3 component texture compression meant for mobile devices
				case ETC1:
				{
					const uint32_t numberOfBytesPerSlice = (width * height) >> 1;
					return (numberOfBytesPerSlice > 8) ? numberOfBytesPerSlice : 8;
				}

				// 16-bit unsigned-normalized-integer format that supports 16 bits for the red channel
				case R16_UNORM:
					return sizeof(uint16_t) * width * height;

				// 32-bit unsigned integer format
				case R32_UINT:
					return sizeof(uint32_t) * width * height;

				// 32-bit float depth format
				case R32_FLOAT:
				case D32_FLOAT:
					return sizeof(float) * width * height;

				// A two-component, 32-bit signed-normalized-integer format that supports 16 bits for the red channel and 16 bits for the green channel
				case R16G16_SNORM:
					return sizeof(uint32_t) * width * height;

				// A two-component, 32-bit floating-point format that supports 16 bits for the red channel and 16 bits for the green channel
				case R16G16_FLOAT:
					return sizeof(float) * width * height;

				// Unknown
				case UNKNOWN:
				case NUMBER_OF_FORMATS:
					return 0;

				default:
					return 0;
			}
		}

	};

	/**
	*  @brief
	*    Texture flags
	*/
	struct TextureFlag final
	{
		enum Enum
		{
			DATA_CONTAINS_MIPMAPS = 1 << 0,	///< The user provided data containing mipmaps from 0-n down to 1x1 linearly in memory
			GENERATE_MIPMAPS      = 1 << 1,	///< Automatically generate mipmaps (avoid this if you can, will be ignored in case the "DATA_CONTAINS_MIPMAPS"-flag is set), for depth textures the mipmaps can only be allocated but not automatically be generated
			RENDER_TARGET         = 1 << 2	///< This texture can be used as render target
		};
	};

	/**
	*  @brief
	*    Texture usage indication
	*
	*  @note
	*    - Only relevant for Direct3D, OpenGL has no texture usage indication
	*    - Original Direct3D comments from http://msdn.microsoft.com/en-us/library/windows/desktop/ff476259%28v=vs.85%29.aspx are used in here
	*    - These constants directly map to Direct3D 10 & 11 constants, do not change them
	*/
	enum class TextureUsage
	{
		DEFAULT   = 0,	///< A resource that requires read and write access by the GPU. This is likely to be the most common usage choice.
		IMMUTABLE = 1,	///< A resource that can only be read by the GPU. It cannot be written by the GPU, and cannot be accessed at all by the CPU. This type of resource must be initialized when it is created, since it cannot be changed after creation.
		DYNAMIC   = 2,	///< A resource that is accessible by both the GPU (read only) and the CPU (write only). A dynamic resource is a good choice for a resource that will be updated by the CPU at least once per frame. To update a dynamic resource, use a map method.
		STAGING   = 3	///< A resource that supports data transfer (copy) from the GPU to the CPU.
	};

	/**
	*  @brief
	*    Optimized clear value
	*
	*  @see
	*    - "ID3D12Device::CreateCommittedResource method" documentation at https://msdn.microsoft.com/de-de/library/windows/desktop/dn899178%28v=vs.85%29.aspx
	*/
	struct OptimizedTextureClearValue final
	{
		union
		{
			float color[4];
			struct
			{
				float depth;
				uint8_t stencil;
			} DepthStencil;
		};
	};




	//[-------------------------------------------------------]
	//[ Renderer/State/BlendStateTypes.h                      ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Blend
	*
	*  @note
	*    - These constants directly map to Direct3D 10 & 11 & 12 constants, do not change them
	*
	*  @see
	*    - "D3D12_BLEND"-documentation for details
	*/
	enum class Blend
	{
		ZERO			 = 1,
		ONE				 = 2,
		SRC_COLOR		 = 3,
		INV_SRC_COLOR	 = 4,
		SRC_ALPHA		 = 5,
		INV_SRC_ALPHA	 = 6,
		DEST_ALPHA		 = 7,
		INV_DEST_ALPHA	 = 8,
		DEST_COLOR		 = 9,
		INV_DEST_COLOR	 = 10,
		SRC_ALPHA_SAT	 = 11,
		BLEND_FACTOR	 = 14,
		INV_BLEND_FACTOR = 15,
		SRC_1_COLOR		 = 16,
		INV_SRC_1_COLOR	 = 17,
		SRC_1_ALPHA		 = 18,
		INV_SRC_1_ALPHA	 = 19
	};

	/**
	*  @brief
	*    Blend operation
	*
	*  @note
	*    - These constants directly map to Direct3D 10 & 11 constants, do not change them
	*
	*  @see
	*    - "D3D12_BLEND_OP"-documentation for details
	*/
	enum class BlendOp
	{
		ADD			 = 1,
		SUBTRACT	 = 2,
		REV_SUBTRACT = 3,
		MIN			 = 4,
		MAX			 = 5
	};

	/**
	*  @brief
	*    Render target blend description
	*
	*  @note
	*    - This render target blend description maps directly to Direct3D 10.1 & 11, do not change it
	*    - This also means that "int" is used over "bool" because in Direct3D it's defined this way
	*    - If you want to know how the default values were chosen, have a look into the "Renderer::BlendStateBuilder::getDefaultBlendState()"-implementation
	*
	*  @see
	*    - "D3D12_RENDER_TARGET_BLEND_DESC"-documentation for details
	*/
	struct RenderTargetBlendDesc final
	{
		int		blendEnable;			///< Boolean value. Default: "false"
		Blend	srcBlend;				///< Default: "Renderer::Blend::ONE"
		Blend	destBlend;				///< Default: "Renderer::Blend::ZERO"
		BlendOp	blendOp;				///< Default: "Renderer::BlendOp::ADD"
		Blend	srcBlendAlpha;			///< Default: "Renderer::Blend::ONE"
		Blend	destBlendAlpha;			///< Default: "Renderer::Blend::ZERO"
		BlendOp	blendOpAlpha;			///< Default: "Renderer::BlendOp::ADD"
		uint8_t	renderTargetWriteMask;	///< Combination of "Renderer::ColorWriteEnableFlag"-flags. Default: "Renderer::ColorWriteEnableFlag::ALL"
	};

	/**
	*  @brief
	*    Blend state
	*
	*  @note
	*    - This blend state maps directly to Direct3D 10.1 & 11 & 12, do not change it
	*    - This also means that "int" is used over "bool" because in Direct3D it's defined this way
	*    - If you want to know how the default values were chosen, have a look into the "Renderer::BlendStateBuilder::getDefaultBlendState()"-implementation
	*
	*  @see
	*    - "D3D12_BLEND_DESC"-documentation for details
	*/
	struct BlendState final
	{
		int					  alphaToCoverageEnable;	///< Boolean value. Default: "false"
		int					  independentBlendEnable;	///< Boolean value. Default: "false"
		RenderTargetBlendDesc renderTarget[8];			///< Default: See "Renderer::RenderTargetBlendDesc"
	};
	struct BlendStateBuilder final
	{
		/**
		*  @brief
		*    Return the default blend state
		*
		*  @return
		*    The default blend state, see "Renderer::BlendState" for the default values
		*/
		static inline const BlendState& getDefaultBlendState()
		{
			// As default values, the one of Direct3D 11 and Direct 10 were chosen in order to make it easier for those renderer implementations
			// (choosing OpenGL default values would bring no benefit due to the design of the OpenGL API)
			// - Direct3D 11 "D3D11_BLEND_DESC structure"-documentation at MSDN: http://msdn.microsoft.com/en-us/library/windows/desktop/ff476087%28v=vs.85%29.aspx
			// - Direct3D 10 "D3D10_BLEND_DESC structure"-documentation at MSDN: http://msdn.microsoft.com/en-us/library/windows/desktop/bb204893%28v=vs.85%29.aspx
			// - Direct3D 9 "D3DRENDERSTATETYPE enumeration"-documentation at MSDN: http://msdn.microsoft.com/en-us/library/windows/desktop/bb172599%28v=vs.85%29.aspx
			// - OpenGL & OpenGL ES 3: The official specifications (unlike Direct3D, OpenGL versions are more compatible to each other)

			// Return default values
			// TODO(co) Finish default state comments
			static constexpr BlendState BLEND_STATE =
			{																																//	Direct3D 11	Direct3D 10	Direct3D 9			OpenGL
				false,								// alphaToCoverageEnable (int)																"false"			"false"
				false,								// independentBlendEnable (int)																"false"			"false"
			
				{ // renderTarget[8]
					// renderTarget[0]
					{ 
						false,						// blendEnable (int)																		"false"			"false"
						Blend::ONE,					// srcBlend (Renderer::Blend)																"ONE"			"ONE"
						Blend::ZERO,				// destBlend (Renderer::Blend)																"ZERO"			"ZERO"
						BlendOp::ADD,				// blendOp (Renderer::BlendOp)																"ADD"			"ADD"
						Blend::ONE,					// srcBlendAlpha (Renderer::Blend)															"ONE"			"ONE"
						Blend::ZERO,				// destBlendAlpha (Renderer::Blend)															"ZERO"			"ZERO"
						BlendOp::ADD,				// blendOpAlpha (Renderer::BlendOp)															"ADD"			"ADD"
						ColorWriteEnableFlag::ALL,	// renderTargetWriteMask (uint8_t), combination of "Renderer::ColorWriteEnableFlag"-flags	"ALL"			"ALL"
					},
					// renderTarget[1]
					{
						false,						// blendEnable (int)																		"false"			"false"
						Blend::ONE,					// srcBlend (Renderer::Blend)																"ONE"			"ONE"
						Blend::ZERO,				// destBlend (Renderer::Blend)																"ZERO"			"ZERO"
						BlendOp::ADD,				// blendOp (Renderer::BlendOp)																"ADD"			"ADD"
						Blend::ONE,					// srcBlendAlpha (Renderer::Blend)															"ONE"			"ONE"
						Blend::ZERO,				// destBlendAlpha (Renderer::Blend)															"ZERO"			"ZERO"
						BlendOp::ADD,				// blendOpAlpha (Renderer::BlendOp)															"ADD"			"ADD"
						ColorWriteEnableFlag::ALL,	// renderTargetWriteMask (uint8_t), combination of "Renderer::ColorWriteEnableFlag"-flags	"ALL"			"ALL"
					},
					// renderTarget[2]
					{
						false,						// blendEnable (int)																		"false"			"false"
						Blend::ONE,					// srcBlend (Renderer::Blend)																"ONE"			"ONE"
						Blend::ZERO,				// destBlend (Renderer::Blend)																"ZERO"			"ZERO"
						BlendOp::ADD,				// blendOp (Renderer::BlendOp)																"ADD"			"ADD"
						Blend::ONE,					// srcBlendAlpha (Renderer::Blend)															"ONE"			"ONE"
						Blend::ZERO,				// destBlendAlpha (Renderer::Blend)															"ZERO"			"ZERO"
						BlendOp::ADD,				// blendOpAlpha (Renderer::BlendOp)															"ADD"			"ADD"
						ColorWriteEnableFlag::ALL,	// renderTargetWriteMask (uint8_t), combination of "Renderer::ColorWriteEnableFlag"-flags	"ALL"			"ALL"
					},
					// renderTarget[3]
					{
						false,						// blendEnable (int)																		"false"			"false"
						Blend::ONE,					// srcBlend (Renderer::Blend)																"ONE"			"ONE"
						Blend::ZERO,				// destBlend (Renderer::Blend)																"ZERO"			"ZERO"
						BlendOp::ADD,				// blendOp (Renderer::BlendOp)																"ADD"			"ADD"
						Blend::ONE,					// srcBlendAlpha (Renderer::Blend)															"ONE"			"ONE"
						Blend::ZERO,				// destBlendAlpha (Renderer::Blend)															"ZERO"			"ZERO"
						BlendOp::ADD,				// blendOpAlpha (Renderer::BlendOp)															"ADD"			"ADD"
						ColorWriteEnableFlag::ALL,	// renderTargetWriteMask (uint8_t), combination of "Renderer::ColorWriteEnableFlag"-flags	"ALL"			"ALL"
					},
					// renderTarget[4]
					{
						false,						// blendEnable (int)																		"false"			"false"
						Blend::ONE,					// srcBlend (Renderer::Blend)																"ONE"			"ONE"
						Blend::ZERO,				// destBlend (Renderer::Blend)																"ZERO"			"ZERO"
						BlendOp::ADD,				// blendOp (Renderer::BlendOp)																"ADD"			"ADD"
						Blend::ONE,					// srcBlendAlpha (Renderer::Blend)															"ONE"			"ONE"
						Blend::ZERO,				// destBlendAlpha (Renderer::Blend)															"ZERO"			"ZERO"
						BlendOp::ADD,				// blendOpAlpha (Renderer::BlendOp)															"ADD"			"ADD"
						ColorWriteEnableFlag::ALL,	// renderTargetWriteMask (uint8_t), combination of "Renderer::ColorWriteEnableFlag"-flags	"ALL"			"ALL"
					},
					// renderTarget[5]
					{
						false,						// blendEnable (int)																		"false"			"false"
						Blend::ONE,					// srcBlend (Renderer::Blend)																"ONE"			"ONE"
						Blend::ZERO,				// destBlend (Renderer::Blend)																"ZERO"			"ZERO"
						BlendOp::ADD,				// blendOp (Renderer::BlendOp)																"ADD"			"ADD"
						Blend::ONE,					// srcBlendAlpha (Renderer::Blend)															"ONE"			"ONE"
						Blend::ZERO,				// destBlendAlpha (Renderer::Blend)															"ZERO"			"ZERO"
						BlendOp::ADD,				// blendOpAlpha (Renderer::BlendOp)															"ADD"			"ADD"
						ColorWriteEnableFlag::ALL,	// renderTargetWriteMask (uint8_t), combination of "Renderer::ColorWriteEnableFlag"-flags	"ALL"			"ALL"
					},
					// renderTarget[6]
					{
						false,						// blendEnable (int)																		"false"			"false"
						Blend::ONE,					// srcBlend (Renderer::Blend)																"ONE"			"ONE"
						Blend::ZERO,				// destBlend (Renderer::Blend)																"ZERO"			"ZERO"
						BlendOp::ADD,				// blendOp (Renderer::BlendOp)																"ADD"			"ADD"
						Blend::ONE,					// srcBlendAlpha (Renderer::Blend)															"ONE"			"ONE"
						Blend::ZERO,				// destBlendAlpha (Renderer::Blend)															"ZERO"			"ZERO"
						BlendOp::ADD,				// blendOpAlpha (Renderer::BlendOp)															"ADD"			"ADD"
						ColorWriteEnableFlag::ALL,	// renderTargetWriteMask (uint8_t), combination of "Renderer::ColorWriteEnableFlag"-flags	"ALL"			"ALL"
					},
					// renderTarget[7]
					{
						false,						// blendEnable (int)																		"false"			"false"
						Blend::ONE,					// srcBlend (Renderer::Blend)																"ONE"			"ONE"
						Blend::ZERO,				// destBlend (Renderer::Blend)																"ZERO"			"ZERO"
						BlendOp::ADD,				// blendOp (Renderer::BlendOp)																"ADD"			"ADD"
						Blend::ONE,					// srcBlendAlpha (Renderer::Blend)															"ONE"			"ONE"
						Blend::ZERO,				// destBlendAlpha (Renderer::Blend)															"ZERO"			"ZERO"
						BlendOp::ADD,				// blendOpAlpha (Renderer::BlendOp)															"ADD"			"ADD"
						ColorWriteEnableFlag::ALL,	// renderTargetWriteMask (uint8_t), combination of "Renderer::ColorWriteEnableFlag"-flags	"ALL"			"ALL"
					},
				},
			};
			return BLEND_STATE;
		}
	};




	//[-------------------------------------------------------]
	//[ Renderer/Buffer/BufferTypes.h                         ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Buffer usage indication
	*
	*  @note
	*    - Direct3D and OpenGL ES 3 have less fine granular usage settings, in this case the usage will be mapped to the closed match
	*    - Original OpenGL "GL_ARB_vertex_buffer_object" extension comments from http://www.opengl.org/registry/specs/ARB/vertex_buffer_object.txt are used in here
	*    - These constants directly map to "GL_ARB_vertex_buffer_object" and OpenGL ES 3 constants, do not change them
	*    - Most common usages: "STREAM_DRAW", "STATIC_DRAW" and "DYNAMIC_DRAW"
	*/
	enum class BufferUsage
	{
		STREAM_DRAW  = 0x88E0,	///< The data store contents will be specified once by the application, and used at most a few times as the source of a OpenGL (drawing) command. (also exists in OpenGL ES 3)
		STREAM_READ  = 0x88E1,	///< The data store contents will be specified once by reading data from the OpenGL, and queried at most a few times by the application.
		STREAM_COPY  = 0x88E2,	///< The data store contents will be specified once by reading data from the OpenGL, and used at most a few times as the source of a OpenGL (drawing) command.
		STATIC_DRAW  = 0x88E4,	///< The data store contents will be specified once by the application, and used many times as the source for OpenGL (drawing) commands. (also exists in OpenGL ES 3)
		STATIC_READ  = 0x88E5,	///< The data store contents will be specified once by reading data from the OpenGL, and queried many times by the application.
		STATIC_COPY  = 0x88E6,	///< The data store contents will be specified once by reading data from the OpenGL, and used many times as the source for OpenGL (drawing) commands.
		DYNAMIC_DRAW = 0x88E8,	///< The data store contents will be respecified repeatedly by the application, and used many times as the source for OpenGL (drawing) commands. (also exists in OpenGL ES 3)
		DYNAMIC_READ = 0x88E9,	///< The data store contents will be respecified repeatedly by reading data from the OpenGL, and queried many times by the application.
		DYNAMIC_COPY = 0x88EA	///< The data store contents will be respecified repeatedly by reading data from the OpenGL, and used many times as the source for OpenGL (drawing) commands.
	};




	//[-------------------------------------------------------]
	//[ Renderer/Buffer/VertexArrayTypes.h                    ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Vertex attribute format
	*/
	enum class VertexAttributeFormat : uint8_t
	{
		FLOAT_1			= 0,	///< Float 1 (one component per element, 32 bit floating point per component), supported by DirectX 9, DirectX 10, DirectX 11, OpenGL and OpenGL ES 3
		FLOAT_2			= 1,	///< Float 2 (two components per element, 32 bit floating point per component), supported by DirectX 9, DirectX 10, DirectX 11, OpenGL and OpenGL ES 3
		FLOAT_3			= 2,	///< Float 3 (three components per element, 32 bit floating point per component), supported by DirectX 9, DirectX 10, DirectX 11, OpenGL and OpenGL ES 3
		FLOAT_4			= 3,	///< Float 4 (four components per element, 32 bit floating point per component), supported by DirectX 9, DirectX 10, DirectX 11, OpenGL and OpenGL ES 3
		R8G8B8A8_UNORM	= 4,	///< Unsigned byte 4 (four components per element, 8 bit integer per component), will be passed in a normalized form into shaders, supported by DirectX 9, DirectX 10, DirectX 11, OpenGL and OpenGL ES 3
		R8G8B8A8_UINT	= 5,	///< Unsigned byte 4 (four components per element, 8 bit integer per component), supported by DirectX 9, DirectX 10, DirectX 11, OpenGL and OpenGL ES 3
		SHORT_2			= 6,	///< Short 2 (two components per element, 16 bit integer per component), supported by DirectX 9, DirectX 10, DirectX 11, OpenGL and OpenGL ES 3
		SHORT_4			= 7,	///< Short 4 (four components per element, 16 bit integer per component), supported by DirectX 9, DirectX 10, DirectX 11, OpenGL and OpenGL ES 3
		UINT_1			= 8		///< Unsigned integer 1 (one components per element, 32 bit unsigned integer per component), supported by DirectX 10, DirectX 11, OpenGL and OpenGL ES 3
	};
	/**
	*  @brief
	*    Vertex attribute ("Input element description" in Direct3D terminology)
	*
	*  @note
	*    - This piece of data is POD and can be serialized/deserialized as a whole (hence the byte alignment compiler setting)
	*/
	#pragma pack(push)
	#pragma pack(1)
		struct VertexAttribute final
		{
			// Data destination
			VertexAttributeFormat vertexAttributeFormat;	///< Vertex attribute format
			char				  name[32];					///< Vertex attribute name
			char				  semanticName[32];			///< Vertex attribute semantic name
			uint32_t			  semanticIndex;			///< Vertex attribute semantic index
			// Data source
			uint32_t			  inputSlot;				///< Index of the vertex input slot to use (see "Renderer::VertexArrayVertexBuffer")
			uint32_t			  alignedByteOffset;		///< Offset (in bytes) from the start of the vertex to this certain attribute
			uint32_t			  strideInBytes;			///< Specifies the size in bytes of each vertex entry
			uint32_t			  instancesPerElement;		/**< Number of instances to draw with the same data before advancing in the buffer by one element.
																 0 for no instancing meaning the data is per-vertex instead of per-instance, 1 for drawing one
																 instance with the same data, 2 for drawing two instances with the same data and so on.
																 Instanced arrays is a shader model 3 feature, only supported if "Renderer::Capabilities::instancedArrays" is true.
																 In order to support Direct3D 9, do not use this within the first attribute. */
		};
	#pragma pack(pop)

	/**
	*  @brief
	*    Vertex attributes ("vertex declaration" in Direct3D 9 terminology, "input layout" in Direct3D 10 & 11 terminology)
	*
	*  @see
	*    - "Renderer::IVertexArray" class documentation
	*/
	#pragma pack(push)
	#pragma pack(1)
		struct VertexAttributes final
		{
			// Public data
				  uint32_t		   numberOfAttributes;	///< Number of attributes (position, color, texture coordinate, normal...), having zero attributes is valid
			const VertexAttribute* attributes;			///< At least "numberOfAttributes" instances of vertex array attributes, can be a null pointer in case there are zero attributes, the data is internally copied and you have to free your memory if you no longer need it

			// Public methods
			inline VertexAttributes() :
				numberOfAttributes(0),
				attributes(nullptr)
			{}
			inline VertexAttributes(uint32_t _numberOfAttributes, const VertexAttribute* _attributes) :
				numberOfAttributes(_numberOfAttributes),
				attributes(_attributes)
			{}
		};
	#pragma pack(pop)

	/**
	*  @brief
	*    Vertex array vertex buffer
	*
	*  @see
	*    - "Renderer::IVertexArray" class documentation
	*/
	struct VertexArrayVertexBuffer final
	{
		IVertexBuffer* vertexBuffer;	///< Vertex buffer used at this vertex input slot (vertex array instances keep a reference to the vertex buffers used by the vertex array attributes, see "Renderer::IRenderer::createVertexArray()" for details)
	};




	//[-------------------------------------------------------]
	//[ Renderer/Buffer/IndexBufferTypes.h                    ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Index buffer data format
	*/
	struct IndexBufferFormat final
	{
		enum Enum
		{
			UNSIGNED_CHAR  = 0,	///< One byte per element, uint8_t (may not be supported by each API, primarily for mobile devices)
			UNSIGNED_SHORT = 1,	///< Two bytes per element, uint16_t (best support across multiple renderer APIs)
			UNSIGNED_INT   = 2	///< Four bytes per element, uint32_t (may not be supported by each API)
		};

		/**
		*  @brief
		*    "Renderer::IndexBufferFormat" to number of bytes per element
		*
		*  @param[in] indexFormat
		*    "Renderer::IndexBufferFormat" to map
		*
		*  @return
		*    Number of bytes per element
		*/
		static inline uint32_t getNumberOfBytesPerElement(Enum indexFormat)
		{
			static constexpr uint32_t MAPPING[] =
			{
				1,	// One byte per element, uint8_t (may not be supported by each API, primarily for mobile devices)
				2,	// Two bytes per element, uint16_t (best support across multiple renderer APIs)
				4	// Four bytes per element, uint32_t (may not be supported by each API)
			};
			return MAPPING[indexFormat];
		}
	};




	//[-------------------------------------------------------]
	//[ Renderer/Buffer/IndirectBufferTypes.h                 ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Draw instanced arguments
	*
	*  @note
	*    - Draw indirect counterpart of "Renderer::IRenderer::drawInstanced()"-method stored inside an "Renderer::IIndirectBuffer"-instance
	*    - This structure directly maps to Direct3D 12, DirectX 11, Vulkan, Metal and OpenGL, do not change it
	*    - Direct3D 12: "D3D12_DRAW_ARGUMENTS"
	*    - Direct3D 11: No structure documentation found, only indications that same arguments and order as "ID3D11DeviceContext::DrawInstanced()"
	*    - Vulkan: "VkDrawIndirectCommand"
	*    - Metal: "MTLDrawPrimitivesIndirectArguments"
	*    - OpenGL:"DrawArraysIndirectCommand"
	*
	*  @see
	*    - "D3D12_DRAW_ARGUMENTS"-documentation for details
	*/
	struct DrawInstancedArguments final
	{
		uint32_t vertexCountPerInstance;
		uint32_t instanceCount;
		uint32_t startVertexLocation;
		uint32_t startInstanceLocation;
		inline DrawInstancedArguments(uint32_t _vertexCountPerInstance, uint32_t _instanceCount = 1, uint32_t _startVertexLocation = 0, uint32_t _startInstanceLocation = 0) :
			vertexCountPerInstance(_vertexCountPerInstance),
			instanceCount(_instanceCount),
			startVertexLocation(_startVertexLocation),
			startInstanceLocation(_startInstanceLocation)
		{}
	};

	/**
	*  @brief
	*    Draw indexed instanced arguments
	*
	*  @note
	*    - Draw indirect counterpart of "Renderer::IRenderer::drawIndexedInstanced()"-method stored inside an "Renderer::IIndirectBuffer"-instance
	*    - This structure directly maps to Direct3D 12, DirectX 11, Vulkan, Metal and OpenGL, do not change it
	*    - Direct3D 12: "D3D12_DRAW_INDEXED_ARGUMENTS"
	*    - Direct3D 11: No structure documentation found, only indications that same arguments and order as "ID3D11DeviceContext::DrawIndexedInstanced()"
	*    - Vulkan: "VkDrawIndexedIndirectCommand"
	*    - Metal: "MTLDrawIndexedPrimitivesIndirectArguments"
	*    - OpenGL:"DrawElementsIndirectCommand"
	*
	*  @see
	*    - "D3D12_DRAW_INDEXED_ARGUMENTS"-documentation for details
	*/
	struct DrawIndexedInstancedArguments final
	{
		uint32_t indexCountPerInstance;
		uint32_t instanceCount;
		uint32_t startIndexLocation;
		int32_t  baseVertexLocation;
		uint32_t startInstanceLocation;
		inline DrawIndexedInstancedArguments(uint32_t _indexCountPerInstance, uint32_t _instanceCount = 1, uint32_t _startIndexLocation = 0, int32_t _baseVertexLocation = 0, uint32_t _startInstanceLocation = 0) :
			indexCountPerInstance(_indexCountPerInstance),
			instanceCount(_instanceCount),
			startIndexLocation(_startIndexLocation),
			baseVertexLocation(_baseVertexLocation),
			startInstanceLocation(_startInstanceLocation)
		{}
	};




	//[-------------------------------------------------------]
	//[ Renderer/State/RasterizerStateTypes.h                 ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Fill mode
	*
	*  @note
	*    - These constants directly map to Direct3D 10 & 11 & 12 constants, do not change them
	*
	*  @see
	*    - "D3D12_FILL_MODE"-documentation for details
	*/
	enum class FillMode
	{
		WIREFRAME = 2,	///< Wireframe
		SOLID     = 3	///< Solid
	};

	/**
	*  @brief
	*    Cull mode
	*
	*  @note
	*    - These constants directly map to Direct3D 10 & 11 & 12 constants, do not change them
	*
	*  @see
	*    - "D3D12_CULL_MODE"-documentation for details
	*/
	enum class CullMode
	{
		NONE  = 1,	///< No culling
		FRONT = 2,	///< Do not draw triangles that are front-facing
		BACK  = 3	///< Do not draw triangles that are back-facing
	};

	/**
	*  @brief
	*    Conservative rasterization mode
	*
	*  @note
	*    - These constants directly map to Direct3D 12 constants, do not change them
	*
	*  @see
	*    - "D3D12_CONSERVATIVE_RASTERIZATION_MODE"-documentation for details
	*/
	enum class ConservativeRasterizationMode
	{
		OFF	= 0,	///< Conservative rasterization is off
		ON	= 1		///< Conservative rasterization is on
	};

	/**
	*  @brief
	*    Rasterizer state
	*
	*  @note
	*    - This rasterizer state maps directly to Direct3D 10 & 11 & 12, do not change it
	*    - This also means that "int" is used over "bool" because in Direct3D it's defined this way
	*    - If you want to know how the default values were chosen, have a look into the "Renderer::RasterizerStateBuilder::getDefaultRasterizerState()"-implementation
	*    - Lookout! In Direct3D 12 the scissor test can't be deactivated and hence one always needs to set a valid scissor rectangle.
	*      Use the convenience "Renderer::Command::SetViewportAndScissorRectangle"-command if possible to not walk into this Direct3D 12 trap.
	*
	*  @see
	*    - "D3D12_RASTERIZER_DESC"-documentation for details
	*/
	struct RasterizerState final
	{
		FillMode						fillMode;						///< Default: "Renderer::FillMode::SOLID"
		CullMode						cullMode;						///< Default: "Renderer::CullMode::BACK"
		int								frontCounterClockwise;			///< Select counter-clockwise polygons as front-facing? Boolean value. Default: "false"
		int								depthBias;						///< Default: "0"
		float							depthBiasClamp;					///< Default: "0.0f"
		float							slopeScaledDepthBias;			///< Default: "0.0f"
		int								depthClipEnable;				///< Boolean value. Default: "true"
		int								multisampleEnable;				///< Boolean value. Default: "false"
		int								antialiasedLineEnable;			///< Boolean value. Default: "false"
		unsigned int					forcedSampleCount;				///< Default: "0"
		ConservativeRasterizationMode	conservativeRasterizationMode;	///< Boolean value. >= Direct3D 12 only. Default: "false"
		int								scissorEnable;					///< Boolean value. Not available in Vulkan or Direct3D 12 (scissor testing is always enabled). Default: "false"
	};
	struct RasterizerStateBuilder final
	{
		/**
		*  @brief
		*    Return the default rasterizer state
		*
		*  @return
		*    The default rasterizer state, see "Renderer::RasterizerState" for the default values
		*/
		static inline const RasterizerState& getDefaultRasterizerState()
		{
			// As default values, the one of Direct3D 11 and Direct 10 were chosen in order to make it easier for those renderer implementations
			// (choosing OpenGL default values would bring no benefit due to the design of the OpenGL API)
			// - Direct3D 12 "D3D12_RASTERIZER_DESC structure"-documentation at MSDN: https://msdn.microsoft.com/de-de/library/windows/desktop/dn770387%28v=vs.85%29.aspx
			// - Direct3D 11 "D3D11_RASTERIZER_DESC structure"-documentation at MSDN: http://msdn.microsoft.com/en-us/library/windows/desktop/ff476198%28v=vs.85%29.aspx
			// - Direct3D 10 "D3D10_RASTERIZER_DESC structure"-documentation at MSDN: http://msdn.microsoft.com/en-us/library/windows/desktop/bb172408(v=vs.85).aspx
			// - Direct3D 9 "D3DRENDERSTATETYPE enumeration"-documentation at MSDN: http://msdn.microsoft.com/en-us/library/windows/desktop/bb172599%28v=vs.85%29.aspx
			// - OpenGL & OpenGL ES 3: The official specifications (unlike Direct3D, OpenGL versions are more compatible to each other)

			// Return default values
			static constexpr RasterizerState RASTERIZER_STATE =
			{																														//	Direct3D 11	Direct3D 10	Direct3D 9		OpenGL
				FillMode::SOLID,					// fillMode (Renderer::FillMode)												"SOLID"			"SOLID"		"SOLID"			"SOLID"
				CullMode::BACK,						// cullMode (Renderer::CullMode)												"BACK"			"Back"		"BACK" (CCW)	"BACK"
				false,								// frontCounterClockwise (int)													"false"			"false"		"false" (CCW)	"true"
				0,									// depthBias (int)																"0"				"0"			"0"				TODO(co)
				0.0f,								// depthBiasClamp (float)														"0.0f"			"0.0f"		<unsupported>	TODO(co)
				0.0f,								// slopeScaledDepthBias (float)													"0.0f"			"0.0f"		"0.0f"			TODO(co)
				true,								// depthClipEnable (int)														"true"			"true"		<unsupported>	TODO(co)
				false,								// multisampleEnable (int)														"false"			"false"		"true"			"true"
				false,								// antialiasedLineEnable (int)													"false"			"false"		"false"			"false"
				0,									// forcedSampleCount (unsigned int)
				ConservativeRasterizationMode::OFF,	// conservativeRasterizationMode (Renderer::ConservativeRasterizationMode)
				false								// scissorEnable (int)															"false"			"false"		"false"			"false"
			};
			return RASTERIZER_STATE;
		}
	};




	//[-------------------------------------------------------]
	//[ Renderer/State/DepthStencilStateTypes.h               ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Depth write mask
	*
	*  @note
	*    - These constants directly map to Direct3D 10 & 11 & 12 constants, do not change them
	*
	*  @see
	*    - "D3D12_DEPTH_WRITE_MASK"-documentation for details
	*/
	enum class DepthWriteMask
	{
		ZERO = 0,
		ALL  = 1
	};

	/**
	*  @brief
	*    Stencil operation
	*
	*  @note
	*    - These constants directly map to Direct3D 10 & 11 & 12 constants, do not change them
	*
	*  @see
	*    - "D3D12_STENCIL_OP"-documentation for details
	*/
	enum class StencilOp
	{
		KEEP	 = 1,
		ZERO	 = 2,
		REPLACE	 = 3,
		INCR_SAT = 4,
		DECR_SAT = 5,
		INVERT	 = 6,
		INCREASE = 7,
		DECREASE = 8
	};

	/**
	*  @brief
	*    Depth stencil operation description
	*
	*  @note
	*    - This depth stencil operation description maps directly to Direct3D 10 & 11 & 12, do not change it
	*    - If you want to know how the default values were chosen, have a look into the "Renderer::DepthStencilStateBuilder::getDefaultDepthStencilState()"-implementation
	*
	*  @see
	*    - "D3D12_DEPTH_STENCILOP_DESC"-documentation for details
	*/
	struct DepthStencilOpDesc final
	{
		StencilOp		stencilFailOp;		///< Default: "Renderer::StencilOp::KEEP"
		StencilOp		stencilDepthFailOp;	///< Default: "Renderer::StencilOp::KEEP"
		StencilOp		stencilPassOp;		///< Default: "Renderer::StencilOp::KEEP"
		ComparisonFunc	stencilFunc;		///< Default: "Renderer::ComparisonFunc::ALWAYS"
	};

	/**
	*  @brief
	*    Depth stencil state
	*
	*  @note
	*    - This depth stencil state maps directly to Direct3D 10 & 11 & 12, do not change it
	*    - This also means that "int" is used over "bool" because in Direct3D it's defined this way
	*    - If you want to know how the default values were chosen, have a look into the "Renderer::DepthStencilStateBuilder::getDefaultDepthStencilState()"-implementation
	*
	*  @see
	*    - "D3D12_DEPTH_STENCIL_DESC"-documentation for details
	*/
	struct DepthStencilState final
	{
		int					depthEnable;		///< Boolean value. Default: "true"
		DepthWriteMask		depthWriteMask;		///< Default: "Renderer::DepthWriteMask::ALL"
		ComparisonFunc		depthFunc;			///< Default: "Renderer::ComparisonFunc::GREATER" instead of "Renderer::ComparisonFunc::LESS" due to usage of Reversed-Z (see e.g. https://developer.nvidia.com/content/depth-precision-visualized and https://nlguillemot.wordpress.com/2016/12/07/reversed-z-in-opengl/)
		int					stencilEnable;		///< Boolean value. Default: "false"
		uint8_t				stencilReadMask;	///< Default: "0xff"
		uint8_t				stencilWriteMask;	///< Default: "0xff"
		DepthStencilOpDesc	frontFace;			///< Default: See "Renderer::DepthStencilOpDesc"
		DepthStencilOpDesc	backFace;			///< Default: See "Renderer::DepthStencilOpDesc"
	};
	struct DepthStencilStateBuilder final
	{
		/**
		*  @brief
		*    Return the default depth stencil state
		*
		*  @return
		*    The default depth stencil state, see "Renderer::DepthStencilState" for the default values
		*/
		static inline const DepthStencilState& getDefaultDepthStencilState()
		{
			// As default values, the one of Direct3D 11 and Direct 10 were chosen in order to make it easier for those renderer implementations
			// (choosing OpenGL default values would bring no benefit due to the design of the OpenGL API)
			// - Direct3D 11 "D3D11_DEPTH_STENCIL_DESC structure"-documentation at MSDN: http://msdn.microsoft.com/en-us/library/windows/desktop/ff476110%28v=vs.85%29.aspx
			// - Direct3D 10 "D3D10_DEPTH_STENCIL_DESC structure"-documentation at MSDN: http://msdn.microsoft.com/en-us/library/windows/desktop/bb205036%28v=vs.85%29.aspx
			// - Direct3D 9 "D3DRENDERSTATETYPE enumeration"-documentation at MSDN: http://msdn.microsoft.com/en-us/library/windows/desktop/bb172599%28v=vs.85%29.aspx
			// - OpenGL & OpenGL ES 3: The official specifications (unlike Direct3D, OpenGL versions are more compatible to each other)

			// Return default values
			static constexpr DepthStencilState DEPTH_STENCIL_STATE =
			{																				//	Direct3D 11		Direct3D 10	Direct3D 9				OpenGL
				true,						// depthEnable (int)							"true"			"true"		"true"					TODO(co)
				DepthWriteMask::ALL,		// depthWriteMask (Renderer::DepthWriteMask)	"ALL"			"ALL"		"ALL"					TODO(co)
				ComparisonFunc::GREATER,	// depthFunc (Renderer::ComparisonFunc)			"LESS"			"LESS"		"LESS_EQUAL"			TODO(co)	- Default: "Renderer::ComparisonFunc::GREATER" instead of "Renderer::ComparisonFunc::LESS" due to usage of Reversed-Z (see e.g. https://developer.nvidia.com/content/depth-precision-visualized and https://nlguillemot.wordpress.com/2016/12/07/reversed-z-in-opengl/)
				false,						// stencilEnable (int)							"false"			"false"		"false"					TODO(co)
				0xff,						// stencilReadMask (uint8_t)					"0xff"			"0xff"		"0xffffffff"			TODO(co)
				0xff,						// stencilWriteMask (uint8_t)					"0xff"			"0xff"		"0xffffffff"			TODO(co)
				{ // sFrontFace (Renderer::DepthStencilOpDesc)
					StencilOp::KEEP,		// stencilFailOp (Renderer::StencilOp			"KEEP"			"KEEP"		"KEEP" (both sides)		TODO(co)
					StencilOp::KEEP,		// stencilDepthFailOp (Renderer::StencilOp)		"KEEP"			"KEEP"		"KEEP" (both sides)		TODO(co)
					StencilOp::KEEP,		// stencilPassOp (Renderer::StencilOp)			"KEEP"			"KEEP"		"KEEP" (both sides)		TODO(co)
					ComparisonFunc::ALWAYS	// stencilFunc (Renderer::ComparisonFunc)		"ALWAYS"		"ALWAYS"	"ALWAYS" (both sides)
				},
				{ // sBackFace (Renderer::DepthStencilOpDesc)
					StencilOp::KEEP,		// stencilFailOp (Renderer::StencilOp)			"KEEP"			"KEEP"		"KEEP" (both sides)		TODO(co)
					StencilOp::KEEP,		// stencilDepthFailOp (Renderer::StencilOp)		"KEEP"			"KEEP"		"KEEP" (both sides)		TODO(co)
					StencilOp::KEEP,		// stencilPassOp (Renderer::StencilOp)			"KEEP"			"KEEP"		"KEEP" (both sides)		TODO(co)
					ComparisonFunc::ALWAYS	// stencilFunc (Renderer::ComparisonFunc)		"ALWAYS"		"ALWAYS"	"ALWAYS" (both sides)	TODO(co)
				}
			};
			return DEPTH_STENCIL_STATE;
		}
	};




	//[-------------------------------------------------------]
	//[ Renderer/State/PipelineStateTypes.h                   ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Input-assembler (IA) stage: Primitive topology types
	*
	*  @note
	*    - These constants directly map to Direct3D 9 & 10 & 11 constants, do not change them
	*/
	enum class PrimitiveTopology
	{
		UNKNOWN        = 0,		///< Unknown primitive type
		POINT_LIST     = 1,		///< Point list, use "PATCH_LIST_1" for tessellation
		LINE_LIST      = 2,		///< Line list, use "PATCH_LIST_2" for tessellation
		LINE_STRIP     = 3,		///< Line strip
		TRIANGLE_LIST  = 4,		///< Triangle list, use "PATCH_LIST_3" for tessellation
		TRIANGLE_STRIP = 5,		///< Triangle strip
		PATCH_LIST_1   = 33,	///< Patch list with 1 vertex per patch (tessellation relevant topology type) - "POINT_LIST" used for tessellation
		PATCH_LIST_2   = 34,	///< Patch list with 2 vertices per patch (tessellation relevant topology type) - "LINE_LIST" used for tessellation
		PATCH_LIST_3   = 35,	///< Patch list with 3 vertices per patch (tessellation relevant topology type) - "TRIANGLE_LIST" used for tessellation
		PATCH_LIST_4   = 36,	///< Patch list with 4 vertices per patch (tessellation relevant topology type)
		PATCH_LIST_5   = 37,	///< Patch list with 5 vertices per patch (tessellation relevant topology type)
		PATCH_LIST_6   = 38,	///< Patch list with 6 vertices per patch (tessellation relevant topology type)
		PATCH_LIST_7   = 39,	///< Patch list with 7 vertices per patch (tessellation relevant topology type)
		PATCH_LIST_8   = 40,	///< Patch list with 8 vertices per patch (tessellation relevant topology type)
		PATCH_LIST_9   = 41,	///< Patch list with 9 vertices per patch (tessellation relevant topology type)
		PATCH_LIST_10  = 42,	///< Patch list with 10 vertices per patch (tessellation relevant topology type)
		PATCH_LIST_11  = 43,	///< Patch list with 11 vertices per patch (tessellation relevant topology type)
		PATCH_LIST_12  = 44,	///< Patch list with 12 vertices per patch (tessellation relevant topology type)
		PATCH_LIST_13  = 45,	///< Patch list with 13 vertices per patch (tessellation relevant topology type)
		PATCH_LIST_14  = 46,	///< Patch list with 14 vertices per patch (tessellation relevant topology type)
		PATCH_LIST_15  = 47,	///< Patch list with 15 vertices per patch (tessellation relevant topology type)
		PATCH_LIST_16  = 48,	///< Patch list with 16 vertices per patch (tessellation relevant topology type)
		PATCH_LIST_17  = 49,	///< Patch list with 17 vertices per patch (tessellation relevant topology type)
		PATCH_LIST_18  = 50,	///< Patch list with 18 vertices per patch (tessellation relevant topology type)
		PATCH_LIST_19  = 51,	///< Patch list with 19 vertices per patch (tessellation relevant topology type)
		PATCH_LIST_20  = 52,	///< Patch list with 20 vertices per patch (tessellation relevant topology type)
		PATCH_LIST_21  = 53,	///< Patch list with 21 vertices per patch (tessellation relevant topology type)
		PATCH_LIST_22  = 54,	///< Patch list with 22 vertices per patch (tessellation relevant topology type)
		PATCH_LIST_23  = 55,	///< Patch list with 23 vertices per patch (tessellation relevant topology type)
		PATCH_LIST_24  = 56,	///< Patch list with 24 vertices per patch (tessellation relevant topology type)
		PATCH_LIST_25  = 57,	///< Patch list with 25 vertices per patch (tessellation relevant topology type)
		PATCH_LIST_26  = 58,	///< Patch list with 26 vertices per patch (tessellation relevant topology type)
		PATCH_LIST_27  = 59,	///< Patch list with 27 vertices per patch (tessellation relevant topology type)
		PATCH_LIST_28  = 60,	///< Patch list with 28 vertices per patch (tessellation relevant topology type)
		PATCH_LIST_29  = 61,	///< Patch list with 29 vertices per patch (tessellation relevant topology type)
		PATCH_LIST_30  = 62,	///< Patch list with 30 vertices per patch (tessellation relevant topology type)
		PATCH_LIST_31  = 63,	///< Patch list with 31 vertices per patch (tessellation relevant topology type)
		PATCH_LIST_32  = 64		///< Patch list with 32 vertices per patch (tessellation relevant topology type)
	};

	/**
	*  @brief
	*    Primitive topology type specifying how the pipeline interprets geometry or hull shader input primitives
	*
	*  @note
	*    - These constants directly map to Direct3D 12 constants, do not change them
	*
	*  @see
	*    - "D3D12_PRIMITIVE_TOPOLOGY_TYPE"-documentation for details
	*/
	enum class PrimitiveTopologyType
	{
		UNDEFINED	= 0,	///< The shader has not been initialized with an input primitive type
		POINT		= 1,	///< Interpret the input primitive as a point
		LINE		= 2,	///< Interpret the input primitive as a line
		TRIANGLE	= 3,	///< Interpret the input primitive as a triangle
		PATCH		= 4		///< Interpret the input primitive as a control point patch
	};

	/**
	*  @brief
	*    Pipeline state
	*/
	struct SerializedPipelineState
	{
		PrimitiveTopology	  primitiveTopology;			///< Input-assembler (IA) stage: Primitive topology used for draw calls
		PrimitiveTopologyType primitiveTopologyType;		///< The primitive topology type specifies how the pipeline interprets geometry or hull shader input primitives
		RasterizerState		  rasterizerState;				///< Rasterizer state
		DepthStencilState	  depthStencilState;			///< Depth stencil state
		BlendState			  blendState;					///< Blend state
		uint32_t			  numberOfRenderTargets;		///< Number of render targets
		TextureFormat::Enum	  renderTargetViewFormats[8];	///< Render target view formats
		TextureFormat::Enum	  depthStencilViewFormat;		///< Depth stencil view formats
	};
	struct PipelineState : public SerializedPipelineState
	{
		IRootSignature*  rootSignature;		///< Root signature (pipeline state instances keep a reference to the root signature), must be valid
		IProgram*		 program;			///< Program used by the pipeline state (pipeline state instances keep a reference to the program), must be valid
		VertexAttributes vertexAttributes;	///< Vertex attributes
		IRenderPass*	 renderPass;		///< Render pass, the pipeline state keeps a reference
	};
	struct PipelineStateBuilder final : public PipelineState
	{
		inline PipelineStateBuilder()
		{
			// "PipelineState"-part
			rootSignature						= nullptr;
			program								= nullptr;
			vertexAttributes.numberOfAttributes	= 0;
			vertexAttributes.attributes			= nullptr;
			renderPass							= nullptr;

			// "SerializedPipelineState"-part
			primitiveTopology					= PrimitiveTopology::TRIANGLE_LIST;
			primitiveTopologyType				= PrimitiveTopologyType::TRIANGLE;
			rasterizerState						= RasterizerStateBuilder::getDefaultRasterizerState();
			depthStencilState					= DepthStencilStateBuilder::getDefaultDepthStencilState();
			blendState							= BlendStateBuilder::getDefaultBlendState();
			numberOfRenderTargets				= 1;
			renderTargetViewFormats[0]			= TextureFormat::R8G8B8A8;
			renderTargetViewFormats[1]			= TextureFormat::R8G8B8A8;
			renderTargetViewFormats[2]			= TextureFormat::R8G8B8A8;
			renderTargetViewFormats[3]			= TextureFormat::R8G8B8A8;
			renderTargetViewFormats[4]			= TextureFormat::R8G8B8A8;
			renderTargetViewFormats[5]			= TextureFormat::R8G8B8A8;
			renderTargetViewFormats[6]			= TextureFormat::R8G8B8A8;
			renderTargetViewFormats[7]			= TextureFormat::R8G8B8A8;
			depthStencilViewFormat				= TextureFormat::D32_FLOAT;
		}

		inline PipelineStateBuilder(IRootSignature* _rootSignature, IProgram* _program, const VertexAttributes& _vertexAttributes, IRenderPass& _renderPass)
		{
			// "PipelineState"-part
			rootSignature				= _rootSignature;
			program						= _program;
			vertexAttributes			= _vertexAttributes;
			renderPass					= &_renderPass;

			// "SerializedPipelineState"-part
			primitiveTopology			= PrimitiveTopology::TRIANGLE_LIST;
			primitiveTopologyType		= PrimitiveTopologyType::TRIANGLE;
			rasterizerState				= RasterizerStateBuilder::getDefaultRasterizerState();
			depthStencilState			= DepthStencilStateBuilder::getDefaultDepthStencilState();
			blendState					= BlendStateBuilder::getDefaultBlendState();
			numberOfRenderTargets		= 1;
			renderTargetViewFormats[0]	= TextureFormat::R8G8B8A8;
			renderTargetViewFormats[1]	= TextureFormat::R8G8B8A8;
			renderTargetViewFormats[2]	= TextureFormat::R8G8B8A8;
			renderTargetViewFormats[3]	= TextureFormat::R8G8B8A8;
			renderTargetViewFormats[4]	= TextureFormat::R8G8B8A8;
			renderTargetViewFormats[5]	= TextureFormat::R8G8B8A8;
			renderTargetViewFormats[6]	= TextureFormat::R8G8B8A8;
			renderTargetViewFormats[7]	= TextureFormat::R8G8B8A8;
			depthStencilViewFormat		= TextureFormat::D32_FLOAT;
		}
	};




	//[-------------------------------------------------------]
	//[ Renderer/Shader/ShaderTypes.h                         ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Geometry shader (GS) input primitive topology
	*
	*  @note
	*    - These constants directly map to OpenGL constants, do not change them
	*/
	enum class GsInputPrimitiveTopology
	{
		POINTS			    = 0x0000,	///< List of point primitives
		LINES			    = 0x0001,	///< List of line or line strip primitives
		LINES_ADJACENCY	    = 0x000A,	///< List of line with adjacency or line strip with adjacency primitives
		TRIANGLES		    = 0x0004,	///< List of triangle or triangle strip primitives
		TRIANGLES_ADJACENCY = 0x000C	///< List of triangle with adjacency or triangle strip with adjacency primitives
	};

	/**
	*  @brief
	*    Geometry shader (GS) primitive topology
	*
	*  @note
	*    - These constants directly map to OpenGL constants, do not change them
	*/
	enum class GsOutputPrimitiveTopology
	{
		POINTS			=  0x0000,	///< A list of point primitives
		LINES			=  0x0001,	///< A list of line primitives
		TRIANGLES_STRIP	=  0x0005	///< A triangle strip primitives
	};

	/**
	*  @brief
	*    Shader bytecode (aka shader microcode, binary large object (BLOB))
	*/
	class ShaderBytecode final
	{

	// Public methods
	public:
		inline ShaderBytecode() :
			mNumberOfBytes(0),
			mBytecode(nullptr)
		{}

		inline ~ShaderBytecode()
		{
			delete [] mBytecode;
		}

		inline uint32_t getNumberOfBytes() const
		{
			return mNumberOfBytes;
		}

		inline const uint8_t* getBytecode() const
		{
			return mBytecode;
		}

		inline void setBytecodeCopy(uint32_t numberOfBytes, uint8_t* bytecode)
		{
			delete [] mBytecode;
			mNumberOfBytes = numberOfBytes;
			mBytecode = new uint8_t[mNumberOfBytes];
			memcpy(mBytecode, bytecode, mNumberOfBytes);
		}

	// Private data
	private:
		uint32_t mNumberOfBytes;	///< Number of bytes in the bytecode
		uint8_t* mBytecode;			///< Shader bytecode, can be a null pointer

	};

	/**
	*  @brief
	*    Shader source code
	*/
	struct ShaderSourceCode final
	{
		// Public data
		const char* sourceCode	= nullptr;	///< Shader ASCII source code, if null pointer or empty string a null pointer will be returned
		const char* profile		= nullptr;	///< Shader ASCII profile to use, if null pointer or empty string, a default profile will be used which usually tries to use the best available profile that runs on most hardware (Examples: "glslf", "arbfp1", "ps_3_0")
		const char* arguments	= nullptr;	///< Optional shader compiler ASCII arguments, can be a null pointer or empty string
		const char* entry		= nullptr;	///< ASCII entry point, if null pointer or empty string, "main" is used as default

		// Public methods
		inline ShaderSourceCode(const char* _sourceCode) :
			sourceCode(_sourceCode)
		{}
	};




	//[-------------------------------------------------------]
	//[ Renderer/RefCount.h                                   ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Reference counter template
	*
	*  @note
	*    - Initially the reference counter is 0
	*/
	template <class AType>
	class RefCount
	{

	// Public methods
	public:
		/**
		*  @brief
		*    Default constructor
		*/
		inline RefCount() :
			mRefCount(0)
		{}

		/**
		*  @brief
		*    Destructor
		*/
		inline virtual ~RefCount()
		{}

		/**
		*  @brief
		*    Get a pointer to the object
		*
		*  @return
		*    Pointer to the reference counter's object, never a null pointer!
		*/
		inline const AType* getPointer() const
		{
			return static_cast<const AType*>(this);
		}

		/**
		*  @brief
		*    Get a pointer to the object
		*
		*  @return
		*    Pointer to the reference counter's object, never a null pointer!
		*/
		inline AType* getPointer()
		{
			return static_cast<AType*>(this);
		}

		/**
		*  @brief
		*    Increases the reference count
		*
		*  @return
		*    Current reference count
		*/
		inline uint32_t addReference()
		{
			// Increment reference count
			++mRefCount;

			// Return current reference count
			return mRefCount;
		}

		/**
		*  @brief
		*    Decreases the reference count
		*
		*  @return
		*    Current reference count
		*
		*  @note
		*    - When the last reference was released, the instance is destroyed automatically
		*/
		inline uint32_t releaseReference()
		{
			// Decrement reference count
			if (mRefCount > 1)
			{
				--mRefCount;

				// Return current reference count
				return mRefCount;
			}

			// Destroy object when no references are left
			else
			{
				selfDestruct();

				// This object is no longer
				return 0;
			}
		}

		/**
		*  @brief
		*    Gets the current reference count
		*
		*  @return
		*    Current reference count
		*/
		inline uint32_t getRefCount() const
		{
			// Return current reference count
			return mRefCount;
		}

	// Protected virtual Renderer::RefCount methods
	protected:
		/**
		*  @brief
		*    Destroy the instance
		*/
		virtual void selfDestruct() = 0;

	// Private data
	private:
		uint32_t mRefCount; ///< Reference count

	};




	//[-------------------------------------------------------]
	//[ Renderer/SmartRefCount.h                              ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Smart reference counter template
	*/
	template <class AType>
	class SmartRefCount
	{

	// Public methods
	public:
		/**
		*  @brief
		*    Default constructor
		*/
		inline SmartRefCount() :
			mPtr(nullptr)
		{}

		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] ptr
		*    Direct pointer to initialize with, can be a null pointer
		*/
		inline explicit SmartRefCount(AType* ptr) :
			mPtr(nullptr)
		{
			setPtr(ptr);
		}

		/**
		*  @brief
		*    Copy constructor
		*
		*  @param[in] ptr
		*    Smart pointer to initialize with
		*/
		inline SmartRefCount(const SmartRefCount<AType>& ptr) :
			mPtr(nullptr)
		{
			setPtr(ptr.getPtr());
		}

		/**
		*  @brief
		*    Destructor
		*/
		inline ~SmartRefCount()
		{
			setPtr(nullptr);
		}

		/**
		*  @brief
		*    Assign a pointer
		*
		*  @param[in] ptr
		*    Direct pointer to assign, can be a null pointer
		*
		*  @return
		*    Reference to the smart pointer
		*/
		inline SmartRefCount<AType>& operator =(AType* ptr)
		{
			if (getPointer() != ptr)
			{
				setPtr(ptr);
			}
			return *this;
		}

		/**
		*  @brief
		*    Assign a smart pointer
		*
		*  @param[in] ptr
		*    Smart pointer to assign
		*
		*  @return
		*    Reference to the smart pointer
		*/
		inline SmartRefCount<AType>& operator =(const SmartRefCount<AType>& ptr)
		{
			if (getPointer() != ptr.getPointer())
			{
				setPtr(ptr.getPtr());
			}
			return *this;
		}

		/**
		*  @brief
		*    Get a direct pointer to the object
		*
		*  @return
		*    Pointer to the object, can be a null pointer
		*/
		inline AType* getPointer() const
		{
			return (nullptr != mPtr) ? static_cast<AType*>(mPtr->getPointer()) : nullptr;
		}

		/**
		*  @brief
		*    Get a pointer to access the object
		*
		*  @return
		*    Pointer to the object, can be a null pointer
		*/
		inline AType* operator ->() const
		{
			return getPointer();
		}

		/**
		*  @brief
		*    Cast to a pointer to the object
		*
		*  @return
		*    Pointer to the object, can be a null pointer
		*/
		inline operator AType*() const
		{
			return getPointer();
		}

		/**
		*  @brief
		*    Check if the pointer is not a null pointer
		*
		*  @return
		*    "true" if the pointer is not a null pointer
		*/
		inline bool operator !() const
		{
			return (nullptr == getPointer());
		}

		/**
		*  @brief
		*    Check for equality
		*
		*  @param[in] ptr
		*    Direct pointer to compare with, can be a null pointer
		*
		*  @return
		*    "true" if the two pointers are equal
		*/
		inline bool operator ==(AType* ptr) const
		{
			return (getPointer() == ptr);
		}

		/**
		*  @brief
		*    Check for equality
		*
		*  @param[in] ptr
		*    Smart pointer to compare with
		*
		*  @return
		*    "true" if the two pointers are equal
		*/
		inline bool operator ==(const SmartRefCount<AType>& ptr) const
		{
			return (getPointer() == ptr.getPointer());
		}

		/**
		*  @brief
		*    Check for equality
		*
		*  @param[in] ptr
		*    Direct pointer to compare with, can be a null pointer
		*
		*  @return
		*    "true" if the two pointers are not equal
		*/
		inline bool operator !=(AType* ptr) const
		{
			return (getPointer() != ptr);
		}

		/**
		*  @brief
		*    Check for equality
		*
		*  @param[in] ptr
		*    Smart pointer to compare with
		*
		*  @return
		*    "true" if the two pointers are not equal
		*/
		inline bool operator !=(const SmartRefCount<AType>& ptr) const
		{
			return (getPointer() != ptr.getPointer());
		}

	// Private methods
	private:
		/**
		*  @brief
		*    Assign a pointer to an object that implements RefCount
		*
		*  @param[in] ptr
		*    Pointer to assign, can be a null pointer
		*/
		inline void setPtr(AType* ptr)
		{
			// Release old pointer
			if (nullptr != mPtr)
			{
				mPtr->releaseReference();
			}

			// Assign new pointer
			if (nullptr != ptr)
			{
				ptr->addReference();
			}
			mPtr = ptr;
		}

		/**
		*  @brief
		*    Get pointer to the reference counted object
		*
		*  @return
		*    Pointer to the RefCount object, can be a null pointer
		*/
		inline AType* getPtr() const
		{
			// Return pointer
			return mPtr;
		}

	// Private data
	private:
		AType* mPtr; ///< Pointer to reference counter, can be a null pointer

	};




	//[-------------------------------------------------------]
	//[ Renderer/Capabilities.h                               ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Capabilities class
	*
	*  @note
	*    - The data is public by intent in order to make it easier to use this class,
	*      no issues involved because the user only gets a constant instance
	*/
	class Capabilities final
	{

	// Public data
	public:
		char				deviceName[128];								///< UTF-8 device name of the used graphics card (e.g. "AMD Radeon R9 200 Series")
		TextureFormat::Enum preferredSwapChainColorTextureFormat;			///< Preferred swap chain color texture format
		TextureFormat::Enum preferredSwapChainDepthStencilTextureFormat;	///< Preferred swap chain depth stencil texture format
		uint32_t			maximumNumberOfViewports;						///< Maximum number of viewports (always at least 1)
		uint32_t			maximumNumberOfSimultaneousRenderTargets;		///< Maximum number of simultaneous render targets (if <1 render to texture is not supported)
		uint32_t			maximumTextureDimension;						///< Maximum texture dimension (usually 2048, 4096, 8192 or 16384)
		uint32_t			maximumNumberOf2DTextureArraySlices;			///< Maximum number of 2D texture array slices (usually 512 up to 8192, in case there's no support for 2D texture arrays it's 0)
		uint32_t			maximumUniformBufferSize;						///< Maximum uniform buffer (UBO) size in bytes (usually at least 4096 *16 bytes, in case there's no support for uniform buffer it's 0)
		uint32_t			maximumTextureBufferSize;						///< Maximum texture buffer (TBO) size in texel (>65536, typically much larger than that of one-dimensional texture, in case there's no support for texture buffer it's 0)
		uint32_t			maximumIndirectBufferSize;						///< Maximum indirect buffer size in bytes
		uint8_t				maximumNumberOfMultisamples;					///< Maximum number of multisamples (always at least 1, usually 8)
		uint8_t				maximumAnisotropy;								///< Maximum anisotropy (always at least 1, usually 16)
		bool				upperLeftOrigin;								///< Upper left origin (true for Vulkan, Direct3D, OpenGL with "GL_ARB_clip_control"-extension)
		bool				zeroToOneClipZ;									///< Zero-to-one clip Z (true for Vulkan, Direct3D, OpenGL with "GL_ARB_clip_control"-extension)
		bool				individualUniforms;								///< Individual uniforms ("constants" in Direct3D terminology) supported? If not, only uniform buffer objects are supported.
		bool				instancedArrays;								///< Instanced arrays supported? (shader model 3 feature, vertex array element advancing per-instance instead of per-vertex)
		bool				drawInstanced;									///< Draw instanced supported? (shader model 4 feature, build in shader variable holding the current instance ID)
		bool				baseVertex;										///< Base vertex supported for draw calls?
		bool				nativeMultiThreading;							///< Does the renderer support native multi-threading? For example Direct3D 11 does meaning we can also create renderer resources asynchronous while for OpenGL we have to create an separate OpenGL context (less efficient, more complex to implement).
		bool				shaderBytecode;									///< Shader bytecode supported?
		// Vertex-shader (VS) stage
		bool				vertexShader;									///< Is there support for vertex shaders (VS)?
		// Tessellation-control-shader (TCS) stage and tessellation-evaluation-shader (TES) stage
		uint32_t			maximumNumberOfPatchVertices;					///< Maximum number of vertices per patch (usually 0 for no tessellation support or 32 which is the maximum number of supported vertices per patch)
		// Geometry-shader (GS) stage
		uint32_t			maximumNumberOfGsOutputVertices;				///< Maximum number of vertices a geometry shader (GS) can emit (usually 0 for no geometry shader support or 1024)
		// Fragment-shader (FS) stage
		bool				fragmentShader;									///< Is there support for fragment shaders (FS)?

	// Public methods
	public:
		/**
		*  @brief
		*    Default constructor
		*/
		inline Capabilities() :
			deviceName{},
			preferredSwapChainColorTextureFormat(TextureFormat::Enum::UNKNOWN),
			preferredSwapChainDepthStencilTextureFormat(TextureFormat::Enum::UNKNOWN),
			maximumNumberOfViewports(0),
			maximumNumberOfSimultaneousRenderTargets(0),
			maximumTextureDimension(0),
			maximumNumberOf2DTextureArraySlices(0),
			maximumUniformBufferSize(0),
			maximumTextureBufferSize(0),
			maximumIndirectBufferSize(0),
			maximumNumberOfMultisamples(1),
			maximumAnisotropy(1),
			upperLeftOrigin(true),
			zeroToOneClipZ(true),
			individualUniforms(false),
			instancedArrays(false),
			drawInstanced(false),
			baseVertex(false),
			nativeMultiThreading(false),
			shaderBytecode(false),
			vertexShader(false),
			maximumNumberOfPatchVertices(0),
			maximumNumberOfGsOutputVertices(0),
			fragmentShader(false)
		{}

		/**
		*  @brief
		*    Destructor
		*/
		inline ~Capabilities()
		{}

	// Private methods
	private:
		explicit Capabilities(const Capabilities& source) = delete;
		Capabilities& operator =(const Capabilities& source) = delete;

	};




	//[-------------------------------------------------------]
	//[ Renderer/Statistics.h                                 ]
	//[-------------------------------------------------------]
	#ifdef RENDERER_STATISTICS
		/**
		*  @brief
		*    Statistics class
		*
		*  @note
		*    - The data is public by intent in order to make it easier to use this class,
		*      no issues involved because the user only gets a constant instance
		*/
		class Statistics final
		{

		// Public data
		public:
			// Resources
			std::atomic<uint32_t> currentNumberOfRootSignatures;				///< Current number of root signature instances
			std::atomic<uint32_t> numberOfCreatedRootSignatures;				///< Number of created root signature instances
			std::atomic<uint32_t> currentNumberOfResourceGroups;				///< Current number of resource group instances
			std::atomic<uint32_t> numberOfCreatedResourceGroups;				///< Number of created resource group instances
			std::atomic<uint32_t> currentNumberOfPrograms;						///< Current number of program instances
			std::atomic<uint32_t> numberOfCreatedPrograms;						///< Number of created program instances
			std::atomic<uint32_t> currentNumberOfVertexArrays;					///< Current number of vertex array object (VAO, input-assembler (IA) stage) instances
			std::atomic<uint32_t> numberOfCreatedVertexArrays;					///< Number of created vertex array object (VAO, input-assembler (IA) stage) instances
			std::atomic<uint32_t> currentNumberOfRenderPasses;					///< Current number of render pass instances
			std::atomic<uint32_t> numberOfCreatedRenderPasses;					///< Number of created render pass instances
			// IRenderTarget
			std::atomic<uint32_t> currentNumberOfSwapChains;					///< Current number of swap chain instances
			std::atomic<uint32_t> numberOfCreatedSwapChains;					///< Number of created swap chain instances
			std::atomic<uint32_t> currentNumberOfFramebuffers;					///< Current number of framebuffer object (FBO) instances
			std::atomic<uint32_t> numberOfCreatedFramebuffers;					///< Number of created framebuffer object (FBO) instances
			// IBuffer
			std::atomic<uint32_t> currentNumberOfIndexBuffers;					///< Current number of index buffer object (IBO, input-assembler (IA) stage) instances
			std::atomic<uint32_t> numberOfCreatedIndexBuffers;					///< Number of created index buffer object (IBO, input-assembler (IA) stage) instances
			std::atomic<uint32_t> currentNumberOfVertexBuffers;					///< Current number of vertex buffer object (VBO, input-assembler (IA) stage) instances
			std::atomic<uint32_t> numberOfCreatedVertexBuffers;					///< Number of created vertex buffer object (VBO, input-assembler (IA) stage) instances
			std::atomic<uint32_t> currentNumberOfUniformBuffers;				///< Current number of uniform buffer object (UBO, "constant buffer" in Direct3D terminology) instances
			std::atomic<uint32_t> numberOfCreatedUniformBuffers;				///< Number of created uniform buffer object (UBO, "constant buffer" in Direct3D terminology) instances
			std::atomic<uint32_t> currentNumberOfTextureBuffers;				///< Current number of texture buffer object (TBO) instances
			std::atomic<uint32_t> numberOfCreatedTextureBuffers;				///< Number of created texture buffer object (TBO) instances
			std::atomic<uint32_t> currentNumberOfIndirectBuffers;				///< Current number of indirect buffer object instances
			std::atomic<uint32_t> numberOfCreatedIndirectBuffers;				///< Number of created indirect buffer object instances
			// ITexture
			std::atomic<uint32_t> currentNumberOfTexture1Ds;					///< Current number of texture 1D instances
			std::atomic<uint32_t> numberOfCreatedTexture1Ds;					///< Number of created texture 1D instances
			std::atomic<uint32_t> currentNumberOfTexture2Ds;					///< Current number of texture 2D instances
			std::atomic<uint32_t> numberOfCreatedTexture2Ds;					///< Number of created texture 2D instances
			std::atomic<uint32_t> currentNumberOfTexture2DArrays;				///< Current number of texture 2D array instances
			std::atomic<uint32_t> numberOfCreatedTexture2DArrays;				///< Number of created texture 2D array instances
			std::atomic<uint32_t> currentNumberOfTexture3Ds;					///< Current number of texture 3D instances
			std::atomic<uint32_t> numberOfCreatedTexture3Ds;					///< Number of created texture 3D instances
			std::atomic<uint32_t> currentNumberOfTextureCubes;					///< Current number of texture cube instances
			std::atomic<uint32_t> numberOfCreatedTextureCubes;					///< Number of created texture cube instances
			// IState
			std::atomic<uint32_t> currentNumberOfPipelineStates;				///< Current number of pipeline state (PSO) instances
			std::atomic<uint32_t> numberOfCreatedPipelineStates;				///< Number of created pipeline state (PSO) instances
			std::atomic<uint32_t> currentNumberOfSamplerStates;					///< Current number of sampler state instances
			std::atomic<uint32_t> numberOfCreatedSamplerStates;					///< Number of created sampler state instances
			// IShader
			std::atomic<uint32_t> currentNumberOfVertexShaders;					///< Current number of vertex shader (VS) instances
			std::atomic<uint32_t> numberOfCreatedVertexShaders;					///< Number of created vertex shader (VS) instances
			std::atomic<uint32_t> currentNumberOfTessellationControlShaders;	///< Current number of tessellation control shader (TCS, "hull shader" in Direct3D terminology) instances
			std::atomic<uint32_t> numberOfCreatedTessellationControlShaders;	///< Number of created tessellation control shader (TCS, "hull shader" in Direct3D terminology) instances
			std::atomic<uint32_t> currentNumberOfTessellationEvaluationShaders;	///< Current number of tessellation evaluation shader (TES, "domain shader" in Direct3D terminology) instances
			std::atomic<uint32_t> numberOfCreatedTessellationEvaluationShaders;	///< Number of created tessellation evaluation shader (TES, "domain shader" in Direct3D terminology) instances
			std::atomic<uint32_t> currentNumberOfGeometryShaders;				///< Current number of geometry shader (GS) instances
			std::atomic<uint32_t> numberOfCreatedGeometryShaders;				///< Number of created geometry shader (GS) instances
			std::atomic<uint32_t> currentNumberOfFragmentShaders;				///< Current number of fragment shader (FS, "pixel shader" in Direct3D terminology) instances
			std::atomic<uint32_t> numberOfCreatedFragmentShaders;				///< Number of created fragment shader (FS, "pixel shader" in Direct3D terminology) instances

		// Public methods
		public:
			/**
			*  @brief
			*    Default constructor
			*/
			inline Statistics() :
				currentNumberOfRootSignatures(0),
				numberOfCreatedRootSignatures(0),
				currentNumberOfResourceGroups(0),
				numberOfCreatedResourceGroups(0),
				currentNumberOfPrograms(0),
				numberOfCreatedPrograms(0),
				currentNumberOfVertexArrays(0),
				numberOfCreatedVertexArrays(0),
				currentNumberOfRenderPasses(0),
				numberOfCreatedRenderPasses(0),
				// IRenderTarget
				currentNumberOfSwapChains(0),
				numberOfCreatedSwapChains(0),
				currentNumberOfFramebuffers(0),
				numberOfCreatedFramebuffers(0),
				// IBuffer
				currentNumberOfIndexBuffers(0),
				numberOfCreatedIndexBuffers(0),
				currentNumberOfVertexBuffers(0),
				numberOfCreatedVertexBuffers(0),
				currentNumberOfUniformBuffers(0),
				numberOfCreatedUniformBuffers(0),
				currentNumberOfTextureBuffers(0),
				numberOfCreatedTextureBuffers(0),
				currentNumberOfIndirectBuffers(0),
				numberOfCreatedIndirectBuffers(0),
				// ITexture
				currentNumberOfTexture1Ds(0),
				numberOfCreatedTexture1Ds(0),
				currentNumberOfTexture2Ds(0),
				numberOfCreatedTexture2Ds(0),
				currentNumberOfTexture2DArrays(0),
				numberOfCreatedTexture2DArrays(0),
				currentNumberOfTexture3Ds(0),
				numberOfCreatedTexture3Ds(0),
				currentNumberOfTextureCubes(0),
				numberOfCreatedTextureCubes(0),
				// IState
				currentNumberOfPipelineStates(0),
				numberOfCreatedPipelineStates(0),
				currentNumberOfSamplerStates(0),
				numberOfCreatedSamplerStates(0),
				// IShader
				currentNumberOfVertexShaders(0),
				numberOfCreatedVertexShaders(0),
				currentNumberOfTessellationControlShaders(0),
				numberOfCreatedTessellationControlShaders(0),
				currentNumberOfTessellationEvaluationShaders(0),
				numberOfCreatedTessellationEvaluationShaders(0),
				currentNumberOfGeometryShaders(0),
				numberOfCreatedGeometryShaders(0),
				currentNumberOfFragmentShaders(0),
				numberOfCreatedFragmentShaders(0)
			{}

			/**
			*  @brief
			*    Destructor
			*/
			inline ~Statistics()
			{}

			/**
			*  @brief
			*    Return the number of current resource instances
			*
			*  @return
			*    The number of current resource instances
			*
			*  @note
			*    - Primarily for debugging
			*    - The result is calculated by using the current statistics, do only call this method if you have to
			*/
			inline uint32_t getNumberOfCurrentResources() const
			{
				// Calculate the current number of resource instances
				return	currentNumberOfRootSignatures +
						currentNumberOfResourceGroups +
						currentNumberOfPrograms +
						currentNumberOfVertexArrays +
						currentNumberOfRenderPasses +
						// IRenderTarget
						currentNumberOfSwapChains +
						currentNumberOfFramebuffers +
						// IBuffer
						currentNumberOfIndexBuffers +
						currentNumberOfVertexBuffers +
						currentNumberOfUniformBuffers +
						currentNumberOfTextureBuffers +
						currentNumberOfIndirectBuffers +
						// ITexture
						currentNumberOfTexture1Ds +
						currentNumberOfTexture2Ds +
						currentNumberOfTexture2DArrays +
						currentNumberOfTexture3Ds +
						currentNumberOfTextureCubes +
						// IState
						currentNumberOfPipelineStates +
						currentNumberOfSamplerStates +
						// IShader
						currentNumberOfVertexShaders +
						currentNumberOfTessellationControlShaders +
						currentNumberOfTessellationEvaluationShaders +
						currentNumberOfGeometryShaders +
						currentNumberOfFragmentShaders;
			}

			/**
			*  @brief
			*    Use debug output to show the current number of resource instances
			*
			*  @param[in] context
			*    The renderer context to use
			*
			*  @note
			*    - Primarily for debugging
			*/
			inline void debugOutputCurrentResouces(const Context& context) const
			{
				// Start
				RENDERER_LOG(context, INFORMATION, "** Number of current renderer resource instances **")

				// Misc
				RENDERER_LOG(context, INFORMATION, "Root signatures: %d", currentNumberOfRootSignatures.load())
				RENDERER_LOG(context, INFORMATION, "Resource groups: %d", currentNumberOfResourceGroups.load())
				RENDERER_LOG(context, INFORMATION, "Programs: %d", currentNumberOfPrograms.load())
				RENDERER_LOG(context, INFORMATION, "Vertex arrays: %d", currentNumberOfVertexArrays.load())
				RENDERER_LOG(context, INFORMATION, "Render passes: %d", currentNumberOfRenderPasses.load())

				// IRenderTarget
				RENDERER_LOG(context, INFORMATION, "Swap chains: %d", currentNumberOfSwapChains.load())
				RENDERER_LOG(context, INFORMATION, "Framebuffers: %d", currentNumberOfFramebuffers.load())

				// IBuffer
				RENDERER_LOG(context, INFORMATION, "Index buffers: %d", currentNumberOfIndexBuffers.load())
				RENDERER_LOG(context, INFORMATION, "Vertex buffers: %d", currentNumberOfVertexBuffers.load())
				RENDERER_LOG(context, INFORMATION, "Uniform buffers: %d", currentNumberOfUniformBuffers.load())
				RENDERER_LOG(context, INFORMATION, "Texture buffers: %d", currentNumberOfTextureBuffers.load())
				RENDERER_LOG(context, INFORMATION, "Indirect buffers: %d", currentNumberOfIndirectBuffers.load())

				// ITexture
				RENDERER_LOG(context, INFORMATION, "1D textures: %d", currentNumberOfTexture1Ds.load())
				RENDERER_LOG(context, INFORMATION, "2D textures: %d", currentNumberOfTexture2Ds.load())
				RENDERER_LOG(context, INFORMATION, "2D texture arrays: %d", currentNumberOfTexture2DArrays.load())
				RENDERER_LOG(context, INFORMATION, "3D textures: %d", currentNumberOfTexture3Ds.load())
				RENDERER_LOG(context, INFORMATION, "Cube textures: %d", currentNumberOfTextureCubes.load())

				// IState
				RENDERER_LOG(context, INFORMATION, "Pipeline states: %d", currentNumberOfPipelineStates.load())
				RENDERER_LOG(context, INFORMATION, "Sampler states: %d", currentNumberOfSamplerStates.load())

				// IShader
				RENDERER_LOG(context, INFORMATION, "Vertex shaders: %d", currentNumberOfVertexShaders.load())
				RENDERER_LOG(context, INFORMATION, "Tessellation control shaders: %d", currentNumberOfTessellationControlShaders.load())
				RENDERER_LOG(context, INFORMATION, "Tessellation evaluation shaders: %d", currentNumberOfTessellationEvaluationShaders.load())
				RENDERER_LOG(context, INFORMATION, "Geometry shaders: %d", currentNumberOfGeometryShaders.load())
				RENDERER_LOG(context, INFORMATION, "Fragment shaders: %d", currentNumberOfFragmentShaders.load())

				// End
				RENDERER_LOG(context, INFORMATION, "***************************************************")
			}

		// Private methods
		private:
			explicit Statistics(const Statistics& source) = delete;
			Statistics& operator =(const Statistics& source) = delete;

		};
	#endif




	//[-------------------------------------------------------]
	//[ Renderer/IRenderer.h                                  ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Renderer backend name as ID
	*/
	enum class NameId : uint32_t
	{
		VULKAN		= 1646768219,	///< Vulkan renderer backend, same value as renderer runtime STRING_ID("Vulkan")
		DIRECT3D12	= 2152506057,	///< Direct3D 12 renderer backend, same value as renderer runtime STRING_ID("Direct3D12")
		DIRECT3D11	= 2102173200,	///< Direct3D 11 renderer backend, same value as renderer runtime STRING_ID("Direct3D11")
		DIRECT3D10	= 2118950819,	///< Direct3D 10 renderer backend, same value as renderer runtime STRING_ID("Direct3D10")
		DIRECT3D9	= 3508528873,	///< Direct3D 9 renderer backend, same value as renderer runtime STRING_ID("Direct3D9")
		OPENGL		= 1149085807,	///< OpenGL renderer backend, same value as renderer runtime STRING_ID("OpenGL")
		OPENGLES3	= 4137012044,	///< OpenGL ES 3 renderer backend, same value as renderer runtime STRING_ID("OpenGLES3")
		NULL_DUMMY	= 3816175889	///< Null renderer backend, same value as renderer runtime STRING_ID("Null")
	};

	/**
	*  @brief
	*    Abstract renderer interface
	*/
	class IRenderer : public RefCount<IRenderer>
	{

	// Friends for none constant statistics access
		friend class IRootSignature;
		friend class IResourceGroup;
		friend class IProgram;
		friend class IVertexArray;
		friend class IRenderPass;
		friend class ISwapChain;
		friend class IFramebuffer;
		friend class IIndexBuffer;
		friend class IVertexBuffer;
		friend class IUniformBuffer;
		friend class ITextureBuffer;
		friend class IIndirectBuffer;
		friend class ITexture1D;
		friend class ITexture2D;
		friend class ITexture2DArray;
		friend class ITexture3D;
		friend class ITextureCube;
		friend class IPipelineState;
		friend class ISamplerState;
		friend class IVertexShader;
		friend class ITessellationControlShader;
		friend class ITessellationEvaluationShader;
		friend class IGeometryShader;
		friend class IFragmentShader;

	// Public methods
	public:
		/**
		*  @brief
		*    Destructor
		*/
		inline virtual ~IRenderer() override
		{}

		/**
		*  @brief
		*    Return the renderer backend name as ID
		*
		*  @return
		*    The renderer backend name as ID
		*/
		inline NameId getNameId() const
		{
			return mNameId;
		}

		/**
		*  @brief
		*    Return the context of the renderer instance
		*
		*  @return
		*    The context of the renderer instance, do not free the memory the returned reference is pointing to
		*/
		inline const Context& getContext() const
		{
			return mContext;
		}

		/**
		*  @brief
		*    Return the capabilities of the renderer instance
		*
		*  @return
		*    The capabilities of the renderer instance, do not free the memory the returned reference is pointing to
		*/
		inline const Capabilities& getCapabilities() const
		{
			return mCapabilities;
		}

		#ifdef RENDERER_STATISTICS
			/**
			*  @brief
			*    Return the statistics of the renderer instance
			*
			*  @return
			*    The statistics of the renderer instance, do not free the memory the returned reference is pointing to
			*
			*  @note
			*    - It's possible that the statistics or part of it are disabled, e.g. due to hight performance constrains
			*/
			inline const Statistics& getStatistics() const
			{
				return mStatistics;
			}
		#endif

	// Public virtual Renderer::IRenderer methods
	public:
		/**
		*  @brief
		*    Return the name of the renderer instance
		*
		*  @return
		*    The ASCII name of the renderer instance, null pointer on error, do not free the memory the returned pointer is pointing to
		*/
		virtual const char* getName() const = 0;

		/**
		*  @brief
		*    Return whether or not the renderer instance is properly initialized
		*
		*  @return
		*    "true" if the renderer instance is properly initialized, else "false"
		*
		*  @note
		*    - Do never ever use a not properly initialized renderer!
		*/
		virtual bool isInitialized() const = 0;

		/**
		*  @brief
		*    Return whether or not debug is enabled
		*
		*  @remarks
		*    By using
		*      "Renderer::IRenderer::isDebugEnabled();"
		*    it is possible to check whether or not your application is currently running
		*    within a known debug/profile tool like e.g. Direct3D PIX (also works directly within VisualStudio
		*    2012 out-of-the-box). In case you want at least try to protect your asset, you might want to stop
		*    the execution of your application when a debug/profile tool is used which can e.g. record your data.
		*    Please be aware that this will only make it a little bit harder to debug and e.g. while doing so
		*    reading out your asset data. Public articles like
		*    "PIX: How to circumvent D3DPERF_SetOptions" at
		*      http://www.gamedev.net/blog/1323/entry-2250952-pix-how-to-circumvent-d3dperf-setoptions/
		*    describe how to "hack around" this security measurement, so, don't rely on it. Those debug
		*    methods work fine when using a Direct3D renderer implementation. OpenGL on the other hand
		*    has no Direct3D PIX like functions or extensions, use for instance "gDEBugger" (http://www.gremedy.com/)
		*    instead.
		*    -> When using Direct3D <11.1, those methods map to the Direct3D 9 PIX functions (D3DPERF_* functions)
		*    -> The Direct3D 9 PIX functions are also used for Direct3D 10 and Direct3D 11. Lookout! As soon as using
		*       the debug methods within this interface, the Direct3D 9 dll will be loaded.
		*    -> Starting with Direct3D 11.1, the Direct3D 9 PIX functions no longer work. Instead, the new
		*       "D3D11_CREATE_DEVICE_PREVENT_ALTERING_LAYER_SETTINGS_FROM_REGISTRY"-flag (does not work with <Direct3D 11.1)
		*       is used when creating the device instance, then the "ID3DUserDefinedAnnotation"-API is used.
		*    -> Optimization: You might want to use those methods only via macros to make it easier to avoid using them
		*       within e.g. a final release build
		*
		*  @return
		*    "true" if debug is enabled, else "false"
		*/
		virtual bool isDebugEnabled() = 0;

		//[-------------------------------------------------------]
		//[ Shader language                                       ]
		//[-------------------------------------------------------]
		/**
		*  @brief
		*    Return the number of supported shader languages
		*
		*  @return
		*    The number of supported shader languages
		*/
		virtual uint32_t getNumberOfShaderLanguages() const = 0;

		/**
		*  @brief
		*    Return the name of a supported shader language at the provided index
		*
		*  @param[in] index
		*    Index of the supported shader language to return the name from ([0, getNumberOfShaderLanguages()-1])
		*
		*  @return
		*    The ASCII name (for example "GLSL" or "HLSL") of the supported shader language at the provided index, can be a null pointer
		*
		*  @note
		*    - Do not free the memory the returned pointer is pointing to
		*    - The default shader language is always at index 0
		*/
		virtual const char* getShaderLanguageName(uint32_t index) const = 0;

		/**
		*  @brief
		*    Return a shader language instance
		*
		*  @param[in] shaderLanguageName
		*    The ASCII name of the shader language (for example "GLSL" or "HLSL"), if null pointer or empty string,
		*    the default renderer shader language is used (see "getShaderLanguageName(0)")
		*
		*  @return
		*    The shader language instance, a null pointer on error, do not release the returned instance unless you added an own reference to it
		*/
		virtual IShaderLanguage* getShaderLanguage(const char* shaderLanguageName = nullptr) = 0;

		//[-------------------------------------------------------]
		//[ Resource creation                                     ]
		//[-------------------------------------------------------]
		/**
		*  @brief
		*    Create a render pass instance
		*
		*  @param[in] numberOfColorAttachments
		*    Number of color render target textures, must be <="Renderer::Capabilities::maximumNumberOfSimultaneousRenderTargets"
		*  @param[in] colorAttachmentTextureFormats
		*    The color render target texture formats, can be a null pointer or can contain null pointers, if not a null pointer there must be at
		*    least "numberOfColorAttachments" textures in the provided C-array of pointers
		*  @param[in] depthStencilAttachmentTextureFormat
		*    The optional depth stencil render target texture format, can be a "Renderer::TextureFormat::UNKNOWN" if there should be no depth buffer
		*  @param[in] numberOfMultisamples
		*    The number of multisamples per pixel (valid values: 1, 2, 4, 8)
		*
		*  @return
		*    The created render pass instance, null pointer on error. Release the returned instance if you no longer need it.
		*/
		virtual IRenderPass* createRenderPass(uint32_t numberOfColorAttachments, const TextureFormat::Enum* colorAttachmentTextureFormats, TextureFormat::Enum depthStencilAttachmentTextureFormat = TextureFormat::UNKNOWN, uint8_t numberOfMultisamples = 1) = 0;

		/**
		*  @brief
		*    Create a swap chain instance
		*
		*  @param[in] renderPass
		*    Render pass to use, the swap chain keeps a reference to the render pass
		*  @param[in] windowHandle
		*    Information about the window to render into
		*  @param[in] useExternalContext
		*    Indicates if an external renderer context is used; in this case the renderer itself has nothing to do with the creation/managing of an renderer context
		*
		*  @return
		*    The created swap chain instance, null pointer on error. Release the returned instance if you no longer need it.
		*/
		virtual ISwapChain* createSwapChain(IRenderPass& renderPass, WindowHandle windowHandle, bool useExternalContext = false) = 0;

		/**
		*  @brief
		*    Create a framebuffer object (FBO) instance
		*
		*  @param[in] renderPass
		*    Render pass to use, the framebuffer keeps a reference to the render pass
		*  @param[in] colorFramebufferAttachments
		*    The color render target textures, can be a null pointer or can contain null pointers, if not a null pointer there must be at
		*    least "Renderer::IRenderPass::getNumberOfColorAttachments()" textures in the provided C-array of pointers
		*  @param[in] depthStencilFramebufferAttachment
		*    The optional depth stencil render target texture, can be a null pointer
		*
		*  @return
		*    The created FBO instance, null pointer on error. Release the returned instance if you no longer need it.
		*
		*  @note
		*    - Only supported if "Renderer::Capabilities::maximumNumberOfSimultaneousRenderTargets" is not 0
		*    - The framebuffer keeps a reference to the provided texture instances
		*    - It's invalid to set the same color texture to multiple render targets at one and the same time
		*    - Depending on the used graphics API and feature set, there might be the requirement that all provided textures have the same size
		*      (in order to be on the save side, ensure that all provided textures have the same size and same MSAA sample count)
		*/
		virtual IFramebuffer* createFramebuffer(IRenderPass& renderPass, const FramebufferAttachment* colorFramebufferAttachments, const FramebufferAttachment* depthStencilFramebufferAttachment = nullptr) = 0;

		/**
		*  @brief
		*    Create a buffer manager instance
		*
		*  @return
		*    The created buffer manager instance, null pointer on error. Release the returned instance if you no longer need it.
		*/
		virtual IBufferManager* createBufferManager() = 0;

		/**
		*  @brief
		*    Create a texture manager instance
		*
		*  @return
		*    The created texture manager instance, null pointer on error. Release the returned instance if you no longer need it.
		*/
		virtual ITextureManager* createTextureManager() = 0;

		/**
		*  @brief
		*    Create a root signature instance
		*
		*  @param[in] rootSignature
		*    Root signature to use
		*
		*  @return
		*    The root signature instance, null pointer on error. Release the returned instance if you no longer need it.
		*/
		virtual IRootSignature* createRootSignature(const RootSignature& rootSignature) = 0;

		/**
		*  @brief
		*    Create a pipeline state instance
		*
		*  @param[in] pipelineState
		*    Pipeline state to use
		*
		*  @return
		*    The pipeline state instance, null pointer on error. Release the returned instance if you no longer need it.
		*/
		virtual IPipelineState* createPipelineState(const PipelineState& pipelineState) = 0;

		/**
		*  @brief
		*    Create a sampler state instance
		*
		*  @param[in] samplerState
		*    Sampler state to use
		*
		*  @return
		*    The sampler state instance, null pointer on error. Release the returned instance if you no longer need it.
		*/
		virtual ISamplerState* createSamplerState(const SamplerState& samplerState) = 0;

		//[-------------------------------------------------------]
		//[ Resource handling                                     ]
		//[-------------------------------------------------------]
		/**
		*  @brief
		*    Map a resource
		*
		*  @param[in]  resource
		*    Resource to map, there's no internal resource validation, so, do only use valid resources in here!
		*  @param[in]  subresource
		*    Subresource
		*  @param[in]  mapType
		*    Map type
		*  @param[in]  mapFlags
		*    Map flags, see "Renderer::MapFlag"-flags
		*  @param[out] mappedSubresource
		*    Receives the mapped subresource information, do only use this data in case this method returns successfully
		*
		*  @return
		*    "true" if all went fine, else "false"
		*/
		virtual bool map(IResource& resource, uint32_t subresource, MapType mapType, uint32_t mapFlags, MappedSubresource& mappedSubresource) = 0;

		/**
		*  @brief
		*    Unmap a resource
		*
		*  @param[in] resource
		*    Resource to unmap, there's no internal resource validation, so, do only use valid resources in here!
		*  @param[in] subresource
		*    Subresource
		*/
		virtual void unmap(IResource& resource, uint32_t subresource) = 0;

		//[-------------------------------------------------------]
		//[ Operations                                            ]
		//[-------------------------------------------------------]
		/**
		*  @brief
		*    Begin scene rendering
		*
		*  @return
		*    "true" if all went fine, else "false" (In this case: Don't dare to render something)
		*
		*  @note
		*    - In order to be graphics API independent, call this method when starting to render something
		*/
		virtual bool beginScene() = 0;

		/**
		*  @brief
		*    Submit command buffer to renderer
		*
		*  @param[in] commandBuffer
		*    Command buffer to submit
		*/
		virtual void submitCommandBuffer(const CommandBuffer& commandBuffer) = 0;

		/**
		*  @brief
		*    End scene rendering
		*
		*  @note
		*    - In order to be graphics API independent, call this method when you're done with rendering
		*/
		virtual void endScene() = 0;

		//[-------------------------------------------------------]
		//[ Synchronization                                       ]
		//[-------------------------------------------------------]
		/**
		*  @brief
		*    Force the execution of render commands in finite time (synchronization)
		*/
		virtual void flush() = 0;

		/**
		*  @brief
		*    Force the execution of render commands in finite time and wait until it's done (synchronization)
		*/
		virtual void finish() = 0;

		//[-------------------------------------------------------]
		//[ Backend specific                                      ]
		//[-------------------------------------------------------]
		virtual void* getD3D11DevicePointer() const
		{
			return nullptr;
		}

		virtual void* getD3D11ImmediateContextPointer() const
		{
			return nullptr;
		}

	// Protected methods
	protected:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] nameId
		*    Renderer backend name as ID
		*  @param[in] context
		*    Renderer context, the renderer context instance must stay valid as long as the renderer instance exists
		*/
		inline IRenderer(NameId nameId, const Context& context) :
			mNameId(nameId),
			mContext(context)
		{}

		explicit IRenderer(const IRenderer& source) = delete;
		IRenderer& operator =(const IRenderer& source) = delete;

		#ifdef RENDERER_STATISTICS
			/**
			*  @brief
			*    Return the statistics of the renderer instance
			*
			*  @return
			*    The statistics of the renderer instance
			*
			*  @note
			*    - Do not free the memory the returned reference is pointing to
			*    - It's possible that the statistics or part of it are disabled, e.g. due to hight performance constrains
			*/
			inline Statistics& getStatistics()
			{
				return mStatistics;
			}
		#endif

	// Protected data
	protected:
		NameId		   mNameId;			///< Renderer backend name as ID
		const Context& mContext;		///< Context
		Capabilities   mCapabilities;	///< Capabilities

	#ifdef RENDERER_STATISTICS
		// Private data
		private:
			Statistics mStatistics;	///< Statistics
	#endif

	};

	typedef SmartRefCount<IRenderer> IRendererPtr;




	//[-------------------------------------------------------]
	//[ Renderer/Shader/IShaderLanguage.h                     ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Abstract shader language interface
	*/
	class IShaderLanguage : public RefCount<IShaderLanguage>
	{

	// Public definitions
	public:
		/**
		*  @brief
		*    Optimization level
		*/
		enum class OptimizationLevel
		{
			Debug = 0,	///< No optimization and debug features enabled, usually only used for debugging
			None,		///< No optimization, usually only used for debugging
			Low,		///< Low optimization
			Medium,		///< Medium optimization
			High,		///< High optimization
			Ultra		///< Ultra optimization
		};

	// Public methods
	public:
		/**
		*  @brief
		*    Destructor
		*/
		inline virtual ~IShaderLanguage() override
		{}

		/**
		*  @brief
		*    Return the owner renderer instance
		*
		*  @return
		*    The owner renderer instance, do not release the returned instance unless you added an own reference to it
		*/
		inline IRenderer& getRenderer() const
		{
			return *mRenderer;
		}

		/**
		*  @brief
		*    Return the optimization level
		*
		*  @return
		*    The optimization level
		*/
		inline OptimizationLevel getOptimizationLevel() const
		{
			return mOptimizationLevel;
		}

		/**
		*  @brief
		*    Set the optimization level
		*
		*  @param[in] optimizationLevel
		*    The optimization level
		*/
		inline void setOptimizationLevel(OptimizationLevel optimizationLevel)
		{
			mOptimizationLevel = optimizationLevel;
		}

		/**
		*  @brief
		*    Create a program and assigns a vertex and fragment shader to it
		*
		*  @param[in] rootSignature
		*    Root signature
		*  @param[in] vertexAttributes
		*    Vertex attributes ("vertex declaration" in Direct3D 9 terminology, "input layout" in Direct3D 10 & 11 terminology)
		*  @param[in] vertexShader
		*    Vertex shader the program is using, can be a null pointer, vertex shader and program language must match!
		*  @param[in] fragmentShader
		*    Fragment shader the program is using, can be a null pointer, fragment shader and program language must match!
		*
		*  @return
		*    The created program, a null pointer on error. Release the returned instance if you no longer need it.
		*
		*  @note
		*    - The program keeps a reference to the provided shaders and releases it when no longer required
		*    - It's valid that a program implementation is adding a reference and releasing it again at once
		*      (this means that in the case of not having any more references, a shader might get destroyed when calling this method)
		*    - Comfort method
		*/
		inline IProgram* createProgram(const IRootSignature& rootSignature, const VertexAttributes& vertexAttributes, IVertexShader* vertexShader, IFragmentShader* fragmentShader)
		{
			return createProgram(rootSignature, vertexAttributes, vertexShader, nullptr, nullptr, nullptr, fragmentShader);
		}

		/**
		*  @brief
		*    Create a program and assigns a vertex, geometry and fragment shader to it
		*
		*  @param[in] rootSignature
		*    Root signature
		*  @param[in] vertexAttributes
		*    Vertex attributes ("vertex declaration" in Direct3D 9 terminology, "input layout" in Direct3D 10 & 11 terminology)
		*  @param[in] vertexShader
		*    Vertex shader the program is using, can be a null pointer, vertex shader and program language must match!
		*  @param[in] geometryShader
		*    Geometry shader the program is using, can be a null pointer, geometry shader and program language must match!
		*  @param[in] fragmentShader
		*    Fragment shader the program is using, can be a null pointer, fragment shader and program language must match!
		*
		*  @return
		*    The created program, a null pointer on error. Release the returned instance if you no longer need it.
		*
		*  @note
		*    - The program keeps a reference to the provided shaders and releases it when no longer required
		*    - It's valid that a program implementation is adding a reference and releasing it again at once
		*      (this means that in the case of not having any more references, a shader might get destroyed when calling this method)
		*    - Comfort method
		*/
		inline IProgram* createProgram(const IRootSignature& rootSignature, const VertexAttributes& vertexAttributes, IVertexShader* vertexShader, IGeometryShader* geometryShader, IFragmentShader* fragmentShader)
		{
			return createProgram(rootSignature, vertexAttributes, vertexShader, nullptr, nullptr, geometryShader, fragmentShader);
		}

		/**
		*  @brief
		*    Create a program and assigns a vertex, tessellation control, tessellation evaluation and fragment shader to it
		*
		*  @param[in] rootSignature
		*    Root signature
		*  @param[in] vertexAttributes
		*    Vertex attributes ("vertex declaration" in Direct3D 9 terminology, "input layout" in Direct3D 10 & 11 terminology)
		*  @param[in] vertexShader
		*    Vertex shader the program is using, can be a null pointer, vertex shader and program language must match!
		*  @param[in] tessellationControlShader
		*    Tessellation control shader the program is using, can be a null pointer, tessellation control shader and program language must match!
		*  @param[in] tessellationEvaluationShader
		*    Tessellation evaluation shader the program is using, can be a null pointer, tessellation evaluation shader and program language must match!
		*  @param[in] fragmentShader
		*    Fragment shader the program is using, can be a null pointer, fragment shader and program language must match!
		*
		*  @return
		*    The created program, a null pointer on error. Release the returned instance if you no longer need it.
		*
		*  @note
		*    - The program keeps a reference to the provided shaders and releases it when no longer required
		*    - It's valid that a program implementation is adding a reference and releasing it again at once
		*      (this means that in the case of not having any more references, a shader might get destroyed when calling this method)
		*    - Comfort method
		*/
		inline IProgram* createProgram(const IRootSignature& rootSignature, const VertexAttributes& vertexAttributes, IVertexShader* vertexShader, ITessellationControlShader* tessellationControlShader, ITessellationEvaluationShader* tessellationEvaluationShader, IFragmentShader* fragmentShader)
		{
			return createProgram(rootSignature, vertexAttributes, vertexShader, tessellationControlShader, tessellationEvaluationShader, nullptr, fragmentShader);
		}

	// Public virtual Renderer::IShaderLanguage methods
	public:
		/**
		*  @brief
		*    Return the name of the shader language
		*
		*  @return
		*    The ASCII name of the shader language (for example "GLSL" or "HLSL"), never a null pointer
		*
		*  @note
		*    - Do not free the memory the returned pointer is pointing to
		*/
		virtual const char* getShaderLanguageName() const = 0;

		/**
		*  @brief
		*    Create a vertex shader from shader bytecode
		*
		*  @param[in] vertexAttributes
		*    Vertex attributes ("vertex declaration" in Direct3D 9 terminology, "input layout" in Direct3D 10 & 11 terminology)
		*  @param[in] shaderBytecode
		*    Shader bytecode
		*
		*  @return
		*    The created vertex shader, a null pointer on error. Release the returned instance if you no longer need it.
		*
		*  @note
		*    - Only supported if "Renderer::Capabilities::vertexShader" is "true"
		*    - The data the given pointers are pointing to is internally copied and you have to free your memory if you no longer need it
		*/
		virtual IVertexShader* createVertexShaderFromBytecode(const VertexAttributes& vertexAttributes, const ShaderBytecode& shaderBytecode) = 0;

		/**
		*  @brief
		*    Create a vertex shader from shader source code
		*
		*  @param[in] vertexAttributes
		*    Vertex attributes ("vertex declaration" in Direct3D 9 terminology, "input layout" in Direct3D 10 & 11 terminology)
		*  @param[in] shaderSourceCode
		*    Shader source code
		*  @param[out] shaderBytecode
		*    If not a null pointer, receives the shader bytecode in case the used renderer API supports this feature
		*
		*  @return
		*    The created vertex shader, a null pointer on error. Release the returned instance if you no longer need it.
		*
		*  @remarks
		*    "profile" is not supported by each shader-API and is in general shader-API dependent. GLSL doesn't have such
		*    profiles, just something named "version" - one has to directly write into the shader. But even when this information
		*    is not used for compiling the GLSL shader, we highly recommend to provide GLSL version information in the form of e.g.
		*    "130" for OpenGL 3.0 shaders ("#version 130").
		*    Please note that the profile is just a hint, if necessary, the implementation is free to choose another profile.
		*    In general, be carefully when explicitly setting a profile.
		*
		*   "entry" is not supported by each shader-API. GLSL doesn't have such an user defined entry point and the main
		*   function must always be "main".
		*
		*   Look out! When working with shaders you have to be prepared that a shader may work on one system, but fails to even
		*   compile on another one. Sadly, even if there are e.g. official GLSL specifications, you can't be sure that every
		*   GPU driver is implementing them in detail. Here are some pitfalls which already produced some headaches...
		*
		*   When using GLSL, don't forget to provide the #version directive! Quote from
		*     "The OpenGL® Shading Language - Language Version: 3.30 - Document Revision: 6 - 11-Mar-2010" Page 14
		*       "Version 1.10 of the language does not require shaders to include this directive, and shaders that do not include
		*        a #version directive will be treated as targeting version 1.10."
		*   It looks like that AMD/ATI drivers ("AMD Catalyst™ 11.3") are NOT using just version 1.10 if there's no #version directive, but a higher
		*   version... so don't trust your GPU driver when your GLSL code, using modern language features, also works for you without
		*   #version directive, because it may not on other systems! OpenGL version and GLSL version can be a bit confusing, so
		*   here's a version table:
		*     GLSL #version    OpenGL version    Some comments
		*     110              2.0
		*     120              2.1
		*     130              3.0               Precision qualifiers added
		*                                        "attribute" deprecated; linkage between a vertex shader and OpenGL for per-vertex data -> use "in" instead
		*                                        "varying"/"centroid varying" deprecated; linkage between a vertex shader and a fragment shader for interpolated data -> use "in"/"out" instead
		*     140              3.1
		*     150              3.2               Almost feature-identical to Direct3D Shader Model 4.0 (Direct3D version 10), geometry shader added
		*     330              3.3               Equivalent to Direct3D Shader Model 4.0 (Direct3D version 10)
		*     400              4.0               Tessellation control ("Hull"-Shader in Direct3D 11) and evaluation ("Domain"-Shader in Direct3D 11) shaders added
		*     410              4.1
		*     420              4.2               Equivalent to Direct3D Shader Model 5.0 (Direct3D version 11)
		*  #version must occur before any other statement in the program as stated within:
		*    "The OpenGL® Shading Language - Language Version: 3.30 - Document Revision: 6 - 11-Mar-2010" Page 15
		*      "The #version directive must occur in a shader before anything else, except for comments and white space."
		*  ... sadly, this time NVIDIA (driver: "266.58 WHQL") is not implementing the specification in detail and while on AMD/ATI drivers ("AMD Catalyst™ 11.3")
		*  you get the error message "error(#105) #version must occur before any other statement in the program" when breaking specification,
		*  NVIDIA just accepts it without any error.
		*
		*  @note
		*    - Only supported if "Renderer::Capabilities::vertexShader" is "true"
		*    - The data the given pointers are pointing to is internally copied and you have to free your memory if you no longer need it
		*/
		virtual IVertexShader* createVertexShaderFromSourceCode(const VertexAttributes& vertexAttributes, const ShaderSourceCode& shaderSourceCode, ShaderBytecode* shaderBytecode = nullptr) = 0;

		/**
		*  @brief
		*    Create a tessellation control shader ("hull shader" in Direct3D terminology) from shader bytecode
		*
		*  @param[in] shaderBytecode
		*    Shader bytecode
		*
		*  @return
		*    The created tessellation control shader, a null pointer on error. Release the returned instance if you no longer need it.
		*
		*  @note
		*    - Only supported if "Renderer::Capabilities::maximumNumberOfPatchVertices" is not 0
		*    - The data the given pointers are pointing to is internally copied and you have to free your memory if you no longer need it
		*/
		virtual ITessellationControlShader* createTessellationControlShaderFromBytecode(const ShaderBytecode& shaderBytecode) = 0;

		/**
		*  @brief
		*    Create a tessellation control shader ("hull shader" in Direct3D terminology) from shader source code
		*
		*  @param[in] shaderSourceCode
		*    Shader source code
		*  @param[out] shaderBytecode
		*    If not a null pointer, receives the shader bytecode in case the used renderer API supports this feature
		*
		*  @return
		*    The created tessellation control shader, a null pointer on error. Release the returned instance if you no longer need it.
		*
		*  @note
		*    - Only supported if "Renderer::Capabilities::maximumNumberOfPatchVertices" is not 0
		*    - The data the given pointers are pointing to is internally copied and you have to free your memory if you no longer need it
		*
		*  @see
		*    - "Renderer::IShaderLanguage::createVertexShader()" for more information
		*/
		virtual ITessellationControlShader* createTessellationControlShaderFromSourceCode(const ShaderSourceCode& shaderSourceCode, ShaderBytecode* shaderBytecode = nullptr) = 0;

		/**
		*  @brief
		*    Create a tessellation evaluation shader ("domain shader" in Direct3D terminology) from shader bytecode
		*
		*  @param[in] shaderBytecode
		*    Shader bytecode
		*
		*  @return
		*    The created tessellation evaluation shader, a null pointer on error. Release the returned instance if you no longer need it.
		*
		*  @note
		*    - Only supported if "Renderer::Capabilities::maximumNumberOfPatchVertices" is not 0
		*    - The data the given pointers are pointing to is internally copied and you have to free your memory if you no longer need it
		*/
		virtual ITessellationEvaluationShader* createTessellationEvaluationShaderFromBytecode(const ShaderBytecode& shaderBytecode) = 0;

		/**
		*  @brief
		*    Create a tessellation evaluation shader ("domain shader" in Direct3D terminology) from shader source code
		*
		*  @param[in] shaderSourceCode
		*    Shader source code
		*  @param[out] shaderBytecode
		*    If not a null pointer, receives the shader bytecode in case the used renderer API supports this feature
		*
		*  @return
		*    The created tessellation evaluation shader, a null pointer on error. Release the returned instance if you no longer need it.
		*
		*  @note
		*    - Only supported if "Renderer::Capabilities::maximumNumberOfPatchVertices" is not 0
		*    - The data the given pointers are pointing to is internally copied and you have to free your memory if you no longer need it
		*
		*  @see
		*    - "Renderer::IShaderLanguage::createVertexShader()" for more information
		*/
		virtual ITessellationEvaluationShader* createTessellationEvaluationShaderFromSourceCode(const ShaderSourceCode& shaderSourceCode, ShaderBytecode* shaderBytecode = nullptr) = 0;

		/**
		*  @brief
		*    Create a geometry shader from shader bytecode
		*
		*  @param[in] shaderBytecode
		*    Shader bytecode
		*  @param[in] gsInputPrimitiveTopology
		*    Geometry shader input primitive topology
		*  @param[in] gsOutputPrimitiveTopology
		*    Geometry shader output primitive topology
		*  @param[in] numberOfOutputVertices
		*    Number of output vertices, maximum is "Renderer::Capabilities::maximumNumberOfGsOutputVertices"
		*
		*  @return
		*    The created geometry shader, a null pointer on error. Release the returned instance if you no longer need it.
		*
		*  @note
		*    - Only supported if "Renderer::Capabilities::maximumNumberOfGsOutputVertices" is not 0
		*    - The data the given pointers are pointing to is internally copied and you have to free your memory if you no longer need it
		*    - Please note that not each internal implementation may actually need information like "gsInputPrimitiveTopology", but it's
		*      highly recommended to provide this information anyway to be able to switch the internal implementation (e.g. using
		*      OpenGL instead of Direct3D)
		*/
		virtual IGeometryShader* createGeometryShaderFromBytecode(const ShaderBytecode& shaderBytecode, GsInputPrimitiveTopology gsInputPrimitiveTopology, GsOutputPrimitiveTopology gsOutputPrimitiveTopology, uint32_t numberOfOutputVertices) = 0;

		/**
		*  @brief
		*    Create a geometry shader from shader source code
		*
		*  @param[in] shaderSourceCode
		*    Shader source code
		*  @param[in] gsInputPrimitiveTopology
		*    Geometry shader input primitive topology
		*  @param[in] gsOutputPrimitiveTopology
		*    Geometry shader output primitive topology
		*  @param[in] numberOfOutputVertices
		*    Number of output vertices, maximum is "Renderer::Capabilities::maximumNumberOfGsOutputVertices"
		*  @param[out] shaderBytecode
		*    If not a null pointer, receives the shader bytecode in case the used renderer API supports this feature
		*
		*  @return
		*    The created geometry shader, a null pointer on error. Release the returned instance if you no longer need it.
		*
		*  @note
		*    - Only supported if "Renderer::Capabilities::maximumNumberOfGsOutputVertices" is not 0
		*    - The data the given pointers are pointing to is internally copied and you have to free your memory if you no longer need it
		*    - Please note that not each internal implementation may actually need information like "gsInputPrimitiveTopology", but it's
		*      highly recommended to provide this information anyway to be able to switch the internal implementation (e.g. using
		*      OpenGL instead of Direct3D)
		*
		*  @see
		*    - "Renderer::IShaderLanguage::createVertexShader()" for more information
		*/
		virtual IGeometryShader* createGeometryShaderFromSourceCode(const ShaderSourceCode& shaderSourceCode, GsInputPrimitiveTopology gsInputPrimitiveTopology, GsOutputPrimitiveTopology gsOutputPrimitiveTopology, uint32_t numberOfOutputVertices, ShaderBytecode* shaderBytecode = nullptr) = 0;

		/**
		*  @brief
		*    Create a fragment shader from shader bytecode
		*
		*  @param[in] shaderBytecode
		*    Shader bytecode
		*
		*  @return
		*    The created fragment shader, a null pointer on error. Release the returned instance if you no longer need it.
		*
		*  @note
		*    - Only supported if "Renderer::Capabilities::fragmentShader" is "true"
		*    - The data the given pointers are pointing to is internally copied and you have to free your memory if you no longer need it
		*/
		virtual IFragmentShader* createFragmentShaderFromBytecode(const ShaderBytecode& shaderBytecode) = 0;

		/**
		*  @brief
		*    Create a fragment shader from shader source code
		*
		*  @param[in] shaderSourceCode
		*    Shader source code
		*  @param[out] shaderBytecode
		*    If not a null pointer, receives the shader bytecode in case the used renderer API supports this feature
		*
		*  @return
		*    The created fragment shader, a null pointer on error. Release the returned instance if you no longer need it.
		*
		*  @note
		*    - Only supported if "Renderer::Capabilities::fragmentShader" is "true"
		*    - The data the given pointers are pointing to is internally copied and you have to free your memory if you no longer need it
		*
		*  @see
		*    - "Renderer::IShaderLanguage::createVertexShader()" for more information
		*/
		virtual IFragmentShader* createFragmentShaderFromSourceCode(const ShaderSourceCode& shaderSourceCode, ShaderBytecode* shaderBytecode = nullptr) = 0;

		/**
		*  @brief
		*    Create a program and assigns a vertex, tessellation control, tessellation evaluation, geometry and fragment shader to it
		*
		*  @param[in] rootSignature
		*    Root signature
		*  @param[in] vertexAttributes
		*    Vertex attributes ("vertex declaration" in Direct3D 9 terminology, "input layout" in Direct3D 10 & 11 terminology)
		*  @param[in] vertexShader
		*    Vertex shader the program is using, can be a null pointer, vertex shader and program language must match!
		*  @param[in] tessellationControlShader
		*    Tessellation control shader the program is using, can be a null pointer, tessellation control shader and program language must match!
		*  @param[in] tessellationEvaluationShader
		*    Tessellation evaluation shader the program is using, can be a null pointer, tessellation evaluation shader and program language must match!
		*  @param[in] geometryShader
		*    Geometry shader the program is using, can be a null pointer, geometry shader and program language must match!
		*  @param[in] fragmentShader
		*    Fragment shader the program is using, can be a null pointer, fragment shader and program language must match!
		*
		*  @return
		*    The created program, a null pointer on error. Release the returned instance if you no longer need it.
		*
		*  @note
		*    - The program keeps a reference to the provided shaders and releases it when no longer required,
		*      so it's safe to directly hand over a fresh created resource without releasing it manually
		*    - It's valid that a program implementation is adding a reference and releasing it again at once
		*      (this means that in the case of not having any more references, a shader might get destroyed when calling this method)
		*/
		virtual IProgram* createProgram(const IRootSignature& rootSignature, const VertexAttributes& vertexAttributes, IVertexShader* vertexShader, ITessellationControlShader* tessellationControlShader, ITessellationEvaluationShader* tessellationEvaluationShader, IGeometryShader* geometryShader, IFragmentShader* fragmentShader) = 0;

	// Protected methods
	protected:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] renderer
		*    Owner renderer instance
		*/
		inline explicit IShaderLanguage(IRenderer& renderer) :
			mRenderer(&renderer),
			mOptimizationLevel(OptimizationLevel::Ultra)
		{}

		explicit IShaderLanguage(const IShaderLanguage& source) = delete;
		IShaderLanguage& operator =(const IShaderLanguage& source) = delete;

	// Private data
	private:
		IRenderer*		  mRenderer;			///< The owner renderer instance, always valid
		OptimizationLevel mOptimizationLevel;	///< Optimization level

	};

	typedef SmartRefCount<IShaderLanguage> IShaderLanguagePtr;




	//[-------------------------------------------------------]
	//[ Renderer/IResource.h                                  ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Abstract resource interface
	*/
	class IResource : public RefCount<IResource>
	{

	// Public methods
	public:
		/**
		*  @brief
		*    Destructor
		*/
		inline virtual ~IResource() override
		{}

		/**
		*  @brief
		*    Return the resource type
		*
		*  @return
		*    The resource type
		*/
		inline ResourceType getResourceType() const
		{
			return mResourceType;
		}

		/**
		*  @brief
		*    Return the owner renderer instance
		*
		*  @return
		*    The owner renderer instance, do not release the returned instance unless you added an own reference to it
		*/
		inline IRenderer& getRenderer() const
		{
			return *mRenderer;
		}

	// Public virtual Renderer::IResource methods
	public:
		//[-------------------------------------------------------]
		//[ Debug                                                 ]
		//[-------------------------------------------------------]
		/**
		*  @brief
		*    Assign a name to the resource for debugging purposes
		*
		*  @param[in] name
		*    ASCII name for debugging purposes, must be valid (there's no internal null pointer test)
		*
		*  @see
		*    - "Renderer::IRenderer::isDebugEnabled()"
		*/
		inline virtual void setDebugName(MAYBE_UNUSED const char* name)
		{}

		//[-------------------------------------------------------]
		//[ Internal                                              ]
		//[-------------------------------------------------------]
		/**
		*  @brief
		*    Return the renderer backend specific resource handle (e.g. native Direct3D texture pointer or OpenGL texture ID)
		*
		*  @return
		*    The renderer backend specific resource handle, can be a null pointer
		*
		*  @note
		*    - Don't use this renderer backend specific method if you don't have to
		*/
		inline virtual void* getInternalResourceHandle() const
		{
			return nullptr;
		}

	// Protected methods
	protected:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] resourceType
		*    The resource type
		*
		*  @note
		*    - Only used for rare border cases, use the constructor with the renderer reference whenever possible
		*/
		inline explicit IResource(ResourceType resourceType) :
			mResourceType(resourceType),
			mRenderer(nullptr)	// Only used for rare border cases, use the constructor with the renderer reference whenever possible. Normally the renderer pointer should never ever be a null pointer. So if you're in here, you're considered to be evil.
		{}

		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] resourceType
		*    The resource type
		*  @param[in] renderer
		*    Owner renderer instance
		*/
		inline IResource(ResourceType resourceType, IRenderer& renderer) :
			mResourceType(resourceType),
			mRenderer(&renderer)
		{}

		explicit IResource(const IResource& source) = delete;
		IResource& operator =(const IResource& source) = delete;

	// Private data
	private:
		ResourceType mResourceType;	///< The resource type
		IRenderer*	 mRenderer;		///< The owner renderer instance, always valid

	};

	typedef SmartRefCount<IResource> IResourcePtr;




	//[-------------------------------------------------------]
	//[ Renderer/RootSignature.h                              ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Abstract root signature ("pipeline layout" in Vulkan terminology) interface
	*
	*  @note
	*    - Overview of the binding models of explicit APIs: "Choosing a binding model" - https://github.com/gpuweb/gpuweb/issues/19
	*/
	class IRootSignature : public IResource
	{

	// Public methods
	public:
		/**
		*  @brief
		*    Destructor
		*/
		inline virtual ~IRootSignature() override
		{
			#ifdef RENDERER_STATISTICS
				// Update the statistics
				--getRenderer().getStatistics().currentNumberOfRootSignatures;
			#endif
		}

	// Public virtual Renderer::IRootSignature methods
	public:
		/**
		*  @brief
		*    Create a resource group instance
		*
		*  @param[in] rootParameterIndex
		*    The root parameter index number for binding
		*  @param[in] numberOfResources
		*    Number of resources, having no resources is invalid
		*  @param[in] resources
		*    At least "numberOfResources" resource pointers, must be valid, the resource group will keep a reference to the resources
		*  @param[in] samplerStates
		*    If not a null pointer at least "numberOfResources" sampler state pointers, must be valid if there's at least one texture resource, the resource group will keep a reference to the sampler states
		*
		*  @return
		*    The created resource group instance, a null pointer on error. Release the returned instance if you no longer need it.
		*/
		virtual IResourceGroup* createResourceGroup(uint32_t rootParameterIndex, uint32_t numberOfResources, IResource** resources, ISamplerState** samplerStates = nullptr) = 0;

	// Protected methods
	protected:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] renderer
		*    Owner renderer instance
		*/
		inline explicit IRootSignature(IRenderer& renderer) :
			IResource(ResourceType::ROOT_SIGNATURE, renderer)
		{
			#ifdef RENDERER_STATISTICS
				// Update the statistics
				++getRenderer().getStatistics().numberOfCreatedRootSignatures;
				++getRenderer().getStatistics().currentNumberOfRootSignatures;
			#endif
		}

		explicit IRootSignature(const IRootSignature& source) = delete;
		IRootSignature& operator =(const IRootSignature& source) = delete;

	};

	typedef SmartRefCount<IRootSignature> IRootSignaturePtr;




	//[-------------------------------------------------------]
	//[ Renderer/ResourceGroup.h                              ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Abstract resource group interface
	*
	*  @note
	*    - A resource group groups resources (Vulkan descriptor set, Direct3D 12 descriptor table)
	*    - A resource group is an instance of a root descriptor table
	*    - Descriptor set comes from Vulkan while root descriptor table comes from Direct3D 12; both APIs have a similar but not identical binding model
	*    - Overview of the binding models of explicit APIs: "Choosing a binding model" - https://github.com/gpuweb/gpuweb/issues/19
	*    - Performance hint: Group resources by binding frequency and set resource groups with a low binding frequency at a low index (e.g. bind a per-pass resource group at index 0)
	*    - Compatibility hint: The number of simultaneous bound resource groups is rather low; try to stick to less or equal to four simultaneous bound resource groups, see http://vulkan.gpuinfo.org/listfeatures.php to check out GPU hardware capabilities
	*    - Compatibility hint: In Direct3D 12, samplers are not allowed in the same descriptor table as CBV/UAV/SRVs, put them into a sampler resource group
	*    - Compatibility hint: In Vulkan, one is usually using a combined image sampler, as a result a sampler resource group doesn't translate to a Vulkan sampler descriptor set.
	*                          Use sampler resource group at the highest binding indices to compensate for this.
	*/
	class IResourceGroup : public IResource
	{

	// Public methods
	public:
		/**
		*  @brief
		*    Destructor
		*/
		inline virtual ~IResourceGroup() override
		{
			#ifdef RENDERER_STATISTICS
				// Update the statistics
				--getRenderer().getStatistics().currentNumberOfResourceGroups;
			#endif
		}

	// Protected methods
	protected:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] renderer
		*    Owner renderer instance
		*/
		inline explicit IResourceGroup(IRenderer& renderer) :
			IResource(ResourceType::RESOURCE_GROUP, renderer)
		{
			#ifdef RENDERER_STATISTICS
				// Update the statistics
				++getRenderer().getStatistics().numberOfCreatedResourceGroups;
				++getRenderer().getStatistics().currentNumberOfResourceGroups;
			#endif
		}

		explicit IResourceGroup(const IResourceGroup& source) = delete;
		IResourceGroup& operator =(const IResourceGroup& source) = delete;

	};

	typedef SmartRefCount<IResourceGroup> IResourceGroupPtr;




	//[-------------------------------------------------------]
	//[ Renderer/Shader/IProgram.h                            ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Abstract program interface
	*/
	class IProgram : public IResource
	{

	// Public methods
	public:
		/**
		*  @brief
		*    Destructor
		*/
		inline virtual ~IProgram() override
		{
			#ifdef RENDERER_STATISTICS
				// Update the statistics
				--getRenderer().getStatistics().currentNumberOfPrograms;
			#endif
		}

	// Public virtual Renderer::IProgram methods
	public:
		// TODO(co) Cleanup
		inline virtual handle getUniformHandle(MAYBE_UNUSED const char* uniformName)
		{
			return NULL_HANDLE;
		}

		inline virtual void setUniform1i(MAYBE_UNUSED handle uniformHandle, MAYBE_UNUSED int value)
		{}

		inline virtual void setUniform1f(MAYBE_UNUSED handle uniformHandle, MAYBE_UNUSED float value)
		{}

		inline virtual void setUniform2fv(MAYBE_UNUSED handle uniformHandle, MAYBE_UNUSED const float* value)
		{}

		inline virtual void setUniform3fv(MAYBE_UNUSED handle uniformHandle, MAYBE_UNUSED const float* value)
		{}

		inline virtual void setUniform4fv(MAYBE_UNUSED handle uniformHandle, MAYBE_UNUSED const float* value)
		{}

		inline virtual void setUniformMatrix3fv(MAYBE_UNUSED handle uniformHandle, MAYBE_UNUSED const float* value)
		{}

		inline virtual void setUniformMatrix4fv(MAYBE_UNUSED handle uniformHandle, MAYBE_UNUSED const float* value)
		{}

	// Protected methods
	protected:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] renderer
		*    Owner renderer instance
		*/
		inline explicit IProgram(IRenderer& renderer) :
			IResource(ResourceType::PROGRAM, renderer)
		{
			#ifdef RENDERER_STATISTICS
				// Update the statistics
				++getRenderer().getStatistics().numberOfCreatedPrograms;
				++getRenderer().getStatistics().currentNumberOfPrograms;
			#endif
		}

		explicit IProgram(const IProgram& source) = delete;
		IProgram& operator =(const IProgram& source) = delete;

	};

	typedef SmartRefCount<IProgram> IProgramPtr;




	//[-------------------------------------------------------]
	//[ Renderer/RenderTarget/IRenderPass.h                   ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Abstract render pass interface
	*/
	class IRenderPass : public IResource
	{

	// Public methods
	public:
		/**
		*  @brief
		*    Destructor
		*/
		inline virtual ~IRenderPass() override
		{
			#ifdef RENDERER_STATISTICS
				// Update the statistics
				--getRenderer().getStatistics().currentNumberOfRenderPasses;
			#endif
		}

	// Protected methods
	protected:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] renderer
		*    Owner renderer instance
		*/
		inline explicit IRenderPass(IRenderer& renderer) :
			IResource(ResourceType::RENDER_PASS, renderer)
		{
			#ifdef RENDERER_STATISTICS
				// Update the statistics
				++getRenderer().getStatistics().numberOfCreatedRenderPasses;
				++getRenderer().getStatistics().currentNumberOfRenderPasses;
			#endif
		}

		explicit IRenderPass(const IRenderPass& source) = delete;
		IRenderPass& operator =(const IRenderPass& source) = delete;

	};

	typedef SmartRefCount<IRenderPass> IRenderPassPtr;




	//[-------------------------------------------------------]
	//[ Renderer/RenderTarget/IRenderTarget.h                 ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Abstract render target interface
	*/
	class IRenderTarget : public IResource
	{

	// Public methods
	public:
		/**
		*  @brief
		*    Destructor
		*/
		inline virtual ~IRenderTarget() override
		{
			mRenderPass.releaseReference();
		}

		/**
		*  @brief
		*    Return the render pass
		*
		*  @return
		*    Render pass, don't release the reference unless you add an own reference
		*/
		inline IRenderPass& getRenderPass() const
		{
			return mRenderPass;
		}

	// Public virtual Renderer::IRenderTarget methods
	public:
		/**
		*  @brief
		*    Return the width and height of the render target
		*
		*  @param[out] width
		*    Receives the width of the render target, guaranteed to be never ever zero
		*  @param[out] height
		*    Receives the height of the render target, guaranteed to be never ever zero
		*/
		virtual void getWidthAndHeight(uint32_t& width, uint32_t& height) const = 0;

	// Protected methods
	protected:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] resourceType
		*    The resource type
		*  @param[in] renderPass
		*    Render pass to use, the render target keeps a reference to the render pass
		*/
		inline IRenderTarget(ResourceType resourceType, IRenderPass& renderPass) :
			IResource(resourceType, renderPass.getRenderer()),
			mRenderPass(renderPass)
		{
			mRenderPass.addReference();
		}

		explicit IRenderTarget(const IRenderTarget& source) = delete;
		IRenderTarget& operator =(const IRenderTarget& source) = delete;

	// Private data
	private:
		IRenderPass& mRenderPass;	///< Render pass to use, the render target keeps a reference to the render pass

	};

	typedef SmartRefCount<IRenderTarget> IRenderTargetPtr;




	//[-------------------------------------------------------]
	//[ Renderer/RenderTarget/ISwapChain.h                    ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Abstract render window interface which is used to implement some platform specific functionality regarding render window needed by the swap chain implementation
	*
	*  @remarks
	*    This interface can be used to implement the needed platform specific functionality for a platform which isn't known by the renderer backend
	*    e.g. the user uses a windowing library (e.g. SDL2) which abstracts the window handling on different windowing platforms (e.g. Win32 or Linux/Wayland)
	*    and the application should run on a windowing platform which isn't supported by the swap chain implementation itself.
	*/
	class IRenderWindow
	{

	// Public methods
	public:
		/**
		*  @brief
		*    Destructor
		*/
		inline virtual ~IRenderWindow()
		{}

	// Public virtual Renderer::IRendererWindow methods
	public:
		/**
		*  @brief
		*    Return the width and height of the render window
		*
		*  @param[out] width
		*    Width of the render window
		*  @param[out] height
		*    Height of the render window
		*/
		virtual void getWidthAndHeight(uint32_t& width, uint32_t& height) const = 0;

		/**
		*  @brief
		*    Present the content of the current back buffer
		*
		*  @note
		*    - Swap of front and back buffer
		*/
		virtual void present() = 0;

	// Protected methods
	protected:
		inline IRenderWindow()
		{}

		explicit IRenderWindow(const IRenderWindow& source) = delete;
		IRenderWindow& operator =(const IRenderWindow& source) = delete;

	};

	/**
	*  @brief
	*    Abstract swap chain interface
	*/
	class ISwapChain : public IRenderTarget
	{

	// Public methods
	public:
		/**
		*  @brief
		*    Destructor
		*/
		inline virtual ~ISwapChain() override
		{
			#ifdef RENDERER_STATISTICS
				// Update the statistics
				--getRenderer().getStatistics().currentNumberOfSwapChains;
			#endif
		}

	// Public virtual Renderer::ISwapChain methods
	public:
		/**
		*  @brief
		*    Return the native window handle
		*
		*  @return
		*    Native window handle the swap chain is using as output window, can be a null handle
		*/
		virtual handle getNativeWindowHandle() const = 0;

		/**
		*  @brief
		*    Set vertical synchronization interval
		*
		*  @param[in] synchronizationInterval
		*    Synchronization interval, >0 if vertical synchronization should be used, else zero
		*/
		virtual void setVerticalSynchronizationInterval(uint32_t synchronizationInterval) = 0;

		/**
		*  @brief
		*    Present the content of the current back buffer
		*
		*  @note
		*    - Swap of front and back buffer
		*/
		virtual void present() = 0;

		/**
		*  @brief
		*    Call this method whenever the size of the native window was changed
		*/
		virtual void resizeBuffers() = 0;

		/**
		*  @brief
		*    Return the current fullscreen state
		*
		*  @return
		*    "true" if fullscreen, else "false"
		*/
		virtual bool getFullscreenState() const = 0;

		/**
		*  @brief
		*    Set the current fullscreen state
		*
		*  @param[in] fullscreen
		*    "true" if fullscreen, else "false"
		*/
		virtual void setFullscreenState(bool fullscreen) = 0;

		/**
		*  @brief
		*    Set an render window instance
		*
		*  @param[in] renderWindow
		*    The render window interface instance, can be a null pointer, if valid the instance must stay valid as long as it's connected to the swap chain instance
		*
		*  @remarks
		*    This method can be used to override the platform specific handling for retrieving window size and doing an buffer swap on the render window (aka present).
		*    An instance can be set when the user don't want that the swap chain itself tempers with the given window handle (the handle might be invalid but non zero)
		*    e.g. the user uses a windowing library (e.g. SDL2) which abstracts the window handling on different windowing platforms (e.g. Win32 or Linux/Wayland) and
		*    the application should run on a windowing platform which isn't supported by the swap chain implementation itself.
		*/
		virtual void setRenderWindow(IRenderWindow* renderWindow) = 0;

	// Protected methods
	protected:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] renderPass
		*    Render pass to use, the swap chain keeps a reference to the render pass
		*/
		inline explicit ISwapChain(IRenderPass& renderPass) :
			IRenderTarget(ResourceType::SWAP_CHAIN, renderPass)
		{
			#ifdef RENDERER_STATISTICS
				// Update the statistics
				++getRenderer().getStatistics().numberOfCreatedSwapChains;
				++getRenderer().getStatistics().currentNumberOfSwapChains;
			#endif
		}

		explicit ISwapChain(const ISwapChain& source) = delete;
		ISwapChain& operator =(const ISwapChain& source) = delete;

	};

	typedef SmartRefCount<ISwapChain> ISwapChainPtr;




	//[-------------------------------------------------------]
	//[ Renderer/RenderTarget/IFramebuffer.h                  ]
	//[-------------------------------------------------------]
	struct FramebufferAttachment final
	{
		ITexture* texture;
		uint32_t  mipmapIndex;
		uint32_t  layerIndex;	///< "slice" in Direct3D terminology, depending on the texture type it's a 2D texture array layer, 3D texture slice or cube map face
		inline FramebufferAttachment() :
			texture(nullptr),
			mipmapIndex(0),
			layerIndex(0)
		{}
		inline FramebufferAttachment(ITexture* _texture, uint32_t _mipmapIndex = 0, uint32_t _layerIndex = 0) :
			texture(_texture),
			mipmapIndex(_mipmapIndex),
			layerIndex(_layerIndex)
		{}
	};

	/**
	*  @brief
	*    Abstract framebuffer (FBO) interface
	*/
	class IFramebuffer : public IRenderTarget
	{

	// Public methods
	public:
		/**
		*  @brief
		*    Destructor
		*/
		inline virtual ~IFramebuffer() override
		{
			#ifdef RENDERER_STATISTICS
				// Update the statistics
				--getRenderer().getStatistics().currentNumberOfFramebuffers;
			#endif
		}

	// Protected methods
	protected:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] renderPass
		*    Render pass to use, the framebuffer a reference to the render pass
		*/
		inline explicit IFramebuffer(IRenderPass& renderPass) :
			IRenderTarget(ResourceType::FRAMEBUFFER, renderPass)
		{
			#ifdef RENDERER_STATISTICS
				// Update the statistics
				++getRenderer().getStatistics().numberOfCreatedFramebuffers;
				++getRenderer().getStatistics().currentNumberOfFramebuffers;
			#endif
		}

		explicit IFramebuffer(const IFramebuffer& source) = delete;
		IFramebuffer& operator =(const IFramebuffer& source) = delete;

	};

	typedef SmartRefCount<IFramebuffer> IFramebufferPtr;




	//[-------------------------------------------------------]
	//[ Renderer/Buffer/IBufferManager.h                      ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Abstract buffer manager interface
	*
	*  @remarks
	*    The buffer manager is responsible for managing fine granular instances of
	*    - Vertex buffer object ("Renderer::IVertexBuffer")
	*    - Index buffer object ("Renderer::IIndexBuffer")
	*    - Vertex array object ("Renderer::IVertexArray")
	*    - Uniform buffer object ("Renderer::IUniformBuffer")
	*    - Texture buffer object ("Renderer::ITextureBuffer")
	*    - Indirect buffer object ("Renderer::IIndirectBuffer")
	*
	*    Implementations are free to implement a naive 1:1 mapping of a resource to an renderer API resource.
	*    For AZDO ("Almost Zero Driver Overhead") implementations might allocate a few big renderer API resources
	*    and manage the granular instances internally. Modern renderer APIs like Vulkan or DirectX 12 are build
	*    around the concept that the user just allocates a few big memory heaps and then managing them by their own.
	*/
	class IBufferManager : public RefCount<IBufferManager>
	{

	// Public methods
	public:
		/**
		*  @brief
		*    Destructor
		*/
		inline virtual ~IBufferManager() override
		{}

		/**
		*  @brief
		*    Return the owner renderer instance
		*
		*  @return
		*    The owner renderer instance, do not release the returned instance unless you added an own reference to it
		*/
		inline IRenderer& getRenderer() const
		{
			return mRenderer;
		}

	// Public virtual Renderer::IBufferManager methods
	public:
		//[-------------------------------------------------------]
		//[ Resource creation                                     ]
		//[-------------------------------------------------------]
		/**
		*  @brief
		*    Create a vertex buffer object (VBO, "array buffer" in OpenGL terminology) instance
		*
		*  @param[in] numberOfBytes
		*    Number of bytes within the vertex buffer, must be valid
		*  @param[in] data
		*    Vertex buffer data, can be a null pointer (empty buffer), the data is internally copied and you have to free your memory if you no longer need it
		*  @param[in] bufferUsage
		*    Indication of the buffer usage
		*
		*  @return
		*    The created VBO instance, null pointer on error. Release the returned instance if you no longer need it.
		*/
		virtual IVertexBuffer* createVertexBuffer(uint32_t numberOfBytes, const void* data = nullptr, BufferUsage bufferUsage = BufferUsage::DYNAMIC_DRAW) = 0;

		/**
		*  @brief
		*    Create an index buffer object (IBO, "element array buffer" in OpenGL terminology) instance
		*
		*  @param[in] numberOfBytes
		*    Number of bytes within the index buffer, must be valid
		*  @param[in] indexBufferFormat
		*    Index buffer data format
		*  @param[in] data
		*    Index buffer data, can be a null pointer (empty buffer), the data is internally copied and you have to free your memory if you no longer need it
		*  @param[in] bufferUsage
		*    Indication of the buffer usage
		*
		*  @return
		*    The created IBO instance, null pointer on error. Release the returned instance if you no longer need it.
		*/
		virtual IIndexBuffer* createIndexBuffer(uint32_t numberOfBytes, IndexBufferFormat::Enum indexBufferFormat, const void* data = nullptr, BufferUsage bufferUsage = BufferUsage::DYNAMIC_DRAW) = 0;

		/**
		*  @brief
		*    Create a vertex array instance
		*
		*  @param[in] vertexAttributes
		*    Vertex attributes ("vertex declaration" in Direct3D 9 terminology, "input layout" in Direct3D 10 & 11 terminology)
		*  @param[in] numberOfVertexBuffers
		*    Number of vertex buffers, having zero vertex buffers is valid
		*  @param[in] vertexBuffers
		*    At least "numberOfVertexBuffers" instances of vertex array vertex buffers, can be a null pointer in case there are zero vertex buffers, the data is internally copied and you have to free your memory if you no longer need it
		*  @param[in] indexBuffer
		*    Optional index buffer to use, can be a null pointer, the vertex array instance keeps a reference to the index buffer
		*
		*  @return
		*    The created vertex array instance, null pointer on error. Release the returned instance if you no longer need it.
		*
		*  @note
		*    - The created vertex array instance keeps a reference to the vertex buffers used by the vertex array attributes
		*    - It's valid that a vertex array implementation is adding a reference and releasing it again at once
		*      (this means that in the case of not having any more references, a vertex buffer might get destroyed when calling this method)
		*/
		virtual IVertexArray* createVertexArray(const VertexAttributes& vertexAttributes, uint32_t numberOfVertexBuffers, const VertexArrayVertexBuffer* vertexBuffers, IIndexBuffer* indexBuffer = nullptr) = 0;

		/**
		*  @brief
		*    Create an uniform buffer object (UBO, "constant buffer" in Direct3D terminology) instance
		*
		*  @param[in] numberOfBytes
		*    Number of bytes within the uniform buffer, must be valid
		*  @param[in] data
		*    Uniform buffer data, can be a null pointer (empty buffer), the data is internally copied and you have to free your memory if you no longer need it
		*  @param[in] bufferUsage
		*    Indication of the buffer usage
		*
		*  @return
		*    The created UBO instance, null pointer on error. Release the returned instance if you no longer need it.
		*
		*  @note
		*    - Only supported if "Renderer::Capabilities::maximumUniformBufferSize" is >0
		*/
		virtual IUniformBuffer* createUniformBuffer(uint32_t numberOfBytes, const void* data = nullptr, BufferUsage bufferUsage = BufferUsage::DYNAMIC_DRAW) = 0;

		/**
		*  @brief
		*    Create an texture buffer object (TBO) instance
		*
		*  @param[in] numberOfBytes
		*    Number of bytes within the texture buffer, must be valid
		*  @param[in] textureFormat
		*    Texture buffer data format
		*  @param[in] data
		*    Texture buffer data, can be a null pointer (empty buffer), the data is internally copied and you have to free your memory if you no longer need it
		*  @param[in] bufferUsage
		*    Indication of the buffer usage
		*
		*  @return
		*    The created TBO instance, null pointer on error. Release the returned instance if you no longer need it.
		*
		*  @note
		*    - Only supported if "Renderer::Capabilities::maximumTextureBufferSize" is not 0
		*/
		virtual ITextureBuffer* createTextureBuffer(uint32_t numberOfBytes, TextureFormat::Enum textureFormat, const void* data = nullptr, BufferUsage bufferUsage = BufferUsage::DYNAMIC_DRAW) = 0;

		/**
		*  @brief
		*    Create an indirect buffer object instance
		*
		*  @param[in] numberOfBytes
		*    Number of bytes within the indirect buffer, must be valid
		*  @param[in] data
		*    Indirect buffer data, can be a null pointer (empty buffer), the data is internally copied and you have to free your memory if you no longer need it
		*  @param[in] bufferUsage
		*    Indication of the buffer usage
		*
		*  @return
		*    The created UBO instance, null pointer on error. Release the returned instance if you no longer need it.
		*
		*  @note
		*    - Only supported if "Renderer::Capabilities::maximumIndirectBufferSize" is >0
		*/
		virtual IIndirectBuffer* createIndirectBuffer(uint32_t numberOfBytes, const void* data = nullptr, BufferUsage bufferUsage = BufferUsage::DYNAMIC_DRAW) = 0;

	// Protected methods
	protected:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] renderer
		*    Owner renderer instance
		*/
		inline explicit IBufferManager(IRenderer& renderer) :
			mRenderer(renderer)
		{}

		explicit IBufferManager(const IBufferManager& source) = delete;
		IBufferManager& operator =(const IBufferManager& source) = delete;

	// Private data
	private:
		IRenderer& mRenderer;	///< The owner renderer instance

	};

	typedef SmartRefCount<IBufferManager> IBufferManagerPtr;




	//[-------------------------------------------------------]
	//[ Renderer/Buffer/IVertexArray.h                        ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Abstract vertex array object (VAO) interface
	*
	*  @remarks
	*    Encapsulates all the data that is associated with the vertex processor.
	*
	*    Direct3D specifies input stream slot indices within "vertex declaration"/"input layout" without binding them to a particular
	*    vertex buffer object (VBO). Vertex buffers are bound to particular input stream slots. OpenGL "vertex array object" (VAO) has no
	*    concept of input stream slot indices, vertex attributes are directly bound to a particular vertex buffer to feed the data into
	*    the input assembly (IA). To be able to map this interface efficiently to Direct3D as well as OpenGL, this interface has to stick
	*    to the OpenGL "vertex array object"-concept. As a result, we have to directly define vertex buffer objects as data source.
	*/
	class IVertexArray : public IResource
	{

	// Public methods
	public:
		/**
		*  @brief
		*    Destructor
		*/
		inline virtual ~IVertexArray() override
		{
			#ifdef RENDERER_STATISTICS
				// Update the statistics
				--getRenderer().getStatistics().currentNumberOfVertexArrays;
			#endif
		}

	// Protected methods
	protected:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] renderer
		*    Owner renderer instance
		*/
		inline explicit IVertexArray(IRenderer& renderer) :
			IResource(ResourceType::VERTEX_ARRAY, renderer)
		{
			#ifdef RENDERER_STATISTICS
				// Update the statistics
				++getRenderer().getStatistics().numberOfCreatedVertexArrays;
				++getRenderer().getStatistics().currentNumberOfVertexArrays;
			#endif
		}

		explicit IVertexArray(const IVertexArray& source) = delete;
		IVertexArray& operator =(const IVertexArray& source) = delete;

	};

	typedef SmartRefCount<IVertexArray> IVertexArrayPtr;




	//[-------------------------------------------------------]
	//[ Renderer/Buffer/IBuffer.h                             ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Abstract buffer interface
	*/
	class IBuffer : public IResource
	{

	// Public methods
	public:
		/**
		*  @brief
		*    Destructor
		*/
		inline virtual ~IBuffer() override
		{}

	// Protected methods
	protected:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] resourceType
		*    The resource type
		*
		*  @note
		*    - Only used for rare border cases, use the constructor with the renderer reference whenever possible
		*/
		inline explicit IBuffer(ResourceType resourceType) :
			IResource(resourceType)
		{}

		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] resourceType
		*    The resource type
		*  @param[in] renderer
		*    Owner renderer instance
		*/
		inline IBuffer(ResourceType resourceType, IRenderer& renderer) :
			IResource(resourceType, renderer)
		{}

		explicit IBuffer(const IBuffer& source) = delete;
		IBuffer& operator =(const IBuffer& source) = delete;

	};

	typedef SmartRefCount<IBuffer> IBufferPtr;




	//[-------------------------------------------------------]
	//[ Renderer/Buffer/IIndexBuffer.h                        ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Abstract index buffer object (IBO, "element array buffer" in OpenGL terminology) interface
	*/
	class IIndexBuffer : public IBuffer
	{

	// Public methods
	public:
		/**
		*  @brief
		*    Destructor
		*/
		inline virtual ~IIndexBuffer() override
		{
			#ifdef RENDERER_STATISTICS
				// Update the statistics
				--getRenderer().getStatistics().currentNumberOfIndexBuffers;
			#endif
		}

	// Protected methods
	protected:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] renderer
		*    Owner renderer instance
		*/
		inline explicit IIndexBuffer(IRenderer& renderer) :
			IBuffer(ResourceType::INDEX_BUFFER, renderer)
		{
			#ifdef RENDERER_STATISTICS
				// Update the statistics
				++getRenderer().getStatistics().numberOfCreatedIndexBuffers;
				++getRenderer().getStatistics().currentNumberOfIndexBuffers;
			#endif
		}

		explicit IIndexBuffer(const IIndexBuffer& source) = delete;
		IIndexBuffer& operator =(const IIndexBuffer& source) = delete;

	};

	typedef SmartRefCount<IIndexBuffer> IIndexBufferPtr;




	//[-------------------------------------------------------]
	//[ Renderer/Buffer/IVertexBuffer.h                       ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Abstract vertex buffer object (VBO, "array buffer" in OpenGL terminology) interface
	*/
	class IVertexBuffer : public IBuffer
	{

	// Public methods
	public:
		/**
		*  @brief
		*    Destructor
		*/
		inline virtual ~IVertexBuffer() override
		{
			#ifdef RENDERER_STATISTICS
				// Update the statistics
				--getRenderer().getStatistics().currentNumberOfVertexBuffers;
			#endif
		}

	// Protected methods
	protected:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] renderer
		*    Owner renderer instance
		*/
		inline explicit IVertexBuffer(IRenderer& renderer) :
			IBuffer(ResourceType::VERTEX_BUFFER, renderer)
		{
			#ifdef RENDERER_STATISTICS
				// Update the statistics
				++getRenderer().getStatistics().numberOfCreatedVertexBuffers;
				++getRenderer().getStatistics().currentNumberOfVertexBuffers;
			#endif
		}

		explicit IVertexBuffer(const IVertexBuffer& source) = delete;
		IVertexBuffer& operator =(const IVertexBuffer& source) = delete;

	};

	typedef SmartRefCount<IVertexBuffer> IVertexBufferPtr;




	//[-------------------------------------------------------]
	//[ Renderer/Buffer/IUniformBuffer.h                      ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Abstract uniform buffer object (UBO, "constant buffer" in Direct3D terminology) interface
	*
	*  @remarks
	*    General usage hint
	*      - Maximum size:                 64KByte (or more)
	*      - Memory access pattern:        Coherent access
	*      - Memory storage:               Usually local memory
	*    OpenGL - http://www.opengl.org/wiki/Uniform_Buffer_Object
	*      - Core in version:              4.2
	*      - Adopted into core in version: 3.1
	*      - ARB extension:                GL_ARB_Uniform_Buffer_Object
	*    Direct3D - "Shader Constants"-documentation - http://msdn.microsoft.com/en-us/library/windows/desktop/bb509581%28v=vs.85%29.aspx
	*      - Direct3D version:             10 and 11
	*      - Shader model:                 4
	*/
	class IUniformBuffer : public IBuffer
	{

	// Public methods
	public:
		/**
		*  @brief
		*    Destructor
		*/
		inline virtual ~IUniformBuffer() override
		{
			#ifdef RENDERER_STATISTICS
				// Update the statistics
				--getRenderer().getStatistics().currentNumberOfUniformBuffers;
			#endif
		}

	// Protected methods
	protected:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] renderer
		*    Owner renderer instance
		*/
		inline explicit IUniformBuffer(IRenderer& renderer) :
			IBuffer(ResourceType::UNIFORM_BUFFER, renderer)
		{
			#ifdef RENDERER_STATISTICS
				// Update the statistics
				++getRenderer().getStatistics().numberOfCreatedUniformBuffers;
				++getRenderer().getStatistics().currentNumberOfUniformBuffers;
			#endif
		}

		explicit IUniformBuffer(const IUniformBuffer& source) = delete;
		IUniformBuffer& operator =(const IUniformBuffer& source) = delete;

	};

	typedef SmartRefCount<IUniformBuffer> IUniformBufferPtr;




	//[-------------------------------------------------------]
	//[ Renderer/Buffer/ITextureBuffer.h                      ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Abstract texture buffer object (TBO) interface
	*
	*  @remarks
	*    General usage hint
	*      - Maximum size:                 128MByte (or more)
	*      - Memory access pattern:        Random access
	*      - Memory storage:               Global texture memory
	*    OpenGL - http://www.opengl.org/wiki/Buffer_Texture
	*      - Core in version:              4.2
	*      - Adopted into core in version: 3.0
	*      - ARB extension:                GL_ARB_texture_buffer_object
	*      - EXT extension:                GL_EXT_texture_buffer_object
	*    Direct3D - "Shader Constants"-documentation - http://msdn.microsoft.com/en-us/library/windows/desktop/bb509581%28v=vs.85%29.aspx
	*      - Direct3D version:             10 and 11
	*      - Shader model:                 4
	*/
	class ITextureBuffer : public IBuffer
	{

	// Public methods
	public:
		/**
		*  @brief
		*    Destructor
		*/
		inline virtual ~ITextureBuffer() override
		{
			#ifdef RENDERER_STATISTICS
				// Update the statistics
				--getRenderer().getStatistics().currentNumberOfTextureBuffers;
			#endif
		}

	// Protected methods
	protected:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] renderer
		*    Owner renderer instance
		*/
		inline explicit ITextureBuffer(IRenderer& renderer) :
			IBuffer(ResourceType::TEXTURE_BUFFER, renderer)
		{
			#ifdef RENDERER_STATISTICS
				// Update the statistics
				++getRenderer().getStatistics().numberOfCreatedTextureBuffers;
				++getRenderer().getStatistics().currentNumberOfTextureBuffers;
			#endif
		}

		explicit ITextureBuffer(const ITextureBuffer& source) = delete;
		ITextureBuffer& operator =(const ITextureBuffer& source) = delete;

	};

	typedef SmartRefCount<ITextureBuffer> ITextureBufferPtr;




	//[-------------------------------------------------------]
	//[ Renderer/Buffer/IIndirectBuffer.h                     ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Abstract indirect buffer object interface
	*
	*  @note
	*    - Contains instances of "Renderer::DrawInstancedArguments" and "Renderer::DrawIndexedInstancedArguments"
	*    - Indirect buffers where originally introduced to be able to let the GPU have some more control over draw commands,
	*      but with the introduction of multi indirect draw it became also interesting for reducing renderer API overhead (AZDO ("Almost Zero Driver Overhead"))
	*/
	class IIndirectBuffer : public IBuffer
	{

	// Public methods
	public:
		/**
		*  @brief
		*    Destructor
		*/
		inline virtual ~IIndirectBuffer() override
		{
			#ifdef RENDERER_STATISTICS
				// Update the statistics
				--getRenderer().getStatistics().currentNumberOfIndirectBuffers;
			#endif
		}

	// Public virtual Renderer::IIndirectBuffer methods
	public:
		/**
		*  @brief
		*    Return indirect buffer emulation data pointer
		*
		*  @return
		*    Indirect buffer emulation data pointer, can be a null pointer, don't destroy the returned instance
		*/
		virtual const uint8_t* getEmulationData() const = 0;

	// Protected methods
	protected:
		/**
		*  @brief
		*    Default constructor
		*
		*  @note
		*    - Only used for rare border cases, use the constructor with the renderer reference whenever possible
		*/
		inline IIndirectBuffer() :
			IBuffer(ResourceType::INDIRECT_BUFFER)
		{}

		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] renderer
		*    Owner renderer instance
		*/
		inline explicit IIndirectBuffer(IRenderer& renderer) :
			IBuffer(ResourceType::INDIRECT_BUFFER, renderer)
		{
			#ifdef RENDERER_STATISTICS
				// Update the statistics
				++getRenderer().getStatistics().numberOfCreatedIndirectBuffers;
				++getRenderer().getStatistics().currentNumberOfIndirectBuffers;
			#endif
		}

		explicit IIndirectBuffer(const IIndirectBuffer& source) = delete;
		IIndirectBuffer& operator =(const IIndirectBuffer& source) = delete;

	};

	typedef SmartRefCount<IIndirectBuffer> IIndirectBufferPtr;




	//[-------------------------------------------------------]
	//[ Renderer/Texture/ITextureManager.h                    ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Abstract texture manager interface
	*
	*  @remarks
	*    The texture manager is responsible for managing fine granular instances of
	*    - 1D texture ("Renderer::ITexture1D")
	*    - 2D texture ("Renderer::ITexture2D")
	*    - 2D texture array ("Renderer::ITexture2DArray")
	*    - 3D texture ("Renderer::ITexture3D")
	*    - Cube texture ("Renderer::ITextureCube")
	*/
	class ITextureManager : public RefCount<ITextureManager>
	{

	// Public methods
	public:
		/**
		*  @brief
		*    Destructor
		*/
		inline virtual ~ITextureManager() override
		{}

		/**
		*  @brief
		*    Return the owner renderer instance
		*
		*  @return
		*    The owner renderer instance, do not release the returned instance unless you added an own reference to it
		*/
		inline IRenderer& getRenderer() const
		{
			return mRenderer;
		}

	// Public virtual Renderer::ITextureManager methods
	public:
		//[-------------------------------------------------------]
		//[ Resource creation                                     ]
		//[-------------------------------------------------------]
		/**
		*  @brief
		*    Create a 1D texture instance
		*
		*  @param[in] width
		*    Texture width, must be >0 else a null pointer is returned
		*  @param[in] textureFormat
		*    Texture format
		*  @param[in] data
		*    Texture data, can be a null pointer, the data is internally copied and you have to free your memory if you no longer need it
		*  @param[in] flags
		*    Texture flags, see "Renderer::TextureFlag::Enum"
		*  @param[in] textureUsage
		*    Indication of the texture usage
		*
		*  @return
		*    The created 1D texture instance, null pointer on error. Release the returned instance if you no longer need it.
		*
		*  @note
		*    - The following texture data layout is expected: Mip0, Mip1, Mip2, Mip3 ...
		*/
		virtual ITexture1D* createTexture1D(uint32_t width, TextureFormat::Enum textureFormat, const void* data = nullptr, uint32_t flags = 0, TextureUsage textureUsage = TextureUsage::DEFAULT) = 0;

		/**
		*  @brief
		*    Create a 2D texture instance
		*
		*  @param[in] width
		*    Texture width, must be >0 else a null pointer is returned
		*  @param[in] height
		*    Texture height, must be >0 else a null pointer is returned
		*  @param[in] textureFormat
		*    Texture data format
		*  @param[in] data
		*    Texture data, can be a null pointer, the data is internally copied and you have to free your memory if you no longer need it
		*  @param[in] flags
		*    Texture flags, see "Renderer::TextureFlag::Enum"
		*  @param[in] textureUsage
		*    Indication of the texture usage
		*  @param[in] numberOfMultisamples
		*    The number of multisamples per pixel (valid values: 1, 2, 4, 8)
		*  @param[in] optimizedTextureClearValue
		*    Optional optimized texture clear value
		*
		*  @return
		*    The created 2D texture instance, null pointer on error. Release the returned instance if you no longer need it.
		*
		*  @note
		*    - The following texture data layout is expected: Mip0, Mip1, Mip2, Mip3 ...
		*/
		virtual ITexture2D* createTexture2D(uint32_t width, uint32_t height, TextureFormat::Enum textureFormat, const void* data = nullptr, uint32_t flags = 0, TextureUsage textureUsage = TextureUsage::DEFAULT, uint8_t numberOfMultisamples = 1, const OptimizedTextureClearValue* optimizedTextureClearValue = nullptr) = 0;

		/**
		*  @brief
		*    Create a 2D array texture instance
		*
		*  @param[in] width
		*    Texture width, must be >0 else a null pointer is returned
		*  @param[in] height
		*    Texture height, must be >0 else a null pointer is returned
		*  @param[in] numberOfSlices
		*    Number of slices, must be >0 else a null pointer is returned
		*  @param[in] textureFormat
		*    Texture format
		*  @param[in] data
		*    Texture data, can be a null pointer, the data is internally copied and you have to free your memory if you no longer need it
		*  @param[in] flags
		*    Texture flags, see "Renderer::TextureFlag::Enum"
		*  @param[in] textureUsage
		*    Indication of the texture usage
		*
		*  @return
		*    The created 2D array texture instance, null pointer on error. Release the returned instance if you no longer need it.
		*
		*  @remarks
		*    The texture array data consists of a sequence of texture slices. Each the texture slice data of a single texture slice has to
		*    be in CRN-texture layout, which means organized in mip-major order, like this:
		*    - Mip0: Slice0, Slice1, Slice2, Slice3, Slice4, Slice5
		*    - Mip1: Slice0, Slice1, Slice2, Slice3, Slice4, Slice5
		*    (DDS-texture layout is using face-major order)
		*
		*  @note
		*    - Only supported if "Renderer::Capabilities::maximumNumberOf2DTextureArraySlices" is not 0
		*/
		virtual ITexture2DArray* createTexture2DArray(uint32_t width, uint32_t height, uint32_t numberOfSlices, TextureFormat::Enum textureFormat, const void* data = nullptr, uint32_t flags = 0, TextureUsage textureUsage = TextureUsage::DEFAULT) = 0;

		/**
		*  @brief
		*    Create a 3D texture instance
		*
		*  @param[in] width
		*    Texture width, must be >0 else a null pointer is returned
		*  @param[in] height
		*    Texture height, must be >0 else a null pointer is returned
		*  @param[in] depth
		*    Texture depth, must be >0 else a null pointer is returned
		*  @param[in] textureFormat
		*    Texture format
		*  @param[in] data
		*    Texture data, can be a null pointer, the data is internally copied and you have to free your memory if you no longer need it
		*  @param[in] flags
		*    Texture flags, see "Renderer::TextureFlag::Enum"
		*  @param[in] textureUsage
		*    Indication of the texture usage
		*
		*  @return
		*    The created 3D texture instance, null pointer on error. Release the returned instance if you no longer need it.
		*
		*  @remarks
		*    The texture data has to be in CRN-texture layout, which means organized in mip-major order, like this:
		*    - Mip0: Slice0, Slice1, Slice2, Slice3, Slice4, Slice5
		*    - Mip1: Slice0, Slice1, Slice2, Slice3, Slice4, Slice5
		*    (DDS-texture layout is using face-major order)
		*/
		virtual ITexture3D* createTexture3D(uint32_t width, uint32_t height, uint32_t depth, TextureFormat::Enum textureFormat, const void* data = nullptr, uint32_t flags = 0, TextureUsage textureUsage = TextureUsage::DEFAULT) = 0;

		/**
		*  @brief
		*    Create a cube texture instance
		*
		*  @param[in] width
		*    Texture width, must be >0 else a null pointer is returned
		*  @param[in] height
		*    Texture height, must be >0 else a null pointer is returned
		*  @param[in] textureFormat
		*    Texture format
		*  @param[in] data
		*    Texture data, can be a null pointer, the data is internally copied and you have to free your memory if you no longer need it
		*  @param[in] flags
		*    Texture flags, see "Renderer::TextureFlag::Enum"
		*  @param[in] textureUsage
		*    Indication of the texture usage
		*
		*  @return
		*    The created cube texture instance, null pointer on error. Release the returned instance if you no longer need it.
		*
		*  @remarks
		*    The texture data has to be in CRN-texture layout, which means organized in mip-major order, like this:
		*    - Mip0: Face0, Face1, Face2, Face3, Face4, Face5
		*    - Mip1: Face0, Face1, Face2, Face3, Face4, Face5
		*    (DDS-texture layout is using face-major order)
		*/
		virtual ITextureCube* createTextureCube(uint32_t width, uint32_t height, TextureFormat::Enum textureFormat, const void* data = nullptr, uint32_t flags = 0, TextureUsage textureUsage = TextureUsage::DEFAULT) = 0;

	// Protected methods
	protected:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] renderer
		*    Owner renderer instance
		*/
		inline explicit ITextureManager(IRenderer& renderer) :
			mRenderer(renderer)
		{}

		explicit ITextureManager(const ITextureManager& source) = delete;
		ITextureManager& operator =(const ITextureManager& source) = delete;

	// Private data
	private:
		IRenderer& mRenderer;	///< The owner renderer instance

	};

	typedef SmartRefCount<ITextureManager> ITextureManagerPtr;




	//[-------------------------------------------------------]
	//[ Renderer/Texture/ITexture.h                           ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Abstract texture interface
	*/
	class ITexture : public IResource
	{

	// Public static methods
	public:
		/**
		*  @brief
		*    Calculate the number of mipmaps for a 1D texture
		*
		*  @param[in] width
		*    Texture width
		*
		*  @return
		*    Number of mipmaps
		*/
		static inline uint32_t getNumberOfMipmaps(uint32_t width)
		{
			// If "std::log2()" wouldn't be supported, we could write the following: "return static_cast<uint32_t>(1 + std::floor(std::log(width) / 0.69314718055994529));"
			// -> log2(x) = log(x) / log(2)
			// -> log(2) = 0.69314718055994529
			return static_cast<uint32_t>(1 + std::floor(std::log2(width)));
		}

		/**
		*  @brief
		*    Calculate the number of mipmaps for a 2D texture
		*
		*  @param[in] width
		*    Texture width
		*  @param[in] height
		*    Texture height
		*
		*  @return
		*    Number of mipmaps
		*/
		static inline uint32_t getNumberOfMipmaps(uint32_t width, uint32_t height)
		{
			return getNumberOfMipmaps((width > height) ? width : height);
		}

		/**
		*  @brief
		*    Calculate the number of mipmaps for a 3D texture
		*
		*  @param[in] width
		*    Texture width
		*  @param[in] height
		*    Texture height
		*  @param[in] depth
		*    Texture depth
		*
		*  @return
		*    Number of mipmaps
		*/
		static inline uint32_t getNumberOfMipmaps(uint32_t width, uint32_t height, uint32_t depth)
		{
			return getNumberOfMipmaps(width, (height > depth) ? height : depth);
		}

		/**
		*  @brief
		*    Calculate the half size
		*
		*  @param[in] size
		*    Size
		*
		*  @return
		*    Half size, 1 as minimum
		*/
		static inline uint32_t getHalfSize(uint32_t size)
		{
			size = (size >> 1);	// /= 2
			return (0u == size) ? 1u : size;
		}

		/**
		*  @brief
		*    Calculate the mipmap size at the given mipmap index
		*
		*  @param[in] mipmapIndex
		*    Mipmap index
		*  @param[in, out] width
		*    Texture width
		*/
		static inline void getMipmapSize(uint32_t mipmapIndex, uint32_t& width)
		{
			if (0u != mipmapIndex)
			{
				width = static_cast<uint32_t>(static_cast<float>(width) / std::exp2f(static_cast<float>(mipmapIndex)));
				if (0u == width)
				{
					width = 1u;
				}
			}
		}

		/**
		*  @brief
		*    Calculate the mipmap size at the given mipmap index
		*
		*  @param[in] mipmapIndex
		*    Mipmap index
		*  @param[in, out] width
		*    Texture width
		*  @param[in, out] height
		*    Texture height
		*/
		static inline void getMipmapSize(uint32_t mipmapIndex, uint32_t& width, uint32_t& height)
		{
			getMipmapSize(mipmapIndex, width);
			getMipmapSize(mipmapIndex, height);
		}

		/**
		*  @brief
		*    Calculate the mipmap size at the given mipmap index
		*
		*  @param[in] mipmapIndex
		*    Mipmap index
		*  @param[in, out] width
		*    Texture width
		*  @param[in, out] height
		*    Texture height
		*  @param[in, out] depth
		*    Texture depth
		*/
		static inline void getMipmapSize(uint32_t mipmapIndex, uint32_t& width, uint32_t& height, uint32_t& depth)
		{
			getMipmapSize(mipmapIndex, width);
			getMipmapSize(mipmapIndex, height);
			getMipmapSize(mipmapIndex, depth);
		}

	// Public methods
	public:
		/**
		*  @brief
		*    Destructor
		*/
		inline virtual ~ITexture() override
		{}

	// Protected methods
	protected:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] resourceType
		*    The resource type
		*  @param[in] renderer
		*    Owner renderer instance
		*/
		inline ITexture(ResourceType resourceType, IRenderer& renderer) :
			IResource(resourceType, renderer)
		{}

		explicit ITexture(const ITexture& source) = delete;
		ITexture& operator =(const ITexture& source) = delete;

	};

	typedef SmartRefCount<ITexture> ITexturePtr;




	//[-------------------------------------------------------]
	//[ Renderer/Texture/ITexture1D.h                         ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Abstract 1D texture interface
	*/
	class ITexture1D : public ITexture
	{

	// Public methods
	public:
		/**
		*  @brief
		*    Destructor
		*/
		inline virtual ~ITexture1D() override
		{
			#ifdef RENDERER_STATISTICS
				// Update the statistics
				--getRenderer().getStatistics().currentNumberOfTexture1Ds;
			#endif
		}

		/**
		*  @brief
		*    Return the width of the texture
		*
		*  @return
		*    The width of the texture
		*/
		inline uint32_t getWidth() const
		{
			return mWidth;
		}

	// Protected methods
	protected:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] renderer
		*    Owner renderer instance
		*  @param[in] width
		*    The width of the texture
		*/
		inline ITexture1D(IRenderer& renderer, uint32_t width) :
			ITexture(ResourceType::TEXTURE_1D, renderer),
			mWidth(width)
		{
			#ifdef RENDERER_STATISTICS
				// Update the statistics
				++getRenderer().getStatistics().numberOfCreatedTexture1Ds;
				++getRenderer().getStatistics().currentNumberOfTexture1Ds;
			#endif
		}

		explicit ITexture1D(const ITexture1D& source) = delete;
		ITexture1D& operator =(const ITexture1D& source) = delete;

	// Private data
	private:
		uint32_t mWidth;	///< The width of the texture

	};

	typedef SmartRefCount<ITexture1D> ITexture1DPtr;




	//[-------------------------------------------------------]
	//[ Renderer/Texture/ITexture2D.h                         ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Abstract 2D texture interface
	*/
	class ITexture2D : public ITexture
	{

	// Public methods
	public:
		/**
		*  @brief
		*    Destructor
		*/
		inline virtual ~ITexture2D() override
		{
			#ifdef RENDERER_STATISTICS
				// Update the statistics
				--getRenderer().getStatistics().currentNumberOfTexture2Ds;
			#endif
		}

		/**
		*  @brief
		*    Return the width of the texture
		*
		*  @return
		*    The width of the texture
		*/
		inline uint32_t getWidth() const
		{
			return mWidth;
		}

		/**
		*  @brief
		*    Return the height of the texture
		*
		*  @return
		*    The height of the texture
		*/
		inline uint32_t getHeight() const
		{
			return mHeight;
		}

	// Protected methods
	protected:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] renderer
		*    Owner renderer instance
		*  @param[in] width
		*    The width of the texture
		*  @param[in] height
		*    The height of the texture
		*/
		inline ITexture2D(IRenderer& renderer, uint32_t width, uint32_t height) :
			ITexture(ResourceType::TEXTURE_2D, renderer),
			mWidth(width),
			mHeight(height)
		{
			#ifdef RENDERER_STATISTICS
				// Update the statistics
				++getRenderer().getStatistics().numberOfCreatedTexture2Ds;
				++getRenderer().getStatistics().currentNumberOfTexture2Ds;
			#endif
		}

		explicit ITexture2D(const ITexture2D& source) = delete;
		ITexture2D& operator =(const ITexture2D& source) = delete;

	// Private data
	private:
		uint32_t mWidth;	///< The width of the texture
		uint32_t mHeight;	///< The height of the texture

	};

	typedef SmartRefCount<ITexture2D> ITexture2DPtr;




	//[-------------------------------------------------------]
	//[ Renderer/Texture/ITexture2DArray.h                    ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Abstract 2D array texture interface
	*/
	class ITexture2DArray : public ITexture
	{

	// Public methods
	public:
		/**
		*  @brief
		*    Destructor
		*/
		inline virtual ~ITexture2DArray() override
		{
			#ifdef RENDERER_STATISTICS
				// Update the statistics
				--getRenderer().getStatistics().currentNumberOfTexture2DArrays;
			#endif
		}

		/**
		*  @brief
		*    Return the width of the texture
		*
		*  @return
		*    The width of the texture
		*/
		inline uint32_t getWidth() const
		{
			return mWidth;
		}

		/**
		*  @brief
		*    Return the height of the texture
		*
		*  @return
		*    The height of the texture
		*/
		inline uint32_t getHeight() const
		{
			return mHeight;
		}

		/**
		*  @brief
		*    Return the number of slices
		*
		*  @return
		*    The number of slices
		*/
		inline uint32_t getNumberOfSlices() const
		{
			return mNumberOfSlices;
		}

	// Protected methods
	protected:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] renderer
		*    Owner renderer instance
		*  @param[in] width
		*    The width of the texture
		*  @param[in] height
		*    The height of the texture
		*  @param[in] numberOfSlices
		*    The number of slices
		*/
		inline ITexture2DArray(IRenderer& renderer, uint32_t width, uint32_t height, uint32_t numberOfSlices) :
			ITexture(ResourceType::TEXTURE_2D_ARRAY, renderer),
			mWidth(width),
			mHeight(height),
			mNumberOfSlices(numberOfSlices)
		{
			#ifdef RENDERER_STATISTICS
				// Update the statistics
				++getRenderer().getStatistics().numberOfCreatedTexture2DArrays;
				++getRenderer().getStatistics().currentNumberOfTexture2DArrays;
			#endif
		}

		explicit ITexture2DArray(const ITexture2DArray& source) = delete;
		ITexture2DArray& operator =(const ITexture2DArray& source) = delete;

	// Private data
	private:
		uint32_t mWidth;			///< The width of the texture
		uint32_t mHeight;			///< The height of the texture
		uint32_t mNumberOfSlices;	///< The number of slices

	};

	typedef SmartRefCount<ITexture2DArray> ITexture2DArrayPtr;




	//[-------------------------------------------------------]
	//[ Renderer/Texture/ITexture3D.h                         ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Abstract 3D texture interface
	*/
	class ITexture3D : public ITexture
	{

	// Public methods
	public:
		/**
		*  @brief
		*    Destructor
		*/
		inline virtual ~ITexture3D() override
		{
			#ifdef RENDERER_STATISTICS
				// Update the statistics
				--getRenderer().getStatistics().currentNumberOfTexture3Ds;
			#endif
		}

		/**
		*  @brief
		*    Return the width of the texture
		*
		*  @return
		*    The width of the texture
		*/
		inline uint32_t getWidth() const
		{
			return mWidth;
		}

		/**
		*  @brief
		*    Return the height of the texture
		*
		*  @return
		*    The height of the texture
		*/
		inline uint32_t getHeight() const
		{
			return mHeight;
		}

		/**
		*  @brief
		*    Return the depth of the texture
		*
		*  @return
		*    The depth of the texture
		*/
		inline uint32_t getDepth() const
		{
			return mDepth;
		}

	// Protected methods
	protected:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] renderer
		*    Owner renderer instance
		*  @param[in] width
		*    The width of the texture
		*  @param[in] height
		*    The height of the texture
		*  @param[in] depth
		*    The depth of the texture
		*/
		inline ITexture3D(IRenderer& renderer, uint32_t width, uint32_t height, uint32_t depth) :
			ITexture(ResourceType::TEXTURE_3D, renderer),
			mWidth(width),
			mHeight(height),
			mDepth(depth)
		{
			#ifdef RENDERER_STATISTICS
				// Update the statistics
				++getRenderer().getStatistics().numberOfCreatedTexture3Ds;
				++getRenderer().getStatistics().currentNumberOfTexture3Ds;
			#endif
		}

		explicit ITexture3D(const ITexture3D& source) = delete;
		ITexture3D& operator =(const ITexture3D& source) = delete;

	// Private data
	private:
		uint32_t mWidth;	///< The width of the texture
		uint32_t mHeight;	///< The height of the texture
		uint32_t mDepth;	///< The depth of the texture

	};

	typedef SmartRefCount<ITexture3D> ITexture3DPtr;




	//[-------------------------------------------------------]
	//[ Renderer/Texture/ITextureCube.h                       ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Abstract cube texture interface
	*/
	class ITextureCube : public ITexture
	{

	// Public methods
	public:
		/**
		*  @brief
		*    Destructor
		*/
		inline virtual ~ITextureCube() override
		{
			#ifdef RENDERER_STATISTICS
				// Update the statistics
				--getRenderer().getStatistics().currentNumberOfTextureCubes;
			#endif
		}

		/**
		*  @brief
		*    Return the width of the texture
		*
		*  @return
		*    The width of the texture
		*/
		inline uint32_t getWidth() const
		{
			return mWidth;
		}

		/**
		*  @brief
		*    Return the height of the texture
		*
		*  @return
		*    The height of the texture
		*/
		inline uint32_t getHeight() const
		{
			return mHeight;
		}

	// Protected methods
	protected:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] renderer
		*    Owner renderer instance
		*  @param[in] width
		*    The width of the texture
		*  @param[in] height
		*    The height of the texture
		*/
		inline ITextureCube(IRenderer& renderer, uint32_t width, uint32_t height) :
			ITexture(ResourceType::TEXTURE_CUBE, renderer),
			mWidth(width),
			mHeight(height)
		{
			#ifdef RENDERER_STATISTICS
				// Update the statistics
				++getRenderer().getStatistics().numberOfCreatedTextureCubes;
				++getRenderer().getStatistics().currentNumberOfTextureCubes;
			#endif
		}

		explicit ITextureCube(const ITextureCube& source) = delete;
		ITextureCube& operator =(const ITextureCube& source) = delete;

	// Private data
	private:
		uint32_t mWidth;	///< The width of the texture
		uint32_t mHeight;	///< The height of the texture

	};

	typedef SmartRefCount<ITextureCube> ITextureCubePtr;




	//[-------------------------------------------------------]
	//[ Renderer/State/IState.h                               ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Abstract state interface
	*/
	class IState : public IResource
	{

	// Public methods
	public:
		/**
		*  @brief
		*    Destructor
		*/
		inline virtual ~IState() override
		{}

	// Protected methods
	protected:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] resourceType
		*    The resource type
		*  @param[in] renderer
		*    Owner renderer instance
		*/
		inline IState(ResourceType resourceType, IRenderer& renderer) :
			IResource(resourceType, renderer)
		{}

		explicit IState(const IState& source) = delete;
		IState& operator =(const IState& source) = delete;

	};

	typedef SmartRefCount<IState> IStatePtr;




	//[-------------------------------------------------------]
	//[ Renderer/State/IPipelineState.h                       ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Abstract pipeline state interface
	*/
	class IPipelineState : public IState
	{

	// Public methods
	public:
		/**
		*  @brief
		*    Destructor
		*/
		inline virtual ~IPipelineState() override
		{
			#ifdef RENDERER_STATISTICS
				// Update the statistics
				--getRenderer().getStatistics().currentNumberOfPipelineStates;
			#endif
		}

	// Protected methods
	protected:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] renderer
		*    Owner renderer instance
		*/
		inline explicit IPipelineState(IRenderer& renderer) :
			IState(ResourceType::PIPELINE_STATE, renderer)
		{
			#ifdef RENDERER_STATISTICS
				// Update the statistics
				++getRenderer().getStatistics().numberOfCreatedPipelineStates;
				++getRenderer().getStatistics().currentNumberOfPipelineStates;
			#endif
		}

		explicit IPipelineState(const IPipelineState& source) = delete;
		IPipelineState& operator =(const IPipelineState& source) = delete;

	};

	typedef SmartRefCount<IPipelineState> IPipelineStatePtr;




	//[-------------------------------------------------------]
	//[ Renderer/State/ISamplerState.h                        ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Abstract sampler state interface
	*/
	class ISamplerState : public IState
	{

	// Public static methods
	public:
		/**
		*  @brief
		*    Return the default sampler state
		*
		*  @return
		*    The default sampler state, see "Renderer::SamplerState" for the default values
		*/
		static inline const SamplerState& getDefaultSamplerState()
		{
			// As default values, the one of Direct3D 11 and Direct 10 were chosen in order to make it easier for those renderer implementations
			// (choosing OpenGL default values would bring no benefit due to the design of the OpenGL API)

			// Direct3D 11 "D3D11_SAMPLER_DESC structure"-documentation at MSDN: http://msdn.microsoft.com/en-us/library/windows/desktop/ff476207%28v=vs.85%29.aspx

			// The Direct3D 10 documentation is buggy: (online and within the "Microsoft DirectX SDK (June 2010)"-SDK, checked it)
			//   - "D3D10_SAMPLER_DESC structure"-documentation at MSDN: http://msdn.microsoft.com/en-us/library/windows/desktop/bb172415%28v=vs.85%29.aspx
			//     -> Says "Default filter is Min_Mag_Mip_Point"
			//   - "ID3D10Device::VSSetSamplers method"-documentation at MSDN: msdn.microsoft.com/en-us/library/windows/desktop/bb173627(v=vs.85).aspx
			//     -> Says "Default filter is Min_Mag_Mip_Linear"
			//   -> When testing the behaviour, it "looks like" Min_Mag_Mip_Linear is used

			// Direct3D 9 "D3DSAMPLERSTATETYPE enumeration"-documentation at MSDN: http://msdn.microsoft.com/en-us/library/windows/desktop/bb172602%28v=vs.85%29.aspx

			// OpenGL & OpenGL ES 3: The official specifications (unlike Direct3D, OpenGL versions are more compatible to each other)

			// Return default values
			static constexpr SamplerState SAMPLER_STATE =
			{																					//	Direct3D 11					Direct3D 10						Direct3D 9				OpenGL
				FilterMode::MIN_MAG_MIP_LINEAR,	// filter (Renderer::FilterMode)				"MIN_MAG_MIP_LINEAR"			"MIN_MAG_MIP_LINEAR"			"MIN_MAG_MIP_POINT"		"MIN_POINT_MAG_MIP_LINEAR"
				TextureAddressMode::CLAMP,		// addressU (Renderer::TextureAddressMode)		"CLAMP"							"CLAMP"							"WRAP"					"WRAP"
				TextureAddressMode::CLAMP,		// addressV (Renderer::TextureAddressMode)		"CLAMP"							"CLAMP"							"WRAP"					"WRAP"
				TextureAddressMode::CLAMP,		// addressW (Renderer::TextureAddressMode)		"CLAMP"							"CLAMP"							"WRAP"					"WRAP"
				0.0f,							// mipLODBias (float)							"0.0f"							"0.0f"							"0.0f"					"0.0f"
				16,								// maxAnisotropy (uint32_t)						"16"							"16"							"1"						"1"
				ComparisonFunc::NEVER,			// comparisonFunc (Renderer::ComparisonFunc)	"NEVER"							"NEVER"							<unsupported>			"LESS_EQUAL"
				{
					0.0f,						// borderColor[0] (float)						"0.0f"							"0.0f"							"0.0f"					"0.0f"
					0.0f,						// borderColor[1] (float)						"0.0f"							"0.0f"							"0.0f"					"0.0f"
					0.0f,						// borderColor[2] (float)						"0.0f"							"0.0f"							"0.0f"					"0.0f"
					0.0f						// borderColor[3] (float)						"0.0f"							"0.0f"							"0.0f"					"0.0f"
				},
				-3.402823466e+38f,				// minLOD (float) - Default: -FLT_MAX			"-3.402823466e+38F (-FLT_MAX)"	"-3.402823466e+38F (-FLT_MAX)"	<unsupported>			"-1000.0f"
				3.402823466e+38f				// maxLOD (float) - Default: FLT_MAX			"3.402823466e+38F (FLT_MAX)"	"3.402823466e+38F (FLT_MAX)"	"0.0f"					"1000.0f"
			};
			return SAMPLER_STATE;
		}

	// Public methods
	public:
		/**
		*  @brief
		*    Destructor
		*/
		inline virtual ~ISamplerState() override
		{
			#ifdef RENDERER_STATISTICS
				// Update the statistics
				--getRenderer().getStatistics().currentNumberOfSamplerStates;
			#endif
		}

	// Protected methods
	protected:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] renderer
		*    Owner renderer instance
		*/
		inline explicit ISamplerState(IRenderer& renderer) :
			IState(ResourceType::SAMPLER_STATE, renderer)
		{
			#ifdef RENDERER_STATISTICS
				// Update the statistics
				++getRenderer().getStatistics().numberOfCreatedSamplerStates;
				++getRenderer().getStatistics().currentNumberOfSamplerStates;
			#endif
		}

		explicit ISamplerState(const ISamplerState& source) = delete;
		ISamplerState& operator =(const ISamplerState& source) = delete;

	};

	typedef SmartRefCount<ISamplerState> ISamplerStatePtr;




	//[-------------------------------------------------------]
	//[ Renderer/Shader/IShader.h                             ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Abstract shader interface
	*/
	class IShader : public IResource
	{

	// Public methods
	public:
		/**
		*  @brief
		*    Destructor
		*/
		inline virtual ~IShader() override
		{}

	// Public virtual Renderer::IShader methods
	public:
		/**
		*  @brief
		*    Return the name of the shader language the shader is using
		*
		*  @return
		*    The ASCII name of the shader language the shader is using (for example "GLSL" or "HLSL"), always valid
		*
		*  @note
		*    - Do not free the memory the returned pointer is pointing to
		*/
		virtual const char* getShaderLanguageName() const = 0;

	// Protected methods
	protected:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] resourceType
		*    Resource type
		*  @param[in] renderer
		*    Owner renderer instance
		*/
		inline IShader(ResourceType resourceType, IRenderer& renderer) :
			IResource(resourceType, renderer)
		{}

		explicit IShader(const IShader& source) = delete;
		IShader& operator =(const IShader& source) = delete;

	};

	typedef SmartRefCount<IShader> IShaderPtr;




	//[-------------------------------------------------------]
	//[ Renderer/Shader/IVertexShader.h                       ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Abstract vertex shader (VS) interface
	*/
	class IVertexShader : public IShader
	{

	// Public methods
	public:
		/**
		*  @brief
		*    Destructor
		*/
		inline virtual ~IVertexShader() override
		{
			#ifdef RENDERER_STATISTICS
				// Update the statistics
				--getRenderer().getStatistics().currentNumberOfVertexShaders;
			#endif
		}

	// Protected methods
	protected:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] renderer
		*    Owner renderer instance
		*/
		inline explicit IVertexShader(IRenderer& renderer) :
			IShader(ResourceType::VERTEX_SHADER, renderer)
		{
			#ifdef RENDERER_STATISTICS
				// Update the statistics
				++getRenderer().getStatistics().numberOfCreatedVertexShaders;
				++getRenderer().getStatistics().currentNumberOfVertexShaders;
			#endif
		}

		explicit IVertexShader(const IVertexShader& source) = delete;
		IVertexShader& operator =(const IVertexShader& source) = delete;

	};

	typedef SmartRefCount<IVertexShader> IVertexShaderPtr;




	//[-------------------------------------------------------]
	//[ Renderer/Shader/ITessellationControlShader.h          ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Abstract tessellation control shader (TCS, "hull shader" in Direct3D terminology) interface
	*/
	class ITessellationControlShader : public IShader
	{

	// Public methods
	public:
		/**
		*  @brief
		*    Destructor
		*/
		inline virtual ~ITessellationControlShader() override
		{
			#ifdef RENDERER_STATISTICS
				// Update the statistics
				--getRenderer().getStatistics().currentNumberOfTessellationControlShaders;
			#endif
		}

	// Protected methods
	protected:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] renderer
		*    Owner renderer instance
		*/
		inline explicit ITessellationControlShader(IRenderer& renderer) :
			IShader(ResourceType::TESSELLATION_CONTROL_SHADER, renderer)
		{
			#ifdef RENDERER_STATISTICS
				// Update the statistics
				++getRenderer().getStatistics().numberOfCreatedTessellationControlShaders;
				++getRenderer().getStatistics().currentNumberOfTessellationControlShaders;
			#endif
		}

		explicit ITessellationControlShader(const ITessellationControlShader& source) = delete;
		ITessellationControlShader& operator =(const ITessellationControlShader& source) = delete;

	};

	typedef SmartRefCount<ITessellationControlShader> ITessellationControlShaderPtr;




	//[-------------------------------------------------------]
	//[ Renderer/Shader/ITessellationEvaluationShader.h       ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Abstract tessellation evaluation shader (TES, "domain shader" in Direct3D terminology) interface
	*/
	class ITessellationEvaluationShader : public IShader
	{

	// Public methods
	public:
		/**
		*  @brief
		*    Destructor
		*/
		inline virtual ~ITessellationEvaluationShader() override
		{
			#ifdef RENDERER_STATISTICS
				// Update the statistics
				--getRenderer().getStatistics().currentNumberOfTessellationEvaluationShaders;
			#endif
		}

	// Protected methods
	protected:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] renderer
		*    Owner renderer instance
		*/
		inline explicit ITessellationEvaluationShader(IRenderer& renderer) :
			IShader(ResourceType::TESSELLATION_EVALUATION_SHADER, renderer)
		{
			#ifdef RENDERER_STATISTICS
				// Update the statistics
				++getRenderer().getStatistics().numberOfCreatedTessellationEvaluationShaders;
				++getRenderer().getStatistics().currentNumberOfTessellationEvaluationShaders;
			#endif
		}

		explicit ITessellationEvaluationShader(const ITessellationEvaluationShader& source) = delete;
		ITessellationEvaluationShader& operator =(const ITessellationEvaluationShader& source) = delete;

	};

	typedef SmartRefCount<ITessellationEvaluationShader> ITessellationEvaluationShaderPtr;




	//[-------------------------------------------------------]
	//[ Renderer/Shader/IGeometryShader.h                     ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Abstract geometry shader (GS) interface
	*/
	class IGeometryShader : public IShader
	{

	// Public methods
	public:
		/**
		*  @brief
		*    Destructor
		*/
		inline virtual ~IGeometryShader() override
		{
			#ifdef RENDERER_STATISTICS
				// Update the statistics
				--getRenderer().getStatistics().currentNumberOfGeometryShaders;
			#endif
		}

	// Protected methods
	protected:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] renderer
		*    Owner renderer instance
		*/
		inline explicit IGeometryShader(IRenderer& renderer) :
			IShader(ResourceType::GEOMETRY_SHADER, renderer)
		{
			#ifdef RENDERER_STATISTICS
				// Update the statistics
				++getRenderer().getStatistics().numberOfCreatedGeometryShaders;
				++getRenderer().getStatistics().currentNumberOfGeometryShaders;
			#endif
		}

		explicit IGeometryShader(const IGeometryShader& source) = delete;
		IGeometryShader& operator =(const IGeometryShader& source) = delete;

	};

	typedef SmartRefCount<IGeometryShader> IGeometryShaderPtr;




	//[-------------------------------------------------------]
	//[ Renderer/Shader/IFragmentShader.h                     ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Abstract fragment shader (FS, "pixel shader" in Direct3D terminology) interface
	*/
	class IFragmentShader : public IShader
	{

	// Public methods
	public:
		/**
		*  @brief
		*    Destructor
		*/
		inline virtual ~IFragmentShader() override
		{
			#ifdef RENDERER_STATISTICS
				// Update the statistics
				--getRenderer().getStatistics().currentNumberOfFragmentShaders;
			#endif
		}

	// Protected methods
	protected:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] renderer
		*    Owner renderer instance
		*/
		inline explicit IFragmentShader(IRenderer& renderer) :
			IShader(ResourceType::FRAGMENT_SHADER, renderer)
		{
			#ifdef RENDERER_STATISTICS
				// Update the statistics
				++getRenderer().getStatistics().numberOfCreatedFragmentShaders;
				++getRenderer().getStatistics().currentNumberOfFragmentShaders;
			#endif
		}

		explicit IFragmentShader(const IFragmentShader& source) = delete;
		IFragmentShader& operator =(const IFragmentShader& source) = delete;

	};

	typedef SmartRefCount<IFragmentShader> IFragmentShaderPtr;




	//[-------------------------------------------------------]
	//[ Renderer/Buffer/CommandBuffer.h                       ]
	//[-------------------------------------------------------]
	enum CommandDispatchFunctionIndex : uint8_t
	{
		// Command buffer
		ExecuteCommandBuffer = 0,
		// Graphics root
		SetGraphicsRootSignature,
		SetGraphicsResourceGroup,
		// States
		SetPipelineState,
		// Input-assembler (IA) stage
		SetVertexArray,
		// Rasterizer (RS) stage
		SetViewports,
		SetScissorRectangles,
		// Output-merger (OM) stage
		SetRenderTarget,
		// Operations
		Clear,
		ResolveMultisampleFramebuffer,
		CopyResource,
		// Draw call
		Draw,
		DrawIndexed,
		// Resource
		SetTextureMinimumMaximumMipmapIndex,
		// Debug
		SetDebugMarker,
		BeginDebugEvent,
		EndDebugEvent,
		// Done
		NumberOfFunctions
	};

	typedef void (*BackendDispatchFunction)(const void*, IRenderer& renderer);
	typedef void* CommandPacket;
	typedef const void* ConstCommandPacket;

	// Global functions
	namespace CommandPacketHelper
	{
		static constexpr uint32_t OFFSET_NEXT_COMMAND_PACKET_BYTE_INDEX	= 0u;
		static constexpr uint32_t OFFSET_BACKEND_DISPATCH_FUNCTION		= OFFSET_NEXT_COMMAND_PACKET_BYTE_INDEX + sizeof(uint32_t);
		static constexpr uint32_t OFFSET_COMMAND						= OFFSET_BACKEND_DISPATCH_FUNCTION + sizeof(uint32_t);	// Don't use "sizeof(CommandDispatchFunctionIndex)" instead of "sizeof(uint32_t)" so we have a known alignment

		template <typename T>
		inline uint32_t getNumberOfBytes(uint32_t numberOfAuxiliaryBytes)
		{
			return OFFSET_COMMAND + sizeof(T) + numberOfAuxiliaryBytes;
		};

		inline uint32_t getNextCommandPacketByteIndex(const CommandPacket commandPacket)
		{
			return *reinterpret_cast<const uint32_t*>(reinterpret_cast<const uint8_t*>(commandPacket) + OFFSET_NEXT_COMMAND_PACKET_BYTE_INDEX);
		}

		inline uint32_t getNextCommandPacketByteIndex(const ConstCommandPacket constCommandPacket)
		{
			return *reinterpret_cast<const uint32_t*>(reinterpret_cast<const uint8_t*>(constCommandPacket) + OFFSET_NEXT_COMMAND_PACKET_BYTE_INDEX);
		}

		inline void storeNextCommandPacketByteIndex(const CommandPacket commandPacket, uint32_t nextPacketByteIndex)
		{
			*reinterpret_cast<uint32_t*>(reinterpret_cast<uint8_t*>(commandPacket) + OFFSET_NEXT_COMMAND_PACKET_BYTE_INDEX) = nextPacketByteIndex;
		}

		inline CommandDispatchFunctionIndex* getCommandDispatchFunctionIndex(const CommandPacket commandPacket)
		{
			return reinterpret_cast<CommandDispatchFunctionIndex*>(reinterpret_cast<uint8_t*>(commandPacket) + OFFSET_BACKEND_DISPATCH_FUNCTION);
		}

		inline const CommandDispatchFunctionIndex* getCommandDispatchFunctionIndex(const ConstCommandPacket constCommandPacket)
		{
			return reinterpret_cast<const CommandDispatchFunctionIndex*>(reinterpret_cast<const uint8_t*>(constCommandPacket) + OFFSET_BACKEND_DISPATCH_FUNCTION);
		}

		inline void storeBackendDispatchFunctionIndex(const CommandPacket commandPacket, CommandDispatchFunctionIndex commandDispatchFunctionIndex)
		{
			*getCommandDispatchFunctionIndex(commandPacket) = commandDispatchFunctionIndex;
		}

		inline CommandDispatchFunctionIndex loadCommandDispatchFunctionIndex(const CommandPacket commandPacket)
		{
			return *getCommandDispatchFunctionIndex(commandPacket);
		}

		inline CommandDispatchFunctionIndex loadCommandDispatchFunctionIndex(const ConstCommandPacket constCommandPacket)
		{
			return *getCommandDispatchFunctionIndex(constCommandPacket);
		}

		template <typename T>
		inline T* getCommand(const CommandPacket commandPacket)
		{
			return reinterpret_cast<T*>(reinterpret_cast<uint8_t*>(commandPacket) + OFFSET_COMMAND);
		}

		inline const void* loadCommand(const CommandPacket commandPacket)
		{
			return reinterpret_cast<uint8_t*>(commandPacket) + OFFSET_COMMAND;
		}

		inline const void* loadCommand(const ConstCommandPacket constCommandPacket)
		{
			return reinterpret_cast<const uint8_t*>(constCommandPacket) + OFFSET_COMMAND;
		}

		/**
		*  @brief
		*    Return auxiliary memory address of the given command; returned memory address is considered unstable and might change as soon as another command is added
		*/
		template <typename T>
		inline uint8_t* getAuxiliaryMemory(T* command)
		{
			return reinterpret_cast<uint8_t*>(command) + sizeof(T);
		}

		/**
		*  @brief
		*    Return auxiliary memory address of the given command; returned memory address is considered unstable and might change as soon as another command is added
		*/
		template <typename T>
		inline const uint8_t* getAuxiliaryMemory(const T* command)
		{
			return reinterpret_cast<const uint8_t*>(command) + sizeof(T);
		}

	};

	/**
	*  @brief
	*    Command buffer
	*
	*  @remarks
	*    Basing on
	*    - http://molecularmusings.wordpress.com/2014/11/06/stateless-layered-multi-threaded-rendering-part-1/ - "Stateless, layered, multi-threaded rendering – Part 1"
	*    - http://molecularmusings.wordpress.com/2014/11/13/stateless-layered-multi-threaded-rendering-part-2-stateless-api-design/ - "Stateless, layered, multi-threaded rendering – Part 2"
	*    - http://molecularmusings.wordpress.com/2014/12/16/stateless-layered-multi-threaded-rendering-part-3-api-design-details/ - "Stateless, layered, multi-threaded rendering – Part 3"
	*    - http://realtimecollisiondetection.net/blog/?p=86 - "Order your graphics draw calls around!"
	*    but without a key inside the more general command buffer. Sorting is a job of a more high level construct like a render queue which also automatically will perform
	*    batching and instancing. Also the memory management is much simplified to be cache friendly.
	*
	*  @note
	*    - The commands are stored as a flat contiguous array to be cache friendly
	*    - Each command can have an additional auxiliary buffer, e.g. to store uniform buffer data to submit to the renderer
	*    - It's valid to record a command buffer only once, and submit it multiple times to the renderer
	*/
	class CommandBuffer final
	{

	// Public methods
	public:
		/**
		*  @brief
		*    Default constructor
		*/
		inline CommandBuffer() :
			mCommandPacketBufferNumberOfBytes(0),
			mCommandPacketBuffer(nullptr),
			mPreviousCommandPacketByteIndex(~0u),
			mCurrentCommandPacketByteIndex(0)
			#ifdef RENDERER_STATISTICS
				, mNumberOfCommands(0)
			#endif
		{}

		/**
		*  @brief
		*    Destructor
		*/
		inline ~CommandBuffer()
		{
			delete [] mCommandPacketBuffer;
		}

		/**
		*  @brief
		*    Return whether or not the command buffer is empty
		*
		*  @return
		*    "true" if the command buffer is empty, else "false"
		*/
		inline bool isEmpty() const
		{
			return (~0u == mPreviousCommandPacketByteIndex);
		}

		#ifdef RENDERER_STATISTICS
			/**
			*  @brief
			*    Return the number of commands inside the command buffer
			*
			*  @return
			*    The number of commands inside the command buffer
			*
			*  @note
			*    - Counting the number of commands inside the command buffer is only a debugging feature and not used in optimized builds
			*/
			inline uint32_t getNumberOfCommands() const
			{
				return mNumberOfCommands;
			}
		#endif

		/**
		*  @brief
		*    Return the command packet buffer
		*
		*  @return
		*    The command packet buffer, can be a null pointer, don't destroy the instance
		*
		*  @note
		*    - Internal, don't access the method if you don't have to
		*/
		inline const uint8_t* getCommandPacketBuffer() const
		{
			return mCommandPacketBuffer;
		}

		/**
		*  @brief
		*    Clear the command buffer
		*/
		inline void clear()
		{
			mPreviousCommandPacketByteIndex = ~0u;
			mCurrentCommandPacketByteIndex = 0;
			#ifdef RENDERER_STATISTICS
				mNumberOfCommands = 0;
			#endif
		}

		/**
		*  @brief
		*    Add command
		*
		*  @param[in] numberOfAuxiliaryBytes
		*    Optional number of auxiliary bytes, e.g. to store uniform buffer data to submit to the renderer
		*
		*  @return
		*    Pointer to the added command, only null pointer in case of apocalypse, don't destroy the memory
		*/
		template <typename U>
		U* addCommand(uint32_t numberOfAuxiliaryBytes = 0)
		{
			// How many command package buffer bytes are consumed by the command to add?
			const uint32_t numberOfCommandBytes = CommandPacketHelper::getNumberOfBytes<U>(numberOfAuxiliaryBytes);

			// 4294967295 is the maximum value of an "uint32_t"-type: Check for overflow
			// -> We use the magic number here to avoid "std::numeric_limits::max()" usage
			ASSERT((static_cast<uint64_t>(mCurrentCommandPacketByteIndex) + numberOfCommandBytes) < 4294967295u);

			// Grow command packet buffer, if required
			if (mCommandPacketBufferNumberOfBytes < mCurrentCommandPacketByteIndex + numberOfCommandBytes)
			{
				// Allocate new memory, grow using a known value but do also add the number of bytes consumed by the current command to add (many auxiliary bytes might be requested)
				const uint32_t newCommandPacketBufferNumberOfBytes = mCommandPacketBufferNumberOfBytes + NUMBER_OF_BYTES_TO_GROW + numberOfCommandBytes;
				uint8_t* newCommandPacketBuffer = new uint8_t[newCommandPacketBufferNumberOfBytes];

				// Copy over current command package buffer content and free it, if required
				if (nullptr != mCommandPacketBuffer)
				{
					memcpy(newCommandPacketBuffer, mCommandPacketBuffer, mCommandPacketBufferNumberOfBytes);
					delete [] mCommandPacketBuffer;
				}

				// Finalize
				mCommandPacketBuffer = newCommandPacketBuffer;
				mCommandPacketBufferNumberOfBytes = newCommandPacketBufferNumberOfBytes;
			}

			// Get command package for the new command
			CommandPacket commandPacket = &mCommandPacketBuffer[mCurrentCommandPacketByteIndex];

			// Setup previous and current command package
			if (~0u != mPreviousCommandPacketByteIndex)
			{
				CommandPacketHelper::storeNextCommandPacketByteIndex(&mCommandPacketBuffer[mPreviousCommandPacketByteIndex], mCurrentCommandPacketByteIndex);
			}
			CommandPacketHelper::storeNextCommandPacketByteIndex(commandPacket, ~0u);
			CommandPacketHelper::storeBackendDispatchFunctionIndex(commandPacket, U::COMMAND_DISPATCH_FUNCTION_INDEX);
			mPreviousCommandPacketByteIndex = mCurrentCommandPacketByteIndex;
			mCurrentCommandPacketByteIndex += numberOfCommandBytes;

			// Done
			#ifdef RENDERER_STATISTICS
				++mNumberOfCommands;
			#endif
			return CommandPacketHelper::getCommand<U>(commandPacket);
		}

		/**
		*  @brief
		*    Submit the command buffer to the renderer without flushing; use this for recording command buffers once and submit them multiple times
		*
		*  @param[in] renderer
		*    Renderer to submit the command buffer to
		*/
		inline void submitToRenderer(IRenderer& renderer) const
		{
			renderer.submitCommandBuffer(*this);
		}

		/**
		*  @brief
		*    Submit the command buffer to the renderer and clear so the command buffer is empty again
		*
		*  @param[in] renderer
		*    Renderer to submit the command buffer to
		*/
		inline void submitToRendererAndClear(IRenderer& renderer)
		{
			renderer.submitCommandBuffer(*this);
			clear();
		}

		/**
		*  @brief
		*    Submit the command buffer to another command buffer without flushing; use this for recording command buffers once and submit them multiple times
		*
		*  @param[in] commandBuffer
		*    Command buffer to submit the command buffer to
		*/
		inline void submitToCommandBuffer(CommandBuffer& commandBuffer) const
		{
			// Sanity check
			ASSERT((this != &commandBuffer) && "Can't submit a command buffer to itself");
			ASSERT(!isEmpty() && "Can't submit empty command buffers");

			// How many command package buffer bytes are consumed by the command to add?
			const uint32_t numberOfCommandBytes = mCurrentCommandPacketByteIndex;

			// 4294967295 is the maximum value of an "uint32_t"-type: Check for overflow
			// -> We use the magic number here to avoid "std::numeric_limits::max()" usage
			ASSERT((static_cast<uint64_t>(commandBuffer.mCurrentCommandPacketByteIndex) + numberOfCommandBytes) < 4294967295u);

			// Grow command packet buffer, if required
			if (commandBuffer.mCommandPacketBufferNumberOfBytes < commandBuffer.mCurrentCommandPacketByteIndex + numberOfCommandBytes)
			{
				// Allocate new memory, grow using a known value but do also add the number of bytes consumed by the current command to add (many auxiliary bytes might be requested)
				const uint32_t newCommandPacketBufferNumberOfBytes = commandBuffer.mCommandPacketBufferNumberOfBytes + NUMBER_OF_BYTES_TO_GROW + numberOfCommandBytes;
				uint8_t* newCommandPacketBuffer = new uint8_t[newCommandPacketBufferNumberOfBytes];

				// Copy over current command package buffer content and free it, if required
				if (nullptr != commandBuffer.mCommandPacketBuffer)
				{
					memcpy(newCommandPacketBuffer, commandBuffer.mCommandPacketBuffer, commandBuffer.mCommandPacketBufferNumberOfBytes);
					delete [] commandBuffer.mCommandPacketBuffer;
				}

				// Finalize
				commandBuffer.mCommandPacketBuffer = newCommandPacketBuffer;
				commandBuffer.mCommandPacketBufferNumberOfBytes = newCommandPacketBufferNumberOfBytes;
			}

			// Copy over the command buffer in one burst
			memcpy(&commandBuffer.mCommandPacketBuffer[commandBuffer.mCurrentCommandPacketByteIndex], mCommandPacketBuffer, mCurrentCommandPacketByteIndex);

			// Setup previous command package
			if (~0u != commandBuffer.mPreviousCommandPacketByteIndex)
			{
				CommandPacketHelper::storeNextCommandPacketByteIndex(&commandBuffer.mCommandPacketBuffer[commandBuffer.mPreviousCommandPacketByteIndex], commandBuffer.mCurrentCommandPacketByteIndex);
			}

			// Update command package indices
			CommandPacket commandPacket = &commandBuffer.mCommandPacketBuffer[commandBuffer.mCurrentCommandPacketByteIndex];
			uint32_t nextCommandPacketByteIndex = CommandPacketHelper::getNextCommandPacketByteIndex(commandPacket);
			while (~0u != nextCommandPacketByteIndex)
			{
				nextCommandPacketByteIndex = commandBuffer.mCurrentCommandPacketByteIndex + nextCommandPacketByteIndex;
				CommandPacketHelper::storeNextCommandPacketByteIndex(commandPacket, nextCommandPacketByteIndex);
				commandPacket = &commandBuffer.mCommandPacketBuffer[nextCommandPacketByteIndex];
				nextCommandPacketByteIndex = CommandPacketHelper::getNextCommandPacketByteIndex(commandPacket);
			}

			// Finalize
			commandBuffer.mPreviousCommandPacketByteIndex = commandBuffer.mCurrentCommandPacketByteIndex + mPreviousCommandPacketByteIndex;
			commandBuffer.mCurrentCommandPacketByteIndex += mCurrentCommandPacketByteIndex;
			#ifdef RENDERER_STATISTICS
				commandBuffer.mNumberOfCommands += mNumberOfCommands;
			#endif
		}

		/**
		*  @brief
		*    Submit the command buffer to another command buffer and clear so the command buffer is empty again
		*
		*  @param[in] commandBuffer
		*    Command buffer to submit the command buffer to
		*/
		inline void submitToCommandBufferAndClear(CommandBuffer& commandBuffer)
		{
			submitToCommandBuffer(commandBuffer);
			clear();
		}

	// Private definitions
	private:
		static constexpr uint32_t NUMBER_OF_BYTES_TO_GROW = 8192;

	// Private data
	private:
		// Memory
		uint32_t mCommandPacketBufferNumberOfBytes;
		uint8_t* mCommandPacketBuffer;
		// Current state
		uint32_t mPreviousCommandPacketByteIndex;
		uint32_t mCurrentCommandPacketByteIndex;
		#ifdef RENDERER_STATISTICS
			uint32_t mNumberOfCommands;
		#endif

	};

	// Concrete commands
	namespace Command
	{

		//[-------------------------------------------------------]
		//[ Command buffer                                        ]
		//[-------------------------------------------------------]
		struct ExecuteCommandBuffer final
		{
			// Static methods
			static inline void create(CommandBuffer& commandBuffer, CommandBuffer* commandBufferToExecute)
			{
				ASSERT(nullptr != commandBufferToExecute);
				*commandBuffer.addCommand<ExecuteCommandBuffer>() = ExecuteCommandBuffer(commandBufferToExecute);
			}
			// Constructor
			inline ExecuteCommandBuffer(CommandBuffer* _commandBufferToExecute) :
				commandBufferToExecute(_commandBufferToExecute)
			{}
			// Data
			CommandBuffer* commandBufferToExecute;
			// Static data
			static constexpr CommandDispatchFunctionIndex COMMAND_DISPATCH_FUNCTION_INDEX = CommandDispatchFunctionIndex::ExecuteCommandBuffer;
		};

		//[-------------------------------------------------------]
		//[ Graphics root                                         ]
		//[-------------------------------------------------------]
		/**
		*  @brief
		*    Set the used graphics root signature
		*
		*  @param[in] rootSignature
		*    Graphics root signature to use, can be an null pointer (default: "nullptr")
		*/
		struct SetGraphicsRootSignature final
		{
			// Static methods
			static inline void create(CommandBuffer& commandBuffer, IRootSignature* rootSignature)
			{
				*commandBuffer.addCommand<SetGraphicsRootSignature>() = SetGraphicsRootSignature(rootSignature);
			}
			// Constructor
			inline SetGraphicsRootSignature(IRootSignature* _rootSignature) :
				rootSignature(_rootSignature)
			{}
			// Data
			IRootSignature* rootSignature;
			// Static data
			static constexpr CommandDispatchFunctionIndex COMMAND_DISPATCH_FUNCTION_INDEX = CommandDispatchFunctionIndex::SetGraphicsRootSignature;
		};

		/**
		*  @brief
		*    Set a graphics resource group
		*
		*  @param[in] rootParameterIndex
		*    The root parameter index number for binding
		*  @param[in] resourceGroup
		*    Resource group to set
		*/
		struct SetGraphicsResourceGroup final
		{
			// Static methods
			static inline void create(CommandBuffer& commandBuffer, uint32_t rootParameterIndex, IResourceGroup* resourceGroup)
			{
				*commandBuffer.addCommand<SetGraphicsResourceGroup>() = SetGraphicsResourceGroup(rootParameterIndex, resourceGroup);
			}
			// Constructor
			inline SetGraphicsResourceGroup(uint32_t _rootParameterIndex, IResourceGroup* _resourceGroup) :
				rootParameterIndex(_rootParameterIndex),
				resourceGroup(_resourceGroup)
			{}
			// Data
			uint32_t		rootParameterIndex;
			IResourceGroup*	resourceGroup;
			// Static data
			static constexpr CommandDispatchFunctionIndex COMMAND_DISPATCH_FUNCTION_INDEX = CommandDispatchFunctionIndex::SetGraphicsResourceGroup;
		};

		//[-------------------------------------------------------]
		//[ States                                                ]
		//[-------------------------------------------------------]
		/**
		*  @brief
		*    Set the used pipeline state
		*
		*  @param[in] pipelineState
		*    Pipeline state to use, can be an null pointer (default: "nullptr")
		*/
		struct SetPipelineState final
		{
			// Static methods
			static inline void create(CommandBuffer& commandBuffer, IPipelineState* pipelineState)
			{
				*commandBuffer.addCommand<SetPipelineState>() = SetPipelineState(pipelineState);
			}
			// Constructor
			inline SetPipelineState(IPipelineState* _pipelineState) :
				pipelineState(_pipelineState)
			{}
			// Data
			IPipelineState* pipelineState;
			// Static data
			static constexpr CommandDispatchFunctionIndex COMMAND_DISPATCH_FUNCTION_INDEX = CommandDispatchFunctionIndex::SetPipelineState;
		};

		//[-------------------------------------------------------]
		//[ Input-assembler (IA) stage                            ]
		//[-------------------------------------------------------]
		/**
		*  @brief
		*    Set the used vertex array
		*
		*  @param[in] vertexArray
		*    Vertex array to use, can be an null pointer (default: "nullptr")
		*/
		struct SetVertexArray final
		{
			// Static methods
			static inline void create(CommandBuffer& commandBuffer, IVertexArray* vertexArray)
			{
				*commandBuffer.addCommand<SetVertexArray>() = SetVertexArray(vertexArray);
			}
			// Constructor
			inline SetVertexArray(IVertexArray* _vertexArray) :
				vertexArray(_vertexArray)
			{}
			// Data
			IVertexArray* vertexArray;
			// Static data
			static constexpr CommandDispatchFunctionIndex COMMAND_DISPATCH_FUNCTION_INDEX = CommandDispatchFunctionIndex::SetVertexArray;
		};

		//[-------------------------------------------------------]
		//[ Rasterizer (RS) stage                                 ]
		//[-------------------------------------------------------]
		/**
		*  @brief
		*    Set the viewports
		*
		*  @param[in] numberOfViewports
		*    Number of viewports, if <1 nothing happens, must be <="Renderer::Capabilities::maximumNumberOfViewports"
		*  @param[in] viewports
		*    C-array of viewports, there must be at least "numberOfViewports"-viewports, in case of a null pointer nothing happens
		*
		*  @note
		*    - The current viewport(s) does not affect the clear operation
		*    - Lookout! In Direct3D 12 the scissor test can't be deactivated and hence one always needs to set a valid scissor rectangle.
		*      Use the convenience "Renderer::Command::SetViewportAndScissorRectangle"-command if possible to not walk into this Direct3D 12 trap.
		*/
		struct SetViewports final
		{
			// Static methods
			static inline void create(CommandBuffer& commandBuffer, uint32_t numberOfViewports, const Viewport* viewports)
			{
				*commandBuffer.addCommand<SetViewports>() = SetViewports(numberOfViewports, viewports);
			}
			static inline void create(CommandBuffer& commandBuffer, uint32_t topLeftX, uint32_t topLeftY, uint32_t width, uint32_t height, float minimumDepth = 0.0f, float maximumDepth = 1.0f)
			{
				SetViewports* setViewportsCommand = commandBuffer.addCommand<SetViewports>(sizeof(Viewport));

				// Set command data
				Viewport* viewport = reinterpret_cast<Viewport*>(CommandPacketHelper::getAuxiliaryMemory(setViewportsCommand));
				viewport->topLeftX = static_cast<float>(topLeftX);
				viewport->topLeftY = static_cast<float>(topLeftY);
				viewport->width	   = static_cast<float>(width);
				viewport->height   = static_cast<float>(height);
				viewport->minDepth = minimumDepth;
				viewport->maxDepth = maximumDepth;

				// Finalize command
				setViewportsCommand->numberOfViewports = 1;
				setViewportsCommand->viewports		   = nullptr;
			}
			// Constructor
			inline SetViewports(uint32_t _numberOfViewports, const Viewport* _viewports) :
				numberOfViewports(_numberOfViewports),
				viewports(_viewports)
			{}
			// Data
			uint32_t		numberOfViewports;
			const Viewport* viewports;	///< If null pointer, command auxiliary memory is used instead
			// Static data
			static constexpr CommandDispatchFunctionIndex COMMAND_DISPATCH_FUNCTION_INDEX = CommandDispatchFunctionIndex::SetViewports;
		};

		/**
		*  @brief
		*    Set the scissor rectangles
		*
		*  @param[in] numberOfScissorRectangles
		*    Number of scissor rectangles, if <1 nothing happens, must be <="Renderer::Capabilities::maximumNumberOfViewports"
		*  @param[in] dcissorRectangles
		*    C-array of scissor rectangles, there must be at least "numberOfScissorRectangles" scissor rectangles, in case of a null pointer nothing happens
		*
		*  @note
		*    - Scissor rectangles are only used when "Renderer::RasterizerState::scissorEnable" is true
		*    - The current scissor rectangle(s) does not affect the clear operation
		*/
		struct SetScissorRectangles final
		{
			// Static methods
			static inline void create(CommandBuffer& commandBuffer, uint32_t numberOfScissorRectangles, const ScissorRectangle* scissorRectangles)
			{
				*commandBuffer.addCommand<SetScissorRectangles>() = SetScissorRectangles(numberOfScissorRectangles, scissorRectangles);
			}
			static inline void create(CommandBuffer& commandBuffer, long topLeftX, long topLeftY, long bottomRightX, long bottomRightY)
			{
				SetScissorRectangles* setScissorRectanglesCommand = commandBuffer.addCommand<SetScissorRectangles>(sizeof(ScissorRectangle));

				// Set command data
				ScissorRectangle* scissorRectangle = reinterpret_cast<ScissorRectangle*>(CommandPacketHelper::getAuxiliaryMemory(setScissorRectanglesCommand));
				scissorRectangle->topLeftX	   = topLeftX;
				scissorRectangle->topLeftY	   = topLeftY;
				scissorRectangle->bottomRightX = bottomRightX;
				scissorRectangle->bottomRightY = bottomRightY;

				// Finalize command
				setScissorRectanglesCommand->numberOfScissorRectangles = 1;
				setScissorRectanglesCommand->scissorRectangles		   = nullptr;
			}
			// Constructor
			inline SetScissorRectangles(uint32_t _numberOfScissorRectangles, const ScissorRectangle* _scissorRectangles) :
				numberOfScissorRectangles(_numberOfScissorRectangles),
				scissorRectangles(_scissorRectangles)
			{}
			// Data
			uint32_t				numberOfScissorRectangles;
			const ScissorRectangle* scissorRectangles;	///< If null pointer, command auxiliary memory is used instead
			// Static data
			static constexpr CommandDispatchFunctionIndex COMMAND_DISPATCH_FUNCTION_INDEX = CommandDispatchFunctionIndex::SetScissorRectangles;
		};

		/**
		*  @brief
		*    Set viewport and scissor rectangle (convenience method)
		*
		*  @param[in] topLeftX
		*    Top left x
		*  @param[in] topLeftY
		*    Top left y
		*  @param[in] width
		*    Width
		*  @param[in] height
		*    Height
		*  @param[in] minimumDepth
		*    Minimum depth
		*  @param[in] maximumDepth
		*    Maximum depth
		*
		*  @note
		*    - Lookout! In Direct3D 12 the scissor test can't be deactivated and hence one always needs to set a valid scissor rectangle.
		*      Use the convenience "Renderer::Command::SetViewportAndScissorRectangle"-command if possible to not walk into this Direct3D 12 trap.
		*/
		struct SetViewportAndScissorRectangle final
		{
			// Static methods
			static inline void create(CommandBuffer& commandBuffer, uint32_t topLeftX, uint32_t topLeftY, uint32_t width, uint32_t height, float minimumDepth = 0.0f, float maximumDepth = 1.0f)
			{
				// Set the viewport
				SetViewports::create(commandBuffer, topLeftX, topLeftY, width, height, minimumDepth, maximumDepth);

				// Set the scissor rectangle
				SetScissorRectangles::create(commandBuffer, static_cast<long>(topLeftX), static_cast<long>(topLeftY), static_cast<long>(topLeftX + width), static_cast<long>(topLeftY + height));
			}
		};

		//[-------------------------------------------------------]
		//[ Output-merger (OM) stage                              ]
		//[-------------------------------------------------------]
		/**
		*  @brief
		*    Set the render target to render into
		*
		*  @param[in] renderTarget
		*    Render target to render into by binding it to the output-merger state, can be an null pointer to render into the primary window
		*/
		struct SetRenderTarget final
		{
			// Static methods
			static inline void create(CommandBuffer& commandBuffer, IRenderTarget* renderTarget)
			{
				*commandBuffer.addCommand<SetRenderTarget>() = SetRenderTarget(renderTarget);
			}
			// Constructor
			inline SetRenderTarget(IRenderTarget* _renderTarget) :
				renderTarget(_renderTarget)
			{}
			// Data
			IRenderTarget* renderTarget;
			// Static data
			static constexpr CommandDispatchFunctionIndex COMMAND_DISPATCH_FUNCTION_INDEX = CommandDispatchFunctionIndex::SetRenderTarget;
		};

		//[-------------------------------------------------------]
		//[ Operations                                            ]
		//[-------------------------------------------------------]
		/**
		*  @brief
		*    Clears the viewport to a specified RGBA color, clears the depth buffer,
		*    and erases the stencil buffer
		*
		*  @param[in] flags
		*    Flags that indicate what should be cleared. This parameter can be any
		*    combination of the following flags, but at least one flag must be used:
		*    "Renderer::ClearFlag::COLOR", "Renderer::ClearFlag::DEPTH" and "Renderer::ClearFlag::STENCIL, see "Renderer::ClearFlag"-flags
		*  @param[in] color
		*    RGBA clear color (used if "Renderer::ClearFlag::COLOR" is set)
		*  @param[in] z
		*    Z clear value. (if "Renderer::ClearFlag::DEPTH" is set)
		*    This parameter can be in the range from 0.0 through 1.0. A value of 0.0
		*    represents the nearest distance to the viewer, and 1.0 the farthest distance.
		*  @param[in] stencil
		*    Value to clear the stencil-buffer with. This parameter can be in the range from
		*    0 through 2^n–1, where n is the bit depth of the stencil buffer.
		*
		*  @note
		*    - The current viewport(s) (see "Renderer::IRenderer::rsSetViewports()") does not affect the clear operation
		*    - The current scissor rectangle(s) (see "Renderer::IRenderer::rsSetScissorRectangles()") does not affect the clear operation
		*    - In case there are multiple active render targets, all render targets are cleared
		*/
		struct Clear final
		{
			// Static methods
			// -> z = 0 instead of 1 due to usage of Reversed-Z (see e.g. https://developer.nvidia.com/content/depth-precision-visualized and https://nlguillemot.wordpress.com/2016/12/07/reversed-z-in-opengl/)
			static inline void create(CommandBuffer& commandBuffer, uint32_t flags, const float color[4], float z = 0.0f, uint32_t stencil = 0)
			{
				*commandBuffer.addCommand<Clear>() = Clear(flags, color, z, stencil);
			}
			// Constructor
			inline Clear(uint32_t _flags, const float _color[4], float _z, uint32_t _stencil) :
				flags(_flags),
				color{_color[0], _color[1], _color[2], _color[3]},
				z(_z),
				stencil(_stencil)
			{}
			// Data
			uint32_t flags;
			float	 color[4];
			float	 z;
			uint32_t stencil;
			// Static data
			static constexpr CommandDispatchFunctionIndex COMMAND_DISPATCH_FUNCTION_INDEX = CommandDispatchFunctionIndex::Clear;
		};

		/**
		*  @brief
		*    Resolve multisample framebuffer
		*
		*  @param[in] destinationRenderTarget
		*    None multisample destination render target
		*  @param[in] sourceMultisampleFramebuffer
		*    Source multisample framebuffer
		*/
		struct ResolveMultisampleFramebuffer final
		{
			// Static methods
			static inline void create(CommandBuffer& commandBuffer, IRenderTarget& destinationRenderTarget, IFramebuffer& sourceMultisampleFramebuffer)
			{
				*commandBuffer.addCommand<ResolveMultisampleFramebuffer>() = ResolveMultisampleFramebuffer(destinationRenderTarget, sourceMultisampleFramebuffer);
			}
			// Constructor
			inline ResolveMultisampleFramebuffer(IRenderTarget& _destinationRenderTarget, IFramebuffer& _sourceMultisampleFramebuffer) :
				destinationRenderTarget(&_destinationRenderTarget),
				sourceMultisampleFramebuffer(&_sourceMultisampleFramebuffer)
			{}
			// Data
			IRenderTarget* destinationRenderTarget;
			IFramebuffer* sourceMultisampleFramebuffer;
			// Static data
			static constexpr CommandDispatchFunctionIndex COMMAND_DISPATCH_FUNCTION_INDEX = CommandDispatchFunctionIndex::ResolveMultisampleFramebuffer;
		};

		/**
		*  @brief
		*    Copy resource
		*
		*  @param[in] destinationResource
		*    Destination resource
		*  @param[in] sourceResource
		*    Source Resource
		*/
		struct CopyResource final
		{
			// Static methods
			static inline void create(CommandBuffer& commandBuffer, IResource& destinationResource, IResource& sourceResource)
			{
				*commandBuffer.addCommand<CopyResource>() = CopyResource(destinationResource, sourceResource);
			}
			// Constructor
			inline CopyResource(IResource& _destinationResource, IResource& _sourceResource) :
				destinationResource(&_destinationResource),
				sourceResource(&_sourceResource)
			{}
			// Data
			IResource* destinationResource;
			IResource* sourceResource;
			// Static data
			static constexpr CommandDispatchFunctionIndex COMMAND_DISPATCH_FUNCTION_INDEX = CommandDispatchFunctionIndex::CopyResource;
		};

		//[-------------------------------------------------------]
		//[ Draw call                                             ]
		//[-------------------------------------------------------]
		/**
		*  @brief
		*    Render the specified geometric primitive, based on an array of vertices instancing and indirect draw
		*
		*  @param[in] indirectBuffer
		*    Indirect buffer to use, the indirect buffer must contain at least "numberOfDraws" instances of "Renderer::DrawInstancedArguments" starting at "indirectBufferOffset"
		*  @param[in] indirectBufferOffset
		*    Indirect buffer offset
		*  @param[in] numberOfDraws
		*    Number of draws, can be 0
		*
		*  @note
		*    - Draw instanced is a shader model 4 feature, only supported if "Renderer::Capabilities::drawInstanced" is true
		*    - In Direct3D 9, instanced arrays with hardware support is only possible when drawing indexed primitives, see
		*      "Efficiently Drawing Multiple Instances of Geometry (Direct3D 9)"-article at MSDN: http://msdn.microsoft.com/en-us/library/windows/desktop/bb173349%28v=vs.85%29.aspx#Drawing_Non_Indexed_Geometry
		*    - Fails if no vertex array is set
		*    - If the multi-draw indirect feature is not supported this parameter, multiple draw calls are emitted
		*    - If the draw indirect feature is not supported, a software indirect buffer is used and multiple draw calls are emitted
		*/
		struct Draw final
		{
			// Static methods
			static inline void create(CommandBuffer& commandBuffer, const IIndirectBuffer& indirectBuffer, uint32_t indirectBufferOffset = 0, uint32_t numberOfDraws = 1)
			{
				*commandBuffer.addCommand<Draw>() = Draw(indirectBuffer, indirectBufferOffset, numberOfDraws);
			}
			static inline void create(CommandBuffer& commandBuffer, uint32_t vertexCountPerInstance, uint32_t instanceCount = 1, uint32_t startVertexLocation = 0, uint32_t startInstanceLocation = 0)
			{
				Draw* drawCommand = commandBuffer.addCommand<Draw>(sizeof(DrawInstancedArguments));

				// Set command data: The command packet auxiliary memory contains an "Renderer::DrawInstancedArguments"-instance
				const DrawInstancedArguments drawInstancedArguments(vertexCountPerInstance, instanceCount, startVertexLocation, startInstanceLocation);
				memcpy(CommandPacketHelper::getAuxiliaryMemory(drawCommand), &drawInstancedArguments, sizeof(DrawInstancedArguments));

				// Finalize command
				drawCommand->indirectBuffer		  = nullptr;
				drawCommand->indirectBufferOffset = 0;
				drawCommand->numberOfDraws		  = 1;
			}
			// Constructor
			inline Draw(const IIndirectBuffer& _indirectBuffer, uint32_t _indirectBufferOffset, uint32_t _numberOfDraws) :
				indirectBuffer(&_indirectBuffer),
				indirectBufferOffset(_indirectBufferOffset),
				numberOfDraws(_numberOfDraws)
			{}
			// Data
			const IIndirectBuffer* indirectBuffer;	///< If null pointer, command auxiliary memory is used instead
			uint32_t			   indirectBufferOffset;
			uint32_t			   numberOfDraws;
			// Static data
			static constexpr CommandDispatchFunctionIndex COMMAND_DISPATCH_FUNCTION_INDEX = CommandDispatchFunctionIndex::Draw;
		};

		/**
		*  @brief
		*    Render the specified geometric primitive, based on indexing into an array of vertices, instancing and indirect draw
		*
		*  @param[in] indirectBuffer
		*    Indirect buffer to use, the indirect buffer must contain at least "numberOfDraws" instances of "Renderer::DrawIndexedInstancedArguments" starting at bindirectBufferOffset"
		*  @param[in] indirectBufferOffset
		*    Indirect buffer offset
		*  @param[in] numberOfDraws
		*    Number of draws, can be 0
		*
		*  @note
		*    - Instanced arrays is a shader model 3 feature, only supported if "Renderer::Capabilities::instancedArrays" is true
		*    - Draw instanced is a shader model 4 feature, only supported if "Renderer::Capabilities::drawInstanced" is true
		*    - This method draws indexed primitives from the current set of data input streams
		*    - Fails if no index and/or vertex array is set
		*    - If the multi-draw indirect feature is not supported this parameter, multiple draw calls are emitted
		*    - If the draw indirect feature is not supported, a software indirect buffer is used and multiple draw calls are emitted
		*/
		struct DrawIndexed final
		{
			// Static methods
			static inline void create(CommandBuffer& commandBuffer, const IIndirectBuffer& indirectBuffer, uint32_t indirectBufferOffset = 0, uint32_t numberOfDraws = 1)
			{
				*commandBuffer.addCommand<DrawIndexed>() = DrawIndexed(indirectBuffer, indirectBufferOffset, numberOfDraws);
			}
			static inline void create(CommandBuffer& commandBuffer, uint32_t indexCountPerInstance, uint32_t instanceCount = 1, uint32_t startIndexLocation = 0, int32_t baseVertexLocation = 0, uint32_t startInstanceLocation = 0)
			{
				DrawIndexed* drawCommand = commandBuffer.addCommand<DrawIndexed>(sizeof(DrawIndexedInstancedArguments));

				// Set command data: The command packet auxiliary memory contains an "Renderer::DrawIndexedInstancedArguments"-instance
				const DrawIndexedInstancedArguments drawIndexedInstancedArguments(indexCountPerInstance, instanceCount, startIndexLocation, baseVertexLocation, startInstanceLocation);
				memcpy(CommandPacketHelper::getAuxiliaryMemory(drawCommand), &drawIndexedInstancedArguments, sizeof(DrawIndexedInstancedArguments));

				// Finalize command
				drawCommand->indirectBuffer		  = nullptr;
				drawCommand->indirectBufferOffset = 0;
				drawCommand->numberOfDraws		  = 1;
			}
			// Constructor
			inline DrawIndexed(const IIndirectBuffer& _indirectBuffer, uint32_t _indirectBufferOffset, uint32_t _numberOfDraws) :
				indirectBuffer(&_indirectBuffer),
				indirectBufferOffset(_indirectBufferOffset),
				numberOfDraws(_numberOfDraws)
			{}
			// Data
			const IIndirectBuffer* indirectBuffer;	///< If null pointer, command auxiliary memory is used instead
			uint32_t			   indirectBufferOffset;
			uint32_t			   numberOfDraws;
			// Static data
			static constexpr CommandDispatchFunctionIndex COMMAND_DISPATCH_FUNCTION_INDEX = CommandDispatchFunctionIndex::DrawIndexed;
		};

		//[-------------------------------------------------------]
		//[ Resource                                              ]
		//[-------------------------------------------------------]
		/**
		*  @brief
		*    Set texture minimum maximum mipmap index
		*
		*  @param[in] texture
		*    Texture to set the minimum maximum mipmap index of
		*  @param[in] minimumMipmapIndex
		*    Minimum mipmap index, the most detailed mipmap, also known as base mipmap, 0 by default
		*  @param[in] maximumMipmapIndex
		*    Maximum mipmap index, the least detailed mipmap, <number of mipmaps> by default
		*/
		struct SetTextureMinimumMaximumMipmapIndex final
		{
			// Static methods
			static inline void create(CommandBuffer& commandBuffer, ITexture& texture, uint32_t minimumMipmapIndex, uint32_t maximumMipmapIndex)
			{
				*commandBuffer.addCommand<SetTextureMinimumMaximumMipmapIndex>() = SetTextureMinimumMaximumMipmapIndex(texture, minimumMipmapIndex, maximumMipmapIndex);
			}
			// Constructor
			inline SetTextureMinimumMaximumMipmapIndex(ITexture& _texture, uint32_t _minimumMipmapIndex, uint32_t _maximumMipmapIndex) :
				texture(&_texture),
				minimumMipmapIndex(_minimumMipmapIndex),
				maximumMipmapIndex(_maximumMipmapIndex)
			{}
			// Data
			ITexture* texture;
			uint32_t  minimumMipmapIndex;
			uint32_t  maximumMipmapIndex;
			// Static data
			static constexpr CommandDispatchFunctionIndex COMMAND_DISPATCH_FUNCTION_INDEX = CommandDispatchFunctionIndex::SetTextureMinimumMaximumMipmapIndex;
		};

		//[-------------------------------------------------------]
		//[ Debug                                                 ]
		//[-------------------------------------------------------]
		/**
		*  @brief
		*    Set a debug marker
		*
		*  @param[in] name
		*    ASCII name of the debug marker, must be valid (there's no internal null pointer test)
		*
		*  @see
		*    - "isDebugEnabled()"
		*/
		struct SetDebugMarker final
		{
			// Static methods
			static inline void create(CommandBuffer& commandBuffer, const char* name)
			{
				*commandBuffer.addCommand<SetDebugMarker>() = SetDebugMarker(name);
			}
			// Constructor
			inline SetDebugMarker(const char* _name)
			{
				ASSERT(strlen(_name) < 128);
				strncpy(name, _name, 128);
				name[127] = '\0';
			};
			// Data
			char name[128];
			// Static data
			static constexpr CommandDispatchFunctionIndex COMMAND_DISPATCH_FUNCTION_INDEX = CommandDispatchFunctionIndex::SetDebugMarker;
		};

		/**
		*  @brief
		*    Begin debug event
		*
		*  @param[in] name
		*    ASCII name of the debug event, must be valid (there's no internal null pointer test)
		*
		*  @see
		*    - "isDebugEnabled()"
		*/
		struct BeginDebugEvent final
		{
			// Static methods
			static inline void create(CommandBuffer& commandBuffer, const char* name)
			{
				*commandBuffer.addCommand<BeginDebugEvent>() = BeginDebugEvent(name);
			}
			// Constructor
			inline BeginDebugEvent(const char* _name)
			{
				ASSERT(strlen(_name) < 128);
				strncpy(name, _name, 128);
				name[127] = '\0';
			};
			// Data
			char name[128];
			// Static data
			static constexpr CommandDispatchFunctionIndex COMMAND_DISPATCH_FUNCTION_INDEX = CommandDispatchFunctionIndex::BeginDebugEvent;
		};

		/**
		*  @brief
		*    End the last started debug event
		*
		*  @see
		*    - "isDebugEnabled()"
		*/
		struct EndDebugEvent final
		{
			// Static methods
			static inline void create(CommandBuffer& commandBuffer)
			{
				commandBuffer.addCommand<EndDebugEvent>();
			}
			// Static data
			static constexpr CommandDispatchFunctionIndex COMMAND_DISPATCH_FUNCTION_INDEX = CommandDispatchFunctionIndex::EndDebugEvent;
		};

	}

	// Debug macros
	#ifdef RENDERER_DEBUG
		/**
		*  @brief
		*    Set a debug marker
		*
		*  @param[in] commandBuffer
		*    Reference to the renderer instance to use
		*  @param[in] name
		*    ASCII name of the debug marker
		*/
		#define COMMAND_SET_DEBUG_MARKER(commandBuffer, name) Renderer::Command::SetDebugMarker::create(commandBuffer, name);

		/**
		*  @brief
		*    Set a debug marker by using the current function name ("__FUNCTION__") as marker name
		*
		*  @param[in] commandBuffer
		*    Reference to the renderer instance to use
		*/
		#define COMMAND_SET_DEBUG_MARKER_FUNCTION(commandBuffer) Renderer::Command::SetDebugMarker::create(commandBuffer, __FUNCTION__);

		/**
		*  @brief
		*    Begin debug event
		*
		*  @param[in] commandBuffer
		*    Reference to the renderer instance to use
		*  @param[in] name
		*    ASCII name of the debug event
		*/
		#define COMMAND_BEGIN_DEBUG_EVENT(commandBuffer, name) Renderer::Command::BeginDebugEvent::create(commandBuffer, name);

		/**
		*  @brief
		*    Begin debug event by using the current function name ("__FUNCTION__") as event name
		*
		*  @param[in] commandBuffer
		*    Reference to the renderer instance to use
		*/
		#define COMMAND_BEGIN_DEBUG_EVENT_FUNCTION(commandBuffer) Renderer::Command::BeginDebugEvent::create(commandBuffer, __FUNCTION__);

		/**
		*  @brief
		*    End the last started debug event
		*
		*  @param[in] commandBuffer
		*    Reference to the renderer instance to use
		*/
		#define COMMAND_END_DEBUG_EVENT(commandBuffer) Renderer::Command::EndDebugEvent::create(commandBuffer);
	#else
		/**
		*  @brief
		*    Set a debug marker
		*
		*  @param[in] commandBuffer
		*    Reference to the renderer instance to use
		*  @param[in] name
		*    ASCII name of the debug marker
		*/
		#define COMMAND_SET_DEBUG_MARKER(commandBuffer, name)

		/**
		*  @brief
		*    Set a debug marker by using the current function name ("__FUNCTION__") as marker name
		*
		*  @param[in] commandBuffer
		*    Reference to the renderer instance to use
		*/
		#define COMMAND_SET_DEBUG_MARKER_FUNCTION(commandBuffer)

		/**
		*  @brief
		*    Begin debug event
		*
		*  @param[in] commandBuffer
		*    Reference to the renderer instance to use
		*  @param[in] name
		*    ASCII name of the debug event
		*/
		#define COMMAND_BEGIN_DEBUG_EVENT(commandBuffer, name)

		/**
		*  @brief
		*    Begin debug event by using the current function name ("__FUNCTION__") as event name
		*
		*  @param[in] commandBuffer
		*    Reference to the renderer instance to use
		*/
		#define COMMAND_BEGIN_DEBUG_EVENT_FUNCTION(commandBuffer)

		/**
		*  @brief
		*    End the last started debug event
		*
		*  @param[in] commandBuffer
		*    Reference to the renderer instance to use
		*/
		#define COMMAND_END_DEBUG_EVENT(commandBuffer)
	#endif




//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
} // Renderer




//[-------------------------------------------------------]
//[ Debug macros                                          ]
//[-------------------------------------------------------]
#ifdef RENDERER_DEBUG
	/**
	*  @brief
	*    Set a debug marker
	*
	*  @param[in] renderer
	*    Pointer to the renderer instance to use, can be a null pointer
	*  @param[in] name
	*    ASCII name of the debug marker
	*
	*  @note
	*    - Only for renderer backend internal usage, don't expose it inside the public renderer header
	*/
	#define RENDERER_SET_DEBUG_MARKER(renderer, name) if (nullptr != renderer) { (renderer)->setDebugMarker(name); }

	/**
	*  @brief
	*    Set a debug marker by using the current function name ("__FUNCTION__") as marker name
	*
	*  @param[in] renderer
	*    Pointer to the renderer instance to use, can be a null pointer
	*
	*  @note
	*    - Only for renderer backend internal usage, don't expose it inside the public renderer header
	*/
	#define RENDERER_SET_DEBUG_MARKER_FUNCTION(renderer) if (nullptr != renderer) { (renderer)->setDebugMarker(__FUNCTION__); }

	/**
	*  @brief
	*    Begin debug event
	*
	*  @param[in] renderer
	*    Pointer to the renderer instance to use, can be a null pointer
	*  @param[in] name
	*    ASCII name of the debug event
	*
	*  @note
	*    - Only for renderer backend internal usage, don't expose it inside the public renderer header
	*/
	#define RENDERER_BEGIN_DEBUG_EVENT(renderer, name) if (nullptr != renderer) { (renderer)->beginDebugEvent(name); }

	/**
	*  @brief
	*    Begin debug event by using the current function name ("__FUNCTION__") as event name
	*
	*  @param[in] renderer
	*    Pointer to the renderer instance to use, can be a null pointer
	*
	*  @note
	*    - Only for renderer backend internal usage, don't expose it inside the public renderer header
	*/
	#define RENDERER_BEGIN_DEBUG_EVENT_FUNCTION(renderer) if (nullptr != renderer) { (renderer)->beginDebugEvent(__FUNCTION__); }

	/**
	*  @brief
	*    End the last started debug event
	*
	*  @param[in] renderer
	*    Pointer to the renderer instance to use, can be a null pointer
	*
	*  @note
	*    - Only for renderer backend internal usage, don't expose it inside the public renderer header
	*/
	#define RENDERER_END_DEBUG_EVENT(renderer) if (nullptr != renderer) { (renderer)->endDebugEvent(); }

	/**
	*  @brief
	*    Assign a name to a given resource for debugging purposes
	*
	*  @param[in] resource
	*    Resource to assign the debug name to, can be a null pointer
	*  @param[in] name
	*    ASCII name for debugging purposes, must be valid (there's no internal null pointer test)
	*/
	#define RENDERER_SET_RESOURCE_DEBUG_NAME(resource, name) if (nullptr != resource) { (resource)->setDebugName(name); }

	/**
	*  @brief
	*    Decorate the debug name to make it easier to see the semantic of the resource
	*
	*  @param[in] name
	*    Debug name provided from the outside
	*  @param[in] decoration
	*    Decoration to append in front (e.g. "IBO", will result in appended "IBO: " in front if the provided name isn't empty)
	*  @param[in] numberOfDecorationCharacters
	*    Number of decoration characters
	*
	*  @note
	*    - The result is in local string variable "detailedName"
	*    - Traditional C-string on the runtime stack used for efficiency reasons (just for debugging, but must still be some kind of usable)
	*/
	#define RENDERER_DECORATED_DEBUG_NAME(name, detailedName, decoration, numberOfDecorationCharacters) \
		RENDERER_ASSERT(getRenderer().getContext(), strlen(name) < 256, "Name is not allowed to exceed 255 characters") \
		char detailedName[256 + numberOfDecorationCharacters] = decoration; \
		if (name[0] != '\0') \
		{ \
			strcat(detailedName, ": "); \
			strncat(detailedName, name, 256); \
		}
#else
	/**
	*  @brief
	*    Set a debug marker
	*
	*  @param[in] renderer
	*    Pointer to the renderer instance to use, can be a null pointer
	*  @param[in] name
	*    ASCII name of the debug marker
	*
	*  @note
	*    - Only for renderer backend internal usage, don't expose it inside the public renderer header
	*/
	#define RENDERER_SET_DEBUG_MARKER(renderer, name)

	/**
	*  @brief
	*    Set a debug marker by using the current function name ("__FUNCTION__") as marker name
	*
	*  @param[in] renderer
	*    Pointer to the renderer instance to use, can be a null pointer
	*
	*  @note
	*    - Only for renderer backend internal usage, don't expose it inside the public renderer header
	*/
	#define RENDERER_SET_DEBUG_MARKER_FUNCTION(renderer)

	/**
	*  @brief
	*    Begin debug event
	*
	*  @param[in] renderer
	*    Pointer to the renderer instance to use, can be a null pointer
	*  @param[in] name
	*    ASCII name of the debug event
	*
	*  @note
	*    - Only for renderer backend internal usage, don't expose it inside the public renderer header
	*/
	#define RENDERER_BEGIN_DEBUG_EVENT(renderer, name)

	/**
	*  @brief
	*    Begin debug event by using the current function name ("__FUNCTION__") as event name
	*
	*  @param[in] renderer
	*    Pointer to the renderer instance to use, can be a null pointer
	*
	*  @note
	*    - Only for renderer backend internal usage, don't expose it inside the public renderer header
	*/
	#define RENDERER_BEGIN_DEBUG_EVENT_FUNCTION(renderer)

	/**
	*  @brief
	*    End the last started debug event
	*
	*  @param[in] renderer
	*    Pointer to the renderer instance to use, can be a null pointer
	*
	*  @note
	*    - Only for renderer backend internal usage, don't expose it inside the public renderer header
	*/
	#define RENDERER_END_DEBUG_EVENT(renderer)

	/**
	*  @brief
	*    Assign a name to a given resource for debugging purposes
	*
	*  @param[in] resource
	*    Resource to assign the debug name to, can be a null pointer
	*  @param[in] name
	*    ASCII name for debugging purposes, must be valid (there's no internal null pointer test)
	*/
	#define RENDERER_SET_RESOURCE_DEBUG_NAME(resource, name)
#endif
