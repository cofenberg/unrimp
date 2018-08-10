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
*    Vulkan renderer amalgamated/unity build implementation
*
*  @remarks
*    == Dependencies ==
*    - Vulkan capable graphics driver
*    - Vulkan headers which can be found at "<unrimp>\External\Vulkan\include\"
*    - smol-v (directly compiled and linked in)
*    - glslang if "RENDERER_VULKAN_GLSLTOSPIRV" is set (directly compiled and linked in)
*
*    == Preprocessor Definitions ==
*    - Set "RENDERER_VULKAN_EXPORTS" as preprocessor definition when building this library as shared library
*    - Set "RENDERER_VULKAN_GLSLTOSPIRV" as preprocessor definition when building this library to add support for compiling GLSL into SPIR-V, increases the binary size around one MiB
*    - Do also have a look into the renderer header file documentation
*/


//[-------------------------------------------------------]
//[ Includes                                              ]
//[-------------------------------------------------------]
#include <Renderer/Public/Renderer.h>

#ifdef _WIN32
	// Set Windows version to Windows Vista (0x0600), we don't support Windows XP (0x0501)
	#ifdef WINVER
		#undef WINVER
	#endif
	#define WINVER			0x0600
	#ifdef _WIN32_WINNT
		#undef _WIN32_WINNT
	#endif
	#define _WIN32_WINNT	0x0600

	// Exclude some stuff from "windows.h" to speed up compilation a bit
	#define WIN32_LEAN_AND_MEAN
	#define NOGDICAPMASKS
	#define NOMENUS
	#define NOICONS
	#define NOKEYSTATES
	#define NOSYSCOMMANDS
	#define NORASTEROPS
	#define OEMRESOURCE
	#define NOATOM
	#define NOMEMMGR
	#define NOMETAFILE
	#define NOOPENFILE
	#define NOSCROLL
	#define NOSERVICE
	#define NOSOUND
	#define NOWH
	#define NOCOMM
	#define NOKANJI
	#define NOHELP
	#define NOPROFILER
	#define NODEFERWINDOWPOS
	#define NOMCX
	#define NOCRYPT
	#include <windows.h>

	#undef max	// Get rid of nasty OS macro
#elif defined __ANDROID__
	#include <link.h>
	#include <dlfcn.h>
#elif defined LINUX
	// TODO(co) Review which of the following headers can be removed
	#include <X11/Xlib.h>

	#include <GL/glx.h>
	#include <GL/glxext.h>

	#include <dlfcn.h>
	#include <link.h>
	#include <iostream>
#else
	#error "Unsupported platform"
#endif

#ifdef RENDERER_VULKAN_GLSLTOSPIRV
	// Disable warnings in external headers, we can't fix them
	PRAGMA_WARNING_PUSH
		PRAGMA_WARNING_DISABLE_MSVC(4061)	// warning C4061: enumerator '<x>' in switch of enum '<y>' is not explicitly handled by a case label
		PRAGMA_WARNING_DISABLE_MSVC(4365)	// warning C4365: 'argument': conversion from '<x>' to '<y>', signed/unsigned mismatch
		PRAGMA_WARNING_DISABLE_MSVC(4530)	// warning C4530: C++ exception handler used, but unwind semantics are not enabled. Specify /EHsc
		PRAGMA_WARNING_DISABLE_MSVC(4571)	// warning C4571: Informational: catch(...) semantics changed since Visual C++ 7.1; structured exceptions (SEH) are no longer caught
		PRAGMA_WARNING_DISABLE_MSVC(4623)	// warning C4623: 'std::_List_node<_Ty,std::_Default_allocator_traits<_Alloc>::void_pointer>': default constructor was implicitly defined as deleted
		PRAGMA_WARNING_DISABLE_MSVC(4625)	// warning C4625: '<x>': copy constructor was implicitly defined as deleted
		PRAGMA_WARNING_DISABLE_MSVC(4626)	// warning C4626: 'std::codecvt_base': assignment operator was implicitly defined as deleted
		PRAGMA_WARNING_DISABLE_MSVC(4774)	// warning C4774: 'sprintf_s' : format string expected in argument 3 is not a string literal
		PRAGMA_WARNING_DISABLE_MSVC(5027)	// warning C5027: 'std::_Generic_error_category': move assignment operator was implicitly defined as deleted
		#include <SPIRV/GlslangToSpv.h>
		#include <glslang/MachineIndependent/localintermediate.h>
	PRAGMA_WARNING_POP
#endif

// Disable warnings in external headers, we can't fix them
PRAGMA_WARNING_PUSH
	PRAGMA_WARNING_DISABLE_MSVC(4365)	// warning C4365: 'argument': conversion from 'long' to 'unsigned int', signed/unsigned mismatch
	#include <smol-v/smolv.h>
PRAGMA_WARNING_POP

// Disable warnings in external headers, we can't fix them
PRAGMA_WARNING_PUSH
	PRAGMA_WARNING_DISABLE_MSVC(4668)	// warning C4668 '<x>' is not defined as a preprocessor macro, replacing with '0' for '#if/#elif'
	PRAGMA_WARNING_DISABLE_MSVC(5039)	// warning C5039: 'TpSetCallbackCleanupGroup': pointer or reference to potentially throwing function passed to extern C function under -EHc. Undefined behavior may occur if this function throws an exception.
	#if defined(_WIN32)
		#define VK_USE_PLATFORM_WIN32_KHR
	#elif defined(__ANDROID__)
		#define VK_USE_PLATFORM_ANDROID_KHR
	#elif defined(LINUX)
		// TODO(sw) Make this optional which platform we support? For now we support xlib and Wayland.
		#define VK_USE_PLATFORM_XLIB_KHR
		#define VK_USE_PLATFORM_WAYLAND_KHR
	#endif
	#define VK_NO_PROTOTYPES
	#include <vulkan/vulkan.h>
PRAGMA_WARNING_POP

// Disable warnings in external headers, we can't fix them
PRAGMA_WARNING_PUSH
	PRAGMA_WARNING_DISABLE_MSVC(4365)	// warning C4365: 'return': conversion from 'int' to 'std::char_traits<wchar_t>::int_type', signed/unsigned mismatch
	PRAGMA_WARNING_DISABLE_MSVC(4571)	// warning C4571: Informational: catch(...) semantics changed since Visual C++ 7.1; structured exceptions (SEH) are no longer caught
	PRAGMA_WARNING_DISABLE_MSVC(4574)	// warning C4574: '_HAS_ITERATOR_DEBUGGING' is defined to be '0': did you mean to use '#if _HAS_ITERATOR_DEBUGGING'?
	PRAGMA_WARNING_DISABLE_MSVC(4625)	// warning C4625: 'std::codecvt_base': copy constructor was implicitly defined as deleted
	PRAGMA_WARNING_DISABLE_MSVC(4626)	// warning C4626: 'std::codecvt_base': assignment operator was implicitly defined as deleted
	PRAGMA_WARNING_DISABLE_MSVC(4668)	// warning C4668: '_M_HYBRID_X86_ARM64' is not defined as a preprocessor macro, replacing with '0' for '#if/#elif'
	PRAGMA_WARNING_DISABLE_MSVC(4774)	// warning C4774: 'sprintf_s' : format string expected in argument 3 is not a string literal
	PRAGMA_WARNING_DISABLE_MSVC(5026)	// warning C5026: 'std::_Generic_error_category': move constructor was implicitly defined as deleted
	PRAGMA_WARNING_DISABLE_MSVC(5027)	// warning C5027: 'std::_Generic_error_category': move assignment operator was implicitly defined as deleted
	#include <array>
	#include <vector>
	#include <sstream>
PRAGMA_WARNING_POP




//[-------------------------------------------------------]
//[ Forward declarations                                  ]
//[-------------------------------------------------------]
namespace VulkanRenderer
{
	class VertexArray;
	class RootSignature;
	class VulkanContext;
	class VulkanRuntimeLinking;
}




//[-------------------------------------------------------]
//[ Macros & definitions                                  ]
//[-------------------------------------------------------]
/*
*  @brief
*    Check whether or not the given resource is owned by the given renderer
*/
#ifdef RENDERER_DEBUG
	#define VULKANRENDERER_RENDERERMATCHCHECK_ASSERT(rendererReference, resourceReference) \
		RENDERER_ASSERT(mContext, &rendererReference == &(resourceReference).getRenderer(), "Vulkan error: The given resource is owned by another renderer instance")
#else
	#define VULKANRENDERER_RENDERERMATCHCHECK_ASSERT(rendererReference, resourceReference)
#endif

#ifdef RENDERER_DEBUG
	#define SET_DEFAULT_DEBUG_NAME setDebugName("");	// Assign a default name to the resource for debugging purposes
	#define DEFINE_SET_DEBUG_NAME_VKBUFFER_VKDEVICEMEMORY(decoration, numberOfDecorationCharacters) \
		virtual void setDebugName(const char* name) override \
		{ \
			if (nullptr != vkDebugMarkerSetObjectNameEXT) \
			{ \
				RENDERER_DECORATED_DEBUG_NAME(name, detailedName, decoration, numberOfDecorationCharacters); \
				const VkDevice vkDevice = static_cast<const VulkanRenderer&>(getRenderer()).getVulkanContext().getVkDevice(); \
				Helper::setDebugObjectName(vkDevice, VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_EXT, (uint64_t)mVkBuffer, detailedName); \
				Helper::setDebugObjectName(vkDevice, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_MEMORY_EXT, (uint64_t)mVkDeviceMemory, detailedName); \
			} \
		}
	#define DEFINE_SET_DEBUG_NAME_TEXTURE() \
		virtual void setDebugName(const char* name) override \
		{ \
			if (nullptr != vkDebugMarkerSetObjectNameEXT) \
			{ \
				const VkDevice vkDevice = static_cast<const VulkanRenderer&>(getRenderer()).getVulkanContext().getVkDevice(); \
				Helper::setDebugObjectName(vkDevice, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT, (uint64_t)mVkImage, name); \
				Helper::setDebugObjectName(vkDevice, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_MEMORY_EXT, (uint64_t)mVkDeviceMemory, name); \
				Helper::setDebugObjectName(vkDevice, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_VIEW_EXT, (uint64_t)mVkImageView, name); \
			} \
		}
	#define DEFINE_SET_DEBUG_NAME_SHADER_MODULE() \
		virtual void setDebugName(const char* name) override \
		{ \
			if (nullptr != vkDebugMarkerSetObjectNameEXT) \
			{ \
				Helper::setDebugObjectName(static_cast<const VulkanRenderer&>(getRenderer()).getVulkanContext().getVkDevice(), VK_DEBUG_REPORT_OBJECT_TYPE_SHADER_MODULE_EXT, (uint64_t)mVkShaderModule, name); \
			} \
		}
#else
	#define SET_DEFAULT_DEBUG_NAME
	#define DEFINE_SET_DEBUG_NAME_VKBUFFER_VKDEVICEMEMORY(decoration, numberOfDecorationCharacters)
	#define DEFINE_SET_DEBUG_NAME_TEXTURE()
	#define DEFINE_SET_DEBUG_NAME_SHADER_MODULE()
#endif

#ifndef FNPTR
	#define FNPTR(name) PFN_##name name;
#endif

// Global Vulkan function pointers
FNPTR(vkGetInstanceProcAddr)
FNPTR(vkGetDeviceProcAddr)
FNPTR(vkEnumerateInstanceExtensionProperties)
FNPTR(vkEnumerateInstanceLayerProperties)
FNPTR(vkCreateInstance)

// Instance based Vulkan function pointers
FNPTR(vkDestroyInstance)
FNPTR(vkEnumeratePhysicalDevices)
FNPTR(vkEnumerateDeviceLayerProperties)
FNPTR(vkEnumerateDeviceExtensionProperties)
FNPTR(vkGetPhysicalDeviceQueueFamilyProperties)
FNPTR(vkGetPhysicalDeviceFeatures)
FNPTR(vkGetPhysicalDeviceFormatProperties)
FNPTR(vkGetPhysicalDeviceMemoryProperties)
FNPTR(vkGetPhysicalDeviceProperties)
FNPTR(vkCreateDevice)
// "VK_EXT_debug_report"-extension
FNPTR(vkCreateDebugReportCallbackEXT)
FNPTR(vkDestroyDebugReportCallbackEXT)
// "VK_KHR_surface"-extension
FNPTR(vkDestroySurfaceKHR)
FNPTR(vkGetPhysicalDeviceSurfaceSupportKHR)
FNPTR(vkGetPhysicalDeviceSurfaceFormatsKHR)
FNPTR(vkGetPhysicalDeviceSurfaceCapabilitiesKHR)
FNPTR(vkGetPhysicalDeviceSurfacePresentModesKHR)
#ifdef VK_USE_PLATFORM_WIN32_KHR
	// "VK_KHR_win32_surface"-extension
	FNPTR(vkCreateWin32SurfaceKHR)
#elif defined VK_USE_PLATFORM_ANDROID_KHR
	// "VK_KHR_android_surface"-extension
	#warning "TODO(co) Not tested"
	FNPTR(vkCreateAndroidSurfaceKHR)
#elif defined VK_USE_PLATFORM_XLIB_KHR || defined VK_USE_PLATFORM_WAYLAND_KHR
	#if defined VK_USE_PLATFORM_XLIB_KHR
		// "VK_KHR_xlib_surface"-extension
		FNPTR(vkCreateXlibSurfaceKHR)
	#endif
	#if defined VK_USE_PLATFORM_WAYLAND_KHR
		// "VK_KHR_wayland_surface"-extension
		FNPTR(vkCreateWaylandSurfaceKHR)
	#endif
#elif defined VK_USE_PLATFORM_XCB_KHR
	// "VK_KHR_xcb_surface"-extension
	#warning "TODO(co) Not tested"
	FNPTR(vkCreateXcbSurfaceKHR)
#else
	#error "Unsupported platform"
#endif

// Device based Vulkan function pointers
FNPTR(vkDestroyDevice)
FNPTR(vkCreateShaderModule)
FNPTR(vkDestroyShaderModule)
FNPTR(vkCreateBuffer)
FNPTR(vkDestroyBuffer)
FNPTR(vkMapMemory)
FNPTR(vkUnmapMemory)
FNPTR(vkCreateBufferView)
FNPTR(vkDestroyBufferView)
FNPTR(vkAllocateMemory)
FNPTR(vkFreeMemory)
FNPTR(vkGetBufferMemoryRequirements)
FNPTR(vkBindBufferMemory)
FNPTR(vkCreateRenderPass)
FNPTR(vkDestroyRenderPass)
FNPTR(vkCreateImage)
FNPTR(vkDestroyImage)
FNPTR(vkGetImageSubresourceLayout)
FNPTR(vkGetImageMemoryRequirements)
FNPTR(vkBindImageMemory)
FNPTR(vkCreateImageView)
FNPTR(vkDestroyImageView)
FNPTR(vkCreateSampler)
FNPTR(vkDestroySampler)
FNPTR(vkCreateSemaphore)
FNPTR(vkDestroySemaphore)
FNPTR(vkCreateFence)
FNPTR(vkDestroyFence)
FNPTR(vkWaitForFences)
FNPTR(vkCreateCommandPool)
FNPTR(vkDestroyCommandPool)
FNPTR(vkAllocateCommandBuffers)
FNPTR(vkFreeCommandBuffers)
FNPTR(vkBeginCommandBuffer)
FNPTR(vkEndCommandBuffer)
FNPTR(vkGetDeviceQueue)
FNPTR(vkQueueSubmit)
FNPTR(vkQueueWaitIdle)
FNPTR(vkDeviceWaitIdle)
FNPTR(vkCreateFramebuffer)
FNPTR(vkDestroyFramebuffer)
FNPTR(vkCreatePipelineCache)
FNPTR(vkDestroyPipelineCache)
FNPTR(vkCreatePipelineLayout)
FNPTR(vkDestroyPipelineLayout)
FNPTR(vkCreateGraphicsPipelines)
FNPTR(vkCreateComputePipelines)
FNPTR(vkDestroyPipeline)
FNPTR(vkCreateDescriptorPool)
FNPTR(vkDestroyDescriptorPool)
FNPTR(vkCreateDescriptorSetLayout)
FNPTR(vkDestroyDescriptorSetLayout)
FNPTR(vkAllocateDescriptorSets)
FNPTR(vkFreeDescriptorSets)
FNPTR(vkUpdateDescriptorSets)
FNPTR(vkCreateQueryPool)
FNPTR(vkDestroyQueryPool)
FNPTR(vkGetQueryPoolResults)
FNPTR(vkCmdBeginQuery)
FNPTR(vkCmdEndQuery)
FNPTR(vkCmdResetQueryPool)
FNPTR(vkCmdCopyQueryPoolResults)
FNPTR(vkCmdPipelineBarrier)
FNPTR(vkCmdBeginRenderPass)
FNPTR(vkCmdEndRenderPass)
FNPTR(vkCmdExecuteCommands)
FNPTR(vkCmdCopyImage)
FNPTR(vkCmdBlitImage)
FNPTR(vkCmdCopyBufferToImage)
FNPTR(vkCmdClearAttachments)
FNPTR(vkCmdCopyBuffer)
FNPTR(vkCmdBindDescriptorSets)
FNPTR(vkCmdBindPipeline)
FNPTR(vkCmdSetViewport)
FNPTR(vkCmdSetScissor)
FNPTR(vkCmdSetLineWidth)
FNPTR(vkCmdSetDepthBias)
FNPTR(vkCmdPushConstants)
FNPTR(vkCmdBindIndexBuffer)
FNPTR(vkCmdBindVertexBuffers)
FNPTR(vkCmdDraw)
FNPTR(vkCmdDrawIndexed)
FNPTR(vkCmdDrawIndirect)
FNPTR(vkCmdDrawIndexedIndirect)
FNPTR(vkCmdDispatch)
FNPTR(vkCmdClearColorImage)
FNPTR(vkCmdClearDepthStencilImage)
// "VK_EXT_debug_marker"-extension
FNPTR(vkDebugMarkerSetObjectTagEXT)
FNPTR(vkDebugMarkerSetObjectNameEXT)
FNPTR(vkCmdDebugMarkerBeginEXT)
FNPTR(vkCmdDebugMarkerEndEXT)
FNPTR(vkCmdDebugMarkerInsertEXT)
// "VK_KHR_swapchain"-extension
FNPTR(vkCreateSwapchainKHR)
FNPTR(vkDestroySwapchainKHR)
FNPTR(vkGetSwapchainImagesKHR)
FNPTR(vkAcquireNextImageKHR)
FNPTR(vkQueuePresentKHR)




//[-------------------------------------------------------]
//[ Anonymous detail namespace                            ]
//[-------------------------------------------------------]
namespace
{
	namespace detail
	{


		//[-------------------------------------------------------]
		//[ Global definitions                                    ]
		//[-------------------------------------------------------]
		static constexpr const char* GLSL_NAME = "GLSL";	///< ASCII name of this shader language, always valid (do not free the memory the returned pointer is pointing to)
		typedef std::vector<VkPhysicalDevice> VkPhysicalDevices;
		typedef std::vector<VkExtensionProperties> VkExtensionPropertiesVector;
		typedef std::array<VkPipelineShaderStageCreateInfo, 5> VkPipelineShaderStageCreateInfos;
		#ifdef __ANDROID__
			// On Android we need to explicitly select all layers
			#warning "TODO(co) Not tested"
			static constexpr uint32_t NUMBER_OF_VALIDATION_LAYERS = 6;
			static constexpr const char* VALIDATION_LAYER_NAMES[] =
			{
				"VK_LAYER_GOOGLE_threading",
				"VK_LAYER_LUNARG_parameter_validation",
				"VK_LAYER_LUNARG_object_tracker",
				"VK_LAYER_LUNARG_core_validation",
				"VK_LAYER_LUNARG_swapchain",
				"VK_LAYER_GOOGLE_unique_objects"
			};
		#else
			// On desktop the LunarG loaders exposes a meta layer that contains all layers
			static constexpr uint32_t NUMBER_OF_VALIDATION_LAYERS = 1;
			static constexpr const char* VALIDATION_LAYER_NAMES[] =
			{
				"VK_LAYER_LUNARG_standard_validation"
			};
		#endif

		#ifdef RENDERER_VULKAN_GLSLTOSPIRV
			static bool GlslangInitialized = false;

			// Settings from "glslang/StandAlone/ResourceLimits.cpp"
			static constexpr TBuiltInResource DefaultTBuiltInResource = {
				32,		///< MaxLights
				6,		///< MaxClipPlanes
				32,		///< MaxTextureUnits
				32,		///< MaxTextureCoords
				64,		///< MaxVertexAttribs
				4096,	///< MaxVertexUniformComponents
				64,		///< MaxVaryingFloats
				32,		///< MaxVertexTextureImageUnits
				80,		///< MaxCombinedTextureImageUnits
				32,		///< MaxTextureImageUnits
				4096,	///< MaxFragmentUniformComponents
				32,		///< MaxDrawBuffers
				128,	///< MaxVertexUniformVectors
				8,		///< MaxVaryingVectors
				16,		///< MaxFragmentUniformVectors
				16,		///< MaxVertexOutputVectors
				15,		///< MaxFragmentInputVectors
				-8,		///< MinProgramTexelOffset
				7,		///< MaxProgramTexelOffset
				8,		///< MaxClipDistances
				65535,	///< MaxComputeWorkGroupCountX
				65535,	///< MaxComputeWorkGroupCountY
				65535,	///< MaxComputeWorkGroupCountZ
				1024,	///< MaxComputeWorkGroupSizeX
				1024,	///< MaxComputeWorkGroupSizeY
				64,		///< MaxComputeWorkGroupSizeZ
				1024,	///< MaxComputeUniformComponents
				16,		///< MaxComputeTextureImageUnits
				8,		///< MaxComputeImageUniforms
				8,		///< MaxComputeAtomicCounters
				1,		///< MaxComputeAtomicCounterBuffers
				60,		///< MaxVaryingComponents
				64,		///< MaxVertexOutputComponents
				64,		///< MaxGeometryInputComponents
				128,	///< MaxGeometryOutputComponents
				128,	///< MaxFragmentInputComponents
				8,		///< MaxImageUnits
				8,		///< MaxCombinedImageUnitsAndFragmentOutputs
				8,		///< MaxCombinedShaderOutputResources
				0,		///< MaxImageSamples
				0,		///< MaxVertexImageUniforms
				0,		///< MaxTessControlImageUniforms
				0,		///< MaxTessEvaluationImageUniforms
				0,		///< MaxGeometryImageUniforms
				8,		///< MaxFragmentImageUniforms
				8,		///< MaxCombinedImageUniforms
				16,		///< MaxGeometryTextureImageUnits
				256,	///< MaxGeometryOutputVertices
				1024,	///< MaxGeometryTotalOutputComponents
				1024,	///< MaxGeometryUniformComponents
				64,		///< MaxGeometryVaryingComponents
				128,	///< MaxTessControlInputComponents
				128,	///< MaxTessControlOutputComponents
				16,		///< MaxTessControlTextureImageUnits
				1024,	///< MaxTessControlUniformComponents
				4096,	///< MaxTessControlTotalOutputComponents
				128,	///< MaxTessEvaluationInputComponents
				128,	///< MaxTessEvaluationOutputComponents
				16,		///< MaxTessEvaluationTextureImageUnits
				1024,	///< MaxTessEvaluationUniformComponents
				120,	///< MaxTessPatchComponents
				32,		///< MaxPatchVertices
				64,		///< MaxTessGenLevel
				16,		///< MaxViewports
				0,		///< MaxVertexAtomicCounters
				0,		///< MaxTessControlAtomicCounters
				0,		///< MaxTessEvaluationAtomicCounters
				0,		///< MaxGeometryAtomicCounters
				8,		///< MaxFragmentAtomicCounters
				8,		///< MaxCombinedAtomicCounters
				1,		///< MaxAtomicCounterBindings
				0,		///< MaxVertexAtomicCounterBuffers
				0,		///< MaxTessControlAtomicCounterBuffers
				0,		///< MaxTessEvaluationAtomicCounterBuffers
				0,		///< MaxGeometryAtomicCounterBuffers
				1,		///< MaxFragmentAtomicCounterBuffers
				1,		///< MaxCombinedAtomicCounterBuffers
				16384,	///< MaxAtomicCounterBufferSize
				4,		///< MaxTransformFeedbackBuffers
				64,		///< MaxTransformFeedbackInterleavedComponents
				8,		///< MaxCullDistances
				8,		///< MaxCombinedClipAndCullDistances
				4,		///< MaxSamples
				{		///< limits
					1,	///< nonInductiveForLoops
					1,	///< whileLoops
					1,	///< doWhileLoops
					1,	///< generalUniformIndexing
					1,	///< generalAttributeMatrixVectorIndexing
					1,	///< generalVaryingIndexing
					1,	///< generalSamplerIndexing
					1,	///< generalVariableIndexing
					1,	///< generalConstantMatrixVectorIndexing
				}
			};
		#endif


		//[-------------------------------------------------------]
		//[ Global functions                                      ]
		//[-------------------------------------------------------]
		void updateWidthHeight(uint32_t mipmapIndex, uint32_t textureWidth, uint32_t textureHeight, uint32_t& width, uint32_t& height)
		{
			Renderer::ITexture::getMipmapSize(mipmapIndex, textureWidth, textureHeight);
			if (width > textureWidth)
			{
				width = textureWidth;
			}
			if (height > textureHeight)
			{
				height = textureHeight;
			}
		}

		void addVkPipelineShaderStageCreateInfo(VkShaderStageFlagBits vkShaderStageFlagBits, VkShaderModule vkShaderModule, VkPipelineShaderStageCreateInfos& vkPipelineShaderStageCreateInfos, uint32_t stageCount)
		{
			VkPipelineShaderStageCreateInfo& vkPipelineShaderStageCreateInfo = vkPipelineShaderStageCreateInfos[stageCount];
			vkPipelineShaderStageCreateInfo.sType				= VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;	// sType (VkStructureType)
			vkPipelineShaderStageCreateInfo.pNext				= nullptr;												// pNext (const void*)
			vkPipelineShaderStageCreateInfo.flags				= 0;													// flags (VkPipelineShaderStageCreateFlags)
			vkPipelineShaderStageCreateInfo.stage				= vkShaderStageFlagBits;								// stage (VkShaderStageFlagBits)
			vkPipelineShaderStageCreateInfo.module				= vkShaderModule;										// module (VkShaderModule)
			vkPipelineShaderStageCreateInfo.pName				= "main";												// pName (const char*)
			vkPipelineShaderStageCreateInfo.pSpecializationInfo	= nullptr;												// pSpecializationInfo (const VkSpecializationInfo*)
		}

		void enumeratePhysicalDevices(const Renderer::Context& context, VkInstance vkInstance, VkPhysicalDevices& vkPhysicalDevices)
		{
			// Get the number of available physical devices
			uint32_t physicalDeviceCount = 0;
			VkResult vkResult = vkEnumeratePhysicalDevices(vkInstance, &physicalDeviceCount, nullptr);
			if (VK_SUCCESS == vkResult)
			{
				if (physicalDeviceCount > 0)
				{
					// Enumerate physical devices
					vkPhysicalDevices.resize(physicalDeviceCount);
					vkResult = vkEnumeratePhysicalDevices(vkInstance, &physicalDeviceCount, vkPhysicalDevices.data());
					if (VK_SUCCESS != vkResult)
					{
						// Error!
						RENDERER_LOG(context, CRITICAL, "Failed to enumerate physical Vulkan devices")
					}
				}
				else
				{
					// Error!
					RENDERER_LOG(context, CRITICAL, "There are no physical Vulkan devices")
				}
			}
			else
			{
				// Error!
				RENDERER_LOG(context, CRITICAL, "Failed to get the number of physical Vulkan devices")
			}
		}

		bool isExtensionAvailable(const char* extensionName, const VkExtensionPropertiesVector& vkExtensionPropertiesVector)
		{
			for (const VkExtensionProperties& vkExtensionProperties : vkExtensionPropertiesVector)
			{
				if (strcmp(vkExtensionProperties.extensionName, extensionName) == 0)
				{
					// The extension is available
					return true;
				}
			}

			// The extension isn't available
			return false;
		}

		VkPhysicalDevice selectPhysicalDevice(const Renderer::Context& context, const VkPhysicalDevices& vkPhysicalDevices, bool validationEnabled, bool& enableDebugMarker)
		{
			// TODO(co) I'am sure this selection can be improved (rating etc.)
			for (const VkPhysicalDevice& vkPhysicalDevice : vkPhysicalDevices)
			{
				// Get of device extensions
				uint32_t propertyCount = 0;
				if ((vkEnumerateDeviceExtensionProperties(vkPhysicalDevice, nullptr, &propertyCount, nullptr) != VK_SUCCESS) || (0 == propertyCount))
				{
					// Reject physical Vulkan device
					continue;
				}
				VkExtensionPropertiesVector vkExtensionPropertiesVector(propertyCount);
				if (vkEnumerateDeviceExtensionProperties(vkPhysicalDevice, nullptr, &propertyCount, vkExtensionPropertiesVector.data()) != VK_SUCCESS)
				{
					// Reject physical Vulkan device
					continue;
				}

				{ // Reject physical Vulkan devices basing on swap chain support
					// Check device extensions
					static constexpr std::array<const char*, 2> deviceExtensions =
					{
						VK_KHR_SWAPCHAIN_EXTENSION_NAME,
						VK_KHR_MAINTENANCE1_EXTENSION_NAME	// We want to be able to specify a negative viewport height, this way we don't have to apply "<output position>.y = -<output position>.y" inside vertex shaders to compensate for the Vulkan coordinate system
					};
					bool rejectDevice = false;
					for (const char* deviceExtension : deviceExtensions)
					{
						if (!isExtensionAvailable(deviceExtension, vkExtensionPropertiesVector))
						{
							rejectDevice = true;
							break;
						}
					}
					if (rejectDevice)
					{
						// Reject physical Vulkan device
						continue;
					}
				}

				{ // Reject physical Vulkan devices basing on supported API version and some basic limits
					VkPhysicalDeviceProperties vkPhysicalDeviceProperties;
					vkGetPhysicalDeviceProperties(vkPhysicalDevice, &vkPhysicalDeviceProperties);
					const uint32_t majorVersion = VK_VERSION_MAJOR(vkPhysicalDeviceProperties.apiVersion);
					if ((majorVersion < 1) || (vkPhysicalDeviceProperties.limits.maxImageDimension2D < 4096))
					{
						// Reject physical Vulkan device
						continue;
					}
				}

				// Reject physical Vulkan devices basing on supported queue family
				uint32_t queueFamilyPropertyCount = 0;
				vkGetPhysicalDeviceQueueFamilyProperties(vkPhysicalDevice, &queueFamilyPropertyCount, nullptr);
				if (0 == queueFamilyPropertyCount)
				{
					// Reject physical Vulkan device
					continue;
				}
				std::vector<VkQueueFamilyProperties> queueFamilyProperties(queueFamilyPropertyCount);
				vkGetPhysicalDeviceQueueFamilyProperties(vkPhysicalDevice, &queueFamilyPropertyCount, queueFamilyProperties.data());
				for (uint32_t i = 0; i < queueFamilyPropertyCount; ++i)
				{
					if ((queueFamilyProperties[i].queueCount > 0) && (queueFamilyProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT))
					{
						// Check whether or not the "VK_EXT_debug_marker"-extension is available
						// -> The "VK_EXT_debug_marker"-extension is only available when the application gets started by tools like RenderDoc ( https://renderdoc.org/ )
						// -> See "Offline debugging in Vulkan with VK_EXT_debug_marker and RenderDoc" - https://www.saschawillems.de/?page_id=2017
						if (enableDebugMarker)
						{
							// Check whether or not the "VK_EXT_debug_marker"-extension is available
							if (isExtensionAvailable(VK_EXT_DEBUG_MARKER_EXTENSION_NAME, vkExtensionPropertiesVector))
							{
								// TODO(co) Currently, when trying to use RenderDoc ( https://renderdoc.org/ ) while having Vulkan debug layers enabled, RenderDoc crashes
								// -> Windows 10 x64
								// -> Radeon software 17.7.2
								// -> GPU: AMD 290X
								// -> LunarG® Vulkan™ SDK 1.0.54.0
								// -> Tried RenderDoc v0.91 as well as "Nightly v0.x @ 2017-08-21 (Win x64)" ("RenderDoc_2017_08_21_177d595d_64.zip")
								if (validationEnabled)
								{
									enableDebugMarker = false;
									RENDERER_LOG(context, WARNING, "Vulkan validation layers are enabled: If you want to use debug markers (\"VK_EXT_debug_marker\"-extension) please disable the validation layers")
								}
							}
							else
							{
								// Silently disable debug marker
								enableDebugMarker = false;
							}
						}

						// Select physical Vulkan device
						return vkPhysicalDevice;
					}
				}
			}

			// Error!
			RENDERER_LOG(context, CRITICAL, "Failed to select a physical Vulkan device")
			return VK_NULL_HANDLE;
		}

		VkResult createVkDevice(const Renderer::Context& context, const VkAllocationCallbacks* vkAllocationCallbacks, VkPhysicalDevice vkPhysicalDevice, const VkDeviceQueueCreateInfo& vkDeviceQueueCreateInfo, bool enableValidation, bool enableDebugMarker, VkDevice& vkDevice)
		{
			// See http://vulkan.gpuinfo.org/listfeatures.php to check out GPU hardware capabilities
			static constexpr std::array<const char*, 3> enabledExtensions =
			{
				VK_KHR_SWAPCHAIN_EXTENSION_NAME,
				VK_KHR_MAINTENANCE1_EXTENSION_NAME,	// We want to be able to specify a negative viewport height, this way we don't have to apply "<output position>.y = -<output position>.y" inside vertex shaders to compensate for the Vulkan coordinate system
				VK_EXT_DEBUG_MARKER_EXTENSION_NAME
			};
			static constexpr VkPhysicalDeviceFeatures vkPhysicalDeviceFeatures =
			{
				VK_FALSE,	// robustBufferAccess (VkBool32)
				VK_FALSE,	// fullDrawIndexUint32 (VkBool32)
				VK_FALSE,	// imageCubeArray (VkBool32)
				VK_FALSE,	// independentBlend (VkBool32)
				VK_TRUE,	// geometryShader (VkBool32)
				VK_TRUE,	// tessellationShader (VkBool32)
				VK_FALSE,	// sampleRateShading (VkBool32)
				VK_FALSE,	// dualSrcBlend (VkBool32)
				VK_FALSE,	// logicOp (VkBool32)
				VK_TRUE,	// multiDrawIndirect (VkBool32)
				VK_FALSE,	// drawIndirectFirstInstance (VkBool32)
				VK_TRUE,	// depthClamp (VkBool32)
				VK_FALSE,	// depthBiasClamp (VkBool32)
				VK_TRUE,	// fillModeNonSolid (VkBool32)
				VK_FALSE,	// depthBounds (VkBool32)
				VK_FALSE,	// wideLines (VkBool32)
				VK_FALSE,	// largePoints (VkBool32)
				VK_FALSE,	// alphaToOne (VkBool32)
				VK_FALSE,	// multiViewport (VkBool32)
				VK_TRUE,	// samplerAnisotropy (VkBool32)
				VK_FALSE,	// textureCompressionETC2 (VkBool32)
				VK_FALSE,	// textureCompressionASTC_LDR (VkBool32)
				VK_TRUE,	// textureCompressionBC (VkBool32)
				VK_FALSE,	// occlusionQueryPrecise (VkBool32)
				VK_FALSE,	// pipelineStatisticsQuery (VkBool32)
				VK_FALSE,	// vertexPipelineStoresAndAtomics (VkBool32)
				VK_FALSE,	// fragmentStoresAndAtomics (VkBool32)
				VK_FALSE,	// shaderTessellationAndGeometryPointSize (VkBool32)
				VK_FALSE,	// shaderImageGatherExtended (VkBool32)
				VK_FALSE,	// shaderStorageImageExtendedFormats (VkBool32)
				VK_FALSE,	// shaderStorageImageMultisample (VkBool32)
				VK_FALSE,	// shaderStorageImageReadWithoutFormat (VkBool32)
				VK_FALSE,	// shaderStorageImageWriteWithoutFormat (VkBool32)
				VK_FALSE,	// shaderUniformBufferArrayDynamicIndexing (VkBool32)
				VK_FALSE,	// shaderSampledImageArrayDynamicIndexing (VkBool32)
				VK_FALSE,	// shaderStorageBufferArrayDynamicIndexing (VkBool32)
				VK_FALSE,	// shaderStorageImageArrayDynamicIndexing (VkBool32)
				VK_FALSE,	// shaderClipDistance (VkBool32)
				VK_FALSE,	// shaderCullDistance (VkBool32)
				VK_FALSE,	// shaderFloat64 (VkBool32)
				VK_FALSE,	// shaderInt64 (VkBool32)
				VK_FALSE,	// shaderInt16 (VkBool32)
				VK_FALSE,	// shaderResourceResidency (VkBool32)
				VK_FALSE,	// shaderResourceMinLod (VkBool32)
				VK_FALSE,	// sparseBinding (VkBool32)
				VK_FALSE,	// sparseResidencyBuffer (VkBool32)
				VK_FALSE,	// sparseResidencyImage2D (VkBool32)
				VK_FALSE,	// sparseResidencyImage3D (VkBool32)
				VK_FALSE,	// sparseResidency2Samples (VkBool32)
				VK_FALSE,	// sparseResidency4Samples (VkBool32)
				VK_FALSE,	// sparseResidency8Samples (VkBool32)
				VK_FALSE,	// sparseResidency16Samples (VkBool32)
				VK_FALSE,	// sparseResidencyAliased (VkBool32)
				VK_FALSE,	// variableMultisampleRate (VkBool32)
				VK_FALSE	// inheritedQueries (VkBool32)
			};
			const VkDeviceCreateInfo vkDeviceCreateInfo =
			{
				VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,							// sType (VkStructureType)
				nullptr,														// pNext (const void*)
				0,																// flags (VkDeviceCreateFlags)
				1,																// queueCreateInfoCount (uint32_t)
				&vkDeviceQueueCreateInfo,										// pQueueCreateInfos (const VkDeviceQueueCreateInfo*)
				enableValidation ? NUMBER_OF_VALIDATION_LAYERS : 0,				// enabledLayerCount (uint32_t)
				enableValidation ? VALIDATION_LAYER_NAMES : nullptr,			// ppEnabledLayerNames (const char* const*)
				enableDebugMarker ? 3u : 2u,									// enabledExtensionCount (uint32_t)
				enabledExtensions.empty() ? nullptr : enabledExtensions.data(),	// ppEnabledExtensionNames (const char* const*)
				&vkPhysicalDeviceFeatures										// pEnabledFeatures (const VkPhysicalDeviceFeatures*)
			};
			const VkResult vkResult = vkCreateDevice(vkPhysicalDevice, &vkDeviceCreateInfo, vkAllocationCallbacks, &vkDevice);
			if (VK_SUCCESS == vkResult && enableDebugMarker)
			{
				// Get "VK_EXT_debug_marker"-extension function pointers

				// Define a helper macro
				PRAGMA_WARNING_PUSH
				PRAGMA_WARNING_DISABLE_MSVC(4191)	// 'reinterpret_cast': unsafe conversion from 'PFN_vkVoidFunction' to '<x>'
				#define IMPORT_FUNC(funcName)																											\
					funcName = reinterpret_cast<PFN_##funcName>(vkGetDeviceProcAddr(vkDevice, #funcName));												\
					if (nullptr == funcName)																											\
					{																																	\
						RENDERER_LOG(context, CRITICAL, "Failed to load instance based Vulkan function pointer \"%s\"", #funcName)	\
					}																																	\

				// "VK_EXT_debug_marker"-extension
				IMPORT_FUNC(vkDebugMarkerSetObjectTagEXT);
				IMPORT_FUNC(vkDebugMarkerSetObjectNameEXT);
				IMPORT_FUNC(vkCmdDebugMarkerBeginEXT);
				IMPORT_FUNC(vkCmdDebugMarkerEndEXT);
				IMPORT_FUNC(vkCmdDebugMarkerInsertEXT);

				// Undefine the helper macro
				#undef IMPORT_FUNC
				PRAGMA_WARNING_POP
			}

			// Done
			return vkResult;
		}

		VkDevice createVkDevice(const Renderer::Context& context, const VkAllocationCallbacks* vkAllocationCallbacks, VkPhysicalDevice vkPhysicalDevice, bool enableValidation, bool enableDebugMarker, uint32_t& graphicsQueueFamilyIndex, uint32_t& presentQueueFamilyIndex)
		{
			VkDevice vkDevice = VK_NULL_HANDLE;

			// Get physical device queue family properties
			uint32_t queueFamilyPropertyCount = 0;
			vkGetPhysicalDeviceQueueFamilyProperties(vkPhysicalDevice, &queueFamilyPropertyCount, nullptr);
			if (queueFamilyPropertyCount > 0)
			{
				std::vector<VkQueueFamilyProperties> vkQueueFamilyProperties;
				vkQueueFamilyProperties.resize(queueFamilyPropertyCount);
				vkGetPhysicalDeviceQueueFamilyProperties(vkPhysicalDevice, &queueFamilyPropertyCount, vkQueueFamilyProperties.data());

				// Find a queue that supports graphics operations
				uint32_t graphicsQueueIndex = 0;
				for (; graphicsQueueIndex < queueFamilyPropertyCount; ++graphicsQueueIndex)
				{
					if (vkQueueFamilyProperties[graphicsQueueIndex].queueFlags & VK_QUEUE_GRAPHICS_BIT)
					{
						// Create logical Vulkan device instance
						static constexpr std::array<float, 1> queuePriorities = { 0.0f };
						const VkDeviceQueueCreateInfo vkDeviceQueueCreateInfo =
						{
							VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,	// sType (VkStructureType)
							nullptr,									// pNext (const void*)
							0,											// flags (VkDeviceQueueCreateFlags)
							graphicsQueueIndex,							// queueFamilyIndex (uint32_t)
							1,											// queueCount (uint32_t)
							queuePriorities.data()						// pQueuePriorities (const float*)
						};
						VkResult vkResult = createVkDevice(context, vkAllocationCallbacks, vkPhysicalDevice, vkDeviceQueueCreateInfo, enableValidation, enableDebugMarker, vkDevice);
						if (VK_ERROR_LAYER_NOT_PRESENT == vkResult && enableValidation)
						{
							// Error! Since the show must go on, try creating a Vulkan device instance without validation enabled...
							RENDERER_LOG(context, WARNING, "Failed to create the Vulkan device instance with validation enabled, layer is not present")
							vkResult = createVkDevice(context, vkAllocationCallbacks, vkPhysicalDevice, vkDeviceQueueCreateInfo, false, enableDebugMarker, vkDevice);
						}
						// TODO(co) Error handling: Evaluate "vkResult"?
						graphicsQueueFamilyIndex = graphicsQueueIndex;
						presentQueueFamilyIndex = graphicsQueueIndex;	// TODO(co) Handle the case of the graphics queue doesn't support present

						// We're done, get us out of the loop
						graphicsQueueIndex = queueFamilyPropertyCount;
					}
				}
			}
			else
			{
				// Error!
				RENDERER_LOG(context, CRITICAL, "Failed to get physical Vulkan device queue family properties")
			}

			// Done
			return vkDevice;
		}

		VkCommandPool createVkCommandPool(const Renderer::Context& context, const VkAllocationCallbacks* vkAllocationCallbacks, VkDevice vkDevice, uint32_t graphicsQueueFamilyIndex)
		{
			VkCommandPool vkCommandPool = VK_NULL_HANDLE;

			// Create Vulkan command pool instance
			const VkCommandPoolCreateInfo vkCommandPoolCreateInfo =
			{
				VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,			// sType (VkStructureType)
				nullptr,											// pNext (const void*)
				VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,	// flags (VkCommandPoolCreateFlags)
				graphicsQueueFamilyIndex,							/// queueFamilyIndex (uint32_t)
			};
			const VkResult vkResult = vkCreateCommandPool(vkDevice, &vkCommandPoolCreateInfo, vkAllocationCallbacks, &vkCommandPool);
			if (VK_SUCCESS != vkResult)
			{
				// Error!
				RENDERER_LOG(context, CRITICAL, "Failed to create Vulkan command pool instance")
			}

			// Done
			return vkCommandPool;
		}

		VkCommandBuffer createVkCommandBuffer(const Renderer::Context& context, VkDevice vkDevice, VkCommandPool vkCommandPool)
		{
			VkCommandBuffer vkCommandBuffer = VK_NULL_HANDLE;

			// Create Vulkan command buffer instance
			const VkCommandBufferAllocateInfo vkCommandBufferAllocateInfo =
			{
				VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,	// sType (VkStructureType)
				nullptr,										// pNext (const void*)
				vkCommandPool,									// commandPool (VkCommandPool)
				VK_COMMAND_BUFFER_LEVEL_PRIMARY,				// level (VkCommandBufferLevel)
				1												// commandBufferCount (uint32_t)
			};
			VkResult vkResult = vkAllocateCommandBuffers(vkDevice, &vkCommandBufferAllocateInfo, &vkCommandBuffer);
			if (VK_SUCCESS != vkResult)
			{
				// Error!
				RENDERER_LOG(context, CRITICAL, "Failed to create Vulkan command buffer instance")
			}

			// Done
			return vkCommandBuffer;
		}

		bool hasVkFormatStencilComponent(VkFormat vkFormat)
		{
			return (VK_FORMAT_D32_SFLOAT_S8_UINT == vkFormat || VK_FORMAT_D24_UNORM_S8_UINT == vkFormat);
		}

		const char* vkDebugReportObjectTypeToString(VkDebugReportObjectTypeEXT vkDebugReportObjectTypeEXT)
		{
			// Define helper macro
			#define VALUE(value) case value: return #value;

			// Evaluate
			switch (vkDebugReportObjectTypeEXT)
			{
				VALUE(VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT)
				VALUE(VK_DEBUG_REPORT_OBJECT_TYPE_INSTANCE_EXT)
				VALUE(VK_DEBUG_REPORT_OBJECT_TYPE_PHYSICAL_DEVICE_EXT)
				VALUE(VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT)
				VALUE(VK_DEBUG_REPORT_OBJECT_TYPE_QUEUE_EXT)
				VALUE(VK_DEBUG_REPORT_OBJECT_TYPE_SEMAPHORE_EXT)
				VALUE(VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT)
				VALUE(VK_DEBUG_REPORT_OBJECT_TYPE_FENCE_EXT)
				VALUE(VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_MEMORY_EXT)
				VALUE(VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_EXT)
				VALUE(VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT)
				VALUE(VK_DEBUG_REPORT_OBJECT_TYPE_EVENT_EXT)
				VALUE(VK_DEBUG_REPORT_OBJECT_TYPE_QUERY_POOL_EXT)
				VALUE(VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_VIEW_EXT)
				VALUE(VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_VIEW_EXT)
				VALUE(VK_DEBUG_REPORT_OBJECT_TYPE_SHADER_MODULE_EXT)
				VALUE(VK_DEBUG_REPORT_OBJECT_TYPE_PIPELINE_CACHE_EXT)
				VALUE(VK_DEBUG_REPORT_OBJECT_TYPE_PIPELINE_LAYOUT_EXT)
				VALUE(VK_DEBUG_REPORT_OBJECT_TYPE_RENDER_PASS_EXT)
				VALUE(VK_DEBUG_REPORT_OBJECT_TYPE_PIPELINE_EXT)
				VALUE(VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT_EXT)
				VALUE(VK_DEBUG_REPORT_OBJECT_TYPE_SAMPLER_EXT)
				VALUE(VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_POOL_EXT)
				VALUE(VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_SET_EXT)
				VALUE(VK_DEBUG_REPORT_OBJECT_TYPE_FRAMEBUFFER_EXT)
				VALUE(VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_POOL_EXT)
				VALUE(VK_DEBUG_REPORT_OBJECT_TYPE_SURFACE_KHR_EXT)
				VALUE(VK_DEBUG_REPORT_OBJECT_TYPE_SWAPCHAIN_KHR_EXT)
				VALUE(VK_DEBUG_REPORT_OBJECT_TYPE_DEBUG_REPORT_CALLBACK_EXT_EXT)
				VALUE(VK_DEBUG_REPORT_OBJECT_TYPE_DISPLAY_KHR_EXT)
				VALUE(VK_DEBUG_REPORT_OBJECT_TYPE_DISPLAY_MODE_KHR_EXT)
				VALUE(VK_DEBUG_REPORT_OBJECT_TYPE_OBJECT_TABLE_NVX_EXT)
				VALUE(VK_DEBUG_REPORT_OBJECT_TYPE_INDIRECT_COMMANDS_LAYOUT_NVX_EXT)
				VALUE(VK_DEBUG_REPORT_OBJECT_TYPE_VALIDATION_CACHE_EXT)
				VALUE(VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_UPDATE_TEMPLATE_KHR_EXT)
				VALUE(VK_DEBUG_REPORT_OBJECT_TYPE_SAMPLER_YCBCR_CONVERSION_KHR_EXT)
				// VALUE(VK_DEBUG_REPORT_OBJECT_TYPE_BEGIN_RANGE_EXT)	- Not possible due to identical value
				// VALUE(VK_DEBUG_REPORT_OBJECT_TYPE_END_RANGE_EXT)		- Not possible due to identical value
				VALUE(VK_DEBUG_REPORT_OBJECT_TYPE_RANGE_SIZE_EXT)
				VALUE(VK_DEBUG_REPORT_OBJECT_TYPE_MAX_ENUM_EXT)
			}

			// Undefine helper macro
			#undef VALUE

			// Error!
			return nullptr;
		}

		VKAPI_ATTR VkBool32 VKAPI_CALL debugReportCallback(VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT objectType, uint64_t object, size_t location, int32_t messageCode, const char* pLayerPrefix, const char* pMessage, void* pUserData)
		{
			const Renderer::Context* context = static_cast<const Renderer::Context*>(pUserData);

			// TODO(co) Inside e.g. the "InstancedCubes"-example the log gets currently flooded with
			//          "Warning: Vulkan debug report callback: Object type: "VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_SET_EXT" Object: "120" Location: "5460" Message code: "0" Layer prefix: "DS" Message: "DescriptorSet 0x78 previously bound as set #0 is incompatible with set 0xc82f498 newly bound as set #0 so set #1 and any subsequent sets were disturbed by newly bound pipelineLayout (0x8b)" ".
			//          It's a known Vulkan API issue regarding validation. See https://github.com/KhronosGroup/Vulkan-Docs/issues/305 - "vkCmdBindDescriptorSets should be able to take NULL sets. #305".
			//          Currently I see no other way then ignoring this message.
			if (VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_SET_EXT == objectType && 5460 == location && 0 == messageCode)
			{
				// The Vulkan call should not be aborted to have the same behavior with and without validation layers enabled
				return VK_FALSE;
			}

			// TODO(co) File "unrimp\source\renderer\private\vulkanrenderer\vulkanrenderer.cpp" | Line 1029 | Critical: Vulkan debug report callback: Object type: "VK_DEBUG_REPORT_OBJECT_TYPE_INSTANCE_EXT" Object: "4963848" Location: "0" Message code: "0" Layer prefix: "Loader Message" Message: "loader_create_device_chain: Failed to find 'vkGetInstanceProcAddr' in layer C:\Program Files (x86)\Steam\.\SteamOverlayVulkanLayer.dll.  Skipping layer." 
			if (VK_DEBUG_REPORT_OBJECT_TYPE_INSTANCE_EXT == objectType && object && 0 == location && 0 == messageCode && nullptr != strstr(pMessage, "SteamOverlayVulkanLayer.dll"))
			{
				return VK_FALSE;
			}

			// Get log message type
			// -> Vulkan is using a flags combination, map it to our log message type enumeration
			Renderer::ILog::Type type = Renderer::ILog::Type::TRACE;
			if ((flags & VK_DEBUG_REPORT_ERROR_BIT_EXT) != 0)
			{
				type = Renderer::ILog::Type::CRITICAL;
			}
			else if ((flags & VK_DEBUG_REPORT_WARNING_BIT_EXT) != 0)
			{
				type = Renderer::ILog::Type::WARNING;
			}
			else if ((flags & VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT) != 0)
			{
				type = Renderer::ILog::Type::PERFORMANCE_WARNING;
			}
			else if ((flags & VK_DEBUG_REPORT_INFORMATION_BIT_EXT) != 0)
			{
				type = Renderer::ILog::Type::INFORMATION;
			}
			else if ((flags & VK_DEBUG_REPORT_DEBUG_BIT_EXT) != 0)
			{
				type = Renderer::ILog::Type::DEBUG;
			}

			// Construct the log message
			std::stringstream message;
			message << "Vulkan debug report callback: ";
			message << "Object type: \"" << vkDebugReportObjectTypeToString(objectType) << "\" ";
			message << "Object: \"" << object << "\" ";
			message << "Location: \"" << location << "\" ";
			message << "Message code: \"" << messageCode << "\" ";
			message << "Layer prefix: \"" << pLayerPrefix << "\" ";
			message << "Message: \"" << pMessage << "\" ";

			// Print log message
			if (context->getLog().print(type, nullptr, __FILE__, static_cast<uint32_t>(__LINE__), message.str().c_str()))
			{
				DEBUG_BREAK;
			}

			// The Vulkan call should not be aborted to have the same behavior with and without validation layers enabled
			return VK_FALSE;
		}

		VkSurfaceKHR createPresentationSurface(const Renderer::Context& context, const VkAllocationCallbacks* vkAllocationCallbacks, VkInstance vkInstance, VkPhysicalDevice vkPhysicalDevice, uint32_t graphicsQueueFamilyIndex, Renderer::WindowHandle windoInfo)
		{
			VkSurfaceKHR vkSurfaceKHR = VK_NULL_HANDLE;

			#ifdef VK_USE_PLATFORM_WIN32_KHR
				const VkWin32SurfaceCreateInfoKHR vkWin32SurfaceCreateInfoKHR =
				{
					VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR,																		// sType (VkStructureType)
					nullptr,																												// pNext (const void*)
					0,																														// flags (VkWin32SurfaceCreateFlagsKHR)
					reinterpret_cast<HINSTANCE>(::GetWindowLongPtr(reinterpret_cast<HWND>(windoInfo.nativeWindowHandle), GWLP_HINSTANCE)),	// hinstance (HINSTANCE)
					reinterpret_cast<HWND>(windoInfo.nativeWindowHandle)																	// hwnd (HWND)
				};
				if (vkCreateWin32SurfaceKHR(vkInstance, &vkWin32SurfaceCreateInfoKHR, vkAllocationCallbacks, &vkSurfaceKHR) != VK_SUCCESS)
				{
					// TODO(co) Can we ensure "vkSurfaceKHR" doesn't get touched by "vkCreateWin32SurfaceKHR()" in case of failure?
					vkSurfaceKHR = VK_NULL_HANDLE;
				}
			#elif defined VK_USE_PLATFORM_ANDROID_KHR
				#warning "TODO(co) Not tested"
				const VkAndroidSurfaceCreateInfoKHR vkAndroidSurfaceCreateInfoKHR =
				{
					VK_STRUCTURE_TYPE_ANDROID_SURFACE_CREATE_INFO_KHR,				// sType (VkStructureType)
					nullptr,														// pNext (const void*)
					0,																// flags (VkAndroidSurfaceCreateFlagsKHR)
					reinterpret_cast<ANativeWindow*>(windoInfo.nativeWindowHandle)	// window (ANativeWindow*)
				};
				if (vkCreateAndroidSurfaceKHR(vkInstance, &vkAndroidSurfaceCreateInfoKHR, vkAllocationCallbacks, &vkSurfaceKHR) != VK_SUCCESS)
				{
					// TODO(co) Can we ensure "vkSurfaceKHR" doesn't get touched by "vkCreateAndroidSurfaceKHR()" in case of failure?
					vkSurfaceKHR = VK_NULL_HANDLE;
				}
			#elif defined VK_USE_PLATFORM_XLIB_KHR || defined VK_USE_PLATFORM_WAYLAND_KHR
				RENDERER_ASSERT(context, context.getType() == Renderer::Context::ContextType::X11 || context.getType() == Renderer::Context::ContextType::WAYLAND, "Invalid Vulkan context type")

				// If the given renderer context is an X11 context use the display connection object provided by the context
				if (context.getType() == Renderer::Context::ContextType::X11)
				{
					const Renderer::X11Context& x11Context = static_cast<const Renderer::X11Context&>(context);
					const VkXlibSurfaceCreateInfoKHR vkXlibSurfaceCreateInfoKHR =
					{
						VK_STRUCTURE_TYPE_XLIB_SURFACE_CREATE_INFO_KHR,	// sType (VkStructureType)
						nullptr,										// pNext (const void*)
						0,												// flags (VkXlibSurfaceCreateFlagsKHR)
						x11Context.getDisplay(),						// dpy (Display*)
						windoInfo.nativeWindowHandle					// window (Window)
					};
					if (vkCreateXlibSurfaceKHR(vkInstance, &vkXlibSurfaceCreateInfoKHR, vkAllocationCallbacks, &vkSurfaceKHR) != VK_SUCCESS)
					{
						// TODO(co) Can we ensure "vkSurfaceKHR" doesn't get touched by "vkCreateXlibSurfaceKHR()" in case of failure?
						vkSurfaceKHR = VK_NULL_HANDLE;
					}
				}
				else if (context.getType() == Renderer::Context::ContextType::WAYLAND)
				{
					const Renderer::WaylandContext& waylandContext = static_cast<const Renderer::WaylandContext&>(context);
					const VkWaylandSurfaceCreateInfoKHR vkWaylandSurfaceCreateInfoKHR =
					{
						VK_STRUCTURE_TYPE_WAYLAND_SURFACE_CREATE_INFO_KHR,	// sType (VkStructureType)
						nullptr,											// pNext (const void*)
						0,													// flags (VkWaylandSurfaceCreateInfoKHR)
						waylandContext.getDisplay(),						// display (wl_display*)
						windoInfo.waylandSurface							// surface (wl_surface*)
					};
					if (vkCreateWaylandSurfaceKHR(vkInstance, &vkWaylandSurfaceCreateInfoKHR, vkAllocationCallbacks, &vkSurfaceKHR) != VK_SUCCESS)
					{
						// TODO(co) Can we ensure "vkSurfaceKHR" doesn't get touched by "vkCreateWaylandSurfaceKHR()" in case of failure?
						vkSurfaceKHR = VK_NULL_HANDLE;
					}
				}
			#elif defined VK_USE_PLATFORM_XCB_KHR
				#error "TODO(co) Complete implementation"
				const VkXcbSurfaceCreateInfoKHR vkXcbSurfaceCreateInfoKHR =
				{
					VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR,	// sType (VkStructureType)
					nullptr,										// pNext (const void*)
					0,												// flags (VkXcbSurfaceCreateFlagsKHR)
					TODO(co)										// connection (xcb_connection_t*)
					TODO(co)										// window (xcb_window_t)
				};
				if (vkCreateXcbSurfaceKHR(vkInstance, &vkXcbSurfaceCreateInfoKHR, vkAllocationCallbacks, &vkSurfaceKHR) != VK_SUCCESS)
				{
					// TODO(co) Can we ensure "vkSurfaceKHR" doesn't get touched by "vkCreateXcbSurfaceKHR()" in case of failure?
					vkSurfaceKHR = VK_NULL_HANDLE;
				}
			#else
				#error "Unsupported platform"
			#endif

			{ // Sanity check: Does the physical Vulkan device support the Vulkan presentation surface?
			  // TODO(co) Inside our renderer API the swap chain is physical device independent, which is a nice thing usability wise.
			  //          On the other hand, the sanity check here can only detect issues but it would be better to not get into such issues in the first place.
				VkBool32 queuePresentSupport = VK_FALSE;
				vkGetPhysicalDeviceSurfaceSupportKHR(vkPhysicalDevice, graphicsQueueFamilyIndex, vkSurfaceKHR, &queuePresentSupport);
				if (VK_FALSE == queuePresentSupport)
				{
					RENDERER_LOG(context, CRITICAL, "The created Vulkan presentation surface has no queue present support")
				}
			}

			// Done
			return vkSurfaceKHR;
		}

		uint32_t getNumberOfSwapChainImages(const VkSurfaceCapabilitiesKHR& vkSurfaceCapabilitiesKHR)
		{
			// Set of images defined in a swap chain may not always be available for application to render to:
			// - One may be displayed and one may wait in a queue to be presented
			// - If application wants to use more images at the same time it must ask for more images
			uint32_t numberOfImages = vkSurfaceCapabilitiesKHR.minImageCount + 1;
			if ((vkSurfaceCapabilitiesKHR.maxImageCount > 0) && (numberOfImages > vkSurfaceCapabilitiesKHR.maxImageCount))
			{
				numberOfImages = vkSurfaceCapabilitiesKHR.maxImageCount;
			}
			return numberOfImages;
		}

		VkSurfaceFormatKHR getSwapChainFormat(const Renderer::Context& context, VkPhysicalDevice vkPhysicalDevice, VkSurfaceKHR vkSurfaceKHR)
		{
			uint32_t surfaceFormatCount = 0;
			if ((vkGetPhysicalDeviceSurfaceFormatsKHR(vkPhysicalDevice, vkSurfaceKHR, &surfaceFormatCount, nullptr) != VK_SUCCESS) || (0 == surfaceFormatCount))
			{
				RENDERER_LOG(context, CRITICAL, "Failed to get physical Vulkan device surface formats")
				return { VK_FORMAT_UNDEFINED, VK_COLOR_SPACE_MAX_ENUM_KHR };
			}

			std::vector<VkSurfaceFormatKHR> surfaceFormats(surfaceFormatCount);
			if (vkGetPhysicalDeviceSurfaceFormatsKHR(vkPhysicalDevice, vkSurfaceKHR, &surfaceFormatCount, surfaceFormats.data()) != VK_SUCCESS)
			{
				RENDERER_LOG(context, CRITICAL, "Failed to get physical Vulkan device surface formats")
				return { VK_FORMAT_UNDEFINED, VK_COLOR_SPACE_MAX_ENUM_KHR };
			}

			// If the list contains only one entry with undefined format it means that there are no preferred surface formats and any can be chosen
			if ((surfaceFormats.size() == 1) && (VK_FORMAT_UNDEFINED == surfaceFormats[0].format))
			{
				return { VK_FORMAT_R8G8B8A8_UNORM, VK_COLORSPACE_SRGB_NONLINEAR_KHR };
			}

			// Check if list contains most widely used R8 G8 B8 A8 format with nonlinear color space
			// -> Not all implementations support RGBA8, some only support BGRA8 formats (e.g. xlib surface under Linux with RADV), so check for both
			for (const VkSurfaceFormatKHR& surfaceFormat : surfaceFormats)
			{
				if (VK_FORMAT_R8G8B8A8_UNORM == surfaceFormat.format || VK_FORMAT_B8G8R8A8_UNORM == surfaceFormat.format)
				{
					return surfaceFormat;
				}
			}

			// Return the first format from the list
			return surfaceFormats[0];
		}

		VkExtent2D getSwapChainExtent(const VkSurfaceCapabilitiesKHR& vkSurfaceCapabilitiesKHR)
		{
			// Special value of surface extent is width == height == -1
			// -> If this is so we define the size by ourselves but it must fit within defined confines, else it's already set to the operation window dimension
			if (-1 == vkSurfaceCapabilitiesKHR.currentExtent.width)
			{
				VkExtent2D swapChainExtent = { 640, 480 };
				if (swapChainExtent.width < vkSurfaceCapabilitiesKHR.minImageExtent.width)
				{
					swapChainExtent.width = vkSurfaceCapabilitiesKHR.minImageExtent.width;
				}
				if (swapChainExtent.height < vkSurfaceCapabilitiesKHR.minImageExtent.height)
				{
					swapChainExtent.height = vkSurfaceCapabilitiesKHR.minImageExtent.height;
				}
				if (swapChainExtent.width > vkSurfaceCapabilitiesKHR.maxImageExtent.width)
				{
					swapChainExtent.width = vkSurfaceCapabilitiesKHR.maxImageExtent.width;
				}
				if (swapChainExtent.height > vkSurfaceCapabilitiesKHR.maxImageExtent.height)
				{
					swapChainExtent.height = vkSurfaceCapabilitiesKHR.maxImageExtent.height;
				}
				return swapChainExtent;
			}

			// Most of the cases we define size of the swap chain images equal to current window's size
			return vkSurfaceCapabilitiesKHR.currentExtent;
		}

		VkImageUsageFlags getSwapChainUsageFlags(const Renderer::Context& context, const VkSurfaceCapabilitiesKHR& vkSurfaceCapabilitiesKHR)
		{
			// Color attachment flag must always be supported. We can define other usage flags but we always need to check if they are supported.
			if (vkSurfaceCapabilitiesKHR.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_DST_BIT)
			{
				return VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
			}

			// Construct the log message
			std::stringstream message;
			message << "VK_IMAGE_USAGE_TRANSFER_DST image usage is not supported by the swap chain: Supported swap chain image usages include:\n";
			message << ((vkSurfaceCapabilitiesKHR.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_SRC_BIT)				? "  VK_IMAGE_USAGE_TRANSFER_SRC\n" : "");
			message << ((vkSurfaceCapabilitiesKHR.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_DST_BIT)				? "  VK_IMAGE_USAGE_TRANSFER_DST\n" : "");
			message << ((vkSurfaceCapabilitiesKHR.supportedUsageFlags & VK_IMAGE_USAGE_SAMPLED_BIT)						? "  VK_IMAGE_USAGE_SAMPLED\n" : "");
			message << ((vkSurfaceCapabilitiesKHR.supportedUsageFlags & VK_IMAGE_USAGE_STORAGE_BIT)						? "  VK_IMAGE_USAGE_STORAGE\n" : "");
			message << ((vkSurfaceCapabilitiesKHR.supportedUsageFlags & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT)			? "  VK_IMAGE_USAGE_COLOR_ATTACHMENT\n" : "");
			message << ((vkSurfaceCapabilitiesKHR.supportedUsageFlags & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT)	? "  VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT\n" : "");
			message << ((vkSurfaceCapabilitiesKHR.supportedUsageFlags & VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT)		? "  VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT\n" : "");
			message << ((vkSurfaceCapabilitiesKHR.supportedUsageFlags & VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT)			? "  VK_IMAGE_USAGE_INPUT_ATTACHMENT" : "");

			// Print log message
			RENDERER_LOG(context, CRITICAL, message.str().c_str())

			// Error!
			return static_cast<VkImageUsageFlags>(-1);
		}

		VkSurfaceTransformFlagBitsKHR getSwapChainTransform(const VkSurfaceCapabilitiesKHR& vkSurfaceCapabilitiesKHR)
		{
			// - Sometimes images must be transformed before they are presented (i.e. due to device's orientation being other than default orientation)
			// - If the specified transform is other than current transform, presentation engine will transform image during presentation operation; this operation may hit performance on some platforms
			// - Here we don't want any transformations to occur so if the identity transform is supported use it otherwise just use the same transform as current transform
			return (vkSurfaceCapabilitiesKHR.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR) ? VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR : vkSurfaceCapabilitiesKHR.currentTransform;
		}

		VkPresentModeKHR getSwapChainPresentMode(const Renderer::Context& context, VkPhysicalDevice vkPhysicalDevice, VkSurfaceKHR vkSurfaceKHR)
		{
			uint32_t presentModeCount = 0;
			if ((vkGetPhysicalDeviceSurfacePresentModesKHR(vkPhysicalDevice, vkSurfaceKHR, &presentModeCount, nullptr) != VK_SUCCESS) || (0 == presentModeCount))
			{
				RENDERER_LOG(context, CRITICAL, "Failed to get physical Vulkan device surface present modes")
				return VK_PRESENT_MODE_MAX_ENUM_KHR;
			}

			std::vector<VkPresentModeKHR> presentModes(presentModeCount);
			if (vkGetPhysicalDeviceSurfacePresentModesKHR(vkPhysicalDevice, vkSurfaceKHR, &presentModeCount, presentModes.data()) != VK_SUCCESS)
			{
				RENDERER_LOG(context, CRITICAL, "Failed to get physical Vulkan device surface present modes")
				return VK_PRESENT_MODE_MAX_ENUM_KHR;
			}

			// - FIFO present mode is always available
			// - MAILBOX is the lowest latency V-Sync enabled mode (something like triple-buffering) so use it if available
			for (const VkPresentModeKHR& presentMode : presentModes)
			{
				if (VK_PRESENT_MODE_MAILBOX_KHR == presentMode)
				{
					return presentMode;
				}
			}
			for (const VkPresentModeKHR& presentMode : presentModes)
			{
				if (VK_PRESENT_MODE_FIFO_KHR == presentMode)
				{
					return presentMode;
				}
			}

			// Error!
			RENDERER_LOG(context, CRITICAL, "FIFO present mode is not supported by the Vulkan swap chain")
			return VK_PRESENT_MODE_MAX_ENUM_KHR;
		}

		VkRenderPass createRenderPass(const Renderer::Context& context, const VkAllocationCallbacks* vkAllocationCallbacks, VkDevice vkDevice, VkFormat colorVkFormat, VkFormat depthVkFormat, VkSampleCountFlagBits vkSampleCountFlagBits)
		{
			const bool hasDepthStencilAttachment = (VK_FORMAT_UNDEFINED != depthVkFormat);

			// Render pass configuration
			const std::array<VkAttachmentDescription, 2> vkAttachmentDescriptions =
			{{
				{
					0,									// flags (VkAttachmentDescriptionFlags)
					colorVkFormat,						// format (VkFormat)
					vkSampleCountFlagBits,				// samples (VkSampleCountFlagBits)
					VK_ATTACHMENT_LOAD_OP_CLEAR,		// loadOp (VkAttachmentLoadOp)
					VK_ATTACHMENT_STORE_OP_STORE,		// storeOp (VkAttachmentStoreOp)
					VK_ATTACHMENT_LOAD_OP_DONT_CARE,	// stencilLoadOp (VkAttachmentLoadOp)
					VK_ATTACHMENT_STORE_OP_DONT_CARE,	// stencilStoreOp (VkAttachmentStoreOp)
					VK_IMAGE_LAYOUT_UNDEFINED,			// initialLayout (VkImageLayout)
					VK_IMAGE_LAYOUT_PRESENT_SRC_KHR		// finalLayout (VkImageLayout)
				},
				{
					0,													// flags (VkAttachmentDescriptionFlags)
					depthVkFormat,										// format (VkFormat)
					vkSampleCountFlagBits,								// samples (VkSampleCountFlagBits)
					VK_ATTACHMENT_LOAD_OP_CLEAR,						// loadOp (VkAttachmentLoadOp)
					VK_ATTACHMENT_STORE_OP_STORE,						// storeOp (VkAttachmentStoreOp)
					VK_ATTACHMENT_LOAD_OP_DONT_CARE,					// stencilLoadOp (VkAttachmentLoadOp)
					VK_ATTACHMENT_STORE_OP_DONT_CARE,					// stencilStoreOp (VkAttachmentStoreOp)
					VK_IMAGE_LAYOUT_UNDEFINED,							// initialLayout (VkImageLayout)
					VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL	// finalLayout (VkImageLayout)
				}
			}};
			static constexpr VkAttachmentReference colorVkAttachmentReference =
			{
				0,											// attachment (uint32_t)
				VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL	// layout (VkImageLayout)
			};
			static constexpr VkAttachmentReference depthVkAttachmentReference =
			{
				1,													// attachment (uint32_t)
				VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL	// layout (VkImageLayout)
			};
			const VkSubpassDescription vkSubpassDescription =
			{
				0,																	// flags (VkSubpassDescriptionFlags)
				VK_PIPELINE_BIND_POINT_GRAPHICS,									// pipelineBindPoint (VkPipelineBindPoint)
				0,																	// inputAttachmentCount (uint32_t)
				nullptr,															// pInputAttachments (const VkAttachmentReference*)
				1,																	// colorAttachmentCount (uint32_t)
				&colorVkAttachmentReference,										// pColorAttachments (const VkAttachmentReference*)
				nullptr,															// pResolveAttachments (const VkAttachmentReference*)
				hasDepthStencilAttachment ? &depthVkAttachmentReference : nullptr,	// pDepthStencilAttachment (const VkAttachmentReference*)
				0,																	// preserveAttachmentCount (uint32_t)
				nullptr																// pPreserveAttachments (const uint32_t*)
			};
			static constexpr VkSubpassDependency vkSubpassDependency =
			{
				VK_SUBPASS_EXTERNAL,														// srcSubpass (uint32_t)
				0,																			// dstSubpass (uint32_t)
				VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,								// srcStageMask (VkPipelineStageFlags)
				VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,								// dstStageMask (VkPipelineStageFlags)
				0,																			// srcAccessMask (VkAccessFlags)
				VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,	// dstAccessMask (VkAccessFlags)
				0																			// dependencyFlags (VkDependencyFlags)
			};
			const VkRenderPassCreateInfo vkRenderPassCreateInfo =
			{
				VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,	// sType (VkStructureType)
				nullptr,									// pNext (const void*)
				0,											// flags (VkRenderPassCreateFlags)
				hasDepthStencilAttachment ? 2u : 1u,		// attachmentCount (uint32_t)
				vkAttachmentDescriptions.data(),			// pAttachments (const VkAttachmentDescription*)
				1,											// subpassCount (uint32_t)
				&vkSubpassDescription,						// pSubpasses (const VkSubpassDescription*)
				1,											// dependencyCount (uint32_t)
				&vkSubpassDependency						// pDependencies (const VkSubpassDependency*)
			};

			// Create render pass
			VkRenderPass vkRenderPass = VK_NULL_HANDLE;
			if (vkCreateRenderPass(vkDevice, &vkRenderPassCreateInfo, vkAllocationCallbacks, &vkRenderPass) != VK_SUCCESS)
			{
				RENDERER_LOG(context, CRITICAL, "Failed to create Vulkan render pass")
			}

			// Done
			return vkRenderPass;
		}

		VkFormat findSupportedVkFormat(VkPhysicalDevice vkPhysicalDevice, const std::vector<VkFormat>& vkFormatCandidates, VkImageTiling vkImageTiling, VkFormatFeatureFlags vkFormatFeatureFlags)
		{
			for (VkFormat vkFormat : vkFormatCandidates)
			{
				VkFormatProperties vkFormatProperties;
				vkGetPhysicalDeviceFormatProperties(vkPhysicalDevice, vkFormat, &vkFormatProperties);
				if (VK_IMAGE_TILING_LINEAR == vkImageTiling && (vkFormatProperties.linearTilingFeatures & vkFormatFeatureFlags) == vkFormatFeatureFlags)
				{
					return vkFormat;
				}
				else if (VK_IMAGE_TILING_OPTIMAL == vkImageTiling && (vkFormatProperties.optimalTilingFeatures & vkFormatFeatureFlags) == vkFormatFeatureFlags)
				{
					return vkFormat;
				}
			}

			// Failed to find supported Vulkan depth format
			return VK_FORMAT_UNDEFINED;
		}

		/**
		*  @brief
		*    Create Vulkan shader module from bytecode
		*
		*  @param[in] context
		*    Renderer context
		*  @param[in] vkAllocationCallbacks
		*    Vulkan allocation callbacks
		*  @param[in] vkDevice
		*    Vulkan device
		*  @param[in] shaderBytecode
		*    Shader SPIR-V bytecode compressed via SMOL-V
		*
		*  @return
		*    The Vulkan shader module, null handle on error
		*/
		VkShaderModule createVkShaderModuleFromBytecode(const Renderer::Context& context, const VkAllocationCallbacks* vkAllocationCallbacks, VkDevice vkDevice, const Renderer::ShaderBytecode& shaderBytecode)
		{
			// Decode from SMOL-V: like Vulkan/Khronos SPIR-V, but smaller
			// -> https://github.com/aras-p/smol-v
			// -> http://aras-p.info/blog/2016/09/01/SPIR-V-Compression/
			const size_t spirvOutputBufferSize = smolv::GetDecodedBufferSize(shaderBytecode.getBytecode(), shaderBytecode.getNumberOfBytes());
			// TODO(co) Try to avoid new/delete by trying to use the C-runtime stack if there aren't too many bytes
			uint8_t* spirvOutputBuffer = RENDERER_MALLOC_TYPED(context, uint8_t, spirvOutputBufferSize);
			smolv::Decode(shaderBytecode.getBytecode(), shaderBytecode.getNumberOfBytes(), spirvOutputBuffer, spirvOutputBufferSize);

			// Create the Vulkan shader module
			const VkShaderModuleCreateInfo vkShaderModuleCreateInfo =
			{
				VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,			// sType (VkStructureType)
				nullptr,												// pNext (const void*)
				0,														// flags (VkShaderModuleCreateFlags)
				spirvOutputBufferSize,									// codeSize (size_t)
				reinterpret_cast<const uint32_t*>(spirvOutputBuffer)	// pCode (const uint32_t*)
			};
			VkShaderModule vkShaderModule = VK_NULL_HANDLE;
			if (vkCreateShaderModule(vkDevice, &vkShaderModuleCreateInfo, vkAllocationCallbacks, &vkShaderModule) != VK_SUCCESS)
			{
				RENDERER_LOG(context, CRITICAL, "Failed to create the Vulkan shader module")
			}

			// Done
			RENDERER_FREE(context, spirvOutputBuffer);
			return vkShaderModule;
		}

		/**
		*  @brief
		*    Create Vulkan shader module from source code
		*
		*  @param[in] vulkanRenderer
		*    Owner Vulkan renderer instance
		*  @param[in] vkShaderStageFlagBits
		*    Vulkan shader stage flag bits (only a single set bit allowed)
		*  @param[in] sourceCode
		*    Shader ASCII source code, must be a valid pointer
		*  @param[out] shaderBytecode
		*    If not a null pointer, this receives the shader SPIR-V bytecode compressed via SMOL-V
		*
		*  @return
		*    The Vulkan shader module, null handle on error
		*/
		// TODO(co) Visual Studio 2017 compile settings: For some reasons I need to disable optimization for the following method or else "glslang::TShader::parse()" will output the error "ERROR: 0:1: '€' : unexpected token" (glslang (latest commit c325f4364666eedb94c20a13670df058a38a14ab - April 20, 2018), Visual Studio 2017 15.7.1)
		#ifdef _MSC_VER
			#pragma optimize("", off)
		#endif
		VkShaderModule createVkShaderModuleFromSourceCode(const Renderer::Context& context, const VkAllocationCallbacks* vkAllocationCallbacks, VkDevice vkDevice, VkShaderStageFlagBits vkShaderStageFlagBits, const char* sourceCode, Renderer::ShaderBytecode* shaderBytecode)
		{
			#ifdef RENDERER_VULKAN_GLSLTOSPIRV
				// Initialize glslang, if necessary
				if (!GlslangInitialized)
				{
					glslang::InitializeProcess();
					GlslangInitialized = true;
				}

				// GLSL to intermediate
				// -> OpenGL 4.5
				static constexpr int glslVersion = 450;
				EShLanguage shLanguage = EShLangCount;
				if ((vkShaderStageFlagBits & VK_SHADER_STAGE_VERTEX_BIT) != 0)
				{
					shLanguage = EShLangVertex;
				}
				else if ((vkShaderStageFlagBits & VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT) != 0)
				{
					shLanguage = EShLangTessControl;
				}
				else if ((vkShaderStageFlagBits & VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT) != 0)
				{
					shLanguage = EShLangTessEvaluation;
				}
				else if ((vkShaderStageFlagBits & VK_SHADER_STAGE_GEOMETRY_BIT) != 0)
				{
					shLanguage = EShLangGeometry;
				}
				else if ((vkShaderStageFlagBits & VK_SHADER_STAGE_FRAGMENT_BIT) != 0)
				{
					shLanguage = EShLangFragment;
				}
				else if ((vkShaderStageFlagBits & VK_SHADER_STAGE_COMPUTE_BIT) != 0)
				{
					shLanguage = EShLangCompute;
				}
				else
				{
					RENDERER_ASSERT(context, false, "Invalid Vulkan shader stage flag bits")
				}
				glslang::TShader shader(shLanguage);
				shader.setEnvInput(glslang::EShSourceGlsl, shLanguage, glslang::EShClientVulkan, glslVersion);
				shader.setEntryPoint("main");
				{
					const char* sourcePointers[] = { sourceCode };
					shader.setStrings(sourcePointers, 1);
				}
				static constexpr EShMessages shMessages = static_cast<EShMessages>(EShMsgDefault | EShMsgSpvRules | EShMsgVulkanRules);
				if (shader.parse(&DefaultTBuiltInResource, glslVersion, false, shMessages))
				{
					glslang::TProgram program;
					program.addShader(&shader);
					if (program.link(shMessages))
					{
						// Intermediate to SPIR-V
						const glslang::TIntermediate* intermediate = program.getIntermediate(shLanguage);
						if (nullptr != intermediate)
						{
							std::vector<unsigned int> spirv;
							glslang::GlslangToSpv(*intermediate, spirv);

							// Optional shader bytecode output
							if (nullptr != shaderBytecode)
							{
								// Encode to SMOL-V: like Vulkan/Khronos SPIR-V, but smaller
								// -> https://github.com/aras-p/smol-v
								// -> http://aras-p.info/blog/2016/09/01/SPIR-V-Compression/
								// -> Don't apply "spv::spirvbin_t::remap()" or the SMOL-V result will be bigger
								smolv::ByteArray byteArray;
								smolv::Encode(spirv.data(), sizeof(unsigned int) * spirv.size(), byteArray, smolv::kEncodeFlagStripDebugInfo);

								// Done
								shaderBytecode->setBytecodeCopy(static_cast<uint32_t>(byteArray.size()), reinterpret_cast<uint8_t*>(byteArray.data()));
							}

							// Create the Vulkan shader module
							const VkShaderModuleCreateInfo vkShaderModuleCreateInfo =
							{
								VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,	// sType (VkStructureType)
								nullptr,										// pNext (const void*)
								0,												// flags (VkShaderModuleCreateFlags)
								sizeof(unsigned int) * spirv.size(),			// codeSize (size_t)
								spirv.data()									// pCode (const uint32_t*)
							};
							VkShaderModule vkShaderModule = VK_NULL_HANDLE;
							if (vkCreateShaderModule(vkDevice, &vkShaderModuleCreateInfo, vkAllocationCallbacks, &vkShaderModule) != VK_SUCCESS)
							{
								RENDERER_LOG(context, CRITICAL, "Failed to create the Vulkan shader module")
							}
							return vkShaderModule;
						}
					}
					else
					{
						// Failed to link the program
						if (context.getLog().print(Renderer::ILog::Type::CRITICAL, sourceCode, __FILE__, static_cast<uint32_t>(__LINE__), "Failed to link the GLSL program: %s", program.getInfoLog()))
						{
							DEBUG_BREAK;
						}
					}
				}
				else
				{
					// Failed to parse the shader source code
					if (context.getLog().print(Renderer::ILog::Type::CRITICAL, sourceCode, __FILE__, static_cast<uint32_t>(__LINE__), "Failed to parse the GLSL shader source code: %s", shader.getInfoLog()))
					{
						DEBUG_BREAK;
					}
				}
			#endif

			// Error!
			return VK_NULL_HANDLE;
		}
		// TODO(co) Visual Studio 2017 compile settings: For some reasons I need to disable optimization for the following method or else "glslang::TShader::parse()" will output the error "ERROR: 0:1: '€' : unexpected token" (glslang (latest commit c325f4364666eedb94c20a13670df058a38a14ab - April 20, 2018), Visual Studio 2017 15.7.1)
		#ifdef _MSC_VER
			#pragma optimize("", on)
		#endif


//[-------------------------------------------------------]
//[ Anonymous detail namespace                            ]
//[-------------------------------------------------------]
	} // detail
}




//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
namespace VulkanRenderer
{




	//[-------------------------------------------------------]
	//[ VulkanRenderer/VulkanRenderer.h                       ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Vulkan renderer class
	*/
	class VulkanRenderer final : public Renderer::IRenderer
	{


	//[-------------------------------------------------------]
	//[ Friends                                               ]
	//[-------------------------------------------------------]
		friend class GraphicsPipelineState;


	//[-------------------------------------------------------]
	//[ Public definitions                                    ]
	//[-------------------------------------------------------]
	public:
		typedef std::array<VkClearValue, 9>	VkClearValues;	///< 8 color render targets and one depth stencil render target


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] context
		*    Renderer context, the renderer context instance must stay valid as long as the renderer instance exists
		*
		*  @note
		*    - Do never ever use a not properly initialized renderer! Use "Renderer::IRenderer::isInitialized()" to check the initialization state.
		*/
		explicit VulkanRenderer(const Renderer::Context& context);

		/**
		*  @brief
		*    Destructor
		*/
		virtual ~VulkanRenderer() override;

		/**
		*  @brief
		*    Return the Vulkan allocation callbacks
		*
		*  @return
		*    Vulkan allocation callbacks, can be a null pointer, don't destroy the instance
		*/
		inline const VkAllocationCallbacks* getVkAllocationCallbacks() const
		{
			#ifdef VK_USE_PLATFORM_WIN32_KHR
				return &mVkAllocationCallbacks;
			#else
				#warning TODO(co) The "Renderer::DefaultAllocator" implementation is currently only tested on MS Window, since Vulkan is using aligment it must be sure the custom standard implemtation runs fine
				return nullptr;
			#endif
		}

		/**
		*  @brief
		*    Return the Vulkan runtime linking instance
		*
		*  @return
		*    The Vulkan runtime linking instance, do not free the memory the reference is pointing to
		*/
		inline const VulkanRuntimeLinking& getVulkanRuntimeLinking() const
		{
			return *mVulkanRuntimeLinking;
		}

		/**
		*  @brief
		*    Return the Vulkan context instance
		*
		*  @return
		*    The Vulkan context instance, do not free the memory the reference is pointing to
		*/
		inline const VulkanContext& getVulkanContext() const
		{
			return *mVulkanContext;
		}

		//[-------------------------------------------------------]
		//[ Graphics                                              ]
		//[-------------------------------------------------------]
		void setGraphicsRootSignature(Renderer::IRootSignature* rootSignature);
		void setGraphicsPipelineState(Renderer::IGraphicsPipelineState* graphicsPipelineState);
		void setGraphicsResourceGroup(uint32_t rootParameterIndex, Renderer::IResourceGroup* resourceGroup);
		void setGraphicsVertexArray(Renderer::IVertexArray* vertexArray);															// Input-assembler (IA) stage
		void setGraphicsViewports(uint32_t numberOfViewports, const Renderer::Viewport* viewports);									// Rasterizer (RS) stage
		void setGraphicsScissorRectangles(uint32_t numberOfScissorRectangles, const Renderer::ScissorRectangle* scissorRectangles);	// Rasterizer (RS) stage
		void setGraphicsRenderTarget(Renderer::IRenderTarget* renderTarget);														// Output-merger (OM) stage
		void clearGraphics(uint32_t clearFlags, const float color[4], float z, uint32_t stencil);
		void drawGraphics(const Renderer::IIndirectBuffer& indirectBuffer, uint32_t indirectBufferOffset = 0, uint32_t numberOfDraws = 1);
		void drawGraphicsEmulated(const uint8_t* emulationData, uint32_t indirectBufferOffset = 0, uint32_t numberOfDraws = 1);
		void drawIndexedGraphics(const Renderer::IIndirectBuffer& indirectBuffer, uint32_t indirectBufferOffset = 0, uint32_t numberOfDraws = 1);
		void drawIndexedGraphicsEmulated(const uint8_t* emulationData, uint32_t indirectBufferOffset = 0, uint32_t numberOfDraws = 1);
		//[-------------------------------------------------------]
		//[ Compute                                               ]
		//[-------------------------------------------------------]
		void setComputeRootSignature(Renderer::IRootSignature* rootSignature);
		void setComputePipelineState(Renderer::IComputePipelineState* computePipelineState);
		void setComputeResourceGroup(uint32_t rootParameterIndex, Renderer::IResourceGroup* resourceGroup);
		//[-------------------------------------------------------]
		//[ Resource                                              ]
		//[-------------------------------------------------------]
		void resolveMultisampleFramebuffer(Renderer::IRenderTarget& destinationRenderTarget, Renderer::IFramebuffer& sourceMultisampleFramebuffer);
		void copyResource(Renderer::IResource& destinationResource, Renderer::IResource& sourceResource);
		//[-------------------------------------------------------]
		//[ Debug                                                 ]
		//[-------------------------------------------------------]
		#ifdef RENDERER_DEBUG
			void setDebugMarker(const char* name);
			void beginDebugEvent(const char* name);
			void endDebugEvent();
		#endif


	//[-------------------------------------------------------]
	//[ Public virtual Renderer::IRenderer methods            ]
	//[-------------------------------------------------------]
	public:
		virtual const char* getName() const override;
		virtual bool isInitialized() const override;
		virtual bool isDebugEnabled() override;
		//[-------------------------------------------------------]
		//[ Shader language                                       ]
		//[-------------------------------------------------------]
		virtual uint32_t getNumberOfShaderLanguages() const override;
		virtual const char* getShaderLanguageName(uint32_t index) const override;
		virtual Renderer::IShaderLanguage* getShaderLanguage(const char* shaderLanguageName = nullptr) override;
		//[-------------------------------------------------------]
		//[ Resource creation                                     ]
		//[-------------------------------------------------------]
		virtual Renderer::IRenderPass* createRenderPass(uint32_t numberOfColorAttachments, const Renderer::TextureFormat::Enum* colorAttachmentTextureFormats, Renderer::TextureFormat::Enum depthStencilAttachmentTextureFormat = Renderer::TextureFormat::UNKNOWN, uint8_t numberOfMultisamples = 1) override;
		virtual Renderer::ISwapChain* createSwapChain(Renderer::IRenderPass& renderPass, Renderer::WindowHandle windowHandle, bool useExternalContext = false) override;
		virtual Renderer::IFramebuffer* createFramebuffer(Renderer::IRenderPass& renderPass, const Renderer::FramebufferAttachment* colorFramebufferAttachments, const Renderer::FramebufferAttachment* depthStencilFramebufferAttachment = nullptr) override;
		virtual Renderer::IBufferManager* createBufferManager() override;
		virtual Renderer::ITextureManager* createTextureManager() override;
		virtual Renderer::IRootSignature* createRootSignature(const Renderer::RootSignature& rootSignature) override;
		virtual Renderer::IGraphicsPipelineState* createGraphicsPipelineState(const Renderer::GraphicsPipelineState& graphicsPipelineState) override;
		virtual Renderer::IComputePipelineState* createComputePipelineState(Renderer::IRootSignature& rootSignature, Renderer::IComputeShader& computeShader) override;
		virtual Renderer::ISamplerState* createSamplerState(const Renderer::SamplerState& samplerState) override;
		//[-------------------------------------------------------]
		//[ Resource handling                                     ]
		//[-------------------------------------------------------]
		virtual bool map(Renderer::IResource& resource, uint32_t subresource, Renderer::MapType mapType, uint32_t mapFlags, Renderer::MappedSubresource& mappedSubresource) override;
		virtual void unmap(Renderer::IResource& resource, uint32_t subresource) override;
		//[-------------------------------------------------------]
		//[ Operations                                            ]
		//[-------------------------------------------------------]
		virtual bool beginScene() override;
		virtual void submitCommandBuffer(const Renderer::CommandBuffer& commandBuffer) override;
		virtual void endScene() override;
		//[-------------------------------------------------------]
		//[ Synchronization                                       ]
		//[-------------------------------------------------------]
		virtual void flush() override;
		virtual void finish() override;


	//[-------------------------------------------------------]
	//[ Protected virtual Renderer::RefCount methods          ]
	//[-------------------------------------------------------]
	protected:
		inline virtual void selfDestruct() override
		{
			RENDERER_DELETE(mContext, VulkanRenderer, this);
		}


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		explicit VulkanRenderer(const VulkanRenderer& source) = delete;
		VulkanRenderer& operator =(const VulkanRenderer& source) = delete;

		/**
		*  @brief
		*    Initialize the capabilities
		*/
		void initializeCapabilities();

		/**
		*  @brief
		*    Unset the currently used vertex array
		*/
		void unsetGraphicsVertexArray();

		/**
		*  @brief
		*    Begin Vulkan render pass
		*/
		void beginVulkanRenderPass();


	//[-------------------------------------------------------]
	//[ Private data                                          ]
	//[-------------------------------------------------------]
	private:
		VkAllocationCallbacks		mVkAllocationCallbacks;		///< Vulkan allocation callbacks
		VulkanRuntimeLinking*		mVulkanRuntimeLinking;		///< Vulkan runtime linking instance, always valid
		VulkanContext*				mVulkanContext;				///< Vulkan context instance, always valid
		Renderer::IShaderLanguage*	mShaderLanguageGlsl;		///< GLSL shader language instance (we keep a reference to it), can be a null pointer
		RootSignature*				mGraphicsRootSignature;		///< Currently set graphics root signature (we keep a reference to it), can be a null pointer
		RootSignature*				mComputeRootSignature;		///< Currently set compute root signature (we keep a reference to it), can be a null pointer
		Renderer::ISamplerState*	mDefaultSamplerState;		///< Default rasterizer state (we keep a reference to it), can be a null pointer
		bool						mInsideVulkanRenderPass;	///< Some Vulkan commands like "vkCmdClearColorImage()" can only be executed outside a Vulkan render pass, so need to delay starting a Vulkan render pass
		VkClearValues				mVkClearValues;
		//[-------------------------------------------------------]
		//[ Input-assembler (IA) stage                            ]
		//[-------------------------------------------------------]
		VertexArray* mVertexArray;	///< Currently set vertex array (we keep a reference to it), can be a null pointer
		//[-------------------------------------------------------]
		//[ Output-merger (OM) stage                              ]
		//[-------------------------------------------------------]
		Renderer::IRenderTarget* mRenderTarget;	///< Currently set render target (we keep a reference to it), can be a null pointer


	};




	//[-------------------------------------------------------]
	//[ VulkanRenderer/VulkanRuntimeLinking.h                 ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Vulkan runtime linking for creating and managing the Vulkan instance ("VkInstance")
	*/
	class VulkanRuntimeLinking final
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] vulkanRenderer
		*    Owner Vulkan renderer instance
		*  @param[in] enableValidation
		*    Enable validation?
		*/
		inline VulkanRuntimeLinking(VulkanRenderer& vulkanRenderer, bool enableValidation) :
			mVulkanRenderer(vulkanRenderer),
			mValidationEnabled(enableValidation),
			mVulkanSharedLibrary(nullptr),
			mEntryPointsRegistered(false),
			mVkInstance(VK_NULL_HANDLE),
			mVkDebugReportCallbackEXT(VK_NULL_HANDLE),
			mInstanceLevelFunctionsRegistered(false),
			mInitialized(false)
		{}

		/**
		*  @brief
		*    Destructor
		*/
		~VulkanRuntimeLinking()
		{
			// Destroy the Vulkan debug report callback
			if (VK_NULL_HANDLE != mVkDebugReportCallbackEXT)
			{
				vkDestroyDebugReportCallbackEXT(mVkInstance, mVkDebugReportCallbackEXT, mVulkanRenderer.getVkAllocationCallbacks());
			}

			// Destroy the Vulkan instance
			if (VK_NULL_HANDLE != mVkInstance)
			{
				vkDestroyInstance(mVkInstance, mVulkanRenderer.getVkAllocationCallbacks());
			}

			// Destroy the shared library instances
			#ifdef _WIN32
				if (nullptr != mVulkanSharedLibrary)
				{
					::FreeLibrary(static_cast<HMODULE>(mVulkanSharedLibrary));
				}
			#elif defined LINUX
				if (nullptr != mVulkanSharedLibrary)
				{
					::dlclose(mVulkanSharedLibrary);
				}
			#else
				#error "Unsupported platform"
			#endif
		}

		/**
		*  @brief
		*    Return whether or not validation is enabled
		*
		*  @return
		*    "true" if validation is enabled, else "false"
		*/
		inline bool isValidationEnabled() const
		{
			return mValidationEnabled;
		}

		/**
		*  @brief
		*    Return whether or not Vulkan is available
		*
		*  @return
		*    "true" if Vulkan is available, else "false"
		*/
		bool isVulkanAvaiable()
		{
			// Already initialized?
			if (!mInitialized)
			{
				// We're now initialized
				mInitialized = true;

				// Load the shared libraries
				if (loadSharedLibraries())
				{
					// Load the global level Vulkan function entry points
					mEntryPointsRegistered = loadGlobalLevelVulkanEntryPoints();
					if (mEntryPointsRegistered)
					{
						// Create the Vulkan instance
						const VkResult vkResult = createVulkanInstance(mValidationEnabled);
						if (VK_SUCCESS == vkResult)
						{
							// Load instance based instance level Vulkan function pointers
							mInstanceLevelFunctionsRegistered = loadInstanceLevelVulkanEntryPoints();

							// Setup debug callback
							if (mInstanceLevelFunctionsRegistered && mValidationEnabled)
							{
								setupDebugCallback();
							}
						}
						else
						{
							// Error!
							RENDERER_LOG(mVulkanRenderer.getContext(), CRITICAL, "Failed to create the Vulkan instance")
						}
					}
				}
			}

			// Entry points successfully registered?
			return (mEntryPointsRegistered && (VK_NULL_HANDLE != mVkInstance) && mInstanceLevelFunctionsRegistered);
		}

		/**
		*  @brief
		*    Return the Vulkan instance
		*
		*  @return
		*    Vulkan instance
		*/
		inline VkInstance getVkInstance() const
		{
			return mVkInstance;
		}

		/**
		*  @brief
		*    Load the device level Vulkan function entry points
		*
		*  @param[in] vkDevice
		*    Vulkan device instance to load the function entry pointers for
		*
		*  @return
		*    "true" if all went fine, else "false"
		*/
		bool loadDeviceLevelVulkanEntryPoints(VkDevice vkDevice) const
		{
			bool result = true;	// Success by default

			// Define a helper macro
			PRAGMA_WARNING_PUSH
			PRAGMA_WARNING_DISABLE_MSVC(4191)	// 'reinterpret_cast': unsafe conversion from 'PFN_vkVoidFunction' to '<x>'
			#define IMPORT_FUNC(funcName)																												\
				if (result)																																\
				{																																		\
					funcName = reinterpret_cast<PFN_##funcName>(vkGetDeviceProcAddr(vkDevice, #funcName));												\
					if (nullptr == funcName)																											\
					{																																	\
						RENDERER_LOG(mVulkanRenderer.getContext(), CRITICAL, "Failed to load instance based Vulkan function pointer \"%s\"", #funcName)	\
						result = false;																													\
					}																																	\
				}

			// Load the Vulkan device level function entry points
			IMPORT_FUNC(vkDestroyDevice)
			IMPORT_FUNC(vkCreateShaderModule)
			IMPORT_FUNC(vkDestroyShaderModule)
			IMPORT_FUNC(vkCreateBuffer)
			IMPORT_FUNC(vkDestroyBuffer)
			IMPORT_FUNC(vkMapMemory)
			IMPORT_FUNC(vkUnmapMemory)
			IMPORT_FUNC(vkCreateBufferView)
			IMPORT_FUNC(vkDestroyBufferView)
			IMPORT_FUNC(vkAllocateMemory)
			IMPORT_FUNC(vkFreeMemory)
			IMPORT_FUNC(vkGetBufferMemoryRequirements)
			IMPORT_FUNC(vkBindBufferMemory)
			IMPORT_FUNC(vkCreateRenderPass)
			IMPORT_FUNC(vkDestroyRenderPass)
			IMPORT_FUNC(vkCreateImage)
			IMPORT_FUNC(vkDestroyImage)
			IMPORT_FUNC(vkGetImageSubresourceLayout)
			IMPORT_FUNC(vkGetImageMemoryRequirements)
			IMPORT_FUNC(vkBindImageMemory)
			IMPORT_FUNC(vkCreateImageView)
			IMPORT_FUNC(vkDestroyImageView)
			IMPORT_FUNC(vkCreateSampler)
			IMPORT_FUNC(vkDestroySampler)
			IMPORT_FUNC(vkCreateSemaphore)
			IMPORT_FUNC(vkDestroySemaphore)
			IMPORT_FUNC(vkCreateFence)
			IMPORT_FUNC(vkDestroyFence)
			IMPORT_FUNC(vkWaitForFences)
			IMPORT_FUNC(vkCreateCommandPool)
			IMPORT_FUNC(vkDestroyCommandPool)
			IMPORT_FUNC(vkAllocateCommandBuffers)
			IMPORT_FUNC(vkFreeCommandBuffers)
			IMPORT_FUNC(vkBeginCommandBuffer)
			IMPORT_FUNC(vkEndCommandBuffer)
			IMPORT_FUNC(vkGetDeviceQueue)
			IMPORT_FUNC(vkQueueSubmit)
			IMPORT_FUNC(vkQueueWaitIdle)
			IMPORT_FUNC(vkDeviceWaitIdle)
			IMPORT_FUNC(vkCreateFramebuffer)
			IMPORT_FUNC(vkDestroyFramebuffer)
			IMPORT_FUNC(vkCreatePipelineCache)
			IMPORT_FUNC(vkDestroyPipelineCache)
			IMPORT_FUNC(vkCreatePipelineLayout)
			IMPORT_FUNC(vkDestroyPipelineLayout)
			IMPORT_FUNC(vkCreateGraphicsPipelines)
			IMPORT_FUNC(vkCreateComputePipelines)
			IMPORT_FUNC(vkDestroyPipeline)
			IMPORT_FUNC(vkCreateDescriptorPool)
			IMPORT_FUNC(vkDestroyDescriptorPool)
			IMPORT_FUNC(vkCreateDescriptorSetLayout)
			IMPORT_FUNC(vkDestroyDescriptorSetLayout)
			IMPORT_FUNC(vkAllocateDescriptorSets)
			IMPORT_FUNC(vkFreeDescriptorSets)
			IMPORT_FUNC(vkUpdateDescriptorSets)
			IMPORT_FUNC(vkCreateQueryPool)
			IMPORT_FUNC(vkDestroyQueryPool)
			IMPORT_FUNC(vkGetQueryPoolResults)
			IMPORT_FUNC(vkCmdBeginQuery)
			IMPORT_FUNC(vkCmdEndQuery)
			IMPORT_FUNC(vkCmdResetQueryPool)
			IMPORT_FUNC(vkCmdCopyQueryPoolResults)
			IMPORT_FUNC(vkCmdPipelineBarrier)
			IMPORT_FUNC(vkCmdBeginRenderPass)
			IMPORT_FUNC(vkCmdEndRenderPass)
			IMPORT_FUNC(vkCmdExecuteCommands)
			IMPORT_FUNC(vkCmdCopyImage)
			IMPORT_FUNC(vkCmdBlitImage)
			IMPORT_FUNC(vkCmdCopyBufferToImage)
			IMPORT_FUNC(vkCmdClearAttachments)
			IMPORT_FUNC(vkCmdCopyBuffer)
			IMPORT_FUNC(vkCmdBindDescriptorSets)
			IMPORT_FUNC(vkCmdBindPipeline)
			IMPORT_FUNC(vkCmdSetViewport)
			IMPORT_FUNC(vkCmdSetScissor)
			IMPORT_FUNC(vkCmdSetLineWidth)
			IMPORT_FUNC(vkCmdSetDepthBias)
			IMPORT_FUNC(vkCmdPushConstants)
			IMPORT_FUNC(vkCmdBindIndexBuffer)
			IMPORT_FUNC(vkCmdBindVertexBuffers)
			IMPORT_FUNC(vkCmdDraw)
			IMPORT_FUNC(vkCmdDrawIndexed)
			IMPORT_FUNC(vkCmdDrawIndirect)
			IMPORT_FUNC(vkCmdDrawIndexedIndirect)
			IMPORT_FUNC(vkCmdDispatch)
			IMPORT_FUNC(vkCmdClearColorImage)
			IMPORT_FUNC(vkCmdClearDepthStencilImage)
			// "VK_KHR_swapchain"-extension
			IMPORT_FUNC(vkCreateSwapchainKHR)
			IMPORT_FUNC(vkDestroySwapchainKHR)
			IMPORT_FUNC(vkGetSwapchainImagesKHR)
			IMPORT_FUNC(vkAcquireNextImageKHR)
			IMPORT_FUNC(vkQueuePresentKHR)

			// Undefine the helper macro
			#undef IMPORT_FUNC
			PRAGMA_WARNING_POP

			// Done
			return result;
		}


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		explicit VulkanRuntimeLinking(const VulkanRuntimeLinking& source) = delete;
		VulkanRuntimeLinking& operator =(const VulkanRuntimeLinking& source) = delete;

		/**
		*  @brief
		*    Load the shared libraries
		*
		*  @return
		*    "true" if all went fine, else "false"
		*/
		bool loadSharedLibraries()
		{
			// Load the shared library
			#ifdef _WIN32
				mVulkanSharedLibrary = ::LoadLibraryExA("vulkan-1.dll", nullptr, LOAD_WITH_ALTERED_SEARCH_PATH);
				if (nullptr == mVulkanSharedLibrary)
				{
					RENDERER_LOG(mVulkanRenderer.getContext(), CRITICAL, "Failed to load in the shared Vulkan library \"vulkan-1.dll\"")
				}
			#elif defined LINUX
				mVulkanSharedLibrary = ::dlopen("libvulkan.so", RTLD_NOW);
				if (nullptr == mVulkanSharedLibrary)
				{
					RENDERER_LOG(mVulkanRenderer.getContext(), CRITICAL, "Failed to load in the shared Vulkan library \"libvulkan-1.so\"")
				}
			#else
				#error "Unsupported platform"
			#endif

			// Done
			return (nullptr != mVulkanSharedLibrary);
		}

		/**
		*  @brief
		*    Load the global level Vulkan function entry points
		*
		*  @return
		*    "true" if all went fine, else "false"
		*/
		bool loadGlobalLevelVulkanEntryPoints() const
		{
			bool result = true;	// Success by default

			// Define a helper macro
			#ifdef _WIN32
				#define IMPORT_FUNC(funcName)																																					\
					if (result)																																									\
					{																																											\
						void* symbol = ::GetProcAddress(static_cast<HMODULE>(mVulkanSharedLibrary), #funcName);																					\
						if (nullptr != symbol)																																					\
						{																																										\
							*(reinterpret_cast<void**>(&(funcName))) = symbol;																													\
						}																																										\
						else																																									\
						{																																										\
							wchar_t moduleFilename[MAX_PATH];																																	\
							moduleFilename[0] = '\0';																																			\
							::GetModuleFileNameW(static_cast<HMODULE>(mVulkanSharedLibrary), moduleFilename, MAX_PATH);																			\
							RENDERER_LOG(mVulkanRenderer.getContext(), CRITICAL, "Failed to locate the entry point \"%s\" within the shared Vulkan library \"%s\"", #funcName, moduleFilename)	\
							result = false;																																						\
						}																																										\
					}
			#elif defined(__ANDROID__)
				#define IMPORT_FUNC(funcName)																																						\
					if (result)																																										\
					{																																												\
						void* symbol = ::dlsym(mVulkanSharedLibrary, #funcName);																													\
						if (nullptr != symbol)																																						\
						{																																											\
							*(reinterpret_cast<void**>(&(funcName))) = symbol;																														\
						}																																											\
						else																																										\
						{																																											\
							const char* libraryName = "unknown";																																	\
							RENDERER_LOG(mVulkanRenderer.getContext(), CRITICAL, "Failed to locate the Vulkan entry point \"%s\" within the Vulkan shared library \"%s\"", #funcName, libraryName)	\
							result = false;																																							\
						}																																											\
					}
			#elif defined LINUX
				#define IMPORT_FUNC(funcName)																																				\
					if (result)																																								\
					{																																										\
						void* symbol = ::dlsym(mVulkanSharedLibrary, #funcName);																											\
						if (nullptr != symbol)																																				\
						{																																									\
							*(reinterpret_cast<void**>(&(funcName))) = symbol;																												\
						}																																									\
						else																																								\
						{																																									\
							link_map *linkMap = nullptr;																																	\
							const char* libraryName = "unknown";																															\
							if (dlinfo(mVulkanSharedLibrary, RTLD_DI_LINKMAP, &linkMap))																									\
							{																																								\
								libraryName = linkMap->l_name;																																\
							}																																								\
							RENDERER_LOG(mVulkanRenderer.getContext(), CRITICAL, "Failed to locate the entry point \"%s\" within the shared Vulkan library \"%s\"", #funcName, libraryName)	\
							result = false;																																					\
						}																																									\
					}
			#else
				#error "Unsupported platform"
			#endif

			// Load the Vulkan global level function entry points
			IMPORT_FUNC(vkGetInstanceProcAddr);
			IMPORT_FUNC(vkGetDeviceProcAddr);
			IMPORT_FUNC(vkEnumerateInstanceExtensionProperties);
			IMPORT_FUNC(vkEnumerateInstanceLayerProperties);
			IMPORT_FUNC(vkCreateInstance);

			// Undefine the helper macro
			#undef IMPORT_FUNC

			// Done
			return result;
		}

		/**
		*  @brief
		*    Create the Vulkan instance
		*
		*  @param[in] enableValidation
		*    Enable validation layer? (don't do this for shipped products)
		*
		*  @return
		*    Vulkan instance creation result
		*/
		VkResult createVulkanInstance(bool enableValidation)
		{
			// Enable surface extensions depending on OS
			std::vector<const char*> enabledExtensions = { VK_KHR_SURFACE_EXTENSION_NAME };
			#ifdef VK_USE_PLATFORM_WIN32_KHR
				enabledExtensions.push_back(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
			#elif defined VK_USE_PLATFORM_ANDROID_KHR
				#warning "TODO(co) Not tested"
				enabledExtensions.push_back(VK_KHR_ANDROID_SURFACE_EXTENSION_NAME);
			#elif defined VK_USE_PLATFORM_XLIB_KHR || defined VK_USE_PLATFORM_WAYLAND_KHR
				#if defined VK_USE_PLATFORM_XLIB_KHR
					enabledExtensions.push_back(VK_KHR_XLIB_SURFACE_EXTENSION_NAME);
				#endif
				#if defined VK_USE_PLATFORM_WAYLAND_KHR
					enabledExtensions.push_back(VK_KHR_WAYLAND_SURFACE_EXTENSION_NAME);
				#endif
			#elif defined VK_USE_PLATFORM_XCB_KHR
				#warning "TODO(co) Not tested"
				enabledExtensions.push_back(VK_KHR_XCB_SURFACE_EXTENSION_NAME);
			#else
				#error "Unsupported platform"
			#endif
			if (enableValidation)
			{
				enabledExtensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
			}

			{ // Ensure the extensions we need are supported
				uint32_t propertyCount = 0;
				if ((vkEnumerateInstanceExtensionProperties(nullptr, &propertyCount, nullptr) != VK_SUCCESS) || (0 == propertyCount))
				{
					RENDERER_LOG(mVulkanRenderer.getContext(), CRITICAL, "Failed to enumerate Vulkan instance extension properties")
					return VK_ERROR_EXTENSION_NOT_PRESENT;
				}
				::detail::VkExtensionPropertiesVector vkExtensionPropertiesVector(propertyCount);
				if (vkEnumerateInstanceExtensionProperties(nullptr, &propertyCount, &vkExtensionPropertiesVector[0]) != VK_SUCCESS)
				{
					RENDERER_LOG(mVulkanRenderer.getContext(), CRITICAL, "Failed to enumerate Vulkan instance extension properties")
					return VK_ERROR_EXTENSION_NOT_PRESENT;
				}
				for (const char* enabledExtension : enabledExtensions)
				{
					if (!::detail::isExtensionAvailable(enabledExtension, vkExtensionPropertiesVector))
					{
						RENDERER_LOG(mVulkanRenderer.getContext(), CRITICAL, "Couldn't find Vulkan instance extension named \"%s\"", enabledExtension)
						return VK_ERROR_EXTENSION_NOT_PRESENT;
					}
				}
			}

			// TODO(co) Make it possible for the user to provide application related information?
			static constexpr VkApplicationInfo vkApplicationInfo =
			{
				VK_STRUCTURE_TYPE_APPLICATION_INFO,	// sType (VkStructureType)
				nullptr,							// pNext (const void*)
				"Unrimp Application",				// pApplicationName (const char*)
				VK_MAKE_VERSION(0, 0, 0),			// applicationVersion (uint32_t)
				"Unrimp",							// pEngineName (const char*)
				VK_MAKE_VERSION(0, 0, 0),			// engineVersion (uint32_t)
				VK_API_VERSION_1_0					// apiVersion (uint32_t)
			};

			const VkInstanceCreateInfo vkInstanceCreateInfo =
			{
				VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,							// sType (VkStructureType)
				nullptr,														// pNext (const void*)
				0,																// flags (VkInstanceCreateFlags)
				&vkApplicationInfo,												// pApplicationInfo (const VkApplicationInfo*)
				enableValidation ? ::detail::NUMBER_OF_VALIDATION_LAYERS : 0,	// enabledLayerCount (uint32_t)
				enableValidation ? ::detail::VALIDATION_LAYER_NAMES : nullptr,	// ppEnabledLayerNames (const char* const*)
				static_cast<uint32_t>(enabledExtensions.size()),				// enabledExtensionCount (uint32_t)
				enabledExtensions.data()										// ppEnabledExtensionNames (const char* const*)
			};
			VkResult vkResult = vkCreateInstance(&vkInstanceCreateInfo, mVulkanRenderer.getVkAllocationCallbacks(), &mVkInstance);
			if (VK_ERROR_LAYER_NOT_PRESENT == vkResult && enableValidation)
			{
				// Error! Since the show must go on, try creating a Vulkan instance without validation enabled...
				RENDERER_LOG(mVulkanRenderer.getContext(), WARNING, "Failed to create the Vulkan instance with validation enabled, layer is not present. Install e.g. the LunarG Vulkan SDK and see e.g. https://vulkan.lunarg.com/doc/view/1.0.51.0/windows/layers.html .")
				mValidationEnabled = false;
				vkResult = createVulkanInstance(mValidationEnabled);
			}

			// Done
			return vkResult;
		}

		/**
		*  @brief
		*    Load the instance level Vulkan function entry points
		*
		*  @return
		*    "true" if all went fine, else "false"
		*/
		bool loadInstanceLevelVulkanEntryPoints() const
		{
			bool result = true;	// Success by default

			// Define a helper macro
			PRAGMA_WARNING_PUSH
			PRAGMA_WARNING_DISABLE_MSVC(4191)	// 'reinterpret_cast': unsafe conversion from 'PFN_vkVoidFunction' to '<x>'
			#define IMPORT_FUNC(funcName)																												\
				if (result)																																\
				{																																		\
					funcName = reinterpret_cast<PFN_##funcName>(vkGetInstanceProcAddr(mVkInstance, #funcName));											\
					if (nullptr == funcName)																											\
					{																																	\
						RENDERER_LOG(mVulkanRenderer.getContext(), CRITICAL, "Failed to load instance based Vulkan function pointer \"%s\"", #funcName)	\
						result = false;																													\
					}																																	\
				}

			// Load the Vulkan instance level function entry points
			IMPORT_FUNC(vkDestroyInstance)
			IMPORT_FUNC(vkEnumeratePhysicalDevices)
			IMPORT_FUNC(vkEnumerateDeviceLayerProperties)
			IMPORT_FUNC(vkEnumerateDeviceExtensionProperties)
			IMPORT_FUNC(vkGetPhysicalDeviceQueueFamilyProperties)
			IMPORT_FUNC(vkGetPhysicalDeviceFeatures)
			IMPORT_FUNC(vkGetPhysicalDeviceFormatProperties)
			IMPORT_FUNC(vkGetPhysicalDeviceMemoryProperties)
			IMPORT_FUNC(vkGetPhysicalDeviceProperties)
			IMPORT_FUNC(vkCreateDevice)
			if (mValidationEnabled)
			{
				// "VK_EXT_debug_report"-extension
				IMPORT_FUNC(vkCreateDebugReportCallbackEXT)
				IMPORT_FUNC(vkDestroyDebugReportCallbackEXT)
			}
			// "VK_KHR_surface"-extension
			IMPORT_FUNC(vkDestroySurfaceKHR)
			IMPORT_FUNC(vkGetPhysicalDeviceSurfaceSupportKHR)
			IMPORT_FUNC(vkGetPhysicalDeviceSurfaceFormatsKHR)
			IMPORT_FUNC(vkGetPhysicalDeviceSurfaceCapabilitiesKHR)
			IMPORT_FUNC(vkGetPhysicalDeviceSurfacePresentModesKHR)
			#ifdef VK_USE_PLATFORM_WIN32_KHR
				// "VK_KHR_win32_surface"-extension
				IMPORT_FUNC(vkCreateWin32SurfaceKHR)
			#elif defined VK_USE_PLATFORM_ANDROID_KHR
				// "VK_KHR_android_surface"-extension
				#warning "TODO(co) Not tested"
				IMPORT_FUNC(vkCreateAndroidSurfaceKHR)
			#elif defined VK_USE_PLATFORM_XLIB_KHR || defined VK_USE_PLATFORM_WAYLAND_KHR
				#if defined VK_USE_PLATFORM_XLIB_KHR
					// "VK_KHR_xlib_surface"-extension
					IMPORT_FUNC(vkCreateXlibSurfaceKHR)
				#endif
				#if defined VK_USE_PLATFORM_WAYLAND_KHR
					// "VK_KHR_wayland_surface"-extension
					IMPORT_FUNC(vkCreateWaylandSurfaceKHR)
				#endif
			#elif defined VK_USE_PLATFORM_XCB_KHR
				// "VK_KHR_xcb_surface"-extension
				#warning "TODO(co) Not tested"
				IMPORT_FUNC(vkCreateXcbSurfaceKHR)
			#else
				#error "Unsupported platform"
			#endif

			// Undefine the helper macro
			#undef IMPORT_FUNC
			PRAGMA_WARNING_POP

			// Done
			return result;
		}

		/**
		*  @brief
		*    Setup debug callback
		*/
		void setupDebugCallback()
		{
			// Sanity check
			RENDERER_ASSERT(mVulkanRenderer.getContext(), mValidationEnabled, "Do only call this Vulkan method if validation is enabled")

			// The report flags determine what type of messages for the layers will be displayed
			// -> Use "VK_DEBUG_REPORT_FLAG_BITS_MAX_ENUM_EXT" to get everything, quite verbose
			static constexpr VkDebugReportFlagsEXT vkDebugReportFlagsEXT = (VK_DEBUG_REPORT_WARNING_BIT_EXT | VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT | VK_DEBUG_REPORT_ERROR_BIT_EXT);

			// Setup debug callback
			const VkDebugReportCallbackCreateInfoEXT vkDebugReportCallbackCreateInfoEXT =
			{
				VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT,		// sType (VkStructureType)
				nullptr,														// pNext (const void*)
				vkDebugReportFlagsEXT,											// flags (VkDebugReportFlagsEXT)
				::detail::debugReportCallback,									// pfnCallback (PFN_vkDebugReportCallbackEXT)
				const_cast<Renderer::Context*>(&mVulkanRenderer.getContext())	// pUserData (void*)
			};
			if (vkCreateDebugReportCallbackEXT(mVkInstance, &vkDebugReportCallbackCreateInfoEXT, mVulkanRenderer.getVkAllocationCallbacks(), &mVkDebugReportCallbackEXT) != VK_SUCCESS)
			{
				RENDERER_LOG(mVulkanRenderer.getContext(), WARNING, "Failed to create the Vulkan debug report callback")
			}
		}


	//[-------------------------------------------------------]
	//[ Private data                                          ]
	//[-------------------------------------------------------]
	private:
		VulkanRenderer&			 mVulkanRenderer;					///< Owner Vulkan renderer instance
		bool					 mValidationEnabled;				///< Validation enabled?
		void*					 mVulkanSharedLibrary;				///< Vulkan shared library, can be a null pointer
		bool					 mEntryPointsRegistered;			///< Entry points successfully registered?
		VkInstance				 mVkInstance;						///< Vulkan instance, stores all per-application states
		VkDebugReportCallbackEXT mVkDebugReportCallbackEXT;			///< Vulkan debug report callback, can be a null handle
		bool					 mInstanceLevelFunctionsRegistered;	///< Instance level Vulkan function pointers registered?
		bool					 mInitialized;						///< Already initialized?


	};




	//[-------------------------------------------------------]
	//[ VulkanRenderer/VulkanContext.h                        ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Vulkan context class
	*/
	class VulkanContext final
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] vulkanRenderer
		*    Owner Vulkan renderer instance
		*/
		explicit VulkanContext(VulkanRenderer& vulkanRenderer) :
			mVulkanRenderer(vulkanRenderer),
			mVkPhysicalDevice(VK_NULL_HANDLE),
			mVkDevice(VK_NULL_HANDLE),
			mGraphicsQueueFamilyIndex(~0u),
			mPresentQueueFamilyIndex(~0u),
			mGraphicsVkQueue(VK_NULL_HANDLE),
			mPresentVkQueue(VK_NULL_HANDLE),
			mVkCommandPool(VK_NULL_HANDLE),
			mVkCommandBuffer(VK_NULL_HANDLE)
		{
			const VulkanRuntimeLinking& vulkanRuntimeLinking = mVulkanRenderer.getVulkanRuntimeLinking();

			// Get the physical Vulkan device this context should use
			bool enableDebugMarker = true;	// TODO(co) Make it possible to setup from the outside whether or not the "VK_EXT_debug_marker"-extension should be used (e.g. retail shipped games might not want to have this enabled)
			{
				detail::VkPhysicalDevices vkPhysicalDevices;
				::detail::enumeratePhysicalDevices(vulkanRenderer.getContext(), vulkanRuntimeLinking.getVkInstance(), vkPhysicalDevices);
				if (!vkPhysicalDevices.empty())
				{
					mVkPhysicalDevice = ::detail::selectPhysicalDevice(vulkanRenderer.getContext(), vkPhysicalDevices, vulkanRenderer.getVulkanRuntimeLinking().isValidationEnabled(), enableDebugMarker);
				}
			}

			// Create the logical Vulkan device instance
			if (VK_NULL_HANDLE != mVkPhysicalDevice)
			{
				mVkDevice = ::detail::createVkDevice(mVulkanRenderer.getContext(), mVulkanRenderer.getVkAllocationCallbacks(), mVkPhysicalDevice, vulkanRuntimeLinking.isValidationEnabled(), enableDebugMarker, mGraphicsQueueFamilyIndex, mPresentQueueFamilyIndex);
				if (VK_NULL_HANDLE != mVkDevice)
				{
					// Load device based instance level Vulkan function pointers
					if (mVulkanRenderer.getVulkanRuntimeLinking().loadDeviceLevelVulkanEntryPoints(mVkDevice))
					{
						// Get the Vulkan device graphics queue that command buffers are submitted to
						vkGetDeviceQueue(mVkDevice, mGraphicsQueueFamilyIndex, 0, &mGraphicsVkQueue);
						if (VK_NULL_HANDLE != mGraphicsVkQueue)
						{
							// Get the Vulkan device present queue
							vkGetDeviceQueue(mVkDevice, mPresentQueueFamilyIndex, 0, &mPresentVkQueue);
							if (VK_NULL_HANDLE != mPresentVkQueue)
							{
								// Create Vulkan command pool instance
								mVkCommandPool = ::detail::createVkCommandPool(mVulkanRenderer.getContext(), mVulkanRenderer.getVkAllocationCallbacks(), mVkDevice, mGraphicsQueueFamilyIndex);
								if (VK_NULL_HANDLE != mVkCommandPool)
								{
									// Create Vulkan command buffer instance
									mVkCommandBuffer = ::detail::createVkCommandBuffer(mVulkanRenderer.getContext(), mVkDevice, mVkCommandPool);
								}
								else
								{
									// Error!
									RENDERER_LOG(vulkanRenderer.getContext(), CRITICAL, "Failed to create Vulkan command pool instance")
								}
							}
						}
						else
						{
							// Error!
							RENDERER_LOG(vulkanRenderer.getContext(), CRITICAL, "Failed to get the Vulkan device graphics queue that command buffers are submitted to")
						}
					}
				}
			}
		}

		/**
		*  @brief
		*    Destructor
		*/
		~VulkanContext()
		{
			if (VK_NULL_HANDLE != mVkDevice)
			{
				if (VK_NULL_HANDLE != mVkCommandPool)
				{
					if (VK_NULL_HANDLE != mVkCommandBuffer)
					{
						vkFreeCommandBuffers(mVkDevice, mVkCommandPool, 1, &mVkCommandBuffer);
					}
					vkDestroyCommandPool(mVkDevice, mVkCommandPool, mVulkanRenderer.getVkAllocationCallbacks());
				}
				vkDeviceWaitIdle(mVkDevice);
				vkDestroyDevice(mVkDevice, mVulkanRenderer.getVkAllocationCallbacks());
			}
		}

		/**
		*  @brief
		*    Return whether or not the content is initialized
		*
		*  @return
		*    "true" if the context is initialized, else "false"
		*/
		inline bool isInitialized() const
		{
			return (VK_NULL_HANDLE != mVkCommandBuffer);
		}

		/**
		*  @brief
		*    Return the owner Vulkan renderer instance
		*
		*  @return
		*    Owner Vulkan renderer instance
		*/
		inline VulkanRenderer& getVulkanRenderer() const
		{
			return mVulkanRenderer;
		}

		/**
		*  @brief
		*    Return the Vulkan physical device this context is using
		*
		*  @return
		*    The Vulkan physical device this context is using
		*/
		inline VkPhysicalDevice getVkPhysicalDevice() const
		{
			return mVkPhysicalDevice;
		}

		/**
		*  @brief
		*    Return the Vulkan device this context is using
		*
		*  @return
		*    The Vulkan device this context is using
		*/
		inline VkDevice getVkDevice() const
		{
			return mVkDevice;
		}

		/**
		*  @brief
		*    Return the used graphics queue family index
		*
		*  @return
		*    Graphics queue family index, ~0u if invalid
		*/
		inline uint32_t getGraphicsQueueFamilyIndex() const
		{
			return mGraphicsQueueFamilyIndex;
		}

		/**
		*  @brief
		*    Return the used present queue family index
		*
		*  @return
		*    Present queue family index, ~0u if invalid
		*/
		inline uint32_t getPresentQueueFamilyIndex() const
		{
			return mPresentQueueFamilyIndex;
		}

		/**
		*  @brief
		*    Return the handle to the Vulkan device graphics queue that command buffers are submitted to
		*
		*  @return
		*    Handle to the Vulkan device graphics queue that command buffers are submitted to
		*/
		inline VkQueue getGraphicsVkQueue() const
		{
			return mGraphicsVkQueue;
		}

		/**
		*  @brief
		*    Return the handle to the Vulkan device present queue
		*
		*  @return
		*    Handle to the Vulkan device present queue
		*/
		inline VkQueue getPresentVkQueue() const
		{
			return mPresentVkQueue;
		}

		/**
		*  @brief
		*    Return the used Vulkan command buffer pool instance
		*
		*  @return
		*    The used Vulkan command buffer pool instance
		*/
		inline VkCommandPool getVkCommandPool() const
		{
			return mVkCommandPool;
		}

		/**
		*  @brief
		*    Return the Vulkan command buffer instance
		*
		*  @return
		*    The Vulkan command buffer instance
		*/
		inline VkCommandBuffer getVkCommandBuffer() const
		{
			return mVkCommandBuffer;
		}

		// TODO(co) Trivial implementation to have something to start with. Need to use more clever memory management and stating buffers later on.
		uint32_t findMemoryTypeIndex(uint32_t typeFilter, VkMemoryPropertyFlags vkMemoryPropertyFlags) const
		{
			VkPhysicalDeviceMemoryProperties vkPhysicalDeviceMemoryProperties;
			vkGetPhysicalDeviceMemoryProperties(mVkPhysicalDevice, &vkPhysicalDeviceMemoryProperties);
			for (uint32_t i = 0; i < vkPhysicalDeviceMemoryProperties.memoryTypeCount; ++i)
			{
				if ((typeFilter & (1 << i)) && (vkPhysicalDeviceMemoryProperties.memoryTypes[i].propertyFlags & vkMemoryPropertyFlags) == vkMemoryPropertyFlags)
				{
					return i;
				}
			}

			// Error!
			RENDERER_LOG(mVulkanRenderer.getContext(), CRITICAL, "Failed to find suitable Vulkan memory type")
			return ~0u;
		}

		inline VkCommandBuffer createVkCommandBuffer() const
		{
			return ::detail::createVkCommandBuffer(mVulkanRenderer.getContext(), mVkDevice, mVkCommandPool);
		}

		void destroyVkCommandBuffer(VkCommandBuffer vkCommandBuffer) const
		{
			if (VK_NULL_HANDLE != mVkCommandBuffer)
			{
				vkFreeCommandBuffers(mVkDevice, mVkCommandPool, 1, &vkCommandBuffer);
			}
		}


	//[-------------------------------------------------------]
	//[ Protected methods                                     ]
	//[-------------------------------------------------------]
	protected:
		explicit VulkanContext(const VulkanContext& source) = delete;
		VulkanContext& operator =(const VulkanContext& source) = delete;


	//[-------------------------------------------------------]
	//[ Private data                                          ]
	//[-------------------------------------------------------]
	private:
		VulkanRenderer&  mVulkanRenderer;			///< Owner Vulkan renderer instance
		VkPhysicalDevice mVkPhysicalDevice;			///< Vulkan physical device this context is using
		VkDevice		 mVkDevice;					///< Vulkan device instance this context is using (equivalent of a OpenGL context or Direct3D 11 device)
		uint32_t		 mGraphicsQueueFamilyIndex;	///< Graphics queue family index, ~0u if invalid
		uint32_t		 mPresentQueueFamilyIndex;	///< Present queue family index, ~0u if invalid
		VkQueue			 mGraphicsVkQueue;			///< Handle to the Vulkan device graphics queue that command buffers are submitted to
		VkQueue			 mPresentVkQueue;			///< Handle to the Vulkan device present queue
		VkCommandPool	 mVkCommandPool;			///< Vulkan command buffer pool instance
		VkCommandBuffer  mVkCommandBuffer;			///< Vulkan command buffer instance


	};




	//[-------------------------------------------------------]
	//[ VulkanRenderer/Mapping.h                              ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Vulkan mapping
	*/
	class Mapping final
	{


	//[-------------------------------------------------------]
	//[ Public static methods                                 ]
	//[-------------------------------------------------------]
	public:
		//[-------------------------------------------------------]
		//[ Renderer::FilterMode                                  ]
		//[-------------------------------------------------------]
		/**
		*  @brief
		*    "Renderer::FilterMode" to Vulkan magnification filter mode
		*
		*  @param[in] context
		*    Renderer context to use
		*  @param[in] filterMode
		*    "Renderer::FilterMode" to map
		*
		*  @return
		*    Vulkan magnification filter mode
		*/
		static VkFilter getVulkanMagFilterMode(MAYBE_UNUSED const Renderer::Context& context, Renderer::FilterMode filterMode)
		{
			switch (filterMode)
			{
				case Renderer::FilterMode::MIN_MAG_MIP_POINT:
					return VK_FILTER_NEAREST;

				case Renderer::FilterMode::MIN_MAG_POINT_MIP_LINEAR:
					return VK_FILTER_NEAREST;

				case Renderer::FilterMode::MIN_POINT_MAG_LINEAR_MIP_POINT:
					return VK_FILTER_LINEAR;

				case Renderer::FilterMode::MIN_POINT_MAG_MIP_LINEAR:
					return VK_FILTER_LINEAR;

				case Renderer::FilterMode::MIN_LINEAR_MAG_MIP_POINT:
					return VK_FILTER_NEAREST;

				case Renderer::FilterMode::MIN_LINEAR_MAG_POINT_MIP_LINEAR:
					return VK_FILTER_NEAREST;

				case Renderer::FilterMode::MIN_MAG_LINEAR_MIP_POINT:
					return VK_FILTER_LINEAR;

				case Renderer::FilterMode::MIN_MAG_MIP_LINEAR:
					return VK_FILTER_LINEAR;

				case Renderer::FilterMode::ANISOTROPIC:
					return VK_FILTER_LINEAR;	// There's no special setting in Vulkan

				case Renderer::FilterMode::COMPARISON_MIN_MAG_MIP_POINT:
					return VK_FILTER_NEAREST;

				case Renderer::FilterMode::COMPARISON_MIN_MAG_POINT_MIP_LINEAR:
					return VK_FILTER_NEAREST;

				case Renderer::FilterMode::COMPARISON_MIN_POINT_MAG_LINEAR_MIP_POINT:
					return VK_FILTER_LINEAR;

				case Renderer::FilterMode::COMPARISON_MIN_POINT_MAG_MIP_LINEAR:
					return VK_FILTER_LINEAR;

				case Renderer::FilterMode::COMPARISON_MIN_LINEAR_MAG_MIP_POINT:
					return VK_FILTER_NEAREST;

				case Renderer::FilterMode::COMPARISON_MIN_LINEAR_MAG_POINT_MIP_LINEAR:
					return VK_FILTER_NEAREST;

				case Renderer::FilterMode::COMPARISON_MIN_MAG_LINEAR_MIP_POINT:
					return VK_FILTER_LINEAR;

				case Renderer::FilterMode::COMPARISON_MIN_MAG_MIP_LINEAR:
					return VK_FILTER_LINEAR;

				case Renderer::FilterMode::COMPARISON_ANISOTROPIC:
					return VK_FILTER_LINEAR;	// There's no special setting in Vulkan

				case Renderer::FilterMode::UNKNOWN:
					RENDERER_ASSERT(context, false, "Vulkan filter mode must not be unknown")
					return VK_FILTER_NEAREST;

				default:
					return VK_FILTER_NEAREST;	// We should never be in here
			}
		}

		/**
		*  @brief
		*    "Renderer::FilterMode" to Vulkan minification filter mode
		*
		*  @param[in] context
		*    Renderer context to use
		*  @param[in] filterMode
		*    "Renderer::FilterMode" to map
		*
		*  @return
		*    Vulkan minification filter mode
		*/
		static VkFilter getVulkanMinFilterMode(MAYBE_UNUSED const Renderer::Context& context, Renderer::FilterMode filterMode)
		{
			switch (filterMode)
			{
				case Renderer::FilterMode::MIN_MAG_MIP_POINT:
					return VK_FILTER_NEAREST;

				case Renderer::FilterMode::MIN_MAG_POINT_MIP_LINEAR:
					return VK_FILTER_NEAREST;

				case Renderer::FilterMode::MIN_POINT_MAG_LINEAR_MIP_POINT:
					return VK_FILTER_NEAREST;

				case Renderer::FilterMode::MIN_POINT_MAG_MIP_LINEAR:
					return VK_FILTER_NEAREST;

				case Renderer::FilterMode::MIN_LINEAR_MAG_MIP_POINT:
					return VK_FILTER_LINEAR;

				case Renderer::FilterMode::MIN_LINEAR_MAG_POINT_MIP_LINEAR:
					return VK_FILTER_LINEAR;

				case Renderer::FilterMode::MIN_MAG_LINEAR_MIP_POINT:
					return VK_FILTER_LINEAR;

				case Renderer::FilterMode::MIN_MAG_MIP_LINEAR:
					return VK_FILTER_LINEAR;

				case Renderer::FilterMode::ANISOTROPIC:
					return VK_FILTER_LINEAR;	// There's no special setting in Vulkan

				case Renderer::FilterMode::COMPARISON_MIN_MAG_MIP_POINT:
					return VK_FILTER_NEAREST;

				case Renderer::FilterMode::COMPARISON_MIN_MAG_POINT_MIP_LINEAR:
					return VK_FILTER_NEAREST;

				case Renderer::FilterMode::COMPARISON_MIN_POINT_MAG_LINEAR_MIP_POINT:
					return VK_FILTER_NEAREST;

				case Renderer::FilterMode::COMPARISON_MIN_POINT_MAG_MIP_LINEAR:
					return VK_FILTER_NEAREST;

				case Renderer::FilterMode::COMPARISON_MIN_LINEAR_MAG_MIP_POINT:
					return VK_FILTER_LINEAR;

				case Renderer::FilterMode::COMPARISON_MIN_LINEAR_MAG_POINT_MIP_LINEAR:
					return VK_FILTER_LINEAR;

				case Renderer::FilterMode::COMPARISON_MIN_MAG_LINEAR_MIP_POINT:
					return VK_FILTER_LINEAR;

				case Renderer::FilterMode::COMPARISON_MIN_MAG_MIP_LINEAR:
					return VK_FILTER_LINEAR;

				case Renderer::FilterMode::COMPARISON_ANISOTROPIC:
					return VK_FILTER_LINEAR;	// There's no special setting in Vulkan

				case Renderer::FilterMode::UNKNOWN:
					RENDERER_ASSERT(context, false, "Vulkan filter mode must not be unknown")
					return VK_FILTER_NEAREST;

				default:
					return VK_FILTER_NEAREST;	// We should never be in here
			}
		}

		/**
		*  @brief
		*    "Renderer::FilterMode" to Vulkan sampler mipmap mode
		*
		*  @param[in] context
		*    Renderer context to use
		*  @param[in] filterMode
		*    "Renderer::FilterMode" to map
		*
		*  @return
		*    Vulkan sampler mipmap mode
		*/
		static VkSamplerMipmapMode getVulkanMipmapMode(MAYBE_UNUSED const Renderer::Context& context, Renderer::FilterMode filterMode)
		{
			switch (filterMode)
			{
				case Renderer::FilterMode::MIN_MAG_MIP_POINT:
					return VK_SAMPLER_MIPMAP_MODE_NEAREST;

				case Renderer::FilterMode::MIN_MAG_POINT_MIP_LINEAR:
					return VK_SAMPLER_MIPMAP_MODE_LINEAR;

				case Renderer::FilterMode::MIN_POINT_MAG_LINEAR_MIP_POINT:
					return VK_SAMPLER_MIPMAP_MODE_NEAREST;

				case Renderer::FilterMode::MIN_POINT_MAG_MIP_LINEAR:
					return VK_SAMPLER_MIPMAP_MODE_LINEAR;

				case Renderer::FilterMode::MIN_LINEAR_MAG_MIP_POINT:
					return VK_SAMPLER_MIPMAP_MODE_NEAREST;

				case Renderer::FilterMode::MIN_LINEAR_MAG_POINT_MIP_LINEAR:
					return VK_SAMPLER_MIPMAP_MODE_LINEAR;

				case Renderer::FilterMode::MIN_MAG_LINEAR_MIP_POINT:
					return VK_SAMPLER_MIPMAP_MODE_NEAREST;

				case Renderer::FilterMode::MIN_MAG_MIP_LINEAR:
					return VK_SAMPLER_MIPMAP_MODE_LINEAR;

				case Renderer::FilterMode::ANISOTROPIC:
					return VK_SAMPLER_MIPMAP_MODE_LINEAR;	// There's no special setting in Vulkan

				case Renderer::FilterMode::COMPARISON_MIN_MAG_MIP_POINT:
					return VK_SAMPLER_MIPMAP_MODE_NEAREST;

				case Renderer::FilterMode::COMPARISON_MIN_MAG_POINT_MIP_LINEAR:
					return VK_SAMPLER_MIPMAP_MODE_LINEAR;

				case Renderer::FilterMode::COMPARISON_MIN_POINT_MAG_LINEAR_MIP_POINT:
					return VK_SAMPLER_MIPMAP_MODE_NEAREST;

				case Renderer::FilterMode::COMPARISON_MIN_POINT_MAG_MIP_LINEAR:
					return VK_SAMPLER_MIPMAP_MODE_LINEAR;

				case Renderer::FilterMode::COMPARISON_MIN_LINEAR_MAG_MIP_POINT:
					return VK_SAMPLER_MIPMAP_MODE_NEAREST;

				case Renderer::FilterMode::COMPARISON_MIN_LINEAR_MAG_POINT_MIP_LINEAR:
					return VK_SAMPLER_MIPMAP_MODE_LINEAR;

				case Renderer::FilterMode::COMPARISON_MIN_MAG_LINEAR_MIP_POINT:
					return VK_SAMPLER_MIPMAP_MODE_NEAREST;

				case Renderer::FilterMode::COMPARISON_MIN_MAG_MIP_LINEAR:
					return VK_SAMPLER_MIPMAP_MODE_LINEAR;

				case Renderer::FilterMode::COMPARISON_ANISOTROPIC:
					return VK_SAMPLER_MIPMAP_MODE_LINEAR;	// There's no special setting in Vulkan

				case Renderer::FilterMode::UNKNOWN:
					RENDERER_ASSERT(context, false, "Vulkan filter mode must not be unknown")
					return VK_SAMPLER_MIPMAP_MODE_NEAREST;

				default:
					return VK_SAMPLER_MIPMAP_MODE_NEAREST;	// We should never be in here
			}
		}

		//[-------------------------------------------------------]
		//[ Renderer::TextureAddressMode                          ]
		//[-------------------------------------------------------]
		/**
		*  @brief
		*    "Renderer::TextureAddressMode" to Vulkan texture address mode
		*
		*  @param[in] textureAddressMode
		*    "Renderer::TextureAddressMode" to map
		*
		*  @return
		*    Vulkan texture address mode
		*/
		static VkSamplerAddressMode getVulkanTextureAddressMode(Renderer::TextureAddressMode textureAddressMode)
		{
			static constexpr VkSamplerAddressMode MAPPING[] =
			{
				VK_SAMPLER_ADDRESS_MODE_REPEAT,					// Renderer::TextureAddressMode::WRAP
				VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT,		// Renderer::TextureAddressMode::MIRROR
				VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,			// Renderer::TextureAddressMode::CLAMP
				VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER,		// Renderer::TextureAddressMode::BORDER
				VK_SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE	// Renderer::TextureAddressMode::MIRROR_ONCE
			};
			return MAPPING[static_cast<int>(textureAddressMode) - 1];	// Lookout! The "Renderer::TextureAddressMode"-values start with 1, not 0
		}

		//[-------------------------------------------------------]
		//[ Renderer::Blend                                       ]
		//[-------------------------------------------------------]
		/**
		*  @brief
		*    "Renderer::Blend" to Vulkan blend factor
		*
		*  @param[in] blend
		*    "Renderer::Blend" to map
		*
		*  @return
		*    Vulkan blend factor
		*/
		static VkBlendFactor getVulkanBlendFactor(Renderer::Blend blend)
		{
			static constexpr VkBlendFactor MAPPING[] =
			{
				VK_BLEND_FACTOR_ZERO,						// Renderer::Blend::ZERO			 = 1
				VK_BLEND_FACTOR_ONE,						// Renderer::Blend::ONE				 = 2
				VK_BLEND_FACTOR_SRC_COLOR,					// Renderer::Blend::SRC_COLOR		 = 3
				VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR,		// Renderer::Blend::INV_SRC_COLOR	 = 4
				VK_BLEND_FACTOR_SRC_ALPHA,					// Renderer::Blend::SRC_ALPHA		 = 5
				VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,		// Renderer::Blend::INV_SRC_ALPHA	 = 6
				VK_BLEND_FACTOR_DST_ALPHA,					// Renderer::Blend::DEST_ALPHA		 = 7
				VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA,		// Renderer::Blend::INV_DEST_ALPHA	 = 8
				VK_BLEND_FACTOR_DST_COLOR,					// Renderer::Blend::DEST_COLOR		 = 9
				VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR,		// Renderer::Blend::INV_DEST_COLOR	 = 10
				VK_BLEND_FACTOR_SRC_ALPHA_SATURATE,			// Renderer::Blend::SRC_ALPHA_SAT	 = 11
				VK_BLEND_FACTOR_MAX_ENUM,					// <undefined>						 = 12 !
				VK_BLEND_FACTOR_MAX_ENUM,					// <undefined>						 = 13 !
				VK_BLEND_FACTOR_CONSTANT_COLOR,				// Renderer::Blend::BLEND_FACTOR	 = 14
				VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_COLOR,	// Renderer::Blend::INV_BLEND_FACTOR = 15
				VK_BLEND_FACTOR_SRC1_COLOR,					// Renderer::Blend::SRC_1_COLOR		 = 16
				VK_BLEND_FACTOR_ONE_MINUS_SRC1_COLOR,		// Renderer::Blend::INV_SRC_1_COLOR	 = 17
				VK_BLEND_FACTOR_SRC1_ALPHA,					// Renderer::Blend::SRC_1_ALPHA		 = 18
				VK_BLEND_FACTOR_ONE_MINUS_SRC1_ALPHA		// Renderer::Blend::INV_SRC_1_ALPHA	 = 19
			};
			return MAPPING[static_cast<int>(blend) - 1];	// Lookout! The "Renderer::Blend"-values start with 1, not 0, there are also holes
		}

		/**
		*  @brief
		*    "Renderer::BlendOp" to Vulkan blend operation
		*
		*  @param[in] blendOp
		*    "Renderer::BlendOp" to map
		*
		*  @return
		*    Vulkan blend operation
		*/
		static VkBlendOp getVulkanBlendOp(Renderer::BlendOp blendOp)
		{
			static constexpr VkBlendOp MAPPING[] =
			{
				VK_BLEND_OP_ADD,				// Renderer::BlendOp::ADD
				VK_BLEND_OP_SUBTRACT,			// Renderer::BlendOp::SUBTRACT
				VK_BLEND_OP_REVERSE_SUBTRACT,	// Renderer::BlendOp::REV_SUBTRACT
				VK_BLEND_OP_MIN,				// Renderer::BlendOp::MIN
				VK_BLEND_OP_MAX					// Renderer::BlendOp::MAX
			};
			return MAPPING[static_cast<int>(blendOp) - 1];	// Lookout! The "Renderer::Blend"-values start with 1, not 0
		}

		//[-------------------------------------------------------]
		//[ Renderer::ComparisonFunc                              ]
		//[-------------------------------------------------------]
		/**
		*  @brief
		*    "Renderer::ComparisonFunc" to Vulkan comparison function
		*
		*  @param[in] comparisonFunc
		*    "Renderer::ComparisonFunc" to map
		*
		*  @return
		*    Vulkan comparison function
		*/
		static VkCompareOp getVulkanComparisonFunc(Renderer::ComparisonFunc comparisonFunc)
		{
			static constexpr VkCompareOp MAPPING[] =
			{
				VK_COMPARE_OP_NEVER,			// Renderer::ComparisonFunc::NEVER
				VK_COMPARE_OP_LESS,				// Renderer::ComparisonFunc::LESS
				VK_COMPARE_OP_EQUAL,			// Renderer::ComparisonFunc::EQUAL
				VK_COMPARE_OP_LESS_OR_EQUAL,	// Renderer::ComparisonFunc::LESS_EQUAL
				VK_COMPARE_OP_GREATER,			// Renderer::ComparisonFunc::GREATER
				VK_COMPARE_OP_NOT_EQUAL,		// Renderer::ComparisonFunc::NOT_EQUAL
				VK_COMPARE_OP_GREATER_OR_EQUAL,	// Renderer::ComparisonFunc::GREATER_EQUAL
				VK_COMPARE_OP_ALWAYS			// Renderer::ComparisonFunc::ALWAYS
			};
			return MAPPING[static_cast<int>(comparisonFunc) - 1];	// Lookout! The "Renderer::ComparisonFunc"-values start with 1, not 0
		}

		//[-------------------------------------------------------]
		//[ Renderer::VertexAttributeFormat and semantic          ]
		//[-------------------------------------------------------]
		/**
		*  @brief
		*    "Renderer::VertexAttributeFormat" to Vulkan format
		*
		*  @param[in] vertexAttributeFormat
		*    "Renderer::VertexAttributeFormat" to map
		*
		*  @return
		*    Vulkan format
		*/
		static VkFormat getVulkanFormat(Renderer::VertexAttributeFormat vertexAttributeFormat)
		{
			static constexpr VkFormat MAPPING[] =
			{
				VK_FORMAT_R32_SFLOAT,			// Renderer::VertexAttributeFormat::FLOAT_1
				VK_FORMAT_R32G32_SFLOAT,		// Renderer::VertexAttributeFormat::FLOAT_2
				VK_FORMAT_R32G32B32_SFLOAT,		// Renderer::VertexAttributeFormat::FLOAT_3
				VK_FORMAT_R32G32B32A32_SFLOAT,	// Renderer::VertexAttributeFormat::FLOAT_4
				VK_FORMAT_R8G8B8A8_UNORM,		// Renderer::VertexAttributeFormat::R8G8B8A8_UNORM
				VK_FORMAT_R8G8B8A8_UINT,		// Renderer::VertexAttributeFormat::R8G8B8A8_UINT
				VK_FORMAT_R16G16_SINT,			// Renderer::VertexAttributeFormat::SHORT_2
				VK_FORMAT_R16G16B16A16_SINT,	// Renderer::VertexAttributeFormat::SHORT_4
				VK_FORMAT_R32_UINT				// Renderer::VertexAttributeFormat::UINT_1
			};
			return MAPPING[static_cast<int>(vertexAttributeFormat)];
		}

		//[-------------------------------------------------------]
		//[ Renderer::IndexBufferFormat                           ]
		//[-------------------------------------------------------]
		/**
		*  @brief
		*    "Renderer::IndexBufferFormat" to Vulkan type
		*
		*  @param[in] context
		*    Renderer context to use
		*  @param[in] indexBufferFormat
		*    "Renderer::IndexBufferFormat" to map
		*
		*  @return
		*    Vulkan index type
		*/
		static VkIndexType getVulkanType(MAYBE_UNUSED const Renderer::Context& context, Renderer::IndexBufferFormat::Enum indexBufferFormat)
		{
			RENDERER_ASSERT(context, Renderer::IndexBufferFormat::UNSIGNED_CHAR != indexBufferFormat, "One byte per element index buffer format isn't supported by Vulkan")
			static constexpr VkIndexType MAPPING[] =
			{
				VK_INDEX_TYPE_MAX_ENUM,	// Renderer::IndexBufferFormat::UNSIGNED_CHAR  - One byte per element, uint8_t (may not be supported by each API) - Not supported by Vulkan
				VK_INDEX_TYPE_UINT16,	// Renderer::IndexBufferFormat::UNSIGNED_SHORT - Two bytes per element, uint16_t
				VK_INDEX_TYPE_UINT32	// Renderer::IndexBufferFormat::UNSIGNED_INT   - Four bytes per element, uint32_t (may not be supported by each API)
			};
			return MAPPING[indexBufferFormat];
		}

		//[-------------------------------------------------------]
		//[ Renderer::PrimitiveTopology                           ]
		//[-------------------------------------------------------]
		/**
		*  @brief
		*    "Renderer::PrimitiveTopology" to Vulkan type
		*
		*  @param[in] primitiveTopology
		*    "Renderer::PrimitiveTopology" to map
		*
		*  @return
		*    Vulkan type
		*/
		static VkPrimitiveTopology getVulkanType(Renderer::PrimitiveTopology primitiveTopology)
		{
			// Tessellation support: Up to 32 vertices per patch are supported "Renderer::PrimitiveTopology::PATCH_LIST_1" ... "Renderer::PrimitiveTopology::PATCH_LIST_32"
			if (primitiveTopology >= Renderer::PrimitiveTopology::PATCH_LIST_1)
			{
				// Use tessellation
				return VK_PRIMITIVE_TOPOLOGY_PATCH_LIST;
			}
			else
			{
				static constexpr VkPrimitiveTopology MAPPING[] =
				{
					VK_PRIMITIVE_TOPOLOGY_POINT_LIST,		// Renderer::PrimitiveTopology::POINT_LIST
					VK_PRIMITIVE_TOPOLOGY_LINE_LIST,		// Renderer::PrimitiveTopology::LINE_LIST
					VK_PRIMITIVE_TOPOLOGY_LINE_STRIP,		// Renderer::PrimitiveTopology::LINE_STRIP
					VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,	// Renderer::PrimitiveTopology::TRIANGLE_LIST
					VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP	// Renderer::PrimitiveTopology::TRIANGLE_STRIP
				};
				return MAPPING[static_cast<int>(primitiveTopology) - 1];	// Lookout! The "Renderer::PrimitiveTopology"-values start with 1, not 0
			}
		}

		//[-------------------------------------------------------]
		//[ Renderer::TextureFormat                               ]
		//[-------------------------------------------------------]
		/**
		*  @brief
		*    "Renderer::TextureFormat" to Vulkan format
		*
		*  @param[in] textureFormat
		*    "Renderer::TextureFormat" to map
		*
		*  @return
		*    Vulkan format
		*/
		static VkFormat getVulkanFormat(Renderer::TextureFormat::Enum textureFormat)
		{
			static constexpr VkFormat MAPPING[] =
			{
				VK_FORMAT_R8_UNORM,					// Renderer::TextureFormat::R8            - 8-bit pixel format, all bits red
				VK_FORMAT_R8G8B8_UNORM,				// Renderer::TextureFormat::R8G8B8        - 24-bit pixel format, 8 bits for red, green and blue
				VK_FORMAT_R8G8B8A8_UNORM,			// Renderer::TextureFormat::R8G8B8A8      - 32-bit pixel format, 8 bits for red, green, blue and alpha
				VK_FORMAT_R8G8B8A8_SRGB,			// Renderer::TextureFormat::R8G8B8A8_SRGB - 32-bit pixel format, 8 bits for red, green, blue and alpha; sRGB = RGB hardware gamma correction, the alpha channel always remains linear
				VK_FORMAT_B8G8R8A8_UNORM,			// Renderer::TextureFormat::B8G8R8A8      - 32-bit pixel format, 8 bits for red, green, blue and alpha
				VK_FORMAT_B10G11R11_UFLOAT_PACK32,	// Renderer::TextureFormat::R11G11B10F    - 32-bit float format using 11 bits the red and green channel, 10 bits the blue channel; red and green channels have a 6 bits mantissa and a 5 bits exponent and blue has a 5 bits mantissa and 5 bits exponent
				VK_FORMAT_R16G16B16A16_SFLOAT,		// Renderer::TextureFormat::R16G16B16A16F - 64-bit float format using 16 bits for the each channel (red, green, blue, alpha)
				VK_FORMAT_R32G32B32A32_SFLOAT,		// Renderer::TextureFormat::R32G32B32A32F - 128-bit float format using 32 bits for the each channel (red, green, blue, alpha)
				VK_FORMAT_BC1_RGB_UNORM_BLOCK,		// Renderer::TextureFormat::BC1           - DXT1 compression (known as BC1 in DirectX 10, RGB compression: 8:1, 8 bytes per block)
				VK_FORMAT_BC1_RGB_SRGB_BLOCK,		// Renderer::TextureFormat::BC1_SRGB      - DXT1 compression (known as BC1 in DirectX 10, RGB compression: 8:1, 8 bytes per block); sRGB = RGB hardware gamma correction, the alpha channel always remains linear
				VK_FORMAT_BC2_UNORM_BLOCK,			// Renderer::TextureFormat::BC2           - DXT3 compression (known as BC2 in DirectX 10, RGBA compression: 4:1, 16 bytes per block)
				VK_FORMAT_BC2_SRGB_BLOCK,			// Renderer::TextureFormat::BC2_SRGB      - DXT3 compression (known as BC2 in DirectX 10, RGBA compression: 4:1, 16 bytes per block); sRGB = RGB hardware gamma correction, the alpha channel always remains linear
				VK_FORMAT_BC3_UNORM_BLOCK,			// Renderer::TextureFormat::BC3           - DXT5 compression (known as BC3 in DirectX 10, RGBA compression: 4:1, 16 bytes per block)
				VK_FORMAT_BC3_SRGB_BLOCK,			// Renderer::TextureFormat::BC3_SRGB      - DXT5 compression (known as BC3 in DirectX 10, RGBA compression: 4:1, 16 bytes per block); sRGB = RGB hardware gamma correction, the alpha channel always remains linear
				VK_FORMAT_BC4_UNORM_BLOCK,			// Renderer::TextureFormat::BC4           - 1 component texture compression (also known as 3DC+/ATI1N, known as BC4 in DirectX 10, 8 bytes per block)
				VK_FORMAT_BC5_UNORM_BLOCK,			// Renderer::TextureFormat::BC5           - 2 component texture compression (luminance & alpha compression 4:1 -> normal map compression, also known as 3DC/ATI2N, known as BC5 in DirectX 10, 16 bytes per block)
				VK_FORMAT_UNDEFINED,				// Renderer::TextureFormat::ETC1          - 3 component texture compression meant for mobile devices - not supported in Direct3D 11 - TODO(co) Check for Vulkan format
				VK_FORMAT_R16_UNORM,				// Renderer::TextureFormat::R16_UNORM     - 16-bit unsigned-normalized-integer format that supports 16 bits for the red channel
				VK_FORMAT_R32_UINT,					// Renderer::TextureFormat::R32_UINT      - 32-bit unsigned integer format
				VK_FORMAT_R32_SFLOAT,				// Renderer::TextureFormat::R32_FLOAT     - 32-bit float format
				VK_FORMAT_D32_SFLOAT,				// Renderer::TextureFormat::D32_FLOAT     - 32-bit float depth format
				VK_FORMAT_R16G16_UNORM,				// Renderer::TextureFormat::R16G16_SNORM  - A two-component, 32-bit signed-normalized-integer format that supports 16 bits for the red channel and 16 bits for the green channel
				VK_FORMAT_R16G16_SFLOAT,			// Renderer::TextureFormat::R16G16_FLOAT  - A two-component, 32-bit floating-point format that supports 16 bits for the red channel and 16 bits for the green channel
				VK_FORMAT_UNDEFINED					// Renderer::TextureFormat::UNKNOWN       - Unknown
			};
			return MAPPING[textureFormat];
		}

		/**
		*  @brief
		*    Number of multisamples to Vulkan sample count flag bits
		*
		*  @param[in] context
		*    Renderer context to use
		*  @param[in] numberOfMultisamples
		*    The number of multisamples per pixel (valid values: 1, 2, 4, 8)
		*
		*  @return
		*    Vulkan sample count flag bits
		*/
		static VkSampleCountFlagBits getVulkanSampleCountFlagBits(MAYBE_UNUSED const Renderer::Context& context, uint8_t numberOfMultisamples)
		{
			RENDERER_ASSERT(context, numberOfMultisamples <= 8, "Invalid number of Vulkan multisamples")
			static constexpr VkSampleCountFlagBits MAPPING[] =
			{
				VK_SAMPLE_COUNT_1_BIT,
				VK_SAMPLE_COUNT_2_BIT,
				VK_SAMPLE_COUNT_4_BIT,
				VK_SAMPLE_COUNT_8_BIT
			};
			return MAPPING[numberOfMultisamples - 1];	// Lookout! The "numberOfMultisamples"-values start with 1, not 0
		}


	};




	//[-------------------------------------------------------]
	//[ VulkanRenderer/Helper.h                               ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Vulkan helper
	*/
	class Helper final
	{


	//[-------------------------------------------------------]
	//[ Public static methods                                 ]
	//[-------------------------------------------------------]
	public:
		//[-------------------------------------------------------]
		//[ Command                                               ]
		//[-------------------------------------------------------]
		static VkCommandBuffer beginSingleTimeCommands(const VulkanRenderer& vulkanRenderer)
		{
			// Create and begin Vulkan command buffer
			VkCommandBuffer vkCommandBuffer = vulkanRenderer.getVulkanContext().createVkCommandBuffer();
			static constexpr VkCommandBufferBeginInfo vkCommandBufferBeginInfo =
			{
				VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,	// sType (VkStructureType)
				nullptr,										// pNext (const void*)
				0,												// flags (VkCommandBufferUsageFlags)
				nullptr											// pInheritanceInfo (const VkCommandBufferInheritanceInfo*)
			};
			if (vkBeginCommandBuffer(vkCommandBuffer, &vkCommandBufferBeginInfo) == VK_SUCCESS)
			{
				// Done
				return vkCommandBuffer;
			}
			else
			{
				// Error!
				RENDERER_LOG(vulkanRenderer.getContext(), CRITICAL, "Failed to begin Vulkan command buffer instance")
				return VK_NULL_HANDLE;
			}
		}

		static void endSingleTimeCommands(const VulkanRenderer& vulkanRenderer, VkCommandBuffer vkCommandBuffer)
		{
			const VulkanContext& vulkanContext = vulkanRenderer.getVulkanContext();
			const VkQueue vkQueue = vulkanContext.getGraphicsVkQueue();

			// End Vulkan command buffer
			vkEndCommandBuffer(vkCommandBuffer);

			// Submit Vulkan command buffer
			const VkSubmitInfo vkSubmitInfo =
			{
				VK_STRUCTURE_TYPE_SUBMIT_INFO,	// sType (VkStructureType)
				nullptr,						// pNext (const void*)
				0,								// waitSemaphoreCount (uint32_t)
				nullptr,						// pWaitSemaphores (const VkSemaphore*)
				nullptr,						// pWaitDstStageMask (const VkPipelineStageFlags*)
				1,								// commandBufferCount (uint32_t)
				&vkCommandBuffer,				// pCommandBuffers (const VkCommandBuffer*)
				0,								// signalSemaphoreCount (uint32_t)
				nullptr							// pSignalSemaphores (const VkSemaphore*)
			};
			if (vkQueueSubmit(vkQueue, 1, &vkSubmitInfo, VK_NULL_HANDLE) != VK_SUCCESS)
			{
				// Error!
				RENDERER_LOG(vulkanRenderer.getContext(), CRITICAL, "Vulkan queue submit failed")
				return;
			}
			if (vkQueueWaitIdle(vkQueue) != VK_SUCCESS)
			{
				// Error!
				RENDERER_LOG(vulkanRenderer.getContext(), CRITICAL, "Vulkan Queue wait idle failed")
				return;
			}

			// Destroy Vulkan command buffer
			vulkanContext.destroyVkCommandBuffer(vkCommandBuffer);
		}

		//[-------------------------------------------------------]
		//[ Transition                                            ]
		//[-------------------------------------------------------]
		static void transitionVkImageLayout(const VulkanRenderer& vulkanRenderer, VkImage vkImage, VkImageAspectFlags vkImageAspectFlags, VkImageLayout oldVkImageLayout, VkImageLayout newVkImageLayout)
		{
			// Create and begin Vulkan command buffer
			VkCommandBuffer vkCommandBuffer = beginSingleTimeCommands(vulkanRenderer);

			// Vulkan image memory barrier
			transitionVkImageLayout(vulkanRenderer, vkCommandBuffer, vkImage, vkImageAspectFlags, 1, 1, oldVkImageLayout, newVkImageLayout);

			// End and destroy Vulkan command buffer
			endSingleTimeCommands(vulkanRenderer, vkCommandBuffer);
		}

		static void transitionVkImageLayout(const VulkanRenderer& vulkanRenderer, VkCommandBuffer vkCommandBuffer, VkImage vkImage, VkImageAspectFlags vkImageAspectFlags, uint32_t levelCount, uint32_t layerCount, VkImageLayout oldVkImageLayout, VkImageLayout newVkImageLayout)
		{
			VkImageMemoryBarrier vkImageMemoryBarrier =
			{
				VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,	// sType (VkStructureType)
				nullptr,								// pNext (const void*)
				0,										// srcAccessMask (VkAccessFlags)
				0,										// dstAccessMask (VkAccessFlags)
				oldVkImageLayout,						// oldLayout (VkImageLayout)
				newVkImageLayout,						// newLayout (VkImageLayout)
				VK_QUEUE_FAMILY_IGNORED,				// srcQueueFamilyIndex (uint32_t)
				VK_QUEUE_FAMILY_IGNORED,				// dstQueueFamilyIndex (uint32_t)
				vkImage,								// image (VkImage)
				{ // subresourceRange (VkImageSubresourceRange)
					vkImageAspectFlags,	// aspectMask (VkImageAspectFlags)
					0,					// baseMipLevel (uint32_t)
					levelCount,			// levelCount (uint32_t)
					0,					// baseArrayLayer (uint32_t)
					layerCount			// layerCount (uint32_t)
				}
			};

			// "srcAccessMask" and "dstAccessMask" configuration
			if (VK_IMAGE_LAYOUT_PREINITIALIZED == oldVkImageLayout && VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL == newVkImageLayout)
			{
				vkImageMemoryBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
				vkImageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			}
			else if (VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL == oldVkImageLayout && VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL == newVkImageLayout)
			{
				vkImageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
				vkImageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
			}
			else if (VK_IMAGE_LAYOUT_UNDEFINED == oldVkImageLayout && VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL == newVkImageLayout)
			{
				vkImageMemoryBarrier.srcAccessMask = 0;
				vkImageMemoryBarrier.dstAccessMask = (VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT);
			}
			else
			{
				RENDERER_LOG(vulkanRenderer.getContext(), CRITICAL, "Unsupported Vulkan image layout transition")
			}

			// Create Vulkan pipeline barrier command
			vkCmdPipelineBarrier(vkCommandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, 0, 0, nullptr, 0, nullptr, 1, &vkImageMemoryBarrier);
		}

		static void transitionVkImageLayout(const VulkanRenderer& vulkanRenderer, VkCommandBuffer vkCommandBuffer, VkImage vkImage, VkImageLayout oldVkImageLayout, VkImageLayout newVkImageLayout, VkImageSubresourceRange vkImageSubresourceRange, VkPipelineStageFlags sourceVkPipelineStageFlags, VkPipelineStageFlags destinationVkPipelineStageFlags)
		{
			// Basing on https://github.com/SaschaWillems/Vulkan/tree/master

			// Create an image barrier object
			VkImageMemoryBarrier vkImageMemoryBarrier =
			{
				VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,	// sType (VkStructureType)
				nullptr,								// pNext (const void*)
				0,										// srcAccessMask (VkAccessFlags)
				0,										// dstAccessMask (VkAccessFlags)
				oldVkImageLayout,						// oldLayout (VkImageLayout)
				newVkImageLayout,						// newLayout (VkImageLayout)
				VK_QUEUE_FAMILY_IGNORED,				// srcQueueFamilyIndex (uint32_t)
				VK_QUEUE_FAMILY_IGNORED,				// dstQueueFamilyIndex (uint32_t)
				vkImage,								// image (VkImage)
				vkImageSubresourceRange					// subresourceRange (VkImageSubresourceRange)
			};

			// Source layouts (old)
			// -> Source access mask controls actions that have to be finished on the old layout before it will be transitioned to the new layout
			switch (oldVkImageLayout)
			{
				case VK_IMAGE_LAYOUT_UNDEFINED:
					// Image layout is undefined (or does not matter)
					// Only valid as initial layout
					// No flags required, listed only for completeness
					vkImageMemoryBarrier.srcAccessMask = 0;
					break;

				case VK_IMAGE_LAYOUT_PREINITIALIZED:
					// Image is preinitialized
					// Only valid as initial layout for linear images, preserves memory contents
					// Make sure host writes have been finished
					vkImageMemoryBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
					break;

				case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
					// Image is a color attachment
					// Make sure any writes to the color buffer have been finished
					vkImageMemoryBarrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
					break;

				case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
					// Image is a depth/stencil attachment
					// Make sure any writes to the depth/stencil buffer have been finished
					vkImageMemoryBarrier.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
					break;

				case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
					// Image is a transfer source 
					// Make sure any reads from the image have been finished
					vkImageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
					break;

				case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
					// Image is a transfer destination
					// Make sure any writes to the image have been finished
					vkImageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
					break;

				case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
					// Image is read by a shader
					// Make sure any shader reads from the image have been finished
					vkImageMemoryBarrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
					break;

				case VK_IMAGE_LAYOUT_GENERAL:
				case VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL:
				case VK_IMAGE_LAYOUT_PRESENT_SRC_KHR:
				case VK_IMAGE_LAYOUT_SHARED_PRESENT_KHR:
				case VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL_KHR:
				case VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL_KHR:
				// case VK_IMAGE_LAYOUT_BEGIN_RANGE:	not possible
				// case VK_IMAGE_LAYOUT_END_RANGE:		not possible
				case VK_IMAGE_LAYOUT_RANGE_SIZE:
				case VK_IMAGE_LAYOUT_MAX_ENUM:
				default:
					// Other source layouts aren't handled (yet)
					RENDERER_LOG(vulkanRenderer.getContext(), CRITICAL, "Unsupported Vulkan image old layout transition")
					break;
			}

			// Target layouts (new)
			// -> Destination access mask controls the dependency for the new image layout
			switch (newVkImageLayout)
			{
				case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
					// Image will be used as a transfer destination
					// Make sure any writes to the image have been finished
					vkImageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
					break;

				case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
					// Image will be used as a transfer source
					// Make sure any reads from the image have been finished
					vkImageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
					break;

				case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
					// Image will be used as a color attachment
					// Make sure any writes to the color buffer have been finished
					vkImageMemoryBarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
					break;

				case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
					// Image layout will be used as a depth/stencil attachment
					// Make sure any writes to depth/stencil buffer have been finished
					vkImageMemoryBarrier.dstAccessMask |= VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
					break;

				case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
					// Image will be read in a shader (sampler, input attachment)
					// Make sure any writes to the image have been finished
					if (vkImageMemoryBarrier.srcAccessMask == 0)
					{
						vkImageMemoryBarrier.srcAccessMask = (VK_ACCESS_HOST_WRITE_BIT | VK_ACCESS_TRANSFER_WRITE_BIT);
					}
					vkImageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
					break;

				case VK_IMAGE_LAYOUT_UNDEFINED:
				case VK_IMAGE_LAYOUT_GENERAL:
				case VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL:
				case VK_IMAGE_LAYOUT_PREINITIALIZED:
				case VK_IMAGE_LAYOUT_PRESENT_SRC_KHR:
				case VK_IMAGE_LAYOUT_SHARED_PRESENT_KHR:
				case VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL_KHR:
				case VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL_KHR:
				// case VK_IMAGE_LAYOUT_BEGIN_RANGE:	not possible
				// case VK_IMAGE_LAYOUT_END_RANGE:		not possible
				case VK_IMAGE_LAYOUT_RANGE_SIZE:
				case VK_IMAGE_LAYOUT_MAX_ENUM:
				default:
					// Other source layouts aren't handled (yet)
					RENDERER_LOG(vulkanRenderer.getContext(), CRITICAL, "Unsupported Vulkan image new layout transition")
					break;
			}

			// Put barrier inside setup command buffer
			vkCmdPipelineBarrier(vkCommandBuffer, sourceVkPipelineStageFlags, destinationVkPipelineStageFlags, 0, 0, nullptr, 0, nullptr, 1, &vkImageMemoryBarrier);
		}

		//[-------------------------------------------------------]
		//[ Buffer                                                ]
		//[-------------------------------------------------------]
		// TODO(co) Trivial implementation to have something to start with. Need to use more clever memory management and stating buffers later on.
		static void createAndAllocateVkBuffer(const VulkanRenderer& vulkanRenderer, VkBufferUsageFlagBits vkBufferUsageFlagBits, VkMemoryPropertyFlags vkMemoryPropertyFlags, VkDeviceSize numberOfBytes, const void* data, VkBuffer& vkBuffer, VkDeviceMemory& vkDeviceMemory)
		{
			const VulkanContext& vulkanContext = vulkanRenderer.getVulkanContext();
			const VkDevice vkDevice = vulkanContext.getVkDevice();

			// Create the Vulkan buffer
			const VkBufferCreateInfo vkBufferCreateInfo =
			{
				VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,					// sType (VkStructureType)
				nullptr,												// pNext (const void*)
				0,														// flags (VkBufferCreateFlags)
				numberOfBytes,											// size (VkDeviceSize)
				static_cast<VkBufferUsageFlags>(vkBufferUsageFlagBits),	// usage (VkBufferUsageFlags)
				VK_SHARING_MODE_EXCLUSIVE,								// sharingMode (VkSharingMode)
				0,														// queueFamilyIndexCount (uint32_t)
				nullptr													// pQueueFamilyIndices (const uint32_t*)
			};
			if (vkCreateBuffer(vkDevice, &vkBufferCreateInfo, vulkanRenderer.getVkAllocationCallbacks(), &vkBuffer) != VK_SUCCESS)
			{
				RENDERER_LOG(vulkanRenderer.getContext(), CRITICAL, "Failed to create the Vulkan buffer")
			}

			// Allocate memory for the Vulkan buffer
			VkMemoryRequirements vkMemoryRequirements = {};
			vkGetBufferMemoryRequirements(vkDevice, vkBuffer, &vkMemoryRequirements);
			const VkMemoryAllocateInfo vkMemoryAllocateInfo =
			{
				VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,															// sType (VkStructureType)
				nullptr,																						// pNext (const void*)
				vkMemoryRequirements.size,																		// allocationSize (VkDeviceSize)
				vulkanContext.findMemoryTypeIndex(vkMemoryRequirements.memoryTypeBits, vkMemoryPropertyFlags)	// memoryTypeIndex (uint32_t)
			};
			if (vkAllocateMemory(vkDevice, &vkMemoryAllocateInfo, vulkanRenderer.getVkAllocationCallbacks(), &vkDeviceMemory) != VK_SUCCESS)
			{
				RENDERER_LOG(vulkanRenderer.getContext(), CRITICAL, "Failed to allocate the Vulkan buffer memory")
			}

			// Bind and fill memory
			vkBindBufferMemory(vkDevice, vkBuffer, vkDeviceMemory, 0);
			if (nullptr != data)
			{
				void* mappedData = nullptr;
				if (vkMapMemory(vkDevice, vkDeviceMemory, 0, vkBufferCreateInfo.size, 0, &mappedData) == VK_SUCCESS)
				{
					memcpy(mappedData, data, static_cast<size_t>(vkBufferCreateInfo.size));
					vkUnmapMemory(vkDevice, vkDeviceMemory);
				}
				else
				{
					RENDERER_LOG(vulkanRenderer.getContext(), CRITICAL, "Failed to map the Vulkan memory")
				}
			}
		}

		static void destroyAndFreeVkBuffer(const VulkanRenderer& vulkanRenderer, VkBuffer& vkBuffer, VkDeviceMemory& vkDeviceMemory)
		{
			if (VK_NULL_HANDLE != vkBuffer)
			{
				const VkDevice vkDevice = vulkanRenderer.getVulkanContext().getVkDevice();
				vkDestroyBuffer(vkDevice, vkBuffer, vulkanRenderer.getVkAllocationCallbacks());
				if (VK_NULL_HANDLE != vkDeviceMemory)
				{
					vkFreeMemory(vkDevice, vkDeviceMemory, vulkanRenderer.getVkAllocationCallbacks());
				}
			}
		}

		//[-------------------------------------------------------]
		//[ Image                                                 ]
		//[-------------------------------------------------------]
		static VkImageLayout getVkImageLayoutByTextureFlags(uint32_t textureFlags)
		{
			if (textureFlags & Renderer::TextureFlag::RENDER_TARGET)
			{
				return VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			}
			else if (textureFlags & Renderer::TextureFlag::UNORDERED_ACCESS)
			{
				return VK_IMAGE_LAYOUT_GENERAL;
			}
			return VK_IMAGE_LAYOUT_PREINITIALIZED;
		}

		// TODO(co) Trivial implementation to have something to start with. Need to use more clever memory management and stating buffers later on.
		static VkFormat createAndFillVkImage(const VulkanRenderer& vulkanRenderer, VkImageType vkImageType, VkImageViewType vkImageViewType, const VkExtent3D& vkExtent3D, Renderer::TextureFormat::Enum textureFormat, const void* data, uint32_t textureFlags, uint8_t numberOfMultisamples, VkImage& vkImage, VkDeviceMemory& vkDeviceMemory, VkImageView& vkImageView)
		{
			// Calculate the number of mipmaps
			const bool dataContainsMipmaps = (textureFlags & Renderer::TextureFlag::DATA_CONTAINS_MIPMAPS);
			const bool generateMipmaps = (!dataContainsMipmaps && (textureFlags & Renderer::TextureFlag::GENERATE_MIPMAPS));
			uint32_t numberOfMipmaps = (dataContainsMipmaps || generateMipmaps) ? Renderer::ITexture::getNumberOfMipmaps(vkExtent3D.width, vkExtent3D.height) : 1;

			// Get Vulkan image usage flags
			RENDERER_ASSERT(vulkanRenderer.getContext(), (textureFlags & Renderer::TextureFlag::RENDER_TARGET) == 0 || nullptr == data, "Vulkan render target textures can't be filled using provided data")
			const bool isDepthTextureFormat = Renderer::TextureFormat::isDepth(textureFormat);
			VkImageUsageFlags vkImageUsageFlags = VK_IMAGE_USAGE_TRANSFER_DST_BIT;
			if (textureFlags & Renderer::TextureFlag::SHADER_RESOURCE)
			{
				vkImageUsageFlags |= VK_IMAGE_USAGE_SAMPLED_BIT;
			}
			if (textureFlags & Renderer::TextureFlag::UNORDERED_ACCESS)
			{
				vkImageUsageFlags |= VK_IMAGE_USAGE_STORAGE_BIT;
			}
			if (textureFlags & Renderer::TextureFlag::RENDER_TARGET)
			{
				if (isDepthTextureFormat)
				{
					vkImageUsageFlags |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
				}
				else
				{
					vkImageUsageFlags |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
				}
			}
			if (generateMipmaps)
			{
				vkImageUsageFlags |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
			}

			// Get Vulkan format
			const VkFormat vkFormat   = Mapping::getVulkanFormat(textureFormat);
			const bool     layered    = (VK_IMAGE_VIEW_TYPE_2D_ARRAY == vkImageViewType || VK_IMAGE_VIEW_TYPE_CUBE == vkImageViewType);
			const uint32_t layerCount = layered ? vkExtent3D.depth : 1;
			const uint32_t depth	  = layered ? 1 : vkExtent3D.depth;
			const VkSampleCountFlagBits vkSampleCountFlagBits = Mapping::getVulkanSampleCountFlagBits(vulkanRenderer.getContext(), numberOfMultisamples);
			VkImageAspectFlags vkImageAspectFlags = isDepthTextureFormat ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;
			if (::detail::hasVkFormatStencilComponent(vkFormat))
			{
				vkImageAspectFlags |= VK_IMAGE_ASPECT_STENCIL_BIT;
			}

			// Calculate the number of bytes
			uint32_t numberOfBytes = 0;
			if (dataContainsMipmaps)
			{
				uint32_t currentWidth  = vkExtent3D.width;
				uint32_t currentHeight = vkExtent3D.height;
				uint32_t currentDepth  = depth;
				for (uint32_t mipmap = 0; mipmap < numberOfMipmaps; ++mipmap)
				{
					numberOfBytes += Renderer::TextureFormat::getNumberOfBytesPerSlice(static_cast<Renderer::TextureFormat::Enum>(textureFormat), currentWidth, currentHeight) * currentDepth;
					currentWidth = Renderer::ITexture::getHalfSize(currentWidth);
					currentHeight = Renderer::ITexture::getHalfSize(currentHeight);
					currentDepth = Renderer::ITexture::getHalfSize(currentDepth);
				}
				numberOfBytes *= vkExtent3D.depth;
			}
			else
			{
				numberOfBytes = Renderer::TextureFormat::getNumberOfBytesPerSlice(textureFormat, vkExtent3D.width, vkExtent3D.height) * vkExtent3D.depth;
			}

			{ // Create and fill Vulkan image
				const VkImageCreateFlags vkImageCreateFlags = (VK_IMAGE_VIEW_TYPE_CUBE == vkImageViewType) ? VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT : 0u;
				createAndAllocateVkImage(vulkanRenderer, vkImageCreateFlags, vkImageType, VkExtent3D{vkExtent3D.width, vkExtent3D.height, depth}, numberOfMipmaps, layerCount, vkFormat, vkSampleCountFlagBits, VK_IMAGE_TILING_OPTIMAL, vkImageUsageFlags, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vkImage, vkDeviceMemory);
			}

			// Create the Vulkan image view
			if ((textureFlags & Renderer::TextureFlag::SHADER_RESOURCE) != 0 || (textureFlags & Renderer::TextureFlag::RENDER_TARGET) != 0 || (textureFlags & Renderer::TextureFlag::UNORDERED_ACCESS) != 0)
			{
				createVkImageView(vulkanRenderer, vkImage, vkImageViewType, numberOfMipmaps, layerCount, vkFormat, vkImageAspectFlags, vkImageView);
			}

			// Upload all mipmaps
			if (nullptr != data)
			{
				// Create Vulkan staging buffer
				VkBuffer stagingVkBuffer = VK_NULL_HANDLE;
				VkDeviceMemory stagingVkDeviceMemory = VK_NULL_HANDLE;
				createAndAllocateVkBuffer(vulkanRenderer, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, numberOfBytes, data, stagingVkBuffer, stagingVkDeviceMemory);

				{ // Upload all mipmaps
					// Create and begin Vulkan command buffer
					VkCommandBuffer vkCommandBuffer = beginSingleTimeCommands(vulkanRenderer);
					transitionVkImageLayout(vulkanRenderer, vkCommandBuffer, vkImage, vkImageAspectFlags, numberOfMipmaps, layerCount, VK_IMAGE_LAYOUT_PREINITIALIZED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

					// Upload all mipmaps
					uint32_t bufferOffset  = 0;
					uint32_t currentWidth  = vkExtent3D.width;
					uint32_t currentHeight = vkExtent3D.height;
					uint32_t currentDepth  = depth;

					// Allocate list of VkBufferImageCopy and setup VkBufferImageCopy data for each mipmap level
					const uint32_t numberOfUploadedMipmaps = generateMipmaps ? 1 : numberOfMipmaps;
					std::vector<VkBufferImageCopy> vkBufferImageCopyList;
					vkBufferImageCopyList.reserve(numberOfUploadedMipmaps);
					for (uint32_t mipmap = 0; mipmap < numberOfUploadedMipmaps; ++mipmap)
					{
						vkBufferImageCopyList.push_back({
							bufferOffset,									// bufferOffset (VkDeviceSize)
							0,												// bufferRowLength (uint32_t)
							0,												// bufferImageHeight (uint32_t)
							{ // imageSubresource (VkImageSubresourceLayers)
								vkImageAspectFlags,							// aspectMask (VkImageAspectFlags)
								mipmap,										// mipLevel (uint32_t)
								0,											// baseArrayLayer (uint32_t)
								layerCount									// layerCount (uint32_t)
							},
							{ 0, 0, 0 },									// imageOffset (VkOffset3D)
							{ currentWidth, currentHeight, currentDepth }	// imageExtent (VkExtent3D)
						});

						// Move on to the next mipmap
						bufferOffset += Renderer::TextureFormat::getNumberOfBytesPerSlice(static_cast<Renderer::TextureFormat::Enum>(textureFormat), currentWidth, currentHeight) * currentDepth;
						currentWidth = Renderer::ITexture::getHalfSize(currentWidth);
						currentHeight = Renderer::ITexture::getHalfSize(currentHeight);
						currentDepth = Renderer::ITexture::getHalfSize(currentDepth);
					}

					// Copy Vulkan buffer to Vulkan image
					vkCmdCopyBufferToImage(vkCommandBuffer, stagingVkBuffer, vkImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, static_cast<uint32_t>(vkBufferImageCopyList.size()), vkBufferImageCopyList.data());

					// End and destroy Vulkan command buffer
					transitionVkImageLayout(vulkanRenderer, vkCommandBuffer, vkImage, vkImageAspectFlags, numberOfMipmaps, layerCount, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
					endSingleTimeCommands(vulkanRenderer, vkCommandBuffer);
				}

				// Destroy Vulkan staging buffer
				destroyAndFreeVkBuffer(vulkanRenderer, stagingVkBuffer, stagingVkDeviceMemory);

				// Generate a complete texture mip-chain at runtime from a base image using image blits and proper image barriers
				// -> Basing on https://github.com/SaschaWillems/Vulkan/tree/master/texturemipmapgen
				// -> We copy down the whole mip chain doing a blit from mip-1 to mip. An alternative way would be to always blit from the first mip level and sample that one down.
				// TODO(co) Some GPUs also offer "asynchronous transfer queues" (check for queue families with only the "VK_QUEUE_TRANSFER_BIT" set) that may be used to speed up such operations
				if (generateMipmaps)
				{
					#ifdef RENDERER_DEBUG
					{
						// Get device properties for the requested Vulkan texture format
						VkFormatProperties vkFormatProperties;
						vkGetPhysicalDeviceFormatProperties(vulkanRenderer.getVulkanContext().getVkPhysicalDevice(), vkFormat, &vkFormatProperties);

						// Mip-chain generation requires support for blit source and destination
						RENDERER_ASSERT(vulkanRenderer.getContext(), vkFormatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_BLIT_SRC_BIT, "Invalid Vulkan optimal tiling features")
						RENDERER_ASSERT(vulkanRenderer.getContext(), vkFormatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_BLIT_DST_BIT, "Invalid Vulkan optimal tiling features")
					}
					#endif

					// Create and begin Vulkan command buffer
					VkCommandBuffer vkCommandBuffer = beginSingleTimeCommands(vulkanRenderer);

					// Copy down mips from n-1 to n
					for (uint32_t i = 1; i < numberOfMipmaps; ++i)
					{
						const VkImageBlit VkImageBlit =
						{
							{ // srcSubresource (VkImageSubresourceLayers)
								vkImageAspectFlags,	// aspectMask (VkImageAspectFlags)
								i - 1,				// mipLevel (uint32_t)
								0,					// baseArrayLayer (uint32_t)
								layerCount			// layerCount (uint32_t)
							},
							{ // srcOffsets[2] (VkOffset3D)
								{ 0, 0, 0 },
								{ std::max(int32_t(vkExtent3D.width >> (i - 1)), 1), std::max(int32_t(vkExtent3D.height >> (i - 1)), 1), 1 }
							},
							{ // dstSubresource (VkImageSubresourceLayers)
								vkImageAspectFlags,	// aspectMask (VkImageAspectFlags)
								i,					// mipLevel (uint32_t)
								0,					// baseArrayLayer (uint32_t)
								layerCount			// layerCount (uint32_t)
							},
							{ // dstOffsets[2] (VkOffset3D)
								{ 0, 0, 0 },
								{ std::max(int32_t(vkExtent3D.width >> i), 1), std::max(int32_t(vkExtent3D.height >> i), 1), 1 }
							}
						};
						const VkImageSubresourceRange vkImageSubresourceRange =
						{
							vkImageAspectFlags,	// aspectMask (VkImageAspectFlags)
							i,					// baseMipLevel (uint32_t)
							1,					// levelCount (uint32_t)
							0,					// baseArrayLayer (uint32_t)
							layerCount			// layerCount (uint32_t)
						};

						// Transition current mip level to transfer destination
						transitionVkImageLayout(vulkanRenderer, vkCommandBuffer, vkImage, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, vkImageSubresourceRange, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_HOST_BIT);

						// Blit from previous level
						vkCmdBlitImage(vkCommandBuffer, vkImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, vkImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &VkImageBlit, VK_FILTER_LINEAR);

						// Transition current mip level to transfer source for read in next iteration
						transitionVkImageLayout(vulkanRenderer, vkCommandBuffer, vkImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, vkImageSubresourceRange, VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);
					}

					{ // After the loop, all mip layers are in "VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL"-layout, so transition all to "VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL"-layout
						const VkImageSubresourceRange vkImageSubresourceRange =
						{
							vkImageAspectFlags,		// aspectMask (VkImageAspectFlags)
							1,						// baseMipLevel (uint32_t)
							numberOfMipmaps - 1,	// levelCount (uint32_t)
							0,						// baseArrayLayer (uint32_t)
							layerCount				// layerCount (uint32_t)
						};
						transitionVkImageLayout(vulkanRenderer, vkCommandBuffer, vkImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, vkImageSubresourceRange, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);
					}

					// End and destroy Vulkan command buffer
					endSingleTimeCommands(vulkanRenderer, vkCommandBuffer);
				}
			}

			// Done
			return vkFormat;
		}

		static void createAndAllocateVkImage(const VulkanRenderer& vulkanRenderer, VkImageCreateFlags vkImageCreateFlags, VkImageType vkImageType, const VkExtent3D& vkExtent3D, uint32_t mipLevels, uint32_t arrayLayers, VkFormat vkFormat, VkSampleCountFlagBits vkSampleCountFlagBits, VkImageTiling vkImageTiling, VkImageUsageFlags vkImageUsageFlags, VkMemoryPropertyFlags vkMemoryPropertyFlags, VkImage& vkImage, VkDeviceMemory& vkDeviceMemory)
		{
			const VulkanContext& vulkanContext = vulkanRenderer.getVulkanContext();
			const VkDevice vkDevice = vulkanContext.getVkDevice();

			{ // Create Vulkan image
				const VkImageCreateInfo vkImageCreateInfo =
				{
					VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,	// sType (VkStructureType)
					nullptr,								// pNext (const void*)
					vkImageCreateFlags,						// flags (VkImageCreateFlags)
					vkImageType,							// imageType (VkImageType)
					vkFormat,								// format (VkFormat)
					vkExtent3D,								// extent (VkExtent3D)
					mipLevels,								// mipLevels (uint32_t)
					arrayLayers,							// arrayLayers (uint32_t)
					vkSampleCountFlagBits,					// samples (VkSampleCountFlagBits)
					vkImageTiling,							// tiling (VkImageTiling)
					vkImageUsageFlags,						// usage (VkImageUsageFlags)
					VK_SHARING_MODE_EXCLUSIVE,				// sharingMode (VkSharingMode)
					0,										// queueFamilyIndexCount (uint32_t)
					nullptr,								// pQueueFamilyIndices (const uint32_t*)
					VK_IMAGE_LAYOUT_PREINITIALIZED			// initialLayout (VkImageLayout)
				};
				if (vkCreateImage(vkDevice, &vkImageCreateInfo, vulkanRenderer.getVkAllocationCallbacks(), &vkImage) != VK_SUCCESS)
				{
					RENDERER_LOG(vulkanRenderer.getContext(), CRITICAL, "Failed to create the Vulkan image")
				}
			}

			{ // Allocate Vulkan memory
				VkMemoryRequirements vkMemoryRequirements = {};
				vkGetImageMemoryRequirements(vkDevice, vkImage, &vkMemoryRequirements);
				const VkMemoryAllocateInfo vkMemoryAllocateInfo =
				{
					VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,															// sType (VkStructureType)
					nullptr,																						// pNext (const void*)
					vkMemoryRequirements.size,																		// allocationSize (VkDeviceSize)
					vulkanContext.findMemoryTypeIndex(vkMemoryRequirements.memoryTypeBits, vkMemoryPropertyFlags)	// memoryTypeIndex (uint32_t)
				};
				if (vkAllocateMemory(vkDevice, &vkMemoryAllocateInfo, vulkanRenderer.getVkAllocationCallbacks(), &vkDeviceMemory) != VK_SUCCESS)
				{
					RENDERER_LOG(vulkanRenderer.getContext(), CRITICAL, "Failed to allocate the Vulkan memory")
				}
				if (vkBindImageMemory(vkDevice, vkImage, vkDeviceMemory, 0) != VK_SUCCESS)
				{
					RENDERER_LOG(vulkanRenderer.getContext(), CRITICAL, "Failed to bind the Vulkan image memory")
				}
			}
		}

		static void destroyAndFreeVkImage(const VulkanRenderer& vulkanRenderer, VkImage& vkImage, VkDeviceMemory& vkDeviceMemory)
		{
			if (VK_NULL_HANDLE != vkImage)
			{
				const VkDevice vkDevice = vulkanRenderer.getVulkanContext().getVkDevice();
				vkDestroyImage(vkDevice, vkImage, vulkanRenderer.getVkAllocationCallbacks());
				vkImage = VK_NULL_HANDLE;
				if (VK_NULL_HANDLE != vkDeviceMemory)
				{
					vkFreeMemory(vkDevice, vkDeviceMemory, vulkanRenderer.getVkAllocationCallbacks());
					vkDeviceMemory = VK_NULL_HANDLE;
				}
			}
		}

		static void destroyAndFreeVkImage(const VulkanRenderer& vulkanRenderer, VkImage& vkImage, VkDeviceMemory& vkDeviceMemory, VkImageView& vkImageView)
		{
			if (VK_NULL_HANDLE != vkImageView)
			{
				vkDestroyImageView(vulkanRenderer.getVulkanContext().getVkDevice(), vkImageView, vulkanRenderer.getVkAllocationCallbacks());
				vkImageView = VK_NULL_HANDLE;
			}
			destroyAndFreeVkImage(vulkanRenderer, vkImage, vkDeviceMemory);
		}

		static void createVkImageView(const VulkanRenderer& vulkanRenderer, VkImage vkImage, VkImageViewType vkImageViewType, uint32_t levelCount, uint32_t layerCount, VkFormat vkFormat, VkImageAspectFlags vkImageAspectFlags, VkImageView& vkImageView)
		{
			const VkImageViewCreateInfo vkImageViewCreateInfo =
			{
				VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,	// sType (VkStructureType)
				nullptr,									// pNext (const void*)
				0,											// flags (VkImageViewCreateFlags)
				vkImage,									// image (VkImage)
				vkImageViewType,							// viewType (VkImageViewType)
				vkFormat,									// format (VkFormat)
				{ // components (VkComponentMapping)
					VK_COMPONENT_SWIZZLE_IDENTITY,			// r (VkComponentSwizzle)
					VK_COMPONENT_SWIZZLE_IDENTITY,			// g (VkComponentSwizzle)
					VK_COMPONENT_SWIZZLE_IDENTITY,			// b (VkComponentSwizzle)
					VK_COMPONENT_SWIZZLE_IDENTITY			// a (VkComponentSwizzle)
				},
				{ // subresourceRange (VkImageSubresourceRange)
					vkImageAspectFlags,						// aspectMask (VkImageAspectFlags)
					0,										// baseMipLevel (uint32_t)
					levelCount,								// levelCount (uint32_t)
					0,										// baseArrayLayer (uint32_t)
					layerCount								// layerCount (uint32_t)
				}
			};
			if (vkCreateImageView(vulkanRenderer.getVulkanContext().getVkDevice(), &vkImageViewCreateInfo, vulkanRenderer.getVkAllocationCallbacks(), &vkImageView) != VK_SUCCESS)
			{
				RENDERER_LOG(vulkanRenderer.getContext(), CRITICAL, "Failed to create Vulkan image view")
			}
		}

		//[-------------------------------------------------------]
		//[ Debug                                                 ]
		//[-------------------------------------------------------]
		#ifdef RENDERER_DEBUG
			static void setDebugObjectName(VkDevice vkDevice, VkDebugReportObjectTypeEXT vkDebugReportObjectTypeEXT, uint64_t object, const char* objectName)
			{
				if (nullptr != vkDebugMarkerSetObjectNameEXT)
				{
					const VkDebugMarkerObjectNameInfoEXT vkDebugMarkerObjectNameInfoEXT =
					{
						VK_STRUCTURE_TYPE_DEBUG_MARKER_OBJECT_NAME_INFO_EXT,	// sType (VkStructureType)
						nullptr,												// pNext (const void*)
						vkDebugReportObjectTypeEXT,								// objectType (VkDebugReportObjectTypeEXT)
						object,													// object (uint64_t)
						objectName												// pObjectName (const char*)
					};
					vkDebugMarkerSetObjectNameEXT(vkDevice, &vkDebugMarkerObjectNameInfoEXT);
				}
			}
		#endif


	};




	//[-------------------------------------------------------]
	//[ VulkanRenderer/RootSignature.h                        ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Vulkan root signature ("pipeline layout" in Vulkan terminology) class
	*/
	class RootSignature final : public Renderer::IRootSignature
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] vulkanRenderer
		*    Owner Vulkan renderer instance
		*  @param[in] rootSignature
		*    Root signature to use
		*/
		RootSignature(VulkanRenderer& vulkanRenderer, const Renderer::RootSignature& rootSignature) :
			IRootSignature(vulkanRenderer),
			mRootSignature(rootSignature),
			mVkPipelineLayout(VK_NULL_HANDLE),
			mVkDescriptorPool(VK_NULL_HANDLE)
		{
			static constexpr uint32_t maxSets = 4242;	// TODO(co) We probably need to get this provided from the outside

			// Copy the parameter data
			const Renderer::Context& context = vulkanRenderer.getContext();
			const uint32_t numberOfRootParameters = mRootSignature.numberOfParameters;
			if (numberOfRootParameters > 0)
			{
				mRootSignature.parameters = RENDERER_MALLOC_TYPED(context, Renderer::RootParameter, numberOfRootParameters);
				Renderer::RootParameter* destinationRootParameters = const_cast<Renderer::RootParameter*>(mRootSignature.parameters);
				memcpy(destinationRootParameters, rootSignature.parameters, sizeof(Renderer::RootParameter) * numberOfRootParameters);

				// Copy the descriptor table data
				for (uint32_t rootParameterIndex = 0; rootParameterIndex < numberOfRootParameters; ++rootParameterIndex)
				{
					Renderer::RootParameter& destinationRootParameter = destinationRootParameters[rootParameterIndex];
					const Renderer::RootParameter& sourceRootParameter = rootSignature.parameters[rootParameterIndex];
					if (Renderer::RootParameterType::DESCRIPTOR_TABLE == destinationRootParameter.parameterType)
					{
						const uint32_t numberOfDescriptorRanges = destinationRootParameter.descriptorTable.numberOfDescriptorRanges;
						destinationRootParameter.descriptorTable.descriptorRanges = reinterpret_cast<uintptr_t>(RENDERER_MALLOC_TYPED(context, Renderer::DescriptorRange, numberOfDescriptorRanges));
						memcpy(reinterpret_cast<Renderer::DescriptorRange*>(destinationRootParameter.descriptorTable.descriptorRanges), reinterpret_cast<const Renderer::DescriptorRange*>(sourceRootParameter.descriptorTable.descriptorRanges), sizeof(Renderer::DescriptorRange) * numberOfDescriptorRanges);
					}
				}
			}

			{ // Copy the static sampler data
				const uint32_t numberOfStaticSamplers = mRootSignature.numberOfStaticSamplers;
				if (numberOfStaticSamplers > 0)
				{
					mRootSignature.staticSamplers = RENDERER_MALLOC_TYPED(context, Renderer::StaticSampler, numberOfStaticSamplers);
					memcpy(const_cast<Renderer::StaticSampler*>(mRootSignature.staticSamplers), rootSignature.staticSamplers, sizeof(Renderer::StaticSampler) * numberOfStaticSamplers);
				}
			}

			// Create the Vulkan descriptor set layout
			const VkDevice vkDevice = vulkanRenderer.getVulkanContext().getVkDevice();
			VkDescriptorSetLayouts vkDescriptorSetLayouts;
			uint32_t numberOfUniformTexelBuffers = 0;	// "VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER"
			uint32_t numberOfStorageTexelBuffers = 0;	// "VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER"
			uint32_t numberOfStorageImage = 0;			// "VK_DESCRIPTOR_TYPE_STORAGE_IMAGE"
			uint32_t numberOfStorageBuffers = 0;		// "VK_DESCRIPTOR_TYPE_STORAGE_BUFFER"
			uint32_t numberOfUniformBuffers = 0;		// "VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER"
			uint32_t numberOfCombinedImageSamplers = 0;	// "VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER"
			if (numberOfRootParameters > 0)
			{
				// Fill the Vulkan descriptor set layout bindings
				vkDescriptorSetLayouts.reserve(numberOfRootParameters);
				mVkDescriptorSetLayouts.resize(numberOfRootParameters);
				std::fill(mVkDescriptorSetLayouts.begin(), mVkDescriptorSetLayouts.end(), static_cast<VkDescriptorSetLayout>(VK_NULL_HANDLE));	// TODO(co) Get rid of this
				typedef std::vector<VkDescriptorSetLayoutBinding> VkDescriptorSetLayoutBindings;
				VkDescriptorSetLayoutBindings vkDescriptorSetLayoutBindings;
				vkDescriptorSetLayoutBindings.reserve(numberOfRootParameters);
				for (uint32_t rootParameterIndex = 0; rootParameterIndex < numberOfRootParameters; ++rootParameterIndex)
				{
					vkDescriptorSetLayoutBindings.clear();

					// TODO(co) For now we only support descriptor tables
					const Renderer::RootParameter& rootParameter = rootSignature.parameters[rootParameterIndex];
					if (Renderer::RootParameterType::DESCRIPTOR_TABLE == rootParameter.parameterType)
					{
						// Process descriptor ranges
						const Renderer::DescriptorRange* descriptorRange = reinterpret_cast<const Renderer::DescriptorRange*>(rootParameter.descriptorTable.descriptorRanges);
						for (uint32_t descriptorRangeIndex = 0; descriptorRangeIndex < rootParameter.descriptorTable.numberOfDescriptorRanges; ++descriptorRangeIndex, ++descriptorRange)
						{
							// Evaluate parameter type
							VkDescriptorType vkDescriptorType = VK_DESCRIPTOR_TYPE_MAX_ENUM;
							switch (descriptorRange->resourceType)
							{
								case Renderer::ResourceType::TEXTURE_BUFFER:
									RENDERER_ASSERT(vulkanRenderer.getContext(), Renderer::DescriptorRangeType::SRV == descriptorRange->rangeType || Renderer::DescriptorRangeType::UAV == descriptorRange->rangeType, "Vulkan renderer backend: Invalid descriptor range type")
									if (Renderer::DescriptorRangeType::SRV == descriptorRange->rangeType)
									{
										vkDescriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER;
										++numberOfUniformTexelBuffers;
									}
									else
									{
										vkDescriptorType = VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER;
										++numberOfStorageTexelBuffers;
									}
									break;

								case Renderer::ResourceType::INDEX_BUFFER:
								case Renderer::ResourceType::VERTEX_BUFFER:
								case Renderer::ResourceType::STRUCTURED_BUFFER:
								case Renderer::ResourceType::INDIRECT_BUFFER:
									RENDERER_ASSERT(vulkanRenderer.getContext(), Renderer::DescriptorRangeType::SRV == descriptorRange->rangeType || Renderer::DescriptorRangeType::UAV == descriptorRange->rangeType, "Vulkan renderer backend: Invalid descriptor range type")
									vkDescriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
									++numberOfStorageBuffers;
									break;

								case Renderer::ResourceType::UNIFORM_BUFFER:
									RENDERER_ASSERT(vulkanRenderer.getContext(), Renderer::DescriptorRangeType::UBV == descriptorRange->rangeType || Renderer::DescriptorRangeType::UAV == descriptorRange->rangeType, "Vulkan renderer backend: Invalid descriptor range type")
									vkDescriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
									++numberOfUniformBuffers;
									break;

								case Renderer::ResourceType::TEXTURE_1D:
								case Renderer::ResourceType::TEXTURE_2D:
								case Renderer::ResourceType::TEXTURE_2D_ARRAY:
								case Renderer::ResourceType::TEXTURE_3D:
								case Renderer::ResourceType::TEXTURE_CUBE:
									RENDERER_ASSERT(vulkanRenderer.getContext(), Renderer::DescriptorRangeType::SRV == descriptorRange->rangeType || Renderer::DescriptorRangeType::UAV == descriptorRange->rangeType, "Vulkan renderer backend: Invalid descriptor range type")
									if (Renderer::DescriptorRangeType::SRV == descriptorRange->rangeType)
									{
										vkDescriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
										++numberOfCombinedImageSamplers;
									}
									else
									{
										vkDescriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
										++numberOfStorageImage;
									}
									break;

								case Renderer::ResourceType::SAMPLER_STATE:
									// Nothing here due to usage of "VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER"
									RENDERER_ASSERT(vulkanRenderer.getContext(), Renderer::DescriptorRangeType::SAMPLER == descriptorRange->rangeType, "Vulkan renderer backend: Invalid descriptor range type")
									break;

								case Renderer::ResourceType::ROOT_SIGNATURE:
								case Renderer::ResourceType::RESOURCE_GROUP:
								case Renderer::ResourceType::GRAPHICS_PROGRAM:
								case Renderer::ResourceType::VERTEX_ARRAY:
								case Renderer::ResourceType::RENDER_PASS:
								case Renderer::ResourceType::SWAP_CHAIN:
								case Renderer::ResourceType::FRAMEBUFFER:
								case Renderer::ResourceType::GRAPHICS_PIPELINE_STATE:
								case Renderer::ResourceType::COMPUTE_PIPELINE_STATE:
								case Renderer::ResourceType::VERTEX_SHADER:
								case Renderer::ResourceType::TESSELLATION_CONTROL_SHADER:
								case Renderer::ResourceType::TESSELLATION_EVALUATION_SHADER:
								case Renderer::ResourceType::GEOMETRY_SHADER:
								case Renderer::ResourceType::FRAGMENT_SHADER:
								case Renderer::ResourceType::COMPUTE_SHADER:
									RENDERER_ASSERT(vulkanRenderer.getContext(), false, "Vulkan renderer backend: Invalid resource type")
									break;
							}

							// Evaluate shader visibility
							VkShaderStageFlags vkShaderStageFlags = VK_SHADER_STAGE_FLAG_BITS_MAX_ENUM;
							switch (descriptorRange->shaderVisibility)
							{
								case Renderer::ShaderVisibility::ALL:
									vkShaderStageFlags = VK_SHADER_STAGE_ALL;
									break;

								case Renderer::ShaderVisibility::VERTEX:
									vkShaderStageFlags = VK_SHADER_STAGE_VERTEX_BIT;
									break;

								case Renderer::ShaderVisibility::TESSELLATION_CONTROL:
									vkShaderStageFlags = VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
									break;

								case Renderer::ShaderVisibility::TESSELLATION_EVALUATION:
									vkShaderStageFlags = VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
									break;

								case Renderer::ShaderVisibility::GEOMETRY:
									vkShaderStageFlags = VK_SHADER_STAGE_GEOMETRY_BIT;
									break;

								case Renderer::ShaderVisibility::FRAGMENT:
									vkShaderStageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
									break;

								case Renderer::ShaderVisibility::COMPUTE:
									vkShaderStageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
									break;

								case Renderer::ShaderVisibility::ALL_GRAPHICS:
									vkShaderStageFlags = VK_SHADER_STAGE_ALL_GRAPHICS;
									break;
							}

							// Add the Vulkan descriptor set layout binding
							if (VK_DESCRIPTOR_TYPE_MAX_ENUM != vkDescriptorType)
							{
								const VkDescriptorSetLayoutBinding vkDescriptorSetLayoutBinding =
								{
									descriptorRangeIndex,	// binding (uint32_t)
									vkDescriptorType,		// descriptorType (VkDescriptorType)
									1,						// descriptorCount (uint32_t)
									vkShaderStageFlags,		// stageFlags (VkShaderStageFlags)
									nullptr					// pImmutableSamplers (const VkSampler*)
								};
								vkDescriptorSetLayoutBindings.push_back(vkDescriptorSetLayoutBinding);
							}
						}
					}

					// Create the Vulkan descriptor set layout
					if (!vkDescriptorSetLayoutBindings.empty())
					{
						const VkDescriptorSetLayoutCreateInfo vkDescriptorSetLayoutCreateInfo =
						{
							VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,			// sType (VkStructureType)
							nullptr,														// pNext (const void*)
							0,																// flags (VkDescriptorSetLayoutCreateFlags)
							static_cast<uint32_t>(vkDescriptorSetLayoutBindings.size()),	// bindingCount (uint32_t)
							vkDescriptorSetLayoutBindings.data()							// pBindings (const VkDescriptorSetLayoutBinding*)
						};
						if (vkCreateDescriptorSetLayout(vkDevice, &vkDescriptorSetLayoutCreateInfo, vulkanRenderer.getVkAllocationCallbacks(), &mVkDescriptorSetLayouts[rootParameterIndex]) != VK_SUCCESS)
						{
							RENDERER_LOG(vulkanRenderer.getContext(), CRITICAL, "Failed to create the Vulkan descriptor set layout")
						}
						vkDescriptorSetLayouts.push_back(mVkDescriptorSetLayouts[rootParameterIndex]);
					}
				}
			}

			{ // Create the Vulkan pipeline layout
				const VkPipelineLayoutCreateInfo vkPipelineLayoutCreateInfo =
				{
					VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,								// sType (VkStructureType)
					nullptr,																	// pNext (const void*)
					0,																			// flags (VkPipelineLayoutCreateFlags)
					static_cast<uint32_t>(vkDescriptorSetLayouts.size()),						// setLayoutCount (uint32_t)
					vkDescriptorSetLayouts.empty() ? nullptr : vkDescriptorSetLayouts.data(),	// pSetLayouts (const VkDescriptorSetLayout*)
					0,																			// pushConstantRangeCount (uint32_t)
					nullptr																		// pPushConstantRanges (const VkPushConstantRange*)
				};
				if (vkCreatePipelineLayout(vkDevice, &vkPipelineLayoutCreateInfo, vulkanRenderer.getVkAllocationCallbacks(), &mVkPipelineLayout) != VK_SUCCESS)
				{
					RENDERER_LOG(vulkanRenderer.getContext(), CRITICAL, "Failed to create the Vulkan pipeline layout")
				}
			}

			{ // Create the Vulkan descriptor pool
				typedef std::array<VkDescriptorPoolSize, 3> VkDescriptorPoolSizes;
				VkDescriptorPoolSizes vkDescriptorPoolSizes;
				uint32_t numberOfVkDescriptorPoolSizes = 0;

				// "VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER"
				if (numberOfCombinedImageSamplers > 0)
				{
					VkDescriptorPoolSize& vkDescriptorPoolSize = vkDescriptorPoolSizes[numberOfVkDescriptorPoolSizes];
					vkDescriptorPoolSize.type			 = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;	// type (VkDescriptorType)
					vkDescriptorPoolSize.descriptorCount = maxSets * numberOfCombinedImageSamplers;		// descriptorCount (uint32_t)
					++numberOfVkDescriptorPoolSizes;
				}

				// "VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER"
				if (numberOfUniformTexelBuffers > 0)
				{
					VkDescriptorPoolSize& vkDescriptorPoolSize = vkDescriptorPoolSizes[numberOfVkDescriptorPoolSizes];
					vkDescriptorPoolSize.type			 = VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER;	// type (VkDescriptorType)
					vkDescriptorPoolSize.descriptorCount = maxSets * numberOfUniformTexelBuffers;	// descriptorCount (uint32_t)
					++numberOfVkDescriptorPoolSizes;
				}

				// "VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER"
				if (numberOfStorageTexelBuffers > 0)
				{
					VkDescriptorPoolSize& vkDescriptorPoolSize = vkDescriptorPoolSizes[numberOfVkDescriptorPoolSizes];
					vkDescriptorPoolSize.type			 = VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER;	// type (VkDescriptorType)
					vkDescriptorPoolSize.descriptorCount = maxSets * numberOfStorageTexelBuffers;	// descriptorCount (uint32_t)
					++numberOfVkDescriptorPoolSizes;
				}

				// "VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER"
				if (numberOfUniformBuffers > 0)
				{
					VkDescriptorPoolSize& vkDescriptorPoolSize = vkDescriptorPoolSizes[numberOfVkDescriptorPoolSizes];
					vkDescriptorPoolSize.type			 = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;	// type (VkDescriptorType)
					vkDescriptorPoolSize.descriptorCount = maxSets * numberOfUniformBuffers;	// descriptorCount (uint32_t)
					++numberOfVkDescriptorPoolSizes;
				}

				// "VK_DESCRIPTOR_TYPE_STORAGE_IMAGE"
				if (numberOfStorageImage > 0)
				{
					VkDescriptorPoolSize& vkDescriptorPoolSize = vkDescriptorPoolSizes[numberOfVkDescriptorPoolSizes];
					vkDescriptorPoolSize.type			 = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;	// type (VkDescriptorType)
					vkDescriptorPoolSize.descriptorCount = maxSets * numberOfStorageImage;		// descriptorCount (uint32_t)
					++numberOfVkDescriptorPoolSizes;
				}

				// "VK_DESCRIPTOR_TYPE_STORAGE_BUFFER"
				if (numberOfStorageBuffers > 0)
				{
					VkDescriptorPoolSize& vkDescriptorPoolSize = vkDescriptorPoolSizes[numberOfVkDescriptorPoolSizes];
					vkDescriptorPoolSize.type			 = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;	// type (VkDescriptorType)
					vkDescriptorPoolSize.descriptorCount = maxSets * numberOfStorageBuffers;	// descriptorCount (uint32_t)
					++numberOfVkDescriptorPoolSizes;
				}

				// Create the Vulkan descriptor pool
				if (numberOfVkDescriptorPoolSizes > 0)
				{
					const VkDescriptorPoolCreateInfo VkDescriptorPoolCreateInfo =
					{
						VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,		// sType (VkStructureType)
						nullptr,											// pNext (const void*)
						VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,	// flags (VkDescriptorPoolCreateFlags)
						maxSets,											// maxSets (uint32_t)
						numberOfVkDescriptorPoolSizes,						// poolSizeCount (uint32_t)
						vkDescriptorPoolSizes.data()						// pPoolSizes (const VkDescriptorPoolSize*)
					};
					if (vkCreateDescriptorPool(vkDevice, &VkDescriptorPoolCreateInfo, vulkanRenderer.getVkAllocationCallbacks(), &mVkDescriptorPool) != VK_SUCCESS)
					{
						RENDERER_LOG(vulkanRenderer.getContext(), CRITICAL, "Failed to create the Vulkan descriptor pool")
					}
				}
			}

			SET_DEFAULT_DEBUG_NAME	// setDebugName("");
		}

		/**
		*  @brief
		*    Destructor
		*/
		virtual ~RootSignature() override
		{
			const VulkanRenderer& vulkanRenderer = static_cast<VulkanRenderer&>(getRenderer());
			const VkDevice vkDevice = vulkanRenderer.getVulkanContext().getVkDevice();

			// Destroy the Vulkan descriptor pool
			if (VK_NULL_HANDLE != mVkDescriptorPool)
			{
				vkDestroyDescriptorPool(vkDevice, mVkDescriptorPool, vulkanRenderer.getVkAllocationCallbacks());
			}

			// Destroy the Vulkan pipeline layout
			if (VK_NULL_HANDLE != mVkPipelineLayout)
			{
				vkDestroyPipelineLayout(vkDevice, mVkPipelineLayout, vulkanRenderer.getVkAllocationCallbacks());
			}

			// Destroy the Vulkan descriptor set layout
			for (VkDescriptorSetLayout vkDescriptorSetLayout : mVkDescriptorSetLayouts)
			{
				if (VK_NULL_HANDLE != vkDescriptorSetLayout)
				{
					vkDestroyDescriptorSetLayout(vkDevice, vkDescriptorSetLayout, vulkanRenderer.getVkAllocationCallbacks());
				}
			}

			// Destroy the root signature data
			const Renderer::Context& context = getRenderer().getContext();
			if (nullptr != mRootSignature.parameters)
			{
				for (uint32_t rootParameterIndex = 0; rootParameterIndex < mRootSignature.numberOfParameters; ++rootParameterIndex)
				{
					const Renderer::RootParameter& rootParameter = mRootSignature.parameters[rootParameterIndex];
					if (Renderer::RootParameterType::DESCRIPTOR_TABLE == rootParameter.parameterType)
					{
						RENDERER_FREE(context, reinterpret_cast<Renderer::DescriptorRange*>(rootParameter.descriptorTable.descriptorRanges));
					}
				}
				RENDERER_FREE(context, const_cast<Renderer::RootParameter*>(mRootSignature.parameters));
			}
			RENDERER_FREE(context, const_cast<Renderer::StaticSampler*>(mRootSignature.staticSamplers));
		}

		/**
		*  @brief
		*    Return the root signature data
		*
		*  @return
		*    The root signature data
		*/
		inline const Renderer::RootSignature& getRootSignature() const
		{
			return mRootSignature;
		}

		/**
		*  @brief
		*    Return the Vulkan pipeline layout
		*
		*  @return
		*    The Vulkan pipeline layout
		*/
		inline VkPipelineLayout getVkPipelineLayout() const
		{
			return mVkPipelineLayout;
		}

		/**
		*  @brief
		*    Return the Vulkan descriptor pool
		*
		*  @return
		*    The Vulkan descriptor pool
		*/
		inline VkDescriptorPool getVkDescriptorPool() const
		{
			return mVkDescriptorPool;
		}


	//[-------------------------------------------------------]
	//[ Public virtual Renderer::IResource methods            ]
	//[-------------------------------------------------------]
	public:
		#ifdef RENDERER_DEBUG
			virtual void setDebugName(const char* name) override
			{
				if (nullptr != vkDebugMarkerSetObjectNameEXT)
				{
					const VkDevice vkDevice = static_cast<const VulkanRenderer&>(getRenderer()).getVulkanContext().getVkDevice();
					for (VkDescriptorSetLayout vkDescriptorSetLayout : mVkDescriptorSetLayouts)
					{
						Helper::setDebugObjectName(vkDevice, VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT_EXT, (uint64_t)vkDescriptorSetLayout, name);
					}
					Helper::setDebugObjectName(vkDevice, VK_DEBUG_REPORT_OBJECT_TYPE_PIPELINE_LAYOUT_EXT, (uint64_t)mVkPipelineLayout, name);
					Helper::setDebugObjectName(vkDevice, VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_POOL_EXT, (uint64_t)mVkDescriptorPool, name);
				}
			}
		#endif


	//[-------------------------------------------------------]
	//[ Public virtual Renderer::IRootSignature methods       ]
	//[-------------------------------------------------------]
	public:
		virtual Renderer::IResourceGroup* createResourceGroup(uint32_t rootParameterIndex, uint32_t numberOfResources, Renderer::IResource** resources, Renderer::ISamplerState** samplerStates = nullptr) override;


	//[-------------------------------------------------------]
	//[ Protected virtual Renderer::RefCount methods          ]
	//[-------------------------------------------------------]
	protected:
		inline virtual void selfDestruct() override
		{
			RENDERER_DELETE(getRenderer().getContext(), RootSignature, this);
		}


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		explicit RootSignature(const RootSignature& source) = delete;
		RootSignature& operator =(const RootSignature& source) = delete;


	//[-------------------------------------------------------]
	//[ Private definitions                                   ]
	//[-------------------------------------------------------]
	private:
		typedef std::vector<VkDescriptorSetLayout> VkDescriptorSetLayouts;


	//[-------------------------------------------------------]
	//[ Private data                                          ]
	//[-------------------------------------------------------]
	private:
		Renderer::RootSignature	mRootSignature;
		VkDescriptorSetLayouts	mVkDescriptorSetLayouts;
		VkPipelineLayout		mVkPipelineLayout;
		VkDescriptorPool		mVkDescriptorPool;


	};




	//[-------------------------------------------------------]
	//[ VulkanRenderer/IndexBuffer.h                          ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Vulkan index buffer object (IBO) interface
	*/
	class IndexBuffer final : public Renderer::IIndexBuffer
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] vulkanRenderer
		*    Owner Vulkan renderer instance
		*  @param[in] numberOfBytes
		*    Number of bytes within the index buffer, must be valid
		*  @param[in] data
		*    Index buffer data, can be a null pointer (empty buffer)
		*  @param[in] bufferFlags
		*    Buffer flags, see "Renderer::BufferFlag"
		*  @param[in] bufferUsage
		*    Indication of the buffer usage
		*  @param[in] indexBufferFormat
		*    Index buffer data format
		*/
		IndexBuffer(VulkanRenderer& vulkanRenderer, uint32_t numberOfBytes, const void* data, uint32_t bufferFlags, MAYBE_UNUSED Renderer::BufferUsage bufferUsage, Renderer::IndexBufferFormat::Enum indexBufferFormat) :
			IIndexBuffer(vulkanRenderer),
			mVkIndexType(Mapping::getVulkanType(vulkanRenderer.getContext(), indexBufferFormat)),
			mVkBuffer(VK_NULL_HANDLE),
			mVkDeviceMemory(VK_NULL_HANDLE)
		{
			int vkBufferUsageFlagBits = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
			if ((bufferFlags & Renderer::BufferFlag::UNORDERED_ACCESS) != 0 || (bufferFlags & Renderer::BufferFlag::SHADER_RESOURCE) != 0)
			{
				vkBufferUsageFlagBits |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
			}
			Helper::createAndAllocateVkBuffer(vulkanRenderer, static_cast<VkBufferUsageFlagBits>(vkBufferUsageFlagBits), VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, numberOfBytes, data, mVkBuffer, mVkDeviceMemory);
			SET_DEFAULT_DEBUG_NAME	// setDebugName("");
		}

		/**
		*  @brief
		*    Destructor
		*/
		inline virtual ~IndexBuffer() override
		{
			Helper::destroyAndFreeVkBuffer(static_cast<const VulkanRenderer&>(getRenderer()), mVkBuffer, mVkDeviceMemory);
		}

		/**
		*  @brief
		*    Return the Vulkan index type
		*
		*  @return
		*    The Vulkan index type
		*/
		inline VkIndexType getVkIndexType() const
		{
			return mVkIndexType;
		}

		/**
		*  @brief
		*    Return the Vulkan index buffer
		*
		*  @return
		*    The Vulkan index buffer
		*/
		inline VkBuffer getVkBuffer() const
		{
			return mVkBuffer;
		}

		/**
		*  @brief
		*    Return the Vulkan device memory
		*
		*  @return
		*    The Vulkan device memory
		*/
		inline VkDeviceMemory getVkDeviceMemory() const
		{
			return mVkDeviceMemory;
		}


	//[-------------------------------------------------------]
	//[ Public virtual Renderer::IResource methods            ]
	//[-------------------------------------------------------]
	public:
		DEFINE_SET_DEBUG_NAME_VKBUFFER_VKDEVICEMEMORY("IBO", 6)


	//[-------------------------------------------------------]
	//[ Protected virtual Renderer::RefCount methods          ]
	//[-------------------------------------------------------]
	protected:
		inline virtual void selfDestruct() override
		{
			RENDERER_DELETE(getRenderer().getContext(), IndexBuffer, this);
		}


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		explicit IndexBuffer(const IndexBuffer& source) = delete;
		IndexBuffer& operator =(const IndexBuffer& source) = delete;


	//[-------------------------------------------------------]
	//[ Private data                                          ]
	//[-------------------------------------------------------]
	private:
		VkIndexType	   mVkIndexType;	///< Vulkan vertex type
		VkBuffer	   mVkBuffer;		///< Vulkan vertex buffer
		VkDeviceMemory mVkDeviceMemory;	///< Vulkan vertex memory


	};




	//[-------------------------------------------------------]
	//[ VulkanRenderer/VertexBuffer.h                         ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Vulkan vertex buffer object (VBO) interface
	*/
	class VertexBuffer final : public Renderer::IVertexBuffer
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] vulkanRenderer
		*    Owner Vulkan renderer instance
		*  @param[in] numberOfBytes
		*    Number of bytes within the vertex buffer, must be valid
		*  @param[in] data
		*    Vertex buffer data, can be a null pointer (empty buffer)
		*  @param[in] bufferFlags
		*    Buffer flags, see "Renderer::BufferFlag"
		*  @param[in] bufferUsage
		*    Indication of the buffer usage
		*/
		VertexBuffer(VulkanRenderer& vulkanRenderer, uint32_t numberOfBytes, const void* data, uint32_t bufferFlags, MAYBE_UNUSED Renderer::BufferUsage bufferUsage) :
			IVertexBuffer(vulkanRenderer),
			mVkBuffer(VK_NULL_HANDLE),
			mVkDeviceMemory(VK_NULL_HANDLE)
		{
			int vkBufferUsageFlagBits = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
			if ((bufferFlags & Renderer::BufferFlag::UNORDERED_ACCESS) != 0 || (bufferFlags & Renderer::BufferFlag::SHADER_RESOURCE) != 0)
			{
				vkBufferUsageFlagBits |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
			}
			Helper::createAndAllocateVkBuffer(vulkanRenderer, static_cast<VkBufferUsageFlagBits>(vkBufferUsageFlagBits), VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, numberOfBytes, data, mVkBuffer, mVkDeviceMemory);
			SET_DEFAULT_DEBUG_NAME	// setDebugName("");
		}

		/**
		*  @brief
		*    Destructor
		*/
		inline virtual ~VertexBuffer() override
		{
			Helper::destroyAndFreeVkBuffer(static_cast<const VulkanRenderer&>(getRenderer()), mVkBuffer, mVkDeviceMemory);
		}

		/**
		*  @brief
		*    Return the Vulkan vertex buffer
		*
		*  @return
		*    The Vulkan vertex buffer
		*/
		inline VkBuffer getVkBuffer() const
		{
			return mVkBuffer;
		}

		/**
		*  @brief
		*    Return the Vulkan device memory
		*
		*  @return
		*    The Vulkan device memory
		*/
		inline VkDeviceMemory getVkDeviceMemory() const
		{
			return mVkDeviceMemory;
		}


	//[-------------------------------------------------------]
	//[ Public virtual Renderer::IResource methods            ]
	//[-------------------------------------------------------]
	public:
		DEFINE_SET_DEBUG_NAME_VKBUFFER_VKDEVICEMEMORY("VBO", 6)


	//[-------------------------------------------------------]
	//[ Protected virtual Renderer::RefCount methods          ]
	//[-------------------------------------------------------]
	protected:
		inline virtual void selfDestruct() override
		{
			RENDERER_DELETE(getRenderer().getContext(), VertexBuffer, this);
		}


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		explicit VertexBuffer(const VertexBuffer& source) = delete;
		VertexBuffer& operator =(const VertexBuffer& source) = delete;


	//[-------------------------------------------------------]
	//[ Private data                                          ]
	//[-------------------------------------------------------]
	private:
		VkBuffer	   mVkBuffer;		///< Vulkan vertex buffer
		VkDeviceMemory mVkDeviceMemory;	///< Vulkan vertex memory


	};




	//[-------------------------------------------------------]
	//[ VulkanRenderer/VertexArray.h                          ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Vulkan vertex array interface
	*/
	class VertexArray final : public Renderer::IVertexArray
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] vulkanRenderer
		*    Owner Vulkan renderer instance
		*  @param[in] vertexAttributes
		*    Vertex attributes ("vertex declaration" in Direct3D 9 terminology, "input layout" in Direct3D 10 & 11 terminology)
		*  @param[in] numberOfVertexBuffers
		*    Number of vertex buffers, having zero vertex buffers is valid
		*  @param[in] vertexBuffers
		*    At least numberOfVertexBuffers instances of vertex array vertex buffers, can be a null pointer in case there are zero vertex buffers, the data is internally copied and you have to free your memory if you no longer need it
		*  @param[in] indexBuffer
		*    Optional index buffer to use, can be a null pointer, the vertex array instance keeps a reference to the index buffer
		*/
		VertexArray(VulkanRenderer& vulkanRenderer, const Renderer::VertexAttributes& vertexAttributes, uint32_t numberOfVertexBuffers, const Renderer::VertexArrayVertexBuffer* vertexBuffers, IndexBuffer* indexBuffer) :
			IVertexArray(static_cast<Renderer::IRenderer&>(vulkanRenderer)),
			mIndexBuffer(indexBuffer),
			mNumberOfSlots(numberOfVertexBuffers),
			mVertexVkBuffers(nullptr),
			mStrides(nullptr),
			mOffsets(nullptr),
			mVertexBuffers(nullptr)
		{
			// Add a reference to the given index buffer
			if (nullptr != mIndexBuffer)
			{
				mIndexBuffer->addReference();
			}

			// Add a reference to the used vertex buffers
			if (mNumberOfSlots > 0)
			{
				const Renderer::Context& context = vulkanRenderer.getContext();
				mVertexVkBuffers = RENDERER_MALLOC_TYPED(context, VkBuffer, mNumberOfSlots);
				mStrides = RENDERER_MALLOC_TYPED(context, uint32_t, mNumberOfSlots);
				mOffsets = RENDERER_MALLOC_TYPED(context, VkDeviceSize, mNumberOfSlots);
				memset(mOffsets, 0, sizeof(VkDeviceSize) * mNumberOfSlots);	// Vertex buffer offset is not supported by OpenGL, so our renderer API doesn't support it either, set everything to zero
				mVertexBuffers = RENDERER_MALLOC_TYPED(context, VertexBuffer*, mNumberOfSlots);

				{ // Loop through all vertex buffers
					VkBuffer* currentVertexVkBuffer = mVertexVkBuffers;
					VertexBuffer** currentVertexBuffer = mVertexBuffers;
					const Renderer::VertexArrayVertexBuffer* vertexBufferEnd = vertexBuffers + mNumberOfSlots;
					for (const Renderer::VertexArrayVertexBuffer* vertexBuffer = vertexBuffers; vertexBuffer < vertexBufferEnd; ++vertexBuffer, ++currentVertexVkBuffer, ++currentVertexBuffer)
					{
						// TODO(co) Add security check: Is the given resource one of the currently used renderer?
						*currentVertexBuffer = static_cast<VertexBuffer*>(vertexBuffer->vertexBuffer);
						*currentVertexVkBuffer = (*currentVertexBuffer)->getVkBuffer();
						(*currentVertexBuffer)->addReference();
					}
				}

				{ // Gather slot related data
					const Renderer::VertexAttribute* attribute = vertexAttributes.attributes;
					const Renderer::VertexAttribute* attributesEnd = attribute + vertexAttributes.numberOfAttributes;
					for (; attribute < attributesEnd;  ++attribute)
					{
						mStrides[attribute->inputSlot] = attribute->strideInBytes;
					}
				}
			}
		}

		/**
		*  @brief
		*    Destructor
		*/
		virtual ~VertexArray() override
		{
			// Release the index buffer reference
			if (nullptr != mIndexBuffer)
			{
				mIndexBuffer->releaseReference();
			}

			// Cleanup Vulkan input slot data
			const Renderer::Context& context = getRenderer().getContext();
			if (mNumberOfSlots > 0)
			{
				RENDERER_FREE(context, mVertexVkBuffers);
				RENDERER_FREE(context, mStrides);
				RENDERER_FREE(context, mOffsets);
			}

			// Release the reference to the used vertex buffers
			if (nullptr != mVertexBuffers)
			{
				// Release references
				VertexBuffer** vertexBuffersEnd = mVertexBuffers + mNumberOfSlots;
				for (VertexBuffer** vertexBuffer = mVertexBuffers; vertexBuffer < vertexBuffersEnd; ++vertexBuffer)
				{
					(*vertexBuffer)->releaseReference();
				}

				// Cleanup
				RENDERER_FREE(context, mVertexBuffers);
			}
		}

		/**
		*  @brief
		*    Return the used index buffer
		*
		*  @return
		*    The used index buffer, can be a null pointer, do not release the returned instance unless you added an own reference to it
		*/
		inline IndexBuffer* getIndexBuffer() const
		{
			return mIndexBuffer;
		}

		/**
		*  @brief
		*    Bind Vulkan buffers
		*
		*  @param[in] vkCommandBuffer
		*    Vulkan command buffer to write into
		*/
		void bindVulkanBuffers(VkCommandBuffer vkCommandBuffer) const
		{
			// Set the Vulkan vertex buffers
			if (nullptr != mVertexVkBuffers)
			{
				vkCmdBindVertexBuffers(vkCommandBuffer, 0, mNumberOfSlots, mVertexVkBuffers, mOffsets);
			}
			else
			{
				// Do nothing since the Vulkan specification says "bindingCount must be greater than 0"
				// vkCmdBindVertexBuffers(vkCommandBuffer, 0, 0, nullptr, nullptr);
			}

			// Set the used index buffer
			// -> In case of no index buffer we don't set null indices, there's not really a point in it
			if (nullptr != mIndexBuffer)
			{
				vkCmdBindIndexBuffer(vkCommandBuffer, mIndexBuffer->getVkBuffer(), 0, mIndexBuffer->getVkIndexType());
			}
		}


	//[-------------------------------------------------------]
	//[ Protected virtual Renderer::RefCount methods          ]
	//[-------------------------------------------------------]
	protected:
		inline virtual void selfDestruct() override
		{
			RENDERER_DELETE(getRenderer().getContext(), VertexArray, this);
		}


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		explicit VertexArray(const VertexArray& source) = delete;
		VertexArray& operator =(const VertexArray& source) = delete;


	//[-------------------------------------------------------]
	//[ Private data                                          ]
	//[-------------------------------------------------------]
	private:
		IndexBuffer*   mIndexBuffer;		///< Optional index buffer to use, can be a null pointer, the vertex array instance keeps a reference to the index buffer
		// Vulkan input slots
		uint32_t	   mNumberOfSlots;		///< Number of used Vulkan input slots
		VkBuffer*	   mVertexVkBuffers;	///< Vulkan vertex buffers
		uint32_t*	   mStrides;			///< Strides in bytes, if "mVertexVkBuffers" is no null pointer this is no null pointer as well
		VkDeviceSize*  mOffsets;			///< Offsets in bytes, if "mVertexVkBuffers" is no null pointer this is no null pointer as well
		// For proper vertex buffer reference counter behaviour
		VertexBuffer** mVertexBuffers;		///< Vertex buffers (we keep a reference to it) used by this vertex array, can be a null pointer


	};




	//[-------------------------------------------------------]
	//[ VulkanRenderer/TextureBuffer.h                        ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Vulkan texture buffer object (TBO) interface
	*/
	class TextureBuffer final : public Renderer::ITextureBuffer
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] vulkanRenderer
		*    Owner Vulkan renderer instance
		*  @param[in] numberOfBytes
		*    Number of bytes within the texture buffer, must be valid
		*  @param[in] data
		*    Texture buffer data, can be a null pointer (empty buffer)
		*  @param[in] bufferFlags
		*    Buffer flags, see "Renderer::BufferFlag"
		*  @param[in] bufferUsage
		*    Indication of the buffer usage
		*  @param[in] textureFormat
		*    Texture buffer data format
		*/
		TextureBuffer(VulkanRenderer& vulkanRenderer, uint32_t numberOfBytes, const void* data, uint32_t bufferFlags, MAYBE_UNUSED Renderer::BufferUsage bufferUsage, Renderer::TextureFormat::Enum textureFormat) :
			ITextureBuffer(vulkanRenderer),
			mVkBuffer(VK_NULL_HANDLE),
			mVkDeviceMemory(VK_NULL_HANDLE),
			mVkBufferView(VK_NULL_HANDLE)
		{
			// Sanity check
			RENDERER_ASSERT(vulkanRenderer.getContext(), (numberOfBytes % Renderer::TextureFormat::getNumberOfBytesPerElement(textureFormat)) == 0, "The Vulkan texture buffer size must be a multiple of the selected texture format bytes per texel")

			// Create the texture buffer
			uint32_t vkBufferUsageFlagBits = 0;
			if (bufferFlags & Renderer::BufferFlag::SHADER_RESOURCE)
			{
				vkBufferUsageFlagBits |= VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT;
			}
			if (bufferFlags & Renderer::BufferFlag::UNORDERED_ACCESS)
			{
				vkBufferUsageFlagBits |= VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT;
			}
			Helper::createAndAllocateVkBuffer(vulkanRenderer, static_cast<VkBufferUsageFlagBits>(vkBufferUsageFlagBits), VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, numberOfBytes, data, mVkBuffer, mVkDeviceMemory);

			// Create Vulkan buffer view
			if ((bufferFlags & Renderer::BufferFlag::SHADER_RESOURCE) != 0 || (bufferFlags & Renderer::BufferFlag::UNORDERED_ACCESS) != 0)
			{
				const VkBufferViewCreateInfo vkBufferViewCreateInfo =
				{
					VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO,	// sType (VkStructureType)
					nullptr,									// pNext (const void*)
					0,											// flags (VkBufferViewCreateFlags)
					mVkBuffer,									// buffer (VkBuffer)
					Mapping::getVulkanFormat(textureFormat),	// format (VkFormat)
					0,											// offset (VkDeviceSize)
					VK_WHOLE_SIZE								// range (VkDeviceSize)
				};
				if (vkCreateBufferView(vulkanRenderer.getVulkanContext().getVkDevice(), &vkBufferViewCreateInfo, vulkanRenderer.getVkAllocationCallbacks(), &mVkBufferView) != VK_SUCCESS)
				{
					RENDERER_LOG(vulkanRenderer.getContext(), CRITICAL, "Failed to create the Vulkan buffer view")
				}
			}

			// Set debug name
			SET_DEFAULT_DEBUG_NAME	// setDebugName("");
		}

		/**
		*  @brief
		*    Destructor
		*/
		virtual ~TextureBuffer() override
		{
			const VulkanRenderer& vulkanRenderer = static_cast<const VulkanRenderer&>(getRenderer());
			if (VK_NULL_HANDLE != mVkBufferView)
			{
				vkDestroyBufferView(vulkanRenderer.getVulkanContext().getVkDevice(), mVkBufferView, vulkanRenderer.getVkAllocationCallbacks());
			}
			Helper::destroyAndFreeVkBuffer(vulkanRenderer, mVkBuffer, mVkDeviceMemory);
		}

		/**
		*  @brief
		*    Return the Vulkan texture buffer
		*
		*  @return
		*    The Vulkan texture buffer
		*/
		inline VkBuffer getVkBuffer() const
		{
			return mVkBuffer;
		}

		/**
		*  @brief
		*    Return the Vulkan device memory
		*
		*  @return
		*    The Vulkan device memory
		*/
		inline VkDeviceMemory getVkDeviceMemory() const
		{
			return mVkDeviceMemory;
		}

		/**
		*  @brief
		*    Return the Vulkan buffer view
		*
		*  @return
		*    The Vulkan buffer view
		*/
		inline VkBufferView getVkBufferView() const
		{
			return mVkBufferView;
		}


	//[-------------------------------------------------------]
	//[ Public virtual Renderer::IResource methods            ]
	//[-------------------------------------------------------]
	public:
		#ifdef RENDERER_DEBUG
			virtual void setDebugName(const char* name) override
			{
				if (nullptr != vkDebugMarkerSetObjectNameEXT)
				{
					const VkDevice vkDevice = static_cast<const VulkanRenderer&>(getRenderer()).getVulkanContext().getVkDevice();
					Helper::setDebugObjectName(vkDevice, VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_EXT, (uint64_t)mVkBuffer, name);
					Helper::setDebugObjectName(vkDevice, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_MEMORY_EXT, (uint64_t)mVkDeviceMemory, name);
					Helper::setDebugObjectName(vkDevice, VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_VIEW_EXT, (uint64_t)mVkBufferView, name);
				}
			}
		#endif


	//[-------------------------------------------------------]
	//[ Protected virtual Renderer::RefCount methods          ]
	//[-------------------------------------------------------]
	protected:
		inline virtual void selfDestruct() override
		{
			RENDERER_DELETE(getRenderer().getContext(), TextureBuffer, this);
		}


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		explicit TextureBuffer(const TextureBuffer& source) = delete;
		TextureBuffer& operator =(const TextureBuffer& source) = delete;


	//[-------------------------------------------------------]
	//[ Private data                                          ]
	//[-------------------------------------------------------]
	private:
		VkBuffer	   mVkBuffer;		///< Vulkan uniform texel buffer
		VkDeviceMemory mVkDeviceMemory;	///< Vulkan uniform texel memory
		VkBufferView   mVkBufferView;	///< Vulkan buffer view


	};




	//[-------------------------------------------------------]
	//[ VulkanRenderer/StructuredBuffer.h                     ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Vulkan structured buffer object interface
	*/
	class StructuredBuffer final : public Renderer::IStructuredBuffer
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] vulkanRenderer
		*    Owner Vulkan renderer instance
		*  @param[in] numberOfBytes
		*    Number of bytes within the structured buffer, must be valid
		*  @param[in] data
		*    Structured buffer data, can be a null pointer (empty buffer)
		*  @param[in] bufferUsage
		*    Indication of the buffer usage
		*  @param[in] numberOfStructureBytes
		*    Number of structure bytes
		*/
		StructuredBuffer(VulkanRenderer& vulkanRenderer, uint32_t numberOfBytes, const void* data, MAYBE_UNUSED Renderer::BufferUsage bufferUsage, MAYBE_UNUSED uint32_t numberOfStructureBytes) :
			IStructuredBuffer(vulkanRenderer),
			mVkBuffer(VK_NULL_HANDLE),
			mVkDeviceMemory(VK_NULL_HANDLE)
		{
			// Sanity checks
			RENDERER_ASSERT(vulkanRenderer.getContext(), (numberOfBytes % numberOfStructureBytes) == 0, "The Vulkan structured buffer size must be a multiple of the given number of structure bytes")
			RENDERER_ASSERT(vulkanRenderer.getContext(), (numberOfBytes % (sizeof(float) * 4)) == 0, "Performance: The Vulkan structured buffer should be aligned to a 128-bit stride, see \"Understanding Structured Buffer Performance\" by Evan Hart, posted Apr 17 2015 at 11:33AM - https://developer.nvidia.com/content/understanding-structured-buffer-performance")

			// Create the structured buffer
			Helper::createAndAllocateVkBuffer(vulkanRenderer, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, numberOfBytes, data, mVkBuffer, mVkDeviceMemory);
			SET_DEFAULT_DEBUG_NAME	// setDebugName("");
		}

		/**
		*  @brief
		*    Destructor
		*/
		virtual ~StructuredBuffer() override
		{
			Helper::destroyAndFreeVkBuffer(static_cast<const VulkanRenderer&>(getRenderer()), mVkBuffer, mVkDeviceMemory);
		}

		/**
		*  @brief
		*    Return the Vulkan structured buffer
		*
		*  @return
		*    The Vulkan structured buffer
		*/
		inline VkBuffer getVkBuffer() const
		{
			return mVkBuffer;
		}

		/**
		*  @brief
		*    Return the Vulkan device memory
		*
		*  @return
		*    The Vulkan device memory
		*/
		inline VkDeviceMemory getVkDeviceMemory() const
		{
			return mVkDeviceMemory;
		}


	//[-------------------------------------------------------]
	//[ Public virtual Renderer::IResource methods            ]
	//[-------------------------------------------------------]
	public:
		#ifdef RENDERER_DEBUG
			virtual void setDebugName(const char* name) override
			{
				if (nullptr != vkDebugMarkerSetObjectNameEXT)
				{
					const VkDevice vkDevice = static_cast<const VulkanRenderer&>(getRenderer()).getVulkanContext().getVkDevice();
					Helper::setDebugObjectName(vkDevice, VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_EXT, (uint64_t)mVkBuffer, name);
					Helper::setDebugObjectName(vkDevice, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_MEMORY_EXT, (uint64_t)mVkDeviceMemory, name);
				}
			}
		#endif


	//[-------------------------------------------------------]
	//[ Protected virtual Renderer::RefCount methods          ]
	//[-------------------------------------------------------]
	protected:
		inline virtual void selfDestruct() override
		{
			RENDERER_DELETE(getRenderer().getContext(), StructuredBuffer, this);
		}


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		explicit StructuredBuffer(const StructuredBuffer& source) = delete;
		StructuredBuffer& operator =(const StructuredBuffer& source) = delete;


	//[-------------------------------------------------------]
	//[ Private data                                          ]
	//[-------------------------------------------------------]
	private:
		VkBuffer	   mVkBuffer;		///< Vulkan uniform texel buffer
		VkDeviceMemory mVkDeviceMemory;	///< Vulkan uniform texel memory


	};




	//[-------------------------------------------------------]
	//[ VulkanRenderer/IndirectBuffer.h                       ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Vulkan indirect buffer object interface
	*/
	class IndirectBuffer final : public Renderer::IIndirectBuffer
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] vulkanRenderer
		*    Owner Vulkan renderer instance
		*  @param[in] numberOfBytes
		*    Number of bytes within the indirect buffer, must be valid
		*  @param[in] data
		*    Indirect buffer data, can be a null pointer (empty buffer)
		*  @param[in] indirectBufferFlags
		*    Indirect buffer flags, see "Renderer::IndirectBufferFlag"
		*  @param[in] bufferUsage
		*    Indication of the buffer usage
		*/
		IndirectBuffer(VulkanRenderer& vulkanRenderer, uint32_t numberOfBytes, const void* data, uint32_t indirectBufferFlags, MAYBE_UNUSED Renderer::BufferUsage bufferUsage) :
			IIndirectBuffer(vulkanRenderer),
			mVkBuffer(VK_NULL_HANDLE),
			mVkDeviceMemory(VK_NULL_HANDLE)
		{
			// Sanity checks
			RENDERER_ASSERT(vulkanRenderer.getContext(), (indirectBufferFlags & Renderer::IndirectBufferFlag::DRAW_ARGUMENTS) != 0 || (indirectBufferFlags & Renderer::IndirectBufferFlag::DRAW_INDEXED_ARGUMENTS) != 0, "Invalid Vulkan flags, indirect buffer element type specification \"DRAW_ARGUMENTS\" or \"DRAW_INDEXED_ARGUMENTS\" is missing")
			RENDERER_ASSERT(vulkanRenderer.getContext(), !((indirectBufferFlags & Renderer::IndirectBufferFlag::DRAW_ARGUMENTS) != 0 && (indirectBufferFlags & Renderer::IndirectBufferFlag::DRAW_INDEXED_ARGUMENTS) != 0), "Invalid Vulkan flags, indirect buffer element type specification \"DRAW_ARGUMENTS\" or \"DRAW_INDEXED_ARGUMENTS\" must be set, but not both at one and the same time")
			RENDERER_ASSERT(vulkanRenderer.getContext(), (indirectBufferFlags & Renderer::IndirectBufferFlag::DRAW_ARGUMENTS) == 0 || (numberOfBytes % sizeof(Renderer::DrawArguments)) == 0, "Vulkan indirect buffer element type flags specification is \"DRAW_ARGUMENTS\" but the given number of bytes don't align to this")
			RENDERER_ASSERT(vulkanRenderer.getContext(), (indirectBufferFlags & Renderer::IndirectBufferFlag::DRAW_INDEXED_ARGUMENTS) == 0 || (numberOfBytes % sizeof(Renderer::DrawIndexedArguments)) == 0, "Vulkan indirect buffer element type flags specification is \"DRAW_INDEXED_ARGUMENTS\" but the given number of bytes don't align to this")

			// Create indirect buffer
			int vkBufferUsageFlagBits = VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT;
			if ((indirectBufferFlags & Renderer::IndirectBufferFlag::UNORDERED_ACCESS) != 0 || (indirectBufferFlags & Renderer::IndirectBufferFlag::SHADER_RESOURCE) != 0)
			{
				vkBufferUsageFlagBits |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
			}
			Helper::createAndAllocateVkBuffer(vulkanRenderer, static_cast<VkBufferUsageFlagBits>(vkBufferUsageFlagBits), VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, numberOfBytes, data, mVkBuffer, mVkDeviceMemory);
			SET_DEFAULT_DEBUG_NAME	// setDebugName("");
		}

		/**
		*  @brief
		*    Destructor
		*/
		inline virtual ~IndirectBuffer() override
		{
			Helper::destroyAndFreeVkBuffer(static_cast<const VulkanRenderer&>(getRenderer()), mVkBuffer, mVkDeviceMemory);
		}

		/**
		*  @brief
		*    Return the Vulkan indirect buffer
		*
		*  @return
		*    The Vulkan indirect buffer
		*/
		inline VkBuffer getVkBuffer() const
		{
			return mVkBuffer;
		}

		/**
		*  @brief
		*    Return the Vulkan device memory
		*
		*  @return
		*    The Vulkan device memory
		*/
		inline VkDeviceMemory getVkDeviceMemory() const
		{
			return mVkDeviceMemory;
		}


	//[-------------------------------------------------------]
	//[ Public virtual Renderer::IIndirectBuffer methods      ]
	//[-------------------------------------------------------]
	public:
		inline virtual const uint8_t* getEmulationData() const override
		{
			return nullptr;
		}


	//[-------------------------------------------------------]
	//[ Public virtual Renderer::IResource methods            ]
	//[-------------------------------------------------------]
	public:
		DEFINE_SET_DEBUG_NAME_VKBUFFER_VKDEVICEMEMORY("IndirectBufferObject", 23)


	//[-------------------------------------------------------]
	//[ Protected virtual Renderer::RefCount methods          ]
	//[-------------------------------------------------------]
	protected:
		inline virtual void selfDestruct() override
		{
			RENDERER_DELETE(getRenderer().getContext(), IndirectBuffer, this);
		}


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		explicit IndirectBuffer(const IndirectBuffer& source) = delete;
		IndirectBuffer& operator =(const IndirectBuffer& source) = delete;


	//[-------------------------------------------------------]
	//[ Private data                                          ]
	//[-------------------------------------------------------]
	private:
		VkBuffer	   mVkBuffer;		///< Vulkan indirect buffer
		VkDeviceMemory mVkDeviceMemory;	///< Vulkan indirect memory


	};




	//[-------------------------------------------------------]
	//[ VulkanRenderer/UniformBuffer.h                        ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Vulkan uniform buffer object (UBO, "constant buffer" in Direct3D terminology) interface
	*/
	class UniformBuffer final : public Renderer::IUniformBuffer
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] vulkanRenderer
		*    Owner Vulkan renderer instance
		*  @param[in] numberOfBytes
		*    Number of bytes within the uniform buffer, must be valid
		*  @param[in] data
		*    Uniform buffer data, can be a null pointer (empty buffer)
		*  @param[in] bufferUsage
		*    Indication of the buffer usage
		*/
		UniformBuffer(VulkanRenderer& vulkanRenderer, uint32_t numberOfBytes, const void* data, MAYBE_UNUSED Renderer::BufferUsage bufferUsage) :
			IUniformBuffer(vulkanRenderer),
			mVkBuffer(VK_NULL_HANDLE),
			mVkDeviceMemory(VK_NULL_HANDLE)
		{
			Helper::createAndAllocateVkBuffer(vulkanRenderer, static_cast<VkBufferUsageFlagBits>(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT), VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, numberOfBytes, data, mVkBuffer, mVkDeviceMemory);
			SET_DEFAULT_DEBUG_NAME	// setDebugName("");
		}

		/**
		*  @brief
		*    Destructor
		*/
		inline virtual ~UniformBuffer() override
		{
			Helper::destroyAndFreeVkBuffer(static_cast<const VulkanRenderer&>(getRenderer()), mVkBuffer, mVkDeviceMemory);
		}

		/**
		*  @brief
		*    Return the Vulkan uniform buffer
		*
		*  @return
		*    The Vulkan uniform buffer
		*/
		inline VkBuffer getVkBuffer() const
		{
			return mVkBuffer;
		}

		/**
		*  @brief
		*    Return the Vulkan device memory
		*
		*  @return
		*    The Vulkan device memory
		*/
		inline VkDeviceMemory getVkDeviceMemory() const
		{
			return mVkDeviceMemory;
		}


	//[-------------------------------------------------------]
	//[ Public virtual Renderer::IResource methods            ]
	//[-------------------------------------------------------]
	public:
		DEFINE_SET_DEBUG_NAME_VKBUFFER_VKDEVICEMEMORY("UBO", 6)


	//[-------------------------------------------------------]
	//[ Protected virtual Renderer::RefCount methods          ]
	//[-------------------------------------------------------]
	protected:
		inline virtual void selfDestruct() override
		{
			RENDERER_DELETE(getRenderer().getContext(), UniformBuffer, this);
		}


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		explicit UniformBuffer(const UniformBuffer& source) = delete;
		UniformBuffer& operator =(const UniformBuffer& source) = delete;


	//[-------------------------------------------------------]
	//[ Private data                                          ]
	//[-------------------------------------------------------]
	private:
		VkBuffer	   mVkBuffer;		///< Vulkan uniform buffer
		VkDeviceMemory mVkDeviceMemory;	///< Vulkan uniform memory


	};




	//[-------------------------------------------------------]
	//[ VulkanRenderer/BufferManager.h                        ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Vulkan buffer manager interface
	*/
	class BufferManager final : public Renderer::IBufferManager
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] vulkanRenderer
		*    Owner Vulkan renderer instance
		*/
		inline explicit BufferManager(VulkanRenderer& vulkanRenderer) :
			IBufferManager(vulkanRenderer)
		{}

		/**
		*  @brief
		*    Destructor
		*/
		inline virtual ~BufferManager() override
		{}


	//[-------------------------------------------------------]
	//[ Public virtual Renderer::IBufferManager methods       ]
	//[-------------------------------------------------------]
	public:
		inline virtual Renderer::IVertexBuffer* createVertexBuffer(uint32_t numberOfBytes, const void* data = nullptr, uint32_t bufferFlags = 0, Renderer::BufferUsage bufferUsage = Renderer::BufferUsage::STATIC_DRAW) override
		{
			return RENDERER_NEW(getRenderer().getContext(), VertexBuffer)(static_cast<VulkanRenderer&>(getRenderer()), numberOfBytes, data, bufferFlags, bufferUsage);
		}

		inline virtual Renderer::IIndexBuffer* createIndexBuffer(uint32_t numberOfBytes, const void* data = nullptr, uint32_t bufferFlags = 0, Renderer::BufferUsage bufferUsage = Renderer::BufferUsage::STATIC_DRAW, Renderer::IndexBufferFormat::Enum indexBufferFormat = Renderer::IndexBufferFormat::UNSIGNED_SHORT) override
		{
			return RENDERER_NEW(getRenderer().getContext(), IndexBuffer)(static_cast<VulkanRenderer&>(getRenderer()), numberOfBytes, data, bufferFlags, bufferUsage, indexBufferFormat);
		}

		inline virtual Renderer::IVertexArray* createVertexArray(const Renderer::VertexAttributes& vertexAttributes, uint32_t numberOfVertexBuffers, const Renderer::VertexArrayVertexBuffer* vertexBuffers, Renderer::IIndexBuffer* indexBuffer = nullptr) override
		{
			return RENDERER_NEW(getRenderer().getContext(), VertexArray)(static_cast<VulkanRenderer&>(getRenderer()), vertexAttributes, numberOfVertexBuffers, vertexBuffers, static_cast<IndexBuffer*>(indexBuffer));
		}

		inline virtual Renderer::ITextureBuffer* createTextureBuffer(uint32_t numberOfBytes, const void* data = nullptr, uint32_t bufferFlags = Renderer::BufferFlag::SHADER_RESOURCE, Renderer::BufferUsage bufferUsage = Renderer::BufferUsage::STATIC_DRAW, Renderer::TextureFormat::Enum textureFormat = Renderer::TextureFormat::R32G32B32A32F) override
		{
			return RENDERER_NEW(getRenderer().getContext(), TextureBuffer)(static_cast<VulkanRenderer&>(getRenderer()), numberOfBytes, data, bufferFlags, bufferUsage, textureFormat);
		}

		inline virtual Renderer::IStructuredBuffer* createStructuredBuffer(uint32_t numberOfBytes, const void* data, MAYBE_UNUSED uint32_t bufferFlags, Renderer::BufferUsage bufferUsage, uint32_t numberOfStructureBytes) override
		{
			return RENDERER_NEW(getRenderer().getContext(), StructuredBuffer)(static_cast<VulkanRenderer&>(getRenderer()), numberOfBytes, data, bufferUsage, numberOfStructureBytes);
		}

		inline virtual Renderer::IIndirectBuffer* createIndirectBuffer(uint32_t numberOfBytes, const void* data = nullptr, uint32_t indirectBufferFlags = 0, Renderer::BufferUsage bufferUsage = Renderer::BufferUsage::STATIC_DRAW) override
		{
			return RENDERER_NEW(getRenderer().getContext(), IndirectBuffer)(static_cast<VulkanRenderer&>(getRenderer()), numberOfBytes, data, indirectBufferFlags, bufferUsage);
		}

		inline virtual Renderer::IUniformBuffer* createUniformBuffer(uint32_t numberOfBytes, const void* data = nullptr, Renderer::BufferUsage bufferUsage = Renderer::BufferUsage::STATIC_DRAW) override
		{
			// Don't remove this reminder comment block: There are no buffer flags by intent since an uniform buffer can't be used for unordered access and as a consequence an uniform buffer must always used as shader resource to not be pointless
			// -> Inside GLSL "layout(binding = 0, std140) writeonly uniform OutputUniformBuffer" will result in the GLSL compiler error "Failed to parse the GLSL shader source code: ERROR: 0:85: 'assign' :  l-value required "anon@6" (can't modify a uniform)"
			// -> Inside GLSL "layout(binding = 0, std430) writeonly buffer  OutputUniformBuffer" will work in OpenGL but will fail in Vulkan with "Vulkan debug report callback: Object type: "VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT" Object: "0" Location: "0" Message code: "13" Layer prefix: "Validation" Message: "Object: VK_NULL_HANDLE (Type = 0) | Type mismatch on descriptor slot 0.0 (used as type `ptr to uniform struct of (vec4 of float32)`) but descriptor of type VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER""
			// RENDERER_ASSERT(getRenderer().getContext(), (bufferFlags & Renderer::BufferFlag::UNORDERED_ACCESS) == 0, "Invalid Vulkan buffer flags, uniform buffer can't be used for unordered access")
			// RENDERER_ASSERT(getRenderer().getContext(), (bufferFlags & Renderer::BufferFlag::SHADER_RESOURCE) != 0, "Invalid Vulkan buffer flags, uniform buffer must be used as shader resource")

			// Create the uniform buffer
			return RENDERER_NEW(getRenderer().getContext(), UniformBuffer)(static_cast<VulkanRenderer&>(getRenderer()), numberOfBytes, data, bufferUsage);
		}


	//[-------------------------------------------------------]
	//[ Protected virtual Renderer::RefCount methods          ]
	//[-------------------------------------------------------]
	protected:
		inline virtual void selfDestruct() override
		{
			RENDERER_DELETE(getRenderer().getContext(), BufferManager, this);
		}


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		explicit BufferManager(const BufferManager& source) = delete;
		BufferManager& operator =(const BufferManager& source) = delete;


	};




	//[-------------------------------------------------------]
	//[ VulkanRenderer/Texture/Texture1D.h                    ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Vulkan 1D texture interface
	*/
	class Texture1D final : public Renderer::ITexture1D
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] vulkanRenderer
		*    Owner Vulkan renderer instance
		*  @param[in] width
		*    Texture width, must be >0
		*  @param[in] textureFormat
		*    Texture format
		*  @param[in] data
		*    Texture data, can be a null pointer
		*  @param[in] textureFlags
		*    Texture flags, see "Renderer::TextureFlag::Enum"
		*/
		Texture1D(VulkanRenderer& vulkanRenderer, uint32_t width, Renderer::TextureFormat::Enum textureFormat, const void* data, uint32_t textureFlags) :
			ITexture1D(vulkanRenderer, width),
			mVkImage(VK_NULL_HANDLE),
			mVkImageLayout(Helper::getVkImageLayoutByTextureFlags(textureFlags)),
			mVkDeviceMemory(VK_NULL_HANDLE),
			mVkImageView(VK_NULL_HANDLE)
		{
			Helper::createAndFillVkImage(vulkanRenderer, VK_IMAGE_TYPE_1D, VK_IMAGE_VIEW_TYPE_1D, { width, 1, 1 }, textureFormat, data, textureFlags, 1, mVkImage, mVkDeviceMemory, mVkImageView);
			SET_DEFAULT_DEBUG_NAME	// setDebugName("");
		}

		/**
		*  @brief
		*    Destructor
		*/
		inline virtual ~Texture1D() override
		{
			Helper::destroyAndFreeVkImage(static_cast<VulkanRenderer&>(getRenderer()), mVkImage, mVkDeviceMemory, mVkImageView);
		}

		/**
		*  @brief
		*    Return the Vulkan image view
		*
		*  @return
		*    The Vulkan image view
		*/
		inline VkImageView getVkImageView() const
		{
			return mVkImageView;
		}

		/**
		*  @brief
		*    Return the Vulkan image layout
		*
		*  @return
		*    The Vulkan image layout
		*/
		inline VkImageLayout getVkImageLayout() const
		{
			return mVkImageLayout;
		}


	//[-------------------------------------------------------]
	//[ Public virtual Renderer::IResource methods            ]
	//[-------------------------------------------------------]
	public:
		DEFINE_SET_DEBUG_NAME_TEXTURE()


	//[-------------------------------------------------------]
	//[ Protected virtual Renderer::RefCount methods          ]
	//[-------------------------------------------------------]
	protected:
		inline virtual void selfDestruct() override
		{
			RENDERER_DELETE(getRenderer().getContext(), Texture1D, this);
		}


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		explicit Texture1D(const Texture1D& source) = delete;
		Texture1D& operator =(const Texture1D& source) = delete;


	//[-------------------------------------------------------]
	//[ Private data                                          ]
	//[-------------------------------------------------------]
	private:
		VkImage		   mVkImage;
		VkImageLayout  mVkImageLayout;
		VkDeviceMemory mVkDeviceMemory;
		VkImageView	   mVkImageView;


	};




	//[-------------------------------------------------------]
	//[ VulkanRenderer/Texture/Texture2D.h                    ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    OpenVR-support: Data required for passing Vulkan textures to IVRCompositor::Submit; Be sure to call OpenVR_Shutdown before destroying these resources
	*
	*  @note
	*    - From OpenVR SDK 1.0.7 "openvr.h"-header
	*/
	struct VRVulkanTextureData_t
	{
		VkImage			 m_nImage;
		VkDevice		 m_pDevice;
		VkPhysicalDevice m_pPhysicalDevice;
		VkInstance		 m_pInstance;
		VkQueue			 m_pQueue;
		uint32_t		 m_nQueueFamilyIndex;
		uint32_t		 m_nWidth;
		uint32_t		 m_nHeight;
		VkFormat		 m_nFormat;
		uint32_t		 m_nSampleCount;
	};

	/**
	*  @brief
	*    Vulkan 2D texture interface
	*/
	class Texture2D final : public Renderer::ITexture2D
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] vulkanRenderer
		*    Owner Vulkan renderer instance
		*  @param[in] width
		*    Texture width, must be >0
		*  @param[in] height
		*    Texture height, must be >0
		*  @param[in] textureFormat
		*    Texture format
		*  @param[in] data
		*    Texture data, can be a null pointer
		*  @param[in] textureFlags
		*    Texture flags, see "Renderer::TextureFlag::Enum"
		*  @param[in] numberOfMultisamples
		*    The number of multisamples per pixel (valid values: 1, 2, 4, 8)
		*/
		Texture2D(VulkanRenderer& vulkanRenderer, uint32_t width, uint32_t height, Renderer::TextureFormat::Enum textureFormat, const void* data, uint32_t textureFlags, uint8_t numberOfMultisamples) :
			ITexture2D(vulkanRenderer, width, height),
			mVrVulkanTextureData{},
			mVkImageLayout(Helper::getVkImageLayoutByTextureFlags(textureFlags)),
			mVkDeviceMemory(VK_NULL_HANDLE),
			mVkImageView(VK_NULL_HANDLE)
		{
			mVrVulkanTextureData.m_nFormat = Helper::createAndFillVkImage(vulkanRenderer, VK_IMAGE_TYPE_2D, VK_IMAGE_VIEW_TYPE_2D, { width, height, 1 }, textureFormat, data, textureFlags, numberOfMultisamples, mVrVulkanTextureData.m_nImage, mVkDeviceMemory, mVkImageView);
			SET_DEFAULT_DEBUG_NAME	// setDebugName("");

			// Fill the rest of the "VRVulkanTextureData_t"-structure
			const VulkanContext& vulkanContext = vulkanRenderer.getVulkanContext();
			const VulkanRuntimeLinking& vulkanRuntimeLinking = vulkanRenderer.getVulkanRuntimeLinking();
																									// m_nImage (VkImage) was set by "VulkanRenderer::Helper::createAndFillVkImage()" above
			mVrVulkanTextureData.m_pDevice			 = vulkanContext.getVkDevice();					// m_pDevice (VkDevice)
			mVrVulkanTextureData.m_pPhysicalDevice	 = vulkanContext.getVkPhysicalDevice();			// m_pPhysicalDevice (VkPhysicalDevice)
			mVrVulkanTextureData.m_pInstance		 = vulkanRuntimeLinking.getVkInstance();		// m_pInstance (VkInstance)
			mVrVulkanTextureData.m_pQueue			 = vulkanContext.getGraphicsVkQueue();			// m_pQueue (VkQueue)
			mVrVulkanTextureData.m_nQueueFamilyIndex = vulkanContext.getGraphicsQueueFamilyIndex();	// m_nQueueFamilyIndex (uint32_t)
			mVrVulkanTextureData.m_nWidth			 = width;										// m_nWidth (uint32_t)
			mVrVulkanTextureData.m_nHeight			 = height;										// m_nHeight (uint32_t)
																									// m_nFormat (VkFormat)  was set by "VulkanRenderer::Helper::createAndFillVkImage()" above
			mVrVulkanTextureData.m_nSampleCount		 = numberOfMultisamples;						// m_nSampleCount (uint32_t)
		}

		/**
		*  @brief
		*    Destructor
		*/
		inline virtual ~Texture2D() override
		{
			Helper::destroyAndFreeVkImage(static_cast<VulkanRenderer&>(getRenderer()), mVrVulkanTextureData.m_nImage, mVkDeviceMemory, mVkImageView);
		}

		/**
		*  @brief
		*    Return the Vulkan image view
		*
		*  @return
		*    The Vulkan image view
		*/
		inline VkImageView getVkImageView() const
		{
			return mVkImageView;
		}

		/**
		*  @brief
		*    Return the Vulkan image layout
		*
		*  @return
		*    The Vulkan image layout
		*/
		inline VkImageLayout getVkImageLayout() const
		{
			return mVkImageLayout;
		}

		/**
		*  @brief
		*    Return the Vulkan format
		*
		*  @return
		*    The Vulkan format
		*/
		inline VkFormat getVkFormat() const
		{
			return mVrVulkanTextureData.m_nFormat;
		}

		/**
		*  @brief
		*    Set minimum maximum mipmap index
		*
		*  @param[in] minimumMipmapIndex
		*    Minimum mipmap index, the most detailed mipmap, also known as base mipmap, 0 by default
		*  @param[in] maximumMipmapIndex
		*    Maximum mipmap index, the least detailed mipmap, <number of mipmaps> by default
		*/
		inline void setMinimumMaximumMipmapIndex(MAYBE_UNUSED uint32_t minimumMipmapIndex, MAYBE_UNUSED uint32_t maximumMipmapIndex)
		{
			// TODO(co) Implement me
		}


	//[-------------------------------------------------------]
	//[ Public virtual Renderer::IResource methods            ]
	//[-------------------------------------------------------]
	public:
		inline virtual void* getInternalResourceHandle() const override
		{
			return reinterpret_cast<void*>(const_cast<VRVulkanTextureData_t*>(&mVrVulkanTextureData));
		}


	//[-------------------------------------------------------]
	//[ Public virtual Renderer::IResource methods            ]
	//[-------------------------------------------------------]
	public:
		#ifdef RENDERER_DEBUG
			virtual void setDebugName(const char* name) override
			{
				if (nullptr != vkDebugMarkerSetObjectNameEXT)
				{
					const VkDevice vkDevice = static_cast<const VulkanRenderer&>(getRenderer()).getVulkanContext().getVkDevice();
					Helper::setDebugObjectName(vkDevice, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT, (uint64_t)mVrVulkanTextureData.m_nImage, name);
					Helper::setDebugObjectName(vkDevice, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_MEMORY_EXT, (uint64_t)mVkDeviceMemory, name);
					Helper::setDebugObjectName(vkDevice, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_VIEW_EXT, (uint64_t)mVkImageView, name);
				}
			}
		#endif


	//[-------------------------------------------------------]
	//[ Protected virtual Renderer::RefCount methods          ]
	//[-------------------------------------------------------]
	protected:
		inline virtual void selfDestruct() override
		{
			RENDERER_DELETE(getRenderer().getContext(), Texture2D, this);
		}


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		explicit Texture2D(const Texture2D& source) = delete;
		Texture2D& operator =(const Texture2D& source) = delete;


	//[-------------------------------------------------------]
	//[ Private data                                          ]
	//[-------------------------------------------------------]
	private:
		VRVulkanTextureData_t mVrVulkanTextureData;
		VkImageLayout		  mVkImageLayout;
		VkDeviceMemory		  mVkDeviceMemory;
		VkImageView			  mVkImageView;


	};




	//[-------------------------------------------------------]
	//[ VulkanRenderer/Texture/Texture2DArray.h               ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Vulkan 2D array texture interface
	*/
	class Texture2DArray final : public Renderer::ITexture2DArray
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] vulkanRenderer
		*    Owner Vulkan renderer instance
		*  @param[in] width
		*    Texture width, must be >0
		*  @param[in] height
		*    Texture height, must be >0
		*  @param[in] numberOfSlices
		*    Number of slices, must be >0
		*  @param[in] textureFormat
		*    Texture format
		*  @param[in] data
		*    Texture data, can be a null pointer
		*  @param[in] textureFlags
		*    Texture flags, see "Renderer::TextureFlag::Enum"
		*/
		Texture2DArray(VulkanRenderer& vulkanRenderer, uint32_t width, uint32_t height, uint32_t numberOfSlices, Renderer::TextureFormat::Enum textureFormat, const void* data, uint32_t textureFlags) :
			ITexture2DArray(vulkanRenderer, width, height, numberOfSlices),
			mVkImage(VK_NULL_HANDLE),
			mVkImageLayout(Helper::getVkImageLayoutByTextureFlags(textureFlags)),
			mVkDeviceMemory(VK_NULL_HANDLE),
			mVkImageView(VK_NULL_HANDLE),
			mVkFormat(Helper::createAndFillVkImage(vulkanRenderer, VK_IMAGE_TYPE_2D, VK_IMAGE_VIEW_TYPE_2D_ARRAY, { width, height, numberOfSlices }, textureFormat, data, textureFlags, 1, mVkImage, mVkDeviceMemory, mVkImageView))
		{
			SET_DEFAULT_DEBUG_NAME	// setDebugName("");
		}

		/**
		*  @brief
		*    Destructor
		*/
		inline virtual ~Texture2DArray() override
		{
			Helper::destroyAndFreeVkImage(static_cast<VulkanRenderer&>(getRenderer()), mVkImage, mVkDeviceMemory, mVkImageView);
		}

		/**
		*  @brief
		*    Return the Vulkan image view
		*
		*  @return
		*    The Vulkan image view
		*/
		inline VkImageView getVkImageView() const
		{
			return mVkImageView;
		}

		/**
		*  @brief
		*    Return the Vulkan image layout
		*
		*  @return
		*    The Vulkan image layout
		*/
		inline VkImageLayout getVkImageLayout() const
		{
			return mVkImageLayout;
		}

		/**
		*  @brief
		*    Return the Vulkan format
		*
		*  @return
		*    The Vulkan format
		*/
		inline VkFormat getVkFormat() const
		{
			return mVkFormat;
		}


	//[-------------------------------------------------------]
	//[ Public virtual Renderer::IResource methods            ]
	//[-------------------------------------------------------]
	public:
		DEFINE_SET_DEBUG_NAME_TEXTURE()


	//[-------------------------------------------------------]
	//[ Protected virtual Renderer::RefCount methods          ]
	//[-------------------------------------------------------]
	protected:
		inline virtual void selfDestruct() override
		{
			RENDERER_DELETE(getRenderer().getContext(), Texture2DArray, this);
		}


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		explicit Texture2DArray(const Texture2DArray& source) = delete;
		Texture2DArray& operator =(const Texture2DArray& source) = delete;


	//[-------------------------------------------------------]
	//[ Private data                                          ]
	//[-------------------------------------------------------]
	private:
		VkImage		   mVkImage;
		VkImageLayout  mVkImageLayout;
		VkDeviceMemory mVkDeviceMemory;
		VkImageView	   mVkImageView;
		VkFormat	   mVkFormat;


	};




	//[-------------------------------------------------------]
	//[ VulkanRenderer/Texture/Texture3D.h                    ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Vulkan 3D texture interface
	*/
	class Texture3D final : public Renderer::ITexture3D
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] vulkanRenderer
		*    Owner Vulkan renderer instance
		*  @param[in] width
		*    Texture width, must be >0
		*  @param[in] height
		*    Texture height, must be >0
		*  @param[in] depth
		*    Texture depth, must be >0
		*  @param[in] textureFormat
		*    Texture format
		*  @param[in] data
		*    Texture data, can be a null pointer
		*  @param[in] textureFlags
		*    Texture flags, see "Renderer::TextureFlag::Enum"
		*/
		Texture3D(VulkanRenderer& vulkanRenderer, uint32_t width, uint32_t height, uint32_t depth, Renderer::TextureFormat::Enum textureFormat, const void* data, uint32_t textureFlags) :
			ITexture3D(vulkanRenderer, width, height, depth),
			mVkImage(VK_NULL_HANDLE),
			mVkImageLayout(Helper::getVkImageLayoutByTextureFlags(textureFlags)),
			mVkDeviceMemory(VK_NULL_HANDLE),
			mVkImageView(VK_NULL_HANDLE)
		{
			Helper::createAndFillVkImage(vulkanRenderer, VK_IMAGE_TYPE_3D, VK_IMAGE_VIEW_TYPE_3D, { width, height, depth }, textureFormat, data, textureFlags, 1, mVkImage, mVkDeviceMemory, mVkImageView);
			SET_DEFAULT_DEBUG_NAME	// setDebugName("");
		}

		/**
		*  @brief
		*    Destructor
		*/
		inline virtual ~Texture3D() override
		{
			Helper::destroyAndFreeVkImage(static_cast<VulkanRenderer&>(getRenderer()), mVkImage, mVkDeviceMemory, mVkImageView);
		}

		/**
		*  @brief
		*    Return the Vulkan image view
		*
		*  @return
		*    The Vulkan image view
		*/
		inline VkImageView getVkImageView() const
		{
			return mVkImageView;
		}

		/**
		*  @brief
		*    Return the Vulkan image layout
		*
		*  @return
		*    The Vulkan image layout
		*/
		inline VkImageLayout getVkImageLayout() const
		{
			return mVkImageLayout;
		}


	//[-------------------------------------------------------]
	//[ Public virtual Renderer::IResource methods            ]
	//[-------------------------------------------------------]
	public:
		DEFINE_SET_DEBUG_NAME_TEXTURE()


	//[-------------------------------------------------------]
	//[ Protected virtual Renderer::RefCount methods          ]
	//[-------------------------------------------------------]
	protected:
		inline virtual void selfDestruct() override
		{
			RENDERER_DELETE(getRenderer().getContext(), Texture3D, this);
		}


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		explicit Texture3D(const Texture3D& source) = delete;
		Texture3D& operator =(const Texture3D& source) = delete;


	//[-------------------------------------------------------]
	//[ Private data                                          ]
	//[-------------------------------------------------------]
	private:
		VkImage		   mVkImage;
		VkImageLayout  mVkImageLayout;
		VkDeviceMemory mVkDeviceMemory;
		VkImageView	   mVkImageView;


	};




	//[-------------------------------------------------------]
	//[ VulkanRenderer/Texture/TextureCube.h                  ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Vulkan cube texture interface
	*/
	class TextureCube final : public Renderer::ITextureCube
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] vulkanRenderer
		*    Owner Vulkan renderer instance
		*  @param[in] width
		*    Texture width, must be >0
		*  @param[in] height
		*    Texture height, must be >0
		*  @param[in] textureFormat
		*    Texture format
		*  @param[in] data
		*    Texture data, can be a null pointer
		*  @param[in] textureFlags
		*    Texture flags, see "Renderer::TextureFlag::Enum"
		*/
		TextureCube(VulkanRenderer& vulkanRenderer, uint32_t width, uint32_t height, Renderer::TextureFormat::Enum textureFormat, const void* data, uint32_t textureFlags) :
			ITextureCube(vulkanRenderer, width, height),
			mVkImage(VK_NULL_HANDLE),
			mVkImageLayout(Helper::getVkImageLayoutByTextureFlags(textureFlags)),
			mVkDeviceMemory(VK_NULL_HANDLE),
			mVkImageView(VK_NULL_HANDLE)
		{
			Helper::createAndFillVkImage(vulkanRenderer, VK_IMAGE_TYPE_2D, VK_IMAGE_VIEW_TYPE_CUBE, { width, height, 6 }, textureFormat, data, textureFlags, 1, mVkImage, mVkDeviceMemory, mVkImageView);
			SET_DEFAULT_DEBUG_NAME	// setDebugName("");
		}

		/**
		*  @brief
		*    Destructor
		*/
		inline virtual ~TextureCube() override
		{
			Helper::destroyAndFreeVkImage(static_cast<VulkanRenderer&>(getRenderer()), mVkImage, mVkDeviceMemory, mVkImageView);
		}

		/**
		*  @brief
		*    Return the Vulkan image view
		*
		*  @return
		*    The Vulkan image view
		*/
		inline VkImageView getVkImageView() const
		{
			return mVkImageView;
		}

		/**
		*  @brief
		*    Return the Vulkan image layout
		*
		*  @return
		*    The Vulkan image layout
		*/
		inline VkImageLayout getVkImageLayout() const
		{
			return mVkImageLayout;
		}


	//[-------------------------------------------------------]
	//[ Public virtual Renderer::IResource methods            ]
	//[-------------------------------------------------------]
	public:
		DEFINE_SET_DEBUG_NAME_TEXTURE()


	//[-------------------------------------------------------]
	//[ Protected virtual Renderer::RefCount methods          ]
	//[-------------------------------------------------------]
	protected:
		inline virtual void selfDestruct() override
		{
			RENDERER_DELETE(getRenderer().getContext(), TextureCube, this);
		}


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		explicit TextureCube(const TextureCube& source) = delete;
		TextureCube& operator =(const TextureCube& source) = delete;


	//[-------------------------------------------------------]
	//[ Private data                                          ]
	//[-------------------------------------------------------]
	private:
		VkImage		   mVkImage;
		VkImageLayout  mVkImageLayout;
		VkDeviceMemory mVkDeviceMemory;
		VkImageView	   mVkImageView;


	};




	//[-------------------------------------------------------]
	//[ VulkanRenderer/Texture/TextureManager.h               ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Vulkan texture manager interface
	*/
	class TextureManager final : public Renderer::ITextureManager
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] vulkanRenderer
		*    Owner Vulkan renderer instance
		*/
		inline explicit TextureManager(VulkanRenderer& vulkanRenderer) :
			ITextureManager(vulkanRenderer)
		{}

		/**
		*  @brief
		*    Destructor
		*/
		inline virtual ~TextureManager() override
		{}


	//[-------------------------------------------------------]
	//[ Public virtual Renderer::ITextureManager methods      ]
	//[-------------------------------------------------------]
	public:
		virtual Renderer::ITexture1D* createTexture1D(uint32_t width, Renderer::TextureFormat::Enum textureFormat, const void* data = nullptr, uint32_t textureFlags = 0, MAYBE_UNUSED Renderer::TextureUsage textureUsage = Renderer::TextureUsage::DEFAULT) override
		{
			// The indication of the texture usage is only relevant for Direct3D, Vulkan has no texture usage indication

			// Check whether or not the given texture dimension is valid
			if (width > 0)
			{
				return RENDERER_NEW(getRenderer().getContext(), Texture1D)(static_cast<VulkanRenderer&>(getRenderer()), width, textureFormat, data, textureFlags);
			}
			else
			{
				return nullptr;
			}
		}

		virtual Renderer::ITexture2D* createTexture2D(uint32_t width, uint32_t height, Renderer::TextureFormat::Enum textureFormat, const void* data = nullptr, uint32_t textureFlags = 0, MAYBE_UNUSED Renderer::TextureUsage textureUsage = Renderer::TextureUsage::DEFAULT, uint8_t numberOfMultisamples = 1, MAYBE_UNUSED const Renderer::OptimizedTextureClearValue* optimizedTextureClearValue = nullptr) override
		{
			// The indication of the texture usage is only relevant for Direct3D, Vulkan has no texture usage indication

			// Check whether or not the given texture dimension is valid
			if (width > 0 && height > 0)
			{
				return RENDERER_NEW(getRenderer().getContext(), Texture2D)(static_cast<VulkanRenderer&>(getRenderer()), width, height, textureFormat, data, textureFlags, numberOfMultisamples);
			}
			else
			{
				return nullptr;
			}
		}

		virtual Renderer::ITexture2DArray* createTexture2DArray(uint32_t width, uint32_t height, uint32_t numberOfSlices, Renderer::TextureFormat::Enum textureFormat, const void* data = nullptr, uint32_t textureFlags = 0, MAYBE_UNUSED Renderer::TextureUsage textureUsage = Renderer::TextureUsage::DEFAULT) override
		{
			// The indication of the texture usage is only relevant for Direct3D, Vulkan has no texture usage indication

			// Check whether or not the given texture dimension is valid
			if (width > 0 && height > 0 && numberOfSlices > 0)
			{
				return RENDERER_NEW(getRenderer().getContext(), Texture2DArray)(static_cast<VulkanRenderer&>(getRenderer()), width, height, numberOfSlices, textureFormat, data, textureFlags);
			}
			else
			{
				return nullptr;
			}
		}

		virtual Renderer::ITexture3D* createTexture3D(uint32_t width, uint32_t height, uint32_t depth, Renderer::TextureFormat::Enum textureFormat, const void* data = nullptr, uint32_t textureFlags = 0, MAYBE_UNUSED Renderer::TextureUsage textureUsage = Renderer::TextureUsage::DEFAULT) override
		{
			// The indication of the texture usage is only relevant for Direct3D, Vulkan has no texture usage indication

			// Check whether or not the given texture dimension is valid
			if (width > 0 && height > 0 && depth > 0)
			{
				return RENDERER_NEW(getRenderer().getContext(), Texture3D)(static_cast<VulkanRenderer&>(getRenderer()), width, height, depth, textureFormat, data, textureFlags);
			}
			else
			{
				return nullptr;
			}
		}

		virtual Renderer::ITextureCube* createTextureCube(uint32_t width, uint32_t height, Renderer::TextureFormat::Enum textureFormat, const void* data = nullptr, uint32_t textureFlags = 0, MAYBE_UNUSED Renderer::TextureUsage textureUsage = Renderer::TextureUsage::DEFAULT) override
		{
			// The indication of the texture usage is only relevant for Direct3D, Vulkan has no texture usage indication

			// Check whether or not the given texture dimension is valid
			if (width > 0 && height > 0)
			{
				return RENDERER_NEW(getRenderer().getContext(), TextureCube)(static_cast<VulkanRenderer&>(getRenderer()), width, height, textureFormat, data, textureFlags);
			}
			else
			{
				return nullptr;
			}
		}


	//[-------------------------------------------------------]
	//[ Protected virtual Renderer::RefCount methods          ]
	//[-------------------------------------------------------]
	protected:
		inline virtual void selfDestruct() override
		{
			RENDERER_DELETE(getRenderer().getContext(), TextureManager, this);
		}


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		explicit TextureManager(const TextureManager& source) = delete;
		TextureManager& operator =(const TextureManager& source) = delete;


	};




	//[-------------------------------------------------------]
	//[ VulkanRenderer/State/SamplerState.h                   ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Vulkan sampler state interface
	*/
	class SamplerState final : public Renderer::ISamplerState
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] vulkanRenderer
		*    Owner Vulkan renderer instance
		*  @param[in] samplerState
		*    Sampler state to use
		*/
		SamplerState(VulkanRenderer& vulkanRenderer, const Renderer::SamplerState& samplerState) :
			ISamplerState(vulkanRenderer),
			mVkSampler(VK_NULL_HANDLE)
		{
			// Sanity checks
			RENDERER_ASSERT(vulkanRenderer.getContext(), samplerState.filter != Renderer::FilterMode::UNKNOWN, "Vulkan filter mode must not be unknown")
			RENDERER_ASSERT(vulkanRenderer.getContext(), samplerState.maxAnisotropy <= vulkanRenderer.getCapabilities().maximumAnisotropy, "Maximum Vulkan anisotropy value violated")

			// TODO(co) Map "Renderer::SamplerState" to VkSamplerCreateInfo
			const bool anisotropyEnable = (Renderer::FilterMode::ANISOTROPIC == samplerState.filter || Renderer::FilterMode::COMPARISON_ANISOTROPIC == samplerState.filter);
			const VkSamplerCreateInfo vkSamplerCreateInfo =
			{
				VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,												// sType (VkStructureType)
				nullptr,																			// pNext (const void*)
				0,																					// flags (VkSamplerCreateFlags)
				Mapping::getVulkanMagFilterMode(vulkanRenderer.getContext(), samplerState.filter),	// magFilter (VkFilter)
				Mapping::getVulkanMinFilterMode(vulkanRenderer.getContext(), samplerState.filter),	// minFilter (VkFilter)
				Mapping::getVulkanMipmapMode(vulkanRenderer.getContext(), samplerState.filter),		// mipmapMode (VkSamplerMipmapMode)
				Mapping::getVulkanTextureAddressMode(samplerState.addressU),						// addressModeU (VkSamplerAddressMode)
				Mapping::getVulkanTextureAddressMode(samplerState.addressV),						// addressModeV (VkSamplerAddressMode)
				Mapping::getVulkanTextureAddressMode(samplerState.addressW),						// addressModeW (VkSamplerAddressMode)
				samplerState.mipLODBias,															// mipLodBias (float)
				static_cast<VkBool32>(anisotropyEnable),											// anisotropyEnable (VkBool32)
				static_cast<float>(samplerState.maxAnisotropy),										// maxAnisotropy (float)
				VK_FALSE,																			// compareEnable (VkBool32)
				VK_COMPARE_OP_ALWAYS,																// compareOp (VkCompareOp)
				samplerState.minLOD,																// minLod (float)
				samplerState.maxLOD,																// maxLod (float)
				VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK,											// borderColor (VkBorderColor)
				VK_FALSE																			// unnormalizedCoordinates (VkBool32)
			};
			if (vkCreateSampler(vulkanRenderer.getVulkanContext().getVkDevice(), &vkSamplerCreateInfo, vulkanRenderer.getVkAllocationCallbacks(), &mVkSampler) != VK_SUCCESS)
			{
				RENDERER_LOG(vulkanRenderer.getContext(), CRITICAL, "Failed to create Vulkan sampler instance")
			}
			else
			{
				SET_DEFAULT_DEBUG_NAME	// setDebugName("");
			}
		}

		/**
		*  @brief
		*    Destructor
		*/
		virtual ~SamplerState() override
		{
			if (VK_NULL_HANDLE != mVkSampler)
			{
				const VulkanRenderer& vulkanRenderer = static_cast<VulkanRenderer&>(getRenderer());
				vkDestroySampler(vulkanRenderer.getVulkanContext().getVkDevice(), mVkSampler, vulkanRenderer.getVkAllocationCallbacks());
			}
		}

		/**
		*  @brief
		*    Return the Vulkan sampler
		*
		*  @return
		*    The Vulkan sampler
		*/
		inline VkSampler getVkSampler() const
		{
			return mVkSampler;
		}


	//[-------------------------------------------------------]
	//[ Public virtual Renderer::IResource methods            ]
	//[-------------------------------------------------------]
	public:
		#ifdef RENDERER_DEBUG
			virtual void setDebugName(const char* name) override
			{
				if (nullptr != vkDebugMarkerSetObjectNameEXT)
				{
					Helper::setDebugObjectName(static_cast<const VulkanRenderer&>(getRenderer()).getVulkanContext().getVkDevice(), VK_DEBUG_REPORT_OBJECT_TYPE_SAMPLER_EXT, (uint64_t)mVkSampler, name);
				}
			}
		#endif


	//[-------------------------------------------------------]
	//[ Protected virtual Renderer::RefCount methods          ]
	//[-------------------------------------------------------]
	protected:
		inline virtual void selfDestruct() override
		{
			RENDERER_DELETE(getRenderer().getContext(), SamplerState, this);
		}


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		explicit SamplerState(const SamplerState& source) = delete;
		SamplerState& operator =(const SamplerState& source) = delete;


	//[-------------------------------------------------------]
	//[ Private data                                          ]
	//[-------------------------------------------------------]
	private:
		VkSampler mVkSampler;	///< Vulkan sampler instance, "VK_NULL_HANDLE" in case of error


	};




	//[-------------------------------------------------------]
	//[ VulkanRenderer/RenderTarget/RenderPass.h              ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Vulkan render pass interface
	*/
	class RenderPass final : public Renderer::IRenderPass
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] vulkanRenderer
		*    Owner Vulkan renderer instance
		*  @param[in] numberOfColorAttachments
		*    Number of color render target textures, must be <="Renderer::Capabilities::maximumNumberOfSimultaneousRenderTargets"
		*  @param[in] colorAttachmentTextureFormats
		*    The color render target texture formats, can be a null pointer or can contain null pointers, if not a null pointer there must be at
		*    least "numberOfColorAttachments" textures in the provided C-array of pointers
		*  @param[in] depthStencilAttachmentTextureFormat
		*    The optional depth stencil render target texture format, can be a "Renderer::TextureFormat::UNKNOWN" if there should be no depth buffer
		*  @param[in] numberOfMultisamples
		*    The number of multisamples per pixel (valid values: 1, 2, 4, 8)
		*/
		RenderPass(VulkanRenderer& vulkanRenderer, uint32_t numberOfColorAttachments, const Renderer::TextureFormat::Enum* colorAttachmentTextureFormats, Renderer::TextureFormat::Enum depthStencilAttachmentTextureFormat, uint8_t numberOfMultisamples) :
			IRenderPass(vulkanRenderer),
			mVkRenderPass(VK_NULL_HANDLE),
			mNumberOfColorAttachments(numberOfColorAttachments),
			mDepthStencilAttachmentTextureFormat(depthStencilAttachmentTextureFormat),
			mVkSampleCountFlagBits(Mapping::getVulkanSampleCountFlagBits(vulkanRenderer.getContext(), numberOfMultisamples))
		{
			const bool hasDepthStencilAttachment = (Renderer::TextureFormat::Enum::UNKNOWN != depthStencilAttachmentTextureFormat);

			// Vulkan attachment descriptions
			std::vector<VkAttachmentDescription> vkAttachmentDescriptions;
			vkAttachmentDescriptions.resize(mNumberOfColorAttachments + (hasDepthStencilAttachment ? 1u : 0u));
			uint32_t currentVkAttachmentDescriptionIndex = 0;

			// Handle color attachments
			typedef std::vector<VkAttachmentReference> VkAttachmentReferences;
			VkAttachmentReferences colorVkAttachmentReferences;
			if (mNumberOfColorAttachments > 0)
			{
				colorVkAttachmentReferences.resize(mNumberOfColorAttachments);
				for (uint32_t i = 0; i < mNumberOfColorAttachments; ++i)
				{
					{ // Setup Vulkan color attachment references
						VkAttachmentReference& vkAttachmentReference = colorVkAttachmentReferences[currentVkAttachmentDescriptionIndex];
						vkAttachmentReference.attachment = currentVkAttachmentDescriptionIndex;			// attachment (uint32_t)
						vkAttachmentReference.layout	 = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;	// layout (VkImageLayout)
					}

					{ // Setup Vulkan color attachment description
						VkAttachmentDescription& vkAttachmentDescription = vkAttachmentDescriptions[currentVkAttachmentDescriptionIndex];
						vkAttachmentDescription.flags		   = 0;																// flags (VkAttachmentDescriptionFlags)
						vkAttachmentDescription.format		   = Mapping::getVulkanFormat(colorAttachmentTextureFormats[i]);	// format (VkFormat)
						vkAttachmentDescription.samples		   = mVkSampleCountFlagBits;										// samples (VkSampleCountFlagBits)
						vkAttachmentDescription.loadOp		   = VK_ATTACHMENT_LOAD_OP_CLEAR;									// loadOp (VkAttachmentLoadOp)
						vkAttachmentDescription.storeOp		   = VK_ATTACHMENT_STORE_OP_STORE;									// storeOp (VkAttachmentStoreOp)
						vkAttachmentDescription.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;								// stencilLoadOp (VkAttachmentLoadOp)
						vkAttachmentDescription.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;								// stencilStoreOp (VkAttachmentStoreOp)
						vkAttachmentDescription.initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;										// initialLayout (VkImageLayout)
						vkAttachmentDescription.finalLayout	   = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;						// finalLayout (VkImageLayout)
					}

					// Advance current Vulkan attachment description index
					++currentVkAttachmentDescriptionIndex;
				}
			}

			// Handle depth stencil attachments
			const VkAttachmentReference depthVkAttachmentReference =
			{
				currentVkAttachmentDescriptionIndex,				// attachment (uint32_t)
				VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL	// layout (VkImageLayout)
			};
			if (hasDepthStencilAttachment)
			{
				// Setup Vulkan depth attachment description
				VkAttachmentDescription& vkAttachmentDescription = vkAttachmentDescriptions[currentVkAttachmentDescriptionIndex];
				vkAttachmentDescription.flags		   = 0;																// flags (VkAttachmentDescriptionFlags)
				vkAttachmentDescription.format		   = Mapping::getVulkanFormat(depthStencilAttachmentTextureFormat);	// format (VkFormat)
				vkAttachmentDescription.samples		   = mVkSampleCountFlagBits;										// samples (VkSampleCountFlagBits)
				vkAttachmentDescription.loadOp		   = VK_ATTACHMENT_LOAD_OP_CLEAR;									// loadOp (VkAttachmentLoadOp)
				vkAttachmentDescription.storeOp		   = VK_ATTACHMENT_STORE_OP_STORE;									// storeOp (VkAttachmentStoreOp)
				vkAttachmentDescription.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;								// stencilLoadOp (VkAttachmentLoadOp)
				vkAttachmentDescription.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;								// stencilStoreOp (VkAttachmentStoreOp)
				vkAttachmentDescription.initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;										// initialLayout (VkImageLayout)
				vkAttachmentDescription.finalLayout	   = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;				// finalLayout (VkImageLayout)
				// ++currentVkAttachmentDescriptionIndex;	// Not needed since we're the last
			}

			// Create Vulkan create render pass
			const VkSubpassDescription vkSubpassDescription =
			{
				0,																				// flags (VkSubpassDescriptionFlags)
				VK_PIPELINE_BIND_POINT_GRAPHICS,												// pipelineBindPoint (VkPipelineBindPoint)
				0,																				// inputAttachmentCount (uint32_t)
				nullptr,																		// pInputAttachments (const VkAttachmentReference*)
				mNumberOfColorAttachments,														// colorAttachmentCount (uint32_t)
				(mNumberOfColorAttachments > 0) ? colorVkAttachmentReferences.data() : nullptr,	// pColorAttachments (const VkAttachmentReference*)
				nullptr,																		// pResolveAttachments (const VkAttachmentReference*)
				hasDepthStencilAttachment ? &depthVkAttachmentReference : nullptr,				// pDepthStencilAttachment (const VkAttachmentReference*)
				0,																				// preserveAttachmentCount (uint32_t)
				nullptr																			// pPreserveAttachments (const uint32_t*)
			};
			static constexpr std::array<VkSubpassDependency, 2> vkSubpassDependencies =
			{{
				{
					VK_SUBPASS_EXTERNAL,														// srcSubpass (uint32_t)
					0,																			// dstSubpass (uint32_t)
					VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,										// srcStageMask (VkPipelineStageFlags)
					VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,								// dstStageMask (VkPipelineStageFlags)
					VK_ACCESS_MEMORY_READ_BIT,													// srcAccessMask (VkAccessFlags)
					VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,	// dstAccessMask (VkAccessFlags)
					VK_DEPENDENCY_BY_REGION_BIT													// dependencyFlags (VkDependencyFlags)
				},
				{
					0,																			// srcSubpass (uint32_t)
					VK_SUBPASS_EXTERNAL,														// dstSubpass (uint32_t)
					VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,								// srcStageMask (VkPipelineStageFlags)
					VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,										// dstStageMask (VkPipelineStageFlags)
					VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,	// srcAccessMask (VkAccessFlags)
					VK_ACCESS_MEMORY_READ_BIT,													// dstAccessMask (VkAccessFlags)
					VK_DEPENDENCY_BY_REGION_BIT													// dependencyFlags (VkDependencyFlags)
				}
			}};
			const VkRenderPassCreateInfo vkRenderPassCreateInfo =
			{
				VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,				// sType (VkStructureType)
				nullptr,												// pNext (const void*)
				0,														// flags (VkRenderPassCreateFlags)
				static_cast<uint32_t>(vkAttachmentDescriptions.size()),	// attachmentCount (uint32_t)
				vkAttachmentDescriptions.data(),						// pAttachments (const VkAttachmentDescription*)
				1,														// subpassCount (uint32_t)
				&vkSubpassDescription,									// pSubpasses (const VkSubpassDescription*)
				static_cast<uint32_t>(vkSubpassDependencies.size()),	// dependencyCount (uint32_t)
				vkSubpassDependencies.data()							// pDependencies (const VkSubpassDependency*)
			};
			const VkDevice vkDevice = vulkanRenderer.getVulkanContext().getVkDevice();
			const Renderer::Context& context = vulkanRenderer.getContext();
			if (vkCreateRenderPass(vkDevice, &vkRenderPassCreateInfo, vulkanRenderer.getVkAllocationCallbacks(), &mVkRenderPass) != VK_SUCCESS)
			{
				RENDERER_LOG(context, CRITICAL, "Failed to create Vulkan render pass")
			}
		}

		/**
		*  @brief
		*    Destructor
		*/
		virtual ~RenderPass() override
		{
			// Destroy Vulkan render pass instance
			if (VK_NULL_HANDLE != mVkRenderPass)
			{
				const VulkanRenderer& vulkanRenderer = static_cast<VulkanRenderer&>(getRenderer());
				vkDestroyRenderPass(vulkanRenderer.getVulkanContext().getVkDevice(), mVkRenderPass, vulkanRenderer.getVkAllocationCallbacks());
			}
		}

		/**
		*  @brief
		*    Return the Vulkan render pass
		*
		*  @return
		*    The Vulkan render pass
		*/
		inline VkRenderPass getVkRenderPass() const
		{
			return mVkRenderPass;
		}

		/**
		*  @brief
		*    Return the number of color render target textures
		*
		*  @return
		*    The number of color render target textures
		*/
		inline uint32_t getNumberOfColorAttachments() const
		{
			return mNumberOfColorAttachments;
		}

		/**
		*  @brief
		*    Return the number of render target textures (color and depth stencil)
		*
		*  @return
		*    The number of render target textures (color and depth stencil)
		*/
		inline uint32_t getNumberOfAttachments() const
		{
			return (mDepthStencilAttachmentTextureFormat != Renderer::TextureFormat::Enum::UNKNOWN) ? (mNumberOfColorAttachments + 1) : mNumberOfColorAttachments;
		}

		/**
		*  @brief
		*    Return the depth stencil attachment texture format
		*
		*  @return
		*    The depth stencil attachment texture format
		*/
		inline Renderer::TextureFormat::Enum getDepthStencilAttachmentTextureFormat() const
		{
			return mDepthStencilAttachmentTextureFormat;
		}

		/**
		*  @brief
		*    Return the Vulkan sample count flag bits
		*
		*  @return
		*    The Vulkan sample count flag bits
		*/
		inline VkSampleCountFlagBits getVkSampleCountFlagBits() const
		{
			return mVkSampleCountFlagBits;
		}


	//[-------------------------------------------------------]
	//[ Protected virtual Renderer::RefCount methods          ]
	//[-------------------------------------------------------]
	protected:
		inline virtual void selfDestruct() override
		{
			RENDERER_DELETE(getRenderer().getContext(), RenderPass, this);
		}


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		explicit RenderPass(const RenderPass& source) = delete;
		RenderPass& operator =(const RenderPass& source) = delete;


	//[-------------------------------------------------------]
	//[ Private data                                          ]
	//[-------------------------------------------------------]
	private:
		VkRenderPass				  mVkRenderPass;						///< Vulkan render pass instance, can be a null handle
		uint32_t					  mNumberOfColorAttachments;			///< Number of color render target textures
		Renderer::TextureFormat::Enum mDepthStencilAttachmentTextureFormat;	///< The depth stencil attachment texture format
		VkSampleCountFlagBits		  mVkSampleCountFlagBits;				///< Vulkan sample count flag bits


	};




	//[-------------------------------------------------------]
	//[ VulkanRenderer/RenderTarget/SwapChain.h               ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Vulkan swap chain class
	*
	*  @todo
	*    - TODO(co) Add support for debug name (not that important while at the same time more complex to implement here, but lets keep the TODO here to know there's room for improvement)
	*/
	class SwapChain final : public Renderer::ISwapChain
	{


	//[-------------------------------------------------------]
	//[ Public static methods                                 ]
	//[-------------------------------------------------------]
	public:
		static VkFormat findColorVkFormat(const Renderer::Context& context, VkInstance vkInstance, const VulkanContext& vulkanContext)
		{
			const VkPhysicalDevice vkPhysicalDevice = vulkanContext.getVkPhysicalDevice();
			const VkSurfaceKHR vkSurfaceKHR = detail::createPresentationSurface(context, vulkanContext.getVulkanRenderer().getVkAllocationCallbacks(), vkInstance, vkPhysicalDevice, vulkanContext.getGraphicsQueueFamilyIndex(), Renderer::WindowHandle{context.getNativeWindowHandle(), nullptr, nullptr});
			const VkSurfaceFormatKHR desiredVkSurfaceFormatKHR = ::detail::getSwapChainFormat(context, vkPhysicalDevice, vkSurfaceKHR);
			vkDestroySurfaceKHR(vkInstance, vkSurfaceKHR, vulkanContext.getVulkanRenderer().getVkAllocationCallbacks());
			return desiredVkSurfaceFormatKHR.format;
		}

		inline static VkFormat findDepthVkFormat(VkPhysicalDevice vkPhysicalDevice)
		{
			return detail::findSupportedVkFormat(vkPhysicalDevice, { VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT }, VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
		}


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] renderPass
		*    Render pass to use, the swap chain keeps a reference to the render pass
		*  @param[in] windowHandle
		*    Information about the window to render into
		*/
		SwapChain(Renderer::IRenderPass& renderPass, Renderer::WindowHandle windowHandle) :
			ISwapChain(renderPass),
			// Operation system window
			mNativeWindowHandle(windowHandle.nativeWindowHandle),
			mRenderWindow(windowHandle.renderWindow),
			// Vulkan presentation surface
			mVkSurfaceKHR(VK_NULL_HANDLE),
			// Vulkan swap chain and color render target related
			mVkSwapchainKHR(VK_NULL_HANDLE),
			mVkRenderPass(VK_NULL_HANDLE),
			mImageAvailableVkSemaphore(VK_NULL_HANDLE),
			mRenderingFinishedVkSemaphore(VK_NULL_HANDLE),
			mCurrentImageIndex(~0u),
			// Depth render target related
			mDepthVkFormat(Mapping::getVulkanFormat(static_cast<RenderPass&>(renderPass).getDepthStencilAttachmentTextureFormat())),
			mDepthVkImage(VK_NULL_HANDLE),
			mDepthVkDeviceMemory(VK_NULL_HANDLE),
			mDepthVkImageView(VK_NULL_HANDLE)
		{
			// Create the Vulkan presentation surface instance depending on the operation system
			VulkanRenderer& vulkanRenderer = static_cast<VulkanRenderer&>(renderPass.getRenderer());
			const VulkanContext&   vulkanContext	= vulkanRenderer.getVulkanContext();
			const VkInstance	   vkInstance		= vulkanRenderer.getVulkanRuntimeLinking().getVkInstance();
			const VkPhysicalDevice vkPhysicalDevice	= vulkanContext.getVkPhysicalDevice();
			mVkSurfaceKHR = detail::createPresentationSurface(vulkanRenderer.getContext(), vulkanRenderer.getVkAllocationCallbacks(), vkInstance, vkPhysicalDevice, vulkanContext.getGraphicsQueueFamilyIndex(), windowHandle);
			if (VK_NULL_HANDLE != mVkSurfaceKHR)
			{
				// Create the Vulkan swap chain
				createVulkanSwapChain();
			}
			else
			{
				// Error!
				RENDERER_LOG(vulkanRenderer.getContext(), CRITICAL, "The swap chain failed to create the Vulkan presentation surface")
			}
		}

		/**
		*  @brief
		*    Destructor
		*/
		virtual ~SwapChain() override
		{
			if (VK_NULL_HANDLE != mVkSurfaceKHR)
			{
				destroyVulkanSwapChain();
				const VulkanRenderer& vulkanRenderer = static_cast<VulkanRenderer&>(getRenderer());
				vkDestroySurfaceKHR(vulkanRenderer.getVulkanRuntimeLinking().getVkInstance(), mVkSurfaceKHR, vulkanRenderer.getVkAllocationCallbacks());
			}
		}

		/**
		*  @brief
		*    Return the Vulkan render pass
		*
		*  @return
		*    The Vulkan render pass
		*/
		inline VkRenderPass getVkRenderPass() const
		{
			return mVkRenderPass;
		}

		/**
		*  @brief
		*    Return the current Vulkan image to render color into
		*
		*  @return
		*    The current Vulkan image to render color into
		*/
		inline VkImage getColorCurrentVkImage() const
		{
			RENDERER_ASSERT(getRenderer().getContext(), ~0u != mCurrentImageIndex, "Invalid index of the current Vulkan swap chain image to render into (Vulkan swap chain creation failed?)")
			RENDERER_ASSERT(getRenderer().getContext(), mCurrentImageIndex < mSwapChainBuffer.size(), "Out-of-bounds index of the current Vulkan swap chain image to render into")
			return mSwapChainBuffer[mCurrentImageIndex].vkImage;
		}

		/**
		*  @brief
		*    Return the Vulkan image to render depth into
		*
		*  @return
		*    The Vulkan image to render depth into
		*/
		inline VkImage getDepthVkImage() const
		{
			return mDepthVkImage;
		}

		/**
		*  @brief
		*    Return the current Vulkan framebuffer to render into
		*
		*  @return
		*    The current Vulkan framebuffer to render into
		*/
		inline VkFramebuffer getCurrentVkFramebuffer() const
		{
			RENDERER_ASSERT(getRenderer().getContext(), ~0u != mCurrentImageIndex, "Invalid index of the current Vulkan swap chain image to render into (Vulkan swap chain creation failed?)")
			RENDERER_ASSERT(getRenderer().getContext(), mCurrentImageIndex < mSwapChainBuffer.size(), "Out-of-bounds index of the current Vulkan swap chain image to render into")
			return mSwapChainBuffer[mCurrentImageIndex].vkFramebuffer;
		}


	//[-------------------------------------------------------]
	//[ Public virtual Renderer::IRenderTarget methods        ]
	//[-------------------------------------------------------]
	public:
		virtual void getWidthAndHeight(uint32_t& width, uint32_t& height) const override
		{
			// Return stored width and height when both valid
			if (nullptr != mRenderWindow)
			{
				mRenderWindow->getWidthAndHeight(width, height);
				return;
			}
			#ifdef _WIN32
				// Is there a valid native OS window?
				if (NULL_HANDLE != mNativeWindowHandle)
				{
					// Get the width and height
					long swapChainWidth  = 1;
					long swapChainHeight = 1;
					{
						// Get the client rectangle of the native output window
						// -> Don't use the width and height stored in "DXGI_SWAP_CHAIN_DESC" -> "DXGI_MODE_DESC"
						//    because it might have been modified in order to avoid zero values
						RECT rect;
						::GetClientRect(reinterpret_cast<HWND>(mNativeWindowHandle), &rect);

						// Get the width and height...
						swapChainWidth  = rect.right  - rect.left;
						swapChainHeight = rect.bottom - rect.top;

						// ... and ensure that none of them is ever zero
						if (swapChainWidth < 1)
						{
							swapChainWidth = 1;
						}
						if (swapChainHeight < 1)
						{
							swapChainHeight = 1;
						}
					}

					// Write out the width and height
					width  = static_cast<UINT>(swapChainWidth);
					height = static_cast<UINT>(swapChainHeight);
				}
				else
			#elif defined(__ANDROID__)
				if (NULL_HANDLE != mNativeWindowHandle)
				{
					// TODO(co) Get size on Android
					width = height = 1;
				}
				else
			#elif defined LINUX
				if (NULL_HANDLE != mNativeWindowHandle)
				{
					VulkanRenderer& vulkanRenderer = static_cast<VulkanRenderer&>(getRenderer());
					const Renderer::Context& context = vulkanRenderer.getContext();
					RENDERER_ASSERT(context, context.getType() == Renderer::Context::ContextType::X11, "Invalid Vulkan context type")

					// If the given renderer context is an X11 context use the display connection object provided by the context
					if (context.getType() == Renderer::Context::ContextType::X11)
					{
						const Renderer::X11Context& x11Context = static_cast<const Renderer::X11Context&>(context);
						Display* display = x11Context.getDisplay();

						// Get the width and height...
						::Window rootWindow = 0;
						int positionX = 0, positionY = 0;
						unsigned int unsignedWidth = 0, unsignedHeight = 0, border = 0, depth = 0;
						if (nullptr != display)
						{
							XGetGeometry(display, mNativeWindowHandle, &rootWindow, &positionX, &positionY, &unsignedWidth, &unsignedHeight, &border, &depth);
						}

						// ... and ensure that none of them is ever zero
						if (unsignedWidth < 1)
						{
							unsignedWidth = 1;
						}
						if (unsignedHeight < 1)
						{
							unsignedHeight = 1;
						}

						// Done
						width = unsignedWidth;
						height = unsignedHeight;
					}
				}
				else
			#else
				#error "Unsupported platform"
			#endif
			{
				// Set known default return values
				width  = 1;
				height = 1;
			}
		}


	//[-------------------------------------------------------]
	//[ Public virtual Renderer::ISwapChain methods           ]
	//[-------------------------------------------------------]
	public:
		inline virtual Renderer::handle getNativeWindowHandle() const override
		{
			return mNativeWindowHandle;
		}

		inline virtual void setVerticalSynchronizationInterval(uint32_t) override
		{
			// TODO(co) Implement usage of "synchronizationInterval"
		}

		virtual void present() override
		{
			// TODO(co) "Renderer::IRenderWindow::present()" support
			/*
			if (nullptr != mRenderWindow)
			{
				mRenderWindow->present();
				return;
			}
			*/

			// Get the Vulkan context
			const VulkanRenderer& vulkanRenderer = static_cast<VulkanRenderer&>(getRenderer());
			const VulkanContext& vulkanContext = vulkanRenderer.getVulkanContext();

			{ // Queue submit
				const VkPipelineStageFlags waitDstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
				const VkCommandBuffer vkCommandBuffer = vulkanContext.getVkCommandBuffer();
				const VkSubmitInfo vkSubmitInfo =
				{
					VK_STRUCTURE_TYPE_SUBMIT_INFO,	// sType (VkStructureType)
					nullptr,						// pNext (const void*)
					1,								// waitSemaphoreCount (uint32_t)
					&mImageAvailableVkSemaphore,	// pWaitSemaphores (const VkSemaphore*)
					&waitDstStageMask,				// pWaitDstStageMask (const VkPipelineStageFlags*)
					1,								// commandBufferCount (uint32_t)
					&vkCommandBuffer,				// pCommandBuffers (const VkCommandBuffer*)
					1,								// signalSemaphoreCount (uint32_t)
					&mRenderingFinishedVkSemaphore	// pSignalSemaphores (const VkSemaphore*)
				};
				if (vkQueueSubmit(vulkanContext.getGraphicsVkQueue(), 1, &vkSubmitInfo, VK_NULL_HANDLE) != VK_SUCCESS)
				{
					// Error!
					RENDERER_LOG(vulkanRenderer.getContext(), CRITICAL, "Vulkan queue submit failed")
					return;
				}
			}

			{ // Queue present
				const VkPresentInfoKHR vkPresentInfoKHR =
				{
					VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,	// sType (VkStructureType)
					nullptr,							// pNext (const void*)
					1,									// waitSemaphoreCount (uint32_t)
					&mRenderingFinishedVkSemaphore,		// pWaitSemaphores (const VkSemaphore*)
					1,									// swapchainCount (uint32_t)
					&mVkSwapchainKHR,					// pSwapchains (const VkSwapchainKHR*)
					&mCurrentImageIndex,				// pImageIndices (const uint32_t*)
					nullptr								// pResults (VkResult*)
				};
				const VkResult vkResult = vkQueuePresentKHR(vulkanContext.getPresentVkQueue(), &vkPresentInfoKHR);
				if (VK_SUCCESS != vkResult)
				{
					if (VK_ERROR_OUT_OF_DATE_KHR == vkResult || VK_SUBOPTIMAL_KHR == vkResult)
					{
						// Recreate the Vulkan swap chain
						createVulkanSwapChain();
						return;
					}
					else
					{
						// Error!
						RENDERER_LOG(vulkanRenderer.getContext(), CRITICAL, "Failed to present Vulkan queue")
						return;
					}
				}
				vkQueueWaitIdle(vulkanContext.getPresentVkQueue());
			}

			// Acquire next image
			acquireNextImage(true);
		}

		inline virtual void resizeBuffers() override
		{
			// Recreate the Vulkan swap chain
			createVulkanSwapChain();
		}

		inline virtual bool getFullscreenState() const override
		{
			// TODO(co) Implement me
			return false;
		}

		inline virtual void setFullscreenState(bool) override
		{
			// TODO(co) Implement me
		}

		inline virtual void setRenderWindow(Renderer::IRenderWindow* renderWindow) override
		{
			mRenderWindow = renderWindow;
		}


	//[-------------------------------------------------------]
	//[ Protected virtual Renderer::RefCount methods          ]
	//[-------------------------------------------------------]
	protected:
		inline virtual void selfDestruct() override
		{
			RENDERER_DELETE(getRenderer().getContext(), SwapChain, this);
		}


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		explicit SwapChain(const SwapChain& source) = delete;
		SwapChain& operator =(const SwapChain& source) = delete;

		void createVulkanSwapChain()
		{
			const Renderer::Context& context = getRenderer().getContext();

			// Get the Vulkan physical device
			const VulkanRenderer&  vulkanRenderer	= static_cast<const VulkanRenderer&>(getRenderer());
			const VulkanContext&   vulkanContext	= vulkanRenderer.getVulkanContext();
			const VkPhysicalDevice vkPhysicalDevice	= vulkanContext.getVkPhysicalDevice();
			const VkDevice		   vkDevice			= vulkanContext.getVkDevice();

			// Sanity checks
			RENDERER_ASSERT(context, VK_NULL_HANDLE != vkPhysicalDevice, "Invalid physical Vulkan device")
			RENDERER_ASSERT(context, VK_NULL_HANDLE != vkDevice, "Invalid Vulkan device")

			// Wait for the Vulkan device to become idle
			vkDeviceWaitIdle(vkDevice);

			// Get Vulkan surface capabilities
			VkSurfaceCapabilitiesKHR vkSurfaceCapabilitiesKHR;
			if (vkGetPhysicalDeviceSurfaceCapabilitiesKHR(vkPhysicalDevice, mVkSurfaceKHR, &vkSurfaceCapabilitiesKHR) != VK_SUCCESS)
			{
				RENDERER_LOG(context, CRITICAL, "Failed to get physical Vulkan device surface capabilities")
				return;
			}

			// Get Vulkan swap chain settings
			const uint32_t                      desiredNumberOfImages				 = ::detail::getNumberOfSwapChainImages(vkSurfaceCapabilitiesKHR);
			const VkSurfaceFormatKHR            desiredVkSurfaceFormatKHR			 = ::detail::getSwapChainFormat(context, vkPhysicalDevice, mVkSurfaceKHR);
			const VkExtent2D                    desiredVkExtent2D					 = ::detail::getSwapChainExtent(vkSurfaceCapabilitiesKHR);
			const VkImageUsageFlags             desiredVkImageUsageFlags			 = ::detail::getSwapChainUsageFlags(context, vkSurfaceCapabilitiesKHR);
			const VkSurfaceTransformFlagBitsKHR desiredVkSurfaceTransformFlagBitsKHR = ::detail::getSwapChainTransform(vkSurfaceCapabilitiesKHR);
			const VkPresentModeKHR              desiredVkPresentModeKHR				 = ::detail::getSwapChainPresentMode(context, vkPhysicalDevice, mVkSurfaceKHR);

			// Validate Vulkan swap chain settings
			if (-1 == static_cast<int>(desiredVkImageUsageFlags))
			{
				RENDERER_LOG(context, CRITICAL, "Invalid desired Vulkan image usage flags")
				return;
			}
			if (VK_PRESENT_MODE_MAX_ENUM_KHR == desiredVkPresentModeKHR)
			{
				RENDERER_LOG(context, CRITICAL, "Invalid desired Vulkan presentation mode")
				return;
			}
			if ((0 == desiredVkExtent2D.width) || (0 == desiredVkExtent2D.height))
			{
				// Current surface size is (0, 0) so we can't create a swap chain and render anything (CanRender == false)
				// But we don't wont to kill the application as this situation may occur i.e. when window gets minimized
				destroyVulkanSwapChain();
				return;
			}

			{ // Create Vulkan swap chain
				VkSwapchainKHR newVkSwapchainKHR = VK_NULL_HANDLE;
				const VkSwapchainCreateInfoKHR vkSwapchainCreateInfoKHR =
				{
					VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,	// sType (VkStructureType)
					nullptr,										// pNext (const void*)
					0,												// flags (VkSwapchainCreateFlagsKHR)
					mVkSurfaceKHR,									// surface (VkSurfaceKHR)
					desiredNumberOfImages,							// minImageCount (uint32_t)
					desiredVkSurfaceFormatKHR.format,				// imageFormat (VkFormat)
					desiredVkSurfaceFormatKHR.colorSpace,			// imageColorSpace (VkColorSpaceKHR)
					desiredVkExtent2D,								// imageExtent (VkExtent2D)
					1,												// imageArrayLayers (uint32_t)
					desiredVkImageUsageFlags,						// imageUsage (VkImageUsageFlags)
					VK_SHARING_MODE_EXCLUSIVE,						// imageSharingMode (VkSharingMode)
					0,												// queueFamilyIndexCount (uint32_t)
					nullptr,										// pQueueFamilyIndices (const uint32_t*)
					desiredVkSurfaceTransformFlagBitsKHR,			// preTransform (VkSurfaceTransformFlagBitsKHR)
					VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,				// compositeAlpha (VkCompositeAlphaFlagBitsKHR)
					desiredVkPresentModeKHR,						// presentMode (VkPresentModeKHR)
					VK_TRUE,										// clipped (VkBool32)
					mVkSwapchainKHR									// oldSwapchain (VkSwapchainKHR)
				};
				if (vkCreateSwapchainKHR(vkDevice, &vkSwapchainCreateInfoKHR, vulkanRenderer.getVkAllocationCallbacks(), &newVkSwapchainKHR) != VK_SUCCESS)
				{
					RENDERER_LOG(context, CRITICAL, "Failed to create Vulkan swap chain")
					return;
				}
				destroyVulkanSwapChain();
				mVkSwapchainKHR = newVkSwapchainKHR;
			}

			// Create depth render target
			createDepthRenderTarget(desiredVkExtent2D);

			// Create render pass
			mVkRenderPass = ::detail::createRenderPass(context, vulkanRenderer.getVkAllocationCallbacks(), vkDevice, desiredVkSurfaceFormatKHR.format, mDepthVkFormat, static_cast<RenderPass&>(getRenderPass()).getVkSampleCountFlagBits());

			// Vulkan swap chain image handling
			if (VK_NULL_HANDLE != mVkRenderPass)
			{
				// Get the swap chain images
				uint32_t swapchainImageCount = 0;
				if (vkGetSwapchainImagesKHR(vkDevice, mVkSwapchainKHR, &swapchainImageCount, nullptr) != VK_SUCCESS)
				{
					RENDERER_LOG(context, CRITICAL, "Failed to get Vulkan swap chain images")
					return;
				}
				std::vector<VkImage> vkImages(swapchainImageCount);
				if (vkGetSwapchainImagesKHR(vkDevice, mVkSwapchainKHR, &swapchainImageCount, vkImages.data()) != VK_SUCCESS)
				{
					RENDERER_LOG(context, CRITICAL, "Failed to get Vulkan swap chain images")
					return;
				}

				// Get the swap chain buffers containing the image and image view
				mSwapChainBuffer.resize(swapchainImageCount);
				const bool hasDepthStencilAttachment = (VK_FORMAT_UNDEFINED != mDepthVkFormat);
				for (uint32_t i = 0; i < swapchainImageCount; ++i)
				{
					SwapChainBuffer& swapChainBuffer = mSwapChainBuffer[i];
					swapChainBuffer.vkImage = vkImages[i];

					// Create the Vulkan image view
					Helper::createVkImageView(vulkanRenderer, swapChainBuffer.vkImage, VK_IMAGE_VIEW_TYPE_2D, 1, 1, desiredVkSurfaceFormatKHR.format, VK_IMAGE_ASPECT_COLOR_BIT, swapChainBuffer.vkImageView);

					{ // Create the Vulkan framebuffer
						const std::array<VkImageView, 2> vkImageViews =
						{
							swapChainBuffer.vkImageView,
							mDepthVkImageView
						};
						const VkFramebufferCreateInfo vkFramebufferCreateInfo =
						{
							VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,	// sType (VkStructureType)
							nullptr,									// pNext (const void*)
							0,											// flags (VkFramebufferCreateFlags)
							mVkRenderPass,								// renderPass (VkRenderPass)
							hasDepthStencilAttachment ? 2u : 1u,		// attachmentCount (uint32_t)
							vkImageViews.data(),						// pAttachments (const VkImageView*)
							desiredVkExtent2D.width,					// width (uint32_t)
							desiredVkExtent2D.height,					// height (uint32_t)
							1											// layers (uint32_t)
						};
						if (vkCreateFramebuffer(vkDevice, &vkFramebufferCreateInfo, vulkanRenderer.getVkAllocationCallbacks(), &swapChainBuffer.vkFramebuffer) != VK_SUCCESS)
						{
							RENDERER_LOG(context, CRITICAL, "Failed to create Vulkan framebuffer")
						}
					}
				}
			}

			{ // Create the Vulkan semaphores
				static constexpr VkSemaphoreCreateInfo vkSemaphoreCreateInfo =
				{
					VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,	// sType (VkStructureType)
					nullptr,									// pNext (const void*)
					0											// flags (VkSemaphoreCreateFlags)
				};
				if ((vkCreateSemaphore(vkDevice, &vkSemaphoreCreateInfo, vulkanRenderer.getVkAllocationCallbacks(), &mImageAvailableVkSemaphore) != VK_SUCCESS) ||
					(vkCreateSemaphore(vkDevice, &vkSemaphoreCreateInfo, vulkanRenderer.getVkAllocationCallbacks(), &mRenderingFinishedVkSemaphore) != VK_SUCCESS))
				{
					RENDERER_LOG(context, CRITICAL, "Failed to create Vulkan semaphore")
				}
			}

			// Acquire next image
			acquireNextImage(false);
		}

		void destroyVulkanSwapChain()
		{
			// Destroy Vulkan swap chain
			if (VK_NULL_HANDLE != mVkRenderPass || !mSwapChainBuffer.empty() || VK_NULL_HANDLE != mVkSwapchainKHR)
			{
				const VulkanRenderer& vulkanRenderer = static_cast<VulkanRenderer&>(getRenderer());
				const VkDevice vkDevice = vulkanRenderer.getVulkanContext().getVkDevice();
				vkDeviceWaitIdle(vkDevice);
				if (VK_NULL_HANDLE != mVkRenderPass)
				{
					vkDestroyRenderPass(vkDevice, mVkRenderPass, vulkanRenderer.getVkAllocationCallbacks());
					mVkRenderPass = VK_NULL_HANDLE;
				}
				if (!mSwapChainBuffer.empty())
				{
					for (const SwapChainBuffer& swapChainBuffer : mSwapChainBuffer)
					{
						vkDestroyFramebuffer(vkDevice, swapChainBuffer.vkFramebuffer, vulkanRenderer.getVkAllocationCallbacks());
						vkDestroyImageView(vkDevice, swapChainBuffer.vkImageView, vulkanRenderer.getVkAllocationCallbacks());
					}
					mSwapChainBuffer.clear();
				}
				if (VK_NULL_HANDLE != mVkSwapchainKHR)
				{
					vkDestroySwapchainKHR(vkDevice, mVkSwapchainKHR, vulkanRenderer.getVkAllocationCallbacks());
					mVkSwapchainKHR = VK_NULL_HANDLE;
				}
				if (VK_NULL_HANDLE != mImageAvailableVkSemaphore)
				{
					vkDestroySemaphore(vulkanRenderer.getVulkanContext().getVkDevice(), mImageAvailableVkSemaphore, vulkanRenderer.getVkAllocationCallbacks());
					mImageAvailableVkSemaphore = VK_NULL_HANDLE;
				}
				if (VK_NULL_HANDLE != mRenderingFinishedVkSemaphore)
				{
					vkDestroySemaphore(vulkanRenderer.getVulkanContext().getVkDevice(), mRenderingFinishedVkSemaphore, vulkanRenderer.getVkAllocationCallbacks());
					mRenderingFinishedVkSemaphore = VK_NULL_HANDLE;
				}
			}

			// Destroy depth render target
			destroyDepthRenderTarget();
		}

		void acquireNextImage(bool recreateSwapChainIfNeeded)
		{
			const VulkanRenderer& vulkanRenderer = static_cast<VulkanRenderer&>(getRenderer());
			const VkResult vkResult = vkAcquireNextImageKHR(vulkanRenderer.getVulkanContext().getVkDevice(), mVkSwapchainKHR, UINT64_MAX, mImageAvailableVkSemaphore, VK_NULL_HANDLE, &mCurrentImageIndex);
			if (VK_SUCCESS != vkResult && VK_SUBOPTIMAL_KHR != vkResult)
			{
				if (VK_ERROR_OUT_OF_DATE_KHR == vkResult)
				{
					// Recreate the Vulkan swap chain
					if (recreateSwapChainIfNeeded)
					{
						createVulkanSwapChain();
					}
				}
				else
				{
					// Error!
					RENDERER_LOG(vulkanRenderer.getContext(), CRITICAL, "Failed to acquire next Vulkan image from swap chain")
				}
			}
		}

		void createDepthRenderTarget(const VkExtent2D& vkExtent2D)
		{
			if (VK_FORMAT_UNDEFINED != mDepthVkFormat)
			{
				const VulkanRenderer& vulkanRenderer = static_cast<VulkanRenderer&>(getRenderer());
				Helper::createAndAllocateVkImage(vulkanRenderer, 0, VK_IMAGE_TYPE_2D, { vkExtent2D.width, vkExtent2D.height, 1 }, 1, 1, mDepthVkFormat, static_cast<RenderPass&>(getRenderPass()).getVkSampleCountFlagBits(), VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, mDepthVkImage, mDepthVkDeviceMemory);
				Helper::createVkImageView(vulkanRenderer, mDepthVkImage, VK_IMAGE_VIEW_TYPE_2D, 1, 1, mDepthVkFormat, VK_IMAGE_ASPECT_DEPTH_BIT, mDepthVkImageView);
				// TODO(co) File "unrimp\source\renderer\private\vulkanrenderer\vulkanrenderer.cpp" | Line 1036 | Critical: Vulkan debug report callback: Object type: "VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT" Object: "103612336" Location: "0" Message code: "461375810" Layer prefix: "Validation" Message: " [ VUID-vkCmdPipelineBarrier-pMemoryBarriers-01185 ] Object: 0x62cffb0 (Type = 6) | vkCmdPipelineBarrier(): pImageMemBarriers[0].dstAccessMask (0x600) is not supported by dstStageMask (0x1). The spec valid usage text states 'Each element of pMemoryBarriers, pBufferMemoryBarriers and pImageMemoryBarriers must not have any access flag included in its dstAccessMask member if that bit is not supported by any of the pipeline stages in dstStageMask, as specified in the table of supported access types.' (https://www.khronos.org/registry/vulkan/specs/1.0/html/vkspec.html#VUID-vkCmdPipelineBarrier-pMemoryBarriers-01185)" 
				//Helper::transitionVkImageLayout(vulkanRenderer, mDepthVkImage, VK_IMAGE_ASPECT_DEPTH_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
			}
		}

		void destroyDepthRenderTarget()
		{
			if (VK_NULL_HANDLE != mDepthVkImage)
			{
				RENDERER_ASSERT(getRenderer().getContext(), VK_NULL_HANDLE != mDepthVkDeviceMemory, "Invalid Vulkan depth device memory")
				RENDERER_ASSERT(getRenderer().getContext(), VK_NULL_HANDLE != mDepthVkImageView, "Invalid Vulkan depth image view")
				Helper::destroyAndFreeVkImage(static_cast<VulkanRenderer&>(getRenderer()), mDepthVkImage, mDepthVkDeviceMemory, mDepthVkImageView);
			}
		}


	//[-------------------------------------------------------]
	//[ Private definitions                                   ]
	//[-------------------------------------------------------]
	private:
		struct SwapChainBuffer
		{
			VkImage		  vkImage		= VK_NULL_HANDLE;	///< Vulkan image, don't destroy since we don't own it
			VkImageView   vkImageView	= VK_NULL_HANDLE;	///< Vulkan image view, destroy if no longer needed
			VkFramebuffer vkFramebuffer	= VK_NULL_HANDLE;	///< Vulkan framebuffer, destroy if no longer needed
		};
		typedef std::vector<SwapChainBuffer> SwapChainBuffers;


	//[-------------------------------------------------------]
	//[ Private data                                          ]
	//[-------------------------------------------------------]
	private:
		// Operation system window
		Renderer::handle		 mNativeWindowHandle;	///< Native window handle window, can be a null handle
		Renderer::IRenderWindow* mRenderWindow;			///< Render window instance, can be a null pointer, don't destroy the instance since we don't own it
		// Vulkan presentation surface
		VkSurfaceKHR mVkSurfaceKHR;	///< Vulkan presentation surface, destroy if no longer needed
		// Vulkan swap chain and color render target related
		VkSwapchainKHR	 mVkSwapchainKHR;				///< Vulkan swap chain, destroy if no longer needed
		VkRenderPass	 mVkRenderPass;					///< Vulkan render pass, destroy if no longer needed (due to "VK_IMAGE_LAYOUT_PRESENT_SRC_KHR" we need an own Vulkan render pass instance)
		SwapChainBuffers mSwapChainBuffer;				///< Swap chain buffer for managing the color render targets
		VkSemaphore		 mImageAvailableVkSemaphore;	///< Vulkan semaphore, destroy if no longer needed
		VkSemaphore		 mRenderingFinishedVkSemaphore;	///< Vulkan semaphore, destroy if no longer needed
		uint32_t		 mCurrentImageIndex;			///< The index of the current Vulkan swap chain image to render into, ~0 if invalid
		// Depth render target related
		VkFormat		mDepthVkFormat;	///< Can be "VK_FORMAT_UNDEFINED" if no depth stencil buffer is needed
		VkImage			mDepthVkImage;
		VkDeviceMemory  mDepthVkDeviceMemory;
		VkImageView		mDepthVkImageView;


	};




	//[-------------------------------------------------------]
	//[ VulkanRenderer/RenderTarget/Framebuffer.h             ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Vulkan framebuffer interface
	*/
	class Framebuffer final : public Renderer::IFramebuffer
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] renderPass
		*    Render pass to use, the swap chain keeps a reference to the render pass
		*  @param[in] colorFramebufferAttachments
		*    The color render target textures, can be a null pointer or can contain null pointers, if not a null pointer there must be at
		*    least "Renderer::IRenderPass::getNumberOfColorAttachments()" textures in the provided C-array of pointers
		*  @param[in] depthStencilFramebufferAttachment
		*    The depth stencil render target texture, can be a null pointer
		*
		*  @note
		*    - The framebuffer keeps a reference to the provided texture instances
		*/
		Framebuffer(Renderer::IRenderPass& renderPass, const Renderer::FramebufferAttachment* colorFramebufferAttachments, const Renderer::FramebufferAttachment* depthStencilFramebufferAttachment) :
			IFramebuffer(renderPass),
			mNumberOfColorTextures(static_cast<RenderPass&>(renderPass).getNumberOfColorAttachments()),
			mColorTextures(nullptr),	// Set below
			mDepthStencilTexture(nullptr),
			mWidth(UINT_MAX),
			mHeight(UINT_MAX),
			mVkRenderPass(static_cast<RenderPass&>(renderPass).getVkRenderPass()),
			mVkFramebuffer(VK_NULL_HANDLE)
		{
			// Vulkan attachment descriptions and views to fill
			std::vector<VkImageView> vkImageViews;
			vkImageViews.resize(mNumberOfColorTextures + ((nullptr != depthStencilFramebufferAttachment) ? 1u : 0u));
			uint32_t currentVkAttachmentDescriptionIndex = 0;

			// Add a reference to the used color textures
			if (mNumberOfColorTextures > 0)
			{
				mColorTextures = RENDERER_MALLOC_TYPED(renderPass.getRenderer().getContext(), Renderer::ITexture*, mNumberOfColorTextures);

				// Loop through all color textures
				Renderer::ITexture** colorTexturesEnd = mColorTextures + mNumberOfColorTextures;
				for (Renderer::ITexture** colorTexture = mColorTextures; colorTexture < colorTexturesEnd; ++colorTexture, ++colorFramebufferAttachments)
				{
					// Sanity check
					RENDERER_ASSERT(renderPass.getRenderer().getContext(), nullptr != colorFramebufferAttachments->texture, "Invalid Vulkan color framebuffer attachment texture")

					// TODO(co) Add security check: Is the given resource one of the currently used renderer?
					*colorTexture = colorFramebufferAttachments->texture;
					(*colorTexture)->addReference();

					// Evaluate the color texture type
					VkImageView vkImageView = VK_NULL_HANDLE;
					switch ((*colorTexture)->getResourceType())
					{
						case Renderer::ResourceType::TEXTURE_2D:
						{
							const Texture2D* texture2D = static_cast<Texture2D*>(*colorTexture);

							// Sanity checks
							RENDERER_ASSERT(renderPass.getRenderer().getContext(), colorFramebufferAttachments->mipmapIndex < Texture2D::getNumberOfMipmaps(texture2D->getWidth(), texture2D->getHeight()), "Invalid Vulkan color framebuffer attachment mipmap index")
							RENDERER_ASSERT(renderPass.getRenderer().getContext(), 0 == colorFramebufferAttachments->layerIndex, "Invalid Vulkan color framebuffer attachment layer index")

							// Update the framebuffer width and height if required
							vkImageView = texture2D->getVkImageView();
							::detail::updateWidthHeight(colorFramebufferAttachments->mipmapIndex, texture2D->getWidth(), texture2D->getHeight(), mWidth, mHeight);
							break;
						}

						case Renderer::ResourceType::TEXTURE_2D_ARRAY:
						{
							// Update the framebuffer width and height if required
							const Texture2DArray* texture2DArray = static_cast<Texture2DArray*>(*colorTexture);
							vkImageView = texture2DArray->getVkImageView();
							::detail::updateWidthHeight(colorFramebufferAttachments->mipmapIndex, texture2DArray->getWidth(), texture2DArray->getHeight(), mWidth, mHeight);
							break;
						}

						case Renderer::ResourceType::ROOT_SIGNATURE:
						case Renderer::ResourceType::RESOURCE_GROUP:
						case Renderer::ResourceType::GRAPHICS_PROGRAM:
						case Renderer::ResourceType::VERTEX_ARRAY:
						case Renderer::ResourceType::RENDER_PASS:
						case Renderer::ResourceType::SWAP_CHAIN:
						case Renderer::ResourceType::FRAMEBUFFER:
						case Renderer::ResourceType::INDEX_BUFFER:
						case Renderer::ResourceType::VERTEX_BUFFER:
						case Renderer::ResourceType::TEXTURE_BUFFER:
						case Renderer::ResourceType::STRUCTURED_BUFFER:
						case Renderer::ResourceType::INDIRECT_BUFFER:
						case Renderer::ResourceType::UNIFORM_BUFFER:
						case Renderer::ResourceType::TEXTURE_1D:
						case Renderer::ResourceType::TEXTURE_3D:
						case Renderer::ResourceType::TEXTURE_CUBE:
						case Renderer::ResourceType::GRAPHICS_PIPELINE_STATE:
						case Renderer::ResourceType::COMPUTE_PIPELINE_STATE:
						case Renderer::ResourceType::SAMPLER_STATE:
						case Renderer::ResourceType::VERTEX_SHADER:
						case Renderer::ResourceType::TESSELLATION_CONTROL_SHADER:
						case Renderer::ResourceType::TESSELLATION_EVALUATION_SHADER:
						case Renderer::ResourceType::GEOMETRY_SHADER:
						case Renderer::ResourceType::FRAGMENT_SHADER:
						case Renderer::ResourceType::COMPUTE_SHADER:
						default:
							// Nothing here
							break;
					}

					// Remember the Vulkan image view
					vkImageViews[currentVkAttachmentDescriptionIndex] = vkImageView;

					// Advance current Vulkan attachment description index
					++currentVkAttachmentDescriptionIndex;
				}
			}

			// Add a reference to the used depth stencil texture
			if (nullptr != depthStencilFramebufferAttachment)
			{
				mDepthStencilTexture = depthStencilFramebufferAttachment->texture;
				RENDERER_ASSERT(renderPass.getRenderer().getContext(), nullptr != mDepthStencilTexture, "Invalid Vulkan depth stencil framebuffer attachment texture")
				mDepthStencilTexture->addReference();

				// Evaluate the depth stencil texture type
				VkImageView vkImageView = VK_NULL_HANDLE;
				switch (mDepthStencilTexture->getResourceType())
				{
					case Renderer::ResourceType::TEXTURE_2D:
					{
						const Texture2D* texture2D = static_cast<Texture2D*>(mDepthStencilTexture);

						// Sanity checks
						RENDERER_ASSERT(renderPass.getRenderer().getContext(), depthStencilFramebufferAttachment->mipmapIndex < Texture2D::getNumberOfMipmaps(texture2D->getWidth(), texture2D->getHeight()), "Invalid Vulkan depth stencil framebuffer attachment mipmap index")
						RENDERER_ASSERT(renderPass.getRenderer().getContext(), 0 == depthStencilFramebufferAttachment->layerIndex, "Invalid Vulkan depth stencil framebuffer attachment layer index")

						// Update the framebuffer width and height if required
						vkImageView = texture2D->getVkImageView();
						::detail::updateWidthHeight(depthStencilFramebufferAttachment->mipmapIndex, texture2D->getWidth(), texture2D->getHeight(), mWidth, mHeight);
						break;
					}

					case Renderer::ResourceType::TEXTURE_2D_ARRAY:
					{
						// Update the framebuffer width and height if required
						const Texture2DArray* texture2DArray = static_cast<Texture2DArray*>(mDepthStencilTexture);
						vkImageView = texture2DArray->getVkImageView();
						::detail::updateWidthHeight(depthStencilFramebufferAttachment->mipmapIndex, texture2DArray->getWidth(), texture2DArray->getHeight(), mWidth, mHeight);
						break;
					}

					case Renderer::ResourceType::ROOT_SIGNATURE:
					case Renderer::ResourceType::RESOURCE_GROUP:
					case Renderer::ResourceType::GRAPHICS_PROGRAM:
					case Renderer::ResourceType::VERTEX_ARRAY:
					case Renderer::ResourceType::RENDER_PASS:
					case Renderer::ResourceType::SWAP_CHAIN:
					case Renderer::ResourceType::FRAMEBUFFER:
					case Renderer::ResourceType::INDEX_BUFFER:
					case Renderer::ResourceType::VERTEX_BUFFER:
					case Renderer::ResourceType::TEXTURE_BUFFER:
					case Renderer::ResourceType::STRUCTURED_BUFFER:
					case Renderer::ResourceType::INDIRECT_BUFFER:
					case Renderer::ResourceType::UNIFORM_BUFFER:
					case Renderer::ResourceType::TEXTURE_1D:
					case Renderer::ResourceType::TEXTURE_3D:
					case Renderer::ResourceType::TEXTURE_CUBE:
					case Renderer::ResourceType::GRAPHICS_PIPELINE_STATE:
					case Renderer::ResourceType::COMPUTE_PIPELINE_STATE:
					case Renderer::ResourceType::SAMPLER_STATE:
					case Renderer::ResourceType::VERTEX_SHADER:
					case Renderer::ResourceType::TESSELLATION_CONTROL_SHADER:
					case Renderer::ResourceType::TESSELLATION_EVALUATION_SHADER:
					case Renderer::ResourceType::GEOMETRY_SHADER:
					case Renderer::ResourceType::FRAGMENT_SHADER:
					case Renderer::ResourceType::COMPUTE_SHADER:
					default:
						// Nothing here
						break;
				}

				// Remember the Vulkan image view
				vkImageViews[currentVkAttachmentDescriptionIndex] = vkImageView;
			}

			// Validate the framebuffer width and height
			if (0 == mWidth || UINT_MAX == mWidth)
			{
				RENDERER_ASSERT(renderPass.getRenderer().getContext(), false, "Invalid Vulkan framebuffer width")
				mWidth = 1;
			}
			if (0 == mHeight || UINT_MAX == mHeight)
			{
				RENDERER_ASSERT(renderPass.getRenderer().getContext(), false, "Invalid Vulkan framebuffer height")
				mHeight = 1;
			}

			// Create Vulkan framebuffer
			const VulkanRenderer& vulkanRenderer = static_cast<VulkanRenderer&>(renderPass.getRenderer());
			const VkFramebufferCreateInfo vkFramebufferCreateInfo =
			{
				VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,	// sType (VkStructureType)
				nullptr,									// pNext (const void*)
				0,											// flags (VkFramebufferCreateFlags)
				mVkRenderPass,								// renderPass (VkRenderPass)
				static_cast<uint32_t>(vkImageViews.size()),	// attachmentCount (uint32_t)
				vkImageViews.data(),						// pAttachments (const VkImageView*)
				mWidth,										// width (uint32_t)
				mHeight,									// height (uint32_t
				1											// layers (uint32_t)
			};
			if (vkCreateFramebuffer(vulkanRenderer.getVulkanContext().getVkDevice(), &vkFramebufferCreateInfo, vulkanRenderer.getVkAllocationCallbacks(), &mVkFramebuffer) != VK_SUCCESS)
			{
				RENDERER_LOG(vulkanRenderer.getContext(), CRITICAL, "Failed to create Vulkan framebuffer")
			}
			else
			{
				SET_DEFAULT_DEBUG_NAME	// setDebugName("");
			}
		}

		/**
		*  @brief
		*    Destructor
		*/
		virtual ~Framebuffer() override
		{
			const VulkanRenderer& vulkanRenderer = static_cast<VulkanRenderer&>(getRenderer());
			const VkDevice vkDevice = vulkanRenderer.getVulkanContext().getVkDevice();

			// Destroy Vulkan framebuffer instance
			if (VK_NULL_HANDLE != mVkFramebuffer)
			{
				vkDestroyFramebuffer(vkDevice, mVkFramebuffer, vulkanRenderer.getVkAllocationCallbacks());
			}

			// Release the reference to the used color textures
			if (nullptr != mColorTextures)
			{
				// Release references
				Renderer::ITexture** colorTexturesEnd = mColorTextures + mNumberOfColorTextures;
				for (Renderer::ITexture** colorTexture = mColorTextures; colorTexture < colorTexturesEnd; ++colorTexture)
				{
					(*colorTexture)->releaseReference();
				}

				// Cleanup
				RENDERER_FREE(getRenderer().getContext(), mColorTextures);
			}

			// Release the reference to the used depth stencil texture
			if (nullptr != mDepthStencilTexture)
			{
				// Release reference
				mDepthStencilTexture->releaseReference();
			}
		}

		/**
		*  @brief
		*    Return the Vulkan render pass
		*
		*  @return
		*    The Vulkan render pass
		*/
		inline VkRenderPass getVkRenderPass() const
		{
			return mVkRenderPass;
		}

		/**
		*  @brief
		*    Return the Vulkan framebuffer to render into
		*
		*  @return
		*    The Vulkan framebuffer to render into
		*/
		inline VkFramebuffer getVkFramebuffer() const
		{
			return mVkFramebuffer;
		}


	//[-------------------------------------------------------]
	//[ Public virtual Renderer::IResource methods            ]
	//[-------------------------------------------------------]
	public:
		#ifdef RENDERER_DEBUG
			virtual void setDebugName(const char* name) override
			{
				if (nullptr != vkDebugMarkerSetObjectNameEXT)
				{
					const VkDevice vkDevice = static_cast<const VulkanRenderer&>(getRenderer()).getVulkanContext().getVkDevice();
					Helper::setDebugObjectName(vkDevice, VK_DEBUG_REPORT_OBJECT_TYPE_RENDER_PASS_EXT, (uint64_t)mVkRenderPass, name);
					Helper::setDebugObjectName(vkDevice, VK_DEBUG_REPORT_OBJECT_TYPE_FRAMEBUFFER_EXT, (uint64_t)mVkFramebuffer, name);
				}
			}
		#endif


	//[-------------------------------------------------------]
	//[ Public virtual Renderer::IRenderTarget methods        ]
	//[-------------------------------------------------------]
	public:
		inline virtual void getWidthAndHeight(uint32_t& width, uint32_t& height) const override
		{
			// No fancy implementation in here, just copy over the internal information
			width  = mWidth;
			height = mHeight;
		}


	//[-------------------------------------------------------]
	//[ Protected virtual Renderer::RefCount methods          ]
	//[-------------------------------------------------------]
	protected:
		inline virtual void selfDestruct() override
		{
			RENDERER_DELETE(getRenderer().getContext(), Framebuffer, this);
		}


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		explicit Framebuffer(const Framebuffer& source) = delete;
		Framebuffer& operator =(const Framebuffer& source) = delete;


	//[-------------------------------------------------------]
	//[ Private data                                          ]
	//[-------------------------------------------------------]
	private:
		uint32_t			 mNumberOfColorTextures;	///< Number of color render target textures
		Renderer::ITexture** mColorTextures;			///< The color render target textures (we keep a reference to it), can be a null pointer or can contain null pointers, if not a null pointer there must be at least "mNumberOfColorTextures" textures in the provided C-array of pointers
		Renderer::ITexture*  mDepthStencilTexture;		///< The depth stencil render target texture (we keep a reference to it), can be a null pointer
		uint32_t			 mWidth;					///< The framebuffer width
		uint32_t			 mHeight;					///< The framebuffer height
		VkRenderPass		 mVkRenderPass;				///< Vulkan render pass instance, can be a null handle, we don't own it
		VkFramebuffer		 mVkFramebuffer;			///< Vulkan framebuffer instance, can be a null handle


	};




	//[-------------------------------------------------------]
	//[ VulkanRenderer/Shader/VertexShaderGlsl.h              ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    GLSL vertex shader class
	*/
	class VertexShaderGlsl final : public Renderer::IVertexShader
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor for creating a vertex shader from shader bytecode
		*
		*  @param[in] vulkanRenderer
		*    Owner Vulkan renderer instance
		*  @param[in] shaderBytecode
		*    Shader bytecode
		*/
		VertexShaderGlsl(VulkanRenderer& vulkanRenderer, const Renderer::ShaderBytecode& shaderBytecode) :
			IVertexShader(vulkanRenderer),
			mVkShaderModule(::detail::createVkShaderModuleFromBytecode(vulkanRenderer.getContext(), vulkanRenderer.getVkAllocationCallbacks(), vulkanRenderer.getVulkanContext().getVkDevice(), shaderBytecode))
		{
			SET_DEFAULT_DEBUG_NAME	// setDebugName("");
		}

		/**
		*  @brief
		*    Constructor for creating a vertex shader from shader source code
		*
		*  @param[in] vulkanRenderer
		*    Owner Vulkan renderer instance
		*  @param[in] sourceCode
		*    Shader ASCII source code, must be valid
		*/
		VertexShaderGlsl(VulkanRenderer& vulkanRenderer, const char* sourceCode, Renderer::ShaderBytecode* shaderBytecode = nullptr) :
			IVertexShader(vulkanRenderer),
			mVkShaderModule(::detail::createVkShaderModuleFromSourceCode(vulkanRenderer.getContext(), vulkanRenderer.getVkAllocationCallbacks(), vulkanRenderer.getVulkanContext().getVkDevice(), VK_SHADER_STAGE_VERTEX_BIT, sourceCode, shaderBytecode))
		{
			SET_DEFAULT_DEBUG_NAME	// setDebugName("");
		}

		/**
		*  @brief
		*    Destructor
		*/
		virtual ~VertexShaderGlsl() override
		{
			if (VK_NULL_HANDLE != mVkShaderModule)
			{
				const VulkanRenderer& vulkanRenderer = static_cast<VulkanRenderer&>(getRenderer());
				vkDestroyShaderModule(vulkanRenderer.getVulkanContext().getVkDevice(), mVkShaderModule, vulkanRenderer.getVkAllocationCallbacks());
			}
		}

		/**
		*  @brief
		*    Return the Vulkan shader module
		*
		*  @return
		*    The Vulkan shader module
		*/
		inline VkShaderModule getVkShaderModule() const
		{
			return mVkShaderModule;
		}


	//[-------------------------------------------------------]
	//[ Public virtual Renderer::IResource methods            ]
	//[-------------------------------------------------------]
	public:
		DEFINE_SET_DEBUG_NAME_SHADER_MODULE()


	//[-------------------------------------------------------]
	//[ Public virtual Renderer::IShader methods              ]
	//[-------------------------------------------------------]
	public:
		inline virtual const char* getShaderLanguageName() const override
		{
			return ::detail::GLSL_NAME;
		}


	//[-------------------------------------------------------]
	//[ Protected virtual Renderer::RefCount methods          ]
	//[-------------------------------------------------------]
	protected:
		inline virtual void selfDestruct() override
		{
			RENDERER_DELETE(getRenderer().getContext(), VertexShaderGlsl, this);
		}


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		explicit VertexShaderGlsl(const VertexShaderGlsl& source) = delete;
		VertexShaderGlsl& operator =(const VertexShaderGlsl& source) = delete;


	//[-------------------------------------------------------]
	//[ Private data                                          ]
	//[-------------------------------------------------------]
	private:
		VkShaderModule mVkShaderModule;	///< Vulkan shader module, destroy it if you no longer need it


	};




	//[-------------------------------------------------------]
	//[ VulkanRenderer/Shader/TessellationControlShaderGlsl.h ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    GLSL tessellation control shader ("hull shader" in Direct3D terminology) class
	*/
	class TessellationControlShaderGlsl final : public Renderer::ITessellationControlShader
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor for creating a tessellation control shader ("hull shader" in Direct3D terminology) shader from shader bytecode
		*
		*  @param[in] vulkanRenderer
		*    Owner Vulkan renderer instance
		*  @param[in] shaderBytecode
		*    Shader bytecode
		*/
		TessellationControlShaderGlsl(VulkanRenderer& vulkanRenderer, const Renderer::ShaderBytecode& shaderBytecode) :
			ITessellationControlShader(vulkanRenderer),
			mVkShaderModule(::detail::createVkShaderModuleFromBytecode(vulkanRenderer.getContext(), vulkanRenderer.getVkAllocationCallbacks(), vulkanRenderer.getVulkanContext().getVkDevice(), shaderBytecode))
		{
			SET_DEFAULT_DEBUG_NAME	// setDebugName("");
		}

		/**
		*  @brief
		*    Constructor for creating a tessellation control shader ("hull shader" in Direct3D terminology) shader from shader source code
		*
		*  @param[in] vulkanRenderer
		*    Owner Vulkan renderer instance
		*  @param[in] sourceCode
		*    Shader ASCII source code, must be valid
		*/
		TessellationControlShaderGlsl(VulkanRenderer& vulkanRenderer, const char* sourceCode, Renderer::ShaderBytecode* shaderBytecode = nullptr) :
			ITessellationControlShader(vulkanRenderer),
			mVkShaderModule(::detail::createVkShaderModuleFromSourceCode(vulkanRenderer.getContext(), vulkanRenderer.getVkAllocationCallbacks(), vulkanRenderer.getVulkanContext().getVkDevice(), VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT, sourceCode, shaderBytecode))
		{
			SET_DEFAULT_DEBUG_NAME	// setDebugName("");
		}

		/**
		*  @brief
		*    Destructor
		*/
		virtual ~TessellationControlShaderGlsl() override
		{
			if (VK_NULL_HANDLE != mVkShaderModule)
			{
				const VulkanRenderer& vulkanRenderer = static_cast<VulkanRenderer&>(getRenderer());
				vkDestroyShaderModule(vulkanRenderer.getVulkanContext().getVkDevice(), mVkShaderModule, vulkanRenderer.getVkAllocationCallbacks());
			}
		}

		/**
		*  @brief
		*    Return the Vulkan shader module
		*
		*  @return
		*    The Vulkan shader module
		*/
		inline VkShaderModule getVkShaderModule() const
		{
			return mVkShaderModule;
		}


	//[-------------------------------------------------------]
	//[ Public virtual Renderer::IResource methods            ]
	//[-------------------------------------------------------]
	public:
		DEFINE_SET_DEBUG_NAME_SHADER_MODULE()


	//[-------------------------------------------------------]
	//[ Public virtual Renderer::IShader methods              ]
	//[-------------------------------------------------------]
	public:
		inline virtual const char* getShaderLanguageName() const override
		{
			return ::detail::GLSL_NAME;
		}


	//[-------------------------------------------------------]
	//[ Protected virtual Renderer::RefCount methods          ]
	//[-------------------------------------------------------]
	protected:
		inline virtual void selfDestruct() override
		{
			RENDERER_DELETE(getRenderer().getContext(), TessellationControlShaderGlsl, this);
		}


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		explicit TessellationControlShaderGlsl(const TessellationControlShaderGlsl& source) = delete;
		TessellationControlShaderGlsl& operator =(const TessellationControlShaderGlsl& source) = delete;


	//[-------------------------------------------------------]
	//[ Private data                                          ]
	//[-------------------------------------------------------]
	private:
		VkShaderModule mVkShaderModule;	///< Vulkan shader module, destroy it if you no longer need it


	};




	//[-------------------------------------------------------]
	//[ VulkanRenderer/Shader/TessellationEvaluationShaderGlsl.h ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    GLSL tessellation evaluation shader ("domain shader" in Direct3D terminology) class
	*/
	class TessellationEvaluationShaderGlsl final : public Renderer::ITessellationEvaluationShader
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor for creating a tessellation evaluation shader ("domain shader" in Direct3D terminology) shader from shader bytecode
		*
		*  @param[in] vulkanRenderer
		*    Owner Vulkan renderer instance
		*  @param[in] shaderBytecode
		*    Shader bytecode
		*/
		TessellationEvaluationShaderGlsl(VulkanRenderer& vulkanRenderer, const Renderer::ShaderBytecode& shaderBytecode) :
			ITessellationEvaluationShader(vulkanRenderer),
			mVkShaderModule(::detail::createVkShaderModuleFromBytecode(vulkanRenderer.getContext(), vulkanRenderer.getVkAllocationCallbacks(), vulkanRenderer.getVulkanContext().getVkDevice(), shaderBytecode))
		{
			SET_DEFAULT_DEBUG_NAME	// setDebugName("");
		}

		/**
		*  @brief
		*    Constructor for creating a tessellation evaluation shader ("domain shader" in Direct3D terminology) shader from shader source code
		*
		*  @param[in] vulkanRenderer
		*    Owner Vulkan renderer instance
		*  @param[in] sourceCode
		*    Shader ASCII source code, must be valid
		*/
		TessellationEvaluationShaderGlsl(VulkanRenderer& vulkanRenderer, const char* sourceCode, Renderer::ShaderBytecode* shaderBytecode = nullptr) :
			ITessellationEvaluationShader(vulkanRenderer),
			mVkShaderModule(::detail::createVkShaderModuleFromSourceCode(vulkanRenderer.getContext(), vulkanRenderer.getVkAllocationCallbacks(), vulkanRenderer.getVulkanContext().getVkDevice(), VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT, sourceCode, shaderBytecode))
		{
			SET_DEFAULT_DEBUG_NAME	// setDebugName("");
		}

		/**
		*  @brief
		*    Destructor
		*/
		virtual ~TessellationEvaluationShaderGlsl() override
		{
			if (VK_NULL_HANDLE != mVkShaderModule)
			{
				const VulkanRenderer& vulkanRenderer = static_cast<VulkanRenderer&>(getRenderer());
				vkDestroyShaderModule(vulkanRenderer.getVulkanContext().getVkDevice(), mVkShaderModule, vulkanRenderer.getVkAllocationCallbacks());
			}
		}

		/**
		*  @brief
		*    Return the Vulkan shader module
		*
		*  @return
		*    The Vulkan shader module
		*/
		inline VkShaderModule getVkShaderModule() const
		{
			return mVkShaderModule;
		}


	//[-------------------------------------------------------]
	//[ Public virtual Renderer::IResource methods            ]
	//[-------------------------------------------------------]
	public:
		DEFINE_SET_DEBUG_NAME_SHADER_MODULE()


	//[-------------------------------------------------------]
	//[ Public virtual Renderer::IShader methods              ]
	//[-------------------------------------------------------]
	public:
		inline virtual const char* getShaderLanguageName() const override
		{
			return ::detail::GLSL_NAME;
		}


	//[-------------------------------------------------------]
	//[ Protected virtual Renderer::RefCount methods          ]
	//[-------------------------------------------------------]
	protected:
		inline virtual void selfDestruct() override
		{
			RENDERER_DELETE(getRenderer().getContext(), TessellationEvaluationShaderGlsl, this);
		}


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		explicit TessellationEvaluationShaderGlsl(const TessellationEvaluationShaderGlsl& source) = delete;
		TessellationEvaluationShaderGlsl& operator =(const TessellationEvaluationShaderGlsl& source) = delete;


	//[-------------------------------------------------------]
	//[ Private data                                          ]
	//[-------------------------------------------------------]
	private:
		VkShaderModule mVkShaderModule;	///< Vulkan shader module, destroy it if you no longer need it


	};




	//[-------------------------------------------------------]
	//[ VulkanRenderer/Shader/GeometryShaderGlsl.h            ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    GLSL geometry shader class
	*/
	class GeometryShaderGlsl final : public Renderer::IGeometryShader
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor for creating a geometry shader from shader bytecode
		*
		*  @param[in] vulkanRenderer
		*    Owner Vulkan renderer instance
		*  @param[in] shaderBytecode
		*    Shader bytecode
		*  @param[in] gsInputPrimitiveTopology
		*    Geometry shader input primitive topology
		*  @param[in] gsOutputPrimitiveTopology
		*    Geometry shader output primitive topology
		*  @param[in] numberOfOutputVertices
		*    Number of output vertices
		*/
		// TODO(co) Remove unused parameters
		GeometryShaderGlsl(VulkanRenderer& vulkanRenderer, const Renderer::ShaderBytecode& shaderBytecode, MAYBE_UNUSED Renderer::GsInputPrimitiveTopology gsInputPrimitiveTopology, MAYBE_UNUSED Renderer::GsOutputPrimitiveTopology gsOutputPrimitiveTopology, MAYBE_UNUSED uint32_t numberOfOutputVertices) :
			IGeometryShader(vulkanRenderer),
			mVkShaderModule(::detail::createVkShaderModuleFromBytecode(vulkanRenderer.getContext(), vulkanRenderer.getVkAllocationCallbacks(), vulkanRenderer.getVulkanContext().getVkDevice(), shaderBytecode))
		{
			SET_DEFAULT_DEBUG_NAME	// setDebugName("");
		}

		/**
		*  @brief
		*    Constructor for creating a geometry shader from shader source code
		*
		*  @param[in] vulkanRenderer
		*    Owner Vulkan renderer instance
		*  @param[in] sourceCode
		*    Shader ASCII source code, must be valid
		*  @param[in] gsInputPrimitiveTopology
		*    Geometry shader input primitive topology
		*  @param[in] gsOutputPrimitiveTopology
		*    Geometry shader output primitive topology
		*  @param[in] numberOfOutputVertices
		*    Number of output vertices
		*/
		// TODO(co) Remove unused parameters
		GeometryShaderGlsl(VulkanRenderer& vulkanRenderer, const char* sourceCode, MAYBE_UNUSED Renderer::GsInputPrimitiveTopology gsInputPrimitiveTopology, MAYBE_UNUSED Renderer::GsOutputPrimitiveTopology gsOutputPrimitiveTopology, MAYBE_UNUSED uint32_t numberOfOutputVertices, Renderer::ShaderBytecode* shaderBytecode = nullptr) :
			IGeometryShader(vulkanRenderer),
			mVkShaderModule(::detail::createVkShaderModuleFromSourceCode(vulkanRenderer.getContext(), vulkanRenderer.getVkAllocationCallbacks(), vulkanRenderer.getVulkanContext().getVkDevice(), VK_SHADER_STAGE_GEOMETRY_BIT, sourceCode, shaderBytecode))
		{
			SET_DEFAULT_DEBUG_NAME	// setDebugName("");
		}

		/**
		*  @brief
		*    Destructor
		*/
		virtual ~GeometryShaderGlsl() override
		{
			if (VK_NULL_HANDLE != mVkShaderModule)
			{
				const VulkanRenderer& vulkanRenderer = static_cast<VulkanRenderer&>(getRenderer());
				vkDestroyShaderModule(vulkanRenderer.getVulkanContext().getVkDevice(), mVkShaderModule, vulkanRenderer.getVkAllocationCallbacks());
			}
		}

		/**
		*  @brief
		*    Return the Vulkan shader module
		*
		*  @return
		*    The Vulkan shader module
		*/
		inline VkShaderModule getVkShaderModule() const
		{
			return mVkShaderModule;
		}


	//[-------------------------------------------------------]
	//[ Public virtual Renderer::IResource methods            ]
	//[-------------------------------------------------------]
	public:
		DEFINE_SET_DEBUG_NAME_SHADER_MODULE()


	//[-------------------------------------------------------]
	//[ Public virtual Renderer::IShader methods              ]
	//[-------------------------------------------------------]
	public:
		inline virtual const char* getShaderLanguageName() const override
		{
			return ::detail::GLSL_NAME;
		}


	//[-------------------------------------------------------]
	//[ Protected virtual Renderer::RefCount methods          ]
	//[-------------------------------------------------------]
	protected:
		inline virtual void selfDestruct() override
		{
			RENDERER_DELETE(getRenderer().getContext(), GeometryShaderGlsl, this);
		}


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		explicit GeometryShaderGlsl(const GeometryShaderGlsl& source) = delete;
		GeometryShaderGlsl& operator =(const GeometryShaderGlsl& source) = delete;


	//[-------------------------------------------------------]
	//[ Private data                                          ]
	//[-------------------------------------------------------]
	private:
		VkShaderModule mVkShaderModule;	///< Vulkan shader module, destroy it if you no longer need it


	};




	//[-------------------------------------------------------]
	//[ VulkanRenderer/Shader/FragmentShaderGlsl.h            ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    GLSL fragment shader (FS, "pixel shader" in Direct3D terminology) class
	*/
	class FragmentShaderGlsl final : public Renderer::IFragmentShader
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor for creating a fragment shader from shader bytecode
		*
		*  @param[in] vulkanRenderer
		*    Owner Vulkan renderer instance
		*  @param[in] shaderBytecode
		*    Shader bytecode
		*/
		FragmentShaderGlsl(VulkanRenderer& vulkanRenderer, const Renderer::ShaderBytecode& shaderBytecode) :
			IFragmentShader(vulkanRenderer),
			mVkShaderModule(::detail::createVkShaderModuleFromBytecode(vulkanRenderer.getContext(), vulkanRenderer.getVkAllocationCallbacks(), vulkanRenderer.getVulkanContext().getVkDevice(), shaderBytecode))
		{
			SET_DEFAULT_DEBUG_NAME	// setDebugName("");
		}

		/**
		*  @brief
		*    Constructor for creating a fragment shader from shader source code
		*
		*  @param[in] vulkanRenderer
		*    Owner Vulkan renderer instance
		*  @param[in] sourceCode
		*    Shader ASCII source code, must be valid
		*/
		FragmentShaderGlsl(VulkanRenderer& vulkanRenderer, const char* sourceCode, Renderer::ShaderBytecode* shaderBytecode = nullptr) :
			IFragmentShader(vulkanRenderer),
			mVkShaderModule(::detail::createVkShaderModuleFromSourceCode(vulkanRenderer.getContext(), vulkanRenderer.getVkAllocationCallbacks(), vulkanRenderer.getVulkanContext().getVkDevice(), VK_SHADER_STAGE_FRAGMENT_BIT, sourceCode, shaderBytecode))
		{
			SET_DEFAULT_DEBUG_NAME	// setDebugName("");
		}

		/**
		*  @brief
		*    Destructor
		*/
		virtual ~FragmentShaderGlsl() override
		{
			if (VK_NULL_HANDLE != mVkShaderModule)
			{
				const VulkanRenderer& vulkanRenderer = static_cast<VulkanRenderer&>(getRenderer());
				vkDestroyShaderModule(vulkanRenderer.getVulkanContext().getVkDevice(), mVkShaderModule, vulkanRenderer.getVkAllocationCallbacks());
			}
		}

		/**
		*  @brief
		*    Return the Vulkan shader module
		*
		*  @return
		*    The Vulkan shader module
		*/
		inline VkShaderModule getVkShaderModule() const
		{
			return mVkShaderModule;
		}


	//[-------------------------------------------------------]
	//[ Public virtual Renderer::IResource methods            ]
	//[-------------------------------------------------------]
	public:
		DEFINE_SET_DEBUG_NAME_SHADER_MODULE()


	//[-------------------------------------------------------]
	//[ Public virtual Renderer::IShader methods              ]
	//[-------------------------------------------------------]
	public:
		inline virtual const char* getShaderLanguageName() const override
		{
			return ::detail::GLSL_NAME;
		}


	//[-------------------------------------------------------]
	//[ Protected virtual Renderer::RefCount methods          ]
	//[-------------------------------------------------------]
	protected:
		inline virtual void selfDestruct() override
		{
			RENDERER_DELETE(getRenderer().getContext(), FragmentShaderGlsl, this);
		}


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		explicit FragmentShaderGlsl(const FragmentShaderGlsl& source) = delete;
		FragmentShaderGlsl& operator =(const FragmentShaderGlsl& source) = delete;


	//[-------------------------------------------------------]
	//[ Private data                                          ]
	//[-------------------------------------------------------]
	private:
		VkShaderModule mVkShaderModule;	///< Vulkan shader module, destroy it if you no longer need it


	};




	//[-------------------------------------------------------]
	//[ VulkanRenderer/Shader/ComputeShaderGlsl.h             ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    GLSL compute shader (CS) class
	*/
	class ComputeShaderGlsl final : public Renderer::IComputeShader
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor for creating a compute shader from shader bytecode
		*
		*  @param[in] vulkanRenderer
		*    Owner Vulkan renderer instance
		*  @param[in] shaderBytecode
		*    Shader bytecode
		*/
		ComputeShaderGlsl(VulkanRenderer& vulkanRenderer, const Renderer::ShaderBytecode& shaderBytecode) :
			IComputeShader(vulkanRenderer),
			mVkShaderModule(::detail::createVkShaderModuleFromBytecode(vulkanRenderer.getContext(), vulkanRenderer.getVkAllocationCallbacks(), vulkanRenderer.getVulkanContext().getVkDevice(), shaderBytecode))
		{
			SET_DEFAULT_DEBUG_NAME	// setDebugName("");
		}

		/**
		*  @brief
		*    Constructor for creating a compute shader from shader source code
		*
		*  @param[in] vulkanRenderer
		*    Owner Vulkan renderer instance
		*  @param[in] sourceCode
		*    Shader ASCII source code, must be valid
		*/
		ComputeShaderGlsl(VulkanRenderer& vulkanRenderer, const char* sourceCode, Renderer::ShaderBytecode* shaderBytecode = nullptr) :
			IComputeShader(vulkanRenderer),
			mVkShaderModule(::detail::createVkShaderModuleFromSourceCode(vulkanRenderer.getContext(), vulkanRenderer.getVkAllocationCallbacks(), vulkanRenderer.getVulkanContext().getVkDevice(), VK_SHADER_STAGE_COMPUTE_BIT, sourceCode, shaderBytecode))
		{
			SET_DEFAULT_DEBUG_NAME	// setDebugName("");
		}

		/**
		*  @brief
		*    Destructor
		*/
		virtual ~ComputeShaderGlsl() override
		{
			if (VK_NULL_HANDLE != mVkShaderModule)
			{
				const VulkanRenderer& vulkanRenderer = static_cast<VulkanRenderer&>(getRenderer());
				vkDestroyShaderModule(vulkanRenderer.getVulkanContext().getVkDevice(), mVkShaderModule, vulkanRenderer.getVkAllocationCallbacks());
			}
		}

		/**
		*  @brief
		*    Return the Vulkan shader module
		*
		*  @return
		*    The Vulkan shader module
		*/
		inline VkShaderModule getVkShaderModule() const
		{
			return mVkShaderModule;
		}


	//[-------------------------------------------------------]
	//[ Public virtual Renderer::IResource methods            ]
	//[-------------------------------------------------------]
	public:
		DEFINE_SET_DEBUG_NAME_SHADER_MODULE()


	//[-------------------------------------------------------]
	//[ Public virtual Renderer::IShader methods              ]
	//[-------------------------------------------------------]
	public:
		inline virtual const char* getShaderLanguageName() const override
		{
			return ::detail::GLSL_NAME;
		}


	//[-------------------------------------------------------]
	//[ Protected virtual Renderer::RefCount methods          ]
	//[-------------------------------------------------------]
	protected:
		inline virtual void selfDestruct() override
		{
			RENDERER_DELETE(getRenderer().getContext(), ComputeShaderGlsl, this);
		}


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		explicit ComputeShaderGlsl(const ComputeShaderGlsl& source) = delete;
		ComputeShaderGlsl& operator =(const ComputeShaderGlsl& source) = delete;


	//[-------------------------------------------------------]
	//[ Private data                                          ]
	//[-------------------------------------------------------]
	private:
		VkShaderModule mVkShaderModule;	///< Vulkan shader module, destroy it if you no longer need it


	};




	//[-------------------------------------------------------]
	//[ VulkanRenderer/Shader/GraphicsProgramGlsl.h           ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    GLSL graphics program class
	*/
	class GraphicsProgramGlsl final : public Renderer::IGraphicsProgram
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] vulkanRenderer
		*    Owner Vulkan renderer instance
		*  @param[in] rootSignature
		*    Root signature
		*  @param[in] vertexAttributes
		*    Vertex attributes ("vertex declaration" in Direct3D 9 terminology, "input layout" in Direct3D 10 & 11 terminology)
		*  @param[in] vertexShaderGlsl
		*    Vertex shader the graphics program is using, can be a null pointer
		*  @param[in] tessellationControlShaderGlsl
		*    Tessellation control shader the graphics program is using, can be a null pointer
		*  @param[in] tessellationEvaluationShaderGlsl
		*    Tessellation evaluation shader the graphics program is using, can be a null pointer
		*  @param[in] geometryShaderGlsl
		*    Geometry shader the graphics program is using, can be a null pointer
		*  @param[in] fragmentShaderGlsl
		*    Fragment shader the graphics program is using, can be a null pointer
		*
		*  @note
		*    - The graphics program keeps a reference to the provided shaders and releases it when no longer required
		*/
		// TODO(co) Remove unused parameters
		GraphicsProgramGlsl(VulkanRenderer& vulkanRenderer, MAYBE_UNUSED const Renderer::IRootSignature& rootSignature, MAYBE_UNUSED const Renderer::VertexAttributes& vertexAttributes, VertexShaderGlsl *vertexShaderGlsl, TessellationControlShaderGlsl *tessellationControlShaderGlsl, TessellationEvaluationShaderGlsl *tessellationEvaluationShaderGlsl, GeometryShaderGlsl *geometryShaderGlsl, FragmentShaderGlsl *fragmentShaderGlsl) :
			IGraphicsProgram(vulkanRenderer),
			mVertexShaderGlsl(vertexShaderGlsl),
			mTessellationControlShaderGlsl(tessellationControlShaderGlsl),
			mTessellationEvaluationShaderGlsl(tessellationEvaluationShaderGlsl),
			mGeometryShaderGlsl(geometryShaderGlsl),
			mFragmentShaderGlsl(fragmentShaderGlsl)
		{
			// Add references to the provided shaders
			if (nullptr != mVertexShaderGlsl)
			{
				mVertexShaderGlsl->addReference();
			}
			if (nullptr != mTessellationControlShaderGlsl)
			{
				mTessellationControlShaderGlsl->addReference();
			}
			if (nullptr != mTessellationEvaluationShaderGlsl)
			{
				mTessellationEvaluationShaderGlsl->addReference();
			}
			if (nullptr != mGeometryShaderGlsl)
			{
				mGeometryShaderGlsl->addReference();
			}
			if (nullptr != mFragmentShaderGlsl)
			{
				mFragmentShaderGlsl->addReference();
			}
		}

		/**
		*  @brief
		*    Destructor
		*/
		virtual ~GraphicsProgramGlsl() override
		{
			// Release the shader references
			if (nullptr != mVertexShaderGlsl)
			{
				mVertexShaderGlsl->releaseReference();
			}
			if (nullptr != mTessellationControlShaderGlsl)
			{
				mTessellationControlShaderGlsl->releaseReference();
			}
			if (nullptr != mTessellationEvaluationShaderGlsl)
			{
				mTessellationEvaluationShaderGlsl->releaseReference();
			}
			if (nullptr != mGeometryShaderGlsl)
			{
				mGeometryShaderGlsl->releaseReference();
			}
			if (nullptr != mFragmentShaderGlsl)
			{
				mFragmentShaderGlsl->releaseReference();
			}
		}

		/**
		*  @brief
		*    Return the GLSL vertex shader the graphics program is using
		*
		*  @return
		*    The GLSL vertex shader the graphics program is using, can be a null pointer, do not release the returned instance unless you added an own reference to it
		*/
		inline VertexShaderGlsl* getVertexShaderGlsl() const
		{
			return mVertexShaderGlsl;
		}

		/**
		*  @brief
		*    Return the GLSL tessellation control shader the graphics program is using
		*
		*  @return
		*    The GLSL tessellation control shader the graphics program is using, can be a null pointer, do not release the returned instance unless you added an own reference to it
		*/
		inline TessellationControlShaderGlsl* getTessellationControlShaderGlsl() const
		{
			return mTessellationControlShaderGlsl;
		}

		/**
		*  @brief
		*    Return the GLSL tessellation evaluation shader the graphics program is using
		*
		*  @return
		*    The GLSL tessellation evaluation shader the graphics program is using, can be a null pointer, do not release the returned instance unless you added an own reference to it
		*/
		inline TessellationEvaluationShaderGlsl* getTessellationEvaluationShaderGlsl() const
		{
			return mTessellationEvaluationShaderGlsl;
		}

		/**
		*  @brief
		*    Return the GLSL geometry shader the graphics program is using
		*
		*  @return
		*    The GLSL geometry shader the graphics program is using, can be a null pointer, do not release the returned instance unless you added an own reference to it
		*/
		inline GeometryShaderGlsl* getGeometryShaderGlsl() const
		{
			return mGeometryShaderGlsl;
		}

		/**
		*  @brief
		*    Return the GLSL fragment shader the graphics program is using
		*
		*  @return
		*    The GLSL fragment shader the graphics program is using, can be a null pointer, do not release the returned instance unless you added an own reference to it
		*/
		inline FragmentShaderGlsl* getFragmentShaderGlsl() const
		{
			return mFragmentShaderGlsl;
		}


	//[-------------------------------------------------------]
	//[ Protected virtual Renderer::RefCount methods          ]
	//[-------------------------------------------------------]
	protected:
		inline virtual void selfDestruct() override
		{
			RENDERER_DELETE(getRenderer().getContext(), GraphicsProgramGlsl, this);
		}


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		explicit GraphicsProgramGlsl(const GraphicsProgramGlsl& source) = delete;
		GraphicsProgramGlsl& operator =(const GraphicsProgramGlsl& source) = delete;


	//[-------------------------------------------------------]
	//[ Private data                                          ]
	//[-------------------------------------------------------]
	private:
		VertexShaderGlsl*				  mVertexShaderGlsl;					///< Vertex shader the graphics program is using (we keep a reference to it), can be a null pointer
		TessellationControlShaderGlsl*	  mTessellationControlShaderGlsl;		///< Tessellation control shader the graphics program is using (we keep a reference to it), can be a null pointer
		TessellationEvaluationShaderGlsl* mTessellationEvaluationShaderGlsl;	///< Tessellation evaluation shader the graphics program is using (we keep a reference to it), can be a null pointer
		GeometryShaderGlsl*				  mGeometryShaderGlsl;					///< Geometry shader the graphics program is using (we keep a reference to it), can be a null pointer
		FragmentShaderGlsl*				  mFragmentShaderGlsl;					///< Fragment shader the graphics program is using (we keep a reference to it), can be a null pointer


	};




	//[-------------------------------------------------------]
	//[ VulkanRenderer/Shader/ShaderLanguageGlsl.h            ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    GLSL shader language class
	*/
	class ShaderLanguageGlsl final : public Renderer::IShaderLanguage
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] vulkanRenderer
		*    Owner Vulkan renderer instance
		*/
		inline explicit ShaderLanguageGlsl(VulkanRenderer& vulkanRenderer) :
			IShaderLanguage(vulkanRenderer)
		{}

		/**
		*  @brief
		*    Destructor
		*/
		inline virtual ~ShaderLanguageGlsl() override
		{
			// De-initialize glslang, if necessary
			#ifdef RENDERER_VULKAN_GLSLTOSPIRV
				if (::detail::GlslangInitialized)
				{
					// TODO(co) Fix glslang related memory leaks. See also
					//		    - "Fix a few memory leaks #916" - https://github.com/KhronosGroup/glslang/pull/916
					//		    - "FreeGlobalPools is never called in glslang::FinalizeProcess()'s path. #928" - https://github.com/KhronosGroup/glslang/issues/928
					glslang::FinalizeProcess();
					::detail::GlslangInitialized = false;
				}
			#endif
		}


	//[-------------------------------------------------------]
	//[ Public virtual Renderer::IShaderLanguage methods      ]
	//[-------------------------------------------------------]
	public:
		inline virtual const char* getShaderLanguageName() const override
		{
			return ::detail::GLSL_NAME;
		}

		inline virtual Renderer::IVertexShader* createVertexShaderFromBytecode(MAYBE_UNUSED const Renderer::VertexAttributes& vertexAttributes, const Renderer::ShaderBytecode& shaderBytecode) override
		{
			return RENDERER_NEW(getRenderer().getContext(), VertexShaderGlsl)(static_cast<VulkanRenderer&>(getRenderer()), shaderBytecode);
		}

		inline virtual Renderer::IVertexShader* createVertexShaderFromSourceCode(MAYBE_UNUSED const Renderer::VertexAttributes& vertexAttributes, const Renderer::ShaderSourceCode& shaderSourceCode, Renderer::ShaderBytecode* shaderBytecode = nullptr) override
		{
			return RENDERER_NEW(getRenderer().getContext(), VertexShaderGlsl)(static_cast<VulkanRenderer&>(getRenderer()), shaderSourceCode.sourceCode, shaderBytecode);
		}

		inline virtual Renderer::ITessellationControlShader* createTessellationControlShaderFromBytecode(const Renderer::ShaderBytecode& shaderBytecode) override
		{
			return RENDERER_NEW(getRenderer().getContext(), TessellationControlShaderGlsl)(static_cast<VulkanRenderer&>(getRenderer()), shaderBytecode);
		}

		inline virtual Renderer::ITessellationControlShader* createTessellationControlShaderFromSourceCode(const Renderer::ShaderSourceCode& shaderSourceCode, Renderer::ShaderBytecode* shaderBytecode = nullptr) override
		{
			return RENDERER_NEW(getRenderer().getContext(), TessellationControlShaderGlsl)(static_cast<VulkanRenderer&>(getRenderer()), shaderSourceCode.sourceCode, shaderBytecode);
		}

		inline virtual Renderer::ITessellationEvaluationShader* createTessellationEvaluationShaderFromBytecode(const Renderer::ShaderBytecode& shaderBytecode) override
		{
			return RENDERER_NEW(getRenderer().getContext(), TessellationEvaluationShaderGlsl)(static_cast<VulkanRenderer&>(getRenderer()), shaderBytecode);
		}

		inline virtual Renderer::ITessellationEvaluationShader* createTessellationEvaluationShaderFromSourceCode(const Renderer::ShaderSourceCode& shaderSourceCode, Renderer::ShaderBytecode* shaderBytecode = nullptr) override
		{
			return RENDERER_NEW(getRenderer().getContext(), TessellationEvaluationShaderGlsl)(static_cast<VulkanRenderer&>(getRenderer()), shaderSourceCode.sourceCode, shaderBytecode);
		}

		inline virtual Renderer::IGeometryShader* createGeometryShaderFromBytecode(const Renderer::ShaderBytecode& shaderBytecode, Renderer::GsInputPrimitiveTopology gsInputPrimitiveTopology, Renderer::GsOutputPrimitiveTopology gsOutputPrimitiveTopology, uint32_t numberOfOutputVertices) override
		{
			return RENDERER_NEW(getRenderer().getContext(), GeometryShaderGlsl)(static_cast<VulkanRenderer&>(getRenderer()), shaderBytecode, gsInputPrimitiveTopology, gsOutputPrimitiveTopology, numberOfOutputVertices);
		}

		inline virtual Renderer::IGeometryShader* createGeometryShaderFromSourceCode(const Renderer::ShaderSourceCode& shaderSourceCode, Renderer::GsInputPrimitiveTopology gsInputPrimitiveTopology, Renderer::GsOutputPrimitiveTopology gsOutputPrimitiveTopology, uint32_t numberOfOutputVertices, Renderer::ShaderBytecode* shaderBytecode = nullptr) override
		{
			return RENDERER_NEW(getRenderer().getContext(), GeometryShaderGlsl)(static_cast<VulkanRenderer&>(getRenderer()), shaderSourceCode.sourceCode, gsInputPrimitiveTopology, gsOutputPrimitiveTopology, numberOfOutputVertices, shaderBytecode);
		}

		inline virtual Renderer::IFragmentShader* createFragmentShaderFromBytecode(const Renderer::ShaderBytecode& shaderBytecode) override
		{
			return RENDERER_NEW(getRenderer().getContext(), FragmentShaderGlsl)(static_cast<VulkanRenderer&>(getRenderer()), shaderBytecode);
		}

		inline virtual Renderer::IFragmentShader* createFragmentShaderFromSourceCode(const Renderer::ShaderSourceCode& shaderSourceCode, Renderer::ShaderBytecode* shaderBytecode = nullptr) override
		{
			return RENDERER_NEW(getRenderer().getContext(), FragmentShaderGlsl)(static_cast<VulkanRenderer&>(getRenderer()), shaderSourceCode.sourceCode, shaderBytecode);
		}

		inline virtual Renderer::IComputeShader* createComputeShaderFromBytecode(const Renderer::ShaderBytecode& shaderBytecode) override
		{
			return RENDERER_NEW(getRenderer().getContext(), ComputeShaderGlsl)(static_cast<VulkanRenderer&>(getRenderer()), shaderBytecode);
		}

		inline virtual Renderer::IComputeShader* createComputeShaderFromSourceCode(const Renderer::ShaderSourceCode& shaderSourceCode, Renderer::ShaderBytecode* shaderBytecode = nullptr) override
		{
			return RENDERER_NEW(getRenderer().getContext(), ComputeShaderGlsl)(static_cast<VulkanRenderer&>(getRenderer()), shaderSourceCode.sourceCode, shaderBytecode);
		}

		virtual Renderer::IGraphicsProgram* createGraphicsProgram(const Renderer::IRootSignature& rootSignature, const Renderer::VertexAttributes& vertexAttributes, Renderer::IVertexShader* vertexShader, Renderer::ITessellationControlShader* tessellationControlShader, Renderer::ITessellationEvaluationShader* tessellationEvaluationShader, Renderer::IGeometryShader* geometryShader, Renderer::IFragmentShader* fragmentShader) override
		{
			VulkanRenderer& vulkanRenderer = static_cast<VulkanRenderer&>(getRenderer());

			// A shader can be a null pointer, but if it's not the shader and graphics program language must match!
			// -> Optimization: Comparing the shader language name by directly comparing the pointer address of
			//    the name is safe because we know that we always reference to one and the same name address
			// TODO(co) Add security check: Is the given resource one of the currently used renderer?
			if (nullptr != vertexShader && vertexShader->getShaderLanguageName() != ::detail::GLSL_NAME)
			{
				// Error! Vertex shader language mismatch!
			}
			else if (nullptr != tessellationControlShader && tessellationControlShader->getShaderLanguageName() != ::detail::GLSL_NAME)
			{
				// Error! Tessellation control shader language mismatch!
			}
			else if (nullptr != tessellationEvaluationShader && tessellationEvaluationShader->getShaderLanguageName() != ::detail::GLSL_NAME)
			{
				// Error! Tessellation evaluation shader language mismatch!
			}
			else if (nullptr != geometryShader && geometryShader->getShaderLanguageName() != ::detail::GLSL_NAME)
			{
				// Error! Geometry shader language mismatch!
			}
			else if (nullptr != fragmentShader && fragmentShader->getShaderLanguageName() != ::detail::GLSL_NAME)
			{
				// Error! Fragment shader language mismatch!
			}
			else
			{
				return RENDERER_NEW(getRenderer().getContext(), GraphicsProgramGlsl)(vulkanRenderer, rootSignature, vertexAttributes, static_cast<VertexShaderGlsl*>(vertexShader), static_cast<TessellationControlShaderGlsl*>(tessellationControlShader), static_cast<TessellationEvaluationShaderGlsl*>(tessellationEvaluationShader), static_cast<GeometryShaderGlsl*>(geometryShader), static_cast<FragmentShaderGlsl*>(fragmentShader));
			}

			// Error! Shader language mismatch!
			// -> Ensure a correct reference counter behaviour, even in the situation of an error
			if (nullptr != vertexShader)
			{
				vertexShader->addReference();
				vertexShader->releaseReference();
			}
			if (nullptr != tessellationControlShader)
			{
				tessellationControlShader->addReference();
				tessellationControlShader->releaseReference();
			}
			if (nullptr != tessellationEvaluationShader)
			{
				tessellationEvaluationShader->addReference();
				tessellationEvaluationShader->releaseReference();
			}
			if (nullptr != geometryShader)
			{
				geometryShader->addReference();
				geometryShader->releaseReference();
			}
			if (nullptr != fragmentShader)
			{
				fragmentShader->addReference();
				fragmentShader->releaseReference();
			}

			// Error!
			return nullptr;
		}


	//[-------------------------------------------------------]
	//[ Protected virtual Renderer::RefCount methods          ]
	//[-------------------------------------------------------]
	protected:
		inline virtual void selfDestruct() override
		{
			RENDERER_DELETE(getRenderer().getContext(), ShaderLanguageGlsl, this);
		}


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		explicit ShaderLanguageGlsl(const ShaderLanguageGlsl& source) = delete;
		ShaderLanguageGlsl& operator =(const ShaderLanguageGlsl& source) = delete;


	};




	//[-------------------------------------------------------]
	//[ VulkanRenderer/State/GraphicsPipelineState.h          ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Vulkan graphics pipeline state class
	*/
	class GraphicsPipelineState final : public Renderer::IGraphicsPipelineState
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] vulkanRenderer
		*    Owner Vulkan renderer instance
		*  @param[in] graphicsPipelineState
		*    Graphics pipeline state to use
		*/
		GraphicsPipelineState(VulkanRenderer& vulkanRenderer, const Renderer::GraphicsPipelineState& graphicsPipelineState) :
			IGraphicsPipelineState(vulkanRenderer),
			mRootSignature(graphicsPipelineState.rootSignature),
			mGraphicsProgram(graphicsPipelineState.graphicsProgram),
			mRenderPass(graphicsPipelineState.renderPass),
			mVkPipeline(VK_NULL_HANDLE)
		{
			// Add a reference to the given root signature, graphics program and render pass
			mRootSignature->addReference();
			mGraphicsProgram->addReference();
			mRenderPass->addReference();

			// Sanity checks
			RENDERER_ASSERT(vulkanRenderer.getContext(), nullptr != mRootSignature, "Invalid Vulkan root signature")
			RENDERER_ASSERT(vulkanRenderer.getContext(), nullptr != mRenderPass, "Invalid Vulkan render pass")

			// Our pipeline state needs to be independent of concrete render targets, so we're using dynamic viewport ("VK_DYNAMIC_STATE_VIEWPORT") and scissor ("VK_DYNAMIC_STATE_SCISSOR") states
			static constexpr uint32_t width  = 42;
			static constexpr uint32_t height = 42;

			// Shaders
			GraphicsProgramGlsl* graphicsProgramGlsl = static_cast<GraphicsProgramGlsl*>(mGraphicsProgram);
			uint32_t stageCount = 0;
			::detail::VkPipelineShaderStageCreateInfos vkPipelineShaderStageCreateInfos;
			{
				// Define helper macro
				#define SHADER_STAGE(vkShaderStageFlagBits, shaderGlsl) \
					if (nullptr != shaderGlsl) \
					{ \
						::detail::addVkPipelineShaderStageCreateInfo(vkShaderStageFlagBits, shaderGlsl->getVkShaderModule(), vkPipelineShaderStageCreateInfos, stageCount); \
						++stageCount; \
					}

				// Shader stages
				SHADER_STAGE(VK_SHADER_STAGE_VERTEX_BIT,				  graphicsProgramGlsl->getVertexShaderGlsl())
				SHADER_STAGE(VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT,	  graphicsProgramGlsl->getTessellationControlShaderGlsl())
				SHADER_STAGE(VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT, graphicsProgramGlsl->getTessellationEvaluationShaderGlsl())
				SHADER_STAGE(VK_SHADER_STAGE_GEOMETRY_BIT,				  graphicsProgramGlsl->getGeometryShaderGlsl())
				SHADER_STAGE(VK_SHADER_STAGE_FRAGMENT_BIT,				  graphicsProgramGlsl->getFragmentShaderGlsl())

				// Undefine helper macro
				#undef SHADER_STAGE
			}

			// Vertex attributes
			const uint32_t numberOfAttributes = graphicsPipelineState.vertexAttributes.numberOfAttributes;
			std::vector<VkVertexInputBindingDescription> vkVertexInputBindingDescriptions;
			std::vector<VkVertexInputAttributeDescription> vkVertexInputAttributeDescriptions(numberOfAttributes);
			for (uint32_t attribute = 0; attribute < numberOfAttributes; ++attribute)
			{
				const Renderer::VertexAttribute* attributes = &graphicsPipelineState.vertexAttributes.attributes[attribute];
				const uint32_t inputSlot = attributes->inputSlot;

				{ // Map to Vulkan vertex input binding description
					if (vkVertexInputBindingDescriptions.size() <= inputSlot)
					{
						vkVertexInputBindingDescriptions.resize(inputSlot + 1);
					}
					VkVertexInputBindingDescription& vkVertexInputBindingDescription = vkVertexInputBindingDescriptions[inputSlot];
					vkVertexInputBindingDescription.binding   = inputSlot;
					vkVertexInputBindingDescription.stride    = attributes->strideInBytes;
					vkVertexInputBindingDescription.inputRate = (attributes->instancesPerElement > 0) ? VK_VERTEX_INPUT_RATE_INSTANCE : VK_VERTEX_INPUT_RATE_VERTEX;
				}

				{ // Map to Vulkan vertex input attribute description
					VkVertexInputAttributeDescription& vkVertexInputAttributeDescription = vkVertexInputAttributeDescriptions[attribute];
					vkVertexInputAttributeDescription.location = attribute;
					vkVertexInputAttributeDescription.binding  = inputSlot;
					vkVertexInputAttributeDescription.format   = Mapping::getVulkanFormat(attributes->vertexAttributeFormat);
					vkVertexInputAttributeDescription.offset   = attributes->alignedByteOffset;
				}
			}

			// Create the Vulkan graphics pipeline
			// TODO(co) Implement the rest of the value mappings
			const VkPipelineVertexInputStateCreateInfo vkPipelineVertexInputStateCreateInfo =
			{
				VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,			// sType (VkStructureType)
				nullptr,															// pNext (const void*)
				0,																	// flags (VkPipelineVertexInputStateCreateFlags)
				static_cast<uint32_t>(vkVertexInputBindingDescriptions.size()),		// vertexBindingDescriptionCount (uint32_t)
				vkVertexInputBindingDescriptions.data(),							// pVertexBindingDescriptions (const VkVertexInputBindingDescription*)
				static_cast<uint32_t>(vkVertexInputAttributeDescriptions.size()),	// vertexAttributeDescriptionCount (uint32_t)
				vkVertexInputAttributeDescriptions.data()							// pVertexAttributeDescriptions (const VkVertexInputAttributeDescription*)
			};
			const VkPipelineInputAssemblyStateCreateInfo vkPipelineInputAssemblyStateCreateInfo =
			{
				VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,		// sType (VkStructureType)
				nullptr,															// pNext (const void*)
				0,																	// flags (VkPipelineInputAssemblyStateCreateFlags)
				Mapping::getVulkanType(graphicsPipelineState.primitiveTopology),	// topology (VkPrimitiveTopology)
				VK_FALSE															// primitiveRestartEnable (VkBool32)
			};
			const VkViewport vkViewport =
			{
				0.0f,						// x (float)
				0.0f,						// y (float)
				static_cast<float>(width),	// width (float)
				static_cast<float>(height),	// height (float)
				0.0f,						// minDepth (float)
				1.0f						// maxDepth (float)
			};
			const VkRect2D scissorVkRect2D =
			{
				{ // offset (VkOffset2D)
					0,	// x (int32_t)
					0	// y (int32_t)
				},
				{ // extent (VkExtent2D)
					width,	// width (uint32_t)
					height	// height (uint32_t)
				}
			};
			const VkPipelineTessellationStateCreateInfo vkPipelineTessellationStateCreateInfo =
			{
				VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO,																																											// sType (VkStructureType)
				nullptr,																																																							// pNext (const void*)
				0,																																																									// flags (VkPipelineTessellationStateCreateFlags)
				(graphicsPipelineState.primitiveTopology >= Renderer::PrimitiveTopology::PATCH_LIST_1) ? static_cast<uint32_t>(graphicsPipelineState.primitiveTopology) - static_cast<uint32_t>(Renderer::PrimitiveTopology::PATCH_LIST_1) + 1 : 1	// patchControlPoints (uint32_t)
			};
			const VkPipelineViewportStateCreateInfo vkPipelineViewportStateCreateInfo =
			{
				VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,	// sType (VkStructureType)
				nullptr,												// pNext (const void*)
				0,														// flags (VkPipelineViewportStateCreateFlags)
				1,														// viewportCount (uint32_t)
				&vkViewport,											// pViewports (const VkViewport*)
				1,														// scissorCount (uint32_t)
				&scissorVkRect2D										// pScissors (const VkRect2D*)
			};
			const float depthBias = static_cast<float>(graphicsPipelineState.rasterizerState.depthBias);
			const float depthBiasClamp = graphicsPipelineState.rasterizerState.depthBiasClamp;
			const float slopeScaledDepthBias = graphicsPipelineState.rasterizerState.slopeScaledDepthBias;
			const VkPipelineRasterizationStateCreateInfo vkPipelineRasterizationStateCreateInfo =
			{
				VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,																			// sType (VkStructureType)
				nullptr,																															// pNext (const void*)
				0,																																	// flags (VkPipelineRasterizationStateCreateFlags)
				static_cast<VkBool32>(graphicsPipelineState.rasterizerState.depthClipEnable),														// depthClampEnable (VkBool32)
				VK_FALSE,																															// rasterizerDiscardEnable (VkBool32)
				(Renderer::FillMode::WIREFRAME == graphicsPipelineState.rasterizerState.fillMode) ? VK_POLYGON_MODE_LINE : VK_POLYGON_MODE_FILL,	// polygonMode (VkPolygonMode)
				static_cast<VkCullModeFlags>(static_cast<int>(graphicsPipelineState.rasterizerState.cullMode) - 1),									// cullMode (VkCullModeFlags)
				(1 == graphicsPipelineState.rasterizerState.frontCounterClockwise) ? VK_FRONT_FACE_COUNTER_CLOCKWISE : VK_FRONT_FACE_CLOCKWISE,		// frontFace (VkFrontFace)
				static_cast<VkBool32>(0.0f != depthBias || 0.0f != depthBiasClamp || 0.0f != slopeScaledDepthBias),									// depthBiasEnable (VkBool32)
				depthBias,																															// depthBiasConstantFactor (float)
				depthBiasClamp,																														// depthBiasClamp (float)
				slopeScaledDepthBias,																												// depthBiasSlopeFactor (float)
				1.0f																																// lineWidth (float)
			};
			const RenderPass* renderPass = static_cast<const RenderPass*>(mRenderPass);
			const VkPipelineMultisampleStateCreateInfo vkPipelineMultisampleStateCreateInfo =
			{
				VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,	// sType (VkStructureType)
				nullptr,													// pNext (const void*)
				0,															// flags (VkPipelineMultisampleStateCreateFlags)
				renderPass->getVkSampleCountFlagBits(),						// rasterizationSamples (VkSampleCountFlagBits)
				VK_FALSE,													// sampleShadingEnable (VkBool32)
				0.0f,														// minSampleShading (float)
				nullptr,													// pSampleMask (const VkSampleMask*)
				VK_FALSE,													// alphaToCoverageEnable (VkBool32)
				VK_FALSE													// alphaToOneEnable (VkBool32)
			};
			const VkPipelineDepthStencilStateCreateInfo vkPipelineDepthStencilStateCreateInfo =
			{
				VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,														// sType (VkStructureType)
				nullptr,																										// pNext (const void*)
				0,																												// flags (VkPipelineDepthStencilStateCreateFlags)
				static_cast<VkBool32>(0 != graphicsPipelineState.depthStencilState.depthEnable),								// depthTestEnable (VkBool32)
				static_cast<VkBool32>(Renderer::DepthWriteMask::ALL == graphicsPipelineState.depthStencilState.depthWriteMask),	// depthWriteEnable (VkBool32)
				Mapping::getVulkanComparisonFunc(graphicsPipelineState.depthStencilState.depthFunc),							// depthCompareOp (VkCompareOp)
				VK_FALSE,																										// depthBoundsTestEnable (VkBool32)
				static_cast<VkBool32>(0 != graphicsPipelineState.depthStencilState.stencilEnable),								// stencilTestEnable (VkBool32)
				{ // front (VkStencilOpState)
					VK_STENCIL_OP_KEEP,																							// failOp (VkStencilOp)
					VK_STENCIL_OP_KEEP,																							// passOp (VkStencilOp)
					VK_STENCIL_OP_KEEP,																							// depthFailOp (VkStencilOp)
					VK_COMPARE_OP_NEVER,																						// compareOp (VkCompareOp)
					0,																											// compareMask (uint32_t)
					0,																											// writeMask (uint32_t)
					0																											// reference (uint32_t)
				},
				{ // back (VkStencilOpState)
					VK_STENCIL_OP_KEEP,																							// failOp (VkStencilOp)
					VK_STENCIL_OP_KEEP,																							// passOp (VkStencilOp)
					VK_STENCIL_OP_KEEP,																							// depthFailOp (VkStencilOp)
					VK_COMPARE_OP_NEVER,																						// compareOp (VkCompareOp)
					0,																											// compareMask (uint32_t)
					0,																											// writeMask (uint32_t)
					0																											// reference (uint32_t)
				},
				0.0f,																											// minDepthBounds (float)
				1.0f																											// maxDepthBounds (float)
			};
			const uint32_t numberOfColorAttachments = renderPass->getNumberOfColorAttachments();
			RENDERER_ASSERT(vulkanRenderer.getContext(), numberOfColorAttachments < 8, "Invalid number of Vulkan color attachments")
			RENDERER_ASSERT(vulkanRenderer.getContext(), numberOfColorAttachments == graphicsPipelineState.numberOfRenderTargets, "Invalid number of Vulkan color attachments")
			std::array<VkPipelineColorBlendAttachmentState, 8> vkPipelineColorBlendAttachmentStates;
			for (uint8_t i = 0; i < numberOfColorAttachments; ++i)
			{
				const Renderer::RenderTargetBlendDesc& renderTargetBlendDesc = graphicsPipelineState.blendState.renderTarget[i];
				VkPipelineColorBlendAttachmentState& vkPipelineColorBlendAttachmentState = vkPipelineColorBlendAttachmentStates[i];
				vkPipelineColorBlendAttachmentState.blendEnable			= static_cast<VkBool32>(renderTargetBlendDesc.blendEnable);				// blendEnable (VkBool32)
				vkPipelineColorBlendAttachmentState.srcColorBlendFactor	= Mapping::getVulkanBlendFactor(renderTargetBlendDesc.srcBlend);		// srcColorBlendFactor (VkBlendFactor)
				vkPipelineColorBlendAttachmentState.dstColorBlendFactor	= Mapping::getVulkanBlendFactor(renderTargetBlendDesc.destBlend);		// dstColorBlendFactor (VkBlendFactor)
				vkPipelineColorBlendAttachmentState.colorBlendOp		= Mapping::getVulkanBlendOp(renderTargetBlendDesc.blendOp);				// colorBlendOp (VkBlendOp)
				vkPipelineColorBlendAttachmentState.srcAlphaBlendFactor	= Mapping::getVulkanBlendFactor(renderTargetBlendDesc.srcBlendAlpha);	// srcAlphaBlendFactor (VkBlendFactor)
				vkPipelineColorBlendAttachmentState.dstAlphaBlendFactor	= Mapping::getVulkanBlendFactor(renderTargetBlendDesc.destBlendAlpha);	// dstAlphaBlendFactor (VkBlendFactor)
				vkPipelineColorBlendAttachmentState.alphaBlendOp		= Mapping::getVulkanBlendOp(renderTargetBlendDesc.blendOpAlpha);		// alphaBlendOp (VkBlendOp)
				vkPipelineColorBlendAttachmentState.colorWriteMask		= renderTargetBlendDesc.renderTargetWriteMask;							// colorWriteMask (VkColorComponentFlags)
			}
			const VkPipelineColorBlendStateCreateInfo vkPipelineColorBlendStateCreateInfo =
			{
				VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,	// sType (VkStructureType)
				nullptr,													// pNext (const void*)
				0,															// flags (VkPipelineColorBlendStateCreateFlags)
				VK_FALSE,													// logicOpEnable (VkBool32)
				VK_LOGIC_OP_COPY,											// logicOp (VkLogicOp)
				numberOfColorAttachments,									// attachmentCount (uint32_t)
				vkPipelineColorBlendAttachmentStates.data(),				// pAttachments (const VkPipelineColorBlendAttachmentState*)
				{ 0.0f, 0.0f, 0.0f, 0.0f }									// blendConstants[4] (float)
			};
			static constexpr std::array<VkDynamicState, 2> vkDynamicStates =
			{
				VK_DYNAMIC_STATE_VIEWPORT,
				VK_DYNAMIC_STATE_SCISSOR
			};
			static const VkPipelineDynamicStateCreateInfo vkPipelineDynamicStateCreateInfo =
			{
				VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,	// sType (VkStructureType)
				nullptr,												// pNext (const void*)
				0,														// flags (VkPipelineDynamicStateCreateFlags)
				static_cast<uint32_t>(vkDynamicStates.size()),			// dynamicStateCount (uint32_t)
				vkDynamicStates.data()									// pDynamicStates (const VkDynamicState*)
			};
			const VkGraphicsPipelineCreateInfo vkGraphicsPipelineCreateInfo =
			{
				VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,					// sType (VkStructureType)
				nullptr,															// pNext (const void*)
				0,																	// flags (VkPipelineCreateFlags)
				stageCount,															// stageCount (uint32_t)
				vkPipelineShaderStageCreateInfos.data(),							// pStages (const VkPipelineShaderStageCreateInfo*)
				&vkPipelineVertexInputStateCreateInfo,								// pVertexInputState (const VkPipelineVertexInputStateCreateInfo*)
				&vkPipelineInputAssemblyStateCreateInfo,							// pInputAssemblyState (const VkPipelineInputAssemblyStateCreateInfo*)
				&vkPipelineTessellationStateCreateInfo,								// pTessellationState (const VkPipelineTessellationStateCreateInfo*)
				&vkPipelineViewportStateCreateInfo,									// pViewportState (const VkPipelineViewportStateCreateInfo*)
				&vkPipelineRasterizationStateCreateInfo,							// pRasterizationState (const VkPipelineRasterizationStateCreateInfo*)
				&vkPipelineMultisampleStateCreateInfo,								// pMultisampleState (const VkPipelineMultisampleStateCreateInfo*)
				&vkPipelineDepthStencilStateCreateInfo,								// pDepthStencilState (const VkPipelineDepthStencilStateCreateInfo*)
				&vkPipelineColorBlendStateCreateInfo,								// pColorBlendState (const VkPipelineColorBlendStateCreateInfo*)
				&vkPipelineDynamicStateCreateInfo,									// pDynamicState (const VkPipelineDynamicStateCreateInfo*)
				static_cast<RootSignature*>(mRootSignature)->getVkPipelineLayout(),	// layout (VkPipelineLayout)
				renderPass->getVkRenderPass(),										// renderPass (VkRenderPass)
				0,																	// subpass (uint32_t)
				VK_NULL_HANDLE,														// basePipelineHandle (VkPipeline)
				0																	// basePipelineIndex (int32_t)
			};
			if (vkCreateGraphicsPipelines(vulkanRenderer.getVulkanContext().getVkDevice(), VK_NULL_HANDLE, 1, &vkGraphicsPipelineCreateInfo, vulkanRenderer.getVkAllocationCallbacks(), &mVkPipeline) != VK_SUCCESS)
			{
				RENDERER_LOG(vulkanRenderer.getContext(), CRITICAL, "Failed to create the Vulkan graphics pipeline")
			}
			else
			{
				SET_DEFAULT_DEBUG_NAME	// setDebugName("");
			}
		}

		/**
		*  @brief
		*    Destructor
		*/
		virtual ~GraphicsPipelineState() override
		{
			// Destroy the Vulkan graphics pipeline
			if (VK_NULL_HANDLE != mVkPipeline)
			{
				const VulkanRenderer& vulkanRenderer = static_cast<VulkanRenderer&>(getRenderer());
				vkDestroyPipeline(vulkanRenderer.getVulkanContext().getVkDevice(), mVkPipeline, vulkanRenderer.getVkAllocationCallbacks());
			}

			// Release the root signature, graphics program and render pass reference
			mRootSignature->releaseReference();
			mGraphicsProgram->releaseReference();
			mRenderPass->releaseReference();
		}

		/**
		*  @brief
		*    Return the Vulkan graphics pipeline
		*
		*  @return
		*    The Vulkan graphics pipeline
		*/
		inline VkPipeline getVkPipeline() const
		{
			return mVkPipeline;
		}


	//[-------------------------------------------------------]
	//[ Public virtual Renderer::IResource methods            ]
	//[-------------------------------------------------------]
	public:
		#ifdef RENDERER_DEBUG
			virtual void setDebugName(const char* name) override
			{
				if (nullptr != vkDebugMarkerSetObjectNameEXT)
				{
					Helper::setDebugObjectName(static_cast<const VulkanRenderer&>(getRenderer()).getVulkanContext().getVkDevice(), VK_DEBUG_REPORT_OBJECT_TYPE_PIPELINE_EXT, (uint64_t)mVkPipeline, name);
				}
			}
		#endif


	//[-------------------------------------------------------]
	//[ Protected virtual Renderer::RefCount methods          ]
	//[-------------------------------------------------------]
	protected:
		inline virtual void selfDestruct() override
		{
			RENDERER_DELETE(getRenderer().getContext(), GraphicsPipelineState, this);
		}


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		explicit GraphicsPipelineState(const GraphicsPipelineState& source) = delete;
		GraphicsPipelineState& operator =(const GraphicsPipelineState& source) = delete;


	//[-------------------------------------------------------]
	//[ Private data                                          ]
	//[-------------------------------------------------------]
	private:
		Renderer::IRootSignature*	mRootSignature;
		Renderer::IGraphicsProgram*	mGraphicsProgram;
		Renderer::IRenderPass*		mRenderPass;
		VkPipeline					mVkPipeline;	///< The Vulkan graphics pipeline


	};




	//[-------------------------------------------------------]
	//[ VulkanRenderer/State/ComputePipelineState.h           ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Vulkan compute pipeline state class
	*/
	class ComputePipelineState final : public Renderer::IComputePipelineState
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] vulkanRenderer
		*    Owner Vulkan renderer instance
		*  @param[in] rootSignature
		*    Root signature to use
		*  @param[in] computeShader
		*    Compute shader to use
		*/
		ComputePipelineState(VulkanRenderer& vulkanRenderer, Renderer::IRootSignature& rootSignature, Renderer::IComputeShader& computeShader) :
			IComputePipelineState(vulkanRenderer),
			mRootSignature(rootSignature),
			mComputeShader(computeShader),
			mVkPipeline(VK_NULL_HANDLE)
		{
			// Add a reference to the given root signature and compute shader
			rootSignature.addReference();
			computeShader.addReference();

			// Create the Vulkan compute pipeline
			const VkComputePipelineCreateInfo vkComputePipelineCreateInfo =
			{
				VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,							// sType (VkStructureType)
				nullptr,																// pNext (const void*)
				0,																		// flags (VkPipelineCreateFlags)
				{																		// stage (VkPipelineShaderStageCreateInfo)
					VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,				// sType (VkStructureType)
					nullptr,															// pNext (const void*)
					0,																	// flags (VkPipelineShaderStageCreateFlags)
					VK_SHADER_STAGE_COMPUTE_BIT,										// stage (VkShaderStageFlagBits)
					static_cast<ComputeShaderGlsl&>(computeShader).getVkShaderModule(),	// module (VkShaderModule)
					"main",																// pName (const char*)
					nullptr																// pSpecializationInfo (const VkSpecializationInfo*)
				},
				static_cast<RootSignature&>(rootSignature).getVkPipelineLayout(),		// layout (VkPipelineLayout)
				VK_NULL_HANDLE,															// basePipelineHandle (VkPipeline)
				0																		// basePipelineIndex (int32_t)
			};
			if (vkCreateComputePipelines(vulkanRenderer.getVulkanContext().getVkDevice(), VK_NULL_HANDLE, 1, &vkComputePipelineCreateInfo, vulkanRenderer.getVkAllocationCallbacks(), &mVkPipeline) != VK_SUCCESS)
			{
				RENDERER_LOG(vulkanRenderer.getContext(), CRITICAL, "Failed to create the Vulkan compute pipeline")
			}
			else
			{
				SET_DEFAULT_DEBUG_NAME	// setDebugName("");
			}
		}

		/**
		*  @brief
		*    Destructor
		*/
		virtual ~ComputePipelineState() override
		{
			// Destroy the Vulkan compute pipeline
			if (VK_NULL_HANDLE != mVkPipeline)
			{
				const VulkanRenderer& vulkanRenderer = static_cast<VulkanRenderer&>(getRenderer());
				vkDestroyPipeline(vulkanRenderer.getVulkanContext().getVkDevice(), mVkPipeline, vulkanRenderer.getVkAllocationCallbacks());
			}

			// Release the root signature and compute shader reference
			mRootSignature.releaseReference();
			mComputeShader.releaseReference();
		}

		/**
		*  @brief
		*    Return the Vulkan compute pipeline
		*
		*  @return
		*    The Vulkan compute pipeline
		*/
		inline VkPipeline getVkPipeline() const
		{
			return mVkPipeline;
		}


	//[-------------------------------------------------------]
	//[ Public virtual Renderer::IResource methods            ]
	//[-------------------------------------------------------]
	public:
		#ifdef RENDERER_DEBUG
			virtual void setDebugName(const char* name) override
			{
				if (nullptr != vkDebugMarkerSetObjectNameEXT)
				{
					Helper::setDebugObjectName(static_cast<const VulkanRenderer&>(getRenderer()).getVulkanContext().getVkDevice(), VK_DEBUG_REPORT_OBJECT_TYPE_PIPELINE_EXT, (uint64_t)mVkPipeline, name);
				}
			}
		#endif


	//[-------------------------------------------------------]
	//[ Protected virtual Renderer::RefCount methods          ]
	//[-------------------------------------------------------]
	protected:
		inline virtual void selfDestruct() override
		{
			RENDERER_DELETE(getRenderer().getContext(), ComputePipelineState, this);
		}


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		explicit ComputePipelineState(const ComputePipelineState& source) = delete;
		ComputePipelineState& operator =(const ComputePipelineState& source) = delete;


	//[-------------------------------------------------------]
	//[ Private data                                          ]
	//[-------------------------------------------------------]
	private:
		Renderer::IRootSignature& mRootSignature;
		Renderer::IComputeShader& mComputeShader;
		VkPipeline				  mVkPipeline;		///< The Vulkan compute pipeline


	};




	//[-------------------------------------------------------]
	//[ VulkanRenderer/ResourceGroup.h                        ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Vulkan resource group class
	*/
	class ResourceGroup final : public Renderer::IResourceGroup
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] rootSignature
		*    Root signature
		*  @param[in] rootParameterIndex
		*    Root parameter index
		*  @param[in] vkDescriptorSet
		*    Wrapped Vulkan descriptor set
		*  @param[in] numberOfResources
		*    Number of resources, having no resources is invalid
		*  @param[in] resources
		*    At least "numberOfResources" resource pointers, must be valid, the resource group will keep a reference to the resources
		*  @param[in] samplerStates
		*    If not a null pointer at least "numberOfResources" sampler state pointers, must be valid if there's at least one texture resource, the resource group will keep a reference to the sampler states
		*/
		ResourceGroup(RootSignature& rootSignature, uint32_t rootParameterIndex, VkDescriptorSet vkDescriptorSet, uint32_t numberOfResources, Renderer::IResource** resources, Renderer::ISamplerState** samplerStates) :
			IResourceGroup(static_cast<VulkanRenderer&>(rootSignature.getRenderer())),
			mRootSignature(rootSignature),
			mVkDescriptorSet(vkDescriptorSet),
			mNumberOfResources(numberOfResources),
			mResources(RENDERER_MALLOC_TYPED(rootSignature.getRenderer().getContext(), Renderer::IResource*, mNumberOfResources)),
			mSamplerStates(nullptr)
		{
			mRootSignature.addReference();

			// Process all resources and add our reference to the renderer resource
			const VulkanRenderer& vulkanRenderer = static_cast<VulkanRenderer&>(getRenderer());
			const VkDevice vkDevice = vulkanRenderer.getVulkanContext().getVkDevice();
			if (nullptr != samplerStates)
			{
				mSamplerStates = RENDERER_MALLOC_TYPED(vulkanRenderer.getContext(), Renderer::ISamplerState*, mNumberOfResources);
				for (uint32_t resourceIndex = 0; resourceIndex < mNumberOfResources; ++resourceIndex)
				{
					Renderer::ISamplerState* samplerState = mSamplerStates[resourceIndex] = samplerStates[resourceIndex];
					if (nullptr != samplerState)
					{
						samplerState->addReference();
					}
				}
			}
			for (uint32_t resourceIndex = 0; resourceIndex < mNumberOfResources; ++resourceIndex, ++resources)
			{
				Renderer::IResource* resource = *resources;
				RENDERER_ASSERT(vulkanRenderer.getContext(), nullptr != resource, "Invalid Vulkan resource")
				mResources[resourceIndex] = resource;
				resource->addReference();

				// Check the type of resource to set
				// TODO(co) Some additional resource type root signature security checks in debug build?
				const Renderer::ResourceType resourceType = resource->getResourceType();
				switch (resourceType)
				{
					case Renderer::ResourceType::INDEX_BUFFER:
					{
						const VkDescriptorBufferInfo vkDescriptorBufferInfo =
						{
							static_cast<IndexBuffer*>(resource)->getVkBuffer(),	// buffer (VkBuffer)
							0,													// offset (VkDeviceSize)
							VK_WHOLE_SIZE										// range (VkDeviceSize)
						};
						const VkWriteDescriptorSet vkWriteDescriptorSet =
						{
							VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,		// sType (VkStructureType)
							nullptr,									// pNext (const void*)
							mVkDescriptorSet,							// dstSet (VkDescriptorSet)
							resourceIndex,								// dstBinding (uint32_t)
							0,											// dstArrayElement (uint32_t)
							1,											// descriptorCount (uint32_t)
							VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,			// descriptorType (VkDescriptorType)
							nullptr,									// pImageInfo (const VkDescriptorImageInfo*)
							&vkDescriptorBufferInfo,					// pBufferInfo (const VkDescriptorBufferInfo*)
							nullptr										// pTexelBufferView (const VkBufferView*)
						};
						vkUpdateDescriptorSets(vkDevice, 1, &vkWriteDescriptorSet, 0, nullptr);
						break;
					}

					case Renderer::ResourceType::VERTEX_BUFFER:
					{
						const VkDescriptorBufferInfo vkDescriptorBufferInfo =
						{
							static_cast<VertexBuffer*>(resource)->getVkBuffer(),	// buffer (VkBuffer)
							0,														// offset (VkDeviceSize)
							VK_WHOLE_SIZE											// range (VkDeviceSize)
						};
						const VkWriteDescriptorSet vkWriteDescriptorSet =
						{
							VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,		// sType (VkStructureType)
							nullptr,									// pNext (const void*)
							mVkDescriptorSet,							// dstSet (VkDescriptorSet)
							resourceIndex,								// dstBinding (uint32_t)
							0,											// dstArrayElement (uint32_t)
							1,											// descriptorCount (uint32_t)
							VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,			// descriptorType (VkDescriptorType)
							nullptr,									// pImageInfo (const VkDescriptorImageInfo*)
							&vkDescriptorBufferInfo,					// pBufferInfo (const VkDescriptorBufferInfo*)
							nullptr										// pTexelBufferView (const VkBufferView*)
						};
						vkUpdateDescriptorSets(vkDevice, 1, &vkWriteDescriptorSet, 0, nullptr);
						break;
					}

					case Renderer::ResourceType::TEXTURE_BUFFER:
					{
						const Renderer::DescriptorRange& descriptorRange = reinterpret_cast<const Renderer::DescriptorRange*>(rootSignature.getRootSignature().parameters[rootParameterIndex].descriptorTable.descriptorRanges)[resourceIndex];
						RENDERER_ASSERT(vulkanRenderer.getContext(), Renderer::DescriptorRangeType::SRV == descriptorRange.rangeType || Renderer::DescriptorRangeType::UAV == descriptorRange.rangeType, "Vulkan texture buffer must bound at SRV or UAV descriptor range type")
						const VkBufferView vkBufferView = static_cast<TextureBuffer*>(resource)->getVkBufferView();
						const VkWriteDescriptorSet vkWriteDescriptorSet =
						{
							VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,																													// sType (VkStructureType)
							nullptr,																																				// pNext (const void*)
							mVkDescriptorSet,																																		// dstSet (VkDescriptorSet)
							resourceIndex,																																			// dstBinding (uint32_t)
							0,																																						// dstArrayElement (uint32_t)
							1,																																						// descriptorCount (uint32_t)
							(Renderer::DescriptorRangeType::SRV == descriptorRange.rangeType) ? VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER : VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER,	// descriptorType (VkDescriptorType)
							nullptr,																																				// pImageInfo (const VkDescriptorImageInfo*)
							nullptr,																																				// pBufferInfo (const VkDescriptorBufferInfo*)
							&vkBufferView																																			// pTexelBufferView (const VkBufferView*)
						};
						vkUpdateDescriptorSets(vkDevice, 1, &vkWriteDescriptorSet, 0, nullptr);
						break;
					}

					case Renderer::ResourceType::STRUCTURED_BUFFER:
					{
						MAYBE_UNUSED const Renderer::DescriptorRange& descriptorRange = reinterpret_cast<const Renderer::DescriptorRange*>(rootSignature.getRootSignature().parameters[rootParameterIndex].descriptorTable.descriptorRanges)[resourceIndex];
						RENDERER_ASSERT(vulkanRenderer.getContext(), Renderer::DescriptorRangeType::SRV == descriptorRange.rangeType || Renderer::DescriptorRangeType::UAV == descriptorRange.rangeType, "Vulkan structured buffer must bound at SRV or UAV descriptor range type")
						const VkDescriptorBufferInfo vkDescriptorBufferInfo =
						{
							static_cast<StructuredBuffer*>(resource)->getVkBuffer(),	// buffer (VkBuffer)
							0,															// offset (VkDeviceSize)
							VK_WHOLE_SIZE												// range (VkDeviceSize)
						};
						const VkWriteDescriptorSet vkWriteDescriptorSet =
						{
							VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,	// sType (VkStructureType)
							nullptr,								// pNext (const void*)
							mVkDescriptorSet,						// dstSet (VkDescriptorSet)
							resourceIndex,							// dstBinding (uint32_t)
							0,										// dstArrayElement (uint32_t)
							1,										// descriptorCount (uint32_t)
							VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,		// descriptorType (VkDescriptorType)
							nullptr,								// pImageInfo (const VkDescriptorImageInfo*)
							&vkDescriptorBufferInfo,				// pBufferInfo (const VkDescriptorBufferInfo*)
							nullptr									// pTexelBufferView (const VkBufferView*)
						};
						vkUpdateDescriptorSets(vkDevice, 1, &vkWriteDescriptorSet, 0, nullptr);
						break;
					}

					case Renderer::ResourceType::INDIRECT_BUFFER:
					{
						const VkDescriptorBufferInfo vkDescriptorBufferInfo =
						{
							static_cast<IndirectBuffer*>(resource)->getVkBuffer(),	// buffer (VkBuffer)
							0,														// offset (VkDeviceSize)
							VK_WHOLE_SIZE											// range (VkDeviceSize)
						};
						const VkWriteDescriptorSet vkWriteDescriptorSet =
						{
							VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,		// sType (VkStructureType)
							nullptr,									// pNext (const void*)
							mVkDescriptorSet,							// dstSet (VkDescriptorSet)
							resourceIndex,								// dstBinding (uint32_t)
							0,											// dstArrayElement (uint32_t)
							1,											// descriptorCount (uint32_t)
							VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,			// descriptorType (VkDescriptorType)
							nullptr,									// pImageInfo (const VkDescriptorImageInfo*)
							&vkDescriptorBufferInfo,					// pBufferInfo (const VkDescriptorBufferInfo*)
							nullptr										// pTexelBufferView (const VkBufferView*)
						};
						vkUpdateDescriptorSets(vkDevice, 1, &vkWriteDescriptorSet, 0, nullptr);
						break;
					}

					case Renderer::ResourceType::UNIFORM_BUFFER:
					{
						const VkDescriptorBufferInfo vkDescriptorBufferInfo =
						{
							static_cast<UniformBuffer*>(resource)->getVkBuffer(),	// buffer (VkBuffer)
							0,														// offset (VkDeviceSize)
							VK_WHOLE_SIZE											// range (VkDeviceSize)
						};
						const VkWriteDescriptorSet vkWriteDescriptorSet =
						{
							VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,	// sType (VkStructureType)
							nullptr,								// pNext (const void*)
							mVkDescriptorSet,						// dstSet (VkDescriptorSet)
							resourceIndex,							// dstBinding (uint32_t)
							0,										// dstArrayElement (uint32_t)
							1,										// descriptorCount (uint32_t)
							VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,		// descriptorType (VkDescriptorType)
							nullptr,								// pImageInfo (const VkDescriptorImageInfo*)
							&vkDescriptorBufferInfo,				// pBufferInfo (const VkDescriptorBufferInfo*)
							nullptr									// pTexelBufferView (const VkBufferView*)
						};
						vkUpdateDescriptorSets(vkDevice, 1, &vkWriteDescriptorSet, 0, nullptr);
						break;
					}

					case Renderer::ResourceType::TEXTURE_1D:
					case Renderer::ResourceType::TEXTURE_2D:
					case Renderer::ResourceType::TEXTURE_2D_ARRAY:
					case Renderer::ResourceType::TEXTURE_3D:
					case Renderer::ResourceType::TEXTURE_CUBE:
					{
						// Evaluate the texture type and get the Vulkan image view
						VkImageView vkImageView = VK_NULL_HANDLE;
						VkImageLayout vkImageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
						switch (resourceType)
						{
							case Renderer::ResourceType::TEXTURE_1D:
							{
								const Texture1D* texture1D = static_cast<Texture1D*>(resource);
								vkImageView = texture1D->getVkImageView();
								vkImageLayout = texture1D->getVkImageLayout();
								break;
							}

							case Renderer::ResourceType::TEXTURE_2D:
							{
								const Texture2D* texture2D = static_cast<Texture2D*>(resource);
								vkImageView = texture2D->getVkImageView();
								vkImageLayout = texture2D->getVkImageLayout();
								break;
							}

							case Renderer::ResourceType::TEXTURE_2D_ARRAY:
							{
								const Texture2DArray* texture2DArray = static_cast<Texture2DArray*>(resource);
								vkImageView = texture2DArray->getVkImageView();
								vkImageLayout = texture2DArray->getVkImageLayout();
								break;
							}

							case Renderer::ResourceType::TEXTURE_3D:
							{
								const Texture3D* texture3D = static_cast<Texture3D*>(resource);
								vkImageView = texture3D->getVkImageView();
								vkImageLayout = texture3D->getVkImageLayout();
								break;
							}

							case Renderer::ResourceType::TEXTURE_CUBE:
							{
								const TextureCube* textureCube = static_cast<TextureCube*>(resource);
								vkImageView = textureCube->getVkImageView();
								vkImageLayout = textureCube->getVkImageLayout();
								break;
							}

							case Renderer::ResourceType::ROOT_SIGNATURE:
							case Renderer::ResourceType::RESOURCE_GROUP:
							case Renderer::ResourceType::GRAPHICS_PROGRAM:
							case Renderer::ResourceType::VERTEX_ARRAY:
							case Renderer::ResourceType::RENDER_PASS:
							case Renderer::ResourceType::SWAP_CHAIN:
							case Renderer::ResourceType::FRAMEBUFFER:
							case Renderer::ResourceType::INDEX_BUFFER:
							case Renderer::ResourceType::VERTEX_BUFFER:
							case Renderer::ResourceType::INDIRECT_BUFFER:
							case Renderer::ResourceType::TEXTURE_BUFFER:
							case Renderer::ResourceType::STRUCTURED_BUFFER:
							case Renderer::ResourceType::UNIFORM_BUFFER:
							case Renderer::ResourceType::GRAPHICS_PIPELINE_STATE:
							case Renderer::ResourceType::COMPUTE_PIPELINE_STATE:
							case Renderer::ResourceType::SAMPLER_STATE:
							case Renderer::ResourceType::VERTEX_SHADER:
							case Renderer::ResourceType::TESSELLATION_CONTROL_SHADER:
							case Renderer::ResourceType::TESSELLATION_EVALUATION_SHADER:
							case Renderer::ResourceType::GEOMETRY_SHADER:
							case Renderer::ResourceType::FRAGMENT_SHADER:
							case Renderer::ResourceType::COMPUTE_SHADER:
								RENDERER_LOG(vulkanRenderer.getContext(), CRITICAL, "Invalid Vulkan renderer backend resource type")
								break;
						}

						// Get the sampler state
						const SamplerState* samplerState = (nullptr != mSamplerStates) ? static_cast<const SamplerState*>(mSamplerStates[resourceIndex]) : nullptr;

						// Update Vulkan descriptor sets
						const VkDescriptorImageInfo vkDescriptorImageInfo =
						{
							(nullptr != samplerState) ? samplerState->getVkSampler() : VK_NULL_HANDLE,	// sampler (VkSampler)
							vkImageView,																// imageView (VkImageView)
							vkImageLayout																// imageLayout (VkImageLayout)
						};
						const VkWriteDescriptorSet vkWriteDescriptorSet =
						{
							VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,																		// sType (VkStructureType)
							nullptr,																									// pNext (const void*)
							mVkDescriptorSet,																							// dstSet (VkDescriptorSet)
							resourceIndex,																								// dstBinding (uint32_t)
							0,																											// dstArrayElement (uint32_t)
							1,																											// descriptorCount (uint32_t)
							(nullptr != samplerState) ? VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER : VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,	// descriptorType (VkDescriptorType)
							&vkDescriptorImageInfo,																						// pImageInfo (const VkDescriptorImageInfo*)
							nullptr,																									// pBufferInfo (const VkDescriptorBufferInfo*)
							nullptr																										// pTexelBufferView (const VkBufferView*)
						};
						vkUpdateDescriptorSets(vkDevice, 1, &vkWriteDescriptorSet, 0, nullptr);
						break;
					}

					case Renderer::ResourceType::SAMPLER_STATE:
						// Nothing to do in here, Vulkan is using combined image samplers
						break;

					case Renderer::ResourceType::ROOT_SIGNATURE:
					case Renderer::ResourceType::RESOURCE_GROUP:
					case Renderer::ResourceType::GRAPHICS_PROGRAM:
					case Renderer::ResourceType::VERTEX_ARRAY:
					case Renderer::ResourceType::RENDER_PASS:
					case Renderer::ResourceType::SWAP_CHAIN:
					case Renderer::ResourceType::FRAMEBUFFER:
					case Renderer::ResourceType::GRAPHICS_PIPELINE_STATE:
					case Renderer::ResourceType::COMPUTE_PIPELINE_STATE:
					case Renderer::ResourceType::VERTEX_SHADER:
					case Renderer::ResourceType::TESSELLATION_CONTROL_SHADER:
					case Renderer::ResourceType::TESSELLATION_EVALUATION_SHADER:
					case Renderer::ResourceType::GEOMETRY_SHADER:
					case Renderer::ResourceType::FRAGMENT_SHADER:
					case Renderer::ResourceType::COMPUTE_SHADER:
						RENDERER_LOG(vulkanRenderer.getContext(), CRITICAL, "Invalid Vulkan renderer backend resource type")
						break;
				}
			}

			SET_DEFAULT_DEBUG_NAME	// setDebugName("");
		}

		/**
		*  @brief
		*    Destructor
		*/
		virtual ~ResourceGroup() override
		{
			// Remove our reference from the renderer resources
			const Renderer::Context& context = getRenderer().getContext();
			if (nullptr != mSamplerStates)
			{
				for (uint32_t resourceIndex = 0; resourceIndex < mNumberOfResources; ++resourceIndex)
				{
					Renderer::ISamplerState* samplerState = mSamplerStates[resourceIndex];
					if (nullptr != samplerState)
					{
						samplerState->releaseReference();
					}
				}
				RENDERER_FREE(context, mSamplerStates);
			}
			for (uint32_t resourceIndex = 0; resourceIndex < mNumberOfResources; ++resourceIndex)
			{
				mResources[resourceIndex]->releaseReference();
			}
			RENDERER_FREE(context, mResources);

			// Free Vulkan descriptor set
			if (VK_NULL_HANDLE != mVkDescriptorSet)
			{
				vkFreeDescriptorSets(static_cast<VulkanRenderer&>(mRootSignature.getRenderer()).getVulkanContext().getVkDevice(), mRootSignature.getVkDescriptorPool(), 1, &mVkDescriptorSet);
			}
			mRootSignature.releaseReference();
		}

		/**
		*  @brief
		*    Return the Vulkan descriptor set
		*
		*  @return
		*    The Vulkan descriptor set, can be a null handle
		*/
		inline VkDescriptorSet getVkDescriptorSet() const
		{
			return mVkDescriptorSet;
		}


	//[-------------------------------------------------------]
	//[ Public virtual Renderer::IResource methods            ]
	//[-------------------------------------------------------]
	public:
		#ifdef RENDERER_DEBUG
			virtual void setDebugName(const char* name) override
			{
				if (nullptr != vkDebugMarkerSetObjectNameEXT)
				{
					Helper::setDebugObjectName(static_cast<const VulkanRenderer&>(getRenderer()).getVulkanContext().getVkDevice(), VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_SET_EXT, (uint64_t)mVkDescriptorSet, name);
				}
			}
		#endif


	//[-------------------------------------------------------]
	//[ Protected virtual Renderer::RefCount methods          ]
	//[-------------------------------------------------------]
	protected:
		inline virtual void selfDestruct() override
		{
			RENDERER_DELETE(getRenderer().getContext(), ResourceGroup, this);
		}


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		explicit ResourceGroup(const ResourceGroup& source) = delete;
		ResourceGroup& operator =(const ResourceGroup& source) = delete;


	//[-------------------------------------------------------]
	//[ Private data                                          ]
	//[-------------------------------------------------------]
	private:
		RootSignature&			  mRootSignature;		///< Root signature
		VkDescriptorSet			  mVkDescriptorSet;		///< "mVkDescriptorPool" of the root signature is the owner which manages the memory, can be a null handle (e.g. for a sampler resource group)
		uint32_t				  mNumberOfResources;	///< Number of resources this resource group groups together
		Renderer::IResource**	  mResources;			///< Renderer resource, we keep a reference to it
		Renderer::ISamplerState** mSamplerStates;		///< Sampler states, we keep a reference to it


	};

	// TODO(co) Try to somehow simplify the internal dependencies to be able to put this method directly into the class
	Renderer::IResourceGroup* RootSignature::createResourceGroup(uint32_t rootParameterIndex, uint32_t numberOfResources, Renderer::IResource** resources, Renderer::ISamplerState** samplerStates)
	{
		VulkanRenderer& vulkanRenderer = static_cast<VulkanRenderer&>(getRenderer());
		const Renderer::Context& context = vulkanRenderer.getContext();

		// Sanity checks
		RENDERER_ASSERT(context, VK_NULL_HANDLE != mVkDescriptorPool, "The Vulkan descriptor pool instance must be valid")
		RENDERER_ASSERT(context, rootParameterIndex < mVkDescriptorSetLayouts.size(), "The Vulkan root parameter index is out-of-bounds")
		RENDERER_ASSERT(context, numberOfResources > 0, "The number of Vulkan resources must not be zero")
		RENDERER_ASSERT(context, nullptr != resources, "The Vulkan resource pointers must be valid")

		// Allocate Vulkan descriptor set
		VkDescriptorSet vkDescriptorSet = VK_NULL_HANDLE;
		if ((*resources)->getResourceType() != Renderer::ResourceType::SAMPLER_STATE)
		{
			const VkDescriptorSetAllocateInfo vkDescriptorSetAllocateInfo =
			{
				VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,	// sType (VkStructureType)
				nullptr,										// pNext (const void*)
				mVkDescriptorPool,								// descriptorPool (VkDescriptorPool)
				1,												// descriptorSetCount (uint32_t)
				&mVkDescriptorSetLayouts[rootParameterIndex]	// pSetLayouts (const VkDescriptorSetLayout*)
			};
			if (vkAllocateDescriptorSets(vulkanRenderer.getVulkanContext().getVkDevice(), &vkDescriptorSetAllocateInfo, &vkDescriptorSet) != VK_SUCCESS)
			{
				RENDERER_LOG(context, CRITICAL, "Failed to allocate the Vulkan descriptor set")
			}
		}

		// Create resource group
		return RENDERER_NEW(context, ResourceGroup)(*this, rootParameterIndex, vkDescriptorSet, numberOfResources, resources, samplerStates);
	}




//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
} // VulkanRenderer




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
		// Implementation from "08/02/2015 Better array 'countof' implementation with C++ 11 (updated)" - https://www.g-truc.net/post-0708.html
		template<typename T, std::size_t N>
		constexpr std::size_t countof(T const (&)[N])
		{
			return N;
		}

		VKAPI_ATTR void* VKAPI_CALL vkAllocationFunction(void* pUserData, size_t size, size_t alignment, VkSystemAllocationScope)
		{
			return reinterpret_cast<Renderer::IAllocator*>(pUserData)->reallocate(nullptr, 0, size, alignment);
		}

		VKAPI_ATTR void* VKAPI_CALL vkReallocationFunction(void* pUserData, void* pOriginal, size_t size, size_t alignment, VkSystemAllocationScope)
		{
			return reinterpret_cast<Renderer::IAllocator*>(pUserData)->reallocate(pOriginal, 0, size, alignment);
		}

		VKAPI_ATTR void VKAPI_CALL vkFreeFunction(void* pUserData, void* pMemory)
		{
			reinterpret_cast<Renderer::IAllocator*>(pUserData)->reallocate(pMemory, 0, 0, 1);
		}

		namespace BackendDispatch
		{


			//[-------------------------------------------------------]
			//[ Command buffer                                        ]
			//[-------------------------------------------------------]
			void ExecuteCommandBuffer(const void* data, Renderer::IRenderer& renderer)
			{
				const Renderer::Command::ExecuteCommandBuffer* realData = static_cast<const Renderer::Command::ExecuteCommandBuffer*>(data);
				RENDERER_ASSERT(renderer.getContext(), nullptr != realData->commandBufferToExecute, "The Vulkan command buffer to execute must be valid")
				renderer.submitCommandBuffer(*realData->commandBufferToExecute);
			}

			//[-------------------------------------------------------]
			//[ Graphics states                                       ]
			//[-------------------------------------------------------]
			void SetGraphicsRootSignature(const void* data, Renderer::IRenderer& renderer)
			{
				const Renderer::Command::SetGraphicsRootSignature* realData = static_cast<const Renderer::Command::SetGraphicsRootSignature*>(data);
				static_cast<VulkanRenderer::VulkanRenderer&>(renderer).setGraphicsRootSignature(realData->rootSignature);
			}

			void SetGraphicsPipelineState(const void* data, Renderer::IRenderer& renderer)
			{
				const Renderer::Command::SetGraphicsPipelineState* realData = static_cast<const Renderer::Command::SetGraphicsPipelineState*>(data);
				static_cast<VulkanRenderer::VulkanRenderer&>(renderer).setGraphicsPipelineState(realData->graphicsPipelineState);
			}

			void SetGraphicsResourceGroup(const void* data, Renderer::IRenderer& renderer)
			{
				const Renderer::Command::SetGraphicsResourceGroup* realData = static_cast<const Renderer::Command::SetGraphicsResourceGroup*>(data);
				static_cast<VulkanRenderer::VulkanRenderer&>(renderer).setGraphicsResourceGroup(realData->rootParameterIndex, realData->resourceGroup);
			}

			void SetGraphicsVertexArray(const void* data, Renderer::IRenderer& renderer)
			{
				// Input-assembler (IA) stage
				const Renderer::Command::SetGraphicsVertexArray* realData = static_cast<const Renderer::Command::SetGraphicsVertexArray*>(data);
				static_cast<VulkanRenderer::VulkanRenderer&>(renderer).setGraphicsVertexArray(realData->vertexArray);
			}

			void SetGraphicsViewports(const void* data, Renderer::IRenderer& renderer)
			{
				// Rasterizer (RS) stage
				const Renderer::Command::SetGraphicsViewports* realData = static_cast<const Renderer::Command::SetGraphicsViewports*>(data);
				static_cast<VulkanRenderer::VulkanRenderer&>(renderer).setGraphicsViewports(realData->numberOfViewports, (nullptr != realData->viewports) ? realData->viewports : reinterpret_cast<const Renderer::Viewport*>(Renderer::CommandPacketHelper::getAuxiliaryMemory(realData)));
			}

			void SetGraphicsScissorRectangles(const void* data, Renderer::IRenderer& renderer)
			{
				// Rasterizer (RS) stage
				const Renderer::Command::SetGraphicsScissorRectangles* realData = static_cast<const Renderer::Command::SetGraphicsScissorRectangles*>(data);
				static_cast<VulkanRenderer::VulkanRenderer&>(renderer).setGraphicsScissorRectangles(realData->numberOfScissorRectangles, (nullptr != realData->scissorRectangles) ? realData->scissorRectangles : reinterpret_cast<const Renderer::ScissorRectangle*>(Renderer::CommandPacketHelper::getAuxiliaryMemory(realData)));
			}

			void SetGraphicsRenderTarget(const void* data, Renderer::IRenderer& renderer)
			{
				// Output-merger (OM) stage
				const Renderer::Command::SetGraphicsRenderTarget* realData = static_cast<const Renderer::Command::SetGraphicsRenderTarget*>(data);
				static_cast<VulkanRenderer::VulkanRenderer&>(renderer).setGraphicsRenderTarget(realData->renderTarget);
			}

			void ClearGraphics(const void* data, Renderer::IRenderer& renderer)
			{
				const Renderer::Command::ClearGraphics* realData = static_cast<const Renderer::Command::ClearGraphics*>(data);
				static_cast<VulkanRenderer::VulkanRenderer&>(renderer).clearGraphics(realData->clearFlags, realData->color, realData->z, realData->stencil);
			}

			void DrawGraphics(const void* data, Renderer::IRenderer& renderer)
			{
				const Renderer::Command::DrawGraphics* realData = static_cast<const Renderer::Command::DrawGraphics*>(data);
				if (nullptr != realData->indirectBuffer)
				{
					static_cast<VulkanRenderer::VulkanRenderer&>(renderer).drawGraphics(*realData->indirectBuffer, realData->indirectBufferOffset, realData->numberOfDraws);
				}
				else
				{
					static_cast<VulkanRenderer::VulkanRenderer&>(renderer).drawGraphicsEmulated(Renderer::CommandPacketHelper::getAuxiliaryMemory(realData), realData->indirectBufferOffset, realData->numberOfDraws);
				}
			}

			void DrawIndexedGraphics(const void* data, Renderer::IRenderer& renderer)
			{
				const Renderer::Command::DrawIndexedGraphics* realData = static_cast<const Renderer::Command::DrawIndexedGraphics*>(data);
				if (nullptr != realData->indirectBuffer)
				{
					static_cast<VulkanRenderer::VulkanRenderer&>(renderer).drawIndexedGraphics(*realData->indirectBuffer, realData->indirectBufferOffset, realData->numberOfDraws);
				}
				else
				{
					static_cast<VulkanRenderer::VulkanRenderer&>(renderer).drawIndexedGraphicsEmulated(Renderer::CommandPacketHelper::getAuxiliaryMemory(realData), realData->indirectBufferOffset, realData->numberOfDraws);
				}
			}

			//[-------------------------------------------------------]
			//[ Compute                                               ]
			//[-------------------------------------------------------]
			void SetComputeRootSignature(const void* data, Renderer::IRenderer& renderer)
			{
				const Renderer::Command::SetComputeRootSignature* realData = static_cast<const Renderer::Command::SetComputeRootSignature*>(data);
				static_cast<VulkanRenderer::VulkanRenderer&>(renderer).setComputeRootSignature(realData->rootSignature);
			}

			void SetComputePipelineState(const void* data, Renderer::IRenderer& renderer)
			{
				const Renderer::Command::SetComputePipelineState* realData = static_cast<const Renderer::Command::SetComputePipelineState*>(data);
				static_cast<VulkanRenderer::VulkanRenderer&>(renderer).setComputePipelineState(realData->computePipelineState);
			}

			void SetComputeResourceGroup(const void* data, Renderer::IRenderer& renderer)
			{
				const Renderer::Command::SetComputeResourceGroup* realData = static_cast<const Renderer::Command::SetComputeResourceGroup*>(data);
				static_cast<VulkanRenderer::VulkanRenderer&>(renderer).setComputeResourceGroup(realData->rootParameterIndex, realData->resourceGroup);
			}

			void DispatchCompute(const void* data, Renderer::IRenderer& renderer)
			{
				const Renderer::Command::DispatchCompute* realData = static_cast<const Renderer::Command::DispatchCompute*>(data);
				vkCmdDispatch(static_cast<VulkanRenderer::VulkanRenderer&>(renderer).getVulkanContext().getVkCommandBuffer(), realData->groupCountX, realData->groupCountY, realData->groupCountZ);
			}

			//[-------------------------------------------------------]
			//[ Resource                                              ]
			//[-------------------------------------------------------]
			void SetTextureMinimumMaximumMipmapIndex(const void* data, Renderer::IRenderer& renderer)
			{
				const Renderer::Command::SetTextureMinimumMaximumMipmapIndex* realData = static_cast<const Renderer::Command::SetTextureMinimumMaximumMipmapIndex*>(data);
				if (realData->texture->getResourceType() == Renderer::ResourceType::TEXTURE_2D)
				{
					static_cast<VulkanRenderer::Texture2D*>(realData->texture)->setMinimumMaximumMipmapIndex(realData->minimumMipmapIndex, realData->maximumMipmapIndex);
				}
				else
				{
					RENDERER_LOG(static_cast<VulkanRenderer::VulkanRenderer&>(renderer).getContext(), CRITICAL, "Unsupported Vulkan texture resource type")
				}
			}

			void ResolveMultisampleFramebuffer(const void* data, Renderer::IRenderer& renderer)
			{
				const Renderer::Command::ResolveMultisampleFramebuffer* realData = static_cast<const Renderer::Command::ResolveMultisampleFramebuffer*>(data);
				static_cast<VulkanRenderer::VulkanRenderer&>(renderer).resolveMultisampleFramebuffer(*realData->destinationRenderTarget, *realData->sourceMultisampleFramebuffer);
			}

			void CopyResource(const void* data, Renderer::IRenderer& renderer)
			{
				const Renderer::Command::CopyResource* realData = static_cast<const Renderer::Command::CopyResource*>(data);
				static_cast<VulkanRenderer::VulkanRenderer&>(renderer).copyResource(*realData->destinationResource, *realData->sourceResource);
			}

			//[-------------------------------------------------------]
			//[ Debug                                                 ]
			//[-------------------------------------------------------]
			#ifdef RENDERER_DEBUG
				void SetDebugMarker(const void* data, Renderer::IRenderer& renderer)
				{
					const Renderer::Command::SetDebugMarker* realData = static_cast<const Renderer::Command::SetDebugMarker*>(data);
					static_cast<VulkanRenderer::VulkanRenderer&>(renderer).setDebugMarker(realData->name);
				}

				void BeginDebugEvent(const void* data, Renderer::IRenderer& renderer)
				{
					const Renderer::Command::BeginDebugEvent* realData = static_cast<const Renderer::Command::BeginDebugEvent*>(data);
					static_cast<VulkanRenderer::VulkanRenderer&>(renderer).beginDebugEvent(realData->name);
				}

				void EndDebugEvent(const void*, Renderer::IRenderer& renderer)
				{
					static_cast<VulkanRenderer::VulkanRenderer&>(renderer).endDebugEvent();
				}
			#else
				void SetDebugMarker(const void*, Renderer::IRenderer&)
				{
					// Nothing here
				}

				void BeginDebugEvent(const void*, Renderer::IRenderer&)
				{
					// Nothing here
				}

				void EndDebugEvent(const void*, Renderer::IRenderer&)
				{
					// Nothing here
				}
			#endif


		}

		void beginVulkanRenderPass(const Renderer::IRenderTarget& renderTarget, VkRenderPass vkRenderPass, VkFramebuffer vkFramebuffer, uint32_t numberOfAttachments, const VulkanRenderer::VulkanRenderer::VkClearValues& vkClearValues, VkCommandBuffer vkCommandBuffer)
		{
			// Get render target dimension
			uint32_t width = 1;
			uint32_t height = 1;
			renderTarget.getWidthAndHeight(width, height);

			// Begin Vulkan render pass
			const VkRenderPassBeginInfo vkRenderPassBeginInfo =
			{
				VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,	// sType (VkStructureType)
				nullptr,									// pNext (const void*)
				vkRenderPass,								// renderPass (VkRenderPass)
				vkFramebuffer,								// framebuffer (VkFramebuffer)
				{ // renderArea (VkRect2D)
					{ 0, 0 },								// offset (VkOffset2D)
					{ width, height }						// extent (VkExtent2D)
				},
				numberOfAttachments,						// clearValueCount (uint32_t)
				vkClearValues.data()						// pClearValues (const VkClearValue*)
			};
			vkCmdBeginRenderPass(vkCommandBuffer, &vkRenderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
		}


		//[-------------------------------------------------------]
		//[ Global definitions                                    ]
		//[-------------------------------------------------------]
		static constexpr Renderer::BackendDispatchFunction DISPATCH_FUNCTIONS[Renderer::CommandDispatchFunctionIndex::NumberOfFunctions] =
		{
			// Command buffer
			&BackendDispatch::ExecuteCommandBuffer,
			// Graphics
			&BackendDispatch::SetGraphicsRootSignature,
			&BackendDispatch::SetGraphicsPipelineState,
			&BackendDispatch::SetGraphicsResourceGroup,
			&BackendDispatch::SetGraphicsVertexArray,		// Input-assembler (IA) stage
			&BackendDispatch::SetGraphicsViewports,			// Rasterizer (RS) stage
			&BackendDispatch::SetGraphicsScissorRectangles,	// Rasterizer (RS) stage
			&BackendDispatch::SetGraphicsRenderTarget,		// Output-merger (OM) stage
			&BackendDispatch::ClearGraphics,
			&BackendDispatch::DrawGraphics,
			&BackendDispatch::DrawIndexedGraphics,
			// Compute
			&BackendDispatch::SetComputeRootSignature,
			&BackendDispatch::SetComputePipelineState,
			&BackendDispatch::SetComputeResourceGroup,
			&BackendDispatch::DispatchCompute,
			// Resource
			&BackendDispatch::SetTextureMinimumMaximumMipmapIndex,
			&BackendDispatch::ResolveMultisampleFramebuffer,
			&BackendDispatch::CopyResource,
			// Debug
			&BackendDispatch::SetDebugMarker,
			&BackendDispatch::BeginDebugEvent,
			&BackendDispatch::EndDebugEvent
		};


//[-------------------------------------------------------]
//[ Anonymous detail namespace                            ]
//[-------------------------------------------------------]
	} // detail
}


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
namespace VulkanRenderer
{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	VulkanRenderer::VulkanRenderer(const Renderer::Context& context) :
		IRenderer(Renderer::NameId::VULKAN, context),
		mVkAllocationCallbacks{&context.getAllocator(), &::detail::vkAllocationFunction, &::detail::vkReallocationFunction, &::detail::vkFreeFunction, nullptr, nullptr},
		mVulkanRuntimeLinking(nullptr),
		mVulkanContext(nullptr),
		mShaderLanguageGlsl(nullptr),
		mGraphicsRootSignature(nullptr),
		mComputeRootSignature(nullptr),
		mDefaultSamplerState(nullptr),
		mInsideVulkanRenderPass(false),
		mVkClearValues{},
		mVertexArray(nullptr),
		mRenderTarget(nullptr)
	{
		// TODO(co) Make it possible to enable/disable validation from the outside?
		#ifdef RENDERER_DEBUG
			const bool enableValidation = true;
		#else
			const bool enableValidation = false;
		#endif

		// Is Vulkan available?
		mVulkanRuntimeLinking = RENDERER_NEW(mContext, VulkanRuntimeLinking)(*this, enableValidation);
		if (mVulkanRuntimeLinking->isVulkanAvaiable())
		{
			// TODO(co) Add external Vulkan context support
			mVulkanContext = RENDERER_NEW(mContext, VulkanContext)(*this);

			// Is the Vulkan context initialized?
			if (mVulkanContext->isInitialized())
			{
				// Initialize the capabilities
				initializeCapabilities();

				// Create the default sampler state
				mDefaultSamplerState = createSamplerState(Renderer::ISamplerState::getDefaultSamplerState());

				// Add references to the default sampler state and set it
				if (nullptr != mDefaultSamplerState)
				{
					mDefaultSamplerState->addReference();
					// TODO(co) Set default sampler states
				}
			}
		}
	}

	VulkanRenderer::~VulkanRenderer()
	{
		// Set no vertex array reference, in case we have one
		if (nullptr != mVertexArray)
		{
			setGraphicsVertexArray(nullptr);
		}

		// Release instances
		if (nullptr != mRenderTarget)
		{
			mRenderTarget->releaseReference();
			mRenderTarget = nullptr;
		}
		if (nullptr != mDefaultSamplerState)
		{
			mDefaultSamplerState->releaseReference();
			mDefaultSamplerState = nullptr;
		}

		// Release the graphics and compute root signature instance, in case we have one
		if (nullptr != mGraphicsRootSignature)
		{
			mGraphicsRootSignature->releaseReference();
		}
		if (nullptr != mComputeRootSignature)
		{
			mComputeRootSignature->releaseReference();
		}

		#ifdef RENDERER_STATISTICS
		{ // For debugging: At this point there should be no resource instances left, validate this!
			// -> Are the currently any resource instances?
			const unsigned long numberOfCurrentResources = getStatistics().getNumberOfCurrentResources();
			if (numberOfCurrentResources > 0)
			{
				// Error!
				if (numberOfCurrentResources > 1)
				{
					RENDERER_LOG(mContext, CRITICAL, "The Vulkan renderer backend is going to be destroyed, but there are still %d resource instances left (memory leak)", numberOfCurrentResources)
				}
				else
				{
					RENDERER_LOG(mContext, CRITICAL, "The Vulkan renderer backend is going to be destroyed, but there is still one resource instance left (memory leak)")
				}

				// Use debug output to show the current number of resource instances
				getStatistics().debugOutputCurrentResouces(mContext);
			}
		}
		#endif

		// Release the GLSL shader language instance, in case we have one
		if (nullptr != mShaderLanguageGlsl)
		{
			mShaderLanguageGlsl->releaseReference();
		}

		// Destroy the Vulkan context instance
		RENDERER_DELETE(mContext, VulkanContext, mVulkanContext);

		// Destroy the Vulkan runtime linking instance
		RENDERER_DELETE(mContext, VulkanRuntimeLinking, mVulkanRuntimeLinking);
	}


	//[-------------------------------------------------------]
	//[ Graphics                                              ]
	//[-------------------------------------------------------]
	void VulkanRenderer::setGraphicsRootSignature(Renderer::IRootSignature* rootSignature)
	{
		if (nullptr != mGraphicsRootSignature)
		{
			mGraphicsRootSignature->releaseReference();
		}
		mGraphicsRootSignature = static_cast<RootSignature*>(rootSignature);
		if (nullptr != mGraphicsRootSignature)
		{
			mGraphicsRootSignature->addReference();

			// Security check: Is the given resource owned by this renderer? (calls "return" in case of a mismatch)
			VULKANRENDERER_RENDERERMATCHCHECK_ASSERT(*this, *rootSignature)
		}
	}

	void VulkanRenderer::setGraphicsPipelineState(Renderer::IGraphicsPipelineState* graphicsPipelineState)
	{
		if (nullptr != graphicsPipelineState)
		{
			// Security check: Is the given resource owned by this renderer? (calls "return" in case of a mismatch)
			VULKANRENDERER_RENDERERMATCHCHECK_ASSERT(*this, *graphicsPipelineState)

			// Bind Vulkan graphics pipeline
			vkCmdBindPipeline(getVulkanContext().getVkCommandBuffer(), VK_PIPELINE_BIND_POINT_GRAPHICS, static_cast<GraphicsPipelineState*>(graphicsPipelineState)->getVkPipeline());
		}
		else
		{
			// TODO(co) Handle this situation?
		}
	}

	void VulkanRenderer::setGraphicsResourceGroup(uint32_t rootParameterIndex, Renderer::IResourceGroup* resourceGroup)
	{
		// Security checks
		#ifdef RENDERER_DEBUG
		{
			if (nullptr == mGraphicsRootSignature)
			{
				RENDERER_LOG(mContext, CRITICAL, "No Vulkan renderer backend graphics root signature set")
				return;
			}
			const Renderer::RootSignature& rootSignature = mGraphicsRootSignature->getRootSignature();
			if (rootParameterIndex >= rootSignature.numberOfParameters)
			{
				RENDERER_LOG(mContext, CRITICAL, "The Vulkan renderer backend root parameter index is out of bounds")
				return;
			}
			const Renderer::RootParameter& rootParameter = rootSignature.parameters[rootParameterIndex];
			if (Renderer::RootParameterType::DESCRIPTOR_TABLE != rootParameter.parameterType)
			{
				RENDERER_LOG(mContext, CRITICAL, "The Vulkan renderer backend root parameter index doesn't reference a descriptor table")
				return;
			}
			if (nullptr == reinterpret_cast<const Renderer::DescriptorRange*>(rootParameter.descriptorTable.descriptorRanges))
			{
				RENDERER_LOG(mContext, CRITICAL, "The Vulkan renderer backend descriptor ranges is a null pointer")
				return;
			}
		}
		#endif

		if (nullptr != resourceGroup)
		{
			// Security check: Is the given resource owned by this renderer? (calls "return" in case of a mismatch)
			VULKANRENDERER_RENDERERMATCHCHECK_ASSERT(*this, *resourceGroup)

			// Bind Vulkan descriptor set
			const VkDescriptorSet vkDescriptorSet = static_cast<ResourceGroup*>(resourceGroup)->getVkDescriptorSet();
			if (VK_NULL_HANDLE != vkDescriptorSet)
			{
				vkCmdBindDescriptorSets(getVulkanContext().getVkCommandBuffer(), VK_PIPELINE_BIND_POINT_GRAPHICS, mGraphicsRootSignature->getVkPipelineLayout(), rootParameterIndex, 1, &vkDescriptorSet, 0, nullptr);
			}
		}
		else
		{
			// TODO(co) Handle this situation?
		}
	}

	void VulkanRenderer::setGraphicsVertexArray(Renderer::IVertexArray* vertexArray)
	{
		// Input-assembler (IA) stage

		// New vertex array?
		if (mVertexArray != vertexArray)
		{
			// Set a vertex array?
			if (nullptr != vertexArray)
			{
				// Security check: Is the given resource owned by this renderer? (calls "return" in case of a mismatch)
				VULKANRENDERER_RENDERERMATCHCHECK_ASSERT(*this, *vertexArray)

				// Unset the currently used vertex array
				unsetGraphicsVertexArray();

				// Set new vertex array and add a reference to it
				mVertexArray = static_cast<VertexArray*>(vertexArray);
				mVertexArray->addReference();

				// Bind Vulkan buffers
				static_cast<VertexArray*>(vertexArray)->bindVulkanBuffers(getVulkanContext().getVkCommandBuffer());
			}
			else
			{
				// Unset the currently used vertex array
				unsetGraphicsVertexArray();
			}
		}
	}

	void VulkanRenderer::setGraphicsViewports(MAYBE_UNUSED uint32_t numberOfViewports, const Renderer::Viewport* viewports)
	{
		// Rasterizer (RS) stage

		// Sanity check
		RENDERER_ASSERT(mContext, numberOfViewports > 0 && nullptr != viewports, "Invalid Vulkan rasterizer state viewports")

		// Set Vulkan viewport
		// -> We're using the "VK_KHR_maintenance1"-extension ("VK_KHR_MAINTENANCE1_EXTENSION_NAME"-definition) to be able to specify a negative viewport height,
		//    this way we don't have to apply "<output position>.y = -<output position>.y" inside vertex shaders to compensate for the Vulkan coordinate system
		// TODO(co) Add support for multiple viewports
		VkViewport vkViewport = reinterpret_cast<const VkViewport*>(viewports)[0];
		vkViewport.y += vkViewport.height;
		vkViewport.height = -vkViewport.height;
		vkCmdSetViewport(getVulkanContext().getVkCommandBuffer(), 0, 1, &vkViewport);
	}

	void VulkanRenderer::setGraphicsScissorRectangles(MAYBE_UNUSED uint32_t numberOfScissorRectangles, const Renderer::ScissorRectangle* scissorRectangles)
	{
		// Rasterizer (RS) stage

		// Sanity check
		RENDERER_ASSERT(mContext, numberOfScissorRectangles > 0 && nullptr != scissorRectangles, "Invalid Vulkan rasterizer state scissor rectangles")

		// Set Vulkan scissor
		// TODO(co) Add support for multiple scissor rectangles. Change "Renderer::ScissorRectangle" to Vulkan style to make it the primary API on the long run?
		const VkRect2D vkRect2D =
		{
			{ static_cast<int32_t>(scissorRectangles[0].topLeftX), static_cast<int32_t>(scissorRectangles[0].topLeftY) },
			{ static_cast<uint32_t>(scissorRectangles[0].bottomRightX - scissorRectangles[0].topLeftX), static_cast<uint32_t>(scissorRectangles[0].bottomRightY - scissorRectangles[0].topLeftY) }
		};
		vkCmdSetScissor(getVulkanContext().getVkCommandBuffer(), 0, 1, &vkRect2D);
	}

	void VulkanRenderer::setGraphicsRenderTarget(Renderer::IRenderTarget* renderTarget)
	{
		// Output-merger (OM) stage

		// New render target?
		if (mRenderTarget != renderTarget)
		{
			// Release the render target reference, in case we have one
			if (nullptr != mRenderTarget)
			{
				// Start Vulkan render pass, if necessary (for e.g. clearing)
				if (!mInsideVulkanRenderPass && ((mRenderTarget->getResourceType() == Renderer::ResourceType::SWAP_CHAIN && nullptr == renderTarget) || mRenderTarget->getResourceType() == Renderer::ResourceType::FRAMEBUFFER))
				{
					beginVulkanRenderPass();
				}

				// End Vulkan render pass, if necessary
				if (mInsideVulkanRenderPass)
				{
					vkCmdEndRenderPass(getVulkanContext().getVkCommandBuffer());
					mInsideVulkanRenderPass = false;
				}

				// Release
				mRenderTarget->releaseReference();
				mRenderTarget = nullptr;
			}

			// Set a render target?
			if (nullptr != renderTarget)
			{
				// Security check: Is the given resource owned by this renderer? (calls "return" in case of a mismatch)
				VULKANRENDERER_RENDERERMATCHCHECK_ASSERT(*this, *renderTarget)

				// Set new render target and add a reference to it
				mRenderTarget = renderTarget;
				mRenderTarget->addReference();

				// Set clear color and clear depth stencil values
				const uint32_t numberOfColorAttachments = static_cast<const RenderPass&>(mRenderTarget->getRenderPass()).getNumberOfColorAttachments();
				RENDERER_ASSERT(mContext, numberOfColorAttachments < 8, "Vulkan only supports 7 render pass color attachments")
				for (uint32_t i = 0; i < numberOfColorAttachments; ++i)
				{
					mVkClearValues[i] = VkClearValue{0.0f, 0.0f, 0.0f, 1.0f};
				}
				mVkClearValues[numberOfColorAttachments] = VkClearValue{ 1.0f, 0 };
			}
		}
	}

	void VulkanRenderer::clearGraphics(uint32_t clearFlags, const float color[4], float z, uint32_t stencil)
	{
		// Sanity check
		RENDERER_ASSERT(mContext, nullptr != mRenderTarget, "Can't execute Vulkan clear command without a render target set")
		RENDERER_ASSERT(mContext, !mInsideVulkanRenderPass, "Can't execute clear command inside a Vulkan render pass")

		// Clear color
		const uint32_t numberOfColorAttachments = static_cast<const RenderPass&>(mRenderTarget->getRenderPass()).getNumberOfColorAttachments();
		RENDERER_ASSERT(mContext, numberOfColorAttachments < 8, "Vulkan only supports 7 render pass color attachments")
		if (clearFlags & Renderer::ClearFlag::COLOR)
		{
			for (uint32_t i = 0; i < numberOfColorAttachments; ++i)
			{
				memcpy(mVkClearValues[i].color.float32, &color[0], sizeof(float) * 4);
			}
		}

		// Clear depth stencil
		if ((clearFlags & Renderer::ClearFlag::DEPTH) || (clearFlags & Renderer::ClearFlag::STENCIL))
		{
			mVkClearValues[numberOfColorAttachments].depthStencil.depth = z;
			mVkClearValues[numberOfColorAttachments].depthStencil.stencil = stencil;
		}
	}

	void VulkanRenderer::drawGraphics(const Renderer::IIndirectBuffer& indirectBuffer, uint32_t indirectBufferOffset, uint32_t numberOfDraws)
	{
		// Sanity check
		RENDERER_ASSERT(mContext, numberOfDraws > 0, "Number of Vulkan draws must not be zero")
		// It's possible to draw without "mVertexArray"

		// Security check: Is the given resource owned by this renderer? (calls "return" in case of a mismatch)
		VULKANRENDERER_RENDERERMATCHCHECK_ASSERT(*this, indirectBuffer)

		// Start Vulkan render pass, if necessary
		if (!mInsideVulkanRenderPass)
		{
			beginVulkanRenderPass();
		}

		// Vulkan draw indirect command
		vkCmdDrawIndirect(getVulkanContext().getVkCommandBuffer(), static_cast<const IndirectBuffer&>(indirectBuffer).getVkBuffer(), indirectBufferOffset, numberOfDraws, sizeof(VkDrawIndirectCommand));
	}

	void VulkanRenderer::drawGraphicsEmulated(const uint8_t* emulationData, uint32_t indirectBufferOffset, uint32_t numberOfDraws)
	{
		// Sanity checks
		RENDERER_ASSERT(mContext, nullptr != emulationData, "The Vulkan emulation data must be valid")
		RENDERER_ASSERT(mContext, numberOfDraws > 0, "The number of Vulkan draws must not be zero")
		// It's possible to draw without "mVertexArray"

		// TODO(co) Currently no buffer overflow check due to lack of interface provided data
		emulationData += indirectBufferOffset;

		// Start Vulkan render pass, if necessary
		if (!mInsideVulkanRenderPass)
		{
			beginVulkanRenderPass();
		}

		// Emit the draw calls
		#ifdef RENDERER_DEBUG
			if (numberOfDraws > 1)
			{
				beginDebugEvent("Multi-draw-indirect emulation");
			}
		#endif
		const VkCommandBuffer vkCommandBuffer = getVulkanContext().getVkCommandBuffer();
		for (uint32_t i = 0; i < numberOfDraws; ++i)
		{
			// Draw and advance
			const Renderer::DrawArguments& drawArguments = *reinterpret_cast<const Renderer::DrawArguments*>(emulationData);
			vkCmdDraw(vkCommandBuffer, drawArguments.vertexCountPerInstance, drawArguments.instanceCount, drawArguments.startVertexLocation, drawArguments.startInstanceLocation);
			emulationData += sizeof(Renderer::DrawArguments);
		}
		#ifdef RENDERER_DEBUG
			if (numberOfDraws > 1)
			{
				endDebugEvent();
			}
		#endif
	}

	void VulkanRenderer::drawIndexedGraphics(const Renderer::IIndirectBuffer& indirectBuffer, uint32_t indirectBufferOffset, uint32_t numberOfDraws)
	{
		// Sanity checks
		RENDERER_ASSERT(mContext, numberOfDraws > 0, "Number of Vulkan draws must not be zero")
		RENDERER_ASSERT(mContext, nullptr != mVertexArray, "Vulkan draw indexed needs a set vertex array")
		RENDERER_ASSERT(mContext, nullptr != mVertexArray->getIndexBuffer(), "Vulkan draw indexed needs a set vertex array which contains an index buffer")

		// Security check: Is the given resource owned by this renderer? (calls "return" in case of a mismatch)
		VULKANRENDERER_RENDERERMATCHCHECK_ASSERT(*this, indirectBuffer)

		// Start Vulkan render pass, if necessary
		if (!mInsideVulkanRenderPass)
		{
			beginVulkanRenderPass();
		}

		// Vulkan draw indexed indirect command
		vkCmdDrawIndexedIndirect(getVulkanContext().getVkCommandBuffer(), static_cast<const IndirectBuffer&>(indirectBuffer).getVkBuffer(), indirectBufferOffset, numberOfDraws, sizeof(VkDrawIndexedIndirectCommand));
	}

	void VulkanRenderer::drawIndexedGraphicsEmulated(const uint8_t* emulationData, uint32_t indirectBufferOffset, uint32_t numberOfDraws)
	{
		// Sanity checks
		RENDERER_ASSERT(mContext, nullptr != emulationData, "The Vulkan emulation data must be valid")
		RENDERER_ASSERT(mContext, numberOfDraws > 0, "The number of Vulkan draws must not be zero")
		RENDERER_ASSERT(mContext, nullptr != mVertexArray, "Vulkan draw indexed needs a set vertex array")
		RENDERER_ASSERT(mContext, nullptr != mVertexArray->getIndexBuffer(), "Vulkan draw indexed needs a set vertex array which contains an index buffer")

		// TODO(co) Currently no buffer overflow check due to lack of interface provided data
		emulationData += indirectBufferOffset;

		// Start Vulkan render pass, if necessary
		if (!mInsideVulkanRenderPass)
		{
			beginVulkanRenderPass();
		}

		// Emit the draw calls
		#ifdef RENDERER_DEBUG
			if (numberOfDraws > 1)
			{
				beginDebugEvent("Multi-indexed-draw-indirect emulation");
			}
		#endif
		const VkCommandBuffer vkCommandBuffer = getVulkanContext().getVkCommandBuffer();
		for (uint32_t i = 0; i < numberOfDraws; ++i)
		{
			// Draw and advance
			const Renderer::DrawIndexedArguments& drawIndexedArguments = *reinterpret_cast<const Renderer::DrawIndexedArguments*>(emulationData);
			vkCmdDrawIndexed(vkCommandBuffer, drawIndexedArguments.indexCountPerInstance, drawIndexedArguments.instanceCount, drawIndexedArguments.startIndexLocation, drawIndexedArguments.baseVertexLocation, drawIndexedArguments.startInstanceLocation);
			emulationData += sizeof(Renderer::DrawIndexedArguments);
		}
		#ifdef RENDERER_DEBUG
			if (numberOfDraws > 1)
			{
				endDebugEvent();
			}
		#endif
	}


	//[-------------------------------------------------------]
	//[ Compute                                               ]
	//[-------------------------------------------------------]
	void VulkanRenderer::setComputeRootSignature(Renderer::IRootSignature* rootSignature)
	{
		if (nullptr != mComputeRootSignature)
		{
			mComputeRootSignature->releaseReference();
		}
		mComputeRootSignature = static_cast<RootSignature*>(rootSignature);
		if (nullptr != mComputeRootSignature)
		{
			mComputeRootSignature->addReference();

			// Security check: Is the given resource owned by this renderer? (calls "return" in case of a mismatch)
			VULKANRENDERER_RENDERERMATCHCHECK_ASSERT(*this, *rootSignature)
		}
	}

	void VulkanRenderer::setComputePipelineState(Renderer::IComputePipelineState* computePipelineState)
	{
		if (nullptr != computePipelineState)
		{
			// Security check: Is the given resource owned by this renderer? (calls "return" in case of a mismatch)
			VULKANRENDERER_RENDERERMATCHCHECK_ASSERT(*this, *computePipelineState)

			// Bind Vulkan compute pipeline
			vkCmdBindPipeline(getVulkanContext().getVkCommandBuffer(), VK_PIPELINE_BIND_POINT_COMPUTE, static_cast<ComputePipelineState*>(computePipelineState)->getVkPipeline());
		}
		else
		{
			// TODO(co) Handle this situation?
		}
	}

	void VulkanRenderer::setComputeResourceGroup(uint32_t rootParameterIndex, Renderer::IResourceGroup* resourceGroup)
	{
		// Security checks
		#ifdef RENDERER_DEBUG
		{
			if (nullptr == mComputeRootSignature)
			{
				RENDERER_LOG(mContext, CRITICAL, "No Vulkan renderer backend compute root signature set")
				return;
			}
			const Renderer::RootSignature& rootSignature = mComputeRootSignature->getRootSignature();
			if (rootParameterIndex >= rootSignature.numberOfParameters)
			{
				RENDERER_LOG(mContext, CRITICAL, "The Vulkan renderer backend root parameter index is out of bounds")
				return;
			}
			const Renderer::RootParameter& rootParameter = rootSignature.parameters[rootParameterIndex];
			if (Renderer::RootParameterType::DESCRIPTOR_TABLE != rootParameter.parameterType)
			{
				RENDERER_LOG(mContext, CRITICAL, "The Vulkan renderer backend root parameter index doesn't reference a descriptor table")
				return;
			}
			if (nullptr == reinterpret_cast<const Renderer::DescriptorRange*>(rootParameter.descriptorTable.descriptorRanges))
			{
				RENDERER_LOG(mContext, CRITICAL, "The Vulkan renderer backend descriptor ranges is a null pointer")
				return;
			}
		}
		#endif

		if (nullptr != resourceGroup)
		{
			// Security check: Is the given resource owned by this renderer? (calls "return" in case of a mismatch)
			VULKANRENDERER_RENDERERMATCHCHECK_ASSERT(*this, *resourceGroup)

			// Bind Vulkan descriptor set
			const VkDescriptorSet vkDescriptorSet = static_cast<ResourceGroup*>(resourceGroup)->getVkDescriptorSet();
			if (VK_NULL_HANDLE != vkDescriptorSet)
			{
				vkCmdBindDescriptorSets(getVulkanContext().getVkCommandBuffer(), VK_PIPELINE_BIND_POINT_COMPUTE, mComputeRootSignature->getVkPipelineLayout(), rootParameterIndex, 1, &vkDescriptorSet, 0, nullptr);
			}
		}
		else
		{
			// TODO(co) Handle this situation?
		}
	}


	//[-------------------------------------------------------]
	//[ Resource                                              ]
	//[-------------------------------------------------------]
	void VulkanRenderer::resolveMultisampleFramebuffer(Renderer::IRenderTarget&, Renderer::IFramebuffer&)
	{
		// TODO(co) Implement me
	}

	void VulkanRenderer::copyResource(Renderer::IResource&, Renderer::IResource&)
	{
		// TODO(co) Implement me
	}


	//[-------------------------------------------------------]
	//[ Debug                                                 ]
	//[-------------------------------------------------------]
	#ifdef RENDERER_DEBUG
		void VulkanRenderer::setDebugMarker(const char* name)
		{
			if (nullptr != vkCmdDebugMarkerInsertEXT)
			{
				RENDERER_ASSERT(mContext, nullptr != name, "Vulkan debug marker names must not be a null pointer")
				const VkDebugMarkerMarkerInfoEXT vkDebugMarkerMarkerInfoEXT =
				{
					VK_STRUCTURE_TYPE_DEBUG_MARKER_MARKER_INFO_EXT,	// sType (VkStructureType)
					nullptr,										// pNext (const void*)
					name,											// pMarkerName (const char*)
					{ // color[4] (float)
						0.0f,
						0.0f,
						1.0f,	// Blue
						1.0f
					}
				};
				vkCmdDebugMarkerInsertEXT(getVulkanContext().getVkCommandBuffer(), &vkDebugMarkerMarkerInfoEXT);
			}
		}

		void VulkanRenderer::beginDebugEvent(const char* name)
		{
			if (nullptr != vkCmdDebugMarkerBeginEXT)
			{
				RENDERER_ASSERT(mContext, nullptr != name, "Vulkan debug event names must not be a null pointer")
				const VkDebugMarkerMarkerInfoEXT vkDebugMarkerMarkerInfoEXT =
				{
					VK_STRUCTURE_TYPE_DEBUG_MARKER_MARKER_INFO_EXT,	// sType (VkStructureType)
					nullptr,										// pNext (const void*)
					name,											// pMarkerName (const char*)
					{ // color[4] (float)
						0.0f,
						1.0f,	// Green
						0.0f,
						1.0f
					}
				};
				vkCmdDebugMarkerBeginEXT(getVulkanContext().getVkCommandBuffer(), &vkDebugMarkerMarkerInfoEXT);
			}
		}

		void VulkanRenderer::endDebugEvent()
		{
			if (nullptr != vkCmdDebugMarkerEndEXT)
			{
				vkCmdDebugMarkerEndEXT(getVulkanContext().getVkCommandBuffer());
			}
		}
	#endif


	//[-------------------------------------------------------]
	//[ Public virtual Renderer::IRenderer methods            ]
	//[-------------------------------------------------------]
	const char* VulkanRenderer::getName() const
	{
		return "Vulkan";
	}

	bool VulkanRenderer::isInitialized() const
	{
		// Is the Vulkan context initialized?
		return (nullptr != mVulkanContext && mVulkanContext->isInitialized());
	}

	bool VulkanRenderer::isDebugEnabled()
	{
		// Check for any "VK_EXT_debug_marker"-extension function pointer
		return (nullptr != vkDebugMarkerSetObjectTagEXT || nullptr != vkDebugMarkerSetObjectNameEXT || nullptr != vkCmdDebugMarkerBeginEXT || nullptr != vkCmdDebugMarkerEndEXT || nullptr != vkCmdDebugMarkerInsertEXT);
	}


	//[-------------------------------------------------------]
	//[ Shader language                                       ]
	//[-------------------------------------------------------]
	uint32_t VulkanRenderer::getNumberOfShaderLanguages() const
	{
		// Done, return the number of supported shader languages
		return 1;
	}

	const char* VulkanRenderer::getShaderLanguageName(uint32_t) const
	{
		return ::detail::GLSL_NAME;
	}

	Renderer::IShaderLanguage* VulkanRenderer::getShaderLanguage(const char* shaderLanguageName)
	{
		// In case "shaderLanguage" is a null pointer, use the default shader language
		if (nullptr != shaderLanguageName)
		{
			// Optimization: Check for shader language name pointer match, first
			// -> "ShaderLanguageSeparate::NAME" has the same value
			if (::detail::GLSL_NAME == shaderLanguageName || !stricmp(shaderLanguageName, ::detail::GLSL_NAME))
			{
				// If required, create the GLSL shader language instance right now
				if (nullptr == mShaderLanguageGlsl)
				{
					mShaderLanguageGlsl = RENDERER_NEW(mContext, ShaderLanguageGlsl(*this));
					mShaderLanguageGlsl->addReference();	// Internal renderer reference
				}
				return mShaderLanguageGlsl;
			}
		}
		else
		{
			// Return the shader language instance as default
			return getShaderLanguage(::detail::GLSL_NAME);
		}

		// Error!
		return nullptr;
	}


	//[-------------------------------------------------------]
	//[ Resource creation                                     ]
	//[-------------------------------------------------------]
	Renderer::IRenderPass* VulkanRenderer::createRenderPass(uint32_t numberOfColorAttachments, const Renderer::TextureFormat::Enum* colorAttachmentTextureFormats, Renderer::TextureFormat::Enum depthStencilAttachmentTextureFormat, uint8_t numberOfMultisamples)
	{
		return RENDERER_NEW(mContext, RenderPass)(*this, numberOfColorAttachments, colorAttachmentTextureFormats, depthStencilAttachmentTextureFormat, numberOfMultisamples);
	}

	Renderer::ISwapChain* VulkanRenderer::createSwapChain(Renderer::IRenderPass& renderPass, Renderer::WindowHandle windowHandle, bool)
	{
		// Sanity checks
		VULKANRENDERER_RENDERERMATCHCHECK_ASSERT(*this, renderPass)
		RENDERER_ASSERT(mContext, NULL_HANDLE != windowHandle.nativeWindowHandle || nullptr != windowHandle.renderWindow, "Vulkan: The provided native window handle or render window must not be a null handle / null pointer")

		// Create the swap chain
		return RENDERER_NEW(mContext, SwapChain)(renderPass, windowHandle);
	}

	Renderer::IFramebuffer* VulkanRenderer::createFramebuffer(Renderer::IRenderPass& renderPass, const Renderer::FramebufferAttachment* colorFramebufferAttachments, const Renderer::FramebufferAttachment* depthStencilFramebufferAttachment)
	{
		// Sanity check
		VULKANRENDERER_RENDERERMATCHCHECK_ASSERT(*this, renderPass)

		// Create the framebuffer
		return RENDERER_NEW(mContext, Framebuffer)(renderPass, colorFramebufferAttachments, depthStencilFramebufferAttachment);
	}

	Renderer::IBufferManager* VulkanRenderer::createBufferManager()
	{
		return RENDERER_NEW(mContext, BufferManager)(*this);
	}

	Renderer::ITextureManager* VulkanRenderer::createTextureManager()
	{
		return RENDERER_NEW(mContext, TextureManager)(*this);
	}

	Renderer::IRootSignature* VulkanRenderer::createRootSignature(const Renderer::RootSignature& rootSignature)
	{
		return RENDERER_NEW(mContext, RootSignature)(*this, rootSignature);
	}

	Renderer::IGraphicsPipelineState* VulkanRenderer::createGraphicsPipelineState(const Renderer::GraphicsPipelineState& graphicsPipelineState)
	{
		return RENDERER_NEW(mContext, GraphicsPipelineState)(*this, graphicsPipelineState);
	}

	Renderer::IComputePipelineState* VulkanRenderer::createComputePipelineState(Renderer::IRootSignature& rootSignature, Renderer::IComputeShader& computeShader)
	{
		// Sanity checks
		VULKANRENDERER_RENDERERMATCHCHECK_ASSERT(*this, rootSignature)
		VULKANRENDERER_RENDERERMATCHCHECK_ASSERT(*this, computeShader)

		// Create the compute pipeline state
		return RENDERER_NEW(mContext, ComputePipelineState)(*this, rootSignature, computeShader);
	}

	Renderer::ISamplerState* VulkanRenderer::createSamplerState(const Renderer::SamplerState& samplerState)
	{
		return RENDERER_NEW(mContext, SamplerState)(*this, samplerState);
	}


	//[-------------------------------------------------------]
	//[ Resource handling                                     ]
	//[-------------------------------------------------------]
	bool VulkanRenderer::map(Renderer::IResource& resource, uint32_t, Renderer::MapType, uint32_t, Renderer::MappedSubresource& mappedSubresource)
	{
		// Evaluate the resource type
		switch (resource.getResourceType())
		{
			case Renderer::ResourceType::INDEX_BUFFER:
			{
				mappedSubresource.rowPitch   = 0;
				mappedSubresource.depthPitch = 0;
				return (vkMapMemory(getVulkanContext().getVkDevice(), static_cast<IndexBuffer&>(resource).getVkDeviceMemory(), 0, VK_WHOLE_SIZE, 0, &mappedSubresource.data) == VK_SUCCESS);
			}

			case Renderer::ResourceType::VERTEX_BUFFER:
			{
				mappedSubresource.rowPitch   = 0;
				mappedSubresource.depthPitch = 0;
				return (vkMapMemory(getVulkanContext().getVkDevice(), static_cast<VertexBuffer&>(resource).getVkDeviceMemory(), 0, VK_WHOLE_SIZE, 0, &mappedSubresource.data) == VK_SUCCESS);
			}

			case Renderer::ResourceType::TEXTURE_BUFFER:
			{
				mappedSubresource.rowPitch   = 0;
				mappedSubresource.depthPitch = 0;
				return (vkMapMemory(getVulkanContext().getVkDevice(), static_cast<TextureBuffer&>(resource).getVkDeviceMemory(), 0, VK_WHOLE_SIZE, 0, &mappedSubresource.data) == VK_SUCCESS);
			}

			case Renderer::ResourceType::STRUCTURED_BUFFER:
			{
				mappedSubresource.rowPitch   = 0;
				mappedSubresource.depthPitch = 0;
				return (vkMapMemory(getVulkanContext().getVkDevice(), static_cast<StructuredBuffer&>(resource).getVkDeviceMemory(), 0, VK_WHOLE_SIZE, 0, &mappedSubresource.data) == VK_SUCCESS);
			}

			case Renderer::ResourceType::INDIRECT_BUFFER:
			{
				mappedSubresource.rowPitch   = 0;
				mappedSubresource.depthPitch = 0;
				return (vkMapMemory(getVulkanContext().getVkDevice(), static_cast<IndirectBuffer&>(resource).getVkDeviceMemory(), 0, VK_WHOLE_SIZE, 0, &mappedSubresource.data) == VK_SUCCESS);
			}

			case Renderer::ResourceType::UNIFORM_BUFFER:
			{
				mappedSubresource.rowPitch   = 0;
				mappedSubresource.depthPitch = 0;
				return (vkMapMemory(getVulkanContext().getVkDevice(), static_cast<UniformBuffer&>(resource).getVkDeviceMemory(), 0, VK_WHOLE_SIZE, 0, &mappedSubresource.data) == VK_SUCCESS);
			}

			case Renderer::ResourceType::TEXTURE_1D:
			{
				// TODO(co) Implement me
				return false;
			}

			case Renderer::ResourceType::TEXTURE_2D:
			{
				// TODO(co) Implement me
				return false;
			}

			case Renderer::ResourceType::TEXTURE_2D_ARRAY:
			{
				// TODO(co) Implement me
				return false;
			}

			case Renderer::ResourceType::TEXTURE_3D:
			{
				// TODO(co) Implement me
				return false;
			}

			case Renderer::ResourceType::TEXTURE_CUBE:
			{
				// TODO(co) Implement me
				return false;
			}

			case Renderer::ResourceType::ROOT_SIGNATURE:
			case Renderer::ResourceType::RESOURCE_GROUP:
			case Renderer::ResourceType::GRAPHICS_PROGRAM:
			case Renderer::ResourceType::VERTEX_ARRAY:
			case Renderer::ResourceType::RENDER_PASS:
			case Renderer::ResourceType::SWAP_CHAIN:
			case Renderer::ResourceType::FRAMEBUFFER:
			case Renderer::ResourceType::GRAPHICS_PIPELINE_STATE:
			case Renderer::ResourceType::COMPUTE_PIPELINE_STATE:
			case Renderer::ResourceType::SAMPLER_STATE:
			case Renderer::ResourceType::VERTEX_SHADER:
			case Renderer::ResourceType::TESSELLATION_CONTROL_SHADER:
			case Renderer::ResourceType::TESSELLATION_EVALUATION_SHADER:
			case Renderer::ResourceType::GEOMETRY_SHADER:
			case Renderer::ResourceType::FRAGMENT_SHADER:
			case Renderer::ResourceType::COMPUTE_SHADER:
			default:
				// Nothing we can map, set known return values
				mappedSubresource.data		 = nullptr;
				mappedSubresource.rowPitch   = 0;
				mappedSubresource.depthPitch = 0;

				// Error!
				return false;
		}
	}

	void VulkanRenderer::unmap(Renderer::IResource& resource, uint32_t)
	{
		// Evaluate the resource type
		switch (resource.getResourceType())
		{
			case Renderer::ResourceType::INDEX_BUFFER:
			{
				vkUnmapMemory(getVulkanContext().getVkDevice(), static_cast<IndexBuffer&>(resource).getVkDeviceMemory());
				break;
			}

			case Renderer::ResourceType::VERTEX_BUFFER:
			{
				vkUnmapMemory(getVulkanContext().getVkDevice(), static_cast<VertexBuffer&>(resource).getVkDeviceMemory());
				break;
			}

			case Renderer::ResourceType::TEXTURE_BUFFER:
			{
				vkUnmapMemory(getVulkanContext().getVkDevice(), static_cast<TextureBuffer&>(resource).getVkDeviceMemory());
				break;
			}

			case Renderer::ResourceType::STRUCTURED_BUFFER:
			{
				vkUnmapMemory(getVulkanContext().getVkDevice(), static_cast<StructuredBuffer&>(resource).getVkDeviceMemory());
				break;
			}

			case Renderer::ResourceType::INDIRECT_BUFFER:
			{
				vkUnmapMemory(getVulkanContext().getVkDevice(), static_cast<IndirectBuffer&>(resource).getVkDeviceMemory());
				break;
			}

			case Renderer::ResourceType::UNIFORM_BUFFER:
			{
				vkUnmapMemory(getVulkanContext().getVkDevice(), static_cast<UniformBuffer&>(resource).getVkDeviceMemory());
				break;
			}

			case Renderer::ResourceType::TEXTURE_1D:
			{
				// TODO(co) Implement me
				break;
			}

			case Renderer::ResourceType::TEXTURE_2D:
			{
				// TODO(co) Implement me
				/*
				// Get the Direct3D 11 resource instance
				ID3D11Resource* d3d11Resource = nullptr;
				static_cast<Texture2D&>(resource).getD3D11ShaderResourceView()->GetResource(&d3d11Resource);
				if (nullptr != d3d11Resource)
				{
					// Unmap the Direct3D 11 resource
					mD3D11DeviceContext->Unmap(d3d11Resource, subresource);

					// Release the Direct3D 11 resource instance
					d3d11Resource->Release();
				}
				*/
				break;
			}

			case Renderer::ResourceType::TEXTURE_2D_ARRAY:
			{
				// TODO(co) Implement me
				/*
				// Get the Direct3D 11 resource instance
				ID3D11Resource* d3d11Resource = nullptr;
				static_cast<Texture2DArray&>(resource).getD3D11ShaderResourceView()->GetResource(&d3d11Resource);
				if (nullptr != d3d11Resource)
				{
					// Unmap the Direct3D 11 resource
					mD3D11DeviceContext->Unmap(d3d11Resource, subresource);

					// Release the Direct3D 11 resource instance
					d3d11Resource->Release();
				}
				*/
				break;
			}

			case Renderer::ResourceType::TEXTURE_3D:
			{
				// TODO(co) Implement me
				break;
			}

			case Renderer::ResourceType::TEXTURE_CUBE:
			{
				// TODO(co) Implement me
				break;
			}

			case Renderer::ResourceType::ROOT_SIGNATURE:
			case Renderer::ResourceType::RESOURCE_GROUP:
			case Renderer::ResourceType::GRAPHICS_PROGRAM:
			case Renderer::ResourceType::VERTEX_ARRAY:
			case Renderer::ResourceType::RENDER_PASS:
			case Renderer::ResourceType::SWAP_CHAIN:
			case Renderer::ResourceType::FRAMEBUFFER:
			case Renderer::ResourceType::GRAPHICS_PIPELINE_STATE:
			case Renderer::ResourceType::COMPUTE_PIPELINE_STATE:
			case Renderer::ResourceType::SAMPLER_STATE:
			case Renderer::ResourceType::VERTEX_SHADER:
			case Renderer::ResourceType::TESSELLATION_CONTROL_SHADER:
			case Renderer::ResourceType::TESSELLATION_EVALUATION_SHADER:
			case Renderer::ResourceType::GEOMETRY_SHADER:
			case Renderer::ResourceType::FRAGMENT_SHADER:
			case Renderer::ResourceType::COMPUTE_SHADER:
			default:
				// Nothing we can unmap
				break;
		}
	}


	//[-------------------------------------------------------]
	//[ Operations                                            ]
	//[-------------------------------------------------------]
	bool VulkanRenderer::beginScene()
	{
		// Begin Vulkan command buffer
		// -> This automatically resets the Vulkan command buffer in case it was previously already recorded
		static constexpr VkCommandBufferBeginInfo vkCommandBufferBeginInfo =
		{
			VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,	// sType (VkStructureType)
			nullptr,										// pNext (const void*)
			0,												// flags (VkCommandBufferUsageFlags)
			nullptr											// pInheritanceInfo (const VkCommandBufferInheritanceInfo*)
		};
		if (vkBeginCommandBuffer(getVulkanContext().getVkCommandBuffer(), &vkCommandBufferBeginInfo) == VK_SUCCESS)
		{
			// Done
			return true;
		}
		else
		{
			// Error!
			RENDERER_LOG(getContext(), CRITICAL, "Failed to begin Vulkan command buffer instance")
			return false;
		}
	}

	void VulkanRenderer::submitCommandBuffer(const Renderer::CommandBuffer& commandBuffer)
	{
		// Loop through all commands
		const uint8_t* commandPacketBuffer = commandBuffer.getCommandPacketBuffer();
		Renderer::ConstCommandPacket constCommandPacket = commandPacketBuffer;
		while (nullptr != constCommandPacket)
		{
			{ // Submit command packet
				const Renderer::CommandDispatchFunctionIndex commandDispatchFunctionIndex = Renderer::CommandPacketHelper::loadCommandDispatchFunctionIndex(constCommandPacket);
				const void* command = Renderer::CommandPacketHelper::loadCommand(constCommandPacket);
				detail::DISPATCH_FUNCTIONS[commandDispatchFunctionIndex](command, *this);
			}

			{ // Next command
				const uint32_t nextCommandPacketByteIndex = Renderer::CommandPacketHelper::getNextCommandPacketByteIndex(constCommandPacket);
				constCommandPacket = (~0u != nextCommandPacketByteIndex) ? &commandPacketBuffer[nextCommandPacketByteIndex] : nullptr;
			}
		}
	}

	void VulkanRenderer::endScene()
	{
		// We need to forget about the currently set render target
		setGraphicsRenderTarget(nullptr);

		// We need to forget about the currently set vertex array
		unsetGraphicsVertexArray();

		// End Vulkan command buffer
		if (vkEndCommandBuffer(getVulkanContext().getVkCommandBuffer()) != VK_SUCCESS)
		{
			// Error!
			RENDERER_LOG(getContext(), CRITICAL, "Failed to end Vulkan command buffer instance")
		}
	}


	//[-------------------------------------------------------]
	//[ Synchronization                                       ]
	//[-------------------------------------------------------]
	void VulkanRenderer::flush()
	{
		// TODO(co) Implement me
	}

	void VulkanRenderer::finish()
	{
		// TODO(co) Implement me
	}


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	void VulkanRenderer::initializeCapabilities()
	{
		{ // Get device name
			VkPhysicalDeviceProperties vkPhysicalDeviceProperties;
			vkGetPhysicalDeviceProperties(mVulkanContext->getVkPhysicalDevice(), &vkPhysicalDeviceProperties);
			const size_t numberOfCharacters = ::detail::countof(mCapabilities.deviceName) - 1;
			strncpy(mCapabilities.deviceName, vkPhysicalDeviceProperties.deviceName, numberOfCharacters);
			mCapabilities.deviceName[numberOfCharacters] = '\0';
		}

		// Preferred swap chain texture format
		mCapabilities.preferredSwapChainColorTextureFormat = (SwapChain::findColorVkFormat(getContext(), mVulkanRuntimeLinking->getVkInstance(), *mVulkanContext) == VK_FORMAT_R8G8B8A8_UNORM) ? Renderer::TextureFormat::Enum::R8G8B8A8 : Renderer::TextureFormat::Enum::B8G8R8A8;

		{ // Preferred swap chain depth stencil texture format
			const VkFormat depthVkFormat = SwapChain::findDepthVkFormat(mVulkanContext->getVkPhysicalDevice());
			if (VK_FORMAT_D32_SFLOAT == depthVkFormat)
			{
				mCapabilities.preferredSwapChainDepthStencilTextureFormat = Renderer::TextureFormat::Enum::D32_FLOAT;
			}
			else
			{
				// TODO(co) Add support for "VK_FORMAT_D32_SFLOAT_S8_UINT" and "VK_FORMAT_D24_UNORM_S8_UINT"
				mCapabilities.preferredSwapChainDepthStencilTextureFormat = Renderer::TextureFormat::Enum::D32_FLOAT;
			}
		}

		// TODO(co) Implement me, this in here is just a placeholder implementation

		{ // "D3D_FEATURE_LEVEL_11_0"
			// Maximum number of viewports (always at least 1)
			mCapabilities.maximumNumberOfViewports = 16;

			// Maximum number of simultaneous render targets (if <1 render to texture is not supported)
			mCapabilities.maximumNumberOfSimultaneousRenderTargets = 8;

			// Maximum texture dimension
			mCapabilities.maximumTextureDimension = 16384;

			// Maximum number of 2D texture array slices (usually 512, in case there's no support for 2D texture arrays it's 0)
			mCapabilities.maximumNumberOf2DTextureArraySlices = 512;

			// Maximum texture buffer (TBO) size in texel (>65536, typically much larger than that of one-dimensional texture, in case there's no support for texture buffer it's 0)
			mCapabilities.maximumTextureBufferSize = mCapabilities.maximumStructuredBufferSize = 128 * 1024 * 1024;	// TODO(co) http://msdn.microsoft.com/en-us/library/ff476876%28v=vs.85%29.aspx does not mention the texture buffer? Currently the OpenGL 3 minimum is used: 128 MiB.

			// Maximum indirect buffer size in bytes
			mCapabilities.maximumIndirectBufferSize = 64 * 1024;	// 64 KiB

			// Maximum number of multisamples (always at least 1, usually 8)
			mCapabilities.maximumNumberOfMultisamples = 1;	// TODO(co) Add multisample support, when setting "VkPhysicalDeviceFeatures" -> "sampleRateShading" -> VK_TRUE don't forget to check whether or not the device sup pots the feature

			// Maximum anisotropy (always at least 1, usually 16)
			mCapabilities.maximumAnisotropy = 16;

			// Instanced arrays supported? (shader model 3 feature, vertex array element advancing per-instance instead of per-vertex)
			mCapabilities.instancedArrays = true;

			// Draw instanced supported? (shader model 4 feature, build in shader variable holding the current instance ID)
			mCapabilities.drawInstanced = true;

			// Maximum number of vertices per patch (usually 0 for no tessellation support or 32 which is the maximum number of supported vertices per patch)
			mCapabilities.maximumNumberOfPatchVertices = 32;

			// Maximum number of vertices a geometry shader can emit (usually 0 for no geometry shader support or 1024)
			mCapabilities.maximumNumberOfGsOutputVertices = 1024;	// TODO(co) http://msdn.microsoft.com/en-us/library/ff476876%28v=vs.85%29.aspx does not mention it, so I assume it's 1024
		}

		// The rest is the same for all feature levels

		// Maximum uniform buffer (UBO) size in bytes (usually at least 4096 * 16 bytes, in case there's no support for uniform buffer it's 0)
		// -> See https://msdn.microsoft.com/en-us/library/windows/desktop/ff819065(v=vs.85).aspx - "Resource Limits (Direct3D 11)" - "Number of elements in a constant buffer D3D11_REQ_CONSTANT_BUFFER_ELEMENT_COUNT (4096)"
		// -> One element = float4 = 16 bytes
		mCapabilities.maximumUniformBufferSize = 4096 * 16;

		// Left-handed coordinate system with clip space depth value range 0..1
		mCapabilities.upperLeftOrigin = mCapabilities.zeroToOneClipZ = true;

		// Individual uniforms ("constants" in Direct3D terminology) supported? If not, only uniform buffer objects are supported.
		mCapabilities.individualUniforms = false;

		// Base vertex supported for draw calls?
		mCapabilities.baseVertex = true;

		// Vulkan has native multi-threading
		mCapabilities.nativeMultiThreading = false;	// TODO(co) Enable native multi-threading when done

		// Vulkan has shader bytecode support
		mCapabilities.shaderBytecode = false;	// TODO(co) Vulkan has shader bytecode support, set "mCapabilities.shaderBytecode" to true later on

		// Is there support for vertex shaders (VS)?
		mCapabilities.vertexShader = true;

		// Is there support for fragment shaders (FS)?
		mCapabilities.fragmentShader = true;

		// Is there support for compute shaders (CS)?
		mCapabilities.computeShader = true;
	}

	void VulkanRenderer::unsetGraphicsVertexArray()
	{
		// Release the currently used vertex array reference, in case we have one
		if (nullptr != mVertexArray)
		{
			// Do nothing since the Vulkan specification says "bindingCount must be greater than 0"
			// vkCmdBindVertexBuffers(getVulkanContext().getVkCommandBuffer(), 0, 0, nullptr, nullptr);

			// Release reference
			mVertexArray->releaseReference();
			mVertexArray = nullptr;
		}
	}

	void VulkanRenderer::beginVulkanRenderPass()
	{
		// Sanity checks
		RENDERER_ASSERT(mContext, !mInsideVulkanRenderPass, "We're already inside a Vulkan render pass")
		RENDERER_ASSERT(mContext, nullptr != mRenderTarget, "Can't begin a Vulkan render pass without a render target set")

		// Start Vulkan render pass
		const uint32_t numberOfAttachments = static_cast<const RenderPass&>(mRenderTarget->getRenderPass()).getNumberOfAttachments();
		RENDERER_ASSERT(mContext, numberOfAttachments < 9, "Vulkan only supports 8 render pass attachments")
		switch (mRenderTarget->getResourceType())
		{
			case Renderer::ResourceType::SWAP_CHAIN:
			{
				const SwapChain* swapChain = static_cast<SwapChain*>(mRenderTarget);
				::detail::beginVulkanRenderPass(*mRenderTarget, swapChain->getVkRenderPass(), swapChain->getCurrentVkFramebuffer(), numberOfAttachments, mVkClearValues, getVulkanContext().getVkCommandBuffer());
				break;
			}

			case Renderer::ResourceType::FRAMEBUFFER:
			{
				const Framebuffer* framebuffer = static_cast<Framebuffer*>(mRenderTarget);
				::detail::beginVulkanRenderPass(*mRenderTarget, framebuffer->getVkRenderPass(), framebuffer->getVkFramebuffer(), numberOfAttachments, mVkClearValues, getVulkanContext().getVkCommandBuffer());
				break;
			}

			case Renderer::ResourceType::ROOT_SIGNATURE:
			case Renderer::ResourceType::RESOURCE_GROUP:
			case Renderer::ResourceType::GRAPHICS_PROGRAM:
			case Renderer::ResourceType::VERTEX_ARRAY:
			case Renderer::ResourceType::RENDER_PASS:
			case Renderer::ResourceType::INDEX_BUFFER:
			case Renderer::ResourceType::VERTEX_BUFFER:
			case Renderer::ResourceType::TEXTURE_BUFFER:
			case Renderer::ResourceType::STRUCTURED_BUFFER:
			case Renderer::ResourceType::INDIRECT_BUFFER:
			case Renderer::ResourceType::UNIFORM_BUFFER:
			case Renderer::ResourceType::TEXTURE_1D:
			case Renderer::ResourceType::TEXTURE_2D:
			case Renderer::ResourceType::TEXTURE_2D_ARRAY:
			case Renderer::ResourceType::TEXTURE_3D:
			case Renderer::ResourceType::TEXTURE_CUBE:
			case Renderer::ResourceType::GRAPHICS_PIPELINE_STATE:
			case Renderer::ResourceType::COMPUTE_PIPELINE_STATE:
			case Renderer::ResourceType::SAMPLER_STATE:
			case Renderer::ResourceType::VERTEX_SHADER:
			case Renderer::ResourceType::TESSELLATION_CONTROL_SHADER:
			case Renderer::ResourceType::TESSELLATION_EVALUATION_SHADER:
			case Renderer::ResourceType::GEOMETRY_SHADER:
			case Renderer::ResourceType::FRAGMENT_SHADER:
			case Renderer::ResourceType::COMPUTE_SHADER:
			default:
				// Not handled in here
				break;
		}
		mInsideVulkanRenderPass = true;
	}


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
} // VulkanRenderer




//[-------------------------------------------------------]
//[ Global functions                                      ]
//[-------------------------------------------------------]
// Export the instance creation function
#ifdef RENDERER_VULKAN_EXPORTS
	#define VULKANRENDERER_FUNCTION_EXPORT GENERIC_FUNCTION_EXPORT
#else
	#define VULKANRENDERER_FUNCTION_EXPORT
#endif
VULKANRENDERER_FUNCTION_EXPORT Renderer::IRenderer* createVulkanRendererInstance(const Renderer::Context& context)
{
	return RENDERER_NEW(context, VulkanRenderer::VulkanRenderer)(context);
}
#undef VULKANRENDERER_FUNCTION_EXPORT
