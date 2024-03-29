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


// Amalgamated/unity build
#ifdef RENDERER_OPENVR
	#include "Public/Vr/OpenVR/OpenVRRuntimeLinking.cpp"
	#include "Public/Vr/OpenVR/VrManagerOpenVR.cpp"
	#include "Public/Vr/OpenVR/Loader/OpenVRMeshResourceLoader.cpp"
	#include "Public/Vr/OpenVR/Loader/OpenVRTextureResourceLoader.cpp"
#endif
#include "Public/Context.cpp"
#include "Public/RendererImpl.cpp"
#include "Public/Asset/AssetManager.cpp"
#include "Public/Asset/AssetPackage.cpp"
#include "Public/Asset/Loader/AssetPackageLoader.cpp"
#include "Public/Core/File/FileSystemHelper.cpp"
#include "Public/Core/File/MemoryFile.cpp"
#include "Public/Core/Math/Frustum.cpp"
#include "Public/Core/Math/Math.cpp"
#include "Public/Core/Math/Transform.cpp"
#include "Public/Core/Platform/PlatformManager.cpp"
#include "Public/Core/Renderer/FramebufferManager.cpp"
#include "Public/Core/Renderer/FramebufferSignature.cpp"
#include "Public/Core/Renderer/RenderPassManager.cpp"
#include "Public/Core/Renderer/RenderTargetTextureManager.cpp"
#include "Public/Core/Renderer/RenderTargetTextureSignature.cpp"
#include "Public/Core/Time/Stopwatch.cpp"
#include "Public/Core/Time/TimeManager.cpp"
#ifdef RENDERER_IMGUI
	#include "Public/DebugGui/DebugGuiHelper.cpp"
	#include "Public/DebugGui/DebugGuiManager.cpp"
	#ifdef _WIN32
		#include "Public/DebugGui/Detail/DebugGuiManagerWindows.cpp"
	#elif UNIX
		#include "Public/DebugGui/Detail/DebugGuiManagerLinux.cpp"
	#endif
#endif
#include "Public/RenderQueue/Renderable.cpp"
#include "Public/RenderQueue/RenderableManager.cpp"
#include "Public/RenderQueue/RenderQueue.cpp"
#include "Public/Resource/IResourceListener.cpp"
#include "Public/Resource/CompositorNode/CompositorNodeInstance.cpp"
#include "Public/Resource/CompositorNode/CompositorNodeResource.cpp"
#include "Public/Resource/CompositorNode/CompositorNodeResourceManager.cpp"
#include "Public/Resource/CompositorNode/CompositorTarget.cpp"
#include "Public/Resource/CompositorNode/Loader/CompositorNodeResourceLoader.cpp"
#include "Public/Resource/CompositorNode/Pass/CompositorPassFactory.cpp"
#include "Public/Resource/CompositorNode/Pass/Clear/CompositorInstancePassClear.cpp"
#include "Public/Resource/CompositorNode/Pass/Clear/CompositorResourcePassClear.cpp"
#include "Public/Resource/CompositorNode/Pass/Compute/CompositorInstancePassCompute.cpp"
#include "Public/Resource/CompositorNode/Pass/Compute/CompositorResourcePassCompute.cpp"
#include "Public/Resource/CompositorNode/Pass/Copy/CompositorInstancePassCopy.cpp"
#include "Public/Resource/CompositorNode/Pass/Copy/CompositorResourcePassCopy.cpp"
#include "Public/Resource/CompositorNode/Pass/DebugGui/CompositorInstancePassDebugGui.cpp"
#include "Public/Resource/CompositorNode/Pass/GenerateMipmaps/CompositorInstancePassGenerateMipmaps.cpp"
#include "Public/Resource/CompositorNode/Pass/GenerateMipmaps/CompositorResourcePassGenerateMipmaps.cpp"
#include "Public/Resource/CompositorNode/Pass/ResolveMultisample/CompositorInstancePassResolveMultisample.cpp"
#include "Public/Resource/CompositorNode/Pass/ResolveMultisample/CompositorResourcePassResolveMultisample.cpp"
#include "Public/Resource/CompositorNode/Pass/Scene/CompositorInstancePassScene.cpp"
#include "Public/Resource/CompositorNode/Pass/Scene/CompositorResourcePassScene.cpp"
#include "Public/Resource/CompositorNode/Pass/ShadowMap/CompositorInstancePassShadowMap.cpp"
#include "Public/Resource/CompositorNode/Pass/ShadowMap/CompositorResourcePassShadowMap.cpp"
#include "Public/Resource/CompositorNode/Pass/VrHiddenAreaMesh/CompositorInstancePassVrHiddenAreaMesh.cpp"
#include "Public/Resource/CompositorNode/Pass/VrHiddenAreaMesh/CompositorResourcePassVrHiddenAreaMesh.cpp"
#include "Public/Resource/CompositorWorkspace/CompositorContextData.cpp"
#include "Public/Resource/CompositorWorkspace/CompositorWorkspaceInstance.cpp"
#include "Public/Resource/CompositorWorkspace/CompositorWorkspaceResourceManager.cpp"
#include "Public/Resource/CompositorWorkspace/Loader/CompositorWorkspaceResourceLoader.cpp"
#include "Public/Resource/IResource.cpp"
#include "Public/Resource/RendererResourceManager.cpp"
#include "Public/Resource/ResourceStreamer.cpp"
#include "Public/Resource/Material/MaterialProperties.cpp"
#include "Public/Resource/Material/MaterialPropertyValue.cpp"
#include "Public/Resource/Material/MaterialResource.cpp"
#include "Public/Resource/Material/MaterialResourceManager.cpp"
#include "Public/Resource/Material/MaterialTechnique.cpp"
#include "Public/Resource/Material/Loader/MaterialResourceLoader.cpp"
#include "Public/Resource/MaterialBlueprint/MaterialBlueprintResource.cpp"
#include "Public/Resource/MaterialBlueprint/MaterialBlueprintResourceManager.cpp"
#include "Public/Resource/MaterialBlueprint/BufferManager/IndirectBufferManager.cpp"
#include "Public/Resource/MaterialBlueprint/BufferManager/UniformInstanceBufferManager.cpp"
#include "Public/Resource/MaterialBlueprint/BufferManager/TextureInstanceBufferManager.cpp"
#include "Public/Resource/MaterialBlueprint/BufferManager/LightBufferManager.cpp"
#include "Public/Resource/MaterialBlueprint/BufferManager/MaterialBufferManager.cpp"
#include "Public/Resource/MaterialBlueprint/BufferManager/MaterialBufferSlot.cpp"
#include "Public/Resource/MaterialBlueprint/BufferManager/PassBufferManager.cpp"
#include "Public/Resource/MaterialBlueprint/Cache/GraphicsPipelineStateCacheManager.cpp"
#include "Public/Resource/MaterialBlueprint/Cache/GraphicsPipelineStateCompiler.cpp"
#include "Public/Resource/MaterialBlueprint/Cache/GraphicsPipelineStateSignature.cpp"
#include "Public/Resource/MaterialBlueprint/Cache/GraphicsProgramCacheManager.cpp"
#include "Public/Resource/MaterialBlueprint/Cache/ComputePipelineStateCacheManager.cpp"
#include "Public/Resource/MaterialBlueprint/Cache/ComputePipelineStateCompiler.cpp"
#include "Public/Resource/MaterialBlueprint/Cache/ComputePipelineStateSignature.cpp"
#include "Public/Resource/MaterialBlueprint/Listener/MaterialBlueprintResourceListener.cpp"
#include "Public/Resource/MaterialBlueprint/Loader/MaterialBlueprintResourceLoader.cpp"
#include "Public/Resource/Mesh/MeshResource.cpp"
#include "Public/Resource/Mesh/MeshResourceManager.cpp"
#include "Public/Resource/Mesh/Loader/IMeshResourceLoader.cpp"
#include "Public/Resource/Mesh/Loader/MeshResourceLoader.cpp"
#include "Public/Resource/Scene/SceneNode.cpp"
#include "Public/Resource/Scene/SceneResource.cpp"
#include "Public/Resource/Scene/SceneResourceManager.cpp"
#include "Public/Resource/Scene/Factory/SceneFactory.cpp"
#include "Public/Resource/Scene/Culling/SceneCullingManager.cpp"
#include "Public/Resource/Scene/Item/ISceneItem.cpp"
#include "Public/Resource/Scene/Item/MaterialSceneItem.cpp"
#include "Public/Resource/Scene/Item/Camera/CameraSceneItem.cpp"
#include "Public/Resource/Scene/Item/Debug/DebugDrawSceneItem.cpp"
#include "Public/Resource/Scene/Item/Grass/GrassSceneItem.cpp"
#include "Public/Resource/Scene/Item/Light/LightSceneItem.cpp"
#include "Public/Resource/Scene/Item/Light/SunlightSceneItem.cpp"
#include "Public/Resource/Scene/Item/Mesh/MeshSceneItem.cpp"
#include "Public/Resource/Scene/Item/Mesh/SkeletonMeshSceneItem.cpp"
#include "Public/Resource/Scene/Item/Particles/ParticlesSceneItem.cpp"
#include "Public/Resource/Scene/Item/Sky/HosekWilkieSky.cpp"
#include "Public/Resource/Scene/Item/Sky/SkySceneItem.cpp"
#include "Public/Resource/Scene/Item/Terrain/TerrainSceneItem.cpp"
#include "Public/Resource/Scene/Item/Volume/VolumeSceneItem.cpp"
#include "Public/Resource/Scene/Loader/SceneResourceLoader.cpp"
#include "Public/Resource/ShaderBlueprint/ShaderBlueprintResourceManager.cpp"
#include "Public/Resource/ShaderBlueprint/Cache/Preprocessor.cpp"
#include "Public/Resource/ShaderBlueprint/Cache/ShaderBuilder.cpp"
#include "Public/Resource/ShaderBlueprint/Cache/ShaderCacheManager.cpp"
#include "Public/Resource/ShaderBlueprint/Cache/ShaderProperties.cpp"
#include "Public/Resource/ShaderBlueprint/Loader/ShaderBlueprintResourceLoader.cpp"
#include "Public/Resource/ShaderPiece/Loader/ShaderPieceResourceLoader.cpp"
#include "Public/Resource/ShaderPiece/ShaderPieceResourceManager.cpp"
#include "Public/Resource/Skeleton/SkeletonResource.cpp"
#include "Public/Resource/Skeleton/SkeletonResourceManager.cpp"
#include "Public/Resource/Skeleton/Loader/SkeletonResourceLoader.cpp"
#include "Public/Resource/SkeletonAnimation/SkeletonAnimationController.cpp"
#include "Public/Resource/SkeletonAnimation/SkeletonAnimationEvaluator.cpp"
#include "Public/Resource/SkeletonAnimation/SkeletonAnimationResourceManager.cpp"
#include "Public/Resource/SkeletonAnimation/Loader/SkeletonAnimationResourceLoader.cpp"
#include "Public/Resource/Texture/TextureResourceManager.cpp"
#include "Public/Resource/Texture/Loader/CrnTextureResourceLoader.cpp"
#include "Public/Resource/Texture/Loader/CrnArrayTextureResourceLoader.cpp"
#include "Public/Resource/Texture/Loader/Lz4DdsTextureResourceLoader.cpp"
#include "Public/Resource/Texture/Loader/ITextureResourceLoader.cpp"
#include "Public/Resource/Texture/Loader/KtxTextureResourceLoader.cpp"
#include "Public/Resource/VertexAttributes/VertexAttributesResourceManager.cpp"
#include "Public/Resource/VertexAttributes/Loader/VertexAttributesResourceLoader.cpp"
