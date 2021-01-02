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
#include "RendererToolkit/Private/AssetCompiler/IAssetCompiler.h"


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
namespace RendererToolkit
{


	//[-------------------------------------------------------]
	//[ Classes                                               ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Texture asset compiler
	*
	*  @remarks
	*    == Texture Semantics ==
	*    Overview of source texture semantics including recommended postfixes to use while authoring texture source assets:
	*    --------------------------------------------------------------
	*    Texture Semantic                 Postfix    Data         sRGB    Comment
	*    --------------------------------------------------------------
	*    ALBEDO_MAP                       _a         rgb          Yes     Albedo map, also known as base color. Raw color with no lighting information. Small amount of ambient occlusion can be baked in if using it for micro-surface occlusion. For a metallic worfkow, the color range for dark values should stay within 30-50 RGB. Never have dark values below 30 RGB. The brightest color value should not go above 240 RGB. With metal/rough, the areas indicated as metal in the metallic map have a corresponding metal reflectance value in the base color map. The metal reflectance value in the base color needs to be a measured real-world value. Transitional areas in the metal map (not raw metal 1.0 white) need to have the metal reflectance value lowered to indicate that its reflectance value is not raw metal.
	*    ALPHA_MAP                        _alpha     luminance    No      Alpha map. 8-bit-alpha as some artists might call it.
	*    NORMAL_MAP                       _n         rgb          No      Tangent space normal map
	*    ROUGHNESS_MAP                    _r         luminance    No      Roughness map = 1 - gloss map. Metallic worfkow: Describes the microsurface of the object. White 1.0 is rough and black 0.0 is smooth. The microsurface if rough can cause the light rays to scatter and make the highlight appear dimmer and more broad. The same amount of light energy is reflected going out as coming into the surface. This map has the most artistic freedom. There is no wrong answers here. This map gives the asset the most character as it truly describes the surface e.g. scratches, fingerprints, smudges, grime etc.
	*    METALLIC_MAP                     _m         luminance    No      Metallic map. Metallic worfkow: Tells the shader if something is metal or not. Raw Metal = 1.0 white and non metal = 0.0 black. There can be transitional gray values that indicate something covering the raw metal such as dirt. With metal/rough, you only have control over metal reflectance values. The dielectric values are set to 0.04 or 4% which is most dielectric materials.
	*    EMISSIVE_MAP                     _e         rgb          Yes     Emissive map
	*    HEIGHT_MAP                       _h         luminance    No      Height map
	*    TINT_MAP                         _t         luminance    No      Tint map
	*    AMBIENT_OCCLUSION_MAP            _ao        luminance    No      Ambient occlusion map
	*    REFLECTION_2D_MAP                _r2d       rgb          Yes     Reflection 2D map
	*    REFLECTION_CUBE_MAP              _rcube     rgb          Yes     Reflection cube map
	*    COLOR_CORRECTION_LOOKUP_TABLE    _lut       rgb          No      Color correction lookup table
	*
	*
	*   == Texture Channel Packing ==
	*   To be as memory efficient as possible during runtime, the texture compiler supports texture channel packing. Meaning for example that the luminance roughness, metallic and height maps are not
	*   used as individual textures during runtime, but are packed into a single texture. The recommended texture asset naming scheme is as following: "<texture name><semantic postfix><optional source component><channel>"
	*
	*   Using the texture semantics as specified in the table above here are more concrete examples and how to read them:
	*   - "<texture name>_argb_nxa", e.g. "stone_argb_nxa"
	*     - RGB channel = Albedo map ("_a"-postfix)
	*     - A channel   = x component of normal map ("_n"-postfix)
	*   - "<texture name>_hr_rg_mb_nya", e.g. "stone_hr_rg_mb_nya"
	*     - R channel = Height map ("_h"-postfix)
	*     - G channel = Roughness map ("_r"-postfix)
	*     - B channel = Metallic map ("_m"-postfix)
	*     - A channel = y component of normal map ("_n"-postfix)
	*
	*   The rest of the textures are not getting packed since those are more special and not that often used textures.
	*/
	class TextureAssetCompiler final : public IAssetCompiler
	{


	//[-------------------------------------------------------]
	//[ Public definitions                                    ]
	//[-------------------------------------------------------]
	public:
		static constexpr uint32_t CLASS_ID = STRING_ID("RendererToolkit::TextureAssetCompiler");


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		explicit TextureAssetCompiler(const Context& context);
		virtual ~TextureAssetCompiler() override;


	//[-------------------------------------------------------]
	//[ Public virtual RendererToolkit::IAssetCompiler methods ]
	//[-------------------------------------------------------]
	public:
		[[nodiscard]] inline virtual AssetCompilerClassId getAssetCompilerClassId() const override
		{
			return CLASS_ID;
		}

		[[nodiscard]] inline virtual std::string_view getOptionalUniqueAssetFilenameExtension() const override
		{
			// Multiple source asset filename extensions, so no unique source asset filename extension here
			return "";
		}

		[[nodiscard]] virtual std::string getVirtualOutputAssetFilename(const Input& input, const Configuration& configuration) const override;
		[[nodiscard]] virtual bool checkIfChanged(const Input& input, const Configuration& configuration) const override;
		virtual void compile(const Input& input, const Configuration& configuration) const override;


	};


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
} // RendererToolkit
