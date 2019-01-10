[![GitHub license](https://img.shields.io/badge/license-MIT-blue.svg)](https://raw.githubusercontent.com/cofenberg/unrimp/master/LICENSE)


Foreword
======
This is a spare time project. There's no support. API changes are done in an time efficient way, meaning without thinking about backward compatibility.


Unrimp Description
======
Unified renderer implementation ("Un r imp"). Extensive examples from basic to advanced are provided as well. Originally started as modern renderer replacement for the [PixelLight](https://github.com/PixelLightFoundation/pixellight) engine which was in active development between 2002-2012.

The first public Unrimp source code release after starting the project on Friday, 1 June 2012 was on January 03, 2013.


Screenshots
======
<img src="https://github.com/cofenberg/unrimp/raw/master/Documentation/Screenshot1.jpg" width="640" height="360">
<img src="https://github.com/cofenberg/unrimp/raw/master/Documentation/Screenshot3.jpg" width="640" height="360">
<img src="https://github.com/cofenberg/unrimp/raw/master/Documentation/Screenshot2.jpg" width="640" height="360">


Features
======
- General
	- C++ 17 and above, no legacy compiler support, compiled with wall warning level
	- Compact user-header for the renderer backend API
		- A single all in one header for ease-of-use and best possible compile times
		- No need to links against the renderer library itself, load renderer interface implementations dynamically during runtime
	- Usage of [Amalgamated](https://blog.forrestthewoods.com/improving-open-source-with-amalgamation-cf293592c5f4)/[Unity](http://buffered.io/posts/the-magic-of-unity-builds/) builds for best possible compile times
	- Using [CMake](https://cmake.org/) for the build process
	- Using [Doxygen](http://www.doxygen.org) for code documentation
	- Lightweight renderer implementations
		- Designed with [AZDO ("Almost Zero Driver Overhead")](https://de.slideshare.net/CassEveritt/approaching-zero-driver-overhead) in mind
		- Implementations try to stick as best as possible close-to-the-metal and as a result are just a few KiB instead of MiB in memory size
		- Implementations load the entry points of Vulkan, Direct3D, OpenGL and so on during runtime, meaning it's possible to react on system failures by e.g. dynamically switching to another renderer implementation
	- Support for static and shared build
	- Separation into backend, runtime and toolkit for asset cooking
		- Backend abstracts way the underlying renderer API like Vulkan/OpenGL/DirectX
		- Runtime designed with end-user and middleware-user in mind
			- Efficiency and responsiveness over flexibility (were it isn't useful in practice)
			- Intended to be controlled by a high-level entity-component system, no unused implementation feature overkill in the basic runtime
		- Toolkit designed with developer fast iterations in mind: Asset source flexibility, asset background compilation, hot-reloading
	- Interfaces for log, assert, memory allocator, graphics debugger and profiler so the user has the control over those things
		- Standard implementations are provided
		- Standard graphics debugger implementation using [RenderDoc](https://renderdoc.org/) is provided
		- Standard profiler implementation using [Remotery](https://github.com/Celtoys/Remotery) is provided
- Cross-platform
	- Microsoft Windows x86 and x64
	- Currently unmaintained
		- Linux
		- Android
	- May come
		- Mac OS X in mind (nothing more at the moment, but in mind is important because Mac OS X 10.11 only supports OpenGL 4.1 and has some other nasty issues)


Renderer API and Backends
======
- Renderer implementations for
	- Vulkan (not feature complete, yet)
	- Direct3D 12 (early phase)
	- Direct3D 11
	- OpenGL (by default a OpenGL 4.1 context is created, the best OpenGL version Mac OS X 10.11 supports, so lowest version we have to support)
	- OpenGL ES 3
	- Legacy = critical features like uniform buffer, texture buffer and/or compute shaders are missing which are required for modern efficient renderers which provide e.g. automatic instancing or clustered deferred rendering, still maintained for curiosity reasons
		- Direct3D 10
		- Direct3D 9
- Render targets
	- Swap chains, render into one or multiple operation system windows
	- Framebuffer object (FBO) used for render to texture, support for multiple render targets (MRT), support for multisample (MSAA)
- Shaders
	- Shader types
		- Vertex shader (VS)
		- Tessellation control shader (TCS, "hull shader" in Direct3D terminology)
		- Tessellation evaluation shader (TES, "domain shader" in Direct3D terminology)
		- Geometry shader (GS)
		- Fragment shader (FS, "pixel shader" in Direct3D terminology)
		- Compute shader (CS)
	- Shader data sources
		- Shader bytecode (aka shader microcode, binary large object (BLOB))
			- Vulkan and OpenGL: SPIR-V support for cross-platform vendor and GPU driver independent shader bytecodes
				- Optional build in online GLSL to SPIR-V compilation using [glslang](https://github.com/KhronosGroup/glslang), offline compilation before shipping a product is preferred of course but not mandatory
				- Using [SMOL-V](https://github.com/aras-p/smol-v): like Vulkan/Khronos SPIR-V, but smaller
		- Shader source code
- Buffers
	- Vertex array object (VAO, input-assembler (IA) stage) with support for multiple vertex streams
		- Vertex buffer object (VBO, input-assembler (IA) stage)
		- Index buffer object (IBO, input-assembler (IA) stage)
	- Texture buffer object (TBO)
	- Structured buffer object
	- Indirect buffer object with optional internal emulation, draw methods always use an indirect buffer to have an unified draw call API
	- Uniform buffer object (UBO, "constant buffer" in Direct3D terminology)
	- Command buffer mandatory by design, not just build on top
- Textures: 1D, 2D, 2D array, 3D, cube
- State objects with mapping to API specific settings during creation, not runtime
	- Graphics pipeline state object (PSO) which directly maps to Direct3D 12, other backends internally subdivide into
		- Rasterizer state object (rasterizer stage (RS))
		- Depth stencil state object (output-merger (OM) stage)
		- Blend state object (output-merger (OM) stage)
	- Sampler state object (SO)
- Instancing support
	- Instanced arrays (shader model 3 feature, vertex array element advancing per-instance instead of per-vertex)
	- Draw instanced (shader model 4 feature, build in shader variable holding the current instance ID)
- Debug methods
	- When using Direct3D <11.1, those methods map to the Direct3D 9 PIX functions (D3DPERF_* functions, also works directly within VisualStudio 2017 out-of-the-box)
	- Used inside the renderer implementations for better renderer debugging
- Supported asynchronous queries: Occlusion, pipeline statistics and timestamp
- Renderer implementation specific optimizations
	- OpenGL: Usage of direct state access (DSA), if available


Renderer Runtime (e.g. "The Game")
======
- During runtime, only platform optimized and compressed binary assets are used
	- No inefficient generic stuff, no e.g. endianess handling, primarily raw chunks which can be fed into memory and GPU as efficient as possible
	- Efficient [CRN](https://github.com/BinomialLLC/crunch) textures are used by default, DDS is supported as well
	- Using [LZ4](http://lz4.github.io/lz4/) compression
- Asynchronous loading for all resources: To fight lags, micro stutter / judder, especially for virtual reality applications one needs a constant framerate
- Material and shader blueprint system which was designed from ground up for pipeline state object (PSO) architecture
	- New material types can be added without a single line of C++ source code, meaning technical artists can create and fine-tune the shaders in realtime
	- Materials reference material blueprints and are just a list of key-value-pairs
	- Shader language specifics are abstracted away: Write shaders once, use them across multiple renderer APIs
	- Support for shader combinations (Uber-shaders)
	- Support for reusable shader pieces
	- Material inheritance for materials which should share common properties, but differ in other properties
	- Using [MojoShader](https://icculus.org/mojoshader/) as shader preprocessor so the resulting shader source codes are compact and easy to debug
	- Asynchronous pipeline state compilation, including a fallback system to reduce visual artifacts in case of pipeline cache misses
- Compositor: Setup your overall rendering flow without a single line of C++ source code
	- The compositor is using the material blueprint system, meaning compact C++ implementation while offering mighty possibilities
	- Using [Reversed-Z](https://developer.nvidia.com/content/depth-precision-visualized) for improved depth buffer precision to reduce z-fighting
	- Using camera relative rendering for rendering large scale scenes without jittering/wobbling
	- Using 64 bit world space position
	- Blurred stabilized cascaded (CSM) exponential variance (EVSM) shadow mapping basing on ["A Sampling of Shadow Techniques"](https://mynameismjp.wordpress.com/2013/09/10/shadow-maps/) by Matt Pettineo
	- Resolution scale support
- Scene as most top-level concept: Fancy-feature set kept simple because more complex applications / games usually add an entity-component-system
- [Sorted render queue](http://realtimecollisiondetection.net/blog/?p=86) fed with generic renderables to decouple concrete constructs like meshes, particles, terrain etc. from the generic rendering
- Using mathematics library [GLM](https://glm.g-truc.net/)
- Using [xsimd](https://github.com/QuantStack/xsimd) for SIMD intrinsics
- Cache efficient SIMD multi-threaded frustum culling basing on ["The Implementation of Frustum Culling in Stingray"](http://bitsquid.blogspot.de/2016/10/the-implementation-of-frustum-culling.html)
- [ImGui](https://github.com/ocornut/imgui) is used as debug GUI to be able to quickly add interactive GUI elements for debugging, prototyping or visualization
	- Usage of [ImGuizmo](https://github.com/CedricGuillemet/ImGuizmo) 3D gizmo extension
- Virtual reality manager which is internally using [OpenVR](https://github.com/ValveSoftware/openvr) for head-mounted display support
	- Animated controller visualization supported
	- Single pass stereo rendering via instancing
	- Hidden area mesh supported
- Abstract file interface so the Unrimp user has control over the file handling
	- Optional header-only standard implementations for UTF-8 STD file streams as well as [PhysicsFS](https://icculus.org/physfs/) for shipping packed asset packages are provided
- Texture top mipmap removal support while loading textures for efficient texture quality reduction
- Light
	- Types: Directional, point and spot
	- High-level sunlight controlled via time-of-day
- Skeleton animation
- Particles rendering
- Terrain
	- This software contains source code provided by NVIDIA Corporation. The height map terrain tessellation implementation is basing on ["DirectX 11 Terrain Tessellation"](https://developer.nvidia.com/sites/default/files/akamai/gamedev/files/sdk/11/TerrainTessellation_WhitePaper.pdf) by Iain Cantlay and the concrete implementation "TerrainTessellation"-sample inside ["NVIDIA Direct3D SDK 11"](https://developer.nvidia.com/dx11-samples).
	- Terrain data created by [Marcel Gonzales](http://www.marcelgonzales.com/)
	- [Procedural shader splatting for elevation/slope based blending](http://www.dice.se/wp-content/uploads/2014/12/Chapter5-Andersson-Terrain_Rendering_in_Frostbite.pdf) with additional splash map
	- [Height map based texture layer blending](https://www.gamedev.net/articles/programming/graphics/advanced-terrain-texture-splatting-r3287/)
	- [Triplanar texture mapping](https://medium.com/@bgolus/normal-mapping-for-a-triplanar-shader-10bf39dca05a)
- Sky
	- Classic environment cube map skybox
	- Procedural sky
		- Hosek-Wilkie sky which is also used to derive a sun color
		- Distance clouds
		- Sun
- Volume rendering


Renderer Toolkit (e.g. "The Editor")
======
- Project compiler will transform source data into runtime data and while doing so tries to detect editing issues at tooltime as early as possible to reduce runtime harm and long debugging seasons
- Asynchronous resource compilation and hot reloading for all resources if the toolkit is enabled (true for production, probably not true for shipped titles)
	- Shader-resource example: It's possible to develop shaders while the application is running and see changes instantly
- Most source file formats are using JSON: [RapidJSON](http://rapidjson.org/) is used for parsing
- Performs optimizations and validations at tooltime instead of runtime. Examples:
	- Strips comments from shader source codes
	- Checks the material blueprint resources for valid uniform buffer packing rules
- Mesh compiler
	- Using [Assimp](http://www.assimp.org/) (Open Asset Import Library) for mesh import and post processing like joining identical vertices, vertex cache optimization etc.
	- Using [mikktspace](https://wiki.blender.org/index.php/Dev:Shading/Tangent_Space_Normal_Maps) by Morten S. Mikkelsen for semi-standard mesh tangent space generation
- Texture compiler
	- Using enhanced [Unity Crunch](https://github.com/Unity-Technologies/crunch/tree/unity) (better encoder performance and ETC2 support) version of [Crunch](https://github.com/BinomialLLC/crunch) for mipmap generation and compression
	- Support for [normal map compression](http://www.nvidia.com/object/real-time-normal-map-dxt-compression.html)
	- Support for [alpha mipmaps](http://the-witness.net/news/2010/09/computing-alpha-mipmaps/)
	- Support for creating a cube-map out of six provided individual textures
	- Support for 2D-LUT to 3D-LUT conversion
	- Support for texture channel packing
	- Support for defining texture arrays
	- Toksvig specular anti-aliasing basing on ["Specular Showdown in the Wild West"](http://blog.selfshadow.com/2011/07/22/specular-showdown/) by Stephen Hill to reduce shimmering/sparkling via texture modifications during texture asset compilation
- [Sketchfab](https://sketchfab.com/) asset importer without the need to unzip the downloaded meshes first


Examples (just some high level keywords)
======
- Memory leaks: On Microsoft Windows, "_CrtMemCheckpoint()" and "_CrtMemDumpAllObjectsSince()" is used by default to detect memory leaks while developing and not later on. In case something triggers, use the [diagnostic tools provided by Visual Studio 2017](http://blogs.microsoft.co.il/sasha/2014/12/01/native-memory-leak-diagnostics-visual-studio-2015/) or third parts tools to locate the memory leak in detail.
- [Hierarchical depth buffer (aka Hi-Z map or HZB)](http://rastergrid.com/blog/2010/10/hierarchical-z-map-based-occlusion-culling/), useful for GPU occlusion culling, screen space reflections as well as using the second depth buffer mipmap for e.g. a half-sized volumetric light/fog bilateral upsampling
- Physically based shading (PBS) using "metallic workflow" (aka "metal-rough-workflow" aka "Albedo/Metallic/Roughness") instead of "specular workflow" (aka "specular-gloss-workflow" aka "Diffuse/Specular/Glossines")
- Microsoft Windows: ["NVIDIA Optimus"](http://developer.download.nvidia.com/devzone/devcenter/gamegraphics/files/OptimusRenderingPolicies.pdf) and ["AMD Dynamic Switchable Graphic"](https://community.amd.com/message/1307599#comment-1307599) awareness to reduce the risk of getting the integrated graphics unit assigned when there's a dedicated graphics card as well
- GPU dual quaternion skinning (DQS), linear blend skinning (LBS) using matrices path is available as well
- [Volumetric light/fog](https://fr.slideshare.net/BenjaminGlatzel/volumetric-lighting-for-many-lights-in-lords-of-the-fallen) (aka crepuscular rays, god rays, sunbeams, sunbursts, light shafts or star flare)
- [Dynamic rain accumulated water in a hole/cracks with puddle and rain drops](https://seblagarde.wordpress.com/2013/04/14/water-drop-3b-physically-based-wet-surfaces/) [and water streaks](https://deepspacebanana.github.io/deepspacebanana.github.io/blog/shader/art/unreal%20engine/Rainy-Surface-Shader-Part-2)
- [Custom resolved MSAA for antialiased deferred rendering and temporal anti-aliasing](https://mynameismjp.wordpress.com/2012/10/28/msaa-resolve-filters/)
- Distortion which can e.g. be used for refraction and heat haze / heat shimmer
- General Purpose Computation on Graphics Processing Unit (GPGPU)
- Gaussian blur, used to e.g. blur the transparent ImGui background
- High dynamic range (HDR) rendering with adaptive luminance
- Tangent space BC5/3DC/ATI2N stored normal maps
- [Color correction via 3D lookup table (LUT)](https://developer.nvidia.com/gpugems/GPUGems2/gpugems2_chapter24.html)
- [Screen space ambient occlusion (SSAO)](http://john-chapman-graphics.blogspot.de/2013/01/ssao-tutorial.html)
- [Fast Approximate Anti-Aliasing (FXAA)](http://developer.download.nvidia.com/assets/gamedev/files/sdk/11/FXAA_WhitePaper.pdf)
- [Parallax occlusion mapping (POM)](https://developer.amd.com/wordpress/media/2012/10/Tatarchuk-POM.pdf)
- [Old CRT post processing effect](https://www.shadertoy.com/view/MsXGD4)
- [Screen space reflections (SSR)](http://casual-effects.blogspot.de/2014/08/screen-space-ray-tracing.html)
- [Physically based wet surfaces](https://seblagarde.wordpress.com/2013/04/14/water-drop-3b-physically-based-wet-surfaces/)
- [Gamma correct rendering](https://developer.nvidia.com/gpugems/GPUGems3/gpugems3_ch24.html)
- Bloom with dirty lens
- Chromatic aberration
- Clustered shading
- [Alpha to coverage](https://medium.com/@bgolus/anti-aliased-alpha-test-the-esoteric-alpha-to-coverage-8b177335ae4f)
- [Pseudo lens flare](http://john-chapman-graphics.blogspot.de/2013/02/pseudo-lens-flare.html)
- Depth of field
- [Soft particles](https://developer.download.nvidia.com/whitepapers/2007/SDK10/SoftParticles_hi.pdf)
- Motion blur
- Film grain
- [RGB dither](http://media.steampowered.com/apps/valve/2015/Alex_Vlachos_Advanced_VR_Rendering_GDC2015.pdf)
- Depth fog
- Vignette
- Sharpen


Terminology and Acronyms
======
- Renderer backend
	- Vertex buffer object (VBO)
	- Index buffer object (IBO)
	- Vertex array object (VAO)
	- Uniform buffer object (UBO), "constant buffer" in Direct3D terminology
	- Texture buffer object (TBO)
	- Sampler state object (SO)
	- Root signature (Direct3D terminology) = pipeline layout in Vulkan terminology
	- Pipeline state object (PSO, there's a graphics PSO and a compute PSO)
	- Vertex shader (VS)
	- Tessellation control shader (TCS), "hull shader" in Direct3D terminology
	- Tessellation evaluation shader (TES), "domain shader" in Direct3D terminology
	- Geometry shader (GS)
	- Fragment shader (FS), "pixel shader" in Direct3D terminology
	- Compute shader (CS)
	- Uniform buffer view (UBV)
	- Shader resource view (SRV)
	- Unordered access view (UAV)
- Renderer runtime
	- Asset: Lightweight content metadata like ID, type and location (texture, mesh, shader etc. - on this abstraction level everything is an asset)
	- Resource: A concrete asset type used during runtime in-memory (texture, mesh, shader etc.)
	- Mesh: 3D-model consisting of a vertex- and index-buffer, geometry subdivided into sub-meshes
	- Material and shader blueprint: High-level rendering description
	- Material: Just a property-set used as shader input
	- HMD: Head mounted display


Microsoft Windows: First Example Kickoff
======
- [Open Visual Studio 2017 and select "File -> Open -> CMake..." -> "unrimp/CMakeLists.txt"](https://blogs.msdn.microsoft.com/vcblog/2016/10/05/cmake-support-in-visual-studio/)
- Build "Windows_x64_Shared" project settings and when done use Visual Studio menu -> CMake -> Install -> Unrimp
- Compile the runtime assets by starting "unrimp/Binary/Windows_x64_Shared/ExampleProjectCompiler.exe"
- Run "unrimp/Binary/Windows_x64_Shared/Examples.exe" (is using default command line arguments "unrimp/Binary/Windows_x64_Shared/Examples.exe ImGuiExampleSelector -r Direct3D11")
- For debugging with Visual Studio 2017, use "Examples.exe (Install with Arguments)" or "ExampleProjectCompiler.exe (Install with Arguments)" as startup item
- Modify ["launch.vs.json"](https://github.com/Microsoft/vscode-cpptools/blob/master/launch.md) to change the Visual Studio application launch options, e.g. to start the "FirstScene"-example by default


Microsoft Windows: Using the Unrimp examples together with [SDL2](https://www.libsdl.org/)
======
- Download e.g. "SDL2-devel-2.0.8-VC.zip" from https://www.libsdl.org/download-2.0.php and extract it into "unrimp/External/SDL2" (directory contains "include" and "lib")
- Inside "unrimp/CMakeSettings.json", set "EXAMPLE_SDL2_ENABLED" to "1"


Microsoft Windows: Targeting Android
======
- Install the Visual Studio 2017 ["Mobile development with C++"-workload](https://blogs.msdn.microsoft.com/vcblog/2017/04/18/android-and-ios-development-with-c-in-visual-studio/)
- Build and run the ["Native Activity Application (Android)"-template](https://docs.microsoft.com/en-us/visualstudio/cross-platform/create-an-android-native-activity-app) to verify your installation and setup
- Unrimp needs at least ["android-ndk-r17"](https://developer.android.com/ndk/downloads/) due to the usage of C++17 features like the filesystem, download this Android NDK if needed
- [Open Visual Studio 2017 and select "File -> Open -> CMake..." -> "unrimp/CMakeLists.txt"](https://blogs.msdn.microsoft.com/vcblog/2016/10/05/cmake-support-in-visual-studio/)
- Build "Android_arm64_Static" project settings
- TODO(co) Work-in-progress: Compile data for mobile target, upload to device to test and debug


Useful Microsoft Windows Developer Tools
======
- When profiling a product
	- Memory
		- [Diagnostic tools provided by Visual Studio 2017](http://blogs.microsoft.co.il/sasha/2014/12/01/native-memory-leak-diagnostics-visual-studio-2015/)
		- Free and open-source: [MTuner](https://github.com/milostosic/MTuner)
			- As of October 14, 2017: Doesn't work with Visual Studio 2017 (v141) x86 but works with x64
	- Graphics
		- Direct3D 11 graphics debugging can be done directly inside Visual Studio 2017
	- CPU
		- For CPU profiling the tool [Very Sleepy](http://www.codersnotes.com/sleepy/) is easy to use while providing useful results
	- Compile time
		- [Header Hero](http://aras-p.info/blog/2018/01/17/Header-Hero-Improvements/)
		- [/d2cgsummary](http://aras-p.info/blog/2017/10/23/Best-unknown-MSVC-flag-d2cgsummary/)
	- Binary size
		- ["Sizer - executable size breakdown (2007)"](http://aras-p.info/projSizer.html): "Command line tool that reports size of things (functions, data, classes, templates, object files) in a Visual Studio compiled exe/dll. Extracts info from debug information (.pdb) file."
			- As of October 14, 2017: Doesn't work with Visual Studio 2017 (v141), compile for Visual Studio 2015 (v140) if you want to analyze the binaries using Sizer
- Static code analysis: [Cppcheck](http://cppcheck.sourceforge.net/)
- Checking external dependencies of exe and dll: [Dependencies](https://github.com/lucasg/Dependencies) which is a rewrite of the old legacy software [Dependency Walker](http://www.dependencywalker.com/)
- Texture handling related: [Compressonator](https://github.com/GPUOpen-Tools/Compressonator)
- When shipping a product, use a static build and e.g. [UPX](https://upx.github.io/) to get executables even more compact on end-user-systems
- OpenGL ES 3 development: PowerVR (SDK 2017 R2 or newer) works well. [Copy "libEGL.dll" and "libGLESv2.dll" from e.g. "<Installation>/PowerVR_Graphics/PowerVR_Tools/PVRVFrame/Library/Windows_x86_32" e.g. into "unrimp/Binary/Windows_x86d_Shared".](https://www.imgtec.com/blog/5-easy-steps-to-add-pvrtrace-libraries-to-your-own-opengl-es-application/)
- ["Visual Studio Spell Checker"](https://marketplace.visualstudio.com/items?itemName=EWoodruff.VisualStudioSpellCheckerVS2017andLater) extension to reduce the risk automatically detectable typos get into APIs were they might stay forever


Useful Online Asset Data Sources
======
- Realtime ready meshes with textures and a web-browser realtime preview: [Sketchfab](https://sketchfab.com/), search for downloadable
- Realtime ready meshes with textures: [NVIDIA ORCA: Open Research Content Archive](https://developer.nvidia.com/orca)
- Realtime ready shaders and a web-browser realtime preview: [Shadertoy](https://www.shadertoy.com/)
- Realtime ready post-processing shaders: [reshade-shaders](https://github.com/crosire/reshade-shaders/tree/master/Shaders)
- [Open Game Art](https://opengameart.org/)


Asset Prefixes
======
The asset prefixes are just used inside the Unrimp examples and are not fixed build in. Feel free to use other asset prefixes or no prefixes at all in your projects.
- "CN" = "Compositor node"
- "CW" = "Compositor workspace"
- "M"  = "Material"
- "MB" = "Material blueprint"
- "S"  = "Scene"
- "SA" = "Skeleton animation"
- "SB" = "Shader blueprint"
- "SM" = "Static or skinned mesh"
- "SP" = "Shader piece"
- "T"  = "Texture"
- "VA" = "Vertex attributes"


Assets
======
- Each source asset which gets compiled into an runtime usable asset has a ".asset"-file which defines which asset compiler should be used as well as an optional asset compiler configuration. The rule is to not manipulate the source asset itself for asset compilation but to just decorate it with additional information. For ease of use there's support for automatically in-memory generated ".asset"-files which works for asset compilers which accept just a single unique filename extension.
- Assets are referenced by using
	- Renderer toolkit source asset ID naming scheme ```"<name>.asset"```
		- Absolute: "${PROJECT_NAME}" inserts the name of the project the currently processed asset is in, only valid at the beginning of source asset IDs
		- Relative: "./" uses the directory the currently processed asset is in, only valid at the beginning of source asset IDs
		- Relative: "../" switches into the parent directory the currently processed asset is in, only valid at the beginning of source asset IDs
	- Compiled or runtime generated asset ID naming scheme ```"<project name>/<asset directory>/<asset name>"```
- Examples for asset references inside source assets
	- Referencing a source asset inside the same project as the currently compiled source asset
		- "${PROJECT_NAME}/Blueprint/Sky/M_Sky.asset": Referencing a source asset which is inside the same project as the currently compiled source asset
		- "./MB_Debug.asset": Referencing a source asset which is inside the same directory as the currently compiled source asset
		- "./MyDirectory/MB_Debug.asset": Referencing a source asset which is inside a sub-directory named "MyDirectory" which is inside same directory as the currently compiled source asset
		- "../MB_Debug.asset": Referencing a source asset which is inside the parent directory of the directory the currently compiled source asset is in
		- "../../MB_Debug.asset": Referencing a source asset which is inside the parent directory of the parent directory of the directory the currently compiled source asset is in
		- "../../MyDirectory/MB_Debug.asset": Referencing a source asset which is inside a sub-directory named "MyDirectory" which is inside the parent directory of the parent directory of the directory the currently compiled source asset is in
	- "MyProject/Blueprint/Sky/M_Sky.asset": Referencing a source asset which is inside another project named "MyProject"
	- "Unrimp/Texture/DynamicByCode/BlackMap2D": Referencing an asset which is dynamically created during runtime without having a compiled source asset


Hints
======
- Error strategy
	- Inside renderer toolkit: Exceptions in extreme, up to no error tolerance. If something smells odd, blame it to make it possible to detect problems as early as possible in the production pipeline.
	- Inside renderer runtime: The show must go on. If the floor breaks, just keep smiling and continue dancing.
- Windows using Visual Studio 2017 C++ Open Folder and CMake: IntelliSense keeps failing
	- Visual Studio 2017 -> Menu bar -> "Options" -> "Text Editor" -> "C/C++" -> "Advanced" -> "Inactive Platform IntelliSense Limit" -> Set it to e.g. 16 (see https://blogs.msdn.microsoft.com/vcblog/2018/01/10/intellisense-enhancements-for-cpp-open-folder-and-cmake/ )
- How to test the 64 bit world space position support?
	- Inside "SceneResourceLoader.cpp" -> "nodeDeserialization()" after reading a node, add an 100.000.000 offset to the node transform position

The unified renderer interface can't unify some graphics API behaviour differences. Here's a list of hints you might want to know:
- Texel coordinate system origin
	- OpenGL: Left/bottom
	- Direct3D: Left/top
		- See ["Coordinate Systems (Direct3D 10)"](http://msdn.microsoft.com/en-us/library/windows/desktop/cc308049%28v=vs.85%29.aspx) at MSDN
- Pixel coordinate system 
	- Direct3D: See ["Coordinate Systems (Direct3D 10)"](http://msdn.microsoft.com/en-us/library/windows/desktop/cc308049%28v=vs.85%29.aspx) at MSDN
- Clip space coordinate system [-1, -1] ... [1, 1]
	- Vulkan, Direct3D, OpenGL with "GL_ARB_clip_control"-extension: Left-handed coordinate system with clip space depth value range 0..1
	- OpenGL without "GL_ARB_clip_control"-extension: Right-handed coordinate system with clip space depth value range -1..1
	- Additional information: ["The Cg Tutorial"-book, section "4.1.8 The Projection Transform"](http://http.developer.nvidia.com/CgTutorial/cg_tutorial_chapter04.html)
- Physically based shading (PBS)
	- ["Physically-Based Rendering, And You Can Too!"](https://www.marmoset.co/posts/physically-based-rendering-and-you-can-too/) - By Joe "EarthQuake" Wilson
	- ["Moving Frostbite to PBR"](https://www.ea.com/frostbite/news/moving-frostbite-to-pb)
	- ["Physically Based Shading and Image Based Lighting"](http://www.trentreed.net/blog/physically-based-shading-and-image-based-lighting/)
	- ["The comprehensive PBR guide"](https://www.allegorithmic.com/pbr-guide)


[MIT License](https://opensource.org/licenses/MIT)
======
Copyright (c) 2012-2019 The Unrimp Team

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.