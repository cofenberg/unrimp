/*********************************************************\
 * Copyright (c) 2018 The Unrimp Team
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


// Amalgamated/unity build: Single-header input library conversion of the PixelLight ( https://www.pixellight.org/ ) input system originally designed and developed by Stefan Buschmann ( https://www.stefanbuschmann.de/ )


// TODO(co) Code style related adjustments and other cosmetic polishing
// TODO(co) Review "PLINPUT_API", it should still be possible to optionally export symbols
// TODO(co) Slim down: Remove "utf8/utf8.h"-dependency, the current external dependency
// TODO(co) Get Linux port up-and-running (was working in PixelLight)
// TODO(co) Get Android port up-and-running (was working in PixelLight)
// TODO(co) Get MacOSX port up-and-running (wasn't working in PixelLight)
// TODO(co) Add log, assert and memory interfaces in order to give the host application the control over those things


//[-------------------------------------------------------]
//[ Header guard                                          ]
//[-------------------------------------------------------]
#pragma once


//[-------------------------------------------------------]
//[ C++ compiler keywords                                 ]
//[-------------------------------------------------------]
#ifdef _WIN32
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
	*    Platform specific "#pragma warning(push)" (Microsoft Windows Visual Studio)
	*/
	#define PRAGMA_WARNING_PUSH __pragma(warning(push))

	/**
	*  @brief
	*    Platform specific "#pragma warning(pop)" (Microsoft Windows Visual Studio)
	*/
	#define PRAGMA_WARNING_POP __pragma(warning(pop))

	/**
	*  @brief
	*    Platform specific "#pragma warning(disable: <x>)" (Microsoft Windows Visual Studio)
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
		*    Platform specific "#pragma warning(disable: <x>)" (Microsoft Windows Visual Studio)
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
		*    Platform specific "#pragma warning(disable: <x>)" (Microsoft Windows Visual Studio)
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
#else
	#error "Unsupported platform"
#endif


//[-------------------------------------------------------]
//[ Includes                                              ]
//[-------------------------------------------------------]
#ifdef _WIN32
	#include <intrin.h>	// For "__nop()"
#endif
// Disable warnings in external headers, we can't fix them
PRAGMA_WARNING_PUSH
	PRAGMA_WARNING_DISABLE_MSVC(4365)	// warning C4365: 'argument': conversion from 'long' to 'unsigned int', signed/unsigned mismatch
	PRAGMA_WARNING_DISABLE_MSVC(4571)	// warning C4571: Informational: catch(...) semantics changed since Visual C++ 7.1; structured exceptions (SEH) are no longer caught
	PRAGMA_WARNING_DISABLE_MSVC(4574)	// warning C4574: '_HAS_ITERATOR_DEBUGGING' is defined to be '0': did you mean to use '#if _HAS_ITERATOR_DEBUGGING'?
	PRAGMA_WARNING_DISABLE_MSVC(4625)	// warning C4625: 'std::_Hash<std::_Umap_traits<_Kty,_Ty,std::_Uhash_compare<_Kty,_Hasher,_Keyeq>,_Alloc,false>>': copy constructor was implicitly defined as deleted
	PRAGMA_WARNING_DISABLE_MSVC(4626)	// warning C4626: 'std::codecvt_base': assignment operator was implicitly defined as deleted
	PRAGMA_WARNING_DISABLE_MSVC(4774)	// warning C4774: 'sprintf_s' : format string expected in argument 3 is not a string literal
	PRAGMA_WARNING_DISABLE_MSVC(5026)	// warning C5026: 'std::_Generic_error_category': move constructor was implicitly defined as deleted
	PRAGMA_WARNING_DISABLE_MSVC(5027)	// warning C5027: 'std::_Generic_error_category': move assignment operator was implicitly defined as deleted
	#include <string>
	#include <vector>
	#include <unordered_map>
PRAGMA_WARNING_POP


//[-------------------------------------------------------]
//[ Project independent generic export                    ]
//[-------------------------------------------------------]
#ifdef _WIN32
	// To export classes, methods and variables
	#ifndef GENERIC_API_EXPORT
		#define GENERIC_API_EXPORT	__declspec(dllexport)
	#endif

	// To export functions
	#ifndef GENERIC_FUNCTION_EXPORT
		#define GENERIC_FUNCTION_EXPORT	extern "C" __declspec(dllexport)
	#endif
#elif LINUX
	// To export classes, methods and variables
	#ifndef GENERIC_API_EXPORT
		#if defined(HAVE_VISIBILITY_ATTR)
			#define GENERIC_API_EXPORT __attribute__ ((visibility("default")))
		#else
			#define GENERIC_API_EXPORT
		#endif
	#endif

	// To export functions
	#ifndef GENERIC_FUNCTION_EXPORT
		#if defined(HAVE_VISIBILITY_ATTR)
			#define GENERIC_FUNCTION_EXPORT extern "C" __attribute__ ((visibility("default")))
		#else
			#define GENERIC_FUNCTION_EXPORT extern "C"
		#endif
	#endif
#else
	#error "Unsupported platform"
#endif


//[-------------------------------------------------------]
//[ Project independent generic import                    ]
//[-------------------------------------------------------]
#ifdef _WIN32
	// To import classes, methods and variables
	#define GENERIC_API_IMPORT	__declspec(dllimport)
#elif LINUX
	// To import classes, methods and variables
	#define GENERIC_API_IMPORT
#else
	#error "Unsupported platform"
#endif


//[-------------------------------------------------------]
//[ Forward declarations                                  ]
//[-------------------------------------------------------]
#ifdef __ANDROID__
	namespace std
	{
		inline namespace __ndk1
		{
			class mutex;
		}
	}
#else
	namespace std
	{
		class mutex;
	}
#endif
namespace DeviceInput
{
	class Axis;
	class Mouse;
	class Device;
	class Button;
	class Control;
	class Keyboard;
	class Provider;
	class HIDDevice;
	class Controller;
	class DeviceImpl;
	class Connection;
	class InputManager;
	class ConnectionDevice;
}


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
namespace DeviceInput
{


	//[-------------------------------------------------------]
	//[ Global definitions                                    ]
	//[-------------------------------------------------------]
	// TODO(co) Review "PLINPUT_API", it should still be possible to optionally export symbols
	/*
	#ifdef PLINPUT_EXPORTS
		// To export classes, methods and variables
		#define PLINPUT_API GENERIC_API_EXPORT
	#else
		// To import classes, methods and variables
		#define PLINPUT_API GENERIC_API_IMPORT
	#endif
	*/
	#define PLINPUT_API

	/**
	*  @brief
	*    Controller type
	*/
	enum class ControllerType
	{
		DEVICE = 0,	///< Controller is a real input device, no input connections are allowed
		VIRTUAL		///< Controller is a virtual controller, input and output connections are allowed
	};

	/**
	*  @brief
	*    Control type
	*/
	enum class ControlType
	{
		UNKNOWN = 0,	///< Unknown control
		BUTTON,			///< Button or key
		AXIS,			///< Axis (can be absolute or relative)
		LED,			///< LED output
		EFFECT			///< Effect output
	};

	/**
	*  @brief
	*    Device backend type
	*/
	enum class DeviceBackendType
	{
		UNKNOWN = 0,		///< Unknown backend
		UPDATE_DEVICE,		///< Update device backend
		CONNECTION_DEVICE,	///< Connection device backend
		HID					///< HID device backend (which is also a connection device)
	};


	//[-------------------------------------------------------]
	//[ Classes                                               ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Input control base class
	*
	*  @remarks
	*    A control is part of an input controller, e.g. a button or an axis.
	*/
	class Control
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] controller
		*    Owning controller
		*  @param[in] controlType
		*    Control type
		*  @param[in] name
		*    UTF-8 control name
		*  @param[in] description
		*    UTF-8 control description
		*/
		PLINPUT_API Control(Controller& controller, ControlType controlType, const std::string& name, const std::string& description);

		/**
		*  @brief
		*    Destructor
		*/
		PLINPUT_API ~Control();

		/**
		*  @brief
		*    Get controller
		*
		*  @return
		*    Reference to controller that owns the control
		*/
		inline Controller& getController() const
		{
			return mController;
		}

		/**
		*  @brief
		*    Get control type
		*
		*  @return
		*    Type of control
		*/
		inline ControlType getControlType() const
		{
			return mControlType;
		}

		/**
		*  @brief
		*    Check if control is input or output control
		*
		*  @return
		*    "true" if control is input control, "false" if output
		*/
		inline bool isInputControl() const
		{
			// Input controls are:  Axis and button
			// Output controls are: Effect and LED
			return (ControlType::AXIS == mControlType || ControlType::BUTTON == mControlType);
		}

		/**
		*  @brief
		*    Get control name
		*
		*  @return
		*    UTF-8 control name
		*/
		inline const std::string& getName() const
		{
			return mName;
		}

		/**
		*  @brief
		*    Get control description
		*
		*  @return
		*    UTF-8 control description
		*/
		inline const std::string& getDescription() const
		{
			return mDescription;
		}


	//[-------------------------------------------------------]
	//[ Protected methods                                     ]
	//[-------------------------------------------------------]
	protected:
		/**
		*  @brief
		*    Inform input manager that control has been changed
		*/
		PLINPUT_API void informUpdate();

		Control& operator =(const Control& control) = delete;


	//[-------------------------------------------------------]
	//[ Protected data                                        ]
	//[-------------------------------------------------------]
	protected:
		Controller& mController;	///< Owning controller
		ControlType mControlType;	///< Control type
		std::string mName;			///< Control name
		std::string mDescription;	///< Control description


	};

	/**
	*  @brief
	*    Button control
	*/
	class Button final : public Control
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] controller
		*    Owning controller
		*  @param[in] name
		*    UTF-8 control name
		*  @param[in] description
		*    UTF-8 control description
		*  @param[in] character
		*    Character associated with the button, "\0" if none
		*/
		inline Button(Controller& controller, const std::string& name, const std::string& description, char character = '\0') :
			Control(controller, ControlType::BUTTON, name, description),
			mCharacter(character),
			mPressed(false),
			mHit(false)
		{
			// Nothing here
		}

		/**
		*  @brief
		*    Copy constructor
		*
		*  @param[in] button
		*    Other button
		*/
		inline explicit Button(const Button& button) :
			Control(button.mController, ControlType::BUTTON, button.mName, button.mDescription),
			mCharacter(button.mCharacter),
			mPressed(button.mPressed),
			mHit(button.mHit)
		{
			// Nothing here
		}

		/**
		*  @brief
		*    Destructor
		*/
		inline ~Button()
		{
			// Nothing here
		}

		/**
		*  @brief
		*    Comparison operator
		*
		*  @param[in] button
		*    Button to compare with
		*
		*  @return
		*    "true", if both buttons are equal, else "false"
		*/
		inline bool operator ==(const Button& button) const
		{
			return (mCharacter == button.mCharacter && mPressed == button.mPressed && mHit == button.mHit);
		}

		/**
		*  @brief
		*    Copy operator
		*
		*  @param[in] button
		*    Other button
		*
		*  @return
		*    Reference to this button
		*/
		inline Button& operator =(const Button& button)
		{
			// Copy value
			mCharacter = button.mCharacter;
			mPressed   = button.mPressed;
			mHit	   = button.mHit;

			// Control has changed
			informUpdate();

			// Return reference to this button
			return *this;
		}

		/**
		*  @brief
		*    Get character
		*
		*  @return
		*    Character associated with the button, "\0" if none
		*/
		inline char getCharacter() const
		{
			return mCharacter;
		}

		/**
		*  @brief
		*    Get button status
		*
		*  @return
		*    "true" if the button is currently pressed, else "false"
		*/
		inline bool isPressed() const
		{
			return mPressed;
		}

		/**
		*  @brief
		*    Set button status
		*
		*  @param[in] pressed
		*    "true" if the button is pressed, else "false"
		*/
		inline void setPressed(bool pressed)
		{
			// If the button was previously pressed but now isn't, we received a hit
			mHit = (mPressed && !pressed);

			// Set state
			mPressed = pressed;

			// Control has changed
			informUpdate();
		}

		/**
		*  @brief
		*    Check if the button has been hit without modifying the internal state
		*
		*  @return
		*    "true" if the button has been hit since the last call of this method, else 'false'
		*
		*  @note
		*    - This method will not reset the hit-state after being called (see "checkHitAndReset()")
		*/
		inline bool isHit() const
		{
			return mHit;
		}

		/**
		*  @brief
		*    Check if the button has been hit including modifying the internal state
		*
		*  @return
		*    "true" if the button has been hit since the last call of this method, else "false"
		*
		*  @remarks
		*    This will return the hit-state of the button and also reset it immediately (so the next call
		*    to "checkHitAndReset()" will return false). If you only want to check, but not reset the hit-state of
		*    a button, you should call "isHit()".
		*/
		inline bool checkHitAndReset()
		{
			const bool bHit = mHit;
			mHit = false;
			return bHit;
		}


	//[-------------------------------------------------------]
	//[ Private data                                          ]
	//[-------------------------------------------------------]
	private:
		// Button information
		char mCharacter;	///< Character associated with the button, "\0" if none

		// Button status
		bool mPressed;	///< Is the button currently pressed?
		bool mHit;		///< Has the button been hit in the meantime?


	};

	/**
	*  @brief
	*    Axis control
	*/
	class Axis final : public Control
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] controller
		*    Owning controller
		*  @param[in] name
		*    UTF-8 control name
		*  @param[in] description
		*    UTF-8 control description
		*/
		inline Axis(Controller& controller, const std::string& name, const std::string& description) :
			Control(controller, ControlType::AXIS, name, description),
			mValue(0.0f),
			mRelativeValue(false)
		{
			// Nothing here
		}

		/**
		*  @brief
		*    Copy constructor
		*
		*  @param[in] axis
		*    Other axis
		*/
		inline explicit Axis(const Axis& axis) :
			Control(axis.mController, ControlType::AXIS, axis.mName, axis.mDescription),
			mValue(axis.mValue),
			mRelativeValue(axis.mRelativeValue)
		{
			// Nothing here
		}

		/**
		*  @brief
		*    Destructor
		*/
		inline ~Axis()
		{
			// Nothing here
		}

		/**
		*  @brief
		*    Comparison operator
		*
		*  @param[in] axis
		*    Axis to compare with
		*
		*  @return
		*    "true" if both axes are equal, else "false"
		*/
		inline bool operator ==(const Axis& axis) const
		{
			return (mValue == axis.mValue && mRelativeValue == axis.mRelativeValue);
		}

		/**
		*  @brief
		*    Copy operator
		*
		*  @param[in] axis
		*    Other axis
		*
		*  @return
		*    Reference to this axis
		*/
		inline Axis& operator =(const Axis& axis)
		{
			// Copy value
			mValue		   = axis.mValue;
			mRelativeValue = axis.mRelativeValue;

			// Control has changed
			informUpdate();

			// Return reference to this axis
			return *this;
		}

		/**
		*  @brief
		*    Get axis value
		*
		*  @return
		*    Current value
		*
		*  @remarks
		*    Please note that a value can be absolute (for instance the x-axis of a joystick) or
		*    relative (for instance the x-axis of a mouse). While an absolute axis is usually timing
		*    independent, a relative axis just tells you about a state change since the last update.
		*    Therefore, we strongly recommend to always use "isRelativeValue()" to check for the value
		*    type in order to, for instance, multiply a absolute value with the current time difference
		*    since the last frame/update to get correctly timed movement.
		*/
		inline float getValue() const
		{
			return mValue;
		}

		/**
		*  @brief
		*    Set axis value
		*
		*  @param[in] value
		*    Current value
		*  @param[in] relativeValue
		*    "true" if the current value is relative, else "false" if it's a absolute value
		*/
		inline void setValue(float value, bool relativeValue)
		{
			// Set value
			mValue		   = value;
			mRelativeValue = relativeValue;

			// Control has changed
			informUpdate();
		}

		/**
		*  @brief
		*    Return whether the current value is relative or absolute
		*
		*  @return
		*    "true" if the current value is relative, else "false" if it's a absolute value
		*/
		inline bool isRelativeValue() const
		{
			return mRelativeValue;
		}


	//[-------------------------------------------------------]
	//[ Private data                                          ]
	//[-------------------------------------------------------]
	private:
		float mValue;			///< Value of the axis
		bool  mRelativeValue;	///< Is the current value a relative one?


	};

	/**
	*  @brief
	*    LED control
	*
	*  @note
	*    - An LED control can manage up to 32 LEDs
	*/
	class LED final : public Control
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] controller
		*    Owning controller
		*  @param[in] name
		*    UTF-8 control name
		*  @param[in] description
		*    UTF-8 control description
		*/
		inline LED(Controller& controller, const std::string& name, const std::string& description) :
			Control(controller, ControlType::LED, name, description),
			mLedStates(0)
		{
			// Nothing here
		}

		/**
		*  @brief
		*    Copy constructor
		*
		*  @param[in] led
		*    Other LED
		*/
		inline explicit LED(const LED& led) :
			Control(led.mController, ControlType::LED, led.mName, led.mDescription),
			mLedStates(led.mLedStates)
		{
			// Nothing here
		}

		/**
		*  @brief
		*    Destructor
		*/
		inline ~LED()
		{
			// Nothing here
		}

		/**
		*  @brief
		*    Comparison operator
		*
		*  @param[in] led
		*    LED to compare with
		*
		*  @return
		*    "true" if both LEDs are equal, else "false"
		*/
		inline bool operator ==(const LED& led) const
		{
			return (mLedStates == led.mLedStates);
		}

		/**
		*  @brief
		*    Copy operator
		*
		*  @param[in] led
		*    Other LED
		*
		*  @return
		*    Reference to this LED
		*/
		inline LED& operator =(const LED& led)
		{
			// Copy value
			mLedStates = led.mLedStates;

			// Control has changed
			informUpdate();

			// Return reference to this LED
			return *this;
		}

		/**
		*  @brief
		*    Get state of all LEDs as a bit field
		*
		*  @return
		*    LED states as a bit field
		*/
		inline uint32_t getLedStates() const
		{
			return mLedStates;
		}

		/**
		*  @brief
		*    Set state of all LEDs as a bit field
		*
		*  @param[in] ledStates
		*    LED states
		*/
		inline void setLedStates(uint32_t ledStates)
		{
			// Set state of LEDs
			mLedStates = ledStates;

			// Control has changed
			informUpdate();
		}

		/**
		*  @brief
		*    Get LED status
		*
		*  @param[in] ledIndex
		*    Index of LED (0..31)
		*
		*  @return
		*    "true" if the LED is currently on, else "false"
		*/
		inline bool isOn(int ledIndex) const
		{
			return (ledIndex >= 0 && ledIndex < 32) ? (((mLedStates >> ledIndex) & 1) != 0) : false;
		}

		/**
		*  @brief
		*    Set LED status
		*
		*  @param[in] ledIndex
		*    Index of LED (0..31)
		*  @param[in] on
		*    "true" if the LED is on, else "false"
		*/
		inline void setOn(int ledIndex, bool on)
		{
			// Check index
			if (ledIndex >= 0 && ledIndex < 32)
			{
				// Set LED state
				const uint32_t mask = (static_cast<uint32_t>(1) << ledIndex);
				if (on)
				{
					mLedStates |= mask;
				}
				else
				{
					mLedStates &= mask;
				}

				// Control has changed
				informUpdate();
			}
		}


	//[-------------------------------------------------------]
	//[ Private data                                          ]
	//[-------------------------------------------------------]
	private:
		uint32_t mLedStates;	///< State of all LEDs


	};

	/**
	*  @brief
	*    Effect control
	*
	*  @note
	*    - Effects are output controls, such as rumble, force-feedback effects etc.
	*/
	class Effect final : public Control
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] controller
		*    Owning controller
		*  @param[in] name
		*    UTF-8 control name
		*  @param[in] description
		*    UTF-8 control description
		*/
		inline Effect(Controller& controller, const std::string& name, const std::string& description) :
			Control(controller, ControlType::EFFECT, name, description),
			mValue(0.0f)
		{
			// Nothing here
		}

		/**
		*  @brief
		*    Copy constructor
		*
		*  @param[in] effect
		*    Other effect
		*/
		inline explicit Effect(const Effect& effect) :
			Control(effect.mController, ControlType::EFFECT, effect.mName, effect.mDescription),
			mValue(effect.mValue)
		{
			// Nothing here
		}

		/**
		*  @brief
		*    Destructor
		*/
		inline ~Effect()
		{
			// Nothing here
		}

		/**
		*  @brief
		*    Comparison operator
		*
		*  @param[in] effect
		*    Effect to compare with
		*
		*  @return
		*    "true" if both effects are equal, else "false"
		*/
		inline bool operator ==(const Effect& effect) const
		{
			return (mValue == effect.mValue);
		}

		/**
		*  @brief
		*    Copy operator
		*
		*  @param[in] effect
		*    Other effect
		*
		*  @return
		*    Reference to this effect
		*/
		inline Effect& operator =(const Effect& effect)
		{
			// Copy value
			mValue = effect.mValue;

			// Control has changed
			informUpdate();

			// Return reference to this effect
			return *this;
		}

		/**
		*  @brief
		*    Get effect value
		*
		*  @return
		*    Current value; usually, an effect value should be in the range of 0..1 (but it's up to the actual device definition)
		*/
		inline float getValue() const
		{
			return mValue;
		}

		/**
		*  @brief
		*    Set effect value
		*
		*  @param[in] value
		*    Current value; usually, an effect value should be in the range of 0..1 (but it's up to the actual device definition)
		*/
		inline void setValue(float value)
		{
			// Set value
			mValue = value;

			// Control has changed
			informUpdate();
		}


	//[-------------------------------------------------------]
	//[ Private data                                          ]
	//[-------------------------------------------------------]
	private:
		float mValue;	///< Value of the effect


	};

	/**
	*  @brief
	*    Connection between two controllers/controls
	*/
	class Connection final
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] inputControl
		*    Input control
		*  @param[in] outputControl
		*    Output control
		*  @param[in] scale
		*    Scale factor
		*/
		PLINPUT_API Connection(Control& inputControl, Control& outputControl, float scale = 1.0f);

		/**
		*  @brief
		*    Destructor
		*/
		inline ~Connection()
		{
			// Nothing here
		}

		/**
		*  @brief
		*    Get input control
		*
		*  @return
		*    Reference to control that is on the input side of the connection
		*/
		inline Control& getInputControl() const
		{
			return mInputControl;
		}

		/**
		*  @brief
		*    Get output control
		*
		*  @return
		*    Reference to control that is on the output side of the connection
		*/
		inline Control& getOutputControl() const
		{
			return mOutputControl;
		}

		/**
		*  @brief
		*    Check if connection is value
		*
		*  @return
		*    "true" if connection is valid, else "false"
		*
		*  @remarks
		*    A connection is invalid e.g. when you try to connect different types of controls
		*    without using the proper connection type (see derived classes for connection classes
		*    that can convert values into other types).
		*    It is also not valid to use a control of a device as an output, because devices can only be
		*    used as input, not as output of controls (a device is controlled by the physical device only).
		*/
		inline bool isValid() const
		{
			return mValid;
		}

		/**
		*  @brief
		*    Pass value from input to output
		*/
		PLINPUT_API void passValue();

		/**
		*  @brief
		*    Pass value backwards from output to input
		*/
		PLINPUT_API void passValueBackwards();

		Connection& operator =(const Connection& control) = delete;


	//[-------------------------------------------------------]
	//[ Private data                                          ]
	//[-------------------------------------------------------]
	private:
		Control& mInputControl;		///< Input control
		Control& mOutputControl;	///< Output control
		bool	 mValid;			///< "true" if connection is valid, else "false"
		float	 mScale;			///< Scale factor


	};




	// TODO(co) Below this: Cleanup
	/**
	*  @brief
	*    Input controller
	*
	*  @remarks
	*    A controller represents an input device, which can either be a real device like e.g. a mouse or joystick,
	*    or a virtual device that is used to map real input devices to virtual axes and keys. A controller consists
	*    of a list of controls, e.g. buttons or axes and provides methods to obtain the status.
	*/
	class Controller
	{


	//[-------------------------------------------------------]
	//[ Friends                                               ]
	//[-------------------------------------------------------]
	friend class InputManager;
	friend class Provider;
	friend class Control;


	//[-------------------------------------------------------]
	//[ Public definitions                                    ]
	//[-------------------------------------------------------]
	public:
		typedef std::vector<Control*> Controls;
		typedef std::vector<Connection*> Connections;


	//[-------------------------------------------------------]
	//[ Public functions                                      ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] inputManager
		*    Owner input manager
		*  @param[in] controllerType
		*    Controller type
		*  @param[in] sName
		*    Controller name
		*  @param[in] sDescription
		*    Controller description
		*/
		inline Controller(InputManager& inputManager, ControllerType controllerType, const std::string &sName, const std::string &sDescription) :
			mInputManager(inputManager),
			mControllerType(controllerType),
			m_sName(sName),
			m_sDescription(sDescription),
			m_bConfirmed(false),
			m_bActive(true),
			m_bChanged(false),
			m_nChar(0)
		{
			// Nothing here
		}

		/**
		*  @brief
		*    Destructor
		*/
		inline virtual ~Controller()
		{
			// Destroy all connections
			while (!m_lstConnections.empty())
			{
				Disconnect(m_lstConnections[0]);
			}
		}

		/**
		*  @brief
		*    Get owner input manager
		*
		*  @return
		*    Owner input manager
		*/
		inline InputManager& GetInputManager() const
		{
			return mInputManager;
		}

		/**
		*  @brief
		*    Get controller type
		*
		*  @return
		*    Controller type
		*/
		inline ControllerType getControllerType() const
		{
			return mControllerType;
		}

		/**
		*  @brief
		*    Get controller name
		*
		*  @return
		*    Name
		*/
		inline const std::string& GetName() const
		{
			return m_sName;
		}

		/**
		*  @brief
		*    Get controller description
		*
		*  @return
		*    Description
		*/
		inline const std::string& GetDescription() const
		{
			return m_sDescription;
		}

		/**
		*  @brief
		*    Check if controller is active
		*
		*  @return
		*    'true' if controller is active, else 'false'
		*
		*  @remarks
		*    If a controller is active, it sends out signals when the state of it's controls has changed.
		*    If a controller is not active, no state changes will occur and all input events from connected
		*    devices will be discarded.
		*/
		inline bool GetActive() const
		{
			return m_bActive;
		}

		/**
		*  @brief
		*    Activate or deactivate controller
		*
		*  @param[in] bActive
		*    'true' if controller is active, else 'false'
		*
		*  @remarks
		*    Virtual controllers can be activated or deactivated, real input devices
		*    are always active and can not be deactivated.
		*/
		inline void SetActive(bool bActive)
		{
			// Only virtual controller can be activated/deactivated
			if (ControllerType::VIRTUAL == mControllerType)
			{
				// Set active-state
				m_bActive = bActive;
			}
		}

		/**
		*  @brief
		*    Check if the controller's state has changed (for polling)
		*
		*  @return
		*    'true', if the state has changed, else 'false'
		*/
		inline bool HasChanged() const
		{
			// Get state
			const bool bChanged = m_bChanged;

			// Reset state
			m_bChanged = false;

			// Return state
			return bChanged;
		}

		/**
		*  @brief
		*    Get all controls of the controller
		*
		*  @return
		*    List of controls
		*/
		inline const Controls &GetControls() const
		{
			return m_lstControls;
		}

		/**
		*  @brief
		*    Get all buttons
		*
		*  @return
		*    List of buttons
		*/
		inline const std::vector<Button*> &GetButtons() const
		{
			// Initialize button list
			if (m_lstButtons.empty())
				InitControlList(ControlType::BUTTON);

			// Return button list
			return m_lstButtons;
		}

		/**
		*  @brief
		*    Get all axes
		*
		*  @return
		*    List of axes
		*/
		inline const std::vector<Axis*> &GetAxes() const
		{
			// Initialize axes list
			if (m_lstAxes.empty())
			{
				InitControlList(ControlType::AXIS);
			}

			// Return axes list
			return m_lstAxes;
		}

		/**
		*  @brief
		*    Get control with a specific name
		*
		*  @param[in] sName
		*    Name of control
		*
		*  @return
		*    Control, or a null pointer if no control with that name could be found
		*/
		inline Control *GetControl(const std::string &sName) const
		{
			ControlMap::const_iterator iterator = m_mapControls.find(sName);
			return (m_mapControls.cend() != iterator) ? iterator->second : nullptr;
		}

		/**
		*  @brief
		*    Get character of last button that was hit
		*
		*  @return
		*    Button character (ASCII), can be '\0'
		*
		*  @remarks
		*    This function returns the character code of the last button that was hit (not pressed!).
		*    The character will then be reset to '\0', so the next call will return '\0', until
		*    a new button is first pressed and then released.
		*/
		inline char GetChar()
		{
			// Get character
			const char nChar = m_nChar;
			m_nChar = 0;

			// Return character
			return nChar;
		}

		/**
		*  @brief
		*    Get connections
		*
		*  @return
		*    List of connections (both incoming and outgoing), do not destroy the returned connection instances!
		*
		*  @remarks
		*    To determine whether a connection is incoming or outgoing, you can check e.g.
		*    "GetOutputControl()->getController()" == this or something similar.
		*/
		inline const Connections &GetConnections()
		{
			return m_lstConnections;
		}

		/**
		*  @brief
		*    Connect to another controller
		*
		*  @param[in] outputControlName
		*    Name of control of this controller (output control)
		*  @param[in] inputControl
		*    Pointer to control (input control)
		*  @param[in] fScale
		*    Scale factor
		*/
		PLINPUT_API void Connect(const std::string& outputControlName, Control& inputControl, float fScale = 1.0f);

		/**
		*  @brief
		*    Connect to another controller
		*
		*  @param[in] pController
		*    Pointer to controller containing the input controls, shouldn't be a null pointer (but a null pointer is caught internally)
		*  @param[in] sPrefixOut
		*    Prefix for controls of this controller
		*  @param[in] sPrefixIn
		*    Prefix for controls of the other controller
		*
		*  @remarks
		*    This connects all controls of the input controller (pController) to the controls of the output
		*    controller (this), if their names are equal, e.g. pController->"Left" will be connected to this->"Left".
		*    You can also provide a prefix for either or both sides, e.g.: ConnectAll(pOtherController, "", "Camera")
		*    will connect pController->"CameraLeft" to this->"Left".
		*/
		PLINPUT_API void ConnectAll(Controller *pController, const std::string &sPrefixOut, const std::string &sPrefixIn);

		/**
		*  @brief
		*    Disconnect connection
		*
		*  @param[in] pConnection
		*    Connection (must be valid!), on successful disconnect, the given "pConnection" instance becomes invalid
		*/
		PLINPUT_API void Disconnect(Connection *pConnection);


	//[-------------------------------------------------------]
	//[ Public virtual Controller functions                   ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Update device once per frame
		*
		*  @remarks
		*    This function can be used e.g. to reset any data of a device once per frame. Usually this is
		*    not needed, but some devices (e.g. RawInput mice etc.) need to reset their data once per frame.
		*
		*  @note
		*    - The default implementation is empty
		*/
		inline virtual void Update()
		{
			// Do nothing by default
		}

		/**
		*  @brief
		*    Update output controls (LEDs, effects etc.)
		*
		*  @param[in] pControl
		*    Output control that has been changed, must be valid!
		*
		*  @remarks
		*    This function is called whenever an output control such as LED or Effect has been changed.
		*    A device should use this function to update the specific control state on the device (or update
		*    all output controls at the same time)
		*
		*  @note
		*    - The default implementation is empty
		*/
		inline virtual void UpdateOutputControl(Control*)
		{
			// Do nothing by default
		}


	//[-------------------------------------------------------]
	//[ Protected functions                                   ]
	//[-------------------------------------------------------]
	protected:
		Controller &operator =(const Controller &cSource) = delete;

		/**
		*  @brief
		*    Add control
		*
		*  @param[in] pControl
		*    Pointer to control, must be valid!
		*/
		PLINPUT_API void AddControl(Control *pControl);

		/**
		*  @brief
		*    Inform controller that a control has changed it's state
		*
		*  @param[in] pControl
		*    Pointer to control, must be valid!
		*/
		PLINPUT_API void InformControl(Control *pControl);

		/**
		*  @brief
		*    Initialize control list
		*
		*  @param[in] controlType
		*    Control type of list that is to be filled
		*/
		PLINPUT_API void InitControlList(ControlType controlType) const;

		/**
		*  @brief
		*    Add connection
		*
		*  @param[in] pConnection
		*    Connection (must be valid!)
		*/
		PLINPUT_API void AddConnection(Connection *pConnection);

		/**
		*  @brief
		*    Remove connection
		*
		*  @param[in] pConnection
		*    Connection (must be valid!)
		*/
		PLINPUT_API void RemoveConnection(Connection *pConnection);


	//[-------------------------------------------------------]
	//[ Protected definitions                                 ]
	//[-------------------------------------------------------]
	protected:
		typedef std::unordered_map<std::string, Control*> ControlMap;


	//[-------------------------------------------------------]
	//[ Protected data                                        ]
	//[-------------------------------------------------------]
	protected:
		// Controller information and state
		InputManager&   mInputManager;		///< Owner input manager
		ControllerType	mControllerType;	///< Controller type
		std::string		m_sName;			///< Controller name
		std::string		m_sDescription;		///< Controller description
		bool			m_bConfirmed;		///< Confirmation flag for "DetectDevices()"
		bool			m_bActive;			///< Is the controller active?
		mutable bool	m_bChanged;			///< Has the controller's state changed?

		// Controls
		Controls					 m_lstControls;	///< List of all controls
		ControlMap					 m_mapControls;	///< Hash map of name -> control
		mutable std::vector<Button*> m_lstButtons;	///< List of buttons (filled on use)
		mutable std::vector<Axis*>	 m_lstAxes;		///< List of absolute axes (filled on use)
		char						 m_nChar;		///< Last hit key character

		// Connections
		Connections m_lstConnections;	///< List of connections


	};

	/**
	*  @brief
	*    Input device
	*
	*  @note
	*    - A device is a controller that represents a real input device rather than a virtual controller
	*/
	class Device : public Controller
	{


	//[-------------------------------------------------------]
	//[ Friends                                               ]
	//[-------------------------------------------------------]
	friend class DeviceImpl;


	//[-------------------------------------------------------]
	//[ Public functions                                      ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] inputManager
		*    Owner input manager
		*  @param[in] sName
		*    Controller name
		*  @param[in] sDescription
		*    Controller description
		*  @param[in] pImpl
		*    System specific device implementation, can, but shouldn't be a null pointer
		*/
		PLINPUT_API Device(InputManager& inputManager, const std::string &sName, const std::string &sDescription, DeviceImpl *pImpl);

		/**
		*  @brief
		*    Destructor
		*/
		PLINPUT_API virtual ~Device() override;

		Device &operator =(const Device &cSource) = delete;

		/**
		*  @brief
		*    Get device implementation
		*
		*  @return
		*    System specific device implementation, can be a null pointer
		*/
		inline DeviceImpl *GetImpl() const
		{
			return m_pImpl;
		}


	//[-------------------------------------------------------]
	//[ Protected data                                        ]
	//[-------------------------------------------------------]
	protected:
		DeviceImpl *m_pImpl;		///< System specific device implementation, can be a null pointer
		bool		m_bDeleteImpl;	///< Destroy device implementation automatically?


	};

	/**
	*  @brief
	*    Keyboard device
	*
	*  @remarks
	*    This class supports the following device backend types:
	*    - UpdateDevice
	*/
	class Keyboard final : public Device
	{


	//[-------------------------------------------------------]
	//[ Controller definition                                 ]
	//[-------------------------------------------------------]
	public:
		Button	Backspace;			///< Backspace
		Button	Tab;				///< Tab
		Button	Clear;				///< Clear (not available everywhere)
		Button	Return;				///< Return (often the same as "Enter")
		Button	Shift;				///< Shift
		Button	Control;			///< Control ("Ctrl")
		Button	Alt;				///< Alt
		Button	Pause;				///< Pause
		Button	CapsLock;			///< Caps lock
		Button	Escape;				///< Escape
		Button	Space;				///< Space
		Button	PageUp;				///< Page up
		Button	PageDown;			///< Page down
		Button	End;				///< End
		Button	Home;				///< Home
		Button	Left;				///< Left arrow
		Button	Up;					///< Up arrow
		Button	Right;				///< Right arrow
		Button	Down;				///< Down arrow
		Button	Select;				///< Select (not available everywhere)
		Button	Execute;			///< Execute (not available everywhere)
		Button	Print;				///< Print screen
		Button	Insert;				///< Insert
		Button	Delete;				///< Delete
		Button	Help;				///< Help (not available everywhere)
		Button	Key0;				///< 0 (control name is "0")
		Button	Key1;				///< 1 (control name is "1")
		Button	Key2;				///< 2 (control name is "2")
		Button	Key3;				///< 3 (control name is "3")
		Button	Key4;				///< 4 (control name is "4")
		Button	Key5;				///< 5 (control name is "5")
		Button	Key6;				///< 6 (control name is "6")
		Button	Key7;				///< 7 (control name is "7")
		Button	Key8;				///< 8 (control name is "8")
		Button	Key9;				///< 9 (control name is "9")
		Button	A;					///< A
		Button	B;					///< B
		Button	C;					///< C
		Button	D;					///< D
		Button	E;					///< E
		Button	F;					///< F
		Button	G;					///< G
		Button	H;					///< H
		Button	I;					///< I
		Button	J;					///< J
		Button	K;					///< K
		Button	L;					///< L
		Button	M;					///< M
		Button	N;					///< N
		Button	O;					///< O
		Button	P;					///< P
		Button	Q;					///< Q
		Button	R;					///< R
		Button	S;					///< S
		Button	T;					///< T
		Button	U;					///< U
		Button	V;					///< V
		Button	W;					///< W
		Button	X;					///< X
		Button	Y;					///< Y
		Button	Z;					///< Z
		Button	Numpad0;			///< Numeric keypad 0
		Button	Numpad1;			///< Numeric keypad 1
		Button	Numpad2;			///< Numeric keypad 2
		Button	Numpad3;			///< Numeric keypad 3
		Button	Numpad4;			///< Numeric keypad 4
		Button	Numpad5;			///< Numeric keypad 5
		Button	Numpad6;			///< Numeric keypad 6
		Button	Numpad7;			///< Numeric keypad 7
		Button	Numpad8;			///< Numeric keypad 8
		Button	Numpad9;			///< Numeric keypad 9
		Button	NumpadMultiply;		///< Numeric keypad "Multiply"
		Button	NumpadAdd;			///< Numeric keypad "Add"
		Button	NumpadSeparator;	///< Numeric keypad "Separator"
		Button	NumpadSubtract;		///< Numeric keypad "Subtract"
		Button	NumpadDecimal;		///< Numeric keypad "Decimal"
		Button	NumpadDivide;		///< Numeric keypad "Divide"
		Button	F1;					///< F1
		Button	F2;					///< F2
		Button	F3;					///< F3
		Button	F4;					///< F4
		Button	F5;					///< F5
		Button	F6;					///< F6
		Button	F7;					///< F7
		Button	F8;					///< F8
		Button	F9;					///< F9
		Button	F10;				///< F10
		Button	F11;				///< F11
		Button	F12;				///< F12
		Button	NumLock;			///< Num lock
		Button	ScrollLock;			///< Scroll lock
		Button	Circumflex;			///< Circumflex (^)
		Button	LeftWindows;		///< Left Windows key (natural keyboard)
		Button	RightWindows;		///< Right Windows key (natural keyboard)
		Button	Applications;		///< Applications key (natural keyboard)
		Button	F13;				///< F13 key
		Button	F14;				///< F14 key
		Button	F15;				///< F15 key
		Button	F16;				///< F16 key
		Button	F17;				///< F17 key
		Button	F18;				///< F18 key
		Button	F19;				///< F19 key
		Button	F20;				///< F20 key
		Button	F21;				///< F21 key
		Button	F22;				///< F22 key
		Button	F23;				///< F23 key
		Button	F24;				///< F24 key
		Button	LeftShift;			///< Left shift key
		Button	RightShift;			///< Right shift key
		Button	LeftControl;		///< Left control key
		Button	RightControl;		///< Right control key
		Button	VolumeMute;			///< Volume mute key
		Button	VolumeDown;			///< Volume down key
		Button	VolumeUp;			///< Volume up key
		Button	MediaNextTrack;		///< Next track key
		Button	MediaPreviousTrack;	///< Previous track key
		Button	MediaStop;			///< Stop media key
		Button	MediaPlayPause;		///< Play/pause media key
		Button	Add;				///< For any country/region, the '+' key
		Button	Separator;			///< For any country/region, the ',' key
		Button	Subtract;			///< For any country/region, the '-' key
		Button	Decimal;			///< For any country/region, the '.' key
		Button	OEM1;				///< Used for miscellaneous characters; it can vary by keyboard. For the US standard keyboard, the ';:' key
		Button	OEM2;				///< Used for miscellaneous characters; it can vary by keyboard. For the US standard keyboard, the '/?' key
		Button	OEM3;				///< Used for miscellaneous characters; it can vary by keyboard. For the US standard keyboard, the '`~' key
		Button	OEM4;				///< Used for miscellaneous characters; it can vary by keyboard. For the US standard keyboard, the '[{' key
		Button	OEM5;				///< Used for miscellaneous characters; it can vary by keyboard. For the US standard keyboard, the '\|' key
		Button	OEM6;				///< Used for miscellaneous characters; it can vary by keyboard. For the US standard keyboard, the ']}' key
		Button	OEM7;				///< Used for miscellaneous characters; it can vary by keyboard. For the US standard keyboard, the 'single-quote/double-quote' key
		Button	OEM8;				///< Used for miscellaneous characters; it can vary by keyboard
		Button	OEM102;				///< Either the angle bracket key or the backslash key on the RT 102-key keyboard


	//[-------------------------------------------------------]
	//[ Public functions                                      ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] inputManager
		*    Owner input manager
		*  @param[in] sName
		*    Device name
		*  @param[in] pImpl
		*    System specific device implementation, can, but shouldn't be a null pointer
		*/
		PLINPUT_API Keyboard(InputManager& inputManager, const std::string &sName, DeviceImpl *pImpl);

		/**
		*  @brief
		*    Destructor
		*/
		inline virtual ~Keyboard() override
		{
			// Nothing here
		}

		/**
		*  @brief
		*    Copy operator
		*
		*  @param[in] cSource
		*    Source to copy from
		*
		*  @return
		*    This instance
		*/
		Keyboard &operator =(const Keyboard &cSource) = delete;


	//[-------------------------------------------------------]
	//[ Public virtual Controller functions                   ]
	//[-------------------------------------------------------]
	public:
		PLINPUT_API virtual void Update() override;


	};

	/**
	*  @brief
	*    Mouse input device
	*
	*  @remarks
	*    This class supports the following device backend types:
	*    - UpdateDevice
	*/
	class Mouse final : public Device
	{


	//[-------------------------------------------------------]
	//[ Controller definition                                 ]
	//[-------------------------------------------------------]
	public:
		Axis	X;			///< X axis (movement data, no absolute data)
		Axis	Y;			///< Y axis (movement data, no absolute data)
		Axis	Wheel;		///< Mouse wheel (movement data, no absolute data)
		Button	Left;		///< Left mouse button (mouse button #0)
		Button	Right;		///< Right mouse button (mouse button #1)
		Button	Middle;		///< Middle mouse button (mouse button #2)
		Button	Button4;	///< Mouse button #4
		Button	Button5;	///< Mouse button #5
		Button	Button6;	///< Mouse button #6
		Button	Button7;	///< Mouse button #7
		Button	Button8;	///< Mouse button #8
		Button	Button9;	///< Mouse button #9
		Button	Button10;	///< Mouse button #10
		Button	Button11;	///< Mouse button #11
		Button	Button12;	///< Mouse button #12


	//[-------------------------------------------------------]
	//[ Public functions                                      ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] inputManager
		*    Owner input manager
		*  @param[in] sName
		*    Device name
		*  @param[in] pImpl
		*    System specific device implementation, can, but shouldn't be a null pointer
		*/
		PLINPUT_API Mouse(InputManager& inputManager, const std::string &sName, DeviceImpl *pImpl);

		/**
		*  @brief
		*    Destructor
		*/
		inline virtual ~Mouse() override
		{
			// Inline
		}

		Mouse &operator =(const Mouse &cSource) = delete;


	//[-------------------------------------------------------]
	//[ Public virtual Controller functions                   ]
	//[-------------------------------------------------------]
	public:
		PLINPUT_API virtual void Update() override;


	};

	/**
	*  @brief
	*    Joystick input device
	*
	*  @remarks
	*    This class supports the following device backend types:
	*    - UpdateDevice
	*    - HIDDevice
	*/
	class Joystick final : public Device
	{


	//[-------------------------------------------------------]
	//[ Controller definition                                 ]
	//[-------------------------------------------------------]
	public:
		// Input
		Axis	X;			///< X axis
		Axis	Y;			///< Y axis
		Axis	Z;			///< Z axis
		Axis	RX;			///< Rotation X axis
		Axis	RY;			///< Rotation Y axis
		Axis	RZ;			///< Rotation Z axis
		Axis	Hat;		///< Hat axis
		Button	Button0;	///< Button #0
		Button	Button1;	///< Button #1
		Button	Button2;	///< Button #2
		Button	Button3;	///< Button #3
		Button	Button4;	///< Button #4
		Button	Button5;	///< Button #5
		Button	Button6;	///< Button #6
		Button	Button7;	///< Button #7
		Button	Button8;	///< Button #8
		Button	Button9;	///< Button #9
		Button	Button10;	///< Button #10
		Button	Button11;	///< Button #11
		Button	Button12;	///< Button #12
		Button	Button13;	///< Button #13
		Button	Button14;	///< Button #14
		Button	Button15;	///< Button #15
		Button	Button16;	///< Button #16
		Button	Button17;	///< Button #17
		Button	Button18;	///< Button #18
		Button	Button19;	///< Button #19
		Button	Button20;	///< Button #20
		Button	Button21;	///< Button #21
		Button	Button22;	///< Button #22
		Button	Button23;	///< Button #23
		Button	Button24;	///< Button #24
		Button	Button25;	///< Button #25
		Button	Button26;	///< Button #26
		Button	Button27;	///< Button #27
		Button	Button28;	///< Button #28
		Button	Button29;	///< Button #29
		Button	Button30;	///< Button #30
		Button	Button31;	///< Button #31

		// Effects
		Effect	Rumble1;	///< Rumble effect (motor #1)
		Effect	Rumble2;	///< Rumble effect (motor #2)
		Effect	Rumble3;	///< Rumble effect (motor #3)
		Effect	Rumble4;	///< Rumble effect (motor #4)


	//[-------------------------------------------------------]
	//[ Public functions                                      ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] inputManager
		*    Owner input manager
		*  @param[in] sName
		*    Device name
		*  @param[in] pImpl
		*    System specific device implementation, can, but shouldn't be a null pointer
		*/
		PLINPUT_API Joystick(InputManager& inputManager, const std::string &sName, DeviceImpl *pImpl);

		/**
		*  @brief
		*    Destructor
		*/
		PLINPUT_API virtual ~Joystick() override;

		/**
		*  @brief
		*    Get threshold
		*
		*  @return
		*    Threshold
		*/
		inline int GetThreshold() const
		{
			return m_nThreshold;
		}

		/**
		*  @brief
		*    Set threshold
		*
		*  @param[in] nThreshold
		*    Threshold
		*/
		inline void SetThreshold(int nThreshold = 12000)
		{
			m_nThreshold = nThreshold;
		}


	//[-------------------------------------------------------]
	//[ Public virtual Controller functions                   ]
	//[-------------------------------------------------------]
	public:
		PLINPUT_API virtual void Update() override;
		PLINPUT_API virtual void UpdateOutputControl(Control *pControl) override;


	//[-------------------------------------------------------]
	//[ Private functions                                     ]
	//[-------------------------------------------------------]
	private:
		/**
		*  @brief
		*    Copy operator
		*
		*  @param[in] cSource
		*    Source to copy from
		*
		*  @return
		*    This instance
		*/
		Joystick &operator =(const Joystick &cSource) = delete;

		/**
		*  @brief
		*    Called when the HID device has read some data
		*/
		void OnDeviceRead();


	//[-------------------------------------------------------]
	//[ Private data                                          ]
	//[-------------------------------------------------------]
	private:
		// HID connection
		HIDDevice *m_pHIDDevice;	///< HID device, can be a null pointer

		// Configuration
		int	m_nThreshold;			///< Movement threshold


	};

	/**
	*  @brief
	*    SpaceMouse input device
	*
	*  @remarks
	*    This class supports the following device backend types:
	*    - HIDDevice
	*/
	class SpaceMouse final : public Device
	{


	//[-------------------------------------------------------]
	//[ Public definitions                                    ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    SpaceMouse IDs
		*
		*  @remarks
		*    It is not recommended to use the ProductID, because it's different for each individual product (NOT future safe)
		*/
		enum EProductID {
			VendorID				 = 1133,	///< '3DConnexion'
			SpaceMousePlus_ProductID = 0xc603,	///< 'SpaceMousePlus'
			SpaceBall_ProductID		 = 0xc621,	///< 'SpaceBall'
			SpaceTraveler_ProductID	 = 0xc623,	///< 'SpaceTraveler'
			SpacePilot_ProductID	 = 0xc625,	///< 'SpacePilot'
			SpaceNavigator_ProductID = 0xc626,	///< 'SpaceNavigator'
			SpaceExplorer_ProductID	 = 0xc627	///< 'SpaceExplorer'
		};


	//[-------------------------------------------------------]
	//[ Controller definition                                 ]
	//[-------------------------------------------------------]
	public:
		Axis	TransX;		///< Absolute x translation axis (the values are usually roughly between [-400 .. 400])
		Axis	TransY;		///< Absolute y translation axis (the values are usually roughly between [-400 .. 400])
		Axis	TransZ;		///< Absolute z translation axis (the values are usually roughly between [-400 .. 400])
		Axis	RotX;		///< Absolute x rotation axis (the values are usually roughly between [-400 .. 400])
		Axis	RotY;		///< Absolute y rotation axis (the values are usually roughly between [-400 .. 400])
		Axis	RotZ;		///< Absolute z rotation axis (the values are usually roughly between [-400 .. 400])
		Button	Button0;	///< Button #0
		Button	Button1;	///< Button #1
		Button	Button2;	///< Button #2
		Button	Button3;	///< Button #3
		Button	Button4;	///< Button #4
		Button	Button5;	///< Button #5
		Button	Button6;	///< Button #6
		Button	Button7;	///< Button #7


	//[-------------------------------------------------------]
	//[ Public functions                                      ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] inputManager
		*    Owner input manager
		*  @param[in] sName
		*    Device name
		*  @param[in] pImpl
		*    System specific device implementation, can, but shouldn't be a null pointer
		*/
		PLINPUT_API SpaceMouse(InputManager& inputManager, const std::string &sName, DeviceImpl *pImpl);

		/**
		*  @brief
		*    Destructor
		*/
		PLINPUT_API virtual ~SpaceMouse() override;


	//[-------------------------------------------------------]
	//[ Private functions                                     ]
	//[-------------------------------------------------------]
	private:
		/**
		*  @brief
		*    Copy operator
		*
		*  @param[in] cSource
		*    Source to copy from
		*
		*  @return
		*    This instance
		*/
		SpaceMouse &operator =(const SpaceMouse &cSource) = delete;

		/**
		*  @brief
		*    Called when the HID device has read some data
		*/
		void OnDeviceRead();


	//[-------------------------------------------------------]
	//[ Private data                                          ]
	//[-------------------------------------------------------]
	private:
		HIDDevice *m_pHIDDevice;	///< HID device


	};

	/**
	*  @brief
	*    WiiMote input device
	*
	*  @remarks
	*    This class supports the following device backend types:
	*    - ConnectionDevice
	*/
	class WiiMote final : public Device
	{


	//[-------------------------------------------------------]
	//[ Controller definition                                 ]
	//[-------------------------------------------------------]
	public:
		// LEDs and effects
		LED		LEDs;				///< LEDs
		Effect	Rumble1;			///< Rumble effect (motor #1)

		// Main buttons
		Button  Button1;			///< Button '1'
		Button  Button2;			///< Button '2'
		Button  ButtonA;			///< Button 'A'
		Button  ButtonB;			///< Button 'B'
		Button  ButtonMinus;		///< Button 'Minus'
		Button  ButtonPlus;			///< Button 'Plus'
		Button  ButtonHome;			///< Button 'Home'
		Button  ButtonLeft;			///< Button 'Left'
		Button  ButtonRight;		///< Button 'Right'
		Button  ButtonUp;			///< Button 'Up'
		Button  ButtonDown;			///< Button 'Down'

		// Main values
		Axis	AccX;				///< Acceleration (X)
		Axis	AccY;				///< Acceleration (Y)
		Axis	AccZ;				///< Acceleration (Z)
		Axis	OrientX;			///< Orientation (X)
		Axis	OrientY;			///< Orientation (Y)
		Axis	OrientZ;			///< Orientation (Z)
		Axis	Roll;				///< Rotation (roll)
		Axis	Pitch;				///< Rotation (pitch)
		Axis	PointerX;			///< Pointer (X)
		Axis	PointerY;			///< Pointer (Y)

		// Nunchuk
		Button  NunchukButtonC;		///< Nunchuk Button 'C'
		Button  NunchukButtonZ;		///< Nunchuk Button 'Z'
		Axis	NunchukAccX;		///< Nunchuk Acceleration (X)
		Axis	NunchukAccY;		///< Nunchuk Acceleration (Y)
		Axis	NunchukAccZ;		///< Nunchuk Acceleration (Z)
		Axis	NunchukOrientX;		///< Nunchuk Orientation (X)
		Axis	NunchukOrientY;		///< Nunchuk Orientation (Y)
		Axis	NunchukOrientZ;		///< Nunchuk Orientation (Z)
		Axis	NunchukRoll;		///< Nunchuk Rotation (roll)
		Axis	NunchukPitch;		///< Nunchuk Rotation (pitch)
		Axis	NunchukX;			///< Nunchuk joystick (X)
		Axis	NunchukY;			///< Nunchuk joystick (Y)


	//[-------------------------------------------------------]
	//[ Public data types                                     ]
	//[-------------------------------------------------------]
	public:
		// Definitions
		enum EProductID {
			// HID definitions
			VendorID  = 0x057e,		///< 'Nintendo'
			ProductID = 0x0306,		///< 'Wiimote'

			// Bluetooth definitions
			DeviceClass0 = 0x04,
			DeviceClass1 = 0x25,
			DeviceClass2 = 0x00
		};

		/**
		*  @brief
		*    Report type
		*/
		enum EReport {
			ReportStatus			= 0x20,	///< Status report
			ReportReadMemory		= 0x21,	///< Data from memory
			ReportButtons			= 0x30,	///< Buttons
			ReportButtonsAccel		= 0x31,	///< Buttons and acceleration
			ReportButtonsAccelIR	= 0x33,	///< Buttons, acceleration and IR (extended)
			ReportButtonsAccelExt	= 0x35,	///< Buttons, acceleration and extension
			ReportButtonsAccelIRExt	= 0x37	///< Buttons, acceleration, IR (basic) and extension
		};

		/**
		*  @brief
		*    Infrared sensor mode
		*/
		enum EIRMode {
			IROff		= 0x00,	///< IR off
			IRBasic		= 0x01,	///< Basic IR mode
			IRExtended	= 0x03,	///< Extended IR mode
			IRFull		= 0x05	///< Full IR mode
		};

		/**
		*  @brief
		*    WiiMote extension
		*/
		enum EExtension {
			ExtNone					= 0x0000,	///< No extension
			ExtNunchuk				= 0xfefe,	///< Nunchuk
			ExtClassic				= 0xfdfd,	///< Classic controller
			ExtPartiallyInserted	= 0xffff	///< Extension not inserted correctly
		};


	//[-------------------------------------------------------]
	//[ Public functions                                      ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] inputManager
		*    Owner input manager
		*  @param[in] sName
		*    Device name
		*  @param[in] pImpl
		*    System specific device implementation, must be valid!
		*/
		PLINPUT_API WiiMote(InputManager& inputManager, const std::string &sName, DeviceImpl *pImpl);

		/**
		*  @brief
		*    Destructor
		*/
		PLINPUT_API virtual ~WiiMote() override;

		/**
		*  @brief
		*    Get report mode
		*
		*  @return
		*    Report mode
		*/
		inline EReport GetReportMode() const
		{
			return m_nReportMode;
		}

		/**
		*  @brief
		*    Set report mode
		*
		*  @param[in] nReportMode
		*    Report mode
		*  @param[in] bContinuous
		*    Continuous updates?
		*/
		PLINPUT_API void SetReportMode(EReport nReportMode, bool bContinuous = false);

		/**
		*  @brief
		*    Get infrared mode
		*
		*  @return
		*    Infrared mode
		*/
		inline EIRMode GetIRMode() const
		{
			return m_nIRMode;
		}

		/**
		*  @brief
		*    Set infrared mode
		*
		*  @param[in] nIRMode
		*    Infrared mode
		*/
		PLINPUT_API void SetIRMode(EIRMode nIRMode);

		/**
		*  @brief
		*    Get extension
		*
		*  @return
		*    Extension type
		*/
		inline EExtension GetExtension() const
		{
			return m_nExtension;
		}

		/**
		*  @brief
		*    Get battery state
		*
		*  @return
		*    Battery state
		*/
		inline uint8_t GetBattery() const
		{
			return m_nBattery;
		}

		/**
		*  @brief
		*    Calibrate device
		*/
		inline void Calibrate()
		{
			SendCalibrationRequest();
		}


	//[-------------------------------------------------------]
	//[ Public virtual Controller functions                   ]
	//[-------------------------------------------------------]
	public:
		PLINPUT_API virtual void UpdateOutputControl(Control *pControl) override;


	//[-------------------------------------------------------]
	//[ Private data types                                    ]
	//[-------------------------------------------------------]
	private:
		/**
		*  @brief
		*    Acceleration data
		*/
		struct SAcceleration {
			// Status
			int nUpdateNearG;	///< Update counter when acceleration is near 1G

			// Status
			float fAccX;		///< Acceleration: X
			float fAccY;		///< Acceleration: Y
			float fAccZ;		///< Acceleration: Z
			float fOrientX;		///< Orientation:  X
			float fOrientY;		///< Orientation:  Y
			float fOrientZ;		///< Orientation:  Z
			float fRoll;		///< Angle:        Roll
			float fPitch;		///< Angle:        Pitch

			// Calibration
			uint8_t nX0;	///< Calibration: X0
			uint8_t nY0;	///< Calibration: Y0
			uint8_t nZ0;	///< Calibration: Z0
			uint8_t nXG;	///< Calibration: XG
			uint8_t nYG;	///< Calibration: YG
			uint8_t nZG;	///< Calibration: ZG

			// Calculate orientation from acceleration data
			void CalculateOrientation();
		};

		/**
		*  @brief
		*    Nunchuk joystick data
		*/
		struct SJoystick {
			// Status
			float fX;	///< X position
			float fY;	///< Y position

			// Calibration
			uint8_t nMinX;	///< Calibration: Minimum X
			uint8_t nMidX;	///< Calibration: Middle  X
			uint8_t nMaxX;	///< Calibration: Maximum X
			uint8_t nMinY;	///< Calibration: Minimum Y
			uint8_t nMidY;	///< Calibration: Middle  Y
			uint8_t nMaxY;	///< Calibration: Maximum Y
		};

		/**
		*  @brief
		*    IR sensor dot
		*/
		struct SDot {
			bool  bFound;	///< The dot has been found
			int   nRawX;	///< Raw X position
			int   nRawY;	///< Raw Y position
			float fX;		///< X position (0..1)
			float fY;		///< Y position (0..1)
			int   nSize;	///< Dot size
		};


	//[-------------------------------------------------------]
	//[ Private functions                                     ]
	//[-------------------------------------------------------]
	private:
		/**
		*  @brief
		*    Copy operator
		*
		*  @param[in] cSource
		*    Source to copy from
		*
		*  @return
		*    This instance
		*/
		WiiMote &operator =(const WiiMote &cSource) = delete;

		/**
		*  @brief
		*    Called when the HID device has been connected
		*/
		void OnDeviceConnect();

		/**
		*  @brief
		*    Called when the HID device has read some data
		*/
		inline void OnDeviceRead()
		{
			// Read data
			OnReadData();

			// Propagate changes
			m_bChanged = true;
		}

		/**
		*  @brief
		*    Data from the WiiMote has been received
		*/
		void OnReadData();

		/**
		*  @brief
		*    Data from memory received
		*/
		void OnReadMemory();

		/**
		*  @brief
		*    Calibration data received
		*/
		void OnReadCalibration();

		/**
		*  @brief
		*    Extension information received
		*/
		void OnReadExtensionType();

		/**
		*  @brief
		*    Nunchuk calibration data received
		*/
		void OnReadNunchukCalibration();

		/**
		*  @brief
		*    Classic calibration data received
		*/
		void OnReadClassicCalibration();

		/**
		*  @brief
		*    Status information received
		*/
		void OnReadStatus();

		/**
		*  @brief
		*    Button status received
		*/
		void OnReadButtons();

		/**
		*  @brief
		*    Acceleration status received
		*/
		void OnReadAccel();

		/**
		*  @brief
		*    IR status received
		*/
		void OnReadIR();

		/**
		*  @brief
		*    Extension status received
		*/
		void OnReadExtension(uint32_t nOffset);

		/**
		*  @brief
		*    Nunchuk status received
		*/
		void OnReadNunchuk(uint32_t nOffset);

		/**
		*  @brief
		*    Classic-controller status received
		*/
		void OnReadClassic(uint32_t nOffset);

		/**
		*  @brief
		*    Read from WiiMote memory
		*
		*  @param[in] nAddress
		*    Address to read from
		*  @param[in] nSize
		*    Size to read
		*/
		void ReadMemory(int nAddress, uint8_t nSize);

		/**
		*  @brief
		*    Write to WiiMote memory
		*
		*  @param[in] nAddress
		*    Address to write to
		*  @param[in] pBuffer
		*    Buffer containing the data
		*  @param[in] nSize
		*    Size to write
		*/
		void WriteMemory(int nAddress, const uint8_t* pBuffer, uint8_t nSize);

		/**
		*  @brief
		*    Write a single byte to WiiMote memory
		*
		*  @param[in] nAddress
		*    Address to write to
		*  @param[in] nData
		*    Byte to write
		*/
		inline void WriteMemory(int nAddress, uint8_t nData)
		{
			// Write one byte
			WriteMemory(nAddress, &nData, 1);
		}

		/**
		*  @brief
		*    Clear output report
		*/
		inline void ClearReport()
		{
			memset(m_pOutputBuffer, 0, 22);
		}

		/**
		*  @brief
		*    Send data to WiiMote device
		*
		*  @param[in] pBuffer
		*    Pointer to buffer (must be valid!)
		*  @param[in] nSize
		*    Size of buffer
		*/
		void Send(uint8_t *pBuffer, uint32_t nSize);

		/**
		*  @brief
		*    Decrypt data
		*
		*  @param[in] nOffset
		*    Start address inside m_nWriteBuffer
		*  @param[in] nSize
		*    Size inside m_nWriteBuffer
		*/
		void DecryptBuffer(uint32_t nOffset, uint32_t nSize);

		/**
		*  @brief
		*    Get WiiMote status
		*/
		void SendStatusRequest();

		/**
		*  @brief
		*    Get calibration information
		*/
		void SendCalibrationRequest();


	//[-------------------------------------------------------]
	//[ Private data                                          ]
	//[-------------------------------------------------------]
	private:
		// HID connection
		ConnectionDevice *m_pConnectionDevice;	///< Connection device, always valid
		uint8_t			 *m_pInputBuffer;		///< Input buffer
		uint8_t			 *m_pOutputBuffer;		///< Output buffer

		// WiiMote options
		EReport	   m_nReportMode;	///< Report mode
		EIRMode	   m_nIRMode;		///< IR mode
		EExtension m_nExtension;	///< Extension type

		// WiiMote status
		uint8_t			m_nBattery;		///< Battery (percent)
		uint8_t			m_nLEDs;		///< LEDs status
		uint8_t			m_nRumble;		///< Rumble state (1=on, 0=off)
		uint16_t		m_nButtons;		///< WiiMote Buttons
		SAcceleration	m_sAcc;			///< Acceleration sensor
		SDot			m_sDots[2];		///< IR dots
		float			m_vIRPos[2];	///< IR position (X, Y between 0..1)

		// Nunchuk status
		uint16_t	  m_nNunchukButtons;	///< Nunchuk buttons
		SAcceleration m_sNunchukAcc;		///< Nunchuk acceleration sensor
		SJoystick	  m_sNunchukJoy;		///< Nunchuk joystick


	};

	/**
	*  @brief
	*    Sensor manager input device
	*
	*  @remarks
	*    The sensor manager is a collection of sensors usually available on a mobile device:
	*    - Accelerometer
	*    - Magnetic field
	*    - Gyroscope
	*    - Light
	*    - Proximity
	*
	*    This class supports the following device backend types:
	*    - UpdateDevice
	*/
	class SensorManager final : public Device
	{


	//[-------------------------------------------------------]
	//[ Controller definition                                 ]
	//[-------------------------------------------------------]
	public:
		// Accelerometer
		Axis	AccelerationX;	///< X acceleration axis (Accelerometer)
		Axis	AccelerationY;	///< Y acceleration axis (Accelerometer)
		Axis	AccelerationZ;	///< Z acceleration axis (Accelerometer)
		// Magnetic field
		Axis	MagneticX;		///< X magnetic axis (Magnetic field)
		Axis	MagneticY;		///< Y magnetic axis (Magnetic field)
		Axis	MagneticZ;		///< Z magnetic axis (Magnetic field)
		// Gyroscope
		Axis	RotationX;		///< X rotation axis (Gyroscope)
		Axis	RotationY;		///< Y rotation axis (Gyroscope)
		Axis	RotationZ;		///< Z rotation axis (Gyroscope)
		// Light
		Axis	Light;			///< Light
		// Proximity
		Axis	Proximity;		///< Proximity


	//[-------------------------------------------------------]
	//[ Public functions                                      ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] inputManager
		*    Owner input manager
		*  @param[in] sName
		*    Device name
		*  @param[in] pImpl
		*    System specific device implementation, can, but shouldn't be a null pointer
		*/
		PLINPUT_API SensorManager(InputManager& inputManager, const std::string &sName, DeviceImpl *pImpl);

		/**
		*  @brief
		*    Destructor
		*/
		inline virtual ~SensorManager() override
		{
			// Nothing here
		}

		SensorManager &operator =(const SensorManager &cSource) = delete;


	//[-------------------------------------------------------]
	//[ Public virtual Controller functions                   ]
	//[-------------------------------------------------------]
	public:
		PLINPUT_API virtual void Update() override;


	};

	/**
	*  @brief
	*    Gamepad device emulation by using a touch screen making it possible to e.g. move & look at the same time
	*
	*  @remarks
	*    This class supports the following device backend types:
	*    - UpdateDevice
	*/
	class SplitTouchPad final : public Device
	{


	//[-------------------------------------------------------]
	//[ Controller definition                                 ]
	//[-------------------------------------------------------]
	public:
		Axis	LeftX;	///< Absolute x axis on the left touchscreen side
		Axis	LeftY;	///< Absolute y axis on the left touchscreen side
		Axis	RightX;	///< Absolute x axis on the right touchscreen side
		Axis	RightY;	///< Absolute y axis on the right touchscreen side


	//[-------------------------------------------------------]
	//[ Public functions                                      ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] inputManager
		*    Owner input manager
		*  @param[in] sName
		*    Device name
		*  @param[in] pImpl
		*    System specific device implementation, can, but shouldn't be a null pointer
		*/
		PLINPUT_API SplitTouchPad(InputManager& inputManager, const std::string &sName, DeviceImpl *pImpl);

		/**
		*  @brief
		*    Destructor
		*/
		inline virtual ~SplitTouchPad() override
		{
			// Nothing here
		}

		/**
		*  @brief
		*    Copy operator
		*
		*  @param[in] cSource
		*    Source to copy from
		*
		*  @return
		*    This instance
		*/
		SplitTouchPad &operator =(const SplitTouchPad &cSource) = delete;


	//[-------------------------------------------------------]
	//[ Public virtual Controller functions                   ]
	//[-------------------------------------------------------]
	public:
		PLINPUT_API virtual void Update() override;


	};

	/**
	*  @brief
	*    Virtual input controller
	*
	*  @remarks
	*    A virtual input controller is a controller that is used to map real input devices to virtual controls.
	*    Usually, you have one virtual input controller for your application, or in rare occasions more than one,
	*    e.g. one for every window or player. The virtual controller connects itself to the physical input devices,
	*    a virtual function can be used to alter this behavior in derived classes. The virtual controller should
	*    then be connected to the controllers of input-enabled objects, such as scene nodes or modifiers.
	*/
	class VirtualController : public Controller
	{


	//[-------------------------------------------------------]
	//[ Public functions                                      ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] inputManager
		*    Owner input manager
		*  @param[in] sName
		*    Controller name
		*  @param[in] sDescription
		*    Controller description
		*/
		inline VirtualController(InputManager& inputManager, const std::string &sName, const std::string &sDescription) :
			Controller(inputManager, ControllerType::VIRTUAL, sName, sDescription)
		{
			// Nothing here
		}

		/**
		*  @brief
		*    Destructor
		*/
		inline virtual ~VirtualController() override
		{
			// Nothing here
		}

		VirtualController &operator =(const VirtualController &cSource) = delete;


	//[-------------------------------------------------------]
	//[ Public virtual VirtualController functions            ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Connect virtual controller to physical devices
		*
		*  @remarks
		*    This function shall enumerate the available devices and connect
		*    itself to the proper controls of those input devices. Override
		*    this function in derived classes to alter it's behavior or to
		*    support your own type of virtual controllers
		*/
		inline virtual void ConnectToDevices()
		{
			// Nothing here
		}


	};

	/**
	*  @brief
	*    Standard virtual input controller
	*
	*  @remarks
	*    3D coordinate system:
	*    @code
	*      y = > Translation: Move up/down (+/-) => Rotation: Yaw (also called 'heading') change is turning to the left or right
	*      |
	*      |
	*      *---x => Translation: Strafe left/right (+/-) => Rotation: Pitch (also called 'bank') change is moving the nose down and the tail up (or vice-versa)
	*     /
	*    z => Translation: Move forwards/backwards (+/-) => Rotation: Roll (also called 'attitude') change is moving one wingtip up and the other down
	*    @endcode
	**/
	class VirtualStandardController final : public VirtualController
	{


	//[-------------------------------------------------------]
	//[ Controller definition                                 ]
	//[-------------------------------------------------------]
	public:
		// Mouse
		Axis		MouseX;						///< X axis (movement data, no absolute data)
		Axis		MouseY;						///< Y axis (movement data, no absolute data)
		Axis		MouseWheel;					///< Mouse wheel (movement data, no absolute data)
		Button		MouseLeft;					///< Left mouse button (mouse button #0)
		Button		MouseRight;					///< Right mouse button (mouse button #1)
		Button		MouseMiddle;				///< Middle mouse button (mouse button #2)
		Button		MouseButton4;				///< Mouse button #4
		Button		MouseButton5;				///< Mouse button #5
		Button		MouseButton6;				///< Mouse button #6
		Button		MouseButton7;				///< Mouse button #7
		Button		MouseButton8;				///< Mouse button #8
		Button		MouseButton9;				///< Mouse button #9
		Button		MouseButton10;				///< Mouse button #10
		Button		MouseButton11;				///< Mouse button #11
		Button		MouseButton12;				///< Mouse button #12

		// Keyboard
		Button		KeyboardBackspace;			///< Backspace
		Button		KeyboardTab;				///< Tab
		Button		KeyboardClear;				///< Clear (not available everywhere)
		Button		KeyboardReturn;				///< Return (often the same as "Enter")
		Button		KeyboardShift;				///< Shift
		Button		KeyboardControl;			///< Control ("Ctrl")
		Button		KeyboardAlt;				///< Alt
		Button		KeyboardPause;				///< Pause
		Button		KeyboardCapsLock;			///< Caps lock
		Button		KeyboardEscape;				///< Escape
		Button		KeyboardSpace;				///< Space
		Button		KeyboardPageUp;				///< Page up
		Button		KeyboardPageDown;			///< Page down
		Button		KeyboardEnd;				///< End
		Button		KeyboardHome;				///< Home
		Button		KeyboardLeft;				///< Left arrow
		Button		KeyboardUp;					///< Up arrow
		Button		KeyboardRight;				///< Right arrow
		Button		KeyboardDown;				///< Down arrow
		Button		KeyboardSelect;				///< Select (not available everywhere)
		Button		KeyboardExecute;			///< Execute (not available everywhere)
		Button		KeyboardPrint;				///< Print screen
		Button		KeyboardInsert;				///< Insert
		Button		KeyboardDelete;				///< Delete
		Button		KeyboardHelp;				///< Help (not available everywhere)
		Button		Keyboard0;					///< 0
		Button		Keyboard1;					///< 1
		Button		Keyboard2;					///< 2
		Button		Keyboard3;					///< 3
		Button		Keyboard4;					///< 4
		Button		Keyboard5;					///< 5
		Button		Keyboard6;					///< 6
		Button		Keyboard7;					///< 7
		Button		Keyboard8;					///< 8
		Button		Keyboard9;					///< 9
		Button		KeyboardA;					///< A
		Button		KeyboardB;					///< B
		Button		KeyboardC;					///< C
		Button		KeyboardD;					///< D
		Button		KeyboardE;					///< E
		Button		KeyboardF;					///< F
		Button		KeyboardG;					///< G
		Button		KeyboardH;					///< H
		Button		KeyboardI;					///< I
		Button		KeyboardJ;					///< J
		Button		KeyboardK;					///< K
		Button		KeyboardL;					///< L
		Button		KeyboardM;					///< M
		Button		KeyboardN;					///< N
		Button		KeyboardO;					///< O
		Button		KeyboardP;					///< P
		Button		KeyboardQ;					///< Q
		Button		KeyboardR;					///< R
		Button		KeyboardS;					///< S
		Button		KeyboardT;					///< T
		Button		KeyboardU;					///< U
		Button		KeyboardV;					///< V
		Button		KeyboardW;					///< W
		Button		KeyboardX;					///< X
		Button		KeyboardY;					///< Y
		Button		KeyboardZ;					///< Z
		Button		KeyboardNumpad0;			///< Numeric keypad 0
		Button		KeyboardNumpad1;			///< Numeric keypad 1
		Button		KeyboardNumpad2;			///< Numeric keypad 2
		Button		KeyboardNumpad3;			///< Numeric keypad 3
		Button		KeyboardNumpad4;			///< Numeric keypad 4
		Button		KeyboardNumpad5;			///< Numeric keypad 5
		Button		KeyboardNumpad6;			///< Numeric keypad 6
		Button		KeyboardNumpad7;			///< Numeric keypad 7
		Button		KeyboardNumpad8;			///< Numeric keypad 8
		Button		KeyboardNumpad9;			///< Numeric keypad 9
		Button		KeyboardNumpadMultiply;		///< Numeric keypad "Multiply"
		Button		KeyboardNumpadAdd;			///< Numeric keypad "Add"
		Button		KeyboardNumpadSeparator;	///< Numeric keypad "Separator"
		Button		KeyboardNumpadSubtract;		///< Numeric keypad "Subtract"
		Button		KeyboardNumpadDecimal;		///< Numeric keypad "Decimal"
		Button		KeyboardNumpadDivide;		///< Numeric keypad "Divide"
		Button		KeyboardF1;					///< F1
		Button		KeyboardF2;					///< F2
		Button		KeyboardF3;					///< F3
		Button		KeyboardF4;					///< F4
		Button		KeyboardF5;					///< F5
		Button		KeyboardF6;					///< F6
		Button		KeyboardF7;					///< F7
		Button		KeyboardF8;					///< F8
		Button		KeyboardF9;					///< F9
		Button		KeyboardF10;				///< F10
		Button		KeyboardF11;				///< F11
		Button		KeyboardF12;				///< F12
		Button		KeyboardNumLock;			///< Num lock
		Button		KeyboardScrollLock;			///< Scroll lock
		Button		KeyboardCircumflex;			///< Circumflex (^)
		Button		KeyboardLeftWindows;		///< Left Windows key (natural keyboard)
		Button		KeyboardRightWindows;		///< Right Windows key (natural keyboard)
		Button		KeyboardApplications;		///< Applications key (natural keyboard)
		Button		KeyboardF13;				///< F13 key
		Button		KeyboardF14;				///< F14 key
		Button		KeyboardF15;				///< F15 key
		Button		KeyboardF16;				///< F16 key
		Button		KeyboardF17;				///< F17 key
		Button		KeyboardF18;				///< F18 key
		Button		KeyboardF19;				///< F19 key
		Button		KeyboardF20;				///< F20 key
		Button		KeyboardF21;				///< F21 key
		Button		KeyboardF22;				///< F22 key
		Button		KeyboardF23;				///< F23 key
		Button		KeyboardF24;				///< F24 key
		Button		KeyboardLeftShift;			///< Left shift key
		Button		KeyboardRightShift;			///< Right shift key
		Button		KeyboardLeftControl;		///< Left control key
		Button		KeyboardRightControl;		///< Right control key
		Button		KeyboardVolumeMute;			///< Volume mute key
		Button		KeyboardVolumeDown;			///< Volume down key
		Button		KeyboardVolumeUp;			///< Volume up key
		Button		KeyboardMediaNextTrack;		///< Next track key
		Button		KeyboardMediaPreviousTrack;	///< Previous track key
		Button		KeyboardMediaStop;			///< Stop media key
		Button		KeyboardMediaPlayPause;		///< Play/pause media key
		Button		KeyboardAdd;				///< For any country/region, the '+' key
		Button		KeyboardSeparator;			///< For any country/region, the ',' key
		Button		KeyboardSubtract;			///< For any country/region, the '-' key
		Button		KeyboardDecimal;			///< For any country/region, the '.' key
		Button		KeyboardOEM1;				///< Used for miscellaneous characters; it can vary by keyboard. For the US standard keyboard, the ';:' key
		Button		KeyboardOEM2;				///< Used for miscellaneous characters; it can vary by keyboard. For the US standard keyboard, the '/?' key
		Button		KeyboardOEM3;				///< Used for miscellaneous characters; it can vary by keyboard. For the US standard keyboard, the '`~' key
		Button		KeyboardOEM4;				///< Used for miscellaneous characters; it can vary by keyboard. For the US standard keyboard, the '[{' key
		Button		KeyboardOEM5;				///< Used for miscellaneous characters; it can vary by keyboard. For the US standard keyboard, the '\|' key
		Button		KeyboardOEM6;				///< Used for miscellaneous characters; it can vary by keyboard. For the US standard keyboard, the ']}' key
		Button		KeyboardOEM7;				///< Used for miscellaneous characters; it can vary by keyboard. For the US standard keyboard, the 'single-quote/double-quote' key
		Button		KeyboardOEM8;				///< Used for miscellaneous characters; it can vary by keyboard
		Button		KeyboardOEM102;				///< Either the angle bracket key or the backslash key on the RT 102-key keyboard

		// Main character controls
		Axis		TransX;						///< X translation axis: Strafe left/right (+/-)
		Axis		TransY;						///< Y translation axis: Move up/down (+/-)
		Axis		TransZ;						///< Z translation axis: Move forwards/backwards (+/-)
		Button		Pan;						///< Keep pressed to pan
		Axis		PanX;						///< X pan translation axis: Strafe left/right (+/-)
		Axis		PanY;						///< Y pan translation axis: Move up/down (+/-)
		Axis		PanZ;						///< Z pan translation axis: Move forwards/backwards (+/-)
		Axis		RotX;						///< X rotation axis: Pitch (also called 'bank') change is moving the nose down and the tail up (or vice-versa)
		Axis		RotY;						///< Y rotation axis: Yaw (also called 'heading') change is turning to the left or right
		Axis		RotZ;						///< Z rotation axis: Roll (also called 'attitude') change is moving one wingtip up and the other down
		Button		Rotate;						///< Keep pressed to rotate
		Button		RotateSlow;					///< Keep pressed to rotate slowly
		Button		Forward;					///< Move forwards
		Button		Backward;					///< Move backwards
		Button		Left;						///< Move (rotate) left
		Button		Right;						///< Move (rotate) right
		Button		StrafeLeft;					///< Strafe left
		Button		StrafeRight;				///< Strafe right
		Button		Up;							///< Move up
		Button		Down;						///< Move down
		Button		Run;						///< Keep pressed to run
		Button		Sneak;						///< Keep pressed to sneak
		Button		Crouch;						///< Keep pressed to crouch
		Button		Jump;						///< Jump
		Button		Zoom;						///< Keep pressed to zoom
		Axis		ZoomAxis;					///< Zoom axis to zoom in or out (+/-)
		Button		Button1;					///< Button for action #1
		Button		Button2;					///< Button for action #2
		Button		Button3;					///< Button for action #3
		Button		Button4;					///< Button for action #4
		Button		Button5;					///< Button for action #5

		// Interaction
		Button		Pickup;						///< Keep pressed to pickup
		Button		Throw;						///< Throw the picked object
		Button		IncreaseForce;				///< Keep pressed to increase the force applied to the picked object
		Button		DecreaseForce;				///< Keep pressed to decrease the force applied to the picked object
		Axis		PushPull;					///< Used to push/pull the picked object


	//[-------------------------------------------------------]
	//[ Public functions                                      ]
	//[-------------------------------------------------------]
	public:
		VirtualStandardController() = delete;

		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] inputManager
		*    Owner input manager
		*/
		PLINPUT_API explicit VirtualStandardController(InputManager& inputManager);

		/**
		*  @brief
		*    Destructor
		*/
		inline virtual ~VirtualStandardController() override
		{
			// Nothing here
		}

		/**
		*  @brief
		*    Copy operator
		*
		*  @param[in] cSource
		*    Source to copy from
		*
		*  @return
		*    This instance
		*/
		VirtualStandardController &operator =(const VirtualStandardController &cSource) = delete;


	//[-------------------------------------------------------]
	//[ Public virtual VirtualController functions            ]
	//[-------------------------------------------------------]
	public:
		PLINPUT_API virtual void ConnectToDevices() override;


	};

	/**
	*  @brief
	*    Input manager
	*
	*  @note
	*    - The input manager stores all available devices that are present on the computer and controls the update of input messages
	*/
	class InputManager final
	{


	//[-------------------------------------------------------]
	//[ Friends                                               ]
	//[-------------------------------------------------------]
	friend class Provider;
	friend class Control;


	//[-------------------------------------------------------]
	//[ Public definitions                                    ]
	//[-------------------------------------------------------]
	public:
		typedef std::vector<Device*> Devices;


	//[-------------------------------------------------------]
	//[ Public functions                                      ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Default constructor
		*/
		InputManager();

		/**
		*  @brief
		*    Destructor
		*/
		~InputManager();

		/**
		*  @brief
		*    Update input manager once per frame
		*
		*  @remarks
		*    This function must be called once per frame to allow devices to update their status
		*    and to process input messages read from these devices. This is also done to make sure
		*    that input messages are processed synchronously in the main thread, rather than sending
		*    messages from other threads asynchronously.
		*/
		PLINPUT_API void Update();

		/**
		*  @brief
		*    Detect devices
		*
		*  @param[in] bReset
		*    If 'true', delete all input devices and re-detect them all. Otherwise,
		*    only new and removed input devices will be detected.
		*
		*  @remarks
		*    bReset = true should only be used if really necessary, because existing
		*    input handlers will most certainly lose their connection to the device.
		*/
		PLINPUT_API void DetectDevices(bool bReset = false);

		/**
		*  @brief
		*    Get list of input providers
		*
		*  @return
		*    Provider list, do not destroy the returned instances
		*/
		inline const std::vector<Provider*> &GetProviders() const
		{
			return m_lstProviders;
		}

		/**
		*  @brief
		*    Get a specific input provider
		*
		*  @param[in] sProvider
		*    Name of provider
		*
		*  @return
		*    Provider, or a null pointer if it doesn't exist, do not destroy the returned instance!
		*/
		inline Provider *GetProvider(const std::string &sProvider)
		{
			ProviderMap::const_iterator iterator = m_mapProviders.find(sProvider);
			return (m_mapProviders.cend() != iterator) ? iterator->second : nullptr;
		}

		/**
		*  @brief
		*    Get list of devices
		*
		*  @return
		*    Device list, do not destroy the returned instances!
		*/
		inline Devices &GetDevices()
		{
			return m_lstDevices;
		}

		/**
		*  @brief
		*    Get a specific device
		*
		*  @param[in] sDevice
		*    Name of device
		*
		*  @return
		*    Device, or a null pointer if it doesn't exist, do not destroy the returned instance!
		*/
		inline Device *GetDevice(const std::string &sDevice) const
		{
			DeviceMap::const_iterator iterator = m_mapDevices.find(sDevice);
			return (m_mapDevices.cend() != iterator) ? iterator->second : nullptr;
		}

		/**
		*  @brief
		*    Get default keyboard device
		*
		*  @return
		*    Default keyboard, can be a null pointer, do not destroy the returned instance!
		*/
		inline Keyboard *GetKeyboard() const
		{
			return static_cast<Keyboard*>(GetDevice("Keyboard"));
		}

		/**
		*  @brief
		*    Get default mouse device
		*
		*  @return
		*    Default mouse, can be a null pointer, do not destroy the returned instance!
		*/
		inline Mouse *GetMouse() const
		{
			return static_cast<Mouse*>(GetDevice("Mouse"));
		}


	//[-------------------------------------------------------]
	//[ Private functions                                     ]
	//[-------------------------------------------------------]
	private:
		/**
		*  @brief
		*    Copy constructor
		*
		*  @param[in] cSource
		*    Source to copy from
		*/
		InputManager(const InputManager &cSource) = delete;

		/**
		*  @brief
		*    Copy operator
		*
		*  @param[in] cSource
		*    Source to copy from
		*
		*  @return
		*    Reference to this input manager instance
		*/
		InputManager& operator= (const InputManager &cSource) = delete;

		/**
		*  @brief
		*    Destroy all input providers and devices
		*/
		void Clear();

		/**
		*  @brief
		*    Detect devices from a specific provider
		*
		*  @param[in] sProvider
		*    Name of provider
		*  @param[in] bReset
		*    If 'true', delete all input devices and re-detect them all. Otherwise,
		*    only new and removed input devices will be detected.
		*
		*  @remarks
		*    If the provider is already present, it's Detect()-method will be called. Otherwise,
		*    a new instance of the provider will be created, then Detect() will be called as well.
		*/
		void DetectProvider(const std::string &sProvider, bool bReset);

		/**
		*  @brief
		*    Add a new input device
		*
		*  @param[in] pDevice
		*    Input device, shouldn't be a null pointer (but a null pointer is caught internally)
		*
		*  @return
		*    'true' if all went fine, else 'false'
		*/
		bool AddDevice(Device *pDevice);

		/**
		*  @brief
		*    Remove device
		*
		*  @param[in] pDevice
		*    Input device, shouldn't be a null pointer (but a null pointer is caught internally)
		*
		*  @return
		*    'true' if all went fine, else 'false'
		*/
		bool RemoveDevice(Device *pDevice);

		/**
		*  @brief
		*    Remove control
		*
		*  @param[in] pControl
		*    Input control to remove, shouldn't be a null pointer (but a null pointer is caught internally)
		*/
		void RemoveControl(Control *pControl);

		/**
		*  @brief
		*    Update control
		*
		*  @param[in] pControl
		*    Input control, shouldn't be a null pointer (but a null pointer is caught internally)
		*
		*  @remarks
		*    This marks the control as being updated recently, which will fire a message
		*    in the next Update()-call.
		*/
		void UpdateControl(Control *pControl);


	//[-------------------------------------------------------]
	//[ Private definitions                                   ]
	//[-------------------------------------------------------]
	private:
		typedef std::vector<Control*> Controls;
		typedef std::unordered_map<std::string, Provider*> ProviderMap;
		typedef std::unordered_map<std::string, Device*> DeviceMap;


	//[-------------------------------------------------------]
	//[ Private data                                          ]
	//[-------------------------------------------------------]
	private:
		// Providers and devices
		std::vector<Provider*>	 m_lstProviders;		///< List of providers
		ProviderMap				 m_mapProviders;		///< Hash map of providers
		Devices					 m_lstDevices;			///< List of devices
		DeviceMap				 m_mapDevices;			///< Hash map of devices
		std::mutex				*mMutex;				///< Mutex for reading/writing input messages, always valid
		Controls				 m_lstUpdatedControls;	///< List of controls that have been updated (message list)


	};


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
} // DeviceInput
