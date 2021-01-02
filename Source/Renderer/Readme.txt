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


== Description ==
Renderer.


== Dependencies ==
- ACL (directly compiled and linked in)
- crunch (header only part of the library, doesn't depend on crn compression library)
- GLM (header only library)
- ImGui (directly compiled and linked in)
- ImGuizmo (directly compiled and linked in)
- lz4 (directly compiled and linked in)
- MikkTSpace (directly compiled and linked in)
- MojoShader (directly compiled and linked in)
- OpenVR (header with dynamic runtime linking)
- PhysicsFS (dependency of projects using the provided interface implementation, directly compiled and linked in)
- Remotery (dependency of projects using the provided interface implementation, directly compiled and linked in)
- RenderDoc (dependency of projects using the provided interface implementation, header only library)
- xsimd (header only library)


== Preprocessor Definitions ==
- Set "RENDERER_EXPORTS" as preprocessor definition when building this library as shared library
- Set "RENDERER_GRAPHICS_DEBUGGER" as preprocessor definition to enable graphics debugger support
- Set "RENDERER_PROFILER" as preprocessor definition to enable profiler support
- Set "RENDERER_IMGUI" as preprocessor definition to enable ImGui support
- Set "RENDERER_OPENVR" as preprocessor definition to enable OpenVR support
- Do also have a look into the RHI header file for RHI implementation preprocessor definitions