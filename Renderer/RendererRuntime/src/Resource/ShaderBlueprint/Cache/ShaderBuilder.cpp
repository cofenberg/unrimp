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

// Heavily basing on the OGRE 2.1 HLMS shader builder which is directly part of the OGRE class "Ogre::Hlms"
/*
-----------------------------------------------------------------------------
This source file is part of OGRE
    (Object-oriented Graphics Rendering Engine)
For the latest info, see http://www.ogre3d.org/

Copyright (c) 2000-2014 Torus Knot Software Ltd

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
-----------------------------------------------------------------------------
*/


//[-------------------------------------------------------]
//[ Includes                                              ]
//[-------------------------------------------------------]
#include "RendererRuntime/Resource/ShaderBlueprint/Cache/ShaderBuilder.h"
#include "RendererRuntime/Resource/ShaderBlueprint/Cache/Preprocessor/Preprocessor.h"
#include "RendererRuntime/Resource/ShaderBlueprint/ShaderBlueprintResourceManager.h"
#include "RendererRuntime/Resource/ShaderBlueprint/ShaderBlueprintResource.h"
#include "RendererRuntime/Resource/ShaderPiece/ShaderPieceResourceManager.h"
#include "RendererRuntime/Resource/ShaderPiece/ShaderPieceResource.h"
#include "RendererRuntime/Asset/AssetManager.h"
#include "RendererRuntime/Core/Math/Math.h"
#include "RendererRuntime/IRendererRuntime.h"

#include <algorithm>


//[-------------------------------------------------------]
//[ Anonymous detail namespace                            ]
//[-------------------------------------------------------]
namespace
{
	namespace detail
	{


		int setOp(int, int op2)
		{
			return op2;
		}

		int addOp(int op1, int op2)
		{
			return op1 + op2;
		}

		int subOp(int op1, int op2)
		{
			return op1 - op2;
		}

		int mulOp(int op1, int op2)
		{
			return op1 * op2;
		}

		int divOp(int op1, int op2)
		{
			return op1 / op2;
		}

		int modOp(int op1, int op2)
		{
			return op1 % op2;
		}

		int minOp(int op1, int op2)
		{
			return std::min(op1, op2);
		}

		int maxOp(int op1, int op2)
		{
			return std::max(op1, op2);
		}

		struct Operation final
		{
			const char*	opName;
			size_t		length;

			int (*opFunc)(int, int);

			Operation(const char* _name, size_t len, int (*_opFunc)(int, int)) :
				opName(_name),
				length(len),
				opFunc(_opFunc)
			{
			}
		};

		const Operation c_operations[8] =
		{
			Operation("pset", sizeof("@pset"), &setOp),
			Operation("padd", sizeof("@padd"), &addOp),
			Operation("psub", sizeof("@psub"), &subOp),
			Operation("pmul", sizeof("@pmul"), &mulOp),
			Operation("pdiv", sizeof("@pdiv"), &divOp),
			Operation("pmod", sizeof("@pmod"), &modOp),
			Operation("pmin", sizeof("@pmin"), &minOp),
			Operation("pmax", sizeof("@pmax"), &maxOp)
		};

		const Operation c_counterOperations[10] =
		{
			Operation("counter", sizeof("@counter"), 0),
			Operation("value", sizeof("@value"), 0),
			Operation("set", sizeof("@set"), &setOp),
			Operation("add", sizeof("@add"), &addOp),
			Operation("sub", sizeof("@sub"), &subOp),
			Operation("mul", sizeof("@mul"), &mulOp),
			Operation("div", sizeof("@div"), &divOp),
			Operation("mod", sizeof("@mod"), &modOp),
			Operation("min", sizeof("@min"), &minOp),
			Operation("max", sizeof("@max"), &maxOp)
		};

		enum ExpressionType
		{
			EXPR_OPERATOR_OR,	//||
			EXPR_OPERATOR_AND,	//&&
			EXPR_OBJECT,		//(...)
			EXPR_VAR
		};

		struct Expression final
		{
			bool					result;
			bool					negated;
			ExpressionType			type;
			std::vector<Expression>	children;
			std::string				value;

			Expression() :
				result(false),
				negated(false),
				type(EXPR_VAR)
			{
			}
		};

		typedef std::vector<Expression> ExpressionVec;
		typedef std::vector<std::string> StringVector;

		class SubStringRef
		{


			const std::string* mOriginal;
			size_t			   mStart;
			size_t			   mEnd;


		public:
			SubStringRef(const std::string* original, size_t start) :
				mOriginal(original),
				mStart(start),
				mEnd(original->size())
			{
				assert(start <= original->size());
			}

			SubStringRef(const std::string* original, size_t _start, size_t _end) :
				mOriginal(original),
				mStart(_start),
				mEnd(_end)
			{
				assert(_start <= _end);
				assert(_end <= original->size());
			}

			SubStringRef(const std::string* original, std::string::const_iterator _start) :
				mOriginal(original),
				mStart(static_cast<size_t>(_start - original->begin())),
				mEnd(original->size())
			{
				// Nothing here
			}

			size_t find(const char* value, size_t pos = 0) const
			{
				size_t retVal = mOriginal->find(value, mStart + pos);
				if (retVal >= mEnd)
				{
					retVal = std::string::npos;
				}
				else if (retVal != std::string::npos)
				{
					retVal -= mStart;
				}

				return retVal;
			}

			size_t find(const std::string& value) const
			{
				size_t retVal = mOriginal->find(value, mStart);
				if (retVal >= mEnd)
				{
					retVal = std::string::npos;
				}
				else if (retVal != std::string::npos)
				{
					retVal -= mStart;
				}

				return retVal;
			}

			size_t findFirstOf(const char* c, size_t pos) const
			{
				size_t retVal = mOriginal->find_first_of(c, mStart + pos);
				if (retVal >= mEnd)
				{
					retVal = std::string::npos;
				}
				else if (retVal != std::string::npos)
				{
					retVal -= mStart;
				}

				return retVal;
			}

			bool matchEqual(const char* stringCompare) const
			{
				const char* origStr = mOriginal->c_str() + mStart;
				ptrdiff_t length = static_cast<ptrdiff_t>(mEnd - mStart);
				while (*origStr == *stringCompare && *origStr && --length)
				{
					++origStr, ++stringCompare;
				}

				return (length == 0 && *origStr == *stringCompare);
			}

			void setStart(size_t newStart)
			{
				mStart = std::min(newStart, mOriginal->size());
			}

			void setEnd(size_t newEnd)
			{
				mEnd = std::min(newEnd, mOriginal->size());
			}

			size_t getStart() const
			{
				return mStart;
			}

			size_t getEnd() const
			{
				return mEnd;
			}

			size_t getSize() const
			{
				return mEnd - mStart;
			}

			std::string::const_iterator begin() const
			{
				return mOriginal->begin() + static_cast<int>(mStart);
			}

			std::string::const_iterator end() const
			{
				return mOriginal->begin() + static_cast<int>(mEnd);
			}

			const std::string& getOriginalBuffer() const
			{
				return *mOriginal;
			}
		};

		size_t calculateLineCount(const std::string& buffer, size_t idx)
		{
			std::string::const_iterator itor = buffer.begin();
			std::string::const_iterator end  = buffer.begin() + static_cast<int>(idx);

			size_t lineCount = 0;

			while (itor != end)
			{
				if ('\n' == *itor)
				{
					++lineCount;
				}
				++itor;
			}

			return lineCount + 1;
		}

		size_t calculateLineCount(const SubStringRef& subString)
		{
			return calculateLineCount(subString.getOriginalBuffer(), subString.getStart());
		}

		void findBlockEnd(const Renderer::Context& context, SubStringRef& outSubString, bool& syntaxError)
		{
			static constexpr const char* blockNames[] =
			{
				"foreach",
				"property",
				"piece"
			};

			std::string::const_iterator it = outSubString.begin();
			std::string::const_iterator en = outSubString.end();

			int nesting = 0;

			while (it != en && nesting >= 0)
			{
				if ('@' == *it)
				{
					const SubStringRef subString(&outSubString.getOriginalBuffer(), it + 1);

					const size_t idx = subString.find("end");
					if (0 == idx)
					{
						--nesting;
						it += sizeof("end") - 1;
						continue;
					}
					else
					{
						for (size_t i = 0; i < sizeof(blockNames) / sizeof(char*); ++i)
						{
							const size_t idxBlock = subString.find(blockNames[i]);
							if (0 == idxBlock)
							{
								it = subString.begin() + static_cast<int>(strlen(blockNames[i]));
								++nesting;
								break;
							}
						}
					}
				}

				++it;
			}

			assert(nesting >= -1);

			if (it != en && nesting < 0)
			{
				outSubString.setEnd(it - outSubString.getOriginalBuffer().begin() - (sizeof("end") - 1));
			}
			else
			{
				syntaxError = true;
				RENDERER_LOG(context, CRITICAL, "Renderer runtime shader builder: Syntax error at line %lu: Start block (e.g. @foreach; @property) without matching @end\n", static_cast<unsigned long>(calculateLineCount(outSubString)))
			}
		}

		size_t evaluateExpressionEnd(const Renderer::Context& context, const SubStringRef& outSubString)
		{
			std::string::const_iterator it = outSubString.begin();
			std::string::const_iterator en = outSubString.end();

			int nesting = 0;

			while (it != en && nesting >= 0)
			{
				if ('('  == *it)
				{
					++nesting;
				}
				else if (')' == *it)
				{
					--nesting;
				}
				++it;
			}

			assert(nesting >= -1);

			size_t returnValue = std::string::npos;
			if (it != en && nesting < 0)
			{
				returnValue = static_cast<size_t>(it - outSubString.begin() - 1);
			}
			else
			{
				RENDERER_LOG(context, CRITICAL, "Renderer runtime shader builder: Syntax error at line %lu: Opening parenthesis without matching closure\n", static_cast<unsigned long>(calculateLineCount(outSubString)))
			}

			return returnValue;
		}

		bool evaluateExpressionRecursive(const Renderer::Context& context, const RendererRuntime::ShaderProperties& shaderProperties, ExpressionVec& expression, bool& outSyntaxError)
		{
			ExpressionVec::iterator itor = expression.begin();
			ExpressionVec::iterator end  = expression.end();

			while (itor != end)
			{
				if (itor->value == "&&")
				{
					itor->type = EXPR_OPERATOR_AND;
				}
				else if (itor->value == "||")
				{
					itor->type = EXPR_OPERATOR_OR;
				}
				else if (!itor->children.empty())
				{
					itor->type = EXPR_OBJECT;
				}
				else
				{
					itor->type = EXPR_VAR;
				}

				++itor;
			}

			bool syntaxError = outSyntaxError;
			bool lastExpWasOperator = true;

			itor = expression.begin();

			while (itor != end && !syntaxError)
			{
				Expression& exp = *itor;
				if (((EXPR_OPERATOR_OR == exp.type || EXPR_OPERATOR_AND == exp.type) && lastExpWasOperator) || ((EXPR_VAR == exp.type || EXPR_OBJECT == exp.type) && !lastExpWasOperator))
				{
					syntaxError = true;
					RENDERER_LOG(context, CRITICAL, "Renderer runtime shader builder: Unrecognized token '%s'", exp.value.c_str())
				}
				else if (EXPR_OPERATOR_OR == exp.type || EXPR_OPERATOR_AND == exp.type)
				{
					lastExpWasOperator = true;
				}
				else if (EXPR_VAR == exp.type)
				{
					int32_t propertyValue = 0;
					shaderProperties.getPropertyValue(RendererRuntime::StringId(exp.value.c_str()), propertyValue);
					exp.result = (propertyValue != 0);
					lastExpWasOperator = false;
				}
				else
				{
					exp.result = evaluateExpressionRecursive(context, shaderProperties, exp.children, syntaxError);
					lastExpWasOperator = false;
				}

				++itor;
			}

			bool retVal = true;

			if (!syntaxError)
			{
				itor = expression.begin();
				bool andMode = true;

				while (itor != end)
				{
					if (EXPR_OPERATOR_OR == itor->type)
					{
						andMode = false;
					}
					else if (EXPR_OPERATOR_AND == itor->type)
					{
						andMode = true;
					}
					else
					{
						if (andMode)
						{
							retVal &= itor->negated ? !itor->result : itor->result;
						}
						else
						{
							retVal |= itor->negated ? !itor->result : itor->result;
						}
					}

					++itor;
				}
			}

			outSyntaxError = syntaxError;

			return retVal;
		}

		bool evaluateExpression(const Renderer::Context& context, const RendererRuntime::ShaderProperties& shaderProperties, SubStringRef& outSubString, bool& outSyntaxError)
		{
			const size_t expEnd = evaluateExpressionEnd(context, outSubString);
			if (std::string::npos == expEnd)
			{
				outSyntaxError = true;
				return false;
			}

			SubStringRef subString(&outSubString.getOriginalBuffer(), outSubString.getStart(), outSubString.getStart() + expEnd);

			outSubString = SubStringRef(&outSubString.getOriginalBuffer(), outSubString.getStart() + expEnd + 1);

			bool textStarted = false;
			bool syntaxError = false;
			bool nextExpressionNegates = false;

			std::vector<Expression*> expressionParents;
			ExpressionVec outExpressions;
			outExpressions.clear();
			outExpressions.resize(1);

			Expression* currentExpression = &outExpressions.back();

			std::string::const_iterator it = subString.begin();
			std::string::const_iterator en = subString.end();

			while (it != en && !syntaxError)
			{
				const char c = *it;

				if ('(' == c)
				{
					currentExpression->children.push_back(Expression());
					expressionParents.push_back(currentExpression);

					currentExpression->children.back().negated = nextExpressionNegates;

					textStarted = false;
					nextExpressionNegates = false;

					currentExpression = &currentExpression->children.back();
				}
				else if (')' == c)
				{
					if (expressionParents.empty())
					{
						syntaxError = true;
					}
					else
					{
						currentExpression = expressionParents.back();
						expressionParents.pop_back();
					}

					textStarted = false;
				}
				else if (' ' == c || '\t' == c || '\n' == c || '\r' == c)
				{
					textStarted = false;
				}
				else if ('!' == c)
				{
					nextExpressionNegates = true;
				}
				else
				{
					if (!textStarted)
					{
						textStarted = true;
						currentExpression->children.push_back(Expression());
						currentExpression->children.back().negated = nextExpressionNegates;
					}

					if ('&' == c || '|' == c)
					{
						if (currentExpression->children.empty() || nextExpressionNegates)
						{
							syntaxError = true;
						}
						else if (!currentExpression->children.back().value.empty() && c != *(currentExpression->children.back().value.end() - 1))
						{
							currentExpression->children.push_back(Expression());
						}
					}

					currentExpression->children.back().value.push_back(c);
					nextExpressionNegates = false;
				}

				++it;
			}

			bool retVal = false;

			if (!expressionParents.empty())
			{
				syntaxError = true;
			}
			if (!syntaxError)
			{
				retVal = evaluateExpressionRecursive(context, shaderProperties, outExpressions, syntaxError);
			}
			if (syntaxError)
			{
				RENDERER_LOG(context, CRITICAL, "Renderer runtime shader builder: Syntax error at line %lu\n", static_cast<unsigned long>(calculateLineCount(subString)))
			}

			outSyntaxError = syntaxError;

			return retVal;
		}

		void evaluateParamArgs(const Renderer::Context& context, SubStringRef& outSubString, StringVector& outArgs, bool& outSyntaxError)
		{
			const size_t expEnd = evaluateExpressionEnd(context, outSubString);
			if (std::string::npos == expEnd)
			{
				outSyntaxError = true;
				return;
			}

			SubStringRef subString(&outSubString.getOriginalBuffer(), outSubString.getStart(), outSubString.getStart() + expEnd);

			outSubString = SubStringRef(&outSubString.getOriginalBuffer(), outSubString.getStart() + expEnd + 1);

			int expressionState = 0;
			bool syntaxError = false;

			outArgs.clear();
			outArgs.push_back(std::string());

			std::string::const_iterator it = subString.begin();
			std::string::const_iterator en = subString.end();

			while (it != en && !syntaxError)
			{
				const char c = *it;

				if ('(' == c || ')' == c || '@' == c || '&' == c || '|' == c)
				{
					syntaxError = true;
				}
				else if (' ' == c || '\t' == c || '\n' == c || '\r' == c)
				{
					if (1 == expressionState)
					{
						expressionState = 2;
					}
				}
				else if (',' == c)
				{
					expressionState = 0;
					outArgs.push_back(std::string());
				}
				else
				{
					if (2 == expressionState)
					{
						RENDERER_LOG(context, CRITICAL, "Renderer runtime shader builder: Syntax Error at line %lu: ',' or ')' expected\n", static_cast<unsigned long>(calculateLineCount(subString)))
						syntaxError = true;
					}
					else
					{
						outArgs.back().push_back(*it);
						expressionState = 1;
					}
				}

				++it;
			}

			if (syntaxError)
			{
				RENDERER_LOG(context, CRITICAL, "Renderer runtime shader builder: Syntax error at line %lu\n", static_cast<unsigned long>(calculateLineCount(subString)))
			}

			outSyntaxError = syntaxError;
		}

		void copy(std::string& outBuffer, const SubStringRef& inSubString, size_t length)
		{
			std::string::const_iterator itor = inSubString.begin();
			std::string::const_iterator end  = inSubString.begin() + static_cast<int>(length);

			while (itor != end)
			{
				outBuffer.push_back(*itor++);
			}
		}

		void repeat(std::string& outBuffer, const SubStringRef& inSubString, size_t length, size_t passNum, const std::string& counterVar)
		{
			std::string::const_iterator itor = inSubString.begin();
			std::string::const_iterator end  = inSubString.begin() + static_cast<int>(length);

			while (itor != end)
			{
				if ('@' == *itor && !counterVar.empty())
				{
					SubStringRef subString(&inSubString.getOriginalBuffer(), itor + 1);
					if (subString.find(counterVar) == 0)
					{
						{
							char temp[16];
							snprintf(temp, 16, "%lu", static_cast<unsigned long>(passNum));
							outBuffer += temp;
						}
						itor += static_cast<int>(counterVar.size()) + 1;
					}
					else
					{
						outBuffer.push_back(*itor++);
					}
				}
				else
				{
					outBuffer.push_back(*itor++);
				}
			}
		}


//[-------------------------------------------------------]
//[ Anonymous detail namespace                            ]
//[-------------------------------------------------------]
	} // detail
}


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
namespace RendererRuntime
{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	void ShaderBuilder::createSourceCode(const ShaderPieceResourceManager& shaderPieceResourceManager, const ShaderBlueprintResource& shaderBlueprintResource, const ShaderProperties& shaderProperties, BuildShader& buildShader)
	{
		mShaderProperties = shaderProperties;
		mShaderProperties.setPropertyValues(static_cast<const ShaderBlueprintResourceManager&>(shaderBlueprintResource.getResourceManager()).getRendererShaderProperties());
		mDynamicShaderPieces.clear();
		buildShader.assetIds.push_back(shaderBlueprintResource.getAssetId());
		const AssetManager& assetManager = shaderPieceResourceManager.getRendererRuntime().getAssetManager();
		uint64_t combinedAssetFileHashes = Math::calculateFNV1a64(reinterpret_cast<const uint8_t*>(&assetManager.getAssetByAssetId(shaderBlueprintResource.getAssetId()).fileHash), sizeof(uint64_t), Math::FNV1a_INITIAL_HASH_64);

		{ // Process the shader piece resources to include
			const ShaderBlueprintResource::IncludeShaderPieceResourceIds& includeShaderPieceResourceIds = shaderBlueprintResource.getIncludeShaderPieceResourceIds();
			const size_t numberOfShaderPieces = includeShaderPieceResourceIds.size();
			for (size_t i = 0; i < numberOfShaderPieces; ++i)
			{
				const ShaderPieceResource* shaderPieceResource = shaderPieceResourceManager.tryGetById(includeShaderPieceResourceIds[i]);
				if (nullptr != shaderPieceResource)
				{
					buildShader.assetIds.push_back(shaderPieceResource->getAssetId());
					combinedAssetFileHashes = Math::calculateFNV1a64(reinterpret_cast<const uint8_t*>(&assetManager.getAssetByAssetId(shaderPieceResource->getAssetId()).fileHash), sizeof(uint64_t), combinedAssetFileHashes);

					// Initialize
					mInString = shaderPieceResource->getShaderSourceCode();
					mOutString.clear();

					// Process
					parseMath(mInString, mOutString);
					parseForEach(mOutString, mInString);
					parseProperties(mInString, mOutString);
					collectPieces(mOutString, mInString);
					parseCounter(mInString, mOutString);
				}
				else
				{
					// TODO(co) Error handling
					assert(false);
				}
			}
		}

		{ // Process the shader blueprint resource
			// Initialize
			mInString = shaderBlueprintResource.getShaderSourceCode();
			mOutString.clear();

			// Process
			bool syntaxError = false;
			syntaxError |= parseMath(mInString, mOutString);
			syntaxError |= parseForEach(mOutString, mInString);
			syntaxError |= parseProperties(mInString, mOutString);
			while (!syntaxError && (mOutString.find("@piece") != std::string::npos || mOutString.find("@insertpiece") != std::string::npos))
			{
				syntaxError |= collectPieces(mOutString, mInString);
				syntaxError |= insertPieces(mInString, mOutString);
			}
			syntaxError |= parseCounter(mOutString, mInString);
		}

		// Apply a C-preprocessor
		Preprocessor::preprocess(shaderPieceResourceManager.getRendererRuntime(), mInString, mOutString);

		// Done
		buildShader.sourceCode = mOutString;
		buildShader.combinedAssetFileHashes = combinedAssetFileHashes;
	}


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	bool ShaderBuilder::parseMath(const std::string& inBuffer, std::string& outBuffer)
	{
		outBuffer.clear();
		outBuffer.reserve(inBuffer.size());

		::detail::StringVector argValues;
		::detail::SubStringRef subString(&inBuffer, 0);

		size_t pos = subString.find("@");
		size_t keyword = static_cast<size_t>(~0);

		while (std::string::npos != pos && static_cast<size_t>(~0) == keyword)
		{
			size_t maxSize = subString.findFirstOf(" \t(", pos + 1);
			maxSize = (std::string::npos == maxSize) ? subString.getSize() : maxSize;
			::detail::SubStringRef keywordStr(&inBuffer, subString.getStart() + pos + 1, subString.getStart() + maxSize);

			for (size_t i = 0; i < 8 && static_cast<size_t>(~0) == keyword; ++i)
			{
				if (keywordStr.matchEqual(::detail::c_operations[i].opName))
				{
					keyword = i;
				}
			}

			if (static_cast<size_t>(~0) == keyword)
			{
				pos = subString.find("@", pos + 1);
			}
		}

		bool syntaxError = false;

		while (std::string::npos != pos && !syntaxError)
		{
			// Copy what comes before the block
			copy(outBuffer, subString, pos);

			subString.setStart(subString.getStart() + pos + ::detail::c_operations[keyword].length);
			evaluateParamArgs(mContext, subString, argValues, syntaxError);

			syntaxError |= (argValues.size() < 2 || argValues.size() > 3);

			if (syntaxError)
			{
				const unsigned long lineCount = static_cast<unsigned long>(calculateLineCount(subString));
				if (keyword <= 1)
				{
					RENDERER_LOG(mContext, CRITICAL, "Renderer runtime shader builder: Syntax error at line %lu: @%s expects one parameter", lineCount, ::detail::c_operations[keyword].opName)
				}
				else
				{
					RENDERER_LOG(mContext, CRITICAL, "Renderer runtime shader builder: Syntax error at line %lu: @%s expects two or three parameters", lineCount, ::detail::c_operations[keyword].opName)
				}
			}
			else
			{
				StringId dstProperty = StringId(argValues[0].c_str());
				size_t idx  = 1;
				StringId srcProperty = dstProperty;
				if (argValues.size() == 3)
				{
					srcProperty = StringId(argValues[idx++].c_str());
				}
				int op1Value = 0;
				mShaderProperties.getPropertyValue(srcProperty, op1Value);
				char* endPtr = nullptr;
				int op2Value = strtol(argValues[idx].c_str(), &endPtr, 10);
				if (argValues[idx].c_str() == endPtr)
				{
					// Not a number, interpret as property
					mShaderProperties.getPropertyValue(StringId(argValues[idx].c_str()), op2Value);
				}

				const int result = ::detail::c_operations[keyword].opFunc(op1Value, op2Value);
				mShaderProperties.setPropertyValue(dstProperty, result);
			}

			pos = subString.find("@");
			keyword = static_cast<size_t>(~0);

			while (std::string::npos != pos && static_cast<size_t>(~0) == keyword)
			{
				size_t maxSize = subString.findFirstOf(" \t(", pos + 1);
				maxSize = (maxSize == std::string::npos) ? subString.getSize() : maxSize;
				::detail::SubStringRef keywordStr(&inBuffer, subString.getStart() + pos + 1, subString.getStart() + maxSize);

				for (size_t i = 0; i < 8 && static_cast<size_t>(~0) == keyword; ++i)
				{
					if (keywordStr.matchEqual(::detail::c_operations[i].opName))
					{
						keyword = i;
					}
				}

				if (static_cast<size_t>(~0) == keyword)
				{
					pos = subString.find("@", pos + 1);
				}
			}
		}

		copy(outBuffer, subString, subString.getSize());

		return syntaxError;
	}

	bool ShaderBuilder::parseForEach(const std::string& inBuffer, std::string& outBuffer) const
	{
		outBuffer.clear();
		outBuffer.reserve(inBuffer.size());

		::detail::StringVector argValues;
		::detail::SubStringRef subString(&inBuffer, 0);
		size_t pos = subString.find("@foreach");

		bool syntaxError = false;

		while (std::string::npos != pos && !syntaxError)
		{
			// Copy what comes before the block
			copy(outBuffer, subString, pos);

			subString.setStart(subString.getStart() + pos + sizeof("@foreach"));
			evaluateParamArgs(mContext, subString, argValues, syntaxError);

			::detail::SubStringRef blockSubString = subString;
			::detail::findBlockEnd(mContext, blockSubString, syntaxError);

			if (!syntaxError)
			{
				char* endPtr = nullptr;
				int count = strtol(argValues[0].c_str(), &endPtr, 10);
				if (argValues[0].c_str() == endPtr)
				{
					// This isn't a number. Let's try if it's a property. If it's no property default to 0 (property might have been optimized out).
					mShaderProperties.getPropertyValue(StringId(argValues[0].c_str()), count);
				}

				std::string counterVar;
				if (argValues.size() > 1)
				{
					counterVar = argValues[1];
				}

				int start = 0;
				if (argValues.size() > 2)
				{
					start = strtol(argValues[2].c_str(), &endPtr, 10);
					if (argValues[2].c_str() == endPtr)
					{
						// This isn't a number. Let's try if it's a property.
						if (!mShaderProperties.getPropertyValue(StringId(argValues[2].c_str()), start, -1))
						{
							RENDERER_LOG(mContext, CRITICAL, "Renderer runtime shader builder: Invalid parameter at line %lu (@foreach). '%s' is not a number nor a variable\n", static_cast<unsigned long>(calculateLineCount(blockSubString)), argValues[2].c_str())
							syntaxError = true;
							start = 0;
							count = 0;
						}
					}
				}

				for (int i = start; i < count; ++i)
				{
					::detail::repeat(outBuffer, blockSubString, blockSubString.getSize(), static_cast<size_t>(i), counterVar);
				}
			}

			subString.setStart(blockSubString.getEnd() + sizeof("@end"));
			pos = subString.find("@foreach");
		}

		copy(outBuffer, subString, subString.getSize());

		return syntaxError;
	}

	bool ShaderBuilder::parseProperties(std::string& inBuffer, std::string& outBuffer) const
	{
		outBuffer.clear();
		outBuffer.reserve(inBuffer.size());

		::detail::SubStringRef subString(&inBuffer, 0);
		size_t pos = subString.find("@property");

		bool syntaxError = false;

		while (std::string::npos != pos && !syntaxError)
		{
			// Copy what comes before the block
			copy(outBuffer, subString, pos);

			subString.setStart(subString.getStart() + pos + sizeof("@property"));
			const bool result = evaluateExpression(mContext, mShaderProperties, subString, syntaxError);

			::detail::SubStringRef blockSubString = subString;
			::detail::findBlockEnd(mContext, blockSubString, syntaxError);

			if (result && !syntaxError)
			{
				::detail::copy(outBuffer, blockSubString, blockSubString.getSize());
			}

			subString.setStart(blockSubString.getEnd() + sizeof("@end"));
			pos = subString.find("@property");
		}

		copy(outBuffer, subString, subString.getSize());

		while (!syntaxError && outBuffer.find("@property") != std::string::npos)
		{
			inBuffer.swap(outBuffer);
			syntaxError = parseProperties(inBuffer, outBuffer);
		}

		return syntaxError;
	}

	bool ShaderBuilder::collectPieces(const std::string& inBuffer, std::string& outBuffer)
	{
		outBuffer.clear();
		outBuffer.reserve(inBuffer.size());

		::detail::StringVector argValues;
		::detail::SubStringRef subString(&inBuffer, 0);
		size_t pos = subString.find("@piece");

		bool syntaxError = false;

		while (std::string::npos != pos && !syntaxError)
		{
			// Copy what comes before the block
			::detail::copy(outBuffer, subString, pos);

			subString.setStart(subString.getStart() + pos + sizeof("@piece"));
			::detail::evaluateParamArgs(mContext, subString, argValues, syntaxError);

			syntaxError |= (argValues.size() != 1);

			if (syntaxError)
			{
				RENDERER_LOG(mContext, CRITICAL, "Renderer runtime shader builder: Syntax error at line %lu: @piece expects one parameter", static_cast<unsigned long>(calculateLineCount(subString)))
			}
			else
			{
				const StringId pieceName(argValues[0].c_str());
				DynamicShaderPieces::const_iterator it = mDynamicShaderPieces.find(pieceName);
				if (it != mDynamicShaderPieces.end())
				{
					syntaxError = true;
					RENDERER_LOG(mContext, CRITICAL, "Renderer runtime shader builder: Error at line %lu: @piece '%s' already defined", static_cast<unsigned long>(calculateLineCount(subString)), argValues[0].c_str())
				}
				else
				{
					::detail::SubStringRef blockSubString = subString;
					::detail::findBlockEnd(mContext, blockSubString, syntaxError);

					std::string tempBuffer;
					::detail::copy(tempBuffer, blockSubString, blockSubString.getSize());
					mDynamicShaderPieces[pieceName] = tempBuffer;

					subString.setStart(blockSubString.getEnd() + sizeof("@end"));
				}
			}

			pos = subString.find("@piece");
		}

		::detail::copy(outBuffer, subString, subString.getSize());

		return syntaxError;
	}

	bool ShaderBuilder::insertPieces(std::string& inBuffer, std::string& outBuffer) const
	{
		outBuffer.clear();
		outBuffer.reserve(inBuffer.size());

		::detail::StringVector argValues;
		::detail::SubStringRef subString(&inBuffer, 0);
		size_t pos = subString.find("@insertpiece");

		bool syntaxError = false;

		while (std::string::npos != pos && !syntaxError)
		{
			// Copy what comes before the block
			::detail::copy(outBuffer, subString, pos);

			subString.setStart(subString.getStart() + pos + sizeof("@insertpiece"));
			::detail::evaluateParamArgs(mContext, subString, argValues, syntaxError);

			syntaxError |= (argValues.size() != 1);

			if (syntaxError)
			{
				RENDERER_LOG(mContext, CRITICAL, "Renderer runtime shader builder: Syntax error at line %lu: @insertpiece expects one parameter", static_cast<unsigned long>(calculateLineCount(subString)))
			}
			else
			{
				const StringId pieceName(argValues[0].c_str());
				DynamicShaderPieces::const_iterator it = mDynamicShaderPieces.find(pieceName);
				if (it != mDynamicShaderPieces.end())
				{
					outBuffer += it->second;
				}
			}

			pos = subString.find("@insertpiece");
		}

		::detail::copy(outBuffer, subString, subString.getSize());

		return syntaxError;
	}

	bool ShaderBuilder::parseCounter(const std::string& inBuffer, std::string& outBuffer)
	{
		outBuffer.clear();
		outBuffer.reserve(inBuffer.size());

		::detail::StringVector argValues;
		::detail::SubStringRef subString(&inBuffer, 0);

		size_t pos = subString.find("@");
		size_t keyword = static_cast<size_t>(~0);

		if (std::string::npos != pos)
		{
			size_t maxSize = subString.findFirstOf(" \t(", pos + 1);
			maxSize = (maxSize == std::string::npos) ? subString.getSize() : maxSize;
			::detail::SubStringRef keywordStr(&inBuffer, subString.getStart() + pos + 1, subString.getStart() + maxSize);

			for (size_t i = 0; i < 10 && static_cast<size_t>(~0) == keyword; ++i)
			{
				if (keywordStr.matchEqual(::detail::c_counterOperations[i].opName))
				{
					keyword = i;
				}
			}

			if (static_cast<size_t>(~0) == keyword)
			{
				pos = std::string::npos;
			}
		}

		bool syntaxError = false;

		while (std::string::npos != pos && !syntaxError)
		{
			// Copy what comes before the block
			::detail::copy(outBuffer, subString, pos);

			subString.setStart(subString.getStart() + pos + ::detail::c_counterOperations[keyword].length);
			evaluateParamArgs(mContext, subString, argValues, syntaxError);

			if (keyword <= 1)
			{
				syntaxError |= (argValues.size() != 1);
			}
			else
			{
				syntaxError |= (argValues.size() < 2 || argValues.size() > 3);
			}

			if (syntaxError)
			{
				const unsigned long lineCount = static_cast<unsigned long>(calculateLineCount(subString));
				if (keyword <= 1)
				{
					RENDERER_LOG(mContext, CRITICAL, "Renderer runtime shader builder: Syntax error at line %lu: @%s expects one parameter", lineCount, ::detail::c_counterOperations[keyword].opName)
				}
				else
				{
					RENDERER_LOG(mContext, CRITICAL, "Renderer runtime shader builder: Syntax error at line %lu: @%s expects two or three parameters", lineCount, ::detail::c_counterOperations[keyword].opName)
				}
			}
			else
			{
				StringId dstProperty;
				StringId srcProperty;
				int op1Value = 0;
				int op2Value = 0;

				if (argValues.size() == 1)
				{
					dstProperty = StringId(argValues[0].c_str());
					srcProperty = dstProperty;
					mShaderProperties.getPropertyValue(srcProperty, op1Value);
					op2Value = op1Value;

					{ // @value & @counter write, the others are invisible
						char temp[16];
						snprintf(temp, 16, "%i", op1Value);
						outBuffer += temp;
					}

					if (0 == keyword)
					{
						++op1Value;
						mShaderProperties.setPropertyValue(dstProperty, op1Value);
					}
				}
				else
				{
					dstProperty = StringId(argValues[0].c_str());
					size_t idx  = 1;
					srcProperty = dstProperty;
					if (argValues.size() == 3)
					{
						srcProperty = StringId(argValues[idx++].c_str());
					}
					mShaderProperties.getPropertyValue(srcProperty, op1Value);
					char* endPtr = nullptr;
					op2Value = strtol(argValues[idx].c_str(), &endPtr, 10);
					if (argValues[idx].c_str() == endPtr)
					{
						// Not a number, interpret as property
						mShaderProperties.getPropertyValue(StringId(argValues[idx].c_str()), op2Value);
					}

					const int result = ::detail::c_counterOperations[keyword].opFunc(op1Value, op2Value);
					mShaderProperties.setPropertyValue(dstProperty, result);
				}
			}

			pos = subString.find("@");
			keyword = static_cast<size_t>(~0);

			if (std::string::npos != pos)
			{
				size_t maxSize = subString.findFirstOf(" \t(", pos + 1);
				maxSize = (maxSize == std::string::npos) ? subString.getSize() : maxSize;
				::detail::SubStringRef keywordStr(&inBuffer, subString.getStart() + pos + 1, subString.getStart() + maxSize);

				for (size_t i = 0; i < 10 && static_cast<size_t>(~0) == keyword; ++i)
				{
					if (keywordStr.matchEqual(::detail::c_counterOperations[i].opName))
					{
						keyword = i;
					}
				}

				if (static_cast<size_t>(~0) == keyword)
				{
					pos = std::string::npos;
				}
			}
		}

		::detail::copy(outBuffer, subString, subString.getSize());

		return syntaxError;
	}

	bool ShaderBuilder::parse(const std::string& inBuffer, std::string& outBuffer) const
	{
		outBuffer.clear();
		outBuffer.reserve(inBuffer.size());

		return parseForEach(inBuffer, outBuffer);
	}


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
} // RendererRuntime
