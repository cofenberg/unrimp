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


// Single-header input library conversion of the PixelLight ( https://www.pixellight.org/ ) input system originally designed and developed by Stefan Buschmann ( https://www.stefanbuschmann.de/ )


//[-------------------------------------------------------]
//[ Includes                                              ]
//[-------------------------------------------------------]
#include "PLInput/Input.h"

#ifdef WIN32
	// Disable warnings in external headers, we can't fix them
	PRAGMA_WARNING_PUSH
		PRAGMA_WARNING_DISABLE_MSVC(4365)	// warning C4365: 'argument': conversion from 'const char' to 'utf8::uint8_t', signed/unsigned mismatch
		// TODO(co) Remove those
		#include <utf8/utf8.h>	// To convert UTF-8 strings to UTF-16
	PRAGMA_WARNING_POP

	// Set windows version to Windows XP
	#define WINVER			0x0501
	#define _WIN32_WINNT	0x0501

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

	// Include windows header
	__pragma(warning(push))
		__pragma(warning(disable: 4365))	// warning C4365: 'argument': conversion from 'long' to 'unsigned int', signed/unsigned mismatch
		__pragma(warning(disable: 4668))	// warning C4668: 'defined_WINBASE_' is not defined as a preprocessor macro, replacing with '0' for '#if/#elif'
		__pragma(warning(disable: 5039))	// warning C5039: 'DSA_DestroyCallback': pointer or reference to potentially throwing function passed to extern C function under -EHc. Undefined behavior may occur if this function throws an exception.
		#include <windows.h>
		#include <mmsystem.h>
		extern "C" {
			#include <setupapi.h> 
		}
	__pragma(warning(pop))

	// We undef these to avoid name conflicts
	#undef DrawText
	#undef LoadImage
	#undef MessageBox
	#undef GetClassName
	#undef CreateDirectory
	#undef SetCurrentDirectory
	#undef GetCurrentDirectory
	#undef GetEnvironmentVariable
	#undef SetEnvironmentVariable
	#undef GetComputerName
	#undef GetUserName
	#undef Yield


	//[-------------------------------------------------------]
	//[ Definitions                                           ]
	//[-------------------------------------------------------]
	// Used HID structures
	typedef struct _HIDP_PREPARSED_DATA *PHIDP_PREPARSED_DATA;
	typedef enum _HIDP_REPORT_TYPE {
		HidP_Input,
		HidP_Output,
		HidP_Feature
	} HIDP_REPORT_TYPE;
	typedef struct _HIDP_DATA {
		USHORT DataIndex;
		USHORT Reserved;
		union {
			ULONG   RawValue;
			BOOLEAN On;
		};
	} HIDP_DATA;
	typedef struct _HIDD_ATTRIBUTES {
		ULONG  Size;
		USHORT VendorID;
		USHORT ProductID;
		USHORT VersionNumber;
	} HIDD_ATTRIBUTES;
	typedef struct _HIDP_CAPS {
		USHORT Usage;
		USHORT UsagePage;
		USHORT InputReportByteLength;
		USHORT OutputReportByteLength;
		USHORT FeatureReportByteLength;
		USHORT Reserved[17];
		USHORT NumberLinkCollectionNodes;
		USHORT NumberInputButtonCaps;
		USHORT NumberInputValueCaps;
		USHORT NumberInputDataIndices;
		USHORT NumberOutputButtonCaps;
		USHORT NumberOutputValueCaps;
		USHORT NumberOutputDataIndices;
		USHORT NumberFeatureButtonCaps;
		USHORT NumberFeatureValueCaps;
		USHORT NumberFeatureDataIndices;
	} HIDP_CAPS;
	typedef struct _HIDP_BUTTON_CAPS {
		USHORT  UsagePage;
		UCHAR   ReportID;
		BOOLEAN IsAlias;
		USHORT  BitField;
		USHORT  LinkCollection;
		USHORT  LinkUsage;
		USHORT  LinkUsagePage;
		BOOLEAN IsRange;
		BOOLEAN IsStringRange;
		BOOLEAN IsDesignatorRange;
		BOOLEAN IsAbsolute;
		ULONG   Reserved[10];
		union {
			struct {
				USHORT UsageMin,        UsageMax;
				USHORT StringMin,       StringMax;
				USHORT DesignatorMin,   DesignatorMax;
				USHORT DataIndexMin,    DataIndexMax;
			} Range;
			struct  {
				USHORT Usage,           Reserved1;
				USHORT StringIndex,     Reserved2;
				USHORT DesignatorIndex, Reserved3;
				USHORT DataIndex,       Reserved4;
			} NotRange;
		};
	} HIDP_BUTTON_CAPS;
	typedef struct _HIDP_VALUE_CAPS {
		USHORT  UsagePage;
		UCHAR   ReportID;
		BOOLEAN IsAlias;
		USHORT  BitField;
		USHORT  LinkCollection;
		USHORT  LinkUsage;
		USHORT  LinkUsagePage;
		BOOLEAN IsRange;
		BOOLEAN IsStringRange;
		BOOLEAN IsDesignatorRange;
		BOOLEAN IsAbsolute;
		BOOLEAN HasNull;
		UCHAR   Reserved;
		USHORT  BitSize;
		USHORT  ReportCount;
		USHORT  Reserved2[5];
		ULONG   UnitsExp;
		ULONG   Units;
		LONG    LogicalMin,  LogicalMax;
		LONG    PhysicalMin, PhysicalMax;
		union {
			struct {
				USHORT UsageMin,        UsageMax;
				USHORT StringMin,       StringMax;
				USHORT DesignatorMin,   DesignatorMax;
				USHORT DataIndexMin,    DataIndexMax;
			} Range;
			struct {
				USHORT Usage,           Reserved1;
				USHORT StringIndex,     Reserved2;
				USHORT DesignatorIndex, Reserved3;
				USHORT DataIndex,       Reserved4;
			} NotRange;
		};
	} HIDP_VALUE_CAPS;
	#define HIDP_STATUS_SUCCESS (((LONG) (((0x0) << 28) | (0x11 << 16) | (0))))

	// Raw Input definitions
	#ifndef HID_USAGE_PAGE_GENERIC
		#define HID_USAGE_PAGE_GENERIC		((USHORT) 0x01)
		#define HID_USAGE_GENERIC_MOUSE		((USHORT) 0x02)
		#define HID_USAGE_GENERIC_JOYSTICK	((USHORT) 0x04)
		#define HID_USAGE_GENERIC_GAMEPAD	((USHORT) 0x05)
		#define HID_USAGE_GENERIC_KEYBOARD	((USHORT) 0x06)
	#endif


	//[-------------------------------------------------------]
	//[ Global HID function pointers                          ]
	//[-------------------------------------------------------]
	// HidD_GetPreparsedData
	typedef BOOLEAN (__stdcall *PFNHIDDGETPREPARSEDDATA) (IN HANDLE HidDeviceObject, OUT PHIDP_PREPARSED_DATA *PreparsedData);
	extern PFNHIDDGETPREPARSEDDATA HidD_GetPreparsedData;
	// HidD_FreePreparsedData
	typedef BOOLEAN (__stdcall *PFNHIDDFREEPREPARSEDDATA) (IN PHIDP_PREPARSED_DATA PreparsedData);
	extern PFNHIDDFREEPREPARSEDDATA HidD_FreePreparsedData;
	// HidP_GetData
	typedef LONG (__stdcall *PFNHIDPGETDATA) (IN HIDP_REPORT_TYPE ReportType, OUT HIDP_DATA *DataList, IN OUT PULONG DataLength, IN PHIDP_PREPARSED_DATA PreparsedData, IN PCHAR Report, IN ULONG ReportLength);
	extern PFNHIDPGETDATA HidP_GetData;
	// HidP_SetData
	typedef LONG (__stdcall *PFNHIDPSETDATA) (IN HIDP_REPORT_TYPE ReportType, IN HIDP_DATA *DataList, IN OUT PULONG DataLength, IN PHIDP_PREPARSED_DATA PreparsedData, IN OUT PCHAR Report, IN ULONG ReportLength);
	extern PFNHIDPSETDATA HidP_SetData;
	// HidD_GetHidGuid
	typedef void (__stdcall *PFNHIDPGETHIDGUID) (OUT LPGUID HidGuid);
	extern PFNHIDPGETHIDGUID HidD_GetHidGuid;
	// HidD_GetAttributes
	typedef BOOLEAN (__stdcall *PFNHIDPGETATTRIBUTES) (IN HANDLE HidDeviceObject, OUT HIDD_ATTRIBUTES *Attributes);
	extern PFNHIDPGETATTRIBUTES HidD_GetAttributes;
	// HidP_GetCaps
	typedef LONG (__stdcall *PFNHIDPGETCAPS) (IN PHIDP_PREPARSED_DATA PreparsedData, OUT HIDP_CAPS *Capabilities);
	extern PFNHIDPGETCAPS HidP_GetCaps;
	// HidP_GetButtonCaps
	typedef LONG (__stdcall *PFNHIDPGETBUTTONCAPS) (IN HIDP_REPORT_TYPE ReportType, OUT HIDP_BUTTON_CAPS *ButtonCaps, IN OUT PUSHORT ButtonCapsLength, IN PHIDP_PREPARSED_DATA PreparsedData);
	extern PFNHIDPGETBUTTONCAPS HidP_GetButtonCaps;
	// HidP_GetValueCaps
	typedef LONG (__stdcall *PFNHIDPGETVALUECAPS) (IN HIDP_REPORT_TYPE ReportType, OUT HIDP_VALUE_CAPS *ValueCaps, IN OUT PUSHORT ValueCapsLength, IN PHIDP_PREPARSED_DATA PreparsedData);
	extern PFNHIDPGETVALUECAPS HidP_GetValueCaps;
#endif

// Disable warnings in external headers, we can't fix them
PRAGMA_WARNING_PUSH
	PRAGMA_WARNING_DISABLE_MSVC(4365)	// warning C4365: 'argument': conversion from 'long' to 'unsigned int', signed/unsigned mismatch
	PRAGMA_WARNING_DISABLE_MSVC(4571)	// warning C4571: Informational: catch(...) semantics changed since Visual C++ 7.1; structured exceptions (SEH) are no longer caught
	PRAGMA_WARNING_DISABLE_MSVC(4625)	// warning C4625: 'std::_Generic_error_category': copy constructor was implicitly defined as deleted
	PRAGMA_WARNING_DISABLE_MSVC(4626)	// warning C4626: 'std::_Generic_error_category': assignment operator was implicitly defined as deleted
	PRAGMA_WARNING_DISABLE_MSVC(5026)	// warning C5026: 'std::_Generic_error_category': move constructor was implicitly defined as deleted
	PRAGMA_WARNING_DISABLE_MSVC(5027)	// warning C5027: 'std::_Generic_error_category': move assignment operator was implicitly defined as deleted
	PRAGMA_WARNING_DISABLE_MSVC(5039)	// warning C5039: '_Thrd_start': pointer or reference to potentially throwing function passed to extern C function under -EHc. Undefined behavior may occur if this function throws an exception.
	#include <inttypes.h>	// For uint32_t, uint64_t etc.
	#include <functional>
	#include <map>
	#include <mutex>
	#include <thread>
	#include <cassert>
PRAGMA_WARNING_POP


// Disable uncritical warning
PRAGMA_WARNING_DISABLE_MSVC(4355)	// warning C4355: 'this': used in base member initializer list


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
namespace PLInput
{


	//[-------------------------------------------------------]
	//[ Signal template                                       ]
	//[-------------------------------------------------------]
	// From http://simmesimme.github.io/tutorials/2015/09/20/signal-slot
	// A signal object may call multiple slots with the
	// same signature. You can connect functions to the signal
	// which will be called when the emit() method on the
	// signal object is invoked. Any argument passed to emit()
	// will be passed to the given functions.

	template <typename... Args>
	class Signal {

	 public:

	  Signal() : current_id_(0) {}

	  // copy creates new signal
	  Signal(Signal const& other) : current_id_(0) {}

	  // connects a member function to this Signal
	  template <typename T>
	  int connect_member(T *inst, void (T::*func)(Args...)) {
		return connect([=](Args... args) { 
		  (inst->*func)(args...); 
		});
	  }

	  // connects a const member function to this Signal
	  template <typename T>
	  int connect_member(T *inst, void (T::*func)(Args...) const) {
		return connect([=](Args... args) {
		  (inst->*func)(args...); 
		});
	  }

	  // connects a std::function to the signal. The returned
	  // value can be used to disconnect the function again
	  int connect(std::function<void(Args...)> const& slot) const {
		slots_.insert(std::make_pair(++current_id_, slot));
		return current_id_;
	  }

	  // disconnects a previously connected function
	  void disconnect(int id) const {
		slots_.erase(id);
	  }

	  // disconnects all previously connected functions
	  void disconnect_all() const {
		slots_.clear();
	  }

	  // calls all connected functions
	  void emit(Args... p) {
		for(auto it : slots_) {
		  it.second(p...);
		}
	  }

	  // assignment creates new Signal
	  Signal& operator=(Signal const& other) {
		disconnect_all();
	  }

	 private:
	  mutable std::map<int, std::function<void(Args...)>> slots_;
	  mutable int current_id_;
	};








	//[-------------------------------------------------------]
	//[ Abstract backend only interfaces                      ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Input provider
	*
	*  @remarks
	*    An input provider is responsible for detecting and managing a specific set of input devices,
	*    e.g. the core provider will look after keyboard and mouse. Other types of input providers can
	*    be written to support new input devices, typically implemented as plugins.
	*/
	class Provider
	{


	//[-------------------------------------------------------]
	//[ Public functions                                      ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Destructor
		*/
		inline virtual ~Provider()
		{
			// Clean up
			Clear();
		}

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
		*    input handlers will most certainly loose their connection to the device.
		*/
		void DetectDevices(bool bReset = false);

		/**
		*  @brief
		*    Get list of devices
		*
		*  @return
		*    Device list, do not destroy the returned instances!
		*/
		inline const std::vector<Device*> &GetDevices() const
		{
			return m_lstDevices;
		}


	//[-------------------------------------------------------]
	//[ Protected functions                                   ]
	//[-------------------------------------------------------]
	protected:
		Provider() = delete;

		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] inputManager
		*    Owner input manager
		*/
		inline explicit Provider(InputManager& inputManager) :
			mInputManager(inputManager)
		{
			// Nothing here
		}

		Provider& operator= (const Provider& source) = delete;

		/**
		*  @brief
		*    Destroy all devices
		*/
		void Clear();

		/**
		*  @brief
		*    Check if a device is already known and update it
		*
		*  @param[in] sName
		*    Name of the input device (e.g. "Mouse0")
		*
		*  @return
		*    'true', if a device with this name is known, else 'false'
		*
		*  @remarks
		*    If the device is already present, it's "confirmed"-flag is set to 'true', so
		*    it will not get deleted by DetectDevices(). Therefore, an input provider must
		*    call this function every time in it's QueryDevices()-function.
		*/
		bool CheckDevice(const std::string &sName);

		/**
		*  @brief
		*    Add a new input device
		*
		*  @param[in] sName
		*    Name of the input device (e.g. "Mouse0")
		*  @param[in] pDevice
		*    Pointer to the device (derived from Device), shouldn't be a null pointer (but a null pointer is caught internally)
		*
		*  @return
		*    'true' if all went fine, else 'false'
		*
		*  @remarks
		*    The function will fail, if the device is already present. Otherwise,
		*    the new device will be registered in the input system. On success, the input provider
		*    takes control over the device instance and will destroy it if it's no longer required.
		*/
		bool AddDevice(const std::string &sName, Device *pDevice);


	//[-------------------------------------------------------]
	//[ Protected virtual Provider functions                  ]
	//[-------------------------------------------------------]
	protected:
		/**
		*  @brief
		*    Query devices
		*
		*  @remarks
		*    The provider is supposed to detect all of it's devices in this function and then call
		*    AddDevice() with the specific name for each device. AddDevice() will fail if the device
		*    is already there, which can be checked before by calling CheckDevice(). The naming scheme
		*    is up to the provider itself, but something like "DeviceType0" .. "DeviceTypeN" is recommended.
		*/
		virtual void QueryDevices() = 0;


	//[-------------------------------------------------------]
	//[ Protected definitions                                 ]
	//[-------------------------------------------------------]
	protected:
		typedef std::vector<Device*> Devices;


	//[-------------------------------------------------------]
	//[ Protected data                                        ]
	//[-------------------------------------------------------]
	protected:
		InputManager& mInputManager;	///< Owner input manager
		Devices		  m_lstDevices;		///< List of devices


	};

	/**
	*  @brief
	*    Device implementation base class
	*
	*  @remarks
	*    A device implementation class represents a concrete device interface implementation, such as a HID device
	*    or a Windows RawInput device. A logical device will use an implementation class to interface with the underlying
	*    system device, and concrete backend implementation will implement those interface for their specific systems.
	*/
	class DeviceImpl
	{


	//[-------------------------------------------------------]
	//[ Friends                                               ]
	//[-------------------------------------------------------]
	friend class Device;


	//[-------------------------------------------------------]
	//[ Public functions                                      ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor
		*/
		inline DeviceImpl() :
			mDeviceBackendType(DeviceBackendType::UNKNOWN),
			m_pDevice(nullptr),
			m_bDelete(true)
		{
			// Nothing here
		}

		/**
		*  @brief
		*    Destructor
		*/
		inline virtual ~DeviceImpl()
		{
			// Remove from device
			if (m_pDevice)
				m_pDevice->m_pImpl = nullptr;
		}

		/**
		*  @brief
		*    Get device backend type
		*
		*  @return
		*    Type of device backend
		*/
		inline DeviceBackendType getDeviceBackendType() const
		{
			return mDeviceBackendType;
		}

		/**
		*  @brief
		*    Get device
		*
		*  @return
		*    Device that owns this device implementation, can be a null pointer
		*/
		inline Device *GetDevice() const
		{
			return m_pDevice;
		}


	//[-------------------------------------------------------]
	//[ Protected functions                                   ]
	//[-------------------------------------------------------]
	protected:
		/**
		*  @brief
		*    Set device
		*
		*  @param[in] pDevice
		*    Device that owns this device implementation, can be a null pointer
		*/
		inline void SetDevice(Device *pDevice)
		{
			m_pDevice = pDevice;
		}


	//[-------------------------------------------------------]
	//[ Protected data                                        ]
	//[-------------------------------------------------------]
	protected:
		DeviceBackendType	 mDeviceBackendType;	///< Device backend type
		Device				*m_pDevice;				///< Device that owns to this device implementation, can be a null pointer
		bool				 m_bDelete;				///< If 'true', the device implementation will be destroyed automatically with the Device, else it must be destroyed manually


	};

	/**
	*  @brief
	*    An update device is a device backend that has an Update()-method which is called
	*    continuously. The backend is supposed to get the device state in this method
	*    and update the device accordingly.
	*/
	class UpdateDevice : public DeviceImpl
	{


	//[-------------------------------------------------------]
	//[ Public functions                                      ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor
		*/
		inline UpdateDevice()
		{
			// Set device backend type
			mDeviceBackendType = DeviceBackendType::UPDATE_DEVICE;
		}

		/**
		*  @brief
		*    Destructor
		*/
		inline virtual ~UpdateDevice() override
		{
			// Nothing here
		}


	//[-------------------------------------------------------]
	//[ Public virtual UpdateDevice functions                 ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Update device once per frame
		*
		*  @remarks
		*    This function should be used to get the current device states and update the device-object accordingly.
		*
		*  @note
		*    - The default implementation is empty
		*/
		inline virtual void Update()
		{
			// To be implemented in derived classes
		}


	};

	/**
	*  @brief
	*    Device connection class
	*
	*  @remarks
	*    A connection device is a device backend that uses e.g. a HID or Bluetooth connection to communicate
	*    directly with the input device (no use of HID features, only read/write commands) and expose an
	*    interface that can be used by the device class to use that connection.
	*/
	class ConnectionDevice : public DeviceImpl
	{


	//[-------------------------------------------------------]
	//[ Public definitions                                    ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Device type
		*/
		enum EDeviceType {
			DeviceTypeUnknown,	///< Unknown device type
			DeviceTypeHID,		///< HID device
			DeviceTypeBluetooth	///< Bluetooth device
		};


	//[-------------------------------------------------------]
	//[ Public signals                                        ]
	//[-------------------------------------------------------]
	public:
		Signal<> OnConnectSignal;		///< Emitted when device has been connected
		Signal<> OnDisconnectSignal;	///< Emitted when device has been disconnected
		Signal<> OnReadSignal;			///< Emitted when data has been read


	//[-------------------------------------------------------]
	//[ Public functions                                      ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor
		*/
		inline ConnectionDevice() :
			m_nDeviceType(DeviceTypeUnknown),
			m_pInputBuffer(nullptr),
			m_pOutputBuffer(nullptr),
			m_nInputReportSize(0),
			m_nOutputReportSize(0),
			m_pThread(nullptr),
			mMutex(nullptr),
			m_bThreadExit(false)
		{
			// Set device backend type
			mDeviceBackendType = DeviceBackendType::CONNECTION_DEVICE;
		}

		/**
		*  @brief
		*    Destructor
		*/
		inline virtual ~ConnectionDevice() override
		{
			// Stop thread
			StopThread();
		}

		/**
		*  @brief
		*    Get device type
		*
		*  @return
		*    Device type
		*/
		inline EDeviceType GetDeviceType() const
		{
			return m_nDeviceType;
		}

		/**
		*  @brief
		*    Get input report size
		*
		*  @return
		*    Size of an input report in bytes (unique to each HID device)
		*/
		inline uint32_t GetInputReportSize() const
		{
			return m_nInputReportSize;
		}

		/**
		*  @brief
		*    Set input report size
		*
		*  @param[in] nSize
		*    Size of an input report in bytes (unique to each HID device)
		*/
		inline void SetInputReportSize(uint32_t nSize)
		{
			m_nInputReportSize = static_cast<uint16_t>(nSize);
		}

		/**
		*  @brief
		*    Get output report size
		*
		*  @return
		*    Size of an output report in bytes
		*/
		inline uint32_t GetOutputReportSize() const
		{
			return m_nOutputReportSize;
		}

		/**
		*  @brief
		*    Set output report size
		*
		*  @param[in] nSize
		*    Size of an output report in bytes (unique to each HID device)
		*/
		inline void SetOutputReportSize(uint32_t nSize)
		{
			m_nOutputReportSize = static_cast<uint16_t>(nSize);
		}

		/**
		*  @brief
		*    Get input buffer
		*
		*  @return
		*    Input buffer (can be a null pointer if the device is not open), do not destroy the returned buffer!
		*/
		inline uint8_t *GetInputBuffer() const
		{
			return m_pInputBuffer;
		}

		/**
		*  @brief
		*    Get output buffer
		*
		*  @return
		*    Output buffer (can be a null pointer if the device is not open), do not destroy the returned buffer!
		*/
		inline uint8_t *GetOutputBuffer() const
		{
			return m_pOutputBuffer;
		}


	//[-------------------------------------------------------]
	//[ Public virtual ConnectionDevice functions             ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Open device connection
		*
		*  @param[in] nOutputPort
		*    Bluetooth port for output channel
		*  @param[in] nInputPort
		*    Bluetooth port for input channel
		*
		*  @return
		*    'true' if all went fine, else 'false'
		*
		*  @note
		*    - The default implementation is empty
		*    - If you are using a "HIDDevice", the output and input ports will be ignored
		*/
		inline virtual bool Open(MAYBE_UNUSED uint16_t nOutputPort = 0, MAYBE_UNUSED uint16_t nInputPort = 0)
		{
			// To be implemented in derived classes
			// Please use InitThread() to start the read thread after successful connection
			// And don't forget to emit "OnConnectSignal" please

			// Not implemented here...
			return false;
		}

		/**
		*  @brief
		*    Close device connection
		*
		*  @return
		*    'true' if all went fine, else 'false'
		*
		*  @note
		*    - The default implementation is empty
		*/
		inline virtual bool Close()
		{
			// To be implemented in derived classes
			// Please use StopThread() to stop the read thread after closing the connection
			// And don't forget to emit "OnDisconnectSignal" please

			// Not implemented here...
			return false;
		}

		/**
		*  @brief
		*    Check if the device is open
		*
		*  @return
		*    'true' if device is open, else 'false'
		*
		*  @note
		*    - The default implementation is empty
		*/
		inline virtual bool IsOpen() const
		{
			// To be implemented in derived classes

			// Not implemented here...
			return false;
		}

		/**
		*  @brief
		*    Read from device
		*
		*  @param[out] pBuffer
		*    Buffer that will receive the data, must be valid and at least "nSize"-bytes long!
		*  @param[in]  nSize
		*    Buffer size in bytes
		*
		*  @return
		*    'true' if all went fine, else 'false'
		*
		*  @note
		*    - The default implementation is empty
		*/
		inline virtual bool Read(uint8_t*, uint32_t)
		{
			// To be implemented in derived classes
			// Please call LockCriticalSection() before and UnlockCriticalSection() after the read operation
			// And don't forget to emit "OnReadSignal" please

			// Not implemented here...
			return false;
		}

		/**
		*  @brief
		*    Write to device
		*
		*  @param[in] pBuffer
		*    Buffer containing the data, must be valid and at least "nSize"-bytes long!
		*  @param[in] nSize
		*    Buffer size in bytes
		*
		*  @return
		*    'true' if all went fine, else 'false'
		*
		*  @note
		*    - The default implementation is empty
		*/
		inline virtual bool Write(const uint8_t*, uint32_t)
		{
			// To be implemented in derived classes
			// Please call LockCriticalSection() before and UnlockCriticalSection() after the write operation

			// Not implemented here...
			return false;
		}


	//[-------------------------------------------------------]
	//[ Protected functions                                   ]
	//[-------------------------------------------------------]
	protected:
		/**
		*  @brief
		*    Initialize and start thread for read/write operations
		*
		*  @remarks
		*    Creates and starts the read thread and initializes "mMutex", which should be
		*    used by derived classes in their Read() and Write() functions.
		*/
		void InitThread();

		/**
		*  @brief
		*    Stop thread for read/write operations
		*/
		void StopThread();

		/**
		*  @brief
		*    Lock read/write critical section
		*
		*  @remarks
		*    This should be used by derived classes inside their Read() and Write() functions!
		*/
		void LockCriticalSection();

		/**
		*  @brief
		*    Unlock read/write critical section
		*
		*  @remarks
		*    This should be used by derived classes inside their Read() and Write() functions!
		*/
		void UnlockCriticalSection();


	//[-------------------------------------------------------]
	//[ Private static functions                              ]
	//[-------------------------------------------------------]
	private:
		/**
		*  @brief
		*    Device update thread function
		*
		*  @param[in] pData
		*    Pointer to this ConnectionDevice instance, always valid!
		*/
		static int ReadThread(void *pData);


	//[-------------------------------------------------------]
	//[ Protected data                                        ]
	//[-------------------------------------------------------]
	protected:
		// Device type
		EDeviceType m_nDeviceType;	///< Device type

		// Input and output buffers
		uint8_t	 *m_pInputBuffer;		///< Input report buffer, can be a null pointer
		uint8_t	 *m_pOutputBuffer;		///< Output report buffer, can be a null pointer
		uint16_t  m_nInputReportSize;	///< Size of input report in bytes
		uint16_t  m_nOutputReportSize;	///< Size of output report in bytes

		// Read thread
		std::thread	*m_pThread;		///< Update thread, can be a null pointer
		std::mutex	*mMutex;		///< Update mutex, can be a null pointer
		bool		 m_bThreadExit;	///< Flag to exit the thread


	};








	//[-------------------------------------------------------]
	//[ HID backend only interfaces                           ]
	//[-------------------------------------------------------]
	//[-------------------------------------------------------]
	//[ Global definitions                                    ]
	//[-------------------------------------------------------]
	// Usage Page: Generic (e.g. Values, Axes)
	static constexpr int UsagePageGeneric	= 0x01;
		static constexpr int UsageJoystick				= 0x04;
		static constexpr int UsageGamepad				= 0x05;
		static constexpr int UsageMultiAxisController	= 0x08;
		static constexpr int UsageX						= 0x30;
		static constexpr int UsageY						= 0x31;
		static constexpr int UsageZ						= 0x32;
		static constexpr int UsageRX					= 0x33;
		static constexpr int UsageRY					= 0x34;
		static constexpr int UsageRZ					= 0x35;
		static constexpr int UsageSlider				= 0x36;
		static constexpr int UsageDial					= 0x37;
		static constexpr int UsageWheel					= 0x38;
		static constexpr int UsageHat					= 0x39;


	// Usage Page: Buttons
	static constexpr int UsagePageButtons	= 0x09;
		static constexpr int UsageNoButton				= 0x00;
		static constexpr int UsageButton1				= 0x01;
		static constexpr int UsageButton2				= 0x02;
		static constexpr int UsageButton3				= 0x03;
		static constexpr int UsageButton4				= 0x04;
		static constexpr int UsageButton5				= 0x05;
		static constexpr int UsageButton6				= 0x06;
		static constexpr int UsageButton7				= 0x07;
		static constexpr int UsageButton8				= 0x08;


	// Usage Page: LED (used for rumble on some gamepads)
	static constexpr int UsagePageLED		= 0x08;
		static constexpr int UsageSlowBlinkOnTime		= 0x43;
		static constexpr int UsageSlowBlinkOffTime		= 0x44;
		static constexpr int UsageFastBlinkOnTime		= 0x45;
		static constexpr int UsageFastBlinkOffTime		= 0x46;


	//[-------------------------------------------------------]
	//[ Forward declarations                                  ]
	//[-------------------------------------------------------]
	class HID;
	class HIDImpl;
	class HIDDevice;


	//[-------------------------------------------------------]
	//[ Classes                                               ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Capability of a HID device
	*/
	class HIDCapability final
	{


	//[-------------------------------------------------------]
	//[ Public functions                                      ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Default constructor
		*/
		HIDCapability() :
			m_nReportID(0),
			m_nLinkCollection(0),
			m_nUsagePage(0),
			m_nUsage(0),
			m_nUsageMin(0),
			m_nUsageMax(0),
			m_nDataIndex(0),
			m_nDataIndexMin(0),
			m_nDataIndexMax(0),
			m_nBitSize(0),
			m_nLogicalMin(0),
			m_nLogicalMax(0),
			m_nPhysicalMin(0),
			m_nPhysicalMax(0),
			m_nValue(0)
		{
			// Nothing here
		}

		/**
		*  @brief
		*    Copy constructor
		*
		*  @param[in] cSource
		*    Capability to copy from
		*/
		explicit HIDCapability(const HIDCapability &cSource) :
			m_nReportID(cSource.m_nReportID),
			m_nLinkCollection(cSource.m_nLinkCollection),
			m_nUsagePage(cSource.m_nUsagePage),
			m_nUsage(cSource.m_nUsage),
			m_nUsageMin(cSource.m_nUsageMin),
			m_nUsageMax(cSource.m_nUsageMax),
			m_nDataIndex(cSource.m_nDataIndex),
			m_nDataIndexMin(cSource.m_nDataIndexMin),
			m_nDataIndexMax(cSource.m_nDataIndexMax),
			m_nBitSize(cSource.m_nBitSize),
			m_nLogicalMin(cSource.m_nLogicalMin),
			m_nLogicalMax(cSource.m_nLogicalMax),
			m_nPhysicalMin(cSource.m_nPhysicalMin),
			m_nPhysicalMax(cSource.m_nPhysicalMax),
			m_nValue(cSource.m_nValue)
		{
			// Nothing here
		}

		/**
		*  @brief
		*    Destructor
		*/
		~HIDCapability()
		{
			// Nothing here
		}

		/**
		*  @brief
		*    Assignment operator
		*
		*  @param[in] cSource
		*    Capability to copy from
		*
		*  @return
		*    Reference to this HIDCapability
		*/
		HIDCapability &operator =(const HIDCapability &cSource)
		{
			m_nReportID			= cSource.m_nReportID;
			m_nLinkCollection	= cSource.m_nLinkCollection;
			m_nUsagePage		= cSource.m_nUsagePage;
			m_nUsage			= cSource.m_nUsage;
			m_nUsageMin			= cSource.m_nUsageMin;
			m_nUsageMax			= cSource.m_nUsageMax;
			m_nDataIndex		= cSource.m_nDataIndex;
			m_nDataIndexMin		= cSource.m_nDataIndexMin;
			m_nDataIndexMax		= cSource.m_nDataIndexMax;
			m_nBitSize			= cSource.m_nBitSize;
			m_nLogicalMin		= cSource.m_nLogicalMin;
			m_nLogicalMax		= cSource.m_nLogicalMax;
			m_nPhysicalMin		= cSource.m_nPhysicalMin;
			m_nPhysicalMax		= cSource.m_nPhysicalMax;
			m_nValue			= cSource.m_nValue;
			return *this;
		}

		/**
		*  @brief
		*    Comparison operator
		*
		*  @param[in] cSource
		*    Capability to compare with
		*
		*  @return
		*    'true' if both are equal, else 'false'
		*/
		bool operator ==(const HIDCapability &cSource) const
		{
			return (m_nReportID	== cSource.m_nReportID &&  m_nLinkCollection == cSource.m_nLinkCollection &&
					m_nUsagePage == cSource.m_nUsagePage && m_nUsage == cSource.m_nUsage &&
					m_nUsageMin == cSource.m_nUsageMin && m_nUsageMax == cSource.m_nUsageMax &&
					m_nDataIndex == cSource.m_nDataIndex && m_nDataIndexMin == cSource.m_nDataIndexMin && m_nDataIndexMax == cSource.m_nDataIndexMax &&
					m_nBitSize == cSource.m_nBitSize && m_nLogicalMin == cSource.m_nLogicalMin &&
					m_nLogicalMax == cSource.m_nLogicalMax && m_nPhysicalMin == cSource.m_nPhysicalMin &&
					m_nPhysicalMax == cSource.m_nPhysicalMax && m_nValue == cSource.m_nValue);
		}


	//[-------------------------------------------------------]
	//[ Public data                                           ]
	//[-------------------------------------------------------]
	public:
		// Capability info
		uint8_t  m_nReportID;		///< Report ID
		uint16_t m_nLinkCollection;	///< Link collection
		uint16_t m_nUsagePage;		///< Usage page ID
		uint16_t m_nUsage;			///< Usage ID
		uint16_t m_nUsageMin;		///< Usage ID minimum
		uint16_t m_nUsageMax;		///< Usage ID maximum
		uint16_t m_nDataIndex;		///< Data index (index in input report, not byte index)
		uint16_t m_nDataIndexMin;	///< Data index minimum
		uint16_t m_nDataIndexMax;	///< Data index maximum
		uint16_t m_nBitSize;		///< Size of data in bits
		uint16_t m_nLogicalMin;		///< Logical minimum value (e.g. for axes)
		uint16_t m_nLogicalMax;		///< Logical maximum value (e.g. for axes)
		uint16_t m_nPhysicalMin;	///< Physical minimum value (e.g. for axes)
		uint16_t m_nPhysicalMax;	///< Physical maximum value (e.g. for axes)

		// Current data value
		uint32_t m_nValue;	///< Current value of input data


	};

	/**
	*  @brief
	*    Information about a HID device
	*/
	class HIDDevice : public ConnectionDevice
	{


	//[-------------------------------------------------------]
	//[ Friends                                               ]
	//[-------------------------------------------------------]
	friend class HID;


	//[-------------------------------------------------------]
	//[ Public functions                                      ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Default constructor
		*/
		HIDDevice() :
			// Device data
			m_nVendor(0),
			m_nProduct(0),
			// Device capabilities
			m_nUsagePage(0),
			m_nUsage(0),
			m_nFeatureReportByteLength(0),
			m_nNumberLinkCollectionNodes(0),
			m_nNumberInputButtonCaps(0),
			m_nNumberInputValueCaps(0),
			m_nNumberInputDataIndices(0),
			m_nNumberOutputButtonCaps(0),
			m_nNumberOutputValueCaps(0),
			m_nNumberOutputDataIndices(0),
			m_nNumberFeatureButtonCaps(0),
			m_nNumberFeatureValueCaps(0),
			m_nNumberFeatureDataIndices(0)
		{
			// Set device backend type
			mDeviceBackendType = DeviceBackendType::HID;

			// Set device type
			m_nDeviceType = ConnectionDevice::DeviceTypeHID;
		}

		/**
		*  @brief
		*    Copy constructor
		*
		*  @param[in] cSource
		*    HIDDevice to copy from
		*/
		explicit HIDDevice(const HIDDevice &cSource) :
			// Device data
			m_sName(cSource.m_sName),
			m_nVendor(cSource.m_nVendor),
			m_nProduct(cSource.m_nProduct),
			// Device capabilities
			m_nUsagePage(cSource.m_nUsagePage),
			m_nUsage(cSource.m_nUsage),
			m_nFeatureReportByteLength(cSource.m_nFeatureReportByteLength),
			m_nNumberLinkCollectionNodes(cSource.m_nNumberLinkCollectionNodes),
			m_nNumberInputButtonCaps(cSource.m_nNumberInputButtonCaps),
			m_nNumberInputValueCaps(cSource.m_nNumberInputValueCaps),
			m_nNumberInputDataIndices(cSource.m_nNumberInputDataIndices),
			m_nNumberOutputButtonCaps(cSource.m_nNumberOutputButtonCaps),
			m_nNumberOutputValueCaps(cSource.m_nNumberOutputValueCaps),
			m_nNumberOutputDataIndices(cSource.m_nNumberOutputDataIndices),
			m_nNumberFeatureButtonCaps(cSource.m_nNumberFeatureButtonCaps),
			m_nNumberFeatureValueCaps(cSource.m_nNumberFeatureValueCaps),
			m_nNumberFeatureDataIndices(cSource.m_nNumberFeatureDataIndices),
			// Controls
			m_lstInputButtons(cSource.m_lstInputButtons),
			m_lstInputValues(cSource.m_lstInputValues),
			m_lstOutputValues(cSource.m_lstOutputValues)
		{
			// Set device type
			m_nDeviceType = ConnectionDevice::DeviceTypeHID;

			// Set input and output report sizes
			m_nInputReportSize  = cSource.m_nInputReportSize;
			m_nOutputReportSize = cSource.m_nOutputReportSize;
		}

		/**
		*  @brief
		*    Destructor
		*/
		inline virtual ~HIDDevice() override
		{
			// Nothing here
		}

		/**
		*  @brief
		*    Assignment operator
		*
		*  @param[in] cSource
		*    HIDDevice to copy from
		*
		*  @return
		*    Reference to this HIDDevice
		*/
		HIDDevice &operator =(const HIDDevice &cSource)
		{
			// Device data
			m_sName							= cSource.m_sName;
			m_nVendor						= cSource.m_nVendor;
			m_nProduct						= cSource.m_nProduct;

			// Device capabilities
			m_nUsagePage					= cSource.m_nUsagePage;
			m_nUsage						= cSource.m_nUsage;
			m_nInputReportSize				= cSource.m_nInputReportSize;
			m_nOutputReportSize				= cSource.m_nOutputReportSize;
			m_nFeatureReportByteLength		= cSource.m_nFeatureReportByteLength;
			m_nNumberLinkCollectionNodes	= cSource.m_nNumberLinkCollectionNodes;
			m_nNumberInputButtonCaps		= cSource.m_nNumberInputButtonCaps;
			m_nNumberInputValueCaps			= cSource.m_nNumberInputValueCaps;
			m_nNumberInputDataIndices		= cSource.m_nNumberInputDataIndices;
			m_nNumberOutputButtonCaps		= cSource.m_nNumberOutputButtonCaps;
			m_nNumberOutputValueCaps		= cSource.m_nNumberOutputValueCaps;
			m_nNumberOutputDataIndices		= cSource.m_nNumberOutputDataIndices;
			m_nNumberFeatureButtonCaps		= cSource.m_nNumberFeatureButtonCaps;
			m_nNumberFeatureValueCaps		= cSource.m_nNumberFeatureValueCaps;
			m_nNumberFeatureDataIndices		= cSource.m_nNumberFeatureDataIndices;

			// Controls
			m_lstInputButtons				= cSource.m_lstInputButtons;
			m_lstInputValues				= cSource.m_lstInputValues;
			m_lstOutputValues				= cSource.m_lstOutputValues;

			// Done
			return *this;
		}

		/**
		*  @brief
		*    Comparison operator
		*
		*  @param[in] cSource
		*    HIDDevice to compare with
		*
		*  @return
		*    'true', if both are equal, else 'false'
		*/
		bool operator ==(const HIDDevice &cSource)
		{
					// Device data
			return (m_nVendor						== cSource.m_nVendor &&
					m_nProduct						== cSource.m_nProduct &&
					// Device capabilities
					m_nUsagePage					== cSource.m_nUsagePage &&
					m_nUsage						== cSource.m_nUsage &&
					m_nInputReportSize				== cSource.m_nInputReportSize &&
					m_nOutputReportSize				== cSource.m_nOutputReportSize &&
					m_nFeatureReportByteLength		== cSource.m_nFeatureReportByteLength &&
					m_nNumberLinkCollectionNodes	== cSource.m_nNumberLinkCollectionNodes &&
					m_nNumberInputButtonCaps		== cSource.m_nNumberInputButtonCaps &&
					m_nNumberInputValueCaps			== cSource.m_nNumberInputValueCaps &&
					m_nNumberInputDataIndices		== cSource.m_nNumberInputDataIndices &&
					m_nNumberOutputButtonCaps		== cSource.m_nNumberOutputButtonCaps &&
					m_nNumberOutputValueCaps		== cSource.m_nNumberOutputValueCaps &&
					m_nNumberOutputDataIndices		== cSource.m_nNumberOutputDataIndices &&
					m_nNumberFeatureButtonCaps		== cSource.m_nNumberFeatureButtonCaps &&
					m_nNumberFeatureValueCaps		== cSource.m_nNumberFeatureValueCaps &&
					m_nNumberFeatureDataIndices		== cSource.m_nNumberFeatureDataIndices &&
					// Controls
					m_lstInputButtons				== cSource.m_lstInputButtons &&
					m_lstInputValues				== cSource.m_lstInputValues &&
					m_lstOutputValues				== cSource.m_lstOutputValues);
		}

		/**
		*  @brief
		*    Get device name
		*
		*  @return
		*    Device name
		*/
		inline const std::string& GetName() const
		{
			return m_sName;
		}

		/**
		*  @brief
		*    Get vendor
		*
		*  @return
		*    Vendor ID
		*/
		inline uint32_t GetVendor() const
		{
			return m_nVendor;
		}

		/**
		*  @brief
		*    Get product
		*
		*  @return
		*    Product ID
		*/
		inline uint32_t GetProduct() const
		{
			return m_nProduct;
		}

		/**
		*  @brief
		*    Get usage
		*
		*  @return
		*    Usage ID
		*/
		inline uint16_t GetUsage() const
		{
			return m_nUsage;
		}

		/**
		*  @brief
		*    Get usage page
		*
		*  @return
		*    Usage page ID
		*/
		inline uint16_t GetUsagePage() const
		{
			return m_nUsagePage;
		}

		/**
		*  @brief
		*    Get input button controls
		*
		*  @return
		*    List of input button controls
		*/
		inline const std::vector<HIDCapability> &GetInputButtons() const
		{
			return m_lstInputButtons;
		}

		/**
		*  @brief
		*    Get input value controls
		*
		*  @return
		*    List of input value controls
		*/
		inline const std::vector<HIDCapability> &GetInputValues() const
		{
			return m_lstInputValues;
		}

		/**
		*  @brief
		*    Get output value controls
		*
		*  @return
		*    List of output value controls
		*/
		inline std::vector<HIDCapability> &GetOutputValues()
		{
			return m_lstOutputValues;
		}

		/**
		*  @brief
		*    Parse input report
		*
		*  @param[in] pInputReport
		*    HID input report
		*  @param[in] nSize
		*    Size of input report (in bytes)
		*/
		inline void ParseInputReport(const uint8_t *pInputReport, uint32_t nSize)
		{
			// Call system specific backend function to parse input report data
			ParseInputReportData(pInputReport, nSize);

			// TODO(co)
			// This function should contain an own implementation to parse the input report rather than
			// relying on system-specific APIs. However, my attempt to implement one failed because the data indices
			// seem to be awkward and I can't see how to parse the data items correctly. So, let's stick with this
			// for a while, until someone with time and knowledge takes the challenge :-)
		}


	//[-------------------------------------------------------]
	//[ Public virtual HIDDevice functions                    ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Send HID output report
		*
		*  @remarks
		*    This function will send an output report including the current values of all
		*    output values of the device. Use this to update e.g. LEDs or rumble effects.
		*/
		inline virtual void SendOutputReport()
		{
			// Call system specific backend function
			SendOutputReportData();

			// TODO(co) This function should contain an own implementation rather than relying on system-specific APIs
		}


	//[-------------------------------------------------------]
	//[ Protected virtual HIDDevice functions                 ]
	//[-------------------------------------------------------]
	protected:
		/**
		*  @brief
		*    Parse HID input report
		*
		*  @param[in] pInputReport
		*    HID input report
		*  @param[in] nSize
		*    Size of input report (in bytes)
		*
		*  @remarks
		*    Implement this function to parse an input report using system specific
		*    functions (e.g. Windows HID API). This method will be called automatically
		*    by ParseInputReport.
		*/
		inline virtual void ParseInputReportData(const uint8_t*, uint32_t)
		{
			// To be implemented in derived classes
		}

		/**
		*  @brief
		*    Send HID output report
		*
		*  @remarks
		*    Implement this function to create an output report including the values of all
		*    output values using system specific functions (e.g. Windows HID API). This method
		*    will be called automatically by SendOutputReport.
		*/
		inline virtual void SendOutputReportData()
		{
			// To be implemented in derived classes
		}


	//[-------------------------------------------------------]
	//[ Protected data                                        ]
	//[-------------------------------------------------------]
	protected:
		// Device data
		std::string	m_sName;	///< Device name
		uint32_t	m_nVendor;	///< Vendor ID
		uint32_t	m_nProduct;	///< Product ID

		// Device capabilities
		uint16_t m_nUsagePage;					///< Device usage page
		uint16_t m_nUsage;						///< Device usage
		uint16_t m_nFeatureReportByteLength;	///< Feature report byte length
		uint16_t m_nNumberLinkCollectionNodes;	///< Number of link collection nodes
		uint16_t m_nNumberInputButtonCaps;		///< Number of input buttons
		uint16_t m_nNumberInputValueCaps;		///< Number of input values
		uint16_t m_nNumberInputDataIndices;		///< Number of input data indices
		uint16_t m_nNumberOutputButtonCaps;		///< Number of output buttons
		uint16_t m_nNumberOutputValueCaps;		///< Number of output values
		uint16_t m_nNumberOutputDataIndices;	///< Number of output data indices
		uint16_t m_nNumberFeatureButtonCaps;	///< Number of feature buttons
		uint16_t m_nNumberFeatureValueCaps;		///< Number of feature values
		uint16_t m_nNumberFeatureDataIndices;	///< Number of feature data indices

		// Controls
		std::vector<HIDCapability> m_lstInputButtons;	///< List of input buttons
		std::vector<HIDCapability> m_lstInputValues;	///< List of input values
		std::vector<HIDCapability> m_lstOutputValues;	///< List of output values


	};

	/**
	*  @brief
	*    Abstract base class for platform specific HID implementations
	*/
	class HIDImpl
	{


	//[-------------------------------------------------------]
	//[ Friends                                               ]
	//[-------------------------------------------------------]
	friend class HID;


	//[-------------------------------------------------------]
	//[ Protected functions                                   ]
	//[-------------------------------------------------------]
	protected:
		/**
		*  @brief
		*    Default constructor
		*/
		HIDImpl()
		{
			// Nothing here
		}

		/**
		*  @brief
		*    Destructor
		*/
		virtual ~HIDImpl()
		{
			// Nothing here
		}


	//[-------------------------------------------------------]
	//[ Protected virtual HIDImpl functions                   ]
	//[-------------------------------------------------------]
	protected:
		/**
		*  @brief
		*    Enumerate HID devices
		*
		*  @param[out] lstDevices
		*    List that will receive the devices
		*/
		virtual void EnumerateDevices(std::vector<HIDDevice*> &lstDevices) = 0;


	};

	/**
	*  @brief
	*    Class for accessing HID (Human Interface Device) devices
	*/
	class HID final
	{


	//[-------------------------------------------------------]
	//[ Friends                                               ]
	//[-------------------------------------------------------]
	friend class HIDProvider;


	//[-------------------------------------------------------]
	//[ Public functions                                      ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Detect available HID devices
		*/
		void DetectDevices()
		{
			// Clear list
			Clear();

			// Check HID implementation
			if (m_pHIDImpl) {
				// Enumerate devices
				m_pHIDImpl->EnumerateDevices(m_lstDevices);
			}
		}

		/**
		*  @brief
		*    Get list of available HID devices
		*
		*  @return
		*    Device list
		*/
		inline const std::vector<HIDDevice*> &GetDevices() const
		{
			return m_lstDevices;
		}


	//[-------------------------------------------------------]
	//[ Private functions                                     ]
	//[-------------------------------------------------------]
	private:
		/**
		*  @brief
		*    Default constructor
		*/
		HID();

		explicit HID(const HID& source) = delete;

		/**
		*  @brief
		*    Destructor
		*/
		inline ~HID()
		{
			// Clear devices
			Clear();

			// Delete HID implementation
			if (m_pHIDImpl)
				delete m_pHIDImpl;
		}

		HID& operator= (const HID& source) = delete;

		/**
		*  @brief
		*    Remove all devices
		*/
		inline void Clear()
		{
			// Destroy devices
			m_lstDevices.clear();
		}


	//[-------------------------------------------------------]
	//[ Private data                                          ]
	//[-------------------------------------------------------]
	private:
		HIDImpl					*m_pHIDImpl;	///< Platform specific HID implementation
		std::vector<HIDDevice*>  m_lstDevices;	///< List of devices


	};

	/**
	*  @brief
	*    Input provider for HID devices
	*/
	class HIDProvider : public Provider
	{


	//[-------------------------------------------------------]
	//[ Public functions                                      ]
	//[-------------------------------------------------------]
	public:
		HIDProvider() = delete;

		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] inputManager
		*    Owner input manager
		*/
		inline explicit HIDProvider(InputManager& inputManager) :
			Provider(inputManager),
			mHid(new HID())
		{
			// Nothing here
		}

		/**
		*  @brief
		*    Destructor
		*/
		inline virtual ~HIDProvider() override
		{
			delete mHid;
		}


	//[-------------------------------------------------------]
	//[ Private virtual Provider functions                    ]
	//[-------------------------------------------------------]
	private:
		virtual void QueryDevices() override
		{
			// Device counter
			int nSpaceMouse = 0;
			int nWiiMote	= 0;
			// TODO(co) HIDJoystick not finished yet
			int nJoystick	= 0;

			// Get list of HID-devices
			const std::vector<HIDDevice*> &lstDevices = mHid->GetDevices();
			for (uint32_t i=0; i<lstDevices.size(); i++) {
				// Get device
				HIDDevice *pDevice = lstDevices[i];

				// Get vendor and product ID
				const uint32_t nVendor	  = pDevice->GetVendor();
				const uint32_t nProduct	  = pDevice->GetProduct();
				const uint32_t nUsage	  = pDevice->GetUsage();
				const uint32_t nUsagePage = pDevice->GetUsagePage();

				// Check device type

				// SpaceMouse
				if (nVendor == SpaceMouse::VendorID && nUsagePage == UsagePageGeneric && nUsage == UsageMultiAxisController) {
					std::string sName = std::string("SpaceMouse") + std::to_string(nSpaceMouse);
					nSpaceMouse++;
					if (!CheckDevice(sName))
						AddDevice(sName, new SpaceMouse(mInputManager, sName, pDevice));

				// WiiMote
				} else if (nVendor == WiiMote::VendorID && nProduct == WiiMote::ProductID) {
					std::string sName = std::string("WiiMote") + std::to_string(nWiiMote);
					nWiiMote++;
					if (!CheckDevice(sName))
						AddDevice(sName, new WiiMote(mInputManager, sName, pDevice));

				// Joystick or Joypad
				} else if (nUsagePage == UsagePageGeneric && (nUsage == UsageJoystick || nUsage == UsageGamepad)) {
					std::string sName = std::string("Joystick") + std::to_string(nJoystick);
					nJoystick++;
					if (!CheckDevice(sName))
						AddDevice(sName, new Joystick(mInputManager, sName, pDevice));
				}
			}
		}


	//[-------------------------------------------------------]
	//[ Private functions                                     ]
	//[-------------------------------------------------------]
	private:
		HIDProvider(const HIDProvider &cSource) = delete;
		HIDProvider &operator =(const HIDProvider &cSource) = delete;


	//[-------------------------------------------------------]
	//[ Private data                                          ]
	//[-------------------------------------------------------]
	private:
		HID* mHid;


	};








	//[-------------------------------------------------------]
	//[ PLInput::Control                                      ]
	//[-------------------------------------------------------]
	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	// TODO(co) We can probably inline this
	Control::Control(Controller& controller, ControlType controlType, const std::string &sName, const std::string &sDescription) :
		mController(controller),
		mControlType(controlType),
		mName(sName),
		mDescription(sDescription)
	{
		// Add control to controller
		mController.AddControl(this);
	}

	// TODO(co) We can probably inline this
	Control::~Control()
	{
		// Inform input manager
		mController.GetInputManager().RemoveControl(this);
	}


	//[-------------------------------------------------------]
	//[ Protected methods                                     ]
	//[-------------------------------------------------------]
	void Control::informUpdate()
	{
		// Inform input manager
		mController.GetInputManager().UpdateControl(this);
	}




	//[-------------------------------------------------------]
	//[ PLInput::Connection                                   ]
	//[-------------------------------------------------------]
	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	Connection::Connection(Control& inputControl, Control& outputControl, float scale) :
		mInputControl(inputControl),
		mOutputControl(outputControl),
		mValid(false),
		mScale(scale)
	{
		// Perform sanity checks
		// -> Check if both controls are valid and of the same control type
		// -> Check that the controls are either both input or both output controls
		// -> Check that the output control does not belong to a device
		if (&mInputControl != &mOutputControl && mInputControl.getControlType() == mOutputControl.getControlType() && mInputControl.isInputControl() == mOutputControl.isInputControl() && mOutputControl.getController().getControllerType() != ControllerType::DEVICE)
		{
			// Connection is valid (can be changed by derived classes that have additional tests)
			mValid = true;
		}
		else
		{
			assert(false && "Invalid input connection");
		}
	}

	void Connection::passValue()
	{
		// Check if connection is valid
		if (mValid)
		{
			// Pass value
			switch (mInputControl.getControlType())
			{
				case ControlType::BUTTON:
					static_cast<Button&>(mOutputControl).setPressed(static_cast<Button&>(mInputControl).isPressed());
					break;

				case ControlType::AXIS:
				{
					const Axis& inputAxisControl = static_cast<Axis&>(mInputControl);
					static_cast<Axis&>(mOutputControl).setValue(inputAxisControl.getValue() * mScale, inputAxisControl.isRelativeValue());
					break;
				}

				case ControlType::UNKNOWN:
				case ControlType::LED:
				case ControlType::EFFECT:
				default:
					// There's nothing to pass on
					break;
			}
		}
	}

	void Connection::passValueBackwards()
	{
		// Check if connection is valid
		if (mValid)
		{
			// Pass value
			switch (mOutputControl.getControlType())
			{
				case ControlType::LED:
					static_cast<LED&>(mInputControl).setLedStates(static_cast<LED&>(mOutputControl).getLedStates());
					break;

				case ControlType::EFFECT:
					static_cast<Effect&>(mInputControl).setValue(static_cast<Effect&>(mOutputControl).getValue());
					break;

				case ControlType::UNKNOWN:
				case ControlType::BUTTON:
				case ControlType::AXIS:
				default:
					// There's nothing to pass backwards
					break;
			}
		}
	}




	// TODO(co) Below this: Cleanup
	//[-------------------------------------------------------]
	//[ PLInput::Controller                                   ]
	//[-------------------------------------------------------]
	//[-------------------------------------------------------]
	//[ Public functions                                      ]
	//[-------------------------------------------------------]
	void Controller::Connect(const std::string& outputControlName, Control& inputControl, float fScale)
	{
		// Get output control
		Control *pOutput = GetControl(outputControlName);
		if (nullptr != pOutput && (&inputControl != pOutput) && (&inputControl.getController() != &pOutput->getController()))
		{
			// Create connection
			Connection *pConnection = new Connection(inputControl, *pOutput, fScale);
			if (pConnection->isValid())
			{
				// Add connection to both controllers
				inputControl.getController().AddConnection(pConnection);
				AddConnection(pConnection);
			}
			else
			{
				// Connection is invalid!
				// TODO(co) Log entry
				delete pConnection;
			}
		}
	}

	void Controller::ConnectAll(Controller *pController, const std::string &sPrefixOut, const std::string &sPrefixIn)
	{
		if (pController) {
			// Get all controls of input controller
			const Controls &lstControls = pController->GetControls();
			for (uint32_t i=0; i<lstControls.size(); i++) {
				// Get control
				Control *pInput = lstControls[i];

				// Check if input control has the right prefix
				std::string sNameIn = pInput->getName();
				if (sPrefixIn.empty() || sNameIn.find(sPrefixIn) == std::string::npos) {
					// Get output name
					std::string sRoot = sNameIn.substr(0, sPrefixIn.size());
					std::string sNameOut = sPrefixOut + sRoot;

					// Try to create connection
					Connect(sNameOut, *pInput);
				}
			}
		}
	}

	void Controller::Disconnect(Connection *pConnection)
	{
		// Check connection
		if (pConnection && std::find(m_lstConnections.cbegin(), m_lstConnections.cend(), pConnection) != m_lstConnections.cend()) {
			// Get other controller
			Controller* controller = &pConnection->getInputControl().getController();
			if (controller == this)
			{
				controller = &pConnection->getOutputControl().getController();
			}

			// Remove connection from both controllers
			controller->RemoveConnection(pConnection);
			RemoveConnection(pConnection);

			// Destroy connection
			delete pConnection;
		}
	}


	//[-------------------------------------------------------]
	//[ Protected functions                                   ]
	//[-------------------------------------------------------]
	void Controller::AddControl(Control *pControl)
	{
		// Check if control is valid
		if (pControl) {
			// Add control to list
			m_lstControls.push_back(pControl);

			// Add control to hash map
			m_mapControls.emplace(pControl->getName(), pControl);
		}
	}

	void Controller::InformControl(Control *pControl)
	{
		// Check if controller is active and control is valid
		if (m_bActive && pControl) {
			// Set changed-flag
			m_bChanged = true;

			// Check if a button was hit
			if (pControl->getControlType() == ControlType::BUTTON && static_cast<Button*>(pControl)->isHit())
			{
				// Save key character
				m_nChar = static_cast<Button*>(pControl)->getCharacter();
			}

			// Check if an output value was changed
			if (pControl->getControlType() == ControlType::LED || pControl->getControlType() == ControlType::EFFECT)
			{
				// Update output control
				UpdateOutputControl(pControl);
			}

			// Check connections
			for (uint32_t i=0; i<m_lstConnections.size(); i++) {
				// Get connection
				Connection *pConnection = m_lstConnections[i];

				// Check 'direction' that we must take
				if (pControl->isInputControl() && (&pConnection->getInputControl() == pControl))
				{
					// Get the pointer to the controller that owns the output control
					// -> In case there's a controller, do only pass on the control event in case the controller is active
					if (pConnection->getOutputControl().getController().GetActive())
					{
						// Input control, pass from connection-input to connection-output
						pConnection->passValue();
					}
				}
				else if (!pControl->isInputControl() && (&pConnection->getOutputControl() == pControl))
				{
					// Get the pointer to the controller that owns the input control
					// -> In case there's a controller, do only pass on the control event in case the controller is active
					if (pConnection->getInputControl().getController().GetActive())
					{
						// Output control, pass backwards: from connection-output to connection-input
						pConnection->passValueBackwards();
					}
				}
			}
		}
	}

	void Controller::InitControlList(ControlType controlType) const
	{
		// Clear list
		if (ControlType::BUTTON == controlType)
		{
			m_lstButtons.clear();
		}
		else if (ControlType::AXIS == controlType)
		{
			m_lstAxes.clear();
		}

		// Loop over all controls
		for (uint32_t i=0; i<m_lstControls.size(); i++) {
			// Get control
			Control *pControl = m_lstControls[i];

			// Add control to appropriate list
			if (pControl->getControlType() == controlType)
			{
				if (ControlType::BUTTON == controlType)
				{
					m_lstButtons.push_back(static_cast<Button*>(pControl));
				}
				else if (ControlType::AXIS == controlType)
				{
					m_lstAxes.push_back(static_cast<Axis*>(pControl));
				}
			}
		}
	}

	void Controller::AddConnection(Connection *pConnection)
	{
		// Avoid duplicate connection entries
		if (pConnection && std::find(m_lstConnections.cbegin(), m_lstConnections.cend(), pConnection) == m_lstConnections.cend()) {
			// Add connection
			m_lstConnections.push_back(pConnection);
		}
	}

	void Controller::RemoveConnection(Connection *pConnection)
	{
		// Check connection
		if (nullptr != pConnection)
		{
			Connections::const_iterator iterator = std::find(m_lstConnections.cbegin(), m_lstConnections.cend(), pConnection);
			if (m_lstConnections.cend() != iterator)
			{
				// Remove connection
				m_lstConnections.erase(iterator);
			}
		}
	}




	//[-------------------------------------------------------]
	//[ PLInput::Device                                       ]
	//[-------------------------------------------------------]
	//[-------------------------------------------------------]
	//[ Public functions                                      ]
	//[-------------------------------------------------------]
	Device::Device(InputManager& inputManager, const std::string &sName, const std::string &sDescription, DeviceImpl *pImpl) :
		Controller(inputManager, ControllerType::DEVICE, sName, sDescription),
		m_pImpl(pImpl),
		m_bDeleteImpl(false)
	{
		// Set device
		if (m_pImpl) {
			m_pImpl->SetDevice(this);
			m_bDeleteImpl = m_pImpl->m_bDelete;
		}
	}

	Device::~Device()
	{
		// Destroy device implementation
		if (m_pImpl && m_bDeleteImpl)
		{
			m_pImpl->SetDevice(nullptr);
			delete m_pImpl;
		}
	}




	//[-------------------------------------------------------]
	//[ PLInput::Keyboard                                     ]
	//[-------------------------------------------------------]
	//[-------------------------------------------------------]
	//[ Public functions                                      ]
	//[-------------------------------------------------------]
	Keyboard::Keyboard(InputManager& inputManager, const std::string &sName, DeviceImpl *pImpl) :
		Device(inputManager, sName, "Keyboard input controller", pImpl),
		Backspace			(*this, "Backspace",			"Backspace",																	0x08),
		Tab					(*this, "Tab",					"Tabulator",																	0x09),
		Clear				(*this, "Clear",				"Clear (not available everywhere)",												0x00),
		Return				(*this, "Return",				"Return (often the same as \"Enter\")",											0x0D),
		Shift				(*this, "Shift",				"Shift",																		0x00),
		Control				(*this, "Control",				"Control (\"Ctrl\")",															0x00),
		Alt					(*this, "Alt",					"Alt",																			0x00),
		Pause				(*this, "Pause",				"Pause",																		0x00),
		CapsLock			(*this, "CapsLock",				"Caps lock",																	0x00),
		Escape				(*this, "Escape",				"Escape",																		0x1B),
		Space				(*this, "Space",				"Space",																		0x20),
		PageUp				(*this, "PageUp",				"Page up",																		0x00),
		PageDown			(*this, "PageDown",				"Page down",																	0x00),
		End					(*this, "End",					"End",																			0x00),
		Home				(*this, "Home",					"Home",																			0x00),
		Left				(*this, "Left",					"Left arrow",																	0x00),
		Up					(*this, "Up",					"Up arrow",																		0x00),
		Right				(*this, "Right",				"Right arrow",																	0x00),
		Down				(*this, "Down",					"Down arrow",																	0x00),
		Select				(*this, "Select",				"Select (not available everywhere)",											0x00),
		Execute				(*this, "Execute",				"Execute (not available everywhere)",											0x00),
		Print				(*this, "Print",				"Print screen",																	0x00),
		Insert				(*this, "Insert",				"Insert",																		0x00),
		Delete				(*this, "Delete",				"Delete",																		0x7F),
		Help				(*this, "Help",					"Help (not available everywhere)",												0x00),
		Key0				(*this, "0",					"0",																			 '0'),
		Key1				(*this, "1",					"1",																			 '1'),
		Key2				(*this, "2",					"2",																			 '2'),
		Key3				(*this, "3",					"3",																			 '3'),
		Key4				(*this, "4",					"4",																			 '4'),
		Key5				(*this, "5",					"5",																			 '5'),
		Key6				(*this, "6",					"6",																			 '6'),
		Key7				(*this, "7",					"7",																			 '7'),
		Key8				(*this, "8",					"8",																			 '8'),
		Key9				(*this, "9",					"9",																			 '9'),
		A					(*this, "A",					"A",																			 'a'),
		B					(*this, "B",					"B",																			 'b'),
		C					(*this, "C",					"C",																			 'c'),
		D					(*this, "D",					"D",																			 'd'),
		E					(*this, "E",					"E",																			 'e'),
		F					(*this, "F",					"F",																			 'f'),
		G					(*this, "G",					"G",																			 'g'),
		H					(*this, "H",					"H",																			 'h'),
		I					(*this, "I",					"I",																			 'i'),
		J					(*this, "J",					"J",																			 'j'),
		K					(*this, "K",					"K",																			 'k'),
		L					(*this, "L",					"L",																			 'l'),
		M					(*this, "M",					"M",																			 'm'),
		N					(*this, "N",					"N",																			 'n'),
		O					(*this, "O",					"O",																			 'o'),
		P					(*this, "P",					"P",																			 'p'),
		Q					(*this, "Q",					"Q",																			 'q'),
		R					(*this, "R",					"R",																			 'r'),
		S					(*this, "S",					"S",																			 's'),
		T					(*this, "T",					"T",																			 't'),
		U					(*this, "U",					"U",																			 'u'),
		V					(*this, "V",					"V",																			 'v'),
		W					(*this, "W",					"W",																			 'w'),
		X					(*this, "X",					"X",																			 'x'),
		Y					(*this, "Y",					"Y",																			 'y'),
		Z					(*this, "Z",					"Z",																			 'z'),
		Numpad0				(*this, "Numpad0",				"Numpad 0",																		 '0'),
		Numpad1				(*this, "Numpad1",				"Numpad 1",																		 '1'),
		Numpad2				(*this, "Numpad2",				"Numpad 2",																		 '2'),
		Numpad3				(*this, "Numpad3",				"Numpad 3",																		 '3'),
		Numpad4				(*this, "Numpad4",				"Numpad 4",																		 '4'),
		Numpad5				(*this, "Numpad5",				"Numpad 5",																		 '5'),
		Numpad6				(*this, "Numpad6",				"Numpad 6",																		 '6'),
		Numpad7				(*this, "Numpad7",				"Numpad 7",																		 '7'),
		Numpad8				(*this, "Numpad8",				"Numpad 8",																		 '8'),
		Numpad9				(*this, "Numpad9",				"Numpad 9",																		 '9'),
		NumpadMultiply		(*this, "NumpadMultiply",		"Numpad multiply",																 '*'),
		NumpadAdd			(*this, "NumpadAdd",			"Numpad add",																	 '+'),
		NumpadSeparator		(*this, "NumpadSeparator",		"Numpad separator",																 ','),
		NumpadSubtract		(*this, "NumpadSubtract",		"Numpad subtract",																 '-'),
		NumpadDecimal		(*this, "NumpadDecimal",		"Numpad decimal",																 '.'),
		NumpadDivide		(*this, "NumpadDivide",			"Numpad divide",																 '/'),
		F1					(*this, "F1",					"F1",																			0x00),
		F2					(*this, "F2",					"F2",																			0x00),
		F3					(*this, "F3",					"F3",																			0x00),
		F4					(*this, "F4",					"F4",																			0x00),
		F5					(*this, "F5",					"F5",																			0x00),
		F6					(*this, "F6",					"F6",																			0x00),
		F7					(*this, "F7",					"F7",																			0x00),
		F8					(*this, "F8",					"F8",																			0x00),
		F9					(*this, "F9",					"F9",																			0x00),
		F10					(*this, "F10",					"F10",																			0x00),
		F11					(*this, "F11",					"F11",																			0x00),
		F12					(*this, "F12",					"F12",																			0x00),
		NumLock				(*this, "NumLock",				"Num lock",																		0x00),
		ScrollLock			(*this, "ScrollLock",			"Scroll lock",																	0x00),
		Circumflex			(*this, "Circumflex",			"Circumflex (^)",																0x00),
		LeftWindows			(*this, "LeftWindows",			"Left Windows key (natural keyboard)",											0x00),
		RightWindows		(*this, "RightWindows",			"Right Windows key (natural keyboard)",											0x00),
		Applications		(*this, "Applications",			"Applications key (natural keyboard)",											0x00),
		F13					(*this, "F13",					"F13",																			0x00),
		F14					(*this, "F14",					"F14",																			0x00),
		F15					(*this, "F15",					"F15",																			0x00),
		F16					(*this, "F16",					"F16",																			0x00),
		F17					(*this, "F17",					"F17",																			0x00),
		F18					(*this, "F18",					"F18",																			0x00),
		F19					(*this, "F19",					"F19",																			0x00),
		F20					(*this, "F20",					"F20",																			0x00),
		F21					(*this, "F21",					"F21",																			0x00),
		F22					(*this, "F22",					"F22",																			0x00),
		F23					(*this, "F23",					"F23",																			0x00),
		F24					(*this, "F24",					"F24",																			0x00),
		LeftShift			(*this, "LeftShift",			"Left shift",																	0x00),
		RightShift			(*this, "RightShift",			"Right shift",																	0x00),
		LeftControl			(*this, "LeftControl",			"Left control",																	0x00),
		RightControl		(*this, "RightControl",			"Right control",																0x00),
		VolumeMute			(*this, "VolumeMute",			"Volume mute",																	0x00),
		VolumeDown			(*this, "VolumeDown",			"Volume down",																	0x00),
		VolumeUp			(*this, "VolumeUp",				"Volume up",																	0x00),
		MediaNextTrack		(*this, "MediaNextTrack",		"Media next track",																0x00),
		MediaPreviousTrack	(*this, "MediaPreviousTrack",	"Media previous track",															0x00),
		MediaStop			(*this, "MediaStop",			"Media stop",																	0x00),
		MediaPlayPause		(*this, "MediaPlayPause",		"Media play pause",																0x00),
		Add					(*this, "Add",					"For any country/region, the '+' key",											 '+'),
		Separator			(*this, "Separator",			"For any country/region, the ',' key",											 ','),
		Subtract			(*this, "Subtract",				"For any country/region, the '-' key",											 '-'),
		Decimal				(*this, "Decimal",				"For any country/region, the '.' key",											 '.'),
		OEM1				(*this, "OEM1",					"For the US standard keyboard, the ';:' key",									0x00),
		OEM2				(*this, "OEM2",					"For the US standard keyboard, the '/?' key",									0x00),
		OEM3				(*this, "OEM3",					"For the US standard keyboard, the '`~' key",									0x00),
		OEM4				(*this, "OEM4",					"For the US standard keyboard, the '[{' key",									0x00),
		OEM5				(*this, "OEM5",					"For the US standard keyboard, the 'backslash|' key",							0x00),
		OEM6				(*this, "OEM6",					"For the US standard keyboard, the ']}' key",									0x00),
		OEM7				(*this, "OEM7",					"For the US standard keyboard, the 'single-quote/double-quote' key",			0x00),
		OEM8				(*this, "OEM8",					"Used for miscellaneous characters; it can vary by keyboard",					0x00),
		OEM102				(*this, "OEM102",				"Either the angle bracket key or the backslash key on the RT 102-key keyboard",	0x00)
	{
		// Nothing here
	}


	//[-------------------------------------------------------]
	//[ Public virtual Controller functions                   ]
	//[-------------------------------------------------------]
	void Keyboard::Update()
	{
		// Update device backend
		if (m_pImpl && m_pImpl->getDeviceBackendType() == DeviceBackendType::UPDATE_DEVICE)
		{
			static_cast<UpdateDevice*>(m_pImpl)->Update();
		}
	}




	//[-------------------------------------------------------]
	//[ PLInput::Mouse                                        ]
	//[-------------------------------------------------------]
	//[-------------------------------------------------------]
	//[ Public functions                                      ]
	//[-------------------------------------------------------]
	Mouse::Mouse(InputManager& inputManager, const std::string &sName, DeviceImpl *pImpl) :
		Device(inputManager, sName, "Mouse input controller", pImpl),
		X		(*this, "X",		"X axis (movement data, no absolute data)"),
		Y		(*this, "Y",		"Y axis (movement data, no absolute data)"),
		Wheel	(*this, "Wheel",	"Mouse wheel (movement data, no absolute data)"),
		Left	(*this, "Left",		"Left mouse button (mouse button #0)"),
		Right	(*this, "Right",	"Right mouse button (mouse button #1)"),
		Middle	(*this, "Middle",	"Middle mouse button (mouse button #2)"),
		Button4	(*this, "Button4",	"Mouse button #4"),
		Button5	(*this, "Button5",	"Mouse button #5"),
		Button6	(*this, "Button6",	"Mouse button #6"),
		Button7	(*this, "Button7",	"Mouse button #7"),
		Button8	(*this, "Button8",	"Mouse button #8"),
		Button9	(*this, "Button9",	"Mouse button #9"),
		Button10(*this, "Button10",	"Mouse button #10"),
		Button11(*this, "Button11",	"Mouse button #11"),
		Button12(*this, "Button12",	"Mouse button #12")
	{
		// Nothing here
	}


	//[-------------------------------------------------------]
	//[ Public virtual Controller functions                   ]
	//[-------------------------------------------------------]
	void Mouse::Update()
	{
		// Update device backend
		if (m_pImpl && m_pImpl->getDeviceBackendType() == DeviceBackendType::UPDATE_DEVICE)
		{
			static_cast<UpdateDevice*>(m_pImpl)->Update();
		}
	}




	//[-------------------------------------------------------]
	//[ PLInput::Joystick                                     ]
	//[-------------------------------------------------------]
	//[-------------------------------------------------------]
	//[ Public functions                                      ]
	//[-------------------------------------------------------]
	Joystick::Joystick(InputManager& inputManager, const std::string &sName, DeviceImpl *pImpl) :
		Device(inputManager, sName, "Joystick input controller", pImpl),
		X		(*this,	"X",		"X axis"),
		Y		(*this,	"Y",		"Y axis"),
		Z		(*this,	"Z",		"Z axis"),
		RX		(*this,	"RX",		"Rotation X axis"),
		RY		(*this,	"RY",		"Rotation Y axis"),
		RZ		(*this,	"RZ",		"Rotation Z axis"),
		Hat		(*this,	"Hat",		"Hat axis"),
		Button0	(*this,	"Button0",	"Button #0"),
		Button1	(*this,	"Button1",	"Button #1"),
		Button2	(*this,	"Button2",	"Button #2"),
		Button3	(*this,	"Button3",	"Button #3"),
		Button4	(*this,	"Button4",	"Button #4"),
		Button5	(*this,	"Button5",	"Button #5"),
		Button6	(*this,	"Button6",	"Button #6"),
		Button7	(*this,	"Button7",	"Button #7"),
		Button8	(*this,	"Button8",	"Button #8"),
		Button9	(*this,	"Button9",	"Button #9"),
		Button10(*this,	"Button10",	"Button #10"),
		Button11(*this,	"Button11",	"Button #11"),
		Button12(*this,	"Button12",	"Button #12"),
		Button13(*this,	"Button13",	"Button #13"),
		Button14(*this,	"Button14",	"Button #14"),
		Button15(*this,	"Button15",	"Button #15"),
		Button16(*this,	"Button16",	"Button #16"),
		Button17(*this,	"Button17",	"Button #17"),
		Button18(*this,	"Button18",	"Button #18"),
		Button19(*this,	"Button19",	"Button #19"),
		Button20(*this,	"Button20",	"Button #20"),
		Button21(*this,	"Button21",	"Button #21"),
		Button22(*this,	"Button22",	"Button #22"),
		Button23(*this,	"Button23",	"Button #23"),
		Button24(*this,	"Button24",	"Button #24"),
		Button25(*this,	"Button25",	"Button #25"),
		Button26(*this,	"Button26",	"Button #26"),
		Button27(*this,	"Button27",	"Button #27"),
		Button28(*this,	"Button28",	"Button #28"),
		Button29(*this,	"Button29",	"Button #29"),
		Button30(*this,	"Button30",	"Button #30"),
		Button31(*this,	"Button31",	"Button #31"),
		Rumble1	(*this,	"Rumble1",	"Rumble motor #1"),
		Rumble2	(*this,	"Rumble2",	"Rumble motor #2"),
		Rumble3	(*this,	"Rumble3",	"Rumble motor #3"),
		Rumble4	(*this,	"Rumble4",	"Rumble motor #4"),
		m_pHIDDevice(nullptr),
		m_nThreshold(12000)
	{
		// Check if we have a HID backend
		if (pImpl && pImpl->getDeviceBackendType() == DeviceBackendType::HID)
		{
			// Save extra pointer to HIDDevice
			m_pHIDDevice = static_cast<HIDDevice*>(pImpl);

			// Connect to HIDDevice events
			m_pHIDDevice->OnReadSignal.connect_member(this, &Joystick::OnDeviceRead);

			// Connect to HID device
			m_pHIDDevice->Open();
		}
	}

	Joystick::~Joystick()
	{
		// Check if we have a HID backend
		if (m_pHIDDevice) {
			// We use m_pImpl here to check, because if the device backend has been deleted before, m_pImpl has
			// been reset to a null pointer, but not m_pHIDDevice as this is unknown in the base class
			if (m_pImpl) {
				// Disconnect
				m_pHIDDevice->Close();
			}
		}
	}


	//[-------------------------------------------------------]
	//[ Public virtual Controller functions                   ]
	//[-------------------------------------------------------]
	void Joystick::Update()
	{
		// Check if we have an update device backend
		if (m_pImpl && m_pImpl->getDeviceBackendType() == DeviceBackendType::UPDATE_DEVICE)
		{
			// Update device backend
			static_cast<UpdateDevice*>(m_pImpl)->Update();
		}
	}

	void Joystick::UpdateOutputControl(Control*)
	{
		// Check if we have a HID backend
		if (m_pHIDDevice) {
			// Get output values
			std::vector<HIDCapability> &lstOutputValues = m_pHIDDevice->GetOutputValues();
			for (uint32_t i=0; i<lstOutputValues.size(); i++) {
				// Get capability
				HIDCapability *pCapability = &lstOutputValues[i];
				Effect *pEffect = nullptr;

				// Rumble
				if (pCapability->m_nUsagePage == UsagePageLED) {
					// Get effect control
					if (pCapability->m_nUsagePage == UsagePageLED && pCapability->m_nUsage == UsageSlowBlinkOnTime)
						pEffect = &Rumble1;
					else if (pCapability->m_nUsagePage == UsagePageLED && pCapability->m_nUsage == UsageSlowBlinkOffTime)
						pEffect = &Rumble2;
					else if (pCapability->m_nUsagePage == UsagePageLED && pCapability->m_nUsage == UsageFastBlinkOnTime)
						pEffect = &Rumble3;
					else if (pCapability->m_nUsagePage == UsagePageLED && pCapability->m_nUsage == UsageFastBlinkOffTime)
						pEffect = &Rumble4;

					// Set effect value
					if (pEffect) {
						// Get value (must be between 0..1)
						float fValue = pEffect->getValue();
						if (fValue < 0.0f)
							fValue = 0.0f;
						if (fValue > 1.0f)
							fValue = 1.0f;

						// Scale from 0..1 to logical range and set value
						pCapability->m_nValue = static_cast<uint32_t>(pCapability->m_nLogicalMin + fValue*(pCapability->m_nLogicalMax-pCapability->m_nLogicalMin));
					}
				}
			}

			// Send output report with that ID that belongs to the given control
			m_pHIDDevice->SendOutputReport();
		}
	}


	//[-------------------------------------------------------]
	//[ Private functions                                     ]
	//[-------------------------------------------------------]
	void Joystick::OnDeviceRead()
	{
		// Get input buffer
		uint8_t *pInputBuffer = m_pHIDDevice->GetInputBuffer();
		if (pInputBuffer) {
			// Parse input report
			m_pHIDDevice->ParseInputReport(pInputBuffer, m_pHIDDevice->GetInputReportSize());

			// Update axes
			const std::vector<HIDCapability> &lstInputValues = m_pHIDDevice->GetInputValues();
			for (uint32_t i=0; i<lstInputValues.size(); i++) {
				// Get raw value and compute logical value
				uint32_t nValue = lstInputValues[i].m_nValue;
				float fValue = static_cast<float>(nValue);
				if (lstInputValues[i].m_nUsage != UsageHat) {
					uint32_t nMin	= static_cast<uint32_t>(lstInputValues[i].m_nLogicalMin);
					uint32_t nMax	= static_cast<uint32_t>(lstInputValues[i].m_nLogicalMax);
					uint32_t nMid	=  nMin/2 + nMax/2;
					fValue			= (static_cast<float>(nValue) - nMid) / (static_cast<float>(nMax) - nMin) * 2.0f;
				}

				// Set axis value
				switch (lstInputValues[i].m_nUsage) {
					case UsageX:
						if (X.getValue() != fValue)
							X.setValue(fValue, false);
						break;

					case UsageY:
						if (Y.getValue() != fValue)
							Y.setValue(fValue, false);
						break;

					case UsageZ:
						if (Z.getValue() != fValue)
							Z.setValue(fValue, false);
						break;

					case UsageRX:
						if (RX.getValue() != fValue)
							RX.setValue(fValue, false);
						break;

					case UsageRY:
						if (RY.getValue() != fValue)
							RY.setValue(fValue, false);
						break;

					case UsageRZ:
						if (RZ.getValue() != fValue)
							RZ.setValue(fValue, false);
						break;

					case UsageHat:
						if (Hat.getValue() != fValue)
							Hat.setValue(fValue, false);
						break;
				}
			}

			// Update buttons
			const std::vector<HIDCapability> &lstButtons = m_pHIDDevice->GetInputButtons();
			for (uint32_t i=0; i<lstButtons.size(); i++) {
				// Get state of buttons
				uint32_t nValue = lstButtons[i].m_nValue;

				// Go through usage range
				for (uint32_t nUsage = lstButtons[i].m_nUsageMin; nUsage <= lstButtons[i].m_nUsageMax; nUsage++) {
					// Check if button is pressed
					bool bPressed = ((nValue & 1) != 0);
					nValue = nValue >> 1;

					// Get button
					Button *pButton = GetButtons()[nUsage - UsageButton1];
					if (pButton)
					{
						// Update only if state has changed
						if (pButton->isPressed() != bPressed)
						{
							pButton->setPressed(bPressed);
						}
					}
				}
			}
		}
	}




	//[-------------------------------------------------------]
	//[ PLInput::SpaceMouse                                   ]
	//[-------------------------------------------------------]
	//[-------------------------------------------------------]
	//[ Public functions                                      ]
	//[-------------------------------------------------------]
	SpaceMouse::SpaceMouse(InputManager& inputManager, const std::string &sName, DeviceImpl *pImpl) :
		Device(inputManager, sName, "SpaceMouse type controller", pImpl),
		TransX	(*this,	"TransX",	"X translation axis"),
		TransY	(*this,	"TransY",	"Y translation axis"),
		TransZ	(*this,	"TransZ",	"Z translation axis"),
		RotX	(*this,	"RotX",		"X rotation axis"),
		RotY	(*this,	"RotY",		"Y rotation axis"),
		RotZ	(*this,	"RotZ",		"Z rotation axis"),
		Button0	(*this,	"Button0",	"Button #0"),
		Button1	(*this,	"Button1",	"Button #1"),
		Button2	(*this,	"Button2",	"Button #2"),
		Button3	(*this,	"Button3",	"Button #3"),
		Button4	(*this,	"Button4",	"Button #4"),
		Button5	(*this,	"Button5",	"Button #5"),
		Button6	(*this,	"Button6",	"Button #6"),
		Button7	(*this,	"Button7",	"Button #7"),
		m_pHIDDevice(nullptr)
	{
		// Check if we have a HID backend
		if (pImpl && pImpl->getDeviceBackendType() == DeviceBackendType::HID)
		{
			// Save extra pointer to HIDDevice
			m_pHIDDevice = static_cast<HIDDevice*>(pImpl);

			// Connect to HIDDevice events
			m_pHIDDevice->OnReadSignal.connect_member(this, &SpaceMouse::OnDeviceRead);

			// Connect to HID device
			m_pHIDDevice->Open();
		}
	}

	SpaceMouse::~SpaceMouse()
	{
		// Check if we have a HID backend
		if (m_pHIDDevice) {
			// We use m_pImpl here to check, because if the device backend has been deleted before, m_pImpl has
			// been reset to a null pointer, but not m_pHIDDevice as this is unknown in the base class
			if (m_pImpl) {
				// Disconnect
				m_pHIDDevice->Close();
			}
		}
	}


	//[-------------------------------------------------------]
	//[ Private functions                                     ]
	//[-------------------------------------------------------]
	void SpaceMouse::OnDeviceRead()
	{
		// Get input buffer
		uint8_t *pInputBuffer = m_pHIDDevice->GetInputBuffer();
		if (pInputBuffer) {
			// Read data
			switch (pInputBuffer[0]) {
				// Translation
				case 0x01:
				{
					float fTransX = static_cast<float>(static_cast<int16_t>((pInputBuffer[1] & 0x000000ff) | (static_cast<int>(pInputBuffer[2])<<8 & 0xffffff00)));
					float fTransY = static_cast<float>(static_cast<int16_t>((pInputBuffer[3] & 0x000000ff) | (static_cast<int>(pInputBuffer[4])<<8 & 0xffffff00)));
					float fTransZ = static_cast<float>(static_cast<int16_t>((pInputBuffer[5] & 0x000000ff) | (static_cast<int>(pInputBuffer[6])<<8 & 0xffffff00)));
					if (TransX.getValue() != fTransX)
						TransX.setValue(fTransX, false);
					if (TransY.getValue() != fTransY)
						TransY.setValue(fTransY, false);
					if (TransZ.getValue() != fTransZ)
						TransZ.setValue(fTransZ, false);
					break;
				}

				// Rotation
				case 0x02:
				{
					float fRotX = static_cast<float>(static_cast<int16_t>((pInputBuffer[1] & 0x000000ff) | (static_cast<int>(pInputBuffer[2])<<8 & 0xffffff00)));
					float fRotY = static_cast<float>(static_cast<int16_t>((pInputBuffer[3] & 0x000000ff) | (static_cast<int>(pInputBuffer[4])<<8 & 0xffffff00)));
					float fRotZ = static_cast<float>(static_cast<int16_t>((pInputBuffer[5] & 0x000000ff) | (static_cast<int>(pInputBuffer[6])<<8 & 0xffffff00)));
					if (RotX.getValue() != fRotX)
						RotX.setValue(fRotX, false);
					if (RotY.getValue() != fRotY)
						RotY.setValue(fRotY, false);
					if (RotZ.getValue() != fRotZ)
						RotZ.setValue(fRotZ, false);
					break;
				}

				// Buttons
				case 0x03:
				{
					bool bPressed = ((pInputBuffer[1] & 0x0001) != 0);
					if (Button0.isPressed() != bPressed)
					{
						Button0.setPressed(bPressed);
					}
					bPressed = ((pInputBuffer[1] & 0x0002) != 0);
					if (Button1.isPressed() != bPressed)
					{
						Button1.setPressed(bPressed);
					}
					bPressed = ((pInputBuffer[1] & 0x0004) != 0);
					if (Button2.isPressed() != bPressed)
					{
						Button2.setPressed(bPressed);
					}
					bPressed = ((pInputBuffer[1] & 0x0008) != 0);
					if (Button3.isPressed() != bPressed)
					{
						Button3.setPressed(bPressed);
					}
					bPressed = ((pInputBuffer[1] & 0x0010) != 0);
					if (Button4.isPressed() != bPressed)
					{
						Button4.setPressed(bPressed);
					}
					bPressed = ((pInputBuffer[1] & 0x0020) != 0);
					if (Button5.isPressed() != bPressed)
					{
						Button5.setPressed(bPressed);
					}
					bPressed = ((pInputBuffer[1] & 0x0040) != 0);
					if (Button6.isPressed() != bPressed)
					{
						Button6.setPressed(bPressed);
					}
					bPressed = ((pInputBuffer[1] & 0x0080) != 0);
					if (Button7.isPressed() != bPressed)
					{
						Button7.setPressed(bPressed);
					}
					break;
				}
			}
		}
	}




	//[-------------------------------------------------------]
	//[ PLInput::WiiMote                                      ]
	//[-------------------------------------------------------]
	//[-------------------------------------------------------]
	//[ Definitions                                           ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    WiiMote ports
	*/
	enum EPort {
		ControlPort		= 17,	///< Control port (output)
		InterruptPort	= 19	///< Interrupt port (input)
	};

	/**
	*  @brief
	*    WiiMote commands
	*/
	enum ECommand {
		CmdNone				= 0x00,	///< No command
		CmdLEDs				= 0x11,	///< Read LEDs
		CmdType				= 0x12,	///< Set report mode
		CmdIR				= 0x13,	///< Enable IR
		CmdSpeakerEnable	= 0x14,	///< Enable speaker
		CmdStatus			= 0x15,	///< Get status
		CmdWriteMemory		= 0x16,	///< Write to memory
		CmdReadMemory		= 0x17,	///< Read from memory
		CmdSpeakerData		= 0x18,	///< Send speaker data
		CmdSpeakerMute		= 0x19,	///< Mute speaker
		CmdIR2				= 0x1a	///< Enable IR
	};

	/**
	*  @brief
	*    WiiMote registers
	*/
	enum ERegister {
		RegCalibration			= 0x00000016,	///< Calibration
		RegIR					= 0x04b00030,	///< IR
		RegIRSensitivity1		= 0x04b00000,	///< IR sensitivity (1)
		RegIRSensitivity2		= 0x04b0001a,	///< IR sensitivity (2)
		RegIRMode				= 0x04b00033,	///< IR mode
		RegExtensionInit		= 0x04a40040,	///< Extension initialization
		RegExtensionType		= 0x04a400fe,	///< Extension type
		RegExtensionCalibration	= 0x04a40020	///< Extension calibration
	};

	/**
	*  @brief
	*    WiiMote buttons
	*/
	enum EButton {
		BtnLeft		= 0x0001,	///< Button "Left"
		BtnRight	= 0x0002,	///< Button "Right"
		BtnDown		= 0x0004,	///< Button "Down"
		BtnUp		= 0x0008,	///< Button "Up"
		BtnPlus		= 0x0010,	///< Button "+"
		Btn2		= 0x0100,	///< Button "2"
		Btn1		= 0x0200,	///< Button "1"
		BtnB		= 0x0400,	///< Button "B"
		BtnA		= 0x0800,	///< Button "A"
		BtnMinus	= 0x1000,	///< Button "-"
		BtnHome		= 0x8000	///< Button "Home"
	};

	/**
	*  @brief
	*    Nunchuk buttons
	*/
	enum ENunchukButton {
		BtnNunchukZ	= 0x0001,	///< Button "Z"
		BtnNunchukC	= 0x0002	///< Button "C"
	};

	// IR camera
	static constexpr int IR_MaxX	= 1016;	///< Effective X Resolution of IR sensor
	static constexpr int IR_MaxY	=  760;	///< Effective Y Resolution of IR sensor


	//[-------------------------------------------------------]
	//[ Global helper functions                               ]
	//[-------------------------------------------------------]
	bool ValueChanged(float fValueOld, float fValueNew)
	{
		// Apply epsilon boundary
		return (fabs(fValueNew - fValueOld) >= 0.01f);
	}


	//[-------------------------------------------------------]
	//[ Public functions                                      ]
	//[-------------------------------------------------------]
	WiiMote::WiiMote(InputManager& inputManager, const std::string &sName, DeviceImpl *pImpl) :
		Device(inputManager, sName, "WiiMote controller", pImpl),
		LEDs			(*this,	"LEDs",				"LEDs"),
		Rumble1			(*this,	"Rumble1",			"Rumble motor #1"),
		Button1			(*this,	"Button1",			"Button '1'"),
		Button2			(*this,	"Button2",			"Button '2'"),
		ButtonA			(*this,	"ButtonA",			"Button 'A'"),
		ButtonB			(*this,	"ButtonB",			"Button 'B'"),
		ButtonMinus		(*this,	"ButtonMinus",		"Button 'Minus'"),
		ButtonPlus		(*this,	"ButtonPlus",		"Button 'Plus'"),
		ButtonHome		(*this,	"ButtonHome",		"Button 'Home'"),
		ButtonLeft		(*this,	"ButtonLeft",		"Button 'Left'"),
		ButtonRight		(*this,	"ButtonRight",		"Button 'Right'"),
		ButtonUp		(*this,	"ButtonUp",			"Button 'Up'"),
		ButtonDown		(*this,	"ButtonDown",		"Button 'Down'"),
		AccX			(*this,	"AccX",				"Acceleration axis (X)"),
		AccY			(*this,	"AccY",				"Acceleration axis (Y)"),
		AccZ			(*this,	"AccZ",				"Acceleration axis (Z)"),
		OrientX			(*this,	"OrientX",			"Orientation axis (X)"),
		OrientY			(*this,	"OrientY",			"Orientation axis (Y)"),
		OrientZ			(*this,	"OrientZ",			"Orientation axis (Z)"),
		Roll			(*this,	"Roll",				"Rotation (roll)"),
		Pitch			(*this,	"Pitch",			"Rotation (pitch)"),
		PointerX		(*this,	"PointerX",			"Pointer(X)"),
		PointerY		(*this,	"PointerY",			"Pointer(Y)"),
		NunchukButtonC	(*this,	"NunchukButtonC",	"Nunchuk button 'C'"),
		NunchukButtonZ	(*this,	"NunchukButtonZ",	"Nunchuk button 'Z'"),
		NunchukAccX		(*this,	"NunchukAccX",		"Nunchuk acceleration axis (X)"),
		NunchukAccY		(*this,	"NunchukAccY",		"Nunchuk acceleration axis (Y)"),
		NunchukAccZ		(*this,	"NunchukAccZ",		"Nunchuk acceleration axis (Z)"),
		NunchukOrientX	(*this,	"NunchukOrientX",	"Nunchuk orientation axis (X)"),
		NunchukOrientY	(*this,	"NunchukOrientY",	"Nunchuk orientation axis (Y)"),
		NunchukOrientZ	(*this,	"NunchukOrientZ",	"Nunchuk orientation axis (Z)"),
		NunchukRoll		(*this,	"NunchukRoll",		"Nunchuk rotation (roll)"),
		NunchukPitch	(*this,	"NunchukPitch",		"Nunchuk rotation (pitch)"),
		NunchukX		(*this,	"NunchukX",			"Nunchuk joystick (X)"),
		NunchukY		(*this,	"NunchukY",			"Nunchuk joystick (Y)"),
		m_pConnectionDevice(static_cast<ConnectionDevice*>(pImpl)),
		m_pInputBuffer(nullptr),
		m_pOutputBuffer(nullptr),
		m_nReportMode(ReportButtons),
		m_nIRMode(IROff),
		m_nExtension(ExtNone),
		m_nBattery(0),
		m_nLEDs(0),
		m_nRumble(0),
		m_nButtons(0),
		m_vIRPos{0.0f, 0.0f},
		m_nNunchukButtons(0)
	{
		// Set input and output report size
		if (m_pConnectionDevice->GetDeviceType() == ConnectionDevice::DeviceTypeBluetooth) {
			m_pConnectionDevice->SetInputReportSize(22);
			m_pConnectionDevice->SetOutputReportSize(22);
		}

		// Connect to HIDDevice events
		m_pConnectionDevice->OnConnectSignal.connect_member(this, &WiiMote::OnDeviceConnect);
		m_pConnectionDevice->OnReadSignal.connect_member(this, &WiiMote::OnDeviceRead);

		// Connect to device
		m_pConnectionDevice->Open(ControlPort, InterruptPort);

		// Initialize data
		m_sAcc.nUpdateNearG			 = 0;
		m_sNunchukAcc.nUpdateNearG	 = 0;
	}

	WiiMote::~WiiMote()
	{
		// We use m_pImpl here to check, because if the device backend has been deleted before, m_pImpl has
		// been reset to a null pointer, but not m_pConnectionDevice as this is unknown in the base class
		if (m_pImpl) {
			// Disconnect
			m_pConnectionDevice->Close();
		}
	}

	void WiiMote::SetReportMode(EReport nReportMode, bool bContinuous)
	{
		// Set report mode
		m_nReportMode = nReportMode;

		// Enable/Disable infrared sensor
		switch (m_nReportMode) {
			case ReportButtonsAccelIR:
				SetIRMode(IRExtended);
				break;

			case ReportButtonsAccelIRExt:
				SetIRMode(IRBasic);
				break;

			case ReportStatus:
			case ReportReadMemory:
			case ReportButtons:
			case ReportButtonsAccel:
			case ReportButtonsAccelExt:
			default:
				SetIRMode(IROff);
				break;
		}

		// Set new mode
		ClearReport();
		m_pOutputBuffer[0] = CmdType;
		m_pOutputBuffer[1] = (bContinuous ? 0x04u : 0x00u) | m_nRumble;
		m_pOutputBuffer[2] = static_cast<uint8_t>(nReportMode);
		Send(m_pOutputBuffer, 3);
	}

	void WiiMote::SetIRMode(EIRMode nIRMode)
	{
		// Set IR mode
		m_nIRMode = nIRMode;

		// Set new mode
		if (m_nIRMode != IROff) {
			ClearReport();
			m_pOutputBuffer[0] = CmdIR;
			m_pOutputBuffer[1] = 0x04u | m_nRumble;
			Send(m_pOutputBuffer, 2);
			m_pOutputBuffer[0] = CmdIR2;
			m_pOutputBuffer[1] = 0x04u | m_nRumble;
			Send(m_pOutputBuffer, 2);

			static constexpr uint8_t ir_sens1[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x90, 0x00, 0xc0};
			static constexpr uint8_t ir_sens2[] = {0x40, 0x00};

			WriteMemory(RegIR, 0x08);
			WriteMemory(RegIRSensitivity1, ir_sens1, sizeof(ir_sens1));
			WriteMemory(RegIRSensitivity2, ir_sens2, sizeof(ir_sens2));
			WriteMemory(RegIRMode, static_cast<uint8_t>(nIRMode));
		} else {
			ClearReport();
			m_pOutputBuffer[0] = CmdIR;
			m_pOutputBuffer[1] = m_nRumble;
			Send(m_pOutputBuffer, 2);
			m_pOutputBuffer[0] = CmdIR2;
			m_pOutputBuffer[1] = m_nRumble;
			Send(m_pOutputBuffer, 2);
		}
	}


	//[-------------------------------------------------------]
	//[ Public virtual Controller functions                   ]
	//[-------------------------------------------------------]
	void WiiMote::UpdateOutputControl(Control *pControl)
	{
		// Update LEDs
		if (pControl == &LEDs) {
			// Set LEDs
			m_nLEDs = static_cast<uint8_t>(LEDs.getLedStates()) & 0x0fu;

			// Send command
			ClearReport();
			m_pOutputBuffer[0] = CmdLEDs;
			m_pOutputBuffer[1] = static_cast<uint8_t>((m_nLEDs << 4) | m_nRumble);
			Send(m_pOutputBuffer, 2);

		// Update rumble
		} else if (pControl == &Rumble1) {
			// Switch rumble on or off?
			if (Rumble1.getValue() > 0) {
				// Enable rumble
				m_nRumble = 1;
				SendStatusRequest();
			} else {
				// Disable rumble
				m_nRumble = 0;
				SendStatusRequest();
			}
		}
	}


	//[-------------------------------------------------------]
	//[ Private functions                                     ]
	//[-------------------------------------------------------]
	void WiiMote::OnDeviceConnect()
	{
		m_pInputBuffer  = m_pConnectionDevice->GetInputBuffer();
		m_pOutputBuffer = m_pConnectionDevice->GetOutputBuffer();

		SendStatusRequest();
		SendCalibrationRequest();
		SetReportMode(ReportButtonsAccelIR, false);
	}

	void WiiMote::OnReadData()
	{
		// What kind of data has been received?
		switch (m_pInputBuffer[0]) {
			// Get buttons
			case ReportButtons:
				OnReadButtons();
				break;

			// Get buttons and acceleration
			case ReportButtonsAccel:
				OnReadButtons();
				OnReadAccel();
				break;

			// Get buttons, acceleration and extension
			case ReportButtonsAccelExt:
				OnReadButtons();
				OnReadAccel();
				DecryptBuffer(0, m_pConnectionDevice->GetInputReportSize());
				OnReadExtension(6);
				break;

			// Get buttons, acceleration and IR (extended mode)
			case ReportButtonsAccelIR:
				OnReadButtons();
				OnReadAccel();
				OnReadIR();
				break;

			// Get buttons, acceleration, IR (basic mode) and extension
			case ReportButtonsAccelIRExt:
				OnReadButtons();
				OnReadAccel();
				OnReadIR();
				DecryptBuffer(0, m_pConnectionDevice->GetInputReportSize());
				OnReadExtension(16);
				break;

			// Get data from memory
			case ReportReadMemory:
				OnReadButtons();
				OnReadMemory();
				break;

			// Get status report
			case ReportStatus:
				OnReadStatus();
				break;
		}
	}

	void WiiMote::OnReadMemory()
	{
		// Check memory address
		if ((m_pInputBuffer[3] & 0x08) != 0) {
			// Error: Invalid read-address
			return;
		} else if((m_pInputBuffer[3] & 0x07) != 0) {
			// Error: Attempt to read from write-only registers
			return;
		}

		// Get size
		int nSize = m_pInputBuffer[3] >> 4;

		// Get address
		int nAddress = m_pInputBuffer[4]<<8 | m_pInputBuffer[5];

		// *NOTE*: this is a major (but convenient) hack!  The returned data only
		//          contains the lower two bytes of the address that was queried.
		//			as these don't collide between any of the addresses/registers
		//			we currently read, it's OK to match just those two bytes
		switch (nAddress) {
			case (RegCalibration & 0xffff):
				if (nSize != 6)
					break;	// Error! Wrong size ...
				OnReadCalibration();
				break;

			case (RegExtensionType & 0xffff):
				if (nSize != 1)
					break;	// Error! Wrong size ...
				OnReadExtensionType();
				break;

			case (RegExtensionCalibration & 0xffff):
				if (nSize != 15)
					break;	// Error! Wrong size ...
				DecryptBuffer(6, 16);
				if (m_nExtension == ExtNunchuk)
					OnReadNunchukCalibration();
				else if (m_nExtension == ExtClassic)
					OnReadClassicCalibration();
				break;
		}
	}

	void WiiMote::OnReadCalibration()
	{
		// Get calibration info
		m_sAcc.nX0 = m_pInputBuffer[6+0];
		m_sAcc.nY0 = m_pInputBuffer[6+1];
		m_sAcc.nZ0 = m_pInputBuffer[6+2];
		m_sAcc.nXG = m_pInputBuffer[6+4];
		m_sAcc.nYG = m_pInputBuffer[6+5];
		m_sAcc.nZG = m_pInputBuffer[6+6];
	}

	void WiiMote::OnReadExtensionType()
	{
		// Get extension type
		uint16_t nExtension = *reinterpret_cast<uint16_t*>(&m_pInputBuffer[6]);

		// If new extension, get calibration info
		if (nExtension == ExtNunchuk && m_nExtension != ExtNunchuk) {
			// Nunchuk registered
			ReadMemory(RegExtensionCalibration, 16);
			m_nExtension = ExtNunchuk;
		} else if (nExtension == ExtClassic && m_nExtension != ExtClassic) {
			// Classic-controller registered
			ReadMemory(RegExtensionCalibration, 16);
			m_nExtension = ExtClassic;
		} else if (nExtension == ExtPartiallyInserted && m_nExtension != ExtPartiallyInserted) {
			// Extension not registered correctly
			m_nExtension = ExtPartiallyInserted;
			SendStatusRequest();
		} else {
			// Unknown extension
		}
	}

	void WiiMote::OnReadNunchukCalibration()
	{
		// Get nunchuk calibration
		m_sNunchukAcc.nX0	 = m_pInputBuffer[6+0];
		m_sNunchukAcc.nY0	 = m_pInputBuffer[6+1];
		m_sNunchukAcc.nZ0	 = m_pInputBuffer[6+2];
		m_sNunchukAcc.nXG	 = m_pInputBuffer[6+4];
		m_sNunchukAcc.nYG	 = m_pInputBuffer[6+5];
		m_sNunchukAcc.nZG	 = m_pInputBuffer[6+6];
		m_sNunchukJoy.nMaxX  = m_pInputBuffer[6+8];
		m_sNunchukJoy.nMinX  = m_pInputBuffer[6+9];
		m_sNunchukJoy.nMidX  = m_pInputBuffer[6+10];
		m_sNunchukJoy.nMaxY  = m_pInputBuffer[6+11];
		m_sNunchukJoy.nMinY  = m_pInputBuffer[6+12];
		m_sNunchukJoy.nMidY  = m_pInputBuffer[6+13];

		// Reset report mode
		SetReportMode(m_nReportMode, false);
	}

	void WiiMote::OnReadClassicCalibration()
	{
		// TODO(co)
		/*
		Internal.ClassicController.CalibrationInfo.MaxXL = m_pInputBuffer[6+ 0] >> 2;
		Internal.ClassicController.CalibrationInfo.MinXL = m_pInputBuffer[6+ 1] >> 2;
		Internal.ClassicController.CalibrationInfo.MidXL = m_pInputBuffer[6+ 2] >> 2;
		Internal.ClassicController.CalibrationInfo.MaxYL = m_pInputBuffer[6+ 3] >> 2;
		Internal.ClassicController.CalibrationInfo.MinYL = m_pInputBuffer[6+ 4] >> 2;
		Internal.ClassicController.CalibrationInfo.MidYL = m_pInputBuffer[6+ 5] >> 2;
		Internal.ClassicController.CalibrationInfo.MaxXR = m_pInputBuffer[6+ 6] >> 3;
		Internal.ClassicController.CalibrationInfo.MinXR = m_pInputBuffer[6+ 7] >> 3;
		Internal.ClassicController.CalibrationInfo.MidXR = m_pInputBuffer[6+ 8] >> 3;
		Internal.ClassicController.CalibrationInfo.MaxYR = m_pInputBuffer[6+ 9] >> 3;
		Internal.ClassicController.CalibrationInfo.MinYR = m_pInputBuffer[6+10] >> 3;
		Internal.ClassicController.CalibrationInfo.MidYR = m_pInputBuffer[6+11] >> 3;
		// this doesn't seem right...
		//	Internal.ClassicController.CalibrationInfo.MinTriggerL = m_pInputBuffer[6+12] >> 3;
		//	Internal.ClassicController.CalibrationInfo.MaxTriggerL = m_pInputBuffer[6+14] >> 3;
		//	Internal.ClassicController.CalibrationInfo.MinTriggerR = m_pInputBuffer[6+13] >> 3;
		//	Internal.ClassicController.CalibrationInfo.MaxTriggerR = m_pInputBuffer[6+15] >> 3;
		Internal.ClassicController.CalibrationInfo.MinTriggerL = 0;
		Internal.ClassicController.CalibrationInfo.MaxTriggerL = 31;
		Internal.ClassicController.CalibrationInfo.MinTriggerR = 0;
		Internal.ClassicController.CalibrationInfo.MaxTriggerR = 31;
		SetReportType(ReportType);
		*/
	}

	void WiiMote::OnReadStatus()
	{
		// Read button state
		OnReadButtons();

		// Get battery
		m_nBattery = m_pInputBuffer[6] / 2u;

		// Get LEDs
		m_nLEDs = static_cast<uint8_t>(m_pInputBuffer[3] >> 4);

		// Get extension
		bool bExtension = ((m_pInputBuffer[3] & 0x02) != 0);
		if ((m_nExtension == ExtNone || m_nExtension == ExtPartiallyInserted) && bExtension) {
			// Initialize extension
			WriteMemory(RegExtensionInit, 0x00);
			ReadMemory (RegExtensionType, 2);
		}
	}

	void WiiMote::OnReadButtons()
	{
		// Get state of buttons
		m_nButtons = *reinterpret_cast<uint16_t*>(&m_pInputBuffer[1]);

		// Button '1'
		bool bPressed = ((m_nButtons & Btn1) != 0);
		if (Button1.isPressed() != bPressed)
		{
			Button1.setPressed(bPressed);
		}

		// Button '2'
		bPressed = ((m_nButtons & Btn2) != 0);
		if (Button2.isPressed() != bPressed)
		{
			Button2.setPressed(bPressed);
		}

		// Button 'A'
		bPressed = ((m_nButtons & BtnA) != 0);
		if (ButtonA.isPressed() != bPressed)
		{
			ButtonA.setPressed(bPressed);
		}

		// Button 'B'
		bPressed = ((m_nButtons & BtnB) != 0);
		if (ButtonB.isPressed() != bPressed)
		{
			ButtonB.setPressed(bPressed);
		}

		// Button 'Minus'
		bPressed = ((m_nButtons & BtnMinus) != 0);
		if (ButtonMinus.isPressed() != bPressed)
		{
			ButtonMinus.setPressed(bPressed);
		}

		// Button 'Plus'
		bPressed = ((m_nButtons & BtnPlus) != 0);
		if (ButtonPlus.isPressed() != bPressed)
		{
			ButtonPlus.setPressed(bPressed);
		}

		// Button 'Home'
		bPressed = ((m_nButtons & BtnHome) != 0);
		if (ButtonHome.isPressed() != bPressed)
		{
			ButtonHome.setPressed(bPressed);
		}

		// Button 'Left'
		bPressed = ((m_nButtons & BtnLeft) != 0);
		if (ButtonLeft.isPressed() != bPressed)
		{
			ButtonLeft.setPressed(bPressed);
		}

		// Button 'Right'
		bPressed = ((m_nButtons & BtnRight) != 0);
		if (ButtonRight.isPressed() != bPressed)
		{
			ButtonRight.setPressed(bPressed);
		}

		// Button 'Up'
		bPressed = ((m_nButtons & BtnUp) != 0);
		if (ButtonUp.isPressed() != bPressed)
		{
			ButtonUp.setPressed(bPressed);
		}

		// Button 'Down'
		bPressed = ((m_nButtons & BtnDown) != 0);
		if (ButtonDown.isPressed() != bPressed)
		{
			ButtonDown.setPressed(bPressed);
		}
	}

	void WiiMote::OnReadAccel()
	{
		// Get raw acceleration data
		uint8_t nRawX = m_pInputBuffer[3];
		uint8_t nRawY = m_pInputBuffer[4];
		uint8_t nRawZ = m_pInputBuffer[5];

		// Compute acceleration
		m_sAcc.fAccX = static_cast<float>(nRawX - m_sAcc.nX0) / static_cast<float>(m_sAcc.nXG - m_sAcc.nX0);
		m_sAcc.fAccY = static_cast<float>(nRawY - m_sAcc.nY0) / static_cast<float>(m_sAcc.nYG - m_sAcc.nY0);
		m_sAcc.fAccZ = static_cast<float>(nRawZ - m_sAcc.nZ0) / static_cast<float>(m_sAcc.nZG - m_sAcc.nZ0);

			// AccX
			if (ValueChanged(AccX.getValue(), m_sAcc.fAccX))
				AccX.setValue(m_sAcc.fAccX, false);

			// AccY
			if (ValueChanged(AccY.getValue(), m_sAcc.fAccY))
				AccY.setValue(m_sAcc.fAccY, false);

			// AccZ
			if (ValueChanged(AccZ.getValue(), m_sAcc.fAccZ))
				AccZ.setValue(m_sAcc.fAccZ, false);

		// Compute orientation
		m_sAcc.CalculateOrientation();

			// OrientX
			if (ValueChanged(OrientX.getValue(), m_sAcc.fOrientX))
				OrientX.setValue(m_sAcc.fOrientX, false);

			// OrientY
			if (ValueChanged(OrientY.getValue(), m_sAcc.fOrientY))
				OrientY.setValue(m_sAcc.fOrientY, false);

			// OrientZ
			if (ValueChanged(OrientZ.getValue(), m_sAcc.fOrientZ))
				OrientZ.setValue(m_sAcc.fOrientZ, false);

			// Roll
			if (ValueChanged(Roll.getValue(), m_sAcc.fRoll))
				Roll.setValue(m_sAcc.fRoll, false);

			// Pitch
			if (ValueChanged(Pitch.getValue(), m_sAcc.fPitch))
				Pitch.setValue(m_sAcc.fPitch, false);
	}

	void WiiMote::OnReadIR()
	{
		// Get IR mode
		switch (m_nIRMode) {
			// Basic mode
			case IRBasic:
				m_sDots[0].bFound = !(m_pInputBuffer[6] == 0xff && m_pInputBuffer[ 7] == 0xff);
				m_sDots[1].bFound = !(m_pInputBuffer[9] == 0xff && m_pInputBuffer[10] == 0xff);
				if (m_sDots[1].bFound) {
					m_sDots[1].nRawX = m_pInputBuffer[ 9] | ((m_pInputBuffer[8] >> 0) & 0x03) << 8;
					m_sDots[1].nRawY = m_pInputBuffer[10] | ((m_pInputBuffer[8] >> 2) & 0x03) << 8;
				}
				m_sDots[0].nSize = 0;
				m_sDots[1].nSize = 0;
				break;
		
			// Extended mode
			case IRExtended:
				m_sDots[0].bFound = !(m_pInputBuffer[6]==0xff && m_pInputBuffer[ 7]==0xff && m_pInputBuffer[ 8]==0xff);
				m_sDots[1].bFound = !(m_pInputBuffer[9]==0xff && m_pInputBuffer[10]==0xff && m_pInputBuffer[11]==0xff);
				if (m_sDots[0].bFound)
					m_sDots[0].nSize = m_pInputBuffer[8] & 0x0f;
				if (m_sDots[1].bFound) {
					m_sDots[1].nRawX = m_pInputBuffer[ 9] | ((m_pInputBuffer[11] >> 4) & 0x03) << 8;
					m_sDots[1].nRawY = m_pInputBuffer[10] | ((m_pInputBuffer[11] >> 6) & 0x03) << 8;
					m_sDots[1].nSize = m_pInputBuffer[11] & 0x0f;
				}
				break;

			// Full mode
			case IRFull:
				// Unimplemented
				return;

			// Off/unknown
			case IROff:
			default:
				return;
		}

		// Check first dot
		if (m_sDots[0].bFound) {
			m_sDots[0].nRawX = m_pInputBuffer[6] | ((m_pInputBuffer[8] >> 4) & 0x03) << 8;
			m_sDots[0].nRawY = m_pInputBuffer[7] | ((m_pInputBuffer[8] >> 6) & 0x03) << 8;
			m_sDots[0].fX    = 1.f - (m_sDots[0].nRawX / static_cast<float>(IR_MaxX));
			m_sDots[0].fY    =	     (m_sDots[0].nRawY / static_cast<float>(IR_MaxY));
		}

		// Check second dot
		if (m_sDots[1].bFound) {
			m_sDots[1].fX = 1.f - (m_sDots[1].nRawX / static_cast<float>(IR_MaxX));
			m_sDots[1].fY =	      (m_sDots[1].nRawY / static_cast<float>(IR_MaxY));
		}

		// Compute IR center
		m_vIRPos[0] = (m_sDots[0].fX + m_sDots[1].fX) / 2;
		m_vIRPos[1] = (m_sDots[0].fY + m_sDots[1].fY) / 2;

			// PointerX
			if (ValueChanged(PointerX.getValue(), m_vIRPos[0]))
				PointerX.setValue(m_vIRPos[0], false);

			// PointerY
			if (ValueChanged(PointerY.getValue(), m_vIRPos[1]))
				PointerY.setValue(m_vIRPos[1], false);
	}

	void WiiMote::OnReadExtension(uint32_t nOffset)
	{
		// Check extension type
		switch (m_nExtension) {
			// Nunchuk
			case ExtNunchuk:
				OnReadNunchuk(nOffset);
				break;

			// Classic controller
			case ExtClassic:
				OnReadClassic(nOffset);
				break;

			case ExtNone:
			case ExtPartiallyInserted:
			default:
				// TODO(co) Review this: Do we need to handle this case?
				break;
		}
	}

	void WiiMote::OnReadNunchuk(uint32_t nOffset)
	{
		// Get buttons
		m_nNunchukButtons = 0;
		if ((m_pInputBuffer[nOffset+5] & 0x02) == 0)
			m_nNunchukButtons |= BtnNunchukC;
		if ((m_pInputBuffer[nOffset+5] & 0x01) == 0)
			m_nNunchukButtons |= BtnNunchukZ;

			// Button 'C'
			bool bPressed = ((m_nButtons & BtnNunchukC) != 0);
			if (NunchukButtonC.isPressed() != bPressed)
			{
				NunchukButtonC.setPressed(bPressed);
			}

			// Button 'Z'
			bPressed = ((m_nButtons & BtnNunchukZ) != 0);
			if (NunchukButtonZ.isPressed() != bPressed)
			{
				NunchukButtonZ.setPressed(bPressed);
			}

		// Get raw acceleration data
		uint8_t nRawX = m_pInputBuffer[nOffset+2];
		uint8_t nRawY = m_pInputBuffer[nOffset+3];
		uint8_t nRawZ = m_pInputBuffer[nOffset+4];

		// Compute acceleration
		m_sNunchukAcc.fAccX = (static_cast<float>(nRawX) - m_sNunchukAcc.nX0) / (static_cast<float>(m_sNunchukAcc.nXG) - m_sNunchukAcc.nX0);
		m_sNunchukAcc.fAccY = (static_cast<float>(nRawY) - m_sNunchukAcc.nY0) / (static_cast<float>(m_sNunchukAcc.nYG) - m_sNunchukAcc.nY0);
		m_sNunchukAcc.fAccZ = (static_cast<float>(nRawZ) - m_sNunchukAcc.nZ0) / (static_cast<float>(m_sNunchukAcc.nZG) - m_sNunchukAcc.nZ0);

			// AccX
			if (ValueChanged(NunchukAccX.getValue(), m_sNunchukAcc.fAccX))
				NunchukAccX.setValue(m_sNunchukAcc.fAccX, false);

			// AccY
			if (ValueChanged(NunchukAccY.getValue(), m_sNunchukAcc.fAccY))
				NunchukAccY.setValue(m_sNunchukAcc.fAccY, false);

			// AccZ
			if (ValueChanged(NunchukAccZ.getValue(), m_sNunchukAcc.fAccZ))
				NunchukAccZ.setValue(m_sNunchukAcc.fAccZ, false);

		// Compute orientation
		m_sNunchukAcc.CalculateOrientation();

			// OrientX
			if (ValueChanged(NunchukOrientX.getValue(), m_sNunchukAcc.fOrientX))
				NunchukOrientX.setValue(m_sNunchukAcc.fOrientX, false);

			// OrientY
			if (ValueChanged(NunchukOrientY.getValue(), m_sNunchukAcc.fOrientY))
				NunchukOrientY.setValue(m_sNunchukAcc.fOrientY, false);

			// OrientZ
			if (ValueChanged(NunchukOrientZ.getValue(), m_sNunchukAcc.fOrientZ))
				NunchukOrientZ.setValue(m_sNunchukAcc.fOrientZ, false);

			// Roll
			if (ValueChanged(NunchukRoll.getValue(), m_sNunchukAcc.fRoll))
				NunchukRoll.setValue(m_sNunchukAcc.fRoll, false);

			// Pitch
			if (ValueChanged(NunchukPitch.getValue(), m_sNunchukAcc.fPitch))
				NunchukPitch.setValue(m_sNunchukAcc.fPitch, false);

		// Get raw joystick position
		uint8_t nJoyRawX = m_pInputBuffer[nOffset+0];
		uint8_t nJoyRawY = m_pInputBuffer[nOffset+1];

		// Compute joystick position
		if (m_sNunchukJoy.nMaxX != 0x00) {
			m_sNunchukJoy.fX  = (static_cast<float>(nJoyRawX) - m_sNunchukJoy.nMidX) / (static_cast<float>(m_sNunchukJoy.nMaxX) - m_sNunchukJoy.nMinX);
			m_sNunchukJoy.fX *= 2.0f;
		}

		if (m_sNunchukJoy.nMaxY != 0x00) {
			m_sNunchukJoy.fY = (static_cast<float>(nJoyRawY) - m_sNunchukJoy.nMidY) / (static_cast<float>(m_sNunchukJoy.nMaxY) - m_sNunchukJoy.nMinY);
			m_sNunchukJoy.fY *= 2.0f;
		}

			// NunchukX
			if (ValueChanged(NunchukX.getValue(), m_sNunchukJoy.fX))
				NunchukX.setValue(m_sNunchukJoy.fX, false);

			// NunchukY
			if (ValueChanged(NunchukY.getValue(), m_sNunchukJoy.fY))
				NunchukY.setValue(m_sNunchukJoy.fY, false);
	}

	void WiiMote::OnReadClassic(uint32_t)
	{
		// TODO(co)
		/*
		// buttons:
		uint16_t bits = *(uint16_t*)(m_pInputBuffer+4);

		// joysticks:
		wiimote_state::joystick &joyL = Internal.ClassicController.JoystickL;
		wiimote_state::joystick &joyR = Internal.ClassicController.JoystickR;

		// copy the current joystick state to detect changes
		wiimote_state::joystick curr_joy_l = joyL;
		wiimote_state::joystick curr_joy_r = joyR;

		joyL.RawX = (float) (m_pInputBuffer[nOffset+0] & 0x3f);
		joyL.RawY = (float) (m_pInputBuffer[nOffset+1] & 0x3f);
		joyR.RawX = (float)((m_pInputBuffer[nOffset+2]			>> 7) |
						   ((m_pInputBuffer[nOffset+1] & 0xc0) >> 5) |
						   ((m_pInputBuffer[nOffset+0] & 0xc0) >> 3));
		joyR.RawY = (float) (m_pInputBuffer[nOffset+2] & 0x1f);

		float xr, yr, xl, yl;
		if (Internal.ClassicController.CalibrationInfo.MaxXL != 0x00)
			xl = ((float)joyL.RawX - Internal.ClassicController.CalibrationInfo.MidXL) / 
				 ((float)Internal.ClassicController.CalibrationInfo.MaxXL -
						 Internal.ClassicController.CalibrationInfo.MinXL);

		if (Internal.ClassicController.CalibrationInfo.MaxYL != 0x00)
			yl = ((float)joyL.RawY - Internal.ClassicController.CalibrationInfo.MidYL) / 
				 ((float)Internal.ClassicController.CalibrationInfo.MaxYL -
						 Internal.ClassicController.CalibrationInfo.MinYL);

		if (Internal.ClassicController.CalibrationInfo.MaxXR != 0x00)
			xr = ((float)joyR.RawX - Internal.ClassicController.CalibrationInfo.MidXR) / 
				 ((float)Internal.ClassicController.CalibrationInfo.MaxXR -
						 Internal.ClassicController.CalibrationInfo.MinXR);

		if (Internal.ClassicController.CalibrationInfo.MaxYR != 0x00)
			yr = ((float)joyR.RawY - Internal.ClassicController.CalibrationInfo.MidYR) / 
				 ((float)Internal.ClassicController.CalibrationInfo.MaxYR -
						 Internal.ClassicController.CalibrationInfo.MinYR);

		// i prefer the joystick outputs to range -1 - +1 (note this also affects
		//  the deadzone calculations)
		xl *= 2; yl *= 2; xr *= 2; yr *= 2;

		// apply the dead zones (if any) and write the final values to the state
		ApplyJoystickDeadZonesAndWrite(joyL, xl, yl);
		ApplyJoystickDeadZonesAndWrite(joyR, xr, yr);

		// have the joystick states changed?
		if (memcmp(&curr_joy_l, &joyL, sizeof(curr_joy_l)) != 0)
			changed |= CLASSIC_JOYSTICK_L_CHANGED;
		if (memcmp(&curr_joy_r, &joyR, sizeof(curr_joy_r)) != 0)
			changed |= CLASSIC_JOYSTICK_R_CHANGED;

		// triggers
		uint8_t raw_trigger_l = ((m_pInputBuffer[nOffset+2] & 0x60) >> 2) |
								(m_pInputBuffer[nOffset+3]		  >> 5);
		uint8_t raw_trigger_r =   m_pInputBuffer[nOffset+3] & 0x1f;
			
		if ((raw_trigger_l != Internal.ClassicController.RawTriggerL) ||
			(raw_trigger_r != Internal.ClassicController.RawTriggerR))
			   changed |= CLASSIC_TRIGGERS_CHANGED;
			
		Internal.ClassicController.RawTriggerL  = raw_trigger_l;
		Internal.ClassicController.RawTriggerR  = raw_trigger_r;

		if (Internal.ClassicController.CalibrationInfo.MaxTriggerL != 0x00)
			Internal.ClassicController.TriggerL =
					 (float)Internal.ClassicController.RawTriggerL / 
					((float)Internal.ClassicController.CalibrationInfo.MaxTriggerL -
							Internal.ClassicController.CalibrationInfo.MinTriggerL);

		if (Internal.ClassicController.CalibrationInfo.MaxTriggerR != 0x00)
			Internal.ClassicController.TriggerR =
					 (float)Internal.ClassicController.RawTriggerR / 
					((float)Internal.ClassicController.CalibrationInfo.MaxTriggerR -
							Internal.ClassicController.CalibrationInfo.MinTriggerR);
		*/
	}

	void WiiMote::ReadMemory(int nAddress, uint8_t nSize)
	{
		// Send command
		ClearReport();
		m_pOutputBuffer[0] = CmdReadMemory;
		m_pOutputBuffer[1] = static_cast<uint8_t>(((nAddress & 0xff000000) >> 24) | m_nRumble);
		m_pOutputBuffer[2] = static_cast<uint8_t>((nAddress	 & 0x00ff0000) >> 16);
		m_pOutputBuffer[3] = static_cast<uint8_t>((nAddress	 & 0x0000ff00) >>  8);
		m_pOutputBuffer[4] = static_cast<uint8_t>((nAddress	 & 0x000000ff));
		m_pOutputBuffer[5] = static_cast<uint8_t>((nSize	 & 0xff00	 ) >>  8);
		m_pOutputBuffer[6] = static_cast<uint8_t>((nSize	 & 0xff));
		Send(m_pOutputBuffer, 7);
	}

	void WiiMote::WriteMemory(int nAddress, const uint8_t* pBuffer, uint8_t nSize)
	{
		// Check buffer
		if (pBuffer && nSize <= 16) {
			// Clear write buffer
			memset(m_pOutputBuffer, 0, m_pConnectionDevice->GetOutputReportSize());

			// Set command
			ClearReport();
			m_pOutputBuffer[0] = CmdWriteMemory;
			m_pOutputBuffer[1] = static_cast<uint8_t>(((nAddress & 0xff000000) >> 24) | m_nRumble);
			m_pOutputBuffer[2] = static_cast<uint8_t>( (nAddress & 0x00ff0000) >> 16);
			m_pOutputBuffer[3] = static_cast<uint8_t>( (nAddress & 0x0000ff00) >>  8);
			m_pOutputBuffer[4] = static_cast<uint8_t>( (nAddress & 0x000000ff));
			m_pOutputBuffer[5] = nSize;
			memcpy(m_pOutputBuffer+6, pBuffer, nSize);

			// Send command
			m_pConnectionDevice->Write(m_pOutputBuffer, m_pConnectionDevice->GetOutputReportSize());
		}
	}

	void WiiMote::DecryptBuffer(uint32_t nOffset, uint32_t nSize)
	{
		// Decrypt buffer
		for (unsigned i=0; i<nSize; i++)
			m_pInputBuffer[nOffset+i] = (((m_pInputBuffer[nOffset+i] ^ 0x17u) + 0x17u) & 0xffu);
	}

	void WiiMote::Send(uint8_t *pBuffer, uint32_t nSize)
	{
		// This is kinda strange. On Windows, using the HID API, a whole output report of size 22 has to be sent to the device,
		// otherwise strange things happen. On Linux, using the BlueZ API, only as many data as needed has to be sent, otherwise
		// we get a ResultErrInvalidParameter error.

		// Send data to device
		if (m_pConnectionDevice->GetDeviceType() == ConnectionDevice::DeviceTypeBluetooth)
			m_pConnectionDevice->Write(pBuffer, nSize);
		else
			m_pConnectionDevice->Write(pBuffer, 22);
	}

	void WiiMote::SendStatusRequest()
	{
		ClearReport();
		m_pOutputBuffer[0] = CmdStatus;
		m_pOutputBuffer[1] = m_nRumble;
		Send(m_pOutputBuffer, 2);
	}

	void WiiMote::SendCalibrationRequest()
	{
		ReadMemory(RegCalibration, 7);
	}

	void WiiMote::SAcceleration::CalculateOrientation()
	{
		// Calculate orientation from acceleration data
		static constexpr float fEpsilon = 0.2f;
		float fSquareLen =  fAccX*fAccX + fAccY*fAccY + fAccZ*fAccZ;
		if ((fSquareLen >= (1.0f - fEpsilon)) && (fSquareLen <= (1.0f + fEpsilon))) {
			// Is the acceleration near 1G for at least 2 update cycles?
			if (++nUpdateNearG >= 2) {
				// Normalize vector
				float inv_len = 1.f / sqrtf(fSquareLen);
				fOrientX = fAccX * inv_len;
				fOrientY = fAccY * inv_len;
				fOrientZ = fAccZ * inv_len;

				// Get pitch and roll angles
				fPitch = -asinf(fOrientY) * 57.2957795f;
				fRoll  =  asinf(fOrientX) * 57.2957795f;
				if (fOrientZ < 0) {
					fPitch = (fOrientY < 0 ?  180 - fPitch : -180 - fPitch);
					fRoll  = (fOrientX < 0 ? -180 - fRoll  :  180 - fRoll);
				}
			}
		} else {
			// Reset update counter
			nUpdateNearG = 0;
		}
	}




	//[-------------------------------------------------------]
	//[ PLInput::SensorManager                                ]
	//[-------------------------------------------------------]
	//[-------------------------------------------------------]
	//[ Public functions                                      ]
	//[-------------------------------------------------------]
	SensorManager::SensorManager(InputManager& inputManager, const std::string &sName, DeviceImpl *pImpl) :
		Device(inputManager, sName, "Sensor manager input controller", pImpl),
		// Accelerometer
		AccelerationX	(*this,	"AccelerationX",	"X acceleration axis (Accelerometer)"),
		AccelerationY	(*this,	"AccelerationY",	"Y acceleration axis (Accelerometer)"),
		AccelerationZ	(*this,	"AccelerationZ",	"Z acceleration axis (Accelerometer)"),
		// Magnetic field
		MagneticX		(*this,	"MagneticX",		"X magnetic axis (Magnetic field)"),
		MagneticY		(*this,	"MagneticY",		"Y magnetic axis (Magnetic field)"),
		MagneticZ		(*this,	"MagneticZ",		"Z magnetic axis (Magnetic field)"),
		// Gyroscope
		RotationX		(*this,	"RotationX",		"X rotation axis (Gyroscope)"),
		RotationY		(*this,	"RotationY",		"Y rotation axis (Gyroscope)"),
		RotationZ		(*this,	"RotationZ",		"Z rotation axis (Gyroscope)"),
		// Light
		Light			(*this,	"Light",			"Light"),
		// Proximity
		Proximity		(*this,	"Proximity",		"Proximity")
	{
		// Nothing here
	}


	//[-------------------------------------------------------]
	//[ Public virtual Controller functions                   ]
	//[-------------------------------------------------------]
	void SensorManager::Update()
	{
		// Update device backend
		if (m_pImpl && m_pImpl->getDeviceBackendType() == DeviceBackendType::UPDATE_DEVICE)
		{
			static_cast<UpdateDevice*>(m_pImpl)->Update();
		}
	}




	//[-------------------------------------------------------]
	//[ PLInput::SplitTouchPad                                ]
	//[-------------------------------------------------------]
	//[-------------------------------------------------------]
	//[ Public functions                                      ]
	//[-------------------------------------------------------]
	SplitTouchPad::SplitTouchPad(InputManager& inputManager, const std::string &sName, DeviceImpl *pImpl) :
		Device(inputManager, sName, "Gamepad device emulation by using a touch screen making it possible to e.g. move & look at the same time", pImpl),
		LeftX	(*this, "LeftX",	"Absolute x axis on the left touchscreen side"),
		LeftY	(*this, "LeftY",	"Absolute y axis on the left touchscreen side"),
		RightX	(*this, "RightX",	"Absolute x axis on the right touchscreen side"),
		RightY	(*this, "RightY",	"Absolute y axis on the right touchscreen side")
	{
	}


	//[-------------------------------------------------------]
	//[ Public virtual Controller functions                   ]
	//[-------------------------------------------------------]
	void SplitTouchPad::Update()
	{
		// Update device backend
		if (m_pImpl && m_pImpl->getDeviceBackendType() == DeviceBackendType::UPDATE_DEVICE)
		{
			static_cast<UpdateDevice*>(m_pImpl)->Update();
		}
	}




	//[-------------------------------------------------------]
	//[ PLInput::VirtualStandardController                    ]
	//[-------------------------------------------------------]
	//[-------------------------------------------------------]
	//[ Public functions                                      ]
	//[-------------------------------------------------------]
	VirtualStandardController::VirtualStandardController(InputManager& inputManager) :
		VirtualController(inputManager, "VirtualStandardController", "Standard virtual input controller"),
		// Mouse
		MouseX						(*this, "MouseX",						"X axis (movement data, no absolute data)"),
		MouseY						(*this, "MouseY",						"Y axis (movement data, no absolute data)"),
		MouseWheel					(*this, "MouseWheel",					"Mouse wheel (movement data, no absolute data)"),
		MouseLeft					(*this, "MouseLeft",					"Left mouse button (mouse button #0)"),
		MouseRight					(*this, "MouseRight",					"Right mouse button (mouse button #1)"),
		MouseMiddle					(*this, "MouseMiddle",					"Middle mouse button (mouse button #2)"),
		MouseButton4				(*this, "MouseButton4",					"Mouse button #4"),
		MouseButton5				(*this, "MouseButton5",					"Mouse button #5"),
		MouseButton6				(*this, "MouseButton6",					"Mouse button #6"),
		MouseButton7				(*this, "MouseButton7",					"Mouse button #7"),
		MouseButton8				(*this, "MouseButton8",					"Mouse button #8"),
		MouseButton9				(*this, "MouseButton9",					"Mouse button #9"),
		MouseButton10				(*this, "MouseButton10",				"Mouse button #10"),
		MouseButton11				(*this, "MouseButton11",				"Mouse button #11"),
		MouseButton12				(*this, "MouseButton12",				"Mouse button #12"),
		// Keyboard
		KeyboardBackspace			(*this, "KeyboardBackspace",			"Backspace",																	0x08),
		KeyboardTab					(*this, "KeyboardTab",					"Tabulator",																	0x09),
		KeyboardClear				(*this, "KeyboardClear",				"Clear (not available everywhere)",												0x00),
		KeyboardReturn				(*this, "KeyboardReturn",				"Return (often the same as \"Enter\")",											0x0D),
		KeyboardShift				(*this, "KeyboardShift",				"Shift",																		0x00),
		KeyboardControl				(*this, "KeyboardControl",				"Control (\"Ctrl\")",															0x00),
		KeyboardAlt					(*this, "KeyboardAlt",					"Alt",																			0x00),
		KeyboardPause				(*this, "KeyboardPause",				"Pause",																		0x00),
		KeyboardCapsLock			(*this, "KeyboardCapsLock",				"Caps lock",																	0x00),
		KeyboardEscape				(*this, "KeyboardEscape",				"Escape",																		0x1B),
		KeyboardSpace				(*this, "KeyboardSpace",				"Space",																		0x20),
		KeyboardPageUp				(*this, "KeyboardPageUp",				"Page up",																		0x00),
		KeyboardPageDown			(*this, "KeyboardPageDown",				"Page down",																	0x00),
		KeyboardEnd					(*this, "KeyboardEnd",					"End",																			0x00),
		KeyboardHome				(*this, "KeyboardHome",					"Home",																			0x00),
		KeyboardLeft				(*this, "KeyboardLeft",					"Left arrow",																	0x00),
		KeyboardUp					(*this, "KeyboardUp",					"Up arrow",																		0x00),
		KeyboardRight				(*this, "KeyboardRight",				"Right arrow",																	0x00),
		KeyboardDown				(*this, "KeyboardDown",					"Down arrow",																	0x00),
		KeyboardSelect				(*this, "KeyboardSelect",				"Select (not available everywhere)",											0x00),
		KeyboardExecute				(*this, "KeyboardExecute",				"Execute (not available everywhere)",											0x00),
		KeyboardPrint				(*this, "KeyboardPrint",				"Print screen",																	0x00),
		KeyboardInsert				(*this, "KeyboardInsert",				"Insert",																		0x00),
		KeyboardDelete				(*this, "KeyboardDelete",				"Delete",																		0x7F),
		KeyboardHelp				(*this, "KeyboardHelp",					"Help (not available everywhere)",												0x00),
		Keyboard0					(*this, "Keyboard0",					"0",																			'0'),
		Keyboard1					(*this, "Keyboard1",					"1",																			'1'),
		Keyboard2					(*this, "Keyboard2",					"2",																			'2'),
		Keyboard3					(*this, "Keyboard3",					"3",																			'3'),
		Keyboard4					(*this, "Keyboard4",					"4",																			'4'),
		Keyboard5					(*this, "Keyboard5",					"5",																			'5'),
		Keyboard6					(*this, "Keyboard6",					"6",																			'6'),
		Keyboard7					(*this, "Keyboard7",					"7",																			'7'),
		Keyboard8					(*this, "Keyboard8",					"8",																			'8'),
		Keyboard9					(*this, "Keyboard9",					"9",																			'9'),
		KeyboardA					(*this, "KeyboardA",					"A",																			'a'),
		KeyboardB					(*this, "KeyboardB",					"B",																			'b'),
		KeyboardC					(*this, "KeyboardC",					"C",																			'c'),
		KeyboardD					(*this, "KeyboardD",					"D",																			'd'),
		KeyboardE					(*this, "KeyboardE",					"E",																			'e'),
		KeyboardF					(*this, "KeyboardF",					"F",																			'f'),
		KeyboardG					(*this, "KeyboardG",					"G",																			'g'),
		KeyboardH					(*this, "KeyboardH",					"H",																			'h'),
		KeyboardI					(*this, "KeyboardI",					"I",																			'i'),
		KeyboardJ					(*this, "KeyboardJ",					"J",																			'j'),
		KeyboardK					(*this, "KeyboardK",					"K",																			'k'),
		KeyboardL					(*this, "KeyboardL",					"L",																			'l'),
		KeyboardM					(*this, "KeyboardM",					"M",																			'm'),
		KeyboardN					(*this, "KeyboardN",					"N",																			'n'),
		KeyboardO					(*this, "KeyboardO",					"O",																			'o'),
		KeyboardP					(*this, "KeyboardP",					"P",																			'p'),
		KeyboardQ					(*this, "KeyboardQ",					"Q",																			'q'),
		KeyboardR					(*this, "KeyboardR",					"R",																			'r'),
		KeyboardS					(*this, "KeyboardS",					"S",																			's'),
		KeyboardT					(*this, "KeyboardT",					"T",																			't'),
		KeyboardU					(*this, "KeyboardU",					"U",																			'u'),
		KeyboardV					(*this, "KeyboardV",					"V",																			'v'),
		KeyboardW					(*this, "KeyboardW",					"W",																			'w'),
		KeyboardX					(*this, "KeyboardX",					"X",																			'x'),
		KeyboardY					(*this, "KeyboardY",					"Y",																			'y'),
		KeyboardZ					(*this, "KeyboardZ",					"Z",																			'z'),
		KeyboardNumpad0				(*this, "KeyboardNumpad0",				"Numpad 0",																		'0'),
		KeyboardNumpad1				(*this, "KeyboardNumpad1",				"Numpad 1",																		'1'),
		KeyboardNumpad2				(*this, "KeyboardNumpad2",				"Numpad 2",																		'2'),
		KeyboardNumpad3				(*this, "KeyboardNumpad3",				"Numpad 3",																		'3'),
		KeyboardNumpad4				(*this, "KeyboardNumpad4",				"Numpad 4",																		'4'),
		KeyboardNumpad5				(*this, "KeyboardNumpad5",				"Numpad 5",																		'5'),
		KeyboardNumpad6				(*this, "KeyboardNumpad6",				"Numpad 6",																		'6'),
		KeyboardNumpad7				(*this, "KeyboardNumpad7",				"Numpad 7",																		'7'),
		KeyboardNumpad8				(*this, "KeyboardNumpad8",				"Numpad 8",																		'8'),
		KeyboardNumpad9				(*this, "KeyboardNumpad9",				"Numpad 9",																		'9'),
		KeyboardNumpadMultiply		(*this, "KeyboardNumpadMultiply",		"Numpad multiply",																'*'),
		KeyboardNumpadAdd			(*this, "KeyboardNumpadAdd",			"Numpad add",																	'+'),
		KeyboardNumpadSeparator		(*this, "KeyboardNumpadSeparator",		"Numpad separator",																','),
		KeyboardNumpadSubtract		(*this, "KeyboardNumpadSubtract",		"Numpad subtract",																'-'),
		KeyboardNumpadDecimal		(*this, "KeyboardNumpadDecimal",		"Numpad decimal",																'.'),
		KeyboardNumpadDivide		(*this, "KeyboardNumpadDivide",			"Numpad divide",																'/'),
		KeyboardF1					(*this, "KeyboardF1",					"F1",																			0x00),
		KeyboardF2					(*this, "KeyboardF2",					"F2",																			0x00),
		KeyboardF3					(*this, "KeyboardF3",					"F3",																			0x00),
		KeyboardF4					(*this, "KeyboardF4",					"F4",																			0x00),
		KeyboardF5					(*this, "KeyboardF5",					"F5",																			0x00),
		KeyboardF6					(*this, "KeyboardF6",					"F6",																			0x00),
		KeyboardF7					(*this, "KeyboardF7",					"F7",																			0x00),
		KeyboardF8					(*this, "KeyboardF8",					"F8",																			0x00),
		KeyboardF9					(*this, "KeyboardF9",					"F9",																			0x00),
		KeyboardF10					(*this, "KeyboardF10",					"F10",																			0x00),
		KeyboardF11					(*this, "KeyboardF11",					"F11",																			0x00),
		KeyboardF12					(*this, "KeyboardF12",					"F12",																			0x00),
		KeyboardNumLock				(*this, "KeyboardNumLock",				"Num lock",																		0x00),
		KeyboardScrollLock			(*this, "KeyboardScrollLock",			"Scroll lock",																	0x00),
		KeyboardCircumflex			(*this, "KeyboardCircumflex",			"Circumflex (^)",																0x00),
		KeyboardLeftWindows			(*this, "KeyboardLeftWindows",			"Left Windows key",																0x00),
		KeyboardRightWindows		(*this, "KeyboardRightWindows",			"Right Windows key",															0x00),
		KeyboardApplications		(*this, "KeyboardApplications",			"Applications key (natural keyboard)",											0x00),
		KeyboardF13					(*this, "KeyboardF13",					"F13",																			0x00),
		KeyboardF14					(*this, "KeyboardF14",					"F14",																			0x00),
		KeyboardF15					(*this, "KeyboardF15",					"F15",																			0x00),
		KeyboardF16					(*this, "KeyboardF16",					"F16",																			0x00),
		KeyboardF17					(*this, "KeyboardF17",					"F17",																			0x00),
		KeyboardF18					(*this, "KeyboardF18",					"F18",																			0x00),
		KeyboardF19					(*this, "KeyboardF19",					"F19",																			0x00),
		KeyboardF20					(*this, "KeyboardF20",					"F20",																			0x00),
		KeyboardF21					(*this, "KeyboardF21",					"F21",																			0x00),
		KeyboardF22					(*this, "KeyboardF22",					"F22",																			0x00),
		KeyboardF23					(*this, "KeyboardF23",					"F23",																			0x00),
		KeyboardF24					(*this, "KeyboardF24",					"F24",																			0x00),
		KeyboardLeftShift			(*this, "KeyboardLeftShift",			"Left shift",																	0x00),
		KeyboardRightShift			(*this, "KeyboardRightShift",			"Right shift",																	0x00),
		KeyboardLeftControl			(*this, "KeyboardLeftControl",			"Left control",																	0x00),
		KeyboardRightControl		(*this, "KeyboardRightControl",			"Right control",																0x00),
		KeyboardVolumeMute			(*this, "KeyboardVolumeMute",			"Volume mute",																	0x00),
		KeyboardVolumeDown			(*this, "KeyboardVolumeDown",			"Volume down",																	0x00),
		KeyboardVolumeUp			(*this, "KeyboardVolumeUp",				"Volume up",																	0x00),
		KeyboardMediaNextTrack		(*this, "KeyboardMediaNextTrack",		"Media next track",																0x00),
		KeyboardMediaPreviousTrack	(*this, "KeyboardMediaPreviousTrack",	"Media previous track",															0x00),
		KeyboardMediaStop			(*this, "KeyboardMediaStop",			"Media stop",																	0x00),
		KeyboardMediaPlayPause		(*this, "KeyboardMediaPlayPause",		"Media play pause",																0x00),
		KeyboardAdd					(*this, "KeyboardAdd",					"For any country/region, the '+' key",											 '+'),
		KeyboardSeparator			(*this, "KeyboardSeparator",			"For any country/region, the ',' key",											 ','),
		KeyboardSubtract			(*this, "KeyboardSubtract",				"For any country/region, the '-' key",											 '-'),
		KeyboardDecimal				(*this, "KeyboardDecimal",				"For any country/region, the '.' key",											 '.'),
		KeyboardOEM1				(*this, "KeyboardOEM1",					"For the US standard keyboard, the ';:' key",									0x00),
		KeyboardOEM2				(*this, "KeyboardOEM2",					"For the US standard keyboard, the '/?' key",									0x00),
		KeyboardOEM3				(*this, "KeyboardOEM3",					"For the US standard keyboard, the '`~' key",									0x00),
		KeyboardOEM4				(*this, "KeyboardOEM4",					"For the US standard keyboard, the '[{' key",									0x00),
		KeyboardOEM5				(*this, "KeyboardOEM5",					"For the US standard keyboard, the 'backslash|' key",							0x00),
		KeyboardOEM6				(*this, "KeyboardOEM6",					"For the US standard keyboard, the ']}' key",									0x00),
		KeyboardOEM7				(*this, "KeyboardOEM7",					"For the US standard keyboard, the 'single-quote/double-quote' key",			0x00),
		KeyboardOEM8				(*this, "KeyboardOEM8",					"Used for miscellaneous characters; it can vary by keyboard",					0x00),
		KeyboardOEM102				(*this, "KeyboardOEM102",				"Either the angle bracket key or the backslash key on the RT 102-key keyboard",	0x00),
		// Main character controls
		TransX						(*this, "TransX",						"X translation axis: Strafe left/right (+/-)"),
		TransY						(*this, "TransY",						"Y translation axis: Move up/down (+/-)"),
		TransZ						(*this, "TransZ",						"Z translation axis: Move forwards/backwards (+/-)"),
		Pan							(*this, "Pan",							"Keep pressed to pan",															0x00),
		PanX						(*this, "PanX",							"X pan translation axis: Strafe left/right (+/-)"),
		PanY						(*this, "PanY",							"Y pan translation axis: Move up/down (+/-)"),
		PanZ						(*this, "PanZ",							"Z pan translation axis: Move forwards/backwards (+/-)"),
		RotX						(*this, "RotX",							"X rotation axis: Pitch (also called 'bank') change is moving the nose down and the tail up (or vice-versa)"),
		RotY						(*this, "RotY",							"Y rotation axis: Yaw (also called 'heading') change is turning to the left or right"),
		RotZ						(*this, "RotZ",							"Z rotation axis: Roll (also called 'attitude') change is moving one wingtip up and the other down"),
		Rotate						(*this, "Rotate",						"Keep pressed to rotate",														0x00),
		RotateSlow					(*this, "RotateSlow",					"Keep pressed to rotate slowly",												0x00),
		Forward						(*this, "Forward",						"Move forwards",																0x00),
		Backward					(*this, "Backward",						"Move backwards",																0x00),
		Left						(*this, "Left",							"Move (rotate) left",															0x00),
		Right						(*this, "Right",						"Move (rotate) right",															0x00),
		StrafeLeft					(*this, "StrafeLeft",					"Strafe left",																	0x00),
		StrafeRight					(*this, "StrafeRight",					"Strafe right",																	0x00),
		Up							(*this, "Up",							"Move up",																		0x00),
		Down						(*this, "Down",							"Move down",																	0x00),
		Run							(*this, "Run",							"Keep pressed to run",															0x00),
		Sneak						(*this, "Sneak",						"Keep pressed to sneak",														0x00),
		Crouch						(*this, "Crouch",						"Keep pressed to crouch",														0x00),
		Jump						(*this, "Jump",							"Jump",																			0x00),
		Zoom						(*this, "Zoom",							"Keep pressed to zoom",															0x00),
		ZoomAxis					(*this, "ZoomAxis",						"Zoom axis to zoom in or out (+/-)"),
		Button1						(*this, "Button1",						"Button for action #1",															0x00),
		Button2						(*this, "Button2",						"Button for action #2",															0x00),
		Button3						(*this, "Button3",						"Button for action #3",															0x00),
		Button4						(*this, "Button4",						"Button for action #4",															0x00),
		Button5						(*this, "Button5",						"Button for action #5",															0x00),
		// Interaction
		Pickup						(*this, "Pickup",						"Keep pressed to pickup",														0x00),
		Throw						(*this, "Throw",						"Throw the picked object",														0x00),
		IncreaseForce				(*this, "IncreaseForce",				"Keep pressed to increase the force applied to the picked object",				0x00),
		DecreaseForce				(*this, "DecreaseForce",				"Keep pressed to decrease the force applied to the picked object",				0x00),
		PushPull					(*this, "PushPull",						"Used to push/pull the picked object")
	{
		ConnectToDevices();
	}


	//[-------------------------------------------------------]
	//[ Public virtual VirtualStandardController functions    ]
	//[-------------------------------------------------------]
	void VirtualStandardController::ConnectToDevices()
	{
		// Connect mouse
		ConnectAll(mInputManager.GetMouse(), "Mouse", "");

		// Connect keyboard
		ConnectAll(mInputManager.GetKeyboard(), "Keyboard", "");

		// Connect character controls
		std::vector<Device*> &lstDevices = mInputManager.GetDevices();
		for (uint32_t i=0; i<lstDevices.size(); i++) {
			// Get device
			Device *pDevice = lstDevices[i];

			// Mouse
			if (pDevice->GetName() == "Mouse") {
				// Get mouse
				Mouse *pMouse = static_cast<Mouse*>(pDevice);

				// Movement
				// Keep pressed to pan
				Connect("Pan",			pMouse->Middle);
				// RotX: Pitch (also called 'bank') change is moving the nose down and the tail up (or vice-versa)
				Connect("RotX",			pMouse->X);
				// RotY: Yaw (also called 'heading') change is turning to the left or right
				Connect("RotY",			pMouse->Y);
				// Keep pressed to rotate
				Connect("Rotate",		pMouse->Right);
				// Pan x
				Connect("PanX",			pMouse->X, -0.05f);
				// Pan y
				Connect("PanY",			pMouse->Y, -0.05f);

				// Zoom
				Connect("Zoom",			pMouse->Middle);
				Connect("ZoomAxis",		pMouse->Wheel, 0.01f);
				Connect("ZoomAxis",		pMouse->Y, -0.1f);

				// Buttons
				Connect("MouseWheel",	pMouse->Wheel);
				Connect("Button1",		pMouse->Left);
				Connect("Button2",		pMouse->Right);
				Connect("Button3",		pMouse->Middle);
				Connect("Button4",		pMouse->Button4);
				Connect("Button5",		pMouse->Button5);

				// Interaction
				Connect("Pickup",		pMouse->Middle);
				Connect("PushPull",		pMouse->Wheel, 0.001f);
			}

			// Keyboard
			else if (pDevice->GetName() == "Keyboard") {
				// Get keyboard
				Keyboard *pKeyboard = static_cast<Keyboard*>(pDevice);

				// Keep pressed to rotate slowly
				Connect("RotateSlow",		pKeyboard->Q);

				// WASD
				Connect("Forward",			pKeyboard->W);
				Connect("Backward",			pKeyboard->S);
				Connect("StrafeLeft",		pKeyboard->A);
				Connect("StrafeRight",		pKeyboard->D);
				Connect("Left",				pKeyboard->Q);
				Connect("Right",			pKeyboard->E);

				// Cursor keys
				Connect("Forward",			pKeyboard->Up);
				Connect("Backward",			pKeyboard->Down);
				Connect("StrafeLeft",		pKeyboard->Left);
				Connect("StrafeRight",		pKeyboard->Right);
				Connect("Left",				pKeyboard->Left);
				Connect("Right",			pKeyboard->Right);

				// Look up/down
				Connect("Up",				pKeyboard->PageUp);
				Connect("Down",				pKeyboard->PageDown);

				// Run/sneak/crouch/jump
				Connect("Run",				pKeyboard->Shift);
				Connect("Sneak",			pKeyboard->Control);
				Connect("Crouch",			pKeyboard->C);
				Connect("Jump",				pKeyboard->Space);

				// Buttons
				Connect("Button1",			pKeyboard->Space);
				Connect("Button2",			pKeyboard->Return);
				Connect("Button3",			pKeyboard->Backspace);
				Connect("Button4",			pKeyboard->Alt);
				Connect("Button5",			pKeyboard->Circumflex);

				// Interaction
				Connect("Throw",			pKeyboard->T);
				Connect("IncreaseForce",	pKeyboard->Shift);
				Connect("DecreaseForce",	pKeyboard->Control);
			}

			// Joystick
			else if (pDevice->GetName().find("Joystick") != std::string::npos) {
				// Get joystick
				Joystick *pJoystick = static_cast<Joystick*>(pDevice);
				static constexpr float ROTATION_SCALE = 500.0f;

				// TODO(co) Movement

				// Rotation
				// RotX: Pitch (also called 'bank') change is moving the nose down and the tail up (or vice-versa)
				Connect("RotX",		pJoystick->X, ROTATION_SCALE);
				// RotY: Yaw (also called 'heading') change is turning to the left or right
				Connect("RotY",		pJoystick->Y, ROTATION_SCALE);

				// Buttons
				Connect("Button1",	pJoystick->Button0);
				Connect("Button2",	pJoystick->Button1);
				Connect("Button3",	pJoystick->Button2);
				Connect("Button4",	pJoystick->Button3);
				Connect("Button5",	pJoystick->Button4);
			}

			// SpaceMouse
			else if (pDevice->GetName().find("SpaceMouse") != std::string::npos) {
				// Get space mouse
				SpaceMouse *pSpaceMouse = static_cast<SpaceMouse*>(pDevice);
				static constexpr float ROTATION_SCALE = 1.5f;
				static constexpr float TRANSFORM_SCALE = -0.004f;

				// Movement
				// RotX: Yaw (also called 'heading', change is turning to the left or right)
				Connect("RotX",		pSpaceMouse->RotZ,    ROTATION_SCALE);
				// RotY: Pitch (also called 'bank', change is moving the nose down and the tail up or vice-versa)
				Connect("RotY",		pSpaceMouse->RotX,   -ROTATION_SCALE);
				// RotZ: Roll (also called 'attitude') change is moving one wingtip up and the other down
				Connect("RotZ",		pSpaceMouse->RotY,   -ROTATION_SCALE);
				// X translation axis: Strafe left/right (+/-)
				Connect("TransX",	pSpaceMouse->TransX, TRANSFORM_SCALE);
				// Y translation axis: Move up/down (+/-)
				Connect("TransY",	pSpaceMouse->TransZ, TRANSFORM_SCALE);
				// Z translation axis: Move forwards/backwards (+/-)
				Connect("TransZ",	pSpaceMouse->TransY, TRANSFORM_SCALE);
				// X pan translation axis: Strafe left/right (+/-)
				Connect("PanX",		pSpaceMouse->TransX, TRANSFORM_SCALE);
				// Y pan translation axis: Move up/down (+/-)
				Connect("PanY",		pSpaceMouse->TransZ, TRANSFORM_SCALE);
				// Z pan translation axis: Move forwards/backwards (+/-)
				Connect("PanZ",		pSpaceMouse->TransY, TRANSFORM_SCALE);

				// Zoom
				Connect("ZoomAxis",	pSpaceMouse->TransY, TRANSFORM_SCALE);

				// Buttons
				Connect("Button1",	pSpaceMouse->Button0);
				Connect("Button2",	pSpaceMouse->Button1);
				Connect("Button3",	pSpaceMouse->Button2);
				Connect("Button4",	pSpaceMouse->Button3);
				Connect("Button5",	pSpaceMouse->Button4);
			}

			// Splitscreen touchpad device
			else if (pDevice->GetName().find("SplitTouchPad") != std::string::npos) {
				// Get splitscreen touchpad device
				SplitTouchPad *pSplitTouchPad = static_cast<SplitTouchPad*>(pDevice);

				// Movement
				// RotX: Pitch (also called 'bank') change is moving the nose down and the tail up (or vice-versa)
				Connect("RotY",		pSplitTouchPad->RightX, -0.5f);
				// RotY: Yaw (also called 'heading') change is turning to the left or right
				Connect("RotX",		pSplitTouchPad->RightY, 0.5f);
				// X translation axis: Strafe left/right (+/-)
				Connect("TransX",	pSplitTouchPad->LeftX,  -0.01f);
				// Y translation axis: Move up/down (+/-)
				Connect("TransZ",	pSplitTouchPad->LeftY,  -0.01f);
			}

			// WiiMote
			else if (pDevice->GetName().find("WiiMote") != std::string::npos) {
				// Get WiiMote
				WiiMote *pWiiMote = static_cast<WiiMote*>(pDevice);

				// Movement
				// TODO(co) We need some more logic here to calculate movement from WiiMote data

				// Cursor keys
				Connect("Forward",	pWiiMote->ButtonUp);
				Connect("Backward",	pWiiMote->ButtonDown);
				Connect("Left",		pWiiMote->ButtonLeft);
				Connect("Right",	pWiiMote->ButtonRight);

				// Buttons
				Connect("Button1",	pWiiMote->ButtonA);
				Connect("Button2",	pWiiMote->ButtonB);
				Connect("Button3",	pWiiMote->Button1);
				Connect("Button4",	pWiiMote->Button2);
				Connect("Button5",	pWiiMote->ButtonHome);
			}
		}
	}




	//[-------------------------------------------------------]
	//[ PLInput::InputManager                                 ]
	//[-------------------------------------------------------]
	//[-------------------------------------------------------]
	//[ Public functions                                      ]
	//[-------------------------------------------------------]
	InputManager::InputManager() :
		mMutex(new std::mutex())
	{
		DetectDevices();
	}

	InputManager::~InputManager()
	{
		// Shut down
		Clear();

		// Destroy mutex
		delete mMutex;
	}

	void InputManager::Update()
	{
		Controls lstUpdatedControls;
		{
			std::lock_guard<std::mutex> mutexLock(*mMutex);

			// Copy list of controls that have changed
			lstUpdatedControls = m_lstUpdatedControls;

			// Clear list
			m_lstUpdatedControls.clear();
		}

		// Inform controllers about changed controls
		for (size_t i=0; i<lstUpdatedControls.size(); i++) {
			// Inform controller
			Control* pControl = lstUpdatedControls[i];
			pControl->getController().InformControl(pControl);
		}

		// Update devices
		for (uint32_t i=0; i<m_lstDevices.size(); i++)
			m_lstDevices[i]->Update();
	}

	void InputManager::DetectDevices(bool bReset)
	{
		std::lock_guard<std::mutex> mutexLock(*mMutex);

		// Delete all existing providers and devices?
		if (bReset)
			Clear();

		// Query available input providers
		// TODO(co) Add provider factory
		DetectProvider("PLInput::HIDProvider", bReset);
		DetectProvider("PLInput::BluetoothProvider", bReset);
		#if defined(WIN32)
			// "PLInput::LegacyJoystickProvider" must be detected after InputHID, otherwise everything will be detected just as joysticks
			DetectProvider("PLInput::RawInputProvider", bReset);
			DetectProvider("PLInput::LegacyJoystickProvider", bReset);
		#elif defined(LINUX)
			DetectProvider("PLInput::LinuxProvider", bReset);
		#elif defined(APPLE)
			DetectProvider("PLInput::MacOSXProvider", bReset);
		#elif defined(ANDROID)
			DetectProvider("PLInput::AndroidProvider", bReset);
		#else
			#error "Unsupported platform"
		#endif
	}


	//[-------------------------------------------------------]
	//[ Private functions                                     ]
	//[-------------------------------------------------------]
	void InputManager::Clear()
	{
		// Destroy all input providers
		for (uint32_t i=0; i<m_lstProviders.size(); i++)
			delete m_lstProviders[i];
		m_lstProviders.clear();
		m_mapProviders.clear();

		// Destroy all left-over input devices (usually, all devices should have been destroyed by their providers)
		for (uint32_t i=0; i<m_lstDevices.size(); i++)
			delete m_lstDevices[i];
		m_lstDevices.clear();
		m_mapDevices.clear();
	}

	bool InputManager::AddDevice(Device *pDevice)
	{
		// Check if the device can be added
		if (pDevice && m_mapDevices.find(pDevice->GetName()) == m_mapDevices.cend()) {
			m_lstDevices.push_back(pDevice);
			m_mapDevices.emplace(pDevice->GetName(), pDevice);
			return true;
		}

		// Error!
		return false;
	}

	bool InputManager::RemoveDevice(Device *pDevice)
	{
		// Check device
		if (pDevice)
		{
			Devices::const_iterator iterator = std::find(m_lstDevices.cbegin(), m_lstDevices.cend(), pDevice);
			if (m_lstDevices.cend() != iterator)
			{
				m_lstDevices.erase(iterator);
				m_mapDevices.erase(pDevice->GetName());
				return true;
			}
		}

		// Error!
		return false;
	}

	void InputManager::RemoveControl(Control *pControl)
	{
		// Valid pointer?
		if (pControl) {
			// Remove control from list (if it's within the list at all)
			std::lock_guard<std::mutex> mutexLock(*mMutex);
			Controls::const_iterator iterator = std::find(m_lstUpdatedControls.cbegin(), m_lstUpdatedControls.cend(), pControl);
			if (m_lstUpdatedControls.cend() != iterator)
			{
				m_lstUpdatedControls.erase(iterator);
			}
		}
	}

	void InputManager::UpdateControl(Control *pControl)
	{
		// Valid pointer?
		if (pControl) {
			// Add control to list, but only if it's not already within the list!
			std::lock_guard<std::mutex> mutexLock(*mMutex);
			if (std::find(m_lstUpdatedControls.cbegin(), m_lstUpdatedControls.cend(), pControl) == m_lstUpdatedControls.cend())
			{
				m_lstUpdatedControls.push_back(pControl);
			}
		}
	}








	//[-------------------------------------------------------]
	//[ Abstract backend only interface implementations       ]
	//[-------------------------------------------------------]
	//[-------------------------------------------------------]
	//[ PLInput::Provider                                     ]
	//[-------------------------------------------------------]
	//[-------------------------------------------------------]
	//[ Public functions                                      ]
	//[-------------------------------------------------------]
	void Provider::DetectDevices(bool bReset)
	{
		// Delete all devices?
		if (bReset)
			Clear();

		// Flag all current devices as 'not confirmed'
		for (uint32_t i=0; i<m_lstDevices.size(); ++i)
			m_lstDevices[i]->m_bConfirmed = false;

		// Detect new devices (the ones that are already there will be ignored by AddDevice)
		QueryDevices();

		// Delete all devices that are no longer there (confirmed == false)
		for (uint32_t i=0; i<m_lstDevices.size(); ++i) {
			Device *pDevice = m_lstDevices[i];
			if (!pDevice->m_bConfirmed) {
				// Remove device
				mInputManager.RemoveDevice(pDevice);
				m_lstDevices.erase(std::find(m_lstDevices.cbegin(), m_lstDevices.cend(), pDevice));
				delete pDevice;
				--i;
			}
		}
	}

	void Provider::Clear()
	{
		// Delete all input devices
		for (uint32_t i=0; i<m_lstDevices.size(); ++i)
		{
			Device *pDevice = m_lstDevices[i];
			mInputManager.RemoveDevice(pDevice);
			delete pDevice;
		}
		m_lstDevices.clear();
	}

	bool Provider::CheckDevice(const std::string &sName)
	{
		// Check if the device is already present
		Device *pDevice = mInputManager.GetDevice(sName);
		if (pDevice) {
			// Update device
			pDevice->m_bConfirmed = true;
			return true;
		}

		// Not found
		return false;
	}

	bool Provider::AddDevice(const std::string &sName, Device *pDevice)
	{
		// Check if the device is already present
		Device *pDeviceFound = mInputManager.GetDevice(sName);
		if (!pDeviceFound) {
			// Add device to manager
			if (mInputManager.AddDevice(pDevice)) {
				// Add device to own list - if we're in here, we now that the pDevice pointer is valid
				m_lstDevices.push_back(pDevice);
				pDevice->m_bConfirmed = true;
				return true;
			}
		}

		// Error!
		return false;
	}




	//[-------------------------------------------------------]
	//[ PLInput::ConnectionDevice                             ]
	//[-------------------------------------------------------]
	void ConnectionDevice::InitThread()
	{
		// Create input buffer
		if (!m_pInputBuffer && m_nInputReportSize > 0)
			m_pInputBuffer  = new uint8_t[m_nInputReportSize];

		// Create output buffer
		if (!m_pOutputBuffer && m_nOutputReportSize > 0)
			m_pOutputBuffer = new uint8_t[m_nOutputReportSize];

		// Create critical section
		if (nullptr == mMutex)
		{
			mMutex = new std::mutex();
		}

		// Start thread
		if (nullptr == m_pThread)
		{
			m_pThread = new std::thread(ConnectionDevice::ReadThread, this);
		}
	}

	void ConnectionDevice::StopThread()
	{
		// Stop thread
		if (m_pThread) {
			// Exit thread function and wait until it has ended
			m_bThreadExit = true;
			m_pThread->join();

			// Delete thread
			delete m_pThread;
			m_pThread = nullptr;
		}

		// Delete critical section
		if (nullptr != mMutex) {
			delete mMutex;
			mMutex = nullptr;
		}

		// Destroy input buffer
		if (m_pInputBuffer) {
			delete [] m_pInputBuffer;
			m_pInputBuffer = nullptr;
		}

		// Destroy output buffer
		if (m_pOutputBuffer) {
			delete [] m_pOutputBuffer;
			m_pOutputBuffer = nullptr;
		}
	}

	void ConnectionDevice::LockCriticalSection()
	{
		// Check critical section
		if (nullptr != mMutex) {
			// Lock critical section (no read/write allowed)
			mMutex->lock();
		}
	}

	void ConnectionDevice::UnlockCriticalSection()
	{
		// Check critical section
		if (nullptr != mMutex) {
			// Unlock critical section
			mMutex->unlock();
		}
	}


	//[-------------------------------------------------------]
	//[ Private static functions                              ]
	//[-------------------------------------------------------]
	int ConnectionDevice::ReadThread(void *pData)
	{
		#ifdef WIN32
			// Set thread name to improve debugging
			__try
			{
				const DWORD MS_VC_EXCEPTION = 0x406D1388;
				#pragma pack(push, 8)
					typedef struct tagTHREADNAME_INFO
					{
						DWORD  dwType;		///< Must be 0x1000
						LPCSTR szName;		///< Pointer to name (in user address space)
						DWORD  dwThreadID;	///< Thread ID (-1 = caller thread)
						DWORD  dwFlags;		///< Reserved for future use, must be zero
					} THREADNAME_INFO;
				#pragma pack(pop)
				const THREADNAME_INFO info = { 0x1000, "Input connection device", ::GetCurrentThreadId(), 0 };
				::RaiseException(MS_VC_EXCEPTION, 0, sizeof(info) / sizeof(ULONG_PTR), reinterpret_cast<const ULONG_PTR*>(&info));
			}
			__except (EXCEPTION_EXECUTE_HANDLER)
			{
				// Nothing here
			}
		#endif

		// Get handler
		ConnectionDevice *pDevice = static_cast<ConnectionDevice*>(pData);
		while (!pDevice->m_bThreadExit) {
			// Read data from device
			pDevice->Read(pDevice->m_pInputBuffer, pDevice->m_nInputReportSize);
		}

		// Done
		return 0;
	}








	//[-------------------------------------------------------]
	//[ Bluetooth backend only interfaces                     ]
	//[-------------------------------------------------------]
	//[-------------------------------------------------------]
	//[ Forward declarations                                  ]
	//[-------------------------------------------------------]
	class BTImpl;
	class BTDevice;


	//[-------------------------------------------------------]
	//[ Classes                                               ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Abstract base class for platform specific Bluetooth implementations
	*/
	class BTImpl
	{


	//[-------------------------------------------------------]
	//[ Friends                                               ]
	//[-------------------------------------------------------]
	friend class Bluetooth;


	//[-------------------------------------------------------]
	//[ Protected functions                                   ]
	//[-------------------------------------------------------]
	protected:
		/**
		*  @brief
		*    Default constructor
		*/
		inline BTImpl()
		{
			// Nothing here
		}

		/**
		*  @brief
		*    Destructor
		*/
		inline virtual ~BTImpl()
		{
			// Nothing here
		}


	//[-------------------------------------------------------]
	//[ Protected virtual BTImpl functions                    ]
	//[-------------------------------------------------------]
	protected:
		/**
		*  @brief
		*    Enumerate Bluetooth devices
		*
		*  @param[out] lstDevices
		*    List that will receive the devices
		*/
		virtual void EnumerateDevices(std::vector<BTDevice*> &lstDevices) = 0;


	};

	/**
	*  @brief
	*    Class for accessing Bluetooth devices
	*/
	class Bluetooth final
	{


	//[-------------------------------------------------------]
	//[ Friends                                               ]
	//[-------------------------------------------------------]
	friend class BluetoothProvider;


	//[-------------------------------------------------------]
	//[ Public functions                                      ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Detect available Bluetooth devices
		*/
		void DetectDevices()
		{
			// Clear list
			Clear();

			// Check Bluetooth implementation
			if (m_pBTImpl) {
				// Enumerate devices
				m_pBTImpl->EnumerateDevices(m_lstDevices);
			}
		}

		/**
		*  @brief
		*    Get list of available Bluetooth devices
		*
		*  @return
		*    Device list
		*/
		inline const std::vector<BTDevice*> &GetDevices() const
		{
			// Return list
			return m_lstDevices;
		}


	//[-------------------------------------------------------]
	//[ Private functions                                     ]
	//[-------------------------------------------------------]
	private:
		/**
		*  @brief
		*    Default constructor
		*/
		Bluetooth();

		explicit Bluetooth(const Bluetooth& source) = delete;

		/**
		*  @brief
		*    Destructor
		*/
		~Bluetooth()
		{
			// Clear devices
			Clear();

			// Delete Bluetooth implementation
			if (m_pBTImpl)
				delete m_pBTImpl;
		}

		Bluetooth& operator= (const Bluetooth& source) = delete;

		/**
		*  @brief
		*    Remove all devices
		*/
		inline void Clear()
		{
			// Destroy devices
			m_lstDevices.clear();
		}


	//[-------------------------------------------------------]
	//[ Private data                                          ]
	//[-------------------------------------------------------]
	private:
		BTImpl					*m_pBTImpl;		///< Platform specific Bluetooth implementation
		std::vector<BTDevice*>	 m_lstDevices;	///< List of devices


	};

	/**
	*  @brief
	*    Bluetooth device
	*/
	class BTDevice : public ConnectionDevice
	{


	//[-------------------------------------------------------]
	//[ Friends                                               ]
	//[-------------------------------------------------------]
	friend class BTLinux;


	//[-------------------------------------------------------]
	//[ Public functions                                      ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Default constructor
		*/
		BTDevice()
		{
			// Set device type
			m_nDeviceType = ConnectionDevice::DeviceTypeBluetooth;

			// Initialize device
			memset(m_nAddress, 0, 6);
			memset(m_nClass,   0, 3);
		}

		/**
		*  @brief
		*    Copy constructor
		*
		*  @param[in] cSource
		*    BTDevice to copy from
		*/
		explicit BTDevice(const BTDevice &cSource) :
			m_sName(cSource.m_sName)
		{
			// Set device type
			m_nDeviceType = ConnectionDevice::DeviceTypeBluetooth;

			// Copy device
			memcpy(m_nAddress, cSource.m_nAddress, 6);
			memcpy(m_nClass,   cSource.m_nClass,   3);
		}

		/**
		*  @brief
		*    Destructor
		*/
		inline virtual ~BTDevice() override
		{
			// Nothing here
		}

		/**
		*  @brief
		*    Assignment operator
		*
		*  @param[in] cSource
		*    BTDevice to copy from
		*
		*  @return
		*    Reference to this BTDevice
		*/
		BTDevice &operator =(const BTDevice &cSource)
		{
			m_sName = cSource.m_sName;
			memcpy(m_nAddress, cSource.m_nAddress, 6);
			memcpy(m_nClass,   cSource.m_nClass,   3);
			return *this;
		}

		/**
		*  @brief
		*    Comparison operator
		*
		*  @param[in] cSource
		*    BTDevice to compare with
		*
		*  @return
		*    'true', if both are equal, else 'false'
		*/
		bool operator ==(const BTDevice &cSource) const
		{
			return (m_sName == cSource.m_sName &&
					memcmp(m_nAddress,	cSource.m_nAddress, 6) == 0 &&
					memcmp(m_nClass,	cSource.m_nClass,   3) == 0);
		}

		/**
		*  @brief
		*    Get device name
		*
		*  @return
		*    Device name
		*/
		inline const std::string& GetName() const
		{
			return m_sName;
		}

		/**
		*  @brief
		*    Get device address
		*
		*  @param[in] nIndex
		*    Address component (0..7)
		*
		*  @return
		*    Address
		*/
		inline uint8_t GetAddress(int8_t nIndex) const
		{
			return (nIndex >= 0 && nIndex < 6) ? m_nAddress[nIndex] : 0u;
		}

		/**
		*  @brief
		*    Get device class
		*
		*  @param[in] nIndex
		*    Class component (0..2)
		*
		*  @return
		*    Class
		*/
		inline uint8_t GetClass(int8_t nIndex) const
		{
			return (nIndex >= 0 && nIndex < 3) ? m_nClass[nIndex] : 0u;
		}


	//[-------------------------------------------------------]
	//[ Private data                                          ]
	//[-------------------------------------------------------]
	private:
		// Device data
		std::string	m_sName;		///< Device name
		uint8_t		m_nAddress[8];	///< Bluetooth address
		uint8_t		m_nClass[3];	///< Bluetooth device class


	};

	/**
	*  @brief
	*    Input provider for Bluetooth devices
	*/
	class BluetoothProvider final : public Provider
	{


	//[-------------------------------------------------------]
	//[ Public functions                                      ]
	//[-------------------------------------------------------]
	public:
		BluetoothProvider() = delete;

		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] inputManager
		*    Owner input manager
		*/
		inline explicit BluetoothProvider(InputManager& inputManager) :
			Provider(inputManager),
			mBluetooth(new Bluetooth())
		{
			// Nothing here
		}

		/**
		*  @brief
		*    Destructor
		*/
		inline virtual ~BluetoothProvider()
		{
			delete mBluetooth;
		}


	//[-------------------------------------------------------]
	//[ Private virtual Provider functions                    ]
	//[-------------------------------------------------------]
	private:
		virtual void QueryDevices() override
		{
			// Device counter
			int nWiiMote = 0;

			// Get list of Bluetooth-devices
			const std::vector<BTDevice*> &lstDevices = mBluetooth->GetDevices();
			for (uint32_t i=0; i<lstDevices.size(); ++i) {
				// Get device
				BTDevice *pDevice = lstDevices[i];

				// Check device type
				if ( pDevice->GetName() == "Nintendo RVL-CNT-01" ||
					(pDevice->GetClass(0) == WiiMote::DeviceClass0 &&
					 pDevice->GetClass(1) == WiiMote::DeviceClass1 &&
					 pDevice->GetClass(2) == WiiMote::DeviceClass2) ) {
					// WiiMote
					std::string sName = std::string("WiiMote") + std::to_string(nWiiMote);
					++nWiiMote;
					if (!CheckDevice(sName))
						AddDevice(sName, new WiiMote(mInputManager, sName, pDevice));
				}
			}
		}


	//[-------------------------------------------------------]
	//[ Private functions                                     ]
	//[-------------------------------------------------------]
	private:
		BluetoothProvider(const BluetoothProvider &cSource) = delete;
		BluetoothProvider &operator =(const BluetoothProvider &cSource) = delete;


	//[-------------------------------------------------------]
	//[ Private data                                          ]
	//[-------------------------------------------------------]
	private:
		Bluetooth* mBluetooth;


	};







	//[-------------------------------------------------------]
	//[ MS Windows backend implementation                     ]
	//[-------------------------------------------------------]
	#if defined(WIN32)
		//[-------------------------------------------------------]
		//[ Includes                                              ]
		//[-------------------------------------------------------]
		#pragma comment(lib, "Winmm.lib")		// For the legacy joystick device
		#pragma comment(lib, "Setupapi.lib")	// For HID


		//[-------------------------------------------------------]
		//[ Forward declarations                                  ]
		//[-------------------------------------------------------]
		class RawInput;
		class RawInputDevice;


		//[-------------------------------------------------------]
		//[ Global definitions                                    ]
		//[-------------------------------------------------------]
		/**
		*  @brief
		*    Virtual keys
		*/
		enum EKey {
			KeyBackspace			= 0x08,		///< Backspace
			KeyTab					= 0x09,		///< Tab
			KeyClear				= 0x0C,		///< Clear (not available everywhere)
			KeyReturn				= 0x0D,		///< Return (often the same as "Enter")
			KeyShift				= 0x10,		///< Shift
			KeyControl				= 0x11,		///< Control ("Ctrl")
			KeyAlt					= 0x12,		///< Alt
			KeyPause				= 0x13,		///< Pause
			KeyCapsLock				= 0x14,		///< Caps lock
			KeyEscape				= 0x1B,		///< Escape
			KeySpace				= 0x20,		///< Space
			KeyPageUp				= 0x21,		///< Page up
			KeyPageDown				= 0x22,		///< Page down
			KeyEnd					= 0x23,		///< End
			KeyHome					= 0x24,		///< Home
			KeyLeft					= 0x25,		///< Left arrow
			KeyUp					= 0x26,		///< Up arrow
			KeyRight				= 0x27,		///< Right arrow
			KeyDown					= 0x28,		///< Down arrow
			KeySelect				= 0x29,		///< Select (not available everywhere)
			KeyExecute				= 0x2B,		///< Execute (not available everywhere)
			KeyPrint				= 0x2C,		///< Print screen
			KeyInsert				= 0x2D,		///< Insert
			KeyDelete				= 0x2E,		///< Delete
			KeyHelp					= 0x2F,		///< Help (not available everywhere)
			Key0					= 0x30,		///< 0
			Key1					= 0x31,		///< 1
			Key2					= 0x32,		///< 2
			Key3					= 0x33,		///< 3
			Key4					= 0x34,		///< 4
			Key5					= 0x35,		///< 5
			Key6					= 0x36,		///< 6
			Key7					= 0x37,		///< 7
			Key8					= 0x38,		///< 8
			Key9					= 0x39,		///< 9
			KeyA					= 0x41,		///< A
			KeyB					= 0x42,		///< B
			KeyC					= 0x43,		///< C
			KeyD					= 0x44,		///< D
			KeyE					= 0x45,		///< E
			KeyF					= 0x46,		///< F
			KeyG					= 0x47,		///< G
			KeyH					= 0x48,		///< H
			KeyI					= 0x49,		///< I
			KeyJ					= 0x4A,		///< J
			KeyK					= 0x4B,		///< K
			KeyL					= 0x4C,		///< L
			KeyM					= 0x4D,		///< M
			KeyN					= 0x4E,		///< N
			KeyO					= 0x4F,		///< O
			KeyP					= 0x50,		///< P
			KeyQ					= 0x51,		///< Q
			KeyR					= 0x52,		///< R
			KeyS					= 0x53,		///< S
			KeyT					= 0x54,		///< T
			KeyU					= 0x55,		///< U
			KeyV					= 0x56,		///< V
			KeyW					= 0x57,		///< W
			KeyX					= 0x58,		///< X
			KeyY					= 0x59,		///< Y
			KeyZ					= 0x5A,		///< Z
			KeyNumpad0				= 0x60,		///< Numeric keypad 0
			KeyNumpad1				= 0x61,		///< Numeric keypad 1
			KeyNumpad2				= 0x62,		///< Numeric keypad 2
			KeyNumpad3				= 0x63,		///< Numeric keypad 3
			KeyNumpad4				= 0x64,		///< Numeric keypad 4
			KeyNumpad5				= 0x65,		///< Numeric keypad 5
			KeyNumpad6				= 0x66,		///< Numeric keypad 6
			KeyNumpad7				= 0x67,		///< Numeric keypad 7
			KeyNumpad8				= 0x68,		///< Numeric keypad 8
			KeyNumpad9				= 0x69,		///< Numeric keypad 9
			KeyNumpadMultiply		= 0x6A,		///< Numeric keypad "Multiply"
			KeyNumpadAdd			= 0x6B,		///< Numeric keypad "Add"
			KeyNumpadSeparator		= 0x6C,		///< Numeric keypad "Separator"
			KeyNumpadSubtract		= 0x6D,		///< Numeric keypad "Subtract"
			KeyNumpadDecimal		= 0x6E,		///< Numeric keypad "Decimal"
			KeyNumpadDivide			= 0x6F,		///< Numeric keypad "Divide"
			KeyF1					= 0x70,		///< F1
			KeyF2					= 0x71,		///< F2
			KeyF3					= 0x72,		///< F3
			KeyF4					= 0x73,		///< F4
			KeyF5					= 0x74,		///< F5
			KeyF6					= 0x75,		///< F6
			KeyF7					= 0x76,		///< F7
			KeyF8					= 0x77,		///< F8
			KeyF9					= 0x78,		///< F9
			KeyF10					= 0x79,		///< F10
			KeyF11					= 0x7A,		///< F11
			KeyF12					= 0x7B,		///< F12
			KeyNumLock				= 0x90,		///< Num lock
			KeyScrollLock			= 0x91,		///< Scroll lock
			KeyCircumflex			= 0xDC,		///< Circumflex (^)
			KeyLeftWindows			= 0x5B,		///< Left Windows key (natural keyboard)
			KeyRightWindows			= 0x5C,		///< Right Windows key (natural keyboard)
			KeyApplications			= 0x5D,		///< Applications key (natural keyboard)
			KeyF13					= 0x7C,		///< F13 key
			KeyF14					= 0x7D,		///< F14 key
			KeyF15					= 0x7E,		///< F15 key
			KeyF16					= 0x7F,		///< F16 key
			KeyF17					= 0x80,		///< F17 key
			KeyF18					= 0x81,		///< F18 key
			KeyF19					= 0x82,		///< F19 key
			KeyF20					= 0x83,		///< F20 key
			KeyF21					= 0x84,		///< F21 key
			KeyF22					= 0x85,		///< F22 key
			KeyF23					= 0x86,		///< F23 key
			KeyF24					= 0x87,		///< F24 key
			KeyLeftShift			= 0xA0,		///< Left shift key
			KeyRightShift			= 0xA1,		///< Right shift key
			KeyLeftControl			= 0xA2,		///< Left control key
			KeyRightControl			= 0xA3,		///< Right control key
			KeyVolumeMute			= 0xAD,		///< Volume mute key
			KeyVolumeDown			= 0xAE,		///< Volume down key
			KeyVolumeUp				= 0xAF,		///< Volume up key
			KeyMediaNextTrack		= 0xB0,		///< Next track key
			KeyMediaPreviousTrack	= 0xB1,		///< Previous track key
			KeyMediaStop			= 0xB2,		///< Stop media key
			KeyMediaPlayPause		= 0xB3,		///< Play/pause media key
			KeyAdd					= 0xBB,		///< For any country/region, the '+' key
			KeySeparator			= 0xBC,		///< For any country/region, the ',' key
			KeySubtract				= 0xBD,		///< For any country/region, the '-' key
			KeyDecimal				= 0xBE,		///< For any country/region, the '.' key
			KeyOEM1					= 0xBA,		///< Used for miscellaneous characters; it can vary by keyboard. For the US standard keyboard, the ';:' key
			KeyOEM2					= 0xBF,		///< Used for miscellaneous characters; it can vary by keyboard. For the US standard keyboard, the '/?' key
			KeyOEM3					= 0xC0,		///< Used for miscellaneous characters; it can vary by keyboard. For the US standard keyboard, the '`~' key
			KeyOEM4					= 0xDB,		///< Used for miscellaneous characters; it can vary by keyboard. For the US standard keyboard, the '[{' key
			KeyOEM5					= 0xDC,		///< Used for miscellaneous characters; it can vary by keyboard. For the US standard keyboard, the '\|' key
			KeyOEM6					= 0xDD,		///< Used for miscellaneous characters; it can vary by keyboard. For the US standard keyboard, the ']}' key
			KeyOEM7					= 0xDE,		///< Used for miscellaneous characters; it can vary by keyboard. For the US standard keyboard, the 'single-quote/double-quote' key
			KeyOEM8					= 0xDF,		///< Used for miscellaneous characters; it can vary by keyboard
			KeyOEM102				= 0xE2		///< Either the angle bracket key or the backslash key on the RT 102-key keyboard
		};


		//[-------------------------------------------------------]
		//[ Global HID function pointers                          ]
		//[-------------------------------------------------------]
		// "hid.dll"
		PFNHIDDGETPREPARSEDDATA		HidD_GetPreparsedData	= nullptr;
		PFNHIDDFREEPREPARSEDDATA	HidD_FreePreparsedData	= nullptr;
		PFNHIDPGETDATA				HidP_GetData			= nullptr;
		PFNHIDPSETDATA				HidP_SetData			= nullptr;
		PFNHIDPGETHIDGUID			HidD_GetHidGuid			= nullptr;
		PFNHIDPGETATTRIBUTES		HidD_GetAttributes		= nullptr;
		PFNHIDPGETCAPS				HidP_GetCaps			= nullptr;
		PFNHIDPGETBUTTONCAPS		HidP_GetButtonCaps		= nullptr;
		PFNHIDPGETVALUECAPS			HidP_GetValueCaps		= nullptr;


		//[-------------------------------------------------------]
		//[ Classes                                               ]
		//[-------------------------------------------------------]
		/**
		*  @brief
		*    Joystick implementation for Windows legacy joystick API
		*/
		class LegacyJoystickDevice : public UpdateDevice
		{


		//[-------------------------------------------------------]
		//[ Public functions                                      ]
		//[-------------------------------------------------------]
		public:
			/**
			*  @brief
			*    Constructor
			*
			*  @param[in] nJoystickID
			*    Joystick ID
			*/
			LegacyJoystickDevice(uint32_t nJoystickID) :
				m_nJoystickID (nJoystickID),
				m_nFallbackPOV(0)
			{
				// Destroy device implementation automatically
				m_bDelete = true;

				// Check whether or not the joystick supports POV
				JOYCAPS sJoyCaps;
				memset(&sJoyCaps, 0, sizeof(sJoyCaps));
				if (joyGetDevCaps (m_nJoystickID, &sJoyCaps, sizeof(sJoyCaps)) == JOYERR_NOERROR && !(sJoyCaps.wCaps & JOYCAPS_HASPOV)) {
					// Get joystick state
					JOYINFOEX sJoyInfo;
					sJoyInfo.dwSize  = sizeof(JOYINFOEX);
					sJoyInfo.dwFlags = JOY_RETURNALL;
					if (joyGetPosEx(m_nJoystickID, &sJoyInfo) == JOYERR_NOERROR) {
						// Use the initial position as POV
						m_nFallbackPOV = static_cast<int>(sJoyInfo.dwXpos);
					}
				}
			}

			/**
			*  @brief
			*    Destructor
			*/
			inline virtual ~LegacyJoystickDevice()
			{
				// Nothing here
			}


		//[-------------------------------------------------------]
		//[ Public virtual UpdateDevice functions                 ]
		//[-------------------------------------------------------]
		public:
			virtual void Update() override
			{
				// Check if input device is valid
				if (m_pDevice) {
					// Get joystick device
					Joystick *pJoystick = static_cast<Joystick*>(m_pDevice);

					// Get joystick state
					JOYINFOEX sJoyInfo;
					sJoyInfo.dwSize  = sizeof(JOYINFOEX);
					sJoyInfo.dwFlags = JOY_RETURNALL;
					if (joyGetPosEx(m_nJoystickID, &sJoyInfo) == JOYERR_NOERROR) {
						// Update axes
						for (uint32_t i=0; i<6; ++i) {
							// Get POV
							int nPOV = static_cast<int>(m_nFallbackPOV ? m_nFallbackPOV : sJoyInfo.dwPOV / 2);

							// Calculate delta
							int nDelta = 0;
							switch (i) {
								case 0: nDelta = static_cast<int>(sJoyInfo.dwXpos - nPOV); break;
								case 1: nDelta = static_cast<int>(sJoyInfo.dwYpos - nPOV); break;
								case 2: nDelta = static_cast<int>(sJoyInfo.dwZpos - nPOV); break;
								case 3: nDelta = static_cast<int>(sJoyInfo.dwRpos - nPOV); break;
								case 4: nDelta = static_cast<int>(sJoyInfo.dwUpos - nPOV); break;
								case 5: nDelta = static_cast<int>(sJoyInfo.dwVpos - nPOV); break;
							}

							// Calculate axis values (between -1.0f and 1.0f)
							float fPos		 = 0.0f;
							int   nThreshold = pJoystick->GetThreshold();
							if (abs(nDelta) >= nThreshold) {
								if (nDelta < 0.0f)
									fPos = static_cast<float>(nDelta + nThreshold) / (nPOV - nThreshold);
								else
									fPos = static_cast<float>(nDelta - nThreshold) / (nPOV - nThreshold);
								if (fPos < -1.0f)
									fPos = -1.0f;
								if (fPos >  1.0f)
									fPos =  1.0f;
							}

							// Set new value
							Axis *pAxis = pJoystick->GetAxes()[i];
							if (pAxis && pAxis->getValue() != fPos)
								pAxis->setValue(fPos, false);
						}

						// Update buttons
						uint32_t nFlag = 1;
						for (uint32_t i=0; i<32; ++i)
						{
							bool bPressed = ((sJoyInfo.dwButtons & nFlag) != 0);
							Button *pButton = pJoystick->GetButtons()[i];
							if (pButton && pButton->isPressed() != bPressed)
							{
								pButton->setPressed(bPressed);
							}
							nFlag = nFlag << 1;
						}
					}
				}
			}


		//[-------------------------------------------------------]
		//[ Private data                                          ]
		//[-------------------------------------------------------]
		private:
			uint32_t m_nJoystickID;		///< Joystick ID
			int		 m_nFallbackPOV;	///< If the joystick doesn't support POV this is used, else this is 0


		};

		/**
		*  @brief
		*    Input provider for Windows legacy joystick API
		*/
		class LegacyJoystickProvider : public Provider
		{


		//[-------------------------------------------------------]
		//[ Public functions                                      ]
		//[-------------------------------------------------------]
		public:
			LegacyJoystickProvider() = delete;

			/**
			*  @brief
			*    Constructor
			*
			*  @param[in] inputManager
			*    Owner input manager
			*/
			inline explicit LegacyJoystickProvider(InputManager& inputManager) :
				Provider(inputManager)
			{
				// Nothing here
			}

			/**
			*  @brief
			*    Destructor
			*/
			inline virtual ~LegacyJoystickProvider() override
			{
				// Nothing here
			}


		//[-------------------------------------------------------]
		//[ Private virtual Provider functions                    ]
		//[-------------------------------------------------------]
		private:
			virtual void QueryDevices() override
			{
				// Get list of joysticks
				JOYINFO sJoyInfo;
				for (uint32_t nJoystick = 0; nJoystick < 16; ++nJoystick)
				{
					// Check if joystick is valid
					if (joyGetPos(nJoystick, &sJoyInfo) == JOYERR_NOERROR)
					{
						// Get name and check if a device with that ID is not used already
						std::string sName = std::string("Joystick") + std::to_string(nJoystick);
						if (!CheckDevice(sName)) {
							// Add device
							LegacyJoystickDevice *pImpl = new LegacyJoystickDevice(nJoystick);
							AddDevice(sName, new Joystick(mInputManager, sName, pImpl));
						}
					}
					else
					{
						break;
					}
				}
			}


		//[-------------------------------------------------------]
		//[ Private functions                                     ]
		//[-------------------------------------------------------]
		private:
			LegacyJoystickProvider(const LegacyJoystickProvider &cSource) = delete;
			LegacyJoystickProvider &operator =(const LegacyJoystickProvider &cSource) = delete;


		};

		/**
		*  @brief
		*    Information about a Raw-Input device
		*/
		class RawInputDevice : public UpdateDevice
		{


		//[-------------------------------------------------------]
		//[ Friends                                               ]
		//[-------------------------------------------------------]
		friend class RawInput;


		//[-------------------------------------------------------]
		//[ Public functions                                      ]
		//[-------------------------------------------------------]
		public:
			/**
			*  @brief
			*    Default constructor
			*/
			RawInputDevice() :
				m_nType(0),
				m_hDevice(nullptr),
				m_bVirtual(false),
				m_nOldX(0),
				m_nOldY(0)
			{
				// Do not destroy device implementation automatically, because this is managed by RawInput
				m_bDelete = false;

				// Clear deviceinfo struct
				memset(&m_sDeviceInfo, 0, sizeof(m_sDeviceInfo));
			}

			/**
			*  @brief
			*    Copy constructor
			*
			*  @param[in] cSource
			*    Source to copy from
			*/
			explicit RawInputDevice(const RawInputDevice &cSource) :
				m_sName(cSource.m_sName),
				m_nType(cSource.m_nType),
				m_hDevice(cSource.m_hDevice),
				m_bVirtual(cSource.m_bVirtual)
			{
				// Do not destroy device implementation automatically, because this is managed by RawInput
				m_bDelete = false;

				// Copy deviceinfo struct
				memcpy(&m_sDeviceInfo, &cSource.m_sDeviceInfo, sizeof(m_sDeviceInfo));
			}

			/**
			*  @brief
			*    Destructor
			*/
			inline virtual ~RawInputDevice() override
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
			RawInputDevice &operator =(const RawInputDevice &cSource)
			{
				m_sName		= cSource.m_sName;
				m_nType		= cSource.m_nType;
				m_hDevice	= cSource.m_hDevice;
				m_bVirtual	= cSource.m_bVirtual;
				memcpy(&m_sDeviceInfo, &cSource.m_sDeviceInfo, sizeof(m_sDeviceInfo));
				return *this;
			}

			/**
			*  @brief
			*    Comparison operator
			*
			*  @param[in] cSource
			*    RawInputDevice to compare with
			*
			*  @return
			*    'true', if both are equal, else 'false'
			*/
			bool operator ==(const RawInputDevice &cSource)
			{
				return (m_sName		== cSource.m_sName &&
						m_nType		== cSource.m_nType &&
						m_hDevice	== cSource.m_hDevice &&
						m_bVirtual	== cSource.m_bVirtual &&
						memcmp(&m_sDeviceInfo, &cSource.m_sDeviceInfo, sizeof(m_sDeviceInfo)) == 0);
			}

			/**
			*  @brief
			*    Get device name
			*
			*  @return
			*    Device name
			*/
			inline const std::string& GetName() const
			{
				return m_sName;
			}

			/**
			*  @brief
			*    Get device type
			*
			*  @return
			*    Device type
			*/
			inline DWORD GetType() const
			{
				return m_nType;
			}

			/**
			*  @brief
			*    Get device handle
			*
			*  @return
			*    Device handle
			*/
			inline HANDLE GetHandle() const
			{
				return m_hDevice;
			}

			/**
			*  @brief
			*    Get device info
			*
			*  @return
			*    Device info
			*/
			inline RID_DEVICE_INFO GetDeviceInfo() const
			{
				return m_sDeviceInfo;
			}

			/**
			*  @brief
			*    Check if the device is a virtual device
			*
			*  @return
			*    'true', if it is a virtual device, else 'false'
			*/
			inline bool IsVirtual() const
			{
				return m_bVirtual;
			}


		//[-------------------------------------------------------]
		//[ Public virtual UpdateDevice functions                 ]
		//[-------------------------------------------------------]
		public:
			virtual void Update() override
			{
				// Check if input device is valid
				if (m_pDevice && m_nType == RIM_TYPEMOUSE) {
					// Get mouse device
					Mouse *pMouse = static_cast<Mouse*>(m_pDevice);

					// Reset mouse movement
					if (pMouse->X.getValue() != 0.0f)
						pMouse->X.setValue(0.0f, true);
					if (pMouse->Y.getValue() != 0.0f)
						pMouse->Y.setValue(0, true);
					if (pMouse->Wheel.getValue() != 0.0f)
						pMouse->Wheel.setValue(0, true);
				}
			}


		//[-------------------------------------------------------]
		//[ Private functions                                     ]
		//[-------------------------------------------------------]
		private:
			/**
			*  @brief
			*    Process raw keyboard data
			*
			*  @param[in] nMakeCode
			*    Scan code
			*  @param[in] nFlags
			*    Miscellaneous scan code information
			*  @param[in] nReserved
			*    Reserved
			*  @param[in] nKey
			*    Key information
			*  @param[in] nMessage
			*    Message information
			*  @param[in] nExtra
			*    Additional device specific event information
			*/
			void ProcessKeyboardData(unsigned short, unsigned short nFlags, unsigned short, unsigned short nKey, unsigned int, unsigned long)
			{
				// Check if input device is valid
				if (m_pDevice && m_nType == RIM_TYPEKEYBOARD) {
					// Get keyboard device
					Keyboard *pKeyboard = static_cast<Keyboard*>(m_pDevice);

					// Get button
					Button *pButton = GetKeyboardKey(pKeyboard, nKey);
					if (pButton) {
						// Make or break code?
						const bool bPressed = ((nFlags & RI_KEY_BREAK) == 0);
						if (pButton->isPressed() != bPressed)
						{
							pButton->setPressed(bPressed);
						}
					}
				}
			}

			/**
			*  @brief
			*    Process raw mouse data
			*
			*  @param[in] nFlags
			*    Flags
			*  @param[in] nButtons
			*    Buttons
			*  @param[in] nButtonFlags
			*    Button flags
			*  @param[in] nButtonData
			*    Button data
			*  @param[in] nRawButtons
			*    Raw buttons
			*  @param[in] nLastX
			*    Movement in the X direction (relative or absolute)
			*  @param[in] nLastY
			*    Movement in the Y direction (relative or absolute)
			*  @param[in] nExtra
			*    Additional device specific event information
			*/
			void ProcessMouseData(unsigned short nFlags, unsigned long, unsigned short nButtonFlags, unsigned short nButtonData, long, long nLastX, long nLastY, unsigned long)
			{
				// Check if input device is valid
				if (m_pDevice && m_nType == RIM_TYPEMOUSE) {
					// Get mouse device
					Mouse *pMouse = static_cast<Mouse*>(m_pDevice);

					// Update axes
					float fX, fY;
					if (nFlags & MOUSE_MOVE_ABSOLUTE) {
						// Absolute position
						fX = pMouse->X.getValue() + static_cast<float>(nLastX - m_nOldX);
						fY = pMouse->Y.getValue() + static_cast<float>(nLastY - m_nOldY);
						m_nOldX = nLastX;
						m_nOldY = nLastY;
					} else {
						// Accumulate relative position
						fX = pMouse->X.getValue() + static_cast<float>(nLastX);
						fY = pMouse->Y.getValue() + static_cast<float>(nLastY);
					}

					// Set new axis values
					if (pMouse->X.getValue() != fX)
						pMouse->X.setValue(fX, true);
					if (pMouse->Y.getValue() != fY)
						pMouse->Y.setValue(fY, true);

					// Update buttons
					if (nButtonFlags) {
						if (nButtonFlags & RI_MOUSE_BUTTON_1_DOWN) {
							if (!pMouse->Left.isPressed())
							{
								pMouse->Left.setPressed(true);
							}
						}
						if (nButtonFlags & RI_MOUSE_BUTTON_1_UP) {
							if (pMouse->Left.isPressed())
							{
								pMouse->Left.setPressed(false);
							}
						}
						if (nButtonFlags & RI_MOUSE_BUTTON_2_DOWN) {
							if (!pMouse->Right.isPressed())
							{
								pMouse->Right.setPressed(true);
							}
						}
						if (nButtonFlags & RI_MOUSE_BUTTON_2_UP) {
							if (pMouse->Right.isPressed())
							{
								pMouse->Right.setPressed(false);
							}
						}
						if (nButtonFlags & RI_MOUSE_BUTTON_3_DOWN) {
							if (!pMouse->Middle.isPressed())
							{
								pMouse->Middle.setPressed(true);
							}
						}
						if (nButtonFlags & RI_MOUSE_BUTTON_3_UP) {
							if (pMouse->Middle.isPressed())
							{
								pMouse->Middle.setPressed(false);
							}
						}
						if (nButtonFlags & RI_MOUSE_BUTTON_4_DOWN) {
							if (!pMouse->Button4.isPressed())
							{
								pMouse->Button4.setPressed(true);
							}
						}
						if (nButtonFlags & RI_MOUSE_BUTTON_4_UP) {
							if (pMouse->Button4.isPressed())
							{
								pMouse->Button4.setPressed(false);
							}
						}
						if (nButtonFlags & RI_MOUSE_BUTTON_5_DOWN) {
							if (!pMouse->Button5.isPressed())
							{
								pMouse->Button5.setPressed(true);
							}
						}
						if (nButtonFlags & RI_MOUSE_BUTTON_5_UP) {
							if (pMouse->Button5.isPressed())
							{
								pMouse->Button5.setPressed(false);
							}
						}
						if (nButtonFlags & RI_MOUSE_WHEEL) {
							// Well ... nButtonData is unsigned, but the wheel delta can be negative. So let's cast USHORT to SHORT.
							const float fValue = static_cast<float>(*reinterpret_cast<short*>(&nButtonData));
							if (pMouse->Wheel.getValue() != fValue)
							{
								pMouse->Wheel.setValue(fValue, true);
							}
						}
					}
				}
			}

			/**
			*  @brief
			*    Get key for virtual key code
			*
			*  @param[in] pKeyboard
			*    Pointer to keyboard device (must be valid!)
			*  @param[in] nKey
			*    Virtual key code
			*
			*  @return
			*    Corresponding key, a null pointer if key code is invalid
			*/
			Button *GetKeyboardKey(Keyboard *pKeyboard, unsigned short nKey)
			{
				// Return key that corresponds to the given key code
				switch (nKey) {
					case KeyBackspace:		return &pKeyboard->Backspace;
					case KeyTab:			return &pKeyboard->Tab;
					case KeyClear:			return &pKeyboard->Clear;
					case KeyReturn:			return &pKeyboard->Return;
					case KeyShift:			return &pKeyboard->Shift;
					case KeyControl:		return &pKeyboard->Control;
					case KeyAlt:			return &pKeyboard->Alt;
					case KeyPause:			return &pKeyboard->Pause;
					case KeyCapsLock:		return &pKeyboard->CapsLock;
					case KeyEscape:			return &pKeyboard->Escape;
					case KeySpace:			return &pKeyboard->Space;
					case KeyPageUp:			return &pKeyboard->PageUp;
					case KeyPageDown:		return &pKeyboard->PageDown;
					case KeyEnd:			return &pKeyboard->End;
					case KeyHome:			return &pKeyboard->Home;
					case KeyLeft:			return &pKeyboard->Left;
					case KeyUp:				return &pKeyboard->Up;
					case KeyRight:			return &pKeyboard->Right;
					case KeyDown:			return &pKeyboard->Down;
					case KeySelect:			return &pKeyboard->Select;
					case KeyExecute:		return &pKeyboard->Execute;
					case KeyPrint:			return &pKeyboard->Print;
					case KeyInsert:			return &pKeyboard->Insert;
					case KeyDelete:			return &pKeyboard->Delete;
					case KeyHelp:			return &pKeyboard->Help;
					case Key0:				return &pKeyboard->Key0;
					case Key1:				return &pKeyboard->Key1;
					case Key2:				return &pKeyboard->Key2;
					case Key3:				return &pKeyboard->Key3;
					case Key4:				return &pKeyboard->Key4;
					case Key5:				return &pKeyboard->Key5;
					case Key6:				return &pKeyboard->Key6;
					case Key7:				return &pKeyboard->Key7;
					case Key8:				return &pKeyboard->Key8;
					case Key9:				return &pKeyboard->Key9;
					case KeyA:				return &pKeyboard->A;
					case KeyB:				return &pKeyboard->B;
					case KeyC:				return &pKeyboard->C;
					case KeyD:				return &pKeyboard->D;
					case KeyE:				return &pKeyboard->E;
					case KeyF:				return &pKeyboard->F;
					case KeyG:				return &pKeyboard->G;
					case KeyH:				return &pKeyboard->H;
					case KeyI:				return &pKeyboard->I;
					case KeyJ:				return &pKeyboard->J;
					case KeyK:				return &pKeyboard->K;
					case KeyL:				return &pKeyboard->L;
					case KeyM:				return &pKeyboard->M;
					case KeyN:				return &pKeyboard->N;
					case KeyO:				return &pKeyboard->O;
					case KeyP:				return &pKeyboard->P;
					case KeyQ:				return &pKeyboard->Q;
					case KeyR:				return &pKeyboard->R;
					case KeyS:				return &pKeyboard->S;
					case KeyT:				return &pKeyboard->T;
					case KeyU:				return &pKeyboard->U;
					case KeyV:				return &pKeyboard->V;
					case KeyW:				return &pKeyboard->W;
					case KeyX:				return &pKeyboard->X;
					case KeyY:				return &pKeyboard->Y;
					case KeyZ:				return &pKeyboard->Z;
					case KeyNumpad0:		return &pKeyboard->Numpad0;
					case KeyNumpad1:		return &pKeyboard->Numpad1;
					case KeyNumpad2:		return &pKeyboard->Numpad2;
					case KeyNumpad3:		return &pKeyboard->Numpad3;
					case KeyNumpad4:		return &pKeyboard->Numpad4;
					case KeyNumpad5:		return &pKeyboard->Numpad5;
					case KeyNumpad6:		return &pKeyboard->Numpad6;
					case KeyNumpad7:		return &pKeyboard->Numpad7;
					case KeyNumpad8:		return &pKeyboard->Numpad8;
					case KeyNumpad9:		return &pKeyboard->Numpad9;
					case KeyNumpadMultiply:	return &pKeyboard->NumpadMultiply;
					case KeyNumpadAdd:		return &pKeyboard->NumpadAdd;
					case KeyNumpadSeparator:return &pKeyboard->NumpadSeparator;
					case KeyNumpadSubtract:	return &pKeyboard->NumpadSubtract;
					case KeyNumpadDecimal:	return &pKeyboard->NumpadDecimal;
					case KeyNumpadDivide:	return &pKeyboard->NumpadDivide;
					case KeyF1:				return &pKeyboard->F1;
					case KeyF2:				return &pKeyboard->F2;
					case KeyF3:				return &pKeyboard->F3;
					case KeyF4:				return &pKeyboard->F4;
					case KeyF5:				return &pKeyboard->F5;
					case KeyF6:				return &pKeyboard->F6;
					case KeyF7:				return &pKeyboard->F7;
					case KeyF8:				return &pKeyboard->F8;
					case KeyF9:				return &pKeyboard->F9;
					case KeyF10:			return &pKeyboard->F10;
					case KeyF11:			return &pKeyboard->F11;
					case KeyF12:			return &pKeyboard->F12;
					case KeyNumLock:		return &pKeyboard->NumLock;
					case KeyScrollLock:		return &pKeyboard->ScrollLock;
					case KeyCircumflex:		return &pKeyboard->Circumflex;
					case KeyLeftWindows:	return &pKeyboard->LeftWindows;
					case KeyRightWindows:	return &pKeyboard->RightWindows;
					case KeyApplications:	return &pKeyboard->Applications;
					case KeyF13:			return &pKeyboard->F13;
					case KeyF14:			return &pKeyboard->F14;
					case KeyF15:			return &pKeyboard->F15;
					case KeyF16:			return &pKeyboard->F16;
					case KeyF17:			return &pKeyboard->F17;
					case KeyF18:			return &pKeyboard->F18;
					case KeyF19:			return &pKeyboard->F19;
					case KeyF20:			return &pKeyboard->F20;
					case KeyF21:			return &pKeyboard->F21;
					case KeyF22:			return &pKeyboard->F22;
					case KeyF23:			return &pKeyboard->F23;
					case KeyF24:			return &pKeyboard->F24;
					case KeyLeftShift:		return &pKeyboard->LeftShift; // figure out how we can get these four instead of only Shift and Control
					case KeyRightShift:		return &pKeyboard->RightShift; // * ^
					case KeyLeftControl:	return &pKeyboard->LeftControl; // * ^
					case KeyRightControl:	return &pKeyboard->RightControl; // * ^
					case KeyVolumeMute:		return &pKeyboard->VolumeMute;
					case KeyVolumeDown:		return &pKeyboard->VolumeDown;
					case KeyVolumeUp:		return &pKeyboard->VolumeUp;
					case KeyMediaNextTrack:		return &pKeyboard->MediaNextTrack;
					case KeyMediaPreviousTrack:	return &pKeyboard->MediaPreviousTrack;
					case KeyMediaStop:			return &pKeyboard->MediaStop;
					case KeyMediaPlayPause:		return &pKeyboard->MediaPlayPause;
					case KeyAdd:			return &pKeyboard->Add;
					case KeySeparator:		return &pKeyboard->Separator;
					case KeySubtract:		return &pKeyboard->Subtract;
					case KeyDecimal:		return &pKeyboard->Decimal;
					case KeyOEM1:			return &pKeyboard->OEM1;
					case KeyOEM2:			return &pKeyboard->OEM2;
					case KeyOEM3:			return &pKeyboard->OEM3;
					case KeyOEM4:			return &pKeyboard->OEM4;
					// case KeyOEM5:			return &pKeyboard->OEM5; // Circumflex was already defined
					case KeyOEM6:			return &pKeyboard->OEM6;
					case KeyOEM7:			return &pKeyboard->OEM7;
					case KeyOEM8:			return &pKeyboard->OEM8;
					case KeyOEM102:			return &pKeyboard->OEM102;
					default:				return  nullptr;
				}
			}


		//[-------------------------------------------------------]
		//[ Private data                                          ]
		//[-------------------------------------------------------]
		private:
			// Device data
			std::string		m_sName;		///< Device name
			DWORD			m_nType;		///< Device type (keyboard, mouse or HID)
			HANDLE			m_hDevice;		///< Device handle
			RID_DEVICE_INFO	m_sDeviceInfo;	///< Device information struct
			bool			m_bVirtual;		///< Virtual device flag

			// Mouse data
			long			m_nOldX;		///< X position (if mouse only supports absolute positions)
			long			m_nOldY;		///< Y position (if mouse only supports absolute positions)


		};

		/**
		*  @brief
		*    Wrapper for the Raw-Input API on Windows
		*/
		class RawInput
		{


		//[-------------------------------------------------------]
		//[ Friends                                               ]
		//[-------------------------------------------------------]
		friend class RawInputProvider;


		//[-------------------------------------------------------]
		//[ Public functions                                      ]
		//[-------------------------------------------------------]
		public:
			/**
			*  @brief
			*    Remove all devices
			*/
			void Clear()
			{
				// Destroy devices
				for (uint32_t i=0; i<m_lstDevices.size(); i++)
					delete m_lstDevices[i];
				m_lstDevices.clear();
				m_mapDevices.clear();
				m_pDeviceKeyboard = nullptr;
				m_pDeviceMouse	  = nullptr;
			}

			/**
			*  @brief
			*    Detect available Raw-Input devices
			*/
			void DetectDevices()
			{
				// Clear device list
				Clear();

				// Get length of device list
				UINT nDeviceCount = 0;
				if (GetRawInputDeviceList(nullptr, &nDeviceCount, sizeof(RAWINPUTDEVICELIST)) == 0 && nDeviceCount > 0) {
					// Create device array
					RAWINPUTDEVICELIST *pDevices = new RAWINPUTDEVICELIST[nDeviceCount];

					// Get device list
					// Note: We pass nDeviceCount, according to the new documentation of GetRawInputDeviceList.
					// Previously, it was the byte size. See http://support.microsoft.com/kb/946924/en/
					if (GetRawInputDeviceList(pDevices, &nDeviceCount, sizeof(RAWINPUTDEVICELIST)) > 0) {
						// Loop through devices
						for (uint32_t i=0; i<nDeviceCount; ++i) {
							// Get device name
							std::string sName = "Unknown";
							UINT nSize = 0;
							GetRawInputDeviceInfo(pDevices[i].hDevice, RIDI_DEVICENAME, nullptr, &nSize);
							if (nSize > 0) {
								std::wstring utf16Name;
								utf16Name.resize(nSize + 1);
								GetRawInputDeviceInfo(pDevices[i].hDevice, RIDI_DEVICENAME, const_cast<wchar_t*>(utf16Name.data()), &nSize);	// TODO(co) Get rid of the evil const-cast
								utf8::utf16to8(utf16Name.begin(), utf16Name.end(), std::back_inserter(sName));
							}

							// Get device info
							RID_DEVICE_INFO sDeviceInfo;
							nSize = sDeviceInfo.cbSize = sizeof(RID_DEVICE_INFO);
							if (GetRawInputDeviceInfo(pDevices[i].hDevice, RIDI_DEVICEINFO, &sDeviceInfo, &nSize) > 0) {
								// Add device
								RawInputDevice *pDevice = new RawInputDevice();
								pDevice->m_sName		= sName;
								pDevice->m_nType		= pDevices[i].dwType;
								pDevice->m_hDevice		= pDevices[i].hDevice;
								pDevice->m_sDeviceInfo	= sDeviceInfo;
								pDevice->m_bVirtual		= false;
								m_lstDevices.push_back(pDevice);
								PRAGMA_WARNING_PUSH
									PRAGMA_WARNING_DISABLE_MSVC(4826)	// warning C4826: Conversion from 'HANDLE' to 'uint64_t' is sign-extended. This may cause unexpected runtime behavior.
									m_mapDevices.emplace(reinterpret_cast<uint64_t>(pDevice->m_hDevice), pDevice);
								PRAGMA_WARNING_POP
							}
						}

						// Add virtual Keyboard device (will catches all events of all keyboard devices)
						m_pDeviceKeyboard = new RawInputDevice();
						m_pDeviceKeyboard->m_sName    = "Keyboard";
						m_pDeviceKeyboard->m_nType    = RIM_TYPEKEYBOARD;
						m_pDeviceKeyboard->m_hDevice  = nullptr;
						m_pDeviceKeyboard->m_bVirtual = true;
						m_lstDevices.push_back(m_pDeviceKeyboard);

						// Add virtual Mouse device (will catches all events of all mouse devices)
						m_pDeviceMouse = new RawInputDevice();
						m_pDeviceMouse->m_sName    = "Mouse";
						m_pDeviceMouse->m_nType    = RIM_TYPEMOUSE;
						m_pDeviceMouse->m_hDevice  = nullptr;
						m_pDeviceMouse->m_bVirtual = true;
						m_lstDevices.push_back(m_pDeviceMouse);
					}

					// Destroy buffer
					delete [] pDevices;
				}
			}

			/**
			*  @brief
			*    Get list of available input devices
			*
			*  @return
			*    Device list
			*
			*  @remarks
			*    There are two virtual devices:
			*    The virtual keyboard device named "Keyboard" catches all events from all keyboard devices.
			*    The virtual mouse device named "Mouse" catches all events from all mouse devices.
			*    All other devices represent real devices and have names given from Windows.
			*/
			inline const std::vector<RawInputDevice*> &GetDevices() const
			{
				return m_lstDevices;
			}


		//[-------------------------------------------------------]
		//[ Private functions                                     ]
		//[-------------------------------------------------------]
		private:
			/**
			*  @brief
			*    Default constructor
			*/
			RawInput() :
				m_hWnd(nullptr)
			{
				// Create window class
				m_cWndClass.lpszClassName	= L"PLInputWindows";
				m_cWndClass.lpfnWndProc		= WndProc;
				m_cWndClass.style			= CS_VREDRAW | CS_HREDRAW;
				m_cWndClass.hInstance		= GetModuleHandle(nullptr);
				m_cWndClass.hIcon			= nullptr;
				m_cWndClass.hCursor			= LoadCursor(nullptr, IDC_ARROW);
				m_cWndClass.hbrBackground	= reinterpret_cast<HBRUSH>(COLOR_WINDOW);
				m_cWndClass.lpszMenuName	= nullptr;
				m_cWndClass.cbClsExtra		= 0;
				m_cWndClass.cbWndExtra		= 0;
				RegisterClass(&m_cWndClass);

				// Create window
				m_hWnd = CreateWindow(L"PLInputWindows", L"PLInputWindows", WS_OVERLAPPEDWINDOW, 0, 0, 0, 0, nullptr, nullptr, GetModuleHandle(nullptr), nullptr);
				SetWindowLongPtr(m_hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));

				// Register Raw-Input devices
				RAWINPUTDEVICE cRawInputDevice[2];
				cRawInputDevice[0].usUsagePage	= HID_USAGE_PAGE_GENERIC;
				cRawInputDevice[0].usUsage		= HID_USAGE_GENERIC_MOUSE;
				cRawInputDevice[0].dwFlags		= RIDEV_INPUTSINK; // TODO(co) Might be: RIDEV_INPUTSINK, to get input even in background.
				cRawInputDevice[0].hwndTarget	= m_hWnd;
				cRawInputDevice[1].usUsagePage	= HID_USAGE_PAGE_GENERIC;
				cRawInputDevice[1].usUsage		= HID_USAGE_GENERIC_KEYBOARD;
				cRawInputDevice[1].dwFlags		= RIDEV_INPUTSINK; // TODO(co) Might be: RIDEV_INPUTSINK, to get input even in background.
				cRawInputDevice[1].hwndTarget	= m_hWnd;
				RegisterRawInputDevices(cRawInputDevice, 2, sizeof(cRawInputDevice[0]));

				// Detect devices
				DetectDevices();
			}

			explicit RawInput(const RawInput& source) = delete;

			/**
			*  @brief
			*    Destructor
			*/
			virtual ~RawInput()
			{
				// Clear
				Clear();

				// Destroy window
				if (m_hWnd) {
					DestroyWindow(m_hWnd);
					m_hWnd = nullptr;
				}

				// Delete window class
				HMODULE hModuleHandle = GetModuleHandle(nullptr);
				if (hModuleHandle)
					UnregisterClass(L"PLInputWindows", hModuleHandle);
			}

			RawInput& operator= (const RawInput& source) = delete;

			/**
			*  @brief
			*    Raw-Input message processing
			*
			*  @param[in] nMsg
			*    Received message
			*  @param[in] wParam
			*    First parameter of the received message
			*  @param[in] lParam
			*    Second parameter of the received message
			*
			*  @return
			*    Error code, 0 = success
			*/
			LRESULT ProcessRawInput(HWND, UINT, WPARAM, LPARAM lParam)
			{
				// Create buffer
				UINT nSize;
				GetRawInputData(reinterpret_cast<HRAWINPUT>(lParam), RID_INPUT, nullptr, &nSize, sizeof(RAWINPUTHEADER));
				BYTE *pBuffer = new BYTE[nSize];
				if (pBuffer) {
					// Read Raw-Input data
					if (GetRawInputData(reinterpret_cast<HRAWINPUT>(lParam), RID_INPUT, pBuffer, &nSize, sizeof(RAWINPUTHEADER)) == nSize) {
						RAWINPUT *pRawInput = reinterpret_cast<RAWINPUT*>(pBuffer);

						// Get device
						PRAGMA_WARNING_PUSH
							PRAGMA_WARNING_DISABLE_MSVC(4826)	// warning C4826: Conversion from 'HANDLE' to 'uint64_t' is sign-extended. This may cause unexpected runtime behavior.
							DeviceMap::const_iterator iterator= m_mapDevices.find(reinterpret_cast<uint64_t>(pRawInput->header.hDevice));
						PRAGMA_WARNING_POP
						RawInputDevice *pDevice = (m_mapDevices.cend() != iterator) ? iterator->second : nullptr;

						// Keyboard input
						if (pRawInput->header.dwType == RIM_TYPEKEYBOARD) {
							// Send to device
							if (pDevice) {
								pDevice->ProcessKeyboardData(
									pRawInput->data.keyboard.MakeCode,
									pRawInput->data.keyboard.Flags,
									pRawInput->data.keyboard.Reserved,
									pRawInput->data.keyboard.VKey,
									pRawInput->data.keyboard.Message,
									pRawInput->data.keyboard.ExtraInformation
								);
							}

							// Send to virtual device "Keyboard"
							if (m_pDeviceKeyboard) {
								m_pDeviceKeyboard->ProcessKeyboardData(
									pRawInput->data.keyboard.MakeCode,
									pRawInput->data.keyboard.Flags,
									pRawInput->data.keyboard.Reserved,
									pRawInput->data.keyboard.VKey,
									pRawInput->data.keyboard.Message,
									pRawInput->data.keyboard.ExtraInformation
								);
							}

						// Mouse input
						} else if (pRawInput->header.dwType == RIM_TYPEMOUSE) {
							// Send to device
							if (pDevice) {
								pDevice->ProcessMouseData(
									pRawInput->data.mouse.usFlags,
									pRawInput->data.mouse.ulButtons,
									pRawInput->data.mouse.usButtonFlags,
									pRawInput->data.mouse.usButtonData,
									static_cast<long>(pRawInput->data.mouse.ulRawButtons),
									pRawInput->data.mouse.lLastX,
									pRawInput->data.mouse.lLastY,
									pRawInput->data.mouse.ulExtraInformation
								);
							}

							// Send to virtual device "Mouse"
							if (m_pDeviceMouse) {
								m_pDeviceMouse->ProcessMouseData(
									pRawInput->data.mouse.usFlags,
									pRawInput->data.mouse.ulButtons,
									pRawInput->data.mouse.usButtonFlags,
									pRawInput->data.mouse.usButtonData,
									static_cast<long>(pRawInput->data.mouse.ulRawButtons),
									pRawInput->data.mouse.lLastX,
									pRawInput->data.mouse.lLastY,
									pRawInput->data.mouse.ulExtraInformation
								);
							}
						}
					} else {
						// Error reading buffer
					}

					// Destroy buffer
					delete [] pBuffer;
				} else {
					// Error creating buffer
				}

				// Done
				return 0;
			}


		//[-------------------------------------------------------]
		//[ Private static functions                              ]
		//[-------------------------------------------------------]
		private:
			/**
			*  @brief
			*    Window callback function
			*
			*  @param[in] hWnd
			*    Window the message came from
			*  @param[in] nMsg
			*    Received message
			*  @param[in] wParam
			*    First parameter of the received message
			*  @param[in] lParam
			*    Second parameter of the received message
			*
			*  @return
			*    Error code, 0 = success
			*/
			static LRESULT WINAPI WndProc(HWND hWnd, UINT nMsg, WPARAM wParam, LPARAM lParam)
			{
				// Catch Raw-Input messages
				if (nMsg == WM_INPUT) {
					// Get device provider instance
					RawInput *pRawInput = reinterpret_cast<RawInput*>(GetWindowLongPtr(hWnd, -21));
					if (pRawInput) {
						// Call Raw-Input message handler
						return pRawInput->ProcessRawInput(hWnd, nMsg, wParam, lParam);
					}
				}

				// Unhandled message
				return DefWindowProc(hWnd, nMsg, wParam, lParam);
			}


		//[-------------------------------------------------------]
		//[ Private definitions                                   ]
		//[-------------------------------------------------------]
		private:
			typedef std::unordered_map<uint64_t, RawInputDevice*> DeviceMap;


		//[-------------------------------------------------------]
		//[ Private data                                          ]
		//[-------------------------------------------------------]
		private:
			// Raw-Input system
			WNDCLASS m_cWndClass;	///< Window class
			HWND	 m_hWnd;		///< Window that will receive input messages

			// Raw-Input devices
			std::vector<RawInputDevice*>  m_lstDevices;			///< List of input devices
			DeviceMap					  m_mapDevices;			///< Hash map: HANDLE -> Raw-Input device
			RawInputDevice				 *m_pDeviceKeyboard;	///< Virtual keyboard device (all events from all keyboards)
			RawInputDevice				 *m_pDeviceMouse;		///< Virtual mouse device (all events from all mice)


		};

		/**
		*  @brief
		*    Input provider for Windows using Raw-Input methods for keyboard and mouse
		*/
		class RawInputProvider : public Provider
		{


		//[-------------------------------------------------------]
		//[ Public functions                                      ]
		//[-------------------------------------------------------]
		public:
			RawInputProvider() = delete;

			/**
			*  @brief
			*    Constructor
			*
			*  @param[in] inputManager
			*    Owner input manager
			*/
			inline explicit RawInputProvider(InputManager& inputManager) :
				Provider(inputManager),
				mRawInput(new RawInput())
			{
				// Nothing here
			}

			/**
			*  @brief
			*    Destructor
			*/
			inline virtual ~RawInputProvider() override
			{
				delete mRawInput;
			}

			RawInputProvider& operator= (const RawInputProvider& source) = delete;


		//[-------------------------------------------------------]
		//[ Private virtual Provider functions                    ]
		//[-------------------------------------------------------]
		private:
			virtual void QueryDevices() override
			{
				// Get list of Raw Input devices
				uint32_t nKeyboard = 0;
				uint32_t nMouse = 0;
				mRawInput->DetectDevices();
				const std::vector<RawInputDevice*> &lstDevices = mRawInput->GetDevices();
				for (uint32_t i=0; i<lstDevices.size(); i++) {
					// Get device
					RawInputDevice *pDevice = lstDevices[i];
					if (pDevice->GetType() == RIM_TYPEKEYBOARD || pDevice->GetType() == RIM_TYPEMOUSE) {
						// Set device name
						std::string sName;
						if (pDevice->GetType() == RIM_TYPEKEYBOARD)	{
							// Keyboard
							sName = "Keyboard";
							if (!pDevice->IsVirtual()) {
								sName += std::to_string(nKeyboard);
								++nKeyboard;
							}
						} else if (pDevice->GetType() == RIM_TYPEMOUSE)	{
							// Mouse
							sName = "Mouse";
							if (!pDevice->IsVirtual()) {
								sName += std::to_string(nMouse);
								++nMouse;
							}
						}

						// Check if device is already known
						if (!CheckDevice(sName)) {
							// Create device
							if (pDevice->GetType() == RIM_TYPEKEYBOARD) {
								// Create a keyboard device
								AddDevice(sName, new Keyboard(mInputManager, sName, pDevice));
							} else if (pDevice->GetType() == RIM_TYPEMOUSE) {
								// Create a mouse device
								AddDevice(sName, new Mouse(mInputManager, sName, pDevice));
							}
						}
					}
				}
			}


		//[-------------------------------------------------------]
		//[ Private data                                          ]
		//[-------------------------------------------------------]
		private:
			RawInput* mRawInput;


		};

		/**
		*  @brief
		*    Windows implementation of 'HIDDevice'
		*/
		class HIDDeviceWindows : public HIDDevice
		{


		//[-------------------------------------------------------]
		//[ Friends                                               ]
		//[-------------------------------------------------------]
		friend class HIDWindows;


		//[-------------------------------------------------------]
		//[ Public functions                                      ]
		//[-------------------------------------------------------]
		public:
			/**
			*  @brief
			*    Default constructor
			*/
			inline HIDDeviceWindows() :
				m_hDevice(nullptr),
				m_pPreparsedData(nullptr)
			{
				// Do not destroy device implementation automatically, because this is managed by HIDWindows
				m_bDelete = false;
			}

			/**
			*  @brief
			*    Destructor
			*/
			inline virtual ~HIDDeviceWindows() override
			{
				// Close a possibly open connection
				Close();

				// Release preparsed data
				if (m_pPreparsedData)
					HidD_FreePreparsedData(m_pPreparsedData);
			}


		//[-------------------------------------------------------]
		//[ Protected virtual HIDDevice functions                 ]
		//[-------------------------------------------------------]
		protected:
			virtual void ParseInputReportData(const uint8_t *pInputReport, uint32_t nSize) override
			{
				// Reset buttons to 0 (not pressed)
				for (uint32_t i=0; i<m_lstInputButtons.size(); i++)
					m_lstInputButtons[i].m_nValue = 0;

				// Create buffer
				unsigned long nItems = m_nNumberInputDataIndices;
				HIDP_DATA *pData = new HIDP_DATA[nItems];

				// Get data
				HidP_GetData(HidP_Input, pData, &nItems, m_pPreparsedData, reinterpret_cast<char*>(const_cast<uint8_t*>(pInputReport)), nSize);
				for (uint32_t i=0; i<nItems; i++) {
					// Find button with the given data index
					for (uint32_t j=0; j<m_lstInputButtons.size(); j++) {
						// Get button
						HIDCapability *pCapability = &m_lstInputButtons[j];

						// Go through all buttons (usages)
						if (pData[i].DataIndex >= pCapability->m_nDataIndexMin && pData[i].DataIndex <= pCapability->m_nDataIndexMax) {
							// Is button set?
							if (pData[i].On) {
								// Set button state to 'pressed'
								uint32_t nValue = static_cast<uint32_t>(1) << (pData[i].DataIndex - pCapability->m_nDataIndexMin);
								pCapability->m_nValue |= nValue;
							}
						}
					}

					// Find input value with the given data index
					for (uint32_t j=0; j<m_lstInputValues.size(); j++) {
						// Get input value
						HIDCapability *pCapability = &m_lstInputValues[j];
						if (pCapability->m_nDataIndex == pData[i].DataIndex) {
							// Set value
							pCapability->m_nValue = pData[i].RawValue;
						}
					}
				}

				// Delete data
				delete[] pData;
			}

			virtual void SendOutputReportData() override
			{
				// Check output report
				if (m_pOutputBuffer && m_nOutputReportSize > 0) {
					// Get number of distinct report IDs
					std::vector<uint8_t> lstReportIDs;
					for (uint32_t i=0; i<m_lstOutputValues.size(); ++i) {
						// Add report ID to list
						uint8_t nReportID = m_lstOutputValues[i].m_nReportID;
						if (std::find(lstReportIDs.cbegin(), lstReportIDs.cend(), nReportID) == lstReportIDs.cend())
						{
							lstReportIDs.push_back(nReportID);
						}
					}

					// Send one output report for every report ID
					for (uint32_t i=0; i<lstReportIDs.size(); ++i) {
						// Clear output report
						memset(m_pOutputBuffer, 0, m_nOutputReportSize);

						// Set report ID
						uint8_t nReportID = lstReportIDs[i];
						m_pOutputBuffer[0] = nReportID;

						// Set output values
						uint32_t nValues = 0;
						HIDP_DATA sData[32];
						for (uint32_t j=0; j<m_lstOutputValues.size(); j++) {
							// Get output value and check, if it belongs to the right report (check report ID)
							HIDCapability *pCapability = &m_lstOutputValues[j];
							if (pCapability->m_nReportID == nReportID) {
								// Set value data
								sData[nValues].DataIndex = pCapability->m_nDataIndex;
								sData[nValues].RawValue  = pCapability->m_nValue;
								nValues++;
							}
						}

						// Fill output report
						if (HidP_SetData(HidP_Output, sData, reinterpret_cast<PULONG>(&nValues), m_pPreparsedData, reinterpret_cast<char*>(m_pOutputBuffer), m_nOutputReportSize) == HIDP_STATUS_SUCCESS) {
							// Send report
							Write(m_pOutputBuffer, m_nOutputReportSize);
						}
					}
				}
			}


		//[-------------------------------------------------------]
		//[ Public virtual ConnectionDevice functions             ]
		//[-------------------------------------------------------]
		public:
			virtual bool Open(uint16_t, uint16_t) override
			{
				// Close first
				if (IsOpen())
					Close();

				// Connect to device
				std::wstring utf16Name;
				utf8::utf8to16(GetName().cbegin(), GetName().cend(), std::back_inserter(utf16Name));
				m_hDevice = CreateFile(utf16Name.c_str(), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, nullptr);
				if (m_hDevice != INVALID_HANDLE_VALUE) {
					// Start read thread
					InitThread();

					// Call event
					OnConnectSignal.emit();

					// Done
					return true;
				}

				// Error
				return false;
			}

			virtual bool Close() override
			{
				// Check device handle
				if (m_hDevice && m_hDevice != INVALID_HANDLE_VALUE) {
					// Stop read thread
					StopThread();

					// Call event
					OnDisconnectSignal.emit();

					// Close handle
					bool bResult = (CloseHandle(m_hDevice) != 0);
					m_hDevice = INVALID_HANDLE_VALUE;
					return bResult;
				}

				// Error
				return false;
			}

			inline virtual bool IsOpen() const override
			{
				return (m_hDevice != INVALID_HANDLE_VALUE);
			}

			virtual bool Read(uint8_t *pBuffer, uint32_t nSize) override
			{
				// Check handle
				if (m_hDevice && m_hDevice != INVALID_HANDLE_VALUE) {
					// Setup asynchronous IO
					m_sOverlapped.sOverlapped.Offset	 = 0;
					m_sOverlapped.sOverlapped.OffsetHigh = 0;
					m_sOverlapped.sOverlapped.hEvent	 = nullptr;
					m_sOverlapped.pDevice				 = this;

					// Read from device and give back immediately, "OnReadSignal" will be emitted, when data has been read
					if (ReadFileEx(m_hDevice, pBuffer, nSize, reinterpret_cast<OVERLAPPED*>(&m_sOverlapped), &HIDDeviceWindows::OnReadComplete)) {
						// Wait for read operation to complete
						SleepEx(1000, TRUE);

						// Get error condition
						return (GetLastError() == ERROR_SUCCESS);
					}
				}

				// Error!
				return false;
			}

			virtual bool Write(const uint8_t *pBuffer, uint32_t nSize) override
			{
				// Check handle
				if (m_hDevice && m_hDevice != INVALID_HANDLE_VALUE) {
					// Setup asynchronous IO
					m_sOverlapped.sOverlapped.Offset	 = 0;
					m_sOverlapped.sOverlapped.OffsetHigh = 0;
					m_sOverlapped.sOverlapped.hEvent	 = nullptr;
					m_sOverlapped.pDevice				 = this;

					// Write to device
					if (WriteFileEx(m_hDevice, pBuffer, nSize, reinterpret_cast<OVERLAPPED*>(&m_sOverlapped), &HIDDeviceWindows::OnWriteComplete)) {
						// Get error condition
						return (GetLastError() == ERROR_SUCCESS);
					}
				}

				// Error!
				return false;
			}


		//[-------------------------------------------------------]
		//[ Private static functions                              ]
		//[-------------------------------------------------------]
		private:
			static void CALLBACK OnReadComplete(DWORD, DWORD, LPOVERLAPPED lpOverlapped) noexcept
			{
				// Get object
				HIDDeviceWindows *pThis = reinterpret_cast<ExtendedOverlapped*>(lpOverlapped)->pDevice;
				if (pThis)
				{
					// Data has been read
					pThis->OnReadSignal.emit();
				}
			}

			static inline void CALLBACK OnWriteComplete(DWORD, DWORD, LPOVERLAPPED) noexcept
			{
				// Nothing here
			}


		//[-------------------------------------------------------]
		//[ Private data types                                    ]
		//[-------------------------------------------------------]
		private:
			/**
			*  @brief
			*    Data structure for asynchronous IO
			*/
			struct ExtendedOverlapped {
				OVERLAPPED		  sOverlapped;
				HIDDeviceWindows *pDevice;
			};


		//[-------------------------------------------------------]
		//[ Private data                                          ]
		//[-------------------------------------------------------]
		private:
			HANDLE					m_hDevice;			///< HID device handle
			PHIDP_PREPARSED_DATA	m_pPreparsedData;	///< HID preparsed data
			ExtendedOverlapped		m_sOverlapped;		///< Data for asynchronous IO


		};

		/**
		*  @brief
		*    Windows implementation of 'HIDImpl'
		*/
		class HIDWindows : public HIDImpl
		{


		//[-------------------------------------------------------]
		//[ Friends                                               ]
		//[-------------------------------------------------------]
		friend class HID;


		//[-------------------------------------------------------]
		//[ Private functions                                     ]
		//[-------------------------------------------------------]
		private:
			/**
			*  @brief
			*    Default constructor
			*/
			HIDWindows() :
				mHidSharedLibrary(nullptr)
			{
				// Try to load "hid.dll"
				mHidSharedLibrary = ::LoadLibraryExA("hid.dll", nullptr, LOAD_WITH_ALTERED_SEARCH_PATH);
				if (nullptr != mHidSharedLibrary)
				{
					HMODULE hModule = static_cast<HMODULE>(mHidSharedLibrary);

					// Define a helper macro
					#define IMPORT_FUNC(funcName)																																					\
					{																																												\
						void* symbol = ::GetProcAddress(hModule, #funcName);																														\
						if (nullptr != symbol)																																						\
						{																																											\
							*(reinterpret_cast<void**>(&(funcName))) = symbol;																														\
						}																																											\
						else																																										\
						{																																											\
							wchar_t moduleFilename[MAX_PATH];																																		\
							moduleFilename[0] = '\0';																																				\
							::GetModuleFileNameW(hModule, moduleFilename, MAX_PATH);																												\
						}																																											\
					}
							// TODO(co) Log support after "GetModuleFileNameW()" above
							// RENDERER_LOG(mDirect3D11Renderer.getContext(), CRITICAL, "Failed to locate the entry point \"%s\" within the HID shared library \"%s\"", #funcName, moduleFilename)

					// Get global HID function pointers
					IMPORT_FUNC(HidD_GetPreparsedData);
					IMPORT_FUNC(HidD_FreePreparsedData);
					IMPORT_FUNC(HidP_GetData);
					IMPORT_FUNC(HidP_SetData);
					IMPORT_FUNC(HidD_GetHidGuid);
					IMPORT_FUNC(HidD_GetAttributes);
					IMPORT_FUNC(HidP_GetCaps);
					IMPORT_FUNC(HidP_GetButtonCaps);
					IMPORT_FUNC(HidP_GetValueCaps);

					// Undefine the helper macro
					#undef IMPORT_FUNC

					// Ensure that we have valid function pointers
					if (!HidD_GetPreparsedData || !HidD_FreePreparsedData || !HidP_GetData || !HidP_SetData || !HidD_GetHidGuid ||
						!HidD_GetAttributes || !HidP_GetCaps || !HidP_GetButtonCaps || !HidP_GetValueCaps) {
						// Error!
						// TODO(co) Log support
						// PL_LOG(Error, "Not all required symbols within \"hid.dll\" were found, as a result, no HID devices can be used")
					}
				} else {
					// Error!
					// TODO(co) Log support
					// PL_LOG(Error, "Failed to load in \"hid.dll\", as a result, no HID devices can be used")
				}

				// Get device interface GUID
				if (HidD_GetHidGuid)
					HidD_GetHidGuid(&m_sGUID);
			}

			/**
			*  @brief
			*    Destructor
			*/
			virtual ~HIDWindows() override
			{
				// Clear devices
				for (uint32_t i=0; i<m_lstDevices.size(); ++i)
					delete m_lstDevices[i];
				m_lstDevices.clear();

				// Free the HID shared library
				if (nullptr != mHidSharedLibrary)
				{
					::FreeLibrary(static_cast<HMODULE>(mHidSharedLibrary));
				}
			}


		//[-------------------------------------------------------]
		//[ Private virtual HIDImpl functions                     ]
		//[-------------------------------------------------------]
		private:
			virtual void EnumerateDevices(std::vector<HIDDevice*> &lstDevices) override
			{
				// Clear devices
				for (uint32_t i=0; i<m_lstDevices.size(); i++)
					delete m_lstDevices[i];
				m_lstDevices.clear();

				// We're going to use HID functions, ensure that we have valid function pointers (there's no need for additional tests within HIDDeviceWindows!)
				if (!HidD_GetPreparsedData || !HidD_FreePreparsedData || !HidP_GetData || !HidP_SetData || !HidD_GetHidGuid ||
					!HidD_GetAttributes || !HidP_GetCaps || !HidP_GetButtonCaps || !HidP_GetValueCaps) {
					return;
				}

				// Create handle
				HDEVINFO hDevInfo = SetupDiGetClassDevs(&m_sGUID, nullptr, nullptr, DIGCF_DEVICEINTERFACE);
				if (!hDevInfo)
					return;

				// Get device interface
				SP_DEVICE_INTERFACE_DATA sDevice;
				sDevice.cbSize = sizeof(sDevice);
				uint32_t nDevice = 0;
				while (SetupDiEnumDeviceInterfaces(hDevInfo, nullptr, &m_sGUID, nDevice, &sDevice)) {
					// Allocate buffer for device interface details
					DWORD nDetailsSize = 0;
					SetupDiGetDeviceInterfaceDetail(hDevInfo, &sDevice, nullptr, 0, &nDetailsSize, nullptr);
					BYTE *pBuffer = new BYTE[nDetailsSize];
					SP_DEVICE_INTERFACE_DETAIL_DATA *sDeviceDetail = reinterpret_cast<SP_DEVICE_INTERFACE_DETAIL_DATA*>(pBuffer);
					sDeviceDetail->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);

					// Get device interface details
					if (SetupDiGetDeviceInterfaceDetail(hDevInfo, &sDevice, sDeviceDetail, nDetailsSize, &nDetailsSize, nullptr)) {
						// Create device
						HIDDeviceWindows *pDevice = new HIDDeviceWindows();
						std::wstring utf16Name = sDeviceDetail->DevicePath;
						utf8::utf16to8(utf16Name.cbegin(), utf16Name.cend(), std::back_inserter(pDevice->m_sName));

						// Open device to get detailed information
						HANDLE hDevice = CreateFile(sDeviceDetail->DevicePath, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, 0, nullptr);
						if (hDevice != INVALID_HANDLE_VALUE) {
							// Get device attributes
							HIDD_ATTRIBUTES sAttrib;
							sAttrib.Size = sizeof(sAttrib);
							if (HidD_GetAttributes(hDevice, &sAttrib)) {
								// Set device attributes
								pDevice->m_nVendor	= sAttrib.VendorID;
								pDevice->m_nProduct	= sAttrib.ProductID;

								// Get preparsed data
								if (HidD_GetPreparsedData(hDevice, &pDevice->m_pPreparsedData) == TRUE) {
									// Get device capabilities
									HIDP_CAPS sCaps;
									if (HidP_GetCaps(pDevice->m_pPreparsedData, &sCaps) == HIDP_STATUS_SUCCESS) {
										// Save capabilities
										pDevice->m_nUsage						= sCaps.Usage;
										pDevice->m_nUsagePage					= sCaps.UsagePage;
										pDevice->m_nInputReportSize				= sCaps.InputReportByteLength;
										pDevice->m_nOutputReportSize			= sCaps.OutputReportByteLength;
										pDevice->m_nFeatureReportByteLength		= sCaps.FeatureReportByteLength;
										pDevice->m_nNumberLinkCollectionNodes	= sCaps.NumberLinkCollectionNodes;
										pDevice->m_nNumberInputButtonCaps		= sCaps.NumberInputButtonCaps;
										pDevice->m_nNumberInputValueCaps		= sCaps.NumberInputValueCaps;
										pDevice->m_nNumberInputDataIndices		= sCaps.NumberInputDataIndices;
										pDevice->m_nNumberOutputButtonCaps		= sCaps.NumberOutputButtonCaps;
										pDevice->m_nNumberOutputValueCaps		= sCaps.NumberOutputValueCaps;
										pDevice->m_nNumberOutputDataIndices		= sCaps.NumberOutputDataIndices;
										pDevice->m_nNumberFeatureButtonCaps		= sCaps.NumberFeatureButtonCaps;
										pDevice->m_nNumberFeatureValueCaps		= sCaps.NumberFeatureValueCaps;
										pDevice->m_nNumberFeatureDataIndices	= sCaps.NumberFeatureDataIndices;

										// Enumerate buttons
										if (sCaps.NumberInputButtonCaps > 0) {
											USHORT nSize = sCaps.NumberInputButtonCaps;
											HIDP_BUTTON_CAPS *pButtons = new HIDP_BUTTON_CAPS[nSize];
											if (HidP_GetButtonCaps(HidP_Input, pButtons, &nSize, pDevice->m_pPreparsedData) == HIDP_STATUS_SUCCESS) {
												for (int i=0; i<nSize; i++) {
													// Save button control
													HIDCapability sCapability;
													sCapability.m_nLinkCollection = pButtons[i].LinkCollection;
													sCapability.m_nUsagePage	  = pButtons[i].UsagePage;
													sCapability.m_nUsage		  = pButtons[i].NotRange.Usage;
													sCapability.m_nReportID		  = pButtons[i].ReportID;
													sCapability.m_nDataIndex	  = pButtons[i].NotRange.DataIndex;
													if (pButtons[i].IsRange) {
														sCapability.m_nUsageMin		= pButtons[i].Range.UsageMin;
														sCapability.m_nUsageMax		= pButtons[i].Range.UsageMax;
														sCapability.m_nDataIndexMin = pButtons[i].Range.DataIndexMin;
														sCapability.m_nDataIndexMax = pButtons[i].Range.DataIndexMax;
														sCapability.m_nBitSize	    = 1;
													} else {
														sCapability.m_nUsageMin		= pButtons[i].NotRange.Usage;
														sCapability.m_nUsageMax		= pButtons[i].NotRange.Usage;
														sCapability.m_nDataIndexMin = pButtons[i].NotRange.DataIndex;
														sCapability.m_nDataIndexMax = pButtons[i].NotRange.DataIndex;
														sCapability.m_nBitSize	    = 1;
													}

													// Add button control
													pDevice->m_lstInputButtons.push_back(sCapability);
												}
											}
											delete [] pButtons;
										}

										// Enumerate input values
										if (sCaps.NumberInputValueCaps > 0) {
											USHORT nSize = sCaps.NumberInputValueCaps;
											HIDP_VALUE_CAPS *pValues = new HIDP_VALUE_CAPS[nSize];
											if (HidP_GetValueCaps(HidP_Input, pValues, &nSize, pDevice->m_pPreparsedData) == HIDP_STATUS_SUCCESS) {
												for (int i=0; i<nSize; ++i) {
													// Save input value control
													HIDCapability sCapability;
													sCapability.m_nLinkCollection = pValues[i].LinkCollection;
													sCapability.m_nUsagePage	  = pValues[i].UsagePage;
													sCapability.m_nUsage		  = pValues[i].NotRange.Usage;
													sCapability.m_nReportID		  = pValues[i].ReportID;
													sCapability.m_nDataIndex	  = pValues[i].NotRange.DataIndex;
													sCapability.m_nBitSize		  = pValues[i].BitSize;
													if (pValues[i].IsRange) {
														sCapability.m_nUsageMin		= pValues[i].Range.UsageMin;
														sCapability.m_nUsageMax		= pValues[i].Range.UsageMax;
														sCapability.m_nDataIndexMin = pValues[i].Range.DataIndexMin;
														sCapability.m_nDataIndexMax = pValues[i].Range.DataIndexMax;
													} else {
														sCapability.m_nUsageMin		= pValues[i].NotRange.Usage;
														sCapability.m_nUsageMax		= pValues[i].NotRange.Usage;
														sCapability.m_nDataIndexMin = pValues[i].NotRange.DataIndex;
														sCapability.m_nDataIndexMax = pValues[i].NotRange.DataIndex;
													}
													sCapability.m_nLogicalMin  = static_cast<uint16_t>(pValues[i].LogicalMin);
													sCapability.m_nLogicalMax  = static_cast<uint16_t>(pValues[i].LogicalMax);
													sCapability.m_nPhysicalMin = static_cast<uint16_t>(pValues[i].PhysicalMin);
													sCapability.m_nPhysicalMax = static_cast<uint16_t>(pValues[i].PhysicalMax);

													// Add input value control
													pDevice->m_lstInputValues.emplace_back(sCapability);
												}
											}
											delete [] pValues;
										}

										// Enumerate output values
										if (sCaps.NumberOutputValueCaps > 0) {
											USHORT nSize = sCaps.NumberOutputValueCaps;
											HIDP_VALUE_CAPS *pValues = new HIDP_VALUE_CAPS[nSize];
											if (HidP_GetValueCaps(HidP_Output, pValues, &nSize, pDevice->m_pPreparsedData) == HIDP_STATUS_SUCCESS) {
												for (int i=0; i<nSize; i++) {
													// Save output value control
													HIDCapability sCapability;
													sCapability.m_nUsagePage   = pValues[i].UsagePage;
													sCapability.m_nUsage	   = pValues[i].NotRange.Usage;
													sCapability.m_nReportID    = pValues[i].ReportID;
													sCapability.m_nDataIndex   = pValues[i].NotRange.DataIndex;
													sCapability.m_nBitSize	   = pValues[i].BitSize;
													sCapability.m_nLogicalMin  = static_cast<uint16_t>(pValues[i].LogicalMin);
													sCapability.m_nLogicalMax  = static_cast<uint16_t>(pValues[i].LogicalMax);
													sCapability.m_nPhysicalMin = static_cast<uint16_t>(pValues[i].PhysicalMin);
													sCapability.m_nPhysicalMax = static_cast<uint16_t>(pValues[i].PhysicalMax);

													// Add output value control
													pDevice->m_lstOutputValues.emplace_back(sCapability);
												}
											}
											delete [] pValues;
										}
									}
								}
							}

							// Close device handle
							CloseHandle(hDevice);

							// Device found
							m_lstDevices.push_back(pDevice);
							  lstDevices.push_back(pDevice);
						} else {
							// TODO(co) Write error message into the log?
							// Get error message
							/*
							DWORD nError = GetLastError();
							LPTSTR s;
							::FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
											nullptr, nError, 0, static_cast<LPTSTR>(&s), 0, nullptr);
							::LocalFree(s);
							*/

							// Destroy device
							delete pDevice;
						}
					}

					// Delete buffer
					delete [] pBuffer;

					// Next device
					++nDevice;
				}

				// Close info handle
				SetupDiDestroyDeviceInfoList(hDevInfo);
			}


		//[-------------------------------------------------------]
		//[ Private data                                          ]
		//[-------------------------------------------------------]
		private:
			void*						   mHidSharedLibrary;	///< Shared HID library, can be a null pointer
			std::vector<HIDDeviceWindows*> m_lstDevices;		///< List of devices
			GUID						   m_sGUID;				///< HID device interface GUID


		};


		//[-------------------------------------------------------]
		//[ HID constructor                                       ]
		//[-------------------------------------------------------]
		HID::HID() :
			m_pHIDImpl(new HIDWindows())
		{
			// Detect devices
			DetectDevices();
		}


		//[-------------------------------------------------------]
		//[ Bluetooth constructor                                 ]
		//[-------------------------------------------------------]
		Bluetooth::Bluetooth() :
			m_pBTImpl(nullptr)
		{
			// Detect devices
			DetectDevices();
		}








	//[-------------------------------------------------------]
	//[ Linux backend implementation                          ]
	//[-------------------------------------------------------]
	#elif defined(LINUX) && !defined(ANDROID) && !defined(APPLE)
		//[-------------------------------------------------------]
		//[ Includes                                              ]
		//[-------------------------------------------------------]
		// TODO(co) Remove this
		#include <PLCore/String/ParseTools.h>
		#include <PLCore/Log/Log.h>

		#include <X11/Xlib.h>
		#include <X11/Xutil.h>
		#include <X11/Xatom.h>
		#include <X11/keysym.h>

		#include <linux/input.h>

		#include <stdlib.h>
		#include <memory.h>
		#include <stdio.h>
		#include <fcntl.h>
		#include <unistd.h>
		#include <dirent.h>
		#include <dbus/dbus.h>
		#include <sys/socket.h>








		//[-------------------------------------------------------]
		//[ Linux standard input                                  ]
		//[-------------------------------------------------------]
		/**
		*  @brief
		*    Mouse implementation for Linux
		*/
		class LinuxMouseDevice : public UpdateDevice
		{


		//[-------------------------------------------------------]
		//[ Public functions                                      ]
		//[-------------------------------------------------------]
		public:
			/**
			*  @brief
			*    Default constructor
			*/
			inline LinuxMouseDevice()
				m_pDisplay(XOpenDisplay(nullptr)),
				m_nMouseX(0),
				m_nMouseY(0)
			{
				// Destroy device implementation automatically
				m_bDelete = true;
			}

			/**
			*  @brief
			*    Destructor
			*/
			inline virtual ~LinuxMouseDevice() override
			{
				// Close display
				if (m_pDisplay)
					XCloseDisplay(m_pDisplay);
			}


		//[-------------------------------------------------------]
		//[ Public virtual UpdateDevice functions                 ]
		//[-------------------------------------------------------]
		public:
			virtual void Update() override
			{
				// Check if display and input device is valid
				if (m_pDisplay && m_pDevice) {
					// Get mouse device
					Mouse *pMouse = static_cast<Mouse*>(m_pDevice);

					// Get mouse state
					::Window 	 nRootWindow, nChildWindow;
					int 		 nRootX, nRootY, nChildX, nChildY;
					unsigned int nMask;
					XLockDisplay(m_pDisplay);
					XQueryPointer(m_pDisplay, DefaultRootWindow(m_pDisplay), &nRootWindow, &nChildWindow, &nRootX, &nRootY, &nChildX, &nChildY, &nMask);
					XUnlockDisplay(m_pDisplay);

					// Update axes
					int nDeltaX = nRootX - m_nMouseX;
					int nDeltaY = nRootY - m_nMouseY;
					m_nMouseX = nRootX;
					m_nMouseY = nRootY;
					if (pMouse->X.getValue() != nDeltaX)
						pMouse->X.setValue(nDeltaX, true);
					if (pMouse->Y.getValue() != nDeltaY)
						pMouse->Y.setValue(nDeltaY, true);

					// Update buttons
					bool bPressed = ((nMask & Button1Mask) != 0);
					if (pMouse->Left.isPressed() != bPressed)
					{
						pMouse->Left.setPressed(bPressed);
					}
					bPressed = ((nMask & Button2Mask) != 0);
					if (pMouse->Right.isPressed() != bPressed)
					{
						pMouse->Right.setPressed(bPressed);
					}
					bPressed = ((nMask & Button3Mask) != 0);
					if (pMouse->Middle.isPressed() != bPressed)
					{
						pMouse->Middle.setPressed(bPressed);
					}
				}
			}


		//[-------------------------------------------------------]
		//[ Private data                                          ]
		//[-------------------------------------------------------]
		private:
			::Display *m_pDisplay;	///< X server display connection, can be a null pointer
			int		   m_nMouseX;	///< Current mouse X position
			int		   m_nMouseY;	///< Current mouse Y position


		};

		/**
		*  @brief
		*    Keyboard implementation for Linux
		*/
		class LinuxKeyboardDevice : public UpdateDevice
		{


		//[-------------------------------------------------------]
		//[ Public functions                                      ]
		//[-------------------------------------------------------]
		public:
			/**
			*  @brief
			*    Default constructor
			*/
			LinuxKeyboardDevice() :
				m_pDisplay(XOpenDisplay(nullptr))
			{
				// Destroy device implementation automatically
				m_bDelete = true;

				// Init keys as 'not pressed'
				for (int i=0; i<32; i++)
					m_nKeys[i] = 0;
			}

			/**
			*  @brief
			*    Destructor
			*/
			virtual ~LinuxKeyboardDevice() override
			{
				// Close display
				if (m_pDisplay)
					XCloseDisplay(m_pDisplay);
			}


		//[-------------------------------------------------------]
		//[ Public virtual UpdateDevice functions                 ]
		//[-------------------------------------------------------]
		public:
			virtual void Update() override
			{
				// Check if display and input device is valid
				if (m_pDisplay && m_pDevice) {
					// Get keyboard device
					Keyboard *pKeyboard = static_cast<Keyboard*>(m_pDevice);

					// Get keyboard state
					XLockDisplay(m_pDisplay);
					XQueryKeymap(m_pDisplay, m_nKeys);
					XUnlockDisplay(m_pDisplay);

					// Find changed keys
					for (int i=0; i<32*8; i++) {
						// Get state
						const int nState = (m_nKeys[i/8] >> (i%8)) & 1;

						// Get virtual key code without any modifier set (otherwise we would get different virtual key codes for one and the same physical key)
						//   Example: The user is pressing the "a"-key and the method returns "XK_a" as virtual key code.
						//            In case the user additionally is pressing the "shift"-key, the method returns "XK_A" (= a different virtual key code!).
						//            When the user releases the "a"-key on the keyboard, but is still holding the "shift"-key pressed down, then the input
						//            system wouldn't see that the "a"-key was released. The reason for this is, that the method returns "XK_A" and not "XK_a"
						//            which was the virtual key code returned when the user pressed the "a"-key in the first place.
						const KeySym nKeySym = XKeycodeToKeysym(m_pDisplay, i, 0);

						// Get button
						Button *pButton = GetKeyboardKey(pKeyboard, nKeySym);
						if (pButton) {
							// Get button state
							const bool bPressed = (nState != 0);

							// Propagate changes
							if (pButton->isPressed() != bPressed)
							{
								pButton->setPressed(bPressed);
							}
						}
					}
				}
			}


		//[-------------------------------------------------------]
		//[ Private functions                                     ]
		//[-------------------------------------------------------]
		private:
			/**
			*  @brief
			*    Get key for virtual key code
			*
			*  @param[in] pKeyboard
			*    Pointer to keyboard device, must be valid
			*  @param[in] nKeySym
			*    Virtual key code
			*
			*  @return
			*    Corresponding key, a null pointer if key code is invalid
			*/
			Button *GetKeyboardKey(Keyboard *pKeyboard, KeySym nKeySym)
			{
				// Return key that corresponds to the given key code
				switch (nKeySym) {
					case XK_BackSpace:		return &pKeyboard->Backspace;
					case XK_Tab:			return &pKeyboard->Tab;
					case XK_Clear:			return &pKeyboard->Clear;
					case XK_Shift_L:		return &pKeyboard->Shift;
					case XK_Control_L:		return &pKeyboard->Control;
					case XK_Menu:			return &pKeyboard->Alt;
					case XK_Pause:			return &pKeyboard->Pause;
					case XK_Caps_Lock:		return &pKeyboard->CapsLock;
					case XK_Escape:			return &pKeyboard->Escape;
					case XK_space:			return &pKeyboard->Space;
					case XK_Prior:			return &pKeyboard->PageUp;
					case XK_Next:			return &pKeyboard->PageDown;
					case XK_End:			return &pKeyboard->End;
					case XK_Home:			return &pKeyboard->Home;
					case XK_Left:			return &pKeyboard->Left;
					case XK_Up:				return &pKeyboard->Up;
					case XK_Right:			return &pKeyboard->Right;
					case XK_Down:			return &pKeyboard->Down;
					case XK_Select:			return &pKeyboard->Select;
					case XK_Execute:		return &pKeyboard->Execute;
					case XK_Print:			return &pKeyboard->Print;
					case XK_Insert:			return &pKeyboard->Insert;
					case XK_Delete:			return &pKeyboard->Delete;
					case XK_Help:			return &pKeyboard->Help;
					case XK_0:				return &pKeyboard->Key0;
					case XK_1:				return &pKeyboard->Key1;
					case XK_2:				return &pKeyboard->Key2;
					case XK_3:				return &pKeyboard->Key3;
					case XK_4:				return &pKeyboard->Key4;
					case XK_5:				return &pKeyboard->Key5;
					case XK_6:				return &pKeyboard->Key6;
					case XK_7:				return &pKeyboard->Key7;
					case XK_8:				return &pKeyboard->Key8;
					case XK_9:				return &pKeyboard->Key9;
					case XK_a:				return &pKeyboard->A;
					case XK_b:				return &pKeyboard->B;
					case XK_c:				return &pKeyboard->C;
					case XK_d:				return &pKeyboard->D;
					case XK_e:				return &pKeyboard->E;
					case XK_f:				return &pKeyboard->F;
					case XK_g:				return &pKeyboard->G;
					case XK_h:				return &pKeyboard->H;
					case XK_i:				return &pKeyboard->I;
					case XK_j:				return &pKeyboard->J;
					case XK_k:				return &pKeyboard->K;
					case XK_l:				return &pKeyboard->L;
					case XK_m:				return &pKeyboard->M;
					case XK_n:				return &pKeyboard->N;
					case XK_o:				return &pKeyboard->O;
					case XK_p:				return &pKeyboard->P;
					case XK_q:				return &pKeyboard->Q;
					case XK_r:				return &pKeyboard->R;
					case XK_s:				return &pKeyboard->S;
					case XK_t:				return &pKeyboard->T;
					case XK_u:				return &pKeyboard->U;
					case XK_v:				return &pKeyboard->V;
					case XK_w:				return &pKeyboard->W;
					case XK_x:				return &pKeyboard->X;
					case XK_y:				return &pKeyboard->Y;
					case XK_z:				return &pKeyboard->Z;
					case XK_KP_0:			return &pKeyboard->Numpad0;
					case XK_KP_1:			return &pKeyboard->Numpad1;
					case XK_KP_2:			return &pKeyboard->Numpad2;
					case XK_KP_3:			return &pKeyboard->Numpad3;
					case XK_KP_4:			return &pKeyboard->Numpad4;
					case XK_KP_5:			return &pKeyboard->Numpad5;
					case XK_KP_6:			return &pKeyboard->Numpad6;
					case XK_KP_7:			return &pKeyboard->Numpad7;
					case XK_KP_8:			return &pKeyboard->Numpad8;
					case XK_KP_9:			return &pKeyboard->Numpad9;
					case XK_KP_Multiply:	return &pKeyboard->NumpadMultiply;
					case XK_KP_Add:			return &pKeyboard->NumpadAdd;
					case XK_KP_Separator:	return &pKeyboard->NumpadSeparator;
					case XK_KP_Subtract:	return &pKeyboard->NumpadSubtract;
					case XK_KP_Decimal:		return &pKeyboard->NumpadDecimal;
					case XK_KP_Divide:		return &pKeyboard->NumpadDivide;
					case XK_F1:				return &pKeyboard->F1;
					case XK_F2:				return &pKeyboard->F2;
					case XK_F3:				return &pKeyboard->F3;
					case XK_F4:				return &pKeyboard->F4;
					case XK_F5:				return &pKeyboard->F5;
					case XK_F6:				return &pKeyboard->F6;
					case XK_F7:				return &pKeyboard->F7;
					case XK_F8:				return &pKeyboard->F8;
					case XK_F9:				return &pKeyboard->F9;
					case XK_F10:			return &pKeyboard->F10;
					case XK_F11:			return &pKeyboard->F11;
					case XK_F12:			return &pKeyboard->F12;
					case XK_Num_Lock:		return &pKeyboard->NumLock;
					case XK_Scroll_Lock:	return &pKeyboard->ScrollLock;
					case XK_asciicircum:	return &pKeyboard->Circumflex;
					default:				return  nullptr;
				}
			}


		//[-------------------------------------------------------]
		//[ Private data                                          ]
		//[-------------------------------------------------------]
		private:
			::Display	*m_pDisplay;	///< X server display connection, can be a null pointer
			char		 m_nKeys[32];	///< State of all keys


		};

		/**
		*  @brief
		*    Linux device implementation using the event subsystem of input.h
		*/
		class LinuxEventDevice : public UpdateDevice
		{


		//[-------------------------------------------------------]
		//[ Public functions                                      ]
		//[-------------------------------------------------------]
		public:
			/**
			*  @brief
			*    Constructor
			*
			*  @param[in] nFile
			*    File handle
			*/
			explicit LinuxEventDevice(int nFile)
				m_nFile(nFile),
				m_nBusType(0),
				m_nVendor(0),
				m_nProduct(0),
				m_nVersion(0)
			{
				// Do not destroy device implementation automatically, because this is managed by HIDLinux
				m_bDelete = false;

				// Get device info
				input_id sDeviceInfo;
				if (!ioctl(m_nFile, EVIOCGID, &sDeviceInfo)) {
					// Save device info
					m_nBusType = sDeviceInfo.bustype;
					m_nVendor  = sDeviceInfo.vendor;
					m_nProduct = sDeviceInfo.product;
					m_nVersion = sDeviceInfo.version;
				}
			}

			/**
			*  @brief
			*    Destructor
			*/
			inline virtual ~LinuxEventDevice() override
			{
				// Close file
				close(m_nFile);
			}

			/**
			*  @brief
			*    Get bus type
			*
			*  @return
			*    Bus type
			*/
			inline uint16_t GetBusType() const
			{
				return m_nBusType;
			}

			/**
			*  @brief
			*    Get vendor ID
			*
			*  @return
			*    Vendor ID
			*/
			inline uint16_t GetVendorID() const
			{
				return m_nVendor;
			}

			/**
			*  @brief
			*    Get product ID
			*
			*  @return
			*    Product ID
			*/
			inline uint16_t GetProductID() const
			{
				return m_nProduct;
			}

			/**
			*  @brief
			*    Get version
			*
			*  @return
			*    Version
			*/
			inline uint16_t GetVersion() const
			{
				return m_nVersion;
			}


		//[-------------------------------------------------------]
		//[ Public virtual UpdateDevice functions                 ]
		//[-------------------------------------------------------]
		public:
			virtual void Update() override
			{
				// Read input events
				struct input_event pEvents[64];
				ssize_t nSize = read(m_nFile, pEvents, sizeof(struct input_event)*64);
				int nEvents = (nSize > 0 ? static_cast<int>(nSize / sizeof(struct input_event)) : 0);
				for (int i=0; i<nEvents; i++) {
					// Get corresponding control
					Control *pControl = nullptr;
					if (pEvents[i].code == ABS_X)
						pControl = m_pDevice->GetControl("X");
					else if (pEvents[i].code == ABS_Y)
						pControl = m_pDevice->GetControl("Y");
					else if (pEvents[i].code == ABS_Z)
						pControl = m_pDevice->GetControl("Z");
					else if (pEvents[i].code == ABS_RX)
						pControl = m_pDevice->GetControl("RX");
					else if (pEvents[i].code == ABS_RY)
						pControl = m_pDevice->GetControl("RY");
					else if (pEvents[i].code == ABS_RZ)
						pControl = m_pDevice->GetControl("RZ");
					else if (pEvents[i].code == ABS_HAT0X)
						pControl = m_pDevice->GetControl("Hat");
					else if (pEvents[i].code >= BTN_JOYSTICK && pEvents[i].code < BTN_GAMEPAD)
						pControl = m_pDevice->GetControl(std::string("Button") + (pEvents[i].code - BTN_JOYSTICK));
					else if (pEvents[i].code >= BTN_GAMEPAD && pEvents[i].code < BTN_DIGI)
						pControl = m_pDevice->GetControl(std::string("Button") + (pEvents[i].code - BTN_GAMEPAD));

					// Get control type
					Axis   *pAxis   = (pControl != nullptr && pControl->getControlType() == ControlType::AXIS)   ? static_cast<Axis*>  (pControl) : nullptr;
					Button *pButton = (pControl != nullptr && pControl->getControlType() == ControlType::BUTTON) ? static_cast<Button*>(pControl) : nullptr;

					// Set control value
					if (pEvents[i].type == EV_KEY && pButton)
					{
						// Button
						pButton->setPressed(pEvents[i].value != 0);
					}
					else if ((pEvents[i].type == EV_ABS || pEvents[i].type == EV_REL) && pAxis)
					{
						// Absolute or relative axis
						// Get minimum and maximum values
						input_absinfo sAbsInfo;
						if (!ioctl(m_nFile, EVIOCGABS(0), &sAbsInfo))
						{
							// Get value in a range of -1.0 - +1.0
							float fValue = (static_cast<float>(pEvents[i].value - sAbsInfo.minimum) / static_cast<float>(sAbsInfo.maximum - sAbsInfo.minimum)) * 2.0f - 1.0f;
							if (fValue >  1.0f)
								fValue =  1.0f;
							if (fValue < -1.0f)
								fValue = -1.0f;
							pAxis->setValue(fValue, false);
						}
					}
				}
			}


		//[-------------------------------------------------------]
		//[ Private data                                          ]
		//[-------------------------------------------------------]
		private:
			int		 m_nFile;		///< File handle
			uint16_t m_nBusType;	///< Bus type (e.g. USB)
			uint16_t m_nVendor;		///< Vendor ID
			uint16_t m_nProduct;	///< Product ID
			uint16_t m_nVersion;	///< Version


		};

		/**
		*  @brief
		*    Standard input provider for Linux
		*/
		class LinuxProvider : public Provider
		{


		//[-------------------------------------------------------]
		//[ Public functions                                      ]
		//[-------------------------------------------------------]
		public:
			LinuxProvider() = delete;

			/**
			*  @brief
			*    Constructor
			*
			*  @param[in] inputManager
			*    Owner input manager
			*/
			explicit LinuxProvider(InputManager& inputManager) :
				Provider(inputManager)
			{
				// Nothing here
			}

			/**
			*  @brief
			*    Destructor
			*/
			virtual ~LinuxProvider() override
			{
				// Nothing here
			}

			LinuxProvider& operator= (const LinuxProvider& source) = delete;


		//[-------------------------------------------------------]
		//[ Private virtual Provider functions                    ]
		//[-------------------------------------------------------]
		private:
			virtual void QueryDevices() override
			{
				// Create a keyboard device
				if (!CheckDevice("Keyboard")) {
					// Add device
					LinuxKeyboardDevice *pImpl = new LinuxKeyboardDevice();
					AddDevice("Keyboard", new Keyboard("Keyboard", pImpl));
				}

				// Create a mouse device
				if (!CheckDevice("Mouse")) {
					LinuxMouseDevice *pImpl = new LinuxMouseDevice();
					AddDevice("Mouse", new Mouse("Mouse", pImpl));
				}

				// List devices in "/dev/input/event*"
				DIR *pDir = opendir("/dev/input");
				if (pDir) {
					int nDevice = 0;

					// Read first entry
					dirent *pEntry = readdir(pDir);
					while (pEntry) {
						// Check if filename is "eventX"
						std::string sFilename = pEntry->d_name;
						if (sFilename.GetSubstring(0, 5) == "event") {
							// Try to open the device
							int f = open(("/dev/input/" + sFilename).GetASCII(), O_RDWR | O_NONBLOCK);
							if (f > 0) {
								// Create device
								LinuxEventDevice *pImpl = new LinuxEventDevice(f);
								std::string sName = std::string("Joystick") + nDevice;
								AddDevice(sName, new Joystick(sName, pImpl));
								nDevice++;
							}
						}

						// Read next entry
						pEntry = readdir(pDir);
					}

					// Be polite and close the directory after we're done...
					closedir(pDir);
				}
			}


		};





		//[-------------------------------------------------------]
		//[ HID                                                   ]
		//[-------------------------------------------------------]
		/**
		*  @brief
		*    Linux implementation of 'HIDDevice'
		*/
		class HIDDeviceLinux : public HIDDevice
		{


		//[-------------------------------------------------------]
		//[ Public functions                                      ]
		//[-------------------------------------------------------]
		public:
			/**
			*  @brief
			*    Default constructor
			*/
			inline HIDDeviceLinux()
			{
				// Do not destroy device implementation automatically, because this is managed by HIDLinux
				m_bDelete = false;
			}

			/**
			*  @brief
			*    Destructor
			*/
			inline virtual ~HIDDeviceLinux() override
			{
				// Close a possibly open connection
				Close();
			}


		//[-------------------------------------------------------]
		//[ Public virtual ConnectionDevice functions             ]
		//[-------------------------------------------------------]
		public:
			virtual bool Open(uint16_t nOutputPort = 0, uint16_t nInputPort = 0) override
			{
				// Close first
				if (IsOpen())
					Close();

				// TODO(co)
				/*
				// Connect

				// Start read thread
				InitThread();

				// Call event
				OnConnectSignal.emit();

				// Done
				return true;
				*/

				// Error
				return false;
			}

			virtual bool Close() override
			{
				if (IsOpen()) {
					// TODO(co)
					/*
					// Stop read thread
					StopThread();

					// Call event
					OnDisconnectSignal.emit();

					// Disconnect

					// Done
					return true;
					*/
				}

				// Error
				return false;
			}

			virtual bool IsOpen() const override
			{
				// TODO(co)
				return false;
			}

			virtual bool Read(uint8_t *pBuffer, uint32_t nSize) override
			{
				// TODO(co)

				// Error!
				return false;
			}

			virtual bool Write(const uint8_t *pBuffer, uint32_t nSize) override
			{
				// TODO(co)

				// Error!
				return false;
			}


		};

		/**
		*  @brief
		*    Linux implementation of 'HIDImpl'
		*
		*  @remarks
		*    This implementation uses the event subsystem (input.h) to access generic
		*    HID devices. Therefore, the HID interface is not fully supported, so no
		*    HID-interna like UsePage-IDs or similar will be present when using this backend.
		*/
		class HIDLinux : public HIDImpl
		{


		//[-------------------------------------------------------]
		//[ Friends                                               ]
		//[-------------------------------------------------------]
		friend class HID;


		//[-------------------------------------------------------]
		//[ Private functions                                     ]
		//[-------------------------------------------------------]
		private:
			/**
			*  @brief
			*    Default constructor
			*/
			inline HIDLinux()
			{
				// Nothing here
			}

			/**
			*  @brief
			*    Destructor
			*/
			virtual ~HIDLinux() override
			{
				// Clear devices
				for (uint32_t i=0; i<m_lstDevices.size(); i++)
					delete m_lstDevices[i];
				m_lstDevices.clear();
			}


		//[-------------------------------------------------------]
		//[ Private virtual HIDImpl functions                     ]
		//[-------------------------------------------------------]
		private:
			virtual void EnumerateDevices(std::vector<HIDDevice*> &lstDevices) override
			{
				// Clear devices
				for (uint32_t i=0; i<m_lstDevices.size(); i++)
					delete m_lstDevices[i];
				m_lstDevices.clear();
			}


		//[-------------------------------------------------------]
		//[ Private data                                          ]
		//[-------------------------------------------------------]
		private:
			std::vector<HIDDeviceLinux*> m_lstDevices;	///< List of devices


		};








		//[-------------------------------------------------------]
		//[ Bluetooth                                             ]
		//[-------------------------------------------------------]
		//[-------------------------------------------------------]
		//[ Public definitions                                    ]
		//[-------------------------------------------------------]
		/**
		*  @brief
		*    _NET_WM state actions
		*/
		#define _NET_WM_STATE_REMOVE	0	///< Remove/unset property
		#define _NET_WM_STATE_ADD		1	///< Add/set property
		#define _NET_WM_STATE_TOGGLE	2	///< Toggle property

		/**
		*  @brief
		*    XEMBED messages
		*/
		#define XEMBED_EMBEDDED_NOTIFY			 0	///< Window has been embedded
		#define XEMBED_WINDOW_ACTIVATE			 1	///< Window has been activated
		#define XEMBED_WINDOW_DEACTIVATE		 2	///< Window has been de-activated
		#define XEMBED_REQUEST_FOCUS			 3	///< Window requests to get the focus
		#define XEMBED_FOCUS_IN					 4	///< Window has got the focus
		#define XEMBED_FOCUS_OUT				 5	///< Window has lost the focus
		#define XEMBED_FOCUS_NEXT				 6	///< Window has reached the end of it's logical tab (tabbing forward)
		#define XEMBED_FOCUS_PREV				 7	///< Window has reached the beginning of it's logical tab (tabbing backwards)
		#define XEMBED_MODALITY_ON				10	///< Window becomes shadowed by a modal dialog
		#define XEMBED_MODALITY_OFF				11	///< Window has been released from a modal dialog
		#define XEMBED_REGISTER_ACCELERATOR		12	///< Register a key accelerator
		#define XEMBED_UNREGISTER_ACCELERATOR	13	///< Unregister a key accelerator
		#define XEMBED_ACTIVATE_ACCELERATOR		14	///< An accelerator has been activated

		/**
		*  @brief
		*    XEMBED flags
		*/
		#define XEMBED_MAPPED	(1 << 0)	///< Client is visible

		/**
		*  @brief
		*    XEMBED options for XEMBED_FOCUS_IN
		*/
		#define XEMBED_FOCUS_CURRENT	0	///< Do not move logical focus
		#define XEMBED_FOCUS_FIRST		1	///< Activate first item in list
		#define XEMBED_FOCUS_LAST		2	///< Activate last item in list

		/**
		*  @brief
		*    XEMBED modifier keys for XEMBED_FOCUS_IN
		*/
		#define XEMBED_MODIFIER_SHIFT	(1 << 0)	///< Shift key
		#define XEMBED_MODIFIER_CONTROL	(1 << 1)	///< Control key ("Ctrl")
		#define XEMBED_MODIFIER_ALT		(1 << 2)	///< ALT key
		#define XEMBED_MODIFIER_SUPER	(1 << 3)	///< Super key
		#define XEMBED_MODIFIER_HYPER	(1 << 4)	///< Hyper key

		/**
		*  @brief
		*    System tray messages
		*/
		#define SYSTEM_TRAY_REQUEST_DOCK	0	///< Dock into systray
		#define SYSTEM_TRAY_BEGIN_MESSAGE	1	///< Show message
		#define SYSTEM_TRAY_CANCEL_MESSAGE	2	///< Hide message

		/**
		*  @brief
		*    Mouse buttons
		*/
		#ifdef Button1
			namespace PLGuiLinuxIncludes {
				enum {
					X11_Button1 = Button1,
					X11_Button2 = Button2,
					X11_Button3 = Button3,
					X11_Button4 = Button4,
					X11_Button5 = Button5
				};
			}

			#undef Button1
			#undef Button2
			#undef Button3
			#undef Button4
			#undef Button5

			namespace XLib {
				enum {
					Button1 = PLGuiLinuxIncludes::X11_Button1,
					Button2 = PLGuiLinuxIncludes::X11_Button2,
					Button3 = PLGuiLinuxIncludes::X11_Button3,
					Button4 = PLGuiLinuxIncludes::X11_Button4,
					Button5 = PLGuiLinuxIncludes::X11_Button5
				};
			}
		#endif

		/**
		*  @brief
		*    Misc
		*/
		#ifdef None
			namespace PLGuiLinuxIncludes {
				enum {
					X11_None = None,
					X11_Always = Always,
					X11_Above = Above,
					X11_Success = Success
				};
			}

			#undef None
			#undef Always
			#undef Above
			#undef Success

			namespace XLib {
				enum {
					None = PLGuiLinuxIncludes::X11_None,
					Always = PLGuiLinuxIncludes::X11_Always,
					Above = PLGuiLinuxIncludes::X11_Above,
					Success = PLGuiLinuxIncludes::X11_Success
				};
			}
		#endif


		//[-------------------------------------------------------]
		//[ Bluetooth definitions                                 ]
		//[-------------------------------------------------------]
		#include <asm/byteorder.h>

		#define htobs(a)        __cpu_to_le16(a)
		#define BTPROTO_L2CAP   0

		typedef struct {
			unsigned char b[6];
		} __attribute__((packed)) bdaddr_t;

		struct sockaddr_l2 {
			sa_family_t     l2_family;
			unsigned short  l2_psm;
			bdaddr_t        l2_bdaddr;
		};


		//[-------------------------------------------------------]
		//[ Definitions                                           ]
		//[-------------------------------------------------------]
		static constexpr int TIMEOUT = 10;	///< Timeout for detecting devices

		/**
		*  @brief
		*    Bluetooth transport types
		*/
		enum ETrans {
			TransHandshake				= 0x00,		// Handshake request
			TransSetReport				= 0x50,		// Report request
			TransData					= 0xA0		// Data request
		};

		/**
		*  @brief
		*    Bluetooth parameters
		*/
		enum EParam {
			ParamInput					= 0x01,		// Input
			ParamOutput					= 0x02,		// Output
			ParamFeature				= 0x03		// Feature
		};

		/**
		*  @brief
		*    Bluetooth handshake responses
		*/
		enum EResult {
			ResultSuccess				= 0x00,		// Handshake OK
			ResultNotReady				= 0x01,		// Device not ready
			ResultErrInvalidReportID	= 0x02,		// Error: Invalid report ID
			ResultErrUnsupportedRequest	= 0x03,		// Error: Unsupported Request
			ResultErrInvalidParameter	= 0x04,		// Error: Invalid parameter
			ResultErrUnknown			= 0x0E,		// Error: Unknown error
			ResultErrFatal				= 0x0F		// Error: Fatal error
		};

		// Internals
		static constexpr int BufferSize	= 128;		// Size of temporary read/write buffer


		/**
		*  @brief
		*    Linux implementation of 'BTDevice'
		*/
		class BTDeviceLinux : public BTDevice
		{


		//[-------------------------------------------------------]
		//[ Friends                                               ]
		//[-------------------------------------------------------]
		friend class BTLinux;


		//[-------------------------------------------------------]
		//[ Private functions                                     ]
		//[-------------------------------------------------------]
		private:
			/**
			*  @brief
			*    Default constructor
			*/
			inline BTDeviceLinux() :
				m_nCtrlSocket(0),
				m_nIntSocket(0)
			{
				// Do not destroy device implementation automatically, because this is managed by BTLinux
				m_bDelete = false;
			}

			/**
			*  @brief
			*    Destructor
			*/
			inline virtual ~BTDeviceLinux() override
			{
				// Nothing here
			}

			/**
			*  @brief
			*    Read handshake response
			*
			*  @return
			*    'true' if the handshake was successful, else 'false'
			*/
			bool ReadHandshake()
			{
				unsigned char nHandshake;
				if (read(m_nCtrlSocket, &nHandshake, 1) != 1) {
					// Error: Could not read handshake
					return false;
				} else if ((nHandshake & 0xF0) != TransHandshake) {
					// Error: Did not receive handshake
					return false;
				} else if ((nHandshake & 0x0F) != ResultSuccess) {
					// Error: Handshake non-successful
					switch (nHandshake & 0x0F) {
						case ResultNotReady:
						case ResultErrInvalidReportID:
						case ResultErrUnsupportedRequest:
						case ResultErrInvalidParameter:
						case ResultErrUnknown:
						case ResultErrFatal:
							break;
					}
					return false;
				}

				// Handshake successful
				return true;
			}


		//[-------------------------------------------------------]
		//[ Public virtual ConnectionDevice functions             ]
		//[-------------------------------------------------------]
		public:
			virtual bool Open(uint16_t nOutputPort = 0, uint16_t nInputPort = 0) override
			{
				// Create socket address for control socket
				struct sockaddr_l2 sCtrlAddr;
				memset(&sCtrlAddr, 0, sizeof(sCtrlAddr));
				sCtrlAddr.l2_family		 = AF_BLUETOOTH;
				sCtrlAddr.l2_bdaddr.b[0] = GetAddress(0);
				sCtrlAddr.l2_bdaddr.b[1] = GetAddress(1);
				sCtrlAddr.l2_bdaddr.b[2] = GetAddress(2);
				sCtrlAddr.l2_bdaddr.b[3] = GetAddress(3);
				sCtrlAddr.l2_bdaddr.b[4] = GetAddress(4);
				sCtrlAddr.l2_bdaddr.b[5] = GetAddress(5);
				sCtrlAddr.l2_psm		 = htobs(nOutputPort);

				// Create socket address for interrupt socket
				struct sockaddr_l2 sIntAddr;
				memset(&sIntAddr, 0, sizeof(sIntAddr));
				sIntAddr.l2_family		= AF_BLUETOOTH;
				sIntAddr.l2_bdaddr.b[0] = GetAddress(0);
				sIntAddr.l2_bdaddr.b[1] = GetAddress(1);
				sIntAddr.l2_bdaddr.b[2] = GetAddress(2);
				sIntAddr.l2_bdaddr.b[3] = GetAddress(3);
				sIntAddr.l2_bdaddr.b[4] = GetAddress(4);
				sIntAddr.l2_bdaddr.b[5] = GetAddress(5);
				sIntAddr.l2_psm			= htobs(nInputPort);

				// Create output socket
				m_nCtrlSocket = socket(AF_BLUETOOTH, SOCK_SEQPACKET, BTPROTO_L2CAP);
				if (connect(m_nCtrlSocket, reinterpret_cast<struct sockaddr*>(&sCtrlAddr), sizeof(sCtrlAddr)) == 0) {
					// Create input socket
					m_nIntSocket = socket(AF_BLUETOOTH, SOCK_SEQPACKET, BTPROTO_L2CAP);
					if (connect(m_nIntSocket, reinterpret_cast<struct sockaddr*>(&sIntAddr), sizeof(sIntAddr)) == 0) {
						// Connection successful
						int nFlags = fcntl(m_nIntSocket, F_GETFL, 0);
						fcntl(m_nIntSocket, F_SETFL, nFlags | O_NONBLOCK);

						// Start read-thread
						InitThread();

						// Device connected
						OnConnectSignal.emit();

						return true;
					}
				}

				// Error!
				return false;
			}

			virtual bool Close() override
			{
				// Stop read-thread
				StopThread();

				// Device disconnected
				OnDisconnectSignal.emit();

				// Close sockets
				close(m_nCtrlSocket);
				close(m_nIntSocket);
				return true;
			}

			inline virtual bool IsOpen() const override
			{
				return (m_nCtrlSocket != 0 && m_nIntSocket != 0);
			}

			virtual bool Read(uint8_t *pBuffer, uint32_t nSize) override
			{
				// Read data
				LockCriticalSection();
				uint8_t nTemp[BufferSize];
				int nRes = read(m_nIntSocket, nTemp, nSize+1);
				if (nRes > 0) {
					if (nTemp[0] == (TransData | ParamInput)) {
						memcpy(pBuffer, &nTemp[1], nRes-1);
						UnlockCriticalSection();
						OnReadSignal.emit();
						return true;
					}
				}

				// Error!
				UnlockCriticalSection();
				return false;
			}

			virtual bool Write(const uint8_t *pBuffer, uint32_t nSize) override
			{
				// Write data
				LockCriticalSection();
				uint8_t nTemp[BufferSize];
				nTemp[0] = TransSetReport | ParamOutput;
				memcpy(nTemp+1, pBuffer, nSize);
				int nRes = write(m_nCtrlSocket, nTemp, nSize+1);
				ReadHandshake();
				UnlockCriticalSection();
				if (nRes > 0)
					return (static_cast<uint32_t>(nRes) - 1 == nSize);
				else
					return false;
			}


		//[-------------------------------------------------------]
		//[ Private data                                          ]
		//[-------------------------------------------------------]
		private:
			int m_nCtrlSocket;	///< Control channel socket
			int m_nIntSocket;	///< Interrupt channel socket


		};

		/**
		*  @brief
		*    Linux implementation of 'BTImpl'
		*/
		class BTLinux : public BTImpl
		{


		//[-------------------------------------------------------]
		//[ Friends                                               ]
		//[-------------------------------------------------------]
		friend class Bluetooth;


		//[-------------------------------------------------------]
		//[ Private functions                                     ]
		//[-------------------------------------------------------]
		private:
			/**
			*  @brief
			*    Default constructor
			*/
			inline BTLinux()
			{
				// Nothing here
			}

			/**
			*  @brief
			*    Destructor
			*/
			virtual ~BTLinux() override
			{
				// Clear devices
				for (uint32_t i=0; i<m_lstDevices.size(); i++)
					delete m_lstDevices[i];
				m_lstDevices.clear();
			}

			/**
			*  @brief
			*    Enumerate Bluetooth devices
			*
			*  @param[out] lstDevices
			*    List that will receive the devices
			*  @param[in]  cConnection
			*    Used DBus connection
			*/
			void EnumerateBluetoothDevices(std::vector<BTDevice*> &lstDevices, DBusConnection &cConnection)
			{
				// Initialize error value
				DBusError sError;
				dbus_error_init(&sError);

				// Listen to signals from org.bluez.Adapter
				dbus_bus_add_match(&cConnection, "type='signal',interface='org.bluez.Adapter'", &sError);
				dbus_connection_flush(&cConnection);
				if (dbus_error_is_set(&sError)) {
					PL_LOG(Error, "BTLinux: DBUS match error (" + std::string(sError.message) + ')')
				} else {
					// Listen for signals
					bool bAbort = false;
					while (!bAbort) {
						// Read next message
						dbus_connection_read_write(&cConnection, 0);
						DBusMessage *pMessage = dbus_connection_pop_message(&cConnection);
						if (pMessage) {
							// Check signal
							if (dbus_message_is_signal(pMessage, "org.bluez.Adapter", "DeviceFound")) {
								// org.bluez.Adapter.DeviceFound
								std::string sDeviceAddress;
								std::string sDeviceName;
								uint32_t nDeviceClass = 0;

								// Get arguments
								DBusMessageIter sIter;
								dbus_message_iter_init(pMessage, &sIter);
								int nType = 0;
								while ((nType = dbus_message_iter_get_arg_type(&sIter)) != DBUS_TYPE_INVALID) {
									// Check argument type
									if (nType == DBUS_TYPE_STRING) {
										// Device address
										char *pszAddress = nullptr;
										dbus_message_iter_get_basic(&sIter, &pszAddress);
										if (pszAddress)
											sDeviceAddress = pszAddress;
									} else if (nType == DBUS_TYPE_ARRAY) {
										// Get device infos
										DBusMessageIter sArrayIter;
										for (dbus_message_iter_recurse(&sIter, &sArrayIter);
												dbus_message_iter_get_arg_type(&sArrayIter) != DBUS_TYPE_INVALID;
												dbus_message_iter_next(&sArrayIter))
										{
											// Dictionary entry
											if (dbus_message_iter_get_arg_type(&sArrayIter) == DBUS_TYPE_DICT_ENTRY) {
												// Get values
												DBusMessageIter sDictIter;
												dbus_message_iter_recurse (&sArrayIter, &sDictIter);
												if (dbus_message_iter_get_arg_type(&sDictIter) == DBUS_TYPE_STRING) {
													// Get name
													char *pszName;
													dbus_message_iter_get_basic(&sDictIter, &pszName);
													std::string sName = pszName;

													// Next
													dbus_message_iter_next(&sDictIter);
													if (dbus_message_iter_get_arg_type(&sDictIter) == DBUS_TYPE_VARIANT) {
														DBusMessageIter sVariantIter;
														dbus_message_iter_recurse(&sDictIter, &sVariantIter);
														if (dbus_message_iter_get_arg_type(&sVariantIter) == DBUS_TYPE_STRING) {
															// Get value
															char *pszValue = nullptr;
															dbus_message_iter_get_basic(&sVariantIter, &pszValue);

															// Save value
															if (sName == "Name" && pszValue) {
																// Device name
																sDeviceName = pszValue;
															}
														} else if (dbus_message_iter_get_arg_type(&sVariantIter) == DBUS_TYPE_UINT32) {
															// Get value
															uint32_t nValue = 0;
															dbus_message_iter_get_basic(&sVariantIter, &nValue);

															// Save value
															if (sName == "Class")
																nDeviceClass = nValue;
														}
													}
												}
											}
										}
									}
									dbus_message_iter_next(&sIter);
								}

								// Device info
								PL_LOG(Info, "BTLinux: Found device '" + sDeviceName + "', Address = " + sDeviceAddress)

								// Convert address from string to bytes
								const int nAddress0 = ParseTools::ParseHexValue(sDeviceAddress.GetSubstring( 0, 2));
								const int nAddress1 = ParseTools::ParseHexValue(sDeviceAddress.GetSubstring( 3, 2));
								const int nAddress2 = ParseTools::ParseHexValue(sDeviceAddress.GetSubstring( 6, 2));
								const int nAddress3 = ParseTools::ParseHexValue(sDeviceAddress.GetSubstring( 9, 2));
								const int nAddress4 = ParseTools::ParseHexValue(sDeviceAddress.GetSubstring(12, 2));
								const int nAddress5 = ParseTools::ParseHexValue(sDeviceAddress.GetSubstring(15, 2));

								// Set device info
								BTDeviceLinux *pDevice = new BTDeviceLinux();
								pDevice->m_sName = sDeviceName;
								pDevice->m_nAddress[0] = nAddress5;
								pDevice->m_nAddress[1] = nAddress4;
								pDevice->m_nAddress[2] = nAddress3;
								pDevice->m_nAddress[3] = nAddress2;
								pDevice->m_nAddress[4] = nAddress1;
								pDevice->m_nAddress[5] = nAddress0;
								pDevice->m_nAddress[6] = 0;
								pDevice->m_nAddress[7] = 0;
								pDevice->m_nClass[0] = (nDeviceClass >>  0) & 255;
								pDevice->m_nClass[1] = (nDeviceClass >>  8) & 255;
								pDevice->m_nClass[2] = (nDeviceClass >> 16) & 255;

								// Add device
								m_lstDevices.Add(pDevice);
								lstDevices.Add(pDevice);

								// Device found, not stop
								bAbort = true;
							} else if (dbus_message_is_signal(pMessage, "org.bluez.Adapter", "PropertyChanged")) {
								// org.bluez.Adapter.PropertyChanged
								DBusMessageIter sIter;
								dbus_message_iter_init(pMessage, &sIter);
								if (dbus_message_iter_get_arg_type(&sIter) == DBUS_TYPE_STRING) {
									// Get name
									char *pszName;
									dbus_message_iter_get_basic(&sIter, &pszName);
									std::string sName = pszName;

									// 'Discovering'
									if (sName == "Discovering") {
										// Get value
										dbus_message_iter_next(&sIter);
										if (dbus_message_iter_get_arg_type(&sIter) == DBUS_TYPE_VARIANT) {
											// Get variant
											DBusMessageIter sVariantIter;
											dbus_message_iter_recurse(&sIter, &sVariantIter);
											if (dbus_message_iter_get_arg_type(&sVariantIter) == DBUS_TYPE_BOOLEAN) {
												// Get device discovery state
												bool bState = false;
												dbus_message_iter_get_basic(&sVariantIter, &bState);

												// Stop loop when Discovering=false
												if (!bState)
													bAbort = true;
											}
										}
									}
								}
							}

							// Release message
							dbus_message_unref(pMessage);
						}
					}
				}

				// De-initialize error value
				dbus_error_free(&sError);
			}


		//[-------------------------------------------------------]
		//[ Private virtual BTImpl functions                      ]
		//[-------------------------------------------------------]
		private:
			virtual void EnumerateDevices(std::vector<BTDevice*> &lstDevices) override
			{
				// Clear devices
				for (uint32_t i=0; i<m_lstDevices.size(); i++)
					delete m_lstDevices[i];
				m_lstDevices.clear();

				// Initialize DBUS
			//	dbus_threads_init_default();

				// Initialize error value
				DBusError sError;
				dbus_error_init(&sError);

				// Get DBUS connection
				PL_LOG(Info, "BTLinux: Discovering Bluetooth devices")
				DBusConnection *pConnection = dbus_bus_get(DBUS_BUS_SYSTEM, &sError);
				if (pConnection) {
					// Get default Bluetooth adapter
					DBusMessage *pMessage = dbus_message_new_method_call("org.bluez", "/", "org.bluez.Manager", "DefaultAdapter");
					DBusMessage *pReply = dbus_connection_send_with_reply_and_block(pConnection, pMessage, -1, &sError);
					dbus_message_unref(pMessage);
					if (pReply) {
						// Get adapter name
						const char *pszAdapter = nullptr;
						dbus_message_get_args(pReply, &sError, DBUS_TYPE_OBJECT_PATH, &pszAdapter, DBUS_TYPE_INVALID);
						std::string sAdapter = pszAdapter;
						dbus_message_unref(pReply);

						// Set timeout for device discovery
						const char *pszTimeout = "DiscoverableTimeout";
						int nTimeout = TIMEOUT;
						pMessage = dbus_message_new_method_call("org.bluez", sAdapter.GetASCII(), "org.bluez.Adapter", "SetProperty");
						DBusMessageIter sIter;
						DBusMessageIter sVariantIter;
						dbus_message_iter_init_append(pMessage, &sIter);
						dbus_message_iter_append_basic(&sIter, DBUS_TYPE_STRING, &pszTimeout);
						dbus_message_iter_open_container(&sIter, DBUS_TYPE_VARIANT, DBUS_TYPE_UINT32_AS_STRING, &sVariantIter);
						dbus_message_iter_append_basic(&sVariantIter, DBUS_TYPE_UINT32, &nTimeout);
						dbus_message_iter_close_container(&sIter, &sVariantIter);
						pReply = dbus_connection_send_with_reply_and_block(pConnection, pMessage, -1, &sError);
						if (pReply)
							dbus_message_unref(pReply);
						if (pMessage)
							dbus_message_unref(pMessage);

						if (dbus_error_is_set(&sError)) {
							PL_LOG(Error, "BTLinux (Set timeout for device discovery): DBUS error (" + std::string(sError.message) + ')')
						} else {
							// Start device discovery
							pMessage = dbus_message_new_method_call("org.bluez", sAdapter.GetASCII(), "org.bluez.Adapter", "StartDiscovery");
							pReply = dbus_connection_send_with_reply_and_block(pConnection, pMessage, -1, &sError);
							if (pReply)
								dbus_message_unref(pReply);
							if (pMessage)
								dbus_message_unref(pMessage);

							if (dbus_error_is_set(&sError))
								PL_LOG(Error, "BTLinux (Start device discovery): DBUS error (" + std::string(sError.message) + ')')
							else
								EnumerateBluetoothDevices(lstDevices, *pConnection);
						}
					}

					// Close connection
					dbus_error_free(&sError);
					dbus_connection_unref(pConnection);
				} else {
					PL_LOG(Error, "BTLinux: Could not create DBUS connection")
				}
			}


		//[-------------------------------------------------------]
		//[ Private data                                          ]
		//[-------------------------------------------------------]
		private:
			std::vector<BTDeviceLinux*> m_lstDevices;	///< List of devices


		};


		//[-------------------------------------------------------]
		//[ HID constructor                                       ]
		//[-------------------------------------------------------]
		HID::HID() :
			m_pHIDImpl(new HIDLinux())
		{
			// Detect devices
			DetectDevices();
		}


		//[-------------------------------------------------------]
		//[ Bluetooth constructor                                 ]
		//[-------------------------------------------------------]
		Bluetooth::Bluetooth() :
			m_pBTImpl(new BTLinux())
		{
			// Detect devices
			DetectDevices();
		}








	//[-------------------------------------------------------]
	//[ MacOSX backend implementation                         ]
	//[-------------------------------------------------------]
	#elif defined(APPLE)
		//[-------------------------------------------------------]
		//[ MacOSX standard input                                 ]
		//[-------------------------------------------------------]
		/**
		*  @brief
		*    Keyboard implementation for Mac OS X
		*/
		class MacOSXKeyboardDevice : public UpdateDevice
		{


		//[-------------------------------------------------------]
		//[ Public functions                                      ]
		//[-------------------------------------------------------]
		public:
			/**
			*  @brief
			*    Default constructor
			*/
			inline MacOSXKeyboardDevice()
			{
				// Destroy device implementation automatically
				m_bDelete = true;
			}

			/**
			*  @brief
			*    Destructor
			*/
			inline virtual ~MacOSXKeyboardDevice() override
			{
				// Nothing here
			}


		//[-------------------------------------------------------]
		//[ Public virtual UpdateDevice functions                 ]
		//[-------------------------------------------------------]
		public:
			virtual void Update() override
			{
				// Check if input device is valid
				if (m_pDevice) {
					// Get keyboard device
					Keyboard *pKeyboard = static_cast<Keyboard*>(m_pDevice);

					// TODO(co) Implement Mac OS X version
					// -> "Cocoa Event Handling Guide" page 13 looks promising: https://developer.apple.com/library/mac/documentation/Cocoa/Conceptual/EventOverview/EventOverview.pdf
				}
			}


		};

		/**
		*  @brief
		*    Mouse implementation for Mac OS X
		*/
		class MacOSXMouseDevice : public UpdateDevice
		{


		//[-------------------------------------------------------]
		//[ Public functions                                      ]
		//[-------------------------------------------------------]
		public:
			/**
			*  @brief
			*    Default constructor
			*/
			inline MacOSXMouseDevice()
			{
				// Destroy device implementation automatically
				m_bDelete = true;
			}

			/**
			*  @brief
			*    Destructor
			*/
			inline virtual ~MacOSXMouseDevice()
			{
				// Nothing here
			}


		//[-------------------------------------------------------]
		//[ Public virtual UpdateDevice functions                 ]
		//[-------------------------------------------------------]
		public:
			virtual void Update() override
			{
				// Check if input device is valid
				if (m_pDevice) {
					// Get mouse device
					Mouse *pMouse = static_cast<Mouse*>(m_pDevice);

					// TODO(co) Implement Mac OS X version
					// -> "Cocoa Event Handling Guide" page 13 looks promising: https://developer.apple.com/library/mac/documentation/Cocoa/Conceptual/EventOverview/EventOverview.pdf
				}
			}


		};

		/**
		*  @brief
		*    Standard input provider for Mac OS X
		*/
		class MacOSXProvider : public Provider
		{


		//[-------------------------------------------------------]
		//[ Public functions                                      ]
		//[-------------------------------------------------------]
		public:
			MacOSXProvider() = delete;

			/**
			*  @brief
			*    Constructor
			*
			*  @param[in] inputManager
			*    Owner input manager
			*/
			inline explicit MacOSXProvider(InputManager& inputManager)
				Provider(inputManager)
			{
				// Nothing here
			}

			/**
			*  @brief
			*    Destructor
			*/
			inline virtual ~MacOSXProvider() override
			{
				// Nothing here
			}

			MacOSXProvider& operator= (const MacOSXProvider& source) = delete;


		//[-------------------------------------------------------]
		//[ Private virtual Provider functions                    ]
		//[-------------------------------------------------------]
		private:
			virtual void QueryDevices() override
			{
				// Create a keyboard device
				if (!CheckDevice("Keyboard")) {
					// Add device
					MacOSXKeyboardDevice *pImpl = new MacOSXKeyboardDevice();
					AddDevice("Keyboard", new Keyboard("Keyboard", pImpl));
				}

				// Create a mouse device
				if (!CheckDevice("Mouse")) {
					MacOSXMouseDevice *pImpl = new MacOSXMouseDevice();
					AddDevice("Mouse", new Mouse("Mouse", pImpl));
				}
			}


		};


		//[-------------------------------------------------------]
		//[ HID constructor                                       ]
		//[-------------------------------------------------------]
		HID::HID() :
			m_pHIDImpl(nullptr)
		{
			// Detect devices
			DetectDevices();
		}


		//[-------------------------------------------------------]
		//[ Bluetooth constructor                                 ]
		//[-------------------------------------------------------]
		Bluetooth::Bluetooth() :
			m_pBTImpl(nullptr)
		{
			// Detect devices
			DetectDevices();
		}








	//[-------------------------------------------------------]
	//[ Android backend implementation                        ]
	//[-------------------------------------------------------]
	#elif defined(ANDROID)
		//[-------------------------------------------------------]
		//[ Includes                                              ]
		//[-------------------------------------------------------]
		// TODO(co) Remove this
		#include <PLCore/Log/Log.h>
		#include <PLCore/System/System.h>
		#include <PLCore/System/SystemAndroid.h>

		#include <android/input.h>
		#include <android/sensor.h>
		#include <android_native_app_glue.h>


		//[-------------------------------------------------------]
		//[ Global functions                                      ]
		//[-------------------------------------------------------]
		/**
		*  @brief
		*    Returns the sign of the given value
		*/
		inline float sign(float x)
		{
			return (x < 0.0f) ? -1.0f : 1.0f;
		}


		//[-------------------------------------------------------]
		//[ Android standard input                                ]
		//[-------------------------------------------------------]
		/**
		*  @brief
		*    Keyboard implementation for Android
		*/
		class AndroidKeyboardDevice : public UpdateDevice
		{


		//[-------------------------------------------------------]
		//[ Public functions                                      ]
		//[-------------------------------------------------------]
		public:
			/**
			*  @brief
			*    Default constructor
			*/
			inline AndroidKeyboardDevice()
			{
				// Destroy device implementation automatically
				m_bDelete = true;
			}

			/**
			*  @brief
			*    Destructor
			*/
			inline virtual ~AndroidKeyboardDevice() override
			{
				// Nothing here
			}

			/**
			*  @brief
			*    Call this to process the next key input event
			*
			*  @param[in] cAKeyInputEvent
			*    Key input event to process
			*/
			void OnKeyInputEvent(const struct AInputEvent &cAKeyInputEvent)
			{
				// Check if input device is valid
				if (m_pDevice) {
					// Get Android key code
					const int32_t nKeyCode = AKeyEvent_getKeyCode(&cAKeyInputEvent);

					// Lookout! The virtual keyboard of Android sends "down" and "up" directly one after another
					// -> This is really a problem and we have to delay further keys...
					if (m_lstProcessedKeys.IsElement(nKeyCode)) {
						// Add key for later processing
						KeyInfo sKeyInfo;
						sKeyInfo.nKeyCode = nKeyCode;
						sKeyInfo.bPressed = (AKeyEvent_getAction(&cAKeyInputEvent) == AKEY_EVENT_ACTION_DOWN);
						m_lstDelayedKeys.Add(sKeyInfo);
					} else {
						// Get keyboard device
						Keyboard *pKeyboard = static_cast<Keyboard*>(m_pDevice);

						// Get button
						Button *pButton = GetKeyboardKey(*pKeyboard, nKeyCode);
						if (pButton) {
							// Get button state
							const bool bPressed = (AKeyEvent_getAction(&cAKeyInputEvent) == AKEY_EVENT_ACTION_DOWN);

							// Propagate changes
							if (pButton->isPressed() != bPressed)
							{
								pButton->setPressed(bPressed);
							}
						}

						// Add this key to the processed keys
						m_lstProcessedKeys.Add(nKeyCode);
					}
				}
			}


		//[-------------------------------------------------------]
		//[ Public virtual UpdateDevice functions                 ]
		//[-------------------------------------------------------]
		public:
			virtual void Update() override
			{
				// Check if input device is valid
				if (m_pDevice) {
					// Get keyboard device
					Keyboard *pKeyboard = static_cast<Keyboard*>(m_pDevice);

					// Process delayed keys
					for (uint32_t i=0; i<m_lstDelayedKeys.size(); i++) {
						const KeyInfo &sKeyInfo = m_lstDelayedKeys[i];

						// Get button
						Button *pButton = GetKeyboardKey(*pKeyboard, sKeyInfo.nKeyCode);
						if (pButton) {
							// Get button state
							const bool bPressed = sKeyInfo.bPressed;

							// Propagate changes
							if (pButton->isPressed() != bPressed)
							{
								pButton->setPressed(bPressed);
							}
						}
					}
					m_lstDelayedKeys.clear();
					m_lstProcessedKeys.clear();
				}
			}


		//[-------------------------------------------------------]
		//[ Private functions                                     ]
		//[-------------------------------------------------------]
		private:
			/**
			*  @brief
			*    Get key for Android key code
			*
			*  @param[in] cKeyboard
			*    Reference to keyboard device
			*  @param[in] int32_t
			*    Key code
			*
			*  @return
			*    Corresponding key, a null pointer if Android key code is invalid
			*/
			Button *GetKeyboardKey(Keyboard &cKeyboard, int32_t nKeyCode)
			{
				// Return key that corresponds to the given Android key code
				switch (nKeyCode) {
					// AKEYCODE_SOFT_LEFT			-> ignored
					// AKEYCODE_SOFT_RIGHT			-> ignored
					case AKEYCODE_HOME:				return &cKeyboard.Home;
					case AKEYCODE_BACK:				return &cKeyboard.Backspace;
					// AKEYCODE_CALL				-> ignored
					// AKEYCODE_ENDCAL				-> ignored
					case AKEYCODE_0:				return &cKeyboard.Key0;
					case AKEYCODE_1:				return &cKeyboard.Key1;
					case AKEYCODE_2:				return &cKeyboard.Key2;
					case AKEYCODE_3:				return &cKeyboard.Key3;
					case AKEYCODE_4:				return &cKeyboard.Key4;
					case AKEYCODE_5:				return &cKeyboard.Key5;
					case AKEYCODE_6:				return &cKeyboard.Key6;
					case AKEYCODE_7:				return &cKeyboard.Key7;
					case AKEYCODE_8:				return &cKeyboard.Key8;
					case AKEYCODE_9:				return &cKeyboard.Key9;
					// AKEYCODE_STAR				-> ignored
					// AKEYCODE_POUND				-> ignored
					case AKEYCODE_DPAD_UP:			return &cKeyboard.Up;
					case AKEYCODE_DPAD_DOWN:		return &cKeyboard.Down;
					case AKEYCODE_DPAD_LEFT:		return &cKeyboard.Left;
					case AKEYCODE_DPAD_RIGHT:		return &cKeyboard.Right;
					// AKEYCODE_DPAD_CENTER			-> ignored
					// AKEYCODE_VOLUME_UP			-> ignored
					// AKEYCODE_VOLUME_DOWN			-> ignored
					// AKEYCODE_POWER				-> ignored
					// AKEYCODE_CAMERA				-> ignored
					case AKEYCODE_CLEAR:			return &cKeyboard.Clear;
					case AKEYCODE_A:				return &cKeyboard.A;
					case AKEYCODE_B:				return &cKeyboard.B;
					case AKEYCODE_C:				return &cKeyboard.C;
					case AKEYCODE_D:				return &cKeyboard.D;
					case AKEYCODE_E:				return &cKeyboard.E;
					case AKEYCODE_F:				return &cKeyboard.F;
					case AKEYCODE_G:				return &cKeyboard.G;
					case AKEYCODE_H:				return &cKeyboard.H;
					case AKEYCODE_I:				return &cKeyboard.I;
					case AKEYCODE_J:				return &cKeyboard.J;
					case AKEYCODE_K:				return &cKeyboard.K;
					case AKEYCODE_L:				return &cKeyboard.L;
					case AKEYCODE_M:				return &cKeyboard.M;
					case AKEYCODE_N:				return &cKeyboard.N;
					case AKEYCODE_O:				return &cKeyboard.O;
					case AKEYCODE_P:				return &cKeyboard.P;
					case AKEYCODE_Q:				return &cKeyboard.Q;
					case AKEYCODE_R:				return &cKeyboard.R;
					case AKEYCODE_S:				return &cKeyboard.S;
					case AKEYCODE_T:				return &cKeyboard.T;
					case AKEYCODE_U:				return &cKeyboard.U;
					case AKEYCODE_V:				return &cKeyboard.V;
					case AKEYCODE_W:				return &cKeyboard.W;
					case AKEYCODE_X:				return &cKeyboard.X;
					case AKEYCODE_Y:				return &cKeyboard.Y;
					case AKEYCODE_Z:				return &cKeyboard.Z;
					// AKEYCODE_COMMA				-> ignored
					// AKEYCODE_PERIOD				-> ignored
					case AKEYCODE_ALT_LEFT:			return &cKeyboard.Alt;
					case AKEYCODE_ALT_RIGHT:		return &cKeyboard.Control;
					case AKEYCODE_SHIFT_LEFT:		return &cKeyboard.Shift;
					// AKEYCODE_SHIFT_RIGHT			-> ignored
					case AKEYCODE_TAB:				return &cKeyboard.Tab;
					case AKEYCODE_SPACE:			return &cKeyboard.Space;
					// AKEYCODE_SYM					-> ignored
					// AKEYCODE_EXPLORER			-> ignored
					// AKEYCODE_ENVELOPE			-> ignored
					case AKEYCODE_ENTER:			return &cKeyboard.Return;
					case AKEYCODE_DEL:				return &cKeyboard.Delete;
					// AKEYCODE_GRAVE				-> ignored
					case AKEYCODE_MINUS:			return &cKeyboard.NumpadSubtract;
					// AKEYCODE_EQUALS				-> ignored
					// AKEYCODE_LEFT_BRACKET		-> ignored
					// AKEYCODE_RIGHT_BRACKET		-> ignored
					// AKEYCODE_BACKSLASH			-> ignored
					// AKEYCODE_SEMICOLON			-> ignored
					// AKEYCODE_APOSTROPHE			-> ignored
					// AKEYCODE_SLASH				-> ignored
					// AKEYCODE_AT					-> ignored
					// AKEYCODE_NUM					-> ignored
					// AKEYCODE_HEADSETHOOK			-> ignored
					// AKEYCODE_FOCUS				-> ignored
					case AKEYCODE_PLUS:				return &cKeyboard.NumpadAdd;
					// AKEYCODE_MENU				-> ignored
					// AKEYCODE_NOTIFICATION		-> ignored
					// AKEYCODE_SEARCH				-> ignored
					case AKEYCODE_MEDIA_PLAY_PAUSE:	return &cKeyboard.Pause;
					// AKEYCODE_MEDIA_STOP			-> ignored
					// AKEYCODE_MEDIA_NEXT			-> ignored
					// AKEYCODE_MEDIA_PREVIOUS		-> ignored
					// AKEYCODE_MEDIA_REWIND		-> ignored
					// AKEYCODE_MEDIA_FAST_FORWARD	-> ignored
					// AKEYCODE_MUTE				-> ignored
					case AKEYCODE_PAGE_UP:			return &cKeyboard.PageUp;
					case AKEYCODE_PAGE_DOWN:		return &cKeyboard.PageDown;
					// AKEYCODE_PICTSYMBOLS			-> ignored
					// AKEYCODE_SWITCH_CHARSET		-> ignored
					case AKEYCODE_BUTTON_A:			return &cKeyboard.A;
					case AKEYCODE_BUTTON_B:			return &cKeyboard.B;
					case AKEYCODE_BUTTON_C:			return &cKeyboard.C;
					case AKEYCODE_BUTTON_X:			return &cKeyboard.X;
					case AKEYCODE_BUTTON_Y:			return &cKeyboard.Y;
					case AKEYCODE_BUTTON_Z:			return &cKeyboard.Z;
					// AKEYCODE_BUTTON_L1			-> ignored
					// AKEYCODE_BUTTON_R1			-> ignored
					// AKEYCODE_BUTTON_L2			-> ignored
					// AKEYCODE_BUTTON_R2			-> ignored
					// AKEYCODE_BUTTON_THUMBL		-> ignored
					// AKEYCODE_BUTTON_THUMBR		-> ignored
					// AKEYCODE_BUTTON_START		-> ignored
					case AKEYCODE_BUTTON_SELECT:	return &cKeyboard.Select;
					// AKEYCODE_BUTTON_MODE			-> ignored
					default:						return nullptr;
				}
			}


		//[-------------------------------------------------------]
		//[ Private data                                          ]
		//[-------------------------------------------------------]
		private:
			std::vector<int32_t> m_lstProcessedKeys;
			class KeyInfo {
				public:
					int32_t nKeyCode;
					bool    bPressed;
					bool operator ==(const KeyInfo &cSource) { return false; }
			};
			std::vector<KeyInfo> m_lstDelayedKeys;


		};

		/**
		*  @brief
		*    Mouse implementation for Android
		*
		*  @remarks
		*    Mouse emulation by using the touchscreen. When moving around the finger on the touchscreen,
		*    the change in movement is used for the mouse axis. A short touch without any movement is handled
		*    as "left mouse button clicked". As soon the mouse is moved, no "left mouse button"-events can be
		*    generated anymore during the current touch. When touching without movement for half a second, the
		*    emulation changes into "left mouse button hold down"-mode, following mouse movement will not change
		*    this mode.
		*/
		class AndroidMouseDevice : public UpdateDevice
		{


		//[-------------------------------------------------------]
		//[ Public functions                                      ]
		//[-------------------------------------------------------]
		public:
			/**
			*  @brief
			*    Constructor
			*
			*  @param[in] pAndroidSplitTouchPadDevice
			*    Optional Android split touch pad device this device interacts with, can be a null pointer
			*/
			inline explicit AndroidMouseDevice(AndroidSplitTouchPadDevice *pAndroidSplitTouchPadDevice)
				m_pAndroidSplitTouchPadDevice(pAndroidSplitTouchPadDevice),
				m_bMouseMoved(false),
				m_fPreviousMousePositionX(0.0f),
				m_fPreviousMousePositionY(0.0f),
				m_fPreviousMousePressure(0.0f),
				m_fMousePositionX(0.0f),
				m_fMousePositionY(0.0f),
				m_fMousePressure(0.0f),
				m_bLeftMouseButton(false)
			{
				// Destroy device implementation automatically
				m_bDelete = true;
			}

			/**
			*  @brief
			*    Destructor
			*/
			inline virtual ~AndroidMouseDevice() override
			{
				// Nothing here
			}

			/**
			*  @brief
			*    Call this to process the next motion input event
			*
			*  @param[in] cAMotionInputEvent
			*    Motion input event to process
			*/
			void OnMotionInputEvent(const struct AInputEvent &cAMotionInputEvent)
			{
				// Get the number of pointers of data contained in this event
				const size_t nAndroidPointerCount = AMotionEvent_getPointerCount(&cAMotionInputEvent);
				if (nAndroidPointerCount) {
					// Get the current X and Y coordinate of this event for the given pointer index
					m_fMousePositionX = AMotionEvent_getX(&cAMotionInputEvent, 0);
					m_fMousePositionY = AMotionEvent_getY(&cAMotionInputEvent, 0);
					m_fMousePressure  = AMotionEvent_getPressure(&cAMotionInputEvent, 0);

					// Get the combined motion event action code and the action code
					const int32_t nAndroidCombinedAction = AMotionEvent_getAction(&cAMotionInputEvent);
					const int32_t nAndroidAction		 = (nAndroidCombinedAction & AMOTION_EVENT_ACTION_MASK);

					// Touch end?
					if (nAndroidAction == AMOTION_EVENT_ACTION_UP) {
						// Jap, touch end, previous mouse position = current mouse position
						m_fPreviousMousePositionX = m_fMousePositionX;
						m_fPreviousMousePositionY = m_fMousePositionY;
						m_fPreviousMousePressure = m_fMousePressure;

						// Mouse moved during the current touch? If no, this is handled as a left mouse button click as well.
						if (!m_bMouseMoved && !m_bLeftMouseButton) {
							// Check if input device is valid
							if (m_pDevice) {
								// Get mouse device
								Mouse *pMouse = static_cast<Mouse*>(m_pDevice);

								// Update button
								if (pMouse->Left.isPressed())
								{
									pMouse->Left.setPressed(true);
								}
							}
						}

						// The left mouse button is now no longer down
						m_bLeftMouseButton = false;
					} else {
						// Touch start?
						if (nAndroidAction == AMOTION_EVENT_ACTION_DOWN) {
							// Jap, touch start, previous mouse position = current mouse position
							m_fPreviousMousePositionX = m_fMousePositionX;
							m_fPreviousMousePositionY = m_fMousePositionY;
							m_fPreviousMousePressure = m_fMousePressure;

							// The mouse was not yet moved
							m_bMouseMoved = false;

							// The left mouse button is not pressed
							m_bLeftMouseButton = false;
						}

						// As long as the mouse was not yet moved, a "left mouse button is hold down" can still be generated
						if (!m_bMouseMoved && !m_bLeftMouseButton) {
							// Get the past time since the touch has been started (in nanoseconds)
							const int64_t nPastTime = AMotionEvent_getEventTime(&cAMotionInputEvent) - AMotionEvent_getDownTime(&cAMotionInputEvent);

							// If the mouse has not been moved for half a second, we go into "left mouse button is hold down"-mode
							if (nPastTime > 500*1000*1000) {
								// The left mouse button is now down
								m_bLeftMouseButton = true;
							}
						}
					}
				}
			}


		//[-------------------------------------------------------]
		//[ Public virtual UpdateDevice functions                 ]
		//[-------------------------------------------------------]
		public:
			virtual void Update() override
			{
				// Check if input device is valid
				if (m_pDevice) {
					// Get mouse device
					Mouse *pMouse = static_cast<Mouse*>(m_pDevice);

					// Update relative axes?
					// ->  In case there's an active "AndroidSplitTouchPadDevice"-instance we have to deactivate the
					//     "mouse movement"-emulation or this will conflict with "AndroidSplitTouchPadDevice"
					if (!m_pAndroidSplitTouchPadDevice || !m_pAndroidSplitTouchPadDevice->GetDevice() || !m_pAndroidSplitTouchPadDevice->GetDevice()->GetActive()) {
						// Get the mouse movement
						float fDeltaX = m_fMousePositionX - m_fPreviousMousePositionX;
						float fDeltaY = m_fMousePositionY - m_fPreviousMousePositionY;
						float fDeltaPressure = m_fMousePressure - m_fPreviousMousePressure;

						// Was the mouse already moved? (if so, we're in "mouse move"-mode, not in "left mouse button click"-mode)
						if (!m_bMouseMoved) {
							// Check whether or not the mouse was moved - with a little bit of tolerance
							if ((fabs(fDeltaX) > 6 || fabs(fDeltaY) > 6) && fabs(fDeltaPressure) < 0.4f) {
								m_bMouseMoved = true;
							} else {
								fDeltaX = fDeltaY = 0.0f;
								m_fPreviousMousePositionX = m_fMousePositionX;
								m_fPreviousMousePositionY = m_fMousePositionY;
							}
						}

						// Update axes
						if (pMouse->X.getValue() != fDeltaX)
							pMouse->X.setValue(fDeltaX, true);
						if (pMouse->Y.getValue() != fDeltaY)
							pMouse->Y.setValue(fDeltaY, true);

						// The current mouse position becomes the previous mouse position
						m_fPreviousMousePositionX = m_fMousePositionX;
						m_fPreviousMousePositionY = m_fMousePositionY;
					}

					// Update buttons
					if (pMouse->Left.isPressed() != m_bLeftMouseButton)
					{
						pMouse->Left.setPressed(m_bLeftMouseButton);
					}
				}
			}


		//[-------------------------------------------------------]
		//[ Private data                                          ]
		//[-------------------------------------------------------]
		private:
			AndroidSplitTouchPadDevice *m_pAndroidSplitTouchPadDevice;	///< Optional Android split touch pad device this device interacts with, can be a null pointer
			bool						m_bMouseMoved;					///< Mouse moved during the current touch?
			float						m_fPreviousMousePositionX;		///< Previous mouse x position
			float						m_fPreviousMousePositionY;		///< Previous mouse y position
			float						m_fPreviousMousePressure;		///< Previous mouse pressure
			float						m_fMousePositionX;				///< Current mouse x position
			float						m_fMousePositionY;				///< Current mouse y position
			float						m_fMousePressure;				///< Current mouse pressure
			bool						m_bLeftMouseButton;				///< Is the left mouse button currently down?


		};

		/**
		*  @brief
		*    Android gamepad device emulation by using a touch screen making it possible to e.g. move & look at the same time
		*/
		class AndroidSplitTouchPadDevice : public UpdateDevice
		{


		//[-------------------------------------------------------]
		//[ Public functions                                      ]
		//[-------------------------------------------------------]
		public:
			/**
			*  @brief
			*    Default constructor
			*/
			inline AndroidSplitTouchPadDevice()
				// Left
				m_nLeftTouchPointerID(-1),
				m_fOriginLeftTouchPositionX(0.0f),
				m_fOriginLeftTouchPositionY(0.0f),
				m_fLeftTouchPositionX(0.0f),
				m_fLeftTouchPositionY(0.0f),
				// Right
				m_nRightTouchPointerID(-1),
				m_fOriginRightTouchPositionX(0.0f),
				m_fOriginRightTouchPositionY(0.0f),
				m_fRightTouchPositionX(0.0f),
				m_fRightTouchPositionY(0.0f)
			{
				// Destroy device implementation automatically
				m_bDelete = true;
			}

			/**
			*  @brief
			*    Destructor
			*/
			inline virtual ~AndroidSplitTouchPadDevice() override
			{
				// Nothing here
			}

			/**
			*  @brief
			*    Call this to process the next motion input event
			*
			*  @param[in] cAMotionInputEvent
			*    Motion input event to process
			*/
			void OnMotionInputEvent(const struct AInputEvent &cAMotionInputEvent)
			{
				// We need the display size and orientation for splitting the screen
				struct android_app *pAndroidApp = reinterpret_cast<SystemAndroid*>(System::GetInstance())->GetAndroidApp();
				if (pAndroidApp) {
					ANativeWindow *pANativeWindow = pAndroidApp->window;
					if (pANativeWindow) {
						// Get the screen width: This is our base line for splitting
						const int32_t nScreenWidth = ANativeWindow_getWidth(pANativeWindow);

						// Get the number of pointers of data contained in this event
						const size_t nAndroidPointerCount = AMotionEvent_getPointerCount(&cAMotionInputEvent);

						// Evaluate every point
						for (size_t i=0; i<nAndroidPointerCount; i++) {
							size_t nAndroidPointerID    = AMotionEvent_getPointerId(&cAMotionInputEvent, i);
							size_t nAndroidAction       = (AMotionEvent_getAction(&cAMotionInputEvent) & AMOTION_EVENT_ACTION_MASK);
							size_t nAndroidPointerIndex = i;
							if (nAndroidAction == AMOTION_EVENT_ACTION_POINTER_DOWN || nAndroidAction == AMOTION_EVENT_ACTION_POINTER_UP) {
								nAndroidPointerIndex = (AMotionEvent_getAction(&cAMotionInputEvent) & AMOTION_EVENT_ACTION_POINTER_INDEX_MASK) >> AMOTION_EVENT_ACTION_POINTER_INDEX_SHIFT;
								nAndroidPointerID    = AMotionEvent_getPointerId(&cAMotionInputEvent, nAndroidPointerIndex);
							}

							// Get the current X and Y coordinate of this event for the given pointer index
							const float fPointerPositionX = AMotionEvent_getX(&cAMotionInputEvent, nAndroidPointerIndex);
							const float fPointerPositionY = AMotionEvent_getY(&cAMotionInputEvent, nAndroidPointerIndex);

							// Does the touch start?
							if (nAndroidAction == AMOTION_EVENT_ACTION_DOWN || nAndroidAction == AMOTION_EVENT_ACTION_POINTER_DOWN) {
								// Decide if the point is in the left or right screen half
								if (fPointerPositionX < (nScreenWidth/2)) {
									// This is the on the left half of the screen
									if (m_nLeftTouchPointerID == -1) {
										// This is our initial start point (origin position = current position)
										m_fOriginLeftTouchPositionX = m_fLeftTouchPositionX = fPointerPositionX;
										m_fOriginLeftTouchPositionY = m_fLeftTouchPositionY = fPointerPositionY;

										// We're now active, save the ID of this pointer
										m_nLeftTouchPointerID = nAndroidPointerID;
									}
								} else {
									// This is on the right half of the screen
									if (m_nRightTouchPointerID == -1) {
										// This is our initial start point (origin position = current position)
										m_fOriginRightTouchPositionX = m_fRightTouchPositionX = fPointerPositionX;
										m_fOriginRightTouchPositionY = m_fRightTouchPositionY = fPointerPositionY;

										// We're now active, save the ID of this pointer
										m_nRightTouchPointerID = nAndroidPointerID;
									}
								}

							// Does the touch stop?
							} else if (nAndroidAction == AMOTION_EVENT_ACTION_UP || nAndroidAction == AMOTION_EVENT_ACTION_POINTER_UP) {
								// Use the pointer ID to figure out whether this is our left or right pointer
								if (m_nLeftTouchPointerID == nAndroidPointerID) {
									// We're now longer active
									m_nLeftTouchPointerID = -1;

									// Let the left simulated pad stick snap back to it's origin
									SplitTouchPad *pSplitTouchPad = static_cast<SplitTouchPad*>(m_pDevice);
									if (pSplitTouchPad) {
										pSplitTouchPad->LeftX.setValue(0.0f, false);
										pSplitTouchPad->LeftY.setValue(0.0f, false);
									}
									m_fLeftTouchPositionX = m_fOriginLeftTouchPositionX;
									m_fLeftTouchPositionY = m_fOriginLeftTouchPositionY;

								} else if (m_nRightTouchPointerID == nAndroidPointerID) {
									// We're now longer active
									m_nRightTouchPointerID = -1;

									// Let the right simulated pad stick snap back to it's origin
									SplitTouchPad *pSplitTouchPad = static_cast<SplitTouchPad*>(m_pDevice);
									if (pSplitTouchPad) {
										pSplitTouchPad->RightX.setValue(0.0f, false);
										pSplitTouchPad->RightY.setValue(0.0f, false);
									}
									m_fRightTouchPositionX = m_fOriginRightTouchPositionX;
									m_fRightTouchPositionY = m_fOriginRightTouchPositionY;
								}

							// Did we move?
							} else if (nAndroidAction == AMOTION_EVENT_ACTION_MOVE) {
								// Use the pointer ID to figure out whether this is our left or right pointer
								if (m_nLeftTouchPointerID == nAndroidPointerID) {
									m_fLeftTouchPositionX = fPointerPositionX;
									m_fLeftTouchPositionY = fPointerPositionY;

								} else if (m_nRightTouchPointerID == nAndroidPointerID) {
									m_fRightTouchPositionX = fPointerPositionX;
									m_fRightTouchPositionY = fPointerPositionY;
								}
							}
						}
					}
				}
			}


		//[-------------------------------------------------------]
		//[ Public virtual UpdateDevice functions                 ]
		//[-------------------------------------------------------]
		public:
			virtual void Update() override
			{
				// Check if input device is valid
				if (m_pDevice) {
					// Get split touch pad device
					SplitTouchPad *pSplitTouchPad = static_cast<SplitTouchPad*>(m_pDevice);

					// Maximum allowed delta
					static constexpr float MaxDelta = 160.0f;
					static constexpr float MinDelta = 5.0f;

					// Update left axes?
					if (m_nLeftTouchPointerID != -1) {
						// Get the left touch movement and clamp it to the maximum allowed delta
						float fLeftDeltaX = m_fLeftTouchPositionX - m_fOriginLeftTouchPositionX;
						float fLeftDeltaY = m_fLeftTouchPositionY - m_fOriginLeftTouchPositionY;
						if (fLeftDeltaX > MaxDelta)
							fLeftDeltaX = MaxDelta;
						if (fLeftDeltaX < -MaxDelta)
							fLeftDeltaX = -MaxDelta;
						if (fLeftDeltaY > MaxDelta)
							fLeftDeltaY = MaxDelta;
						if (fLeftDeltaY < -MaxDelta)
							fLeftDeltaY = -MaxDelta;

						// Give our fat finger some space to sit down :D
						if (fabs(fLeftDeltaX) < MinDelta)
							fLeftDeltaX = 0.0f;
						else
							fLeftDeltaX -= MinDelta * sign(fLeftDeltaX);
						if (fabs(fLeftDeltaY) < MinDelta)
							fLeftDeltaY = 0.0f;
						else
							fLeftDeltaY -= MinDelta * sign(fLeftDeltaY);

						// Now update left axes
						if (pSplitTouchPad->LeftX.getValue() != fLeftDeltaX)
							pSplitTouchPad->LeftX.setValue(fLeftDeltaX, false);
						if (pSplitTouchPad->LeftY.getValue() != fLeftDeltaY)
							pSplitTouchPad->LeftY.setValue(fLeftDeltaY, false);
					}

					// Update right axes?
					if (m_nRightTouchPointerID != -1) {
						// Get the right touch movement and clamp it to the maximum allowed delta
						float fRightDeltaX = m_fRightTouchPositionX - m_fOriginRightTouchPositionX;
						float fRightDeltaY = m_fRightTouchPositionY - m_fOriginRightTouchPositionY;
						if (fRightDeltaX > MaxDelta)
							fRightDeltaX = MaxDelta;
						if (fRightDeltaX < -MaxDelta)
							fRightDeltaX = -MaxDelta;
						if (fRightDeltaY > MaxDelta)
							fRightDeltaY = MaxDelta;
						if (fRightDeltaY < -MaxDelta)
							fRightDeltaY = -MaxDelta;

						// Give our fat finger some space to sit down :D
						if (fabs(fRightDeltaX) < MinDelta)
							fRightDeltaX = 0.0f;
						else
							fRightDeltaX -= MinDelta * sign(fRightDeltaX);
						if (fabs(fRightDeltaY) < MinDelta)
							fRightDeltaY = 0.0f;
						else
							fRightDeltaY -= MinDelta * sign(fRightDeltaY);

						// Now update right axes
						if (pSplitTouchPad->RightX.getValue() != fRightDeltaX)
							pSplitTouchPad->RightX.setValue(fRightDeltaX, false);
						if (pSplitTouchPad->RightY.getValue() != fRightDeltaY)
							pSplitTouchPad->RightY.setValue(fRightDeltaY, false);
					}
				}
			}


		//[-------------------------------------------------------]
		//[ Private data                                          ]
		//[-------------------------------------------------------]
		private:
			// Left
			int   m_nLeftTouchPointerID;		///< Left screen touch pointer ID, -1 if none
			float m_fOriginLeftTouchPositionX;	///< Origin left touch x position
			float m_fOriginLeftTouchPositionY;	///< Origin left touch y position
			float m_fLeftTouchPositionX;		///< Current left touch x position
			float m_fLeftTouchPositionY;		///< Current left touch y position
			// Right
			int   m_nRightTouchPointerID;		///< Right screen touch pointer ID, -1 if none
			float m_fOriginRightTouchPositionX;	///< Origin right touch x position
			float m_fOriginRightTouchPositionY;	///< Origin right touch y position
			float m_fRightTouchPositionX;		///< Current right touch x position
			float m_fRightTouchPositionY;		///< Current right touch y position


		};

		/**
		*  @brief
		*    Sensor manager implementation for Android
		*/
		class AndroidSensorManagerDevice : public UpdateDevice
		{


		//[-------------------------------------------------------]
		//[ Public functions                                      ]
		//[-------------------------------------------------------]
		public:
			/**
			*  @brief
			*    Default constructor
			*/
			AndroidSensorManagerDevice() :
				m_pSensorManager(nullptr),
				m_pSensorEventQueue(nullptr),
				m_pAccelerometerSensor(nullptr),
				m_pMagneticFieldSensor(nullptr),
				m_pGyroscopeSensor(nullptr),
				m_pLightSensor(nullptr),
				m_pProximitySensor(nullptr)
			{
				// Destroy device implementation automatically
				m_bDelete = true;

				// TODO(co) Sensors are working, but it's no good idea to enable all available sensors by default with a 60 events per second rate for all sensors
				// -> The device stops responding, so, rethink this very first experiment... maybe one device per sensor which is deactivated by default and has the
				//    option to be enabled/disabled + event rate configuration?

				// Get the Android sensor manager instance
				m_pSensorManager = ASensorManager_getInstance();
				if (m_pSensorManager) {
					// Create sensor event queue instance
					m_pSensorEventQueue = ASensorManager_createEventQueue(m_pSensorManager, ALooper_forThread(), LOOPER_ID_USER, nullptr, nullptr);
			/*
					// Get accelerometer sensor instance
					m_pAccelerometerSensor = ASensorManager_getDefaultSensor(m_pSensorManager, ASENSOR_TYPE_ACCELEROMETER);
					if (m_pAccelerometerSensor) {
						// Write some debug information into the log
						PL_LOG(Debug, String("Android sensor manager accelerometer sensor instance found: Name=\"") + ASensor_getName(m_pAccelerometerSensor) +
																									  "\" Vendor=\"" + ASensor_getVendor(m_pAccelerometerSensor) +
																									  "\" Resolution=\"" + ASensor_getResolution(m_pAccelerometerSensor) +
																									  "\" Minimum delay=\"" + ASensor_getMinDelay(m_pAccelerometerSensor) + '\"')

						// Start monitoring the accelerometer
						if (ASensorEventQueue_enableSensor(m_pSensorEventQueue, m_pAccelerometerSensor) >= 0) {
							// We'd like to get 60 events per second (in us)
							ASensorEventQueue_setEventRate(m_pSensorEventQueue, m_pAccelerometerSensor, (1000L/60)*1000);
						}
					}

					// Get magnetic field sensor instance
					m_pMagneticFieldSensor = ASensorManager_getDefaultSensor(m_pSensorManager, ASENSOR_TYPE_MAGNETIC_FIELD);
					if (m_pMagneticFieldSensor) {
						// Write some debug information into the log
						PL_LOG(Debug, String("Android sensor manager magnetic field sensor instance found: Name=\"") + ASensor_getName(m_pMagneticFieldSensor) +
																									   "\" Vendor=\"" + ASensor_getVendor(m_pMagneticFieldSensor) +
																									   "\" Resolution=\"" + ASensor_getResolution(m_pMagneticFieldSensor) +
																									   "\" Minimum delay=\"" + ASensor_getMinDelay(m_pAccelerometerSensor) + '\"')

						// Start monitoring the accelerometer
						if (ASensorEventQueue_enableSensor(m_pSensorEventQueue, m_pMagneticFieldSensor) >= 0) {
							// We'd like to get 60 events per second (in us)
							ASensorEventQueue_setEventRate(m_pSensorEventQueue, m_pMagneticFieldSensor, (1000L/60)*1000);
						}
					}

					// Get gyroscope sensor instance
					m_pGyroscopeSensor = ASensorManager_getDefaultSensor(m_pSensorManager, ASENSOR_TYPE_GYROSCOPE);
					if (m_pGyroscopeSensor) {
						// Write some debug information into the log
						PL_LOG(Debug, String("Android sensor manager gyroscope sensor instance found: Name=\"") + ASensor_getName(m_pGyroscopeSensor) +
																								  "\" Vendor=\"" + ASensor_getVendor(m_pGyroscopeSensor) +
																								  "\" Resolution=\"" + ASensor_getResolution(m_pGyroscopeSensor) +
																								  "\" Minimum delay=\"" + ASensor_getMinDelay(m_pGyroscopeSensor) + '\"')

						// Start monitoring the accelerometer
						if (ASensorEventQueue_enableSensor(m_pSensorEventQueue, m_pGyroscopeSensor) >= 0) {
							// We'd like to get 60 events per second (in us)
							ASensorEventQueue_setEventRate(m_pSensorEventQueue, m_pGyroscopeSensor, (1000L/60)*1000);
						}
					}

					// Get light sensor instance
					m_pLightSensor = ASensorManager_getDefaultSensor(m_pSensorManager, ASENSOR_TYPE_LIGHT);
					if (m_pLightSensor) {
						// Write some debug information into the log
						PL_LOG(Debug, String("Android sensor manager light sensor instance found: Name=\"") + ASensor_getName(m_pLightSensor) +
																							  "\" Vendor=\"" + ASensor_getVendor(m_pLightSensor) +
																							  "\" Resolution=\"" + ASensor_getResolution(m_pLightSensor) +
																							  "\" Minimum delay=\"" + ASensor_getMinDelay(m_pLightSensor) + '\"')

						// Start monitoring the accelerometer
						if (ASensorEventQueue_enableSensor(m_pSensorEventQueue, m_pLightSensor) >= 0) {
							// We'd like to get 60 events per second (in us)
							ASensorEventQueue_setEventRate(m_pSensorEventQueue, m_pLightSensor, (1000L/60)*1000);
						}
					}

					// Get proximity sensor instance
					m_pProximitySensor = ASensorManager_getDefaultSensor(m_pSensorManager, ASENSOR_TYPE_PROXIMITY);
					if (m_pProximitySensor) {
						// Write some debug information into the log
						PL_LOG(Debug, String("Android sensor manager proximity sensor instance found: Name=\"") + ASensor_getName(m_pProximitySensor) +
																								  "\" Vendor=\"" + ASensor_getVendor(m_pProximitySensor) +
																								  "\" Resolution=\"" + ASensor_getResolution(m_pProximitySensor) +
																								  "\" Minimum delay=\"" + ASensor_getMinDelay(m_pProximitySensor) + '\"')

						// Start monitoring the accelerometer
						if (ASensorEventQueue_enableSensor(m_pSensorEventQueue, m_pProximitySensor) >= 0) {
							// We'd like to get 60 events per second (in us)
							ASensorEventQueue_setEventRate(m_pSensorEventQueue, m_pProximitySensor, (1000L/60)*1000);
						}
					}
					*/
				}
			}

			/**
			*  @brief
			*    Destructor
			*/
			inline virtual ~AndroidSensorManagerDevice() override
			{
				// Destroy sensor event queue instance
				ASensorManager_destroyEventQueue(m_pSensorManager, m_pSensorEventQueue);
			}


		//[-------------------------------------------------------]
		//[ Public virtual UpdateDevice functions                 ]
		//[-------------------------------------------------------]
		public:
			virtual void Update() override
			{
				// Check if input device and event queue are valid
				if (m_pDevice && m_pSensorEventQueue) {
					// Get sensor manager device
					SensorManager *pSensorManager = static_cast<SensorManager*>(m_pDevice);

					// Process sensor event queue instance
					ASensorEvent sASensorEvent;
					while (ASensorEventQueue_getEvents(m_pSensorEventQueue, &sASensorEvent, 1) > 0) {
						// Check the sensor type
						switch (sASensorEvent.type) {
							// Accelerometer
							case ASENSOR_TYPE_ACCELEROMETER:
								if (pSensorManager->AccelerationX.getValue() != sASensorEvent.acceleration.x)
									pSensorManager->AccelerationX.setValue(sASensorEvent.acceleration.x, true);
								if (pSensorManager->AccelerationY.getValue() != sASensorEvent.acceleration.y)
									pSensorManager->AccelerationY.setValue(sASensorEvent.acceleration.y, true);
								if (pSensorManager->AccelerationZ.getValue() != sASensorEvent.acceleration.z)
									pSensorManager->AccelerationZ.setValue(sASensorEvent.acceleration.z, true);
								break;

							// Magnetic field
							case ASENSOR_TYPE_MAGNETIC_FIELD:
								if (pSensorManager->MagneticX.getValue() != sASensorEvent.magnetic.x)
									pSensorManager->MagneticX.setValue(sASensorEvent.magnetic.x, true);
								if (pSensorManager->MagneticY.getValue() != sASensorEvent.magnetic.y)
									pSensorManager->MagneticY.setValue(sASensorEvent.magnetic.y, true);
								if (pSensorManager->MagneticZ.getValue() != sASensorEvent.magnetic.z)
									pSensorManager->MagneticZ.setValue(sASensorEvent.magnetic.z, true);
								break;

							// Gyroscope
							case ASENSOR_TYPE_GYROSCOPE:
								if (pSensorManager->RotationX.getValue() != sASensorEvent.vector.x)
									pSensorManager->RotationX.setValue(sASensorEvent.vector.x, true);
								if (pSensorManager->RotationY.getValue() != sASensorEvent.vector.y)
									pSensorManager->RotationY.setValue(sASensorEvent.vector.y, true);
								if (pSensorManager->RotationZ.getValue() != sASensorEvent.vector.z)
									pSensorManager->RotationZ.setValue(sASensorEvent.vector.z, true);
								break;

							// Light
							case ASENSOR_TYPE_LIGHT:
								if (pSensorManager->Light.getValue() != sASensorEvent.light)
									pSensorManager->Light.setValue(sASensorEvent.light, true);
								break;

							// Proximity
							case ASENSOR_TYPE_PROXIMITY:
								if (pSensorManager->Proximity.getValue() != sASensorEvent.distance)
									pSensorManager->Proximity.setValue(sASensorEvent.distance, true);
								break;
						}
					}
				}
			}


		//[-------------------------------------------------------]
		//[ Private data                                          ]
		//[-------------------------------------------------------]
		private:
			ASensorManager    *m_pSensorManager;		///< Android sensor manager, can be a null pointer
			ASensorEventQueue *m_pSensorEventQueue;		///< Sensor event queue, can be a null pointer
			const ASensor     *m_pAccelerometerSensor;	///< Accelerometer sensor, can be a null pointer
			const ASensor     *m_pMagneticFieldSensor;	///< Magnetic field sensor, can be a null pointer
			const ASensor     *m_pGyroscopeSensor;		///< Gyroscope sensor, can be a null pointer
			const ASensor     *m_pLightSensor;			///< Light sensor, can be a null pointer
			const ASensor     *m_pProximitySensor;		///< Proximity sensor, can be a null pointer


		};

		/**
		*  @brief
		*    Standard input provider for Android
		*/
		class AndroidProvider : public Provider
		{


		//[-------------------------------------------------------]
		//[ Public functions                                      ]
		//[-------------------------------------------------------]
		public:
			AndroidProvider() = delete;

			/**
			*  @brief
			*    Constructor
			*
			*  @param[in] inputManager
			*    Owner input manager
			*/
			inline explicit AndroidProvider(InputManager& inputManager) :
				Provider(inputManager),
				m_pAndroidKeyboardDevice(nullptr),
				m_pAndroidSplitTouchPadDevice(nullptr),
				m_pAndroidMouseDevice(nullptr)
			{
				// Connect the Android input event handler
				// "TODO(co) See https://github.com/PixelLightFoundation/pixellight - "AndroidProvider::OnInputEvent()" must be called
				// PLCore::SystemAndroid::EventInputEvent.Connect(SlotInputEvent);
			}

			/**
			*  @brief
			*    Destructor
			*/
			inline virtual ~AndroidProvider() override
			{
			}

			AndroidProvider& operator= (const AndroidProvider& source) = delete;


		//[-------------------------------------------------------]
		//[ Private virtual Provider functions                    ]
		//[-------------------------------------------------------]
		private:
			virtual void QueryDevices() override
			{
				// Create a keyboard device
				if (!CheckDevice("Keyboard")) {
					// Add device
					m_pAndroidKeyboardDevice = new AndroidKeyboardDevice();
					AddDevice("Keyboard", new Keyboard("Keyboard", m_pAndroidKeyboardDevice));
				}

				// Create a splitscreen touch pad device
				if (!CheckDevice("SplitTouchPad")) {
					// Add device
					m_pAndroidSplitTouchPadDevice = new AndroidSplitTouchPadDevice();
					AddDevice("SplitTouchPad", new SplitTouchPad("SplitTouchPad", m_pAndroidSplitTouchPadDevice));
				}

				// Create a mouse device
				if (!CheckDevice("Mouse")) {
					// Add device
					m_pAndroidMouseDevice = new AndroidMouseDevice(m_pAndroidSplitTouchPadDevice);
					AddDevice("Mouse", new Mouse("Mouse", m_pAndroidMouseDevice));
				}

				// Create a sensor manager device
				if (!CheckDevice("SensorManager")) {
					AndroidSensorManagerDevice *pImpl = new AndroidSensorManagerDevice();
					AddDevice("SensorManager", new SensorManager("SensorManager", pImpl));
				}
			}


		//[-------------------------------------------------------]
		//[ Private functions                                     ]
		//[-------------------------------------------------------]
		private:
			/**
			*  @brief
			*    Called to process the next Android input event
			*
			*  @param[in] cAInputEvent
			*    Android input event to process
			*/
			void OnInputEvent(const struct AInputEvent &cAInputEvent)
			{
				// Check the input event type
				switch (AInputEvent_getType(&cAInputEvent)) {
					// Key (e.g. from the soft keyboard)
					case AINPUT_EVENT_TYPE_KEY:
						if (m_pAndroidKeyboardDevice)
							m_pAndroidKeyboardDevice->OnKeyInputEvent(cAInputEvent);
						break;

					// Motion (e.g. from the touchscreen)
					case AINPUT_EVENT_TYPE_MOTION:
						if (m_pAndroidMouseDevice)
							m_pAndroidMouseDevice->OnMotionInputEvent(cAInputEvent);
						if (m_pAndroidSplitTouchPadDevice)
							m_pAndroidSplitTouchPadDevice->OnMotionInputEvent(cAInputEvent);
						break;
				}
			}


		//[-------------------------------------------------------]
		//[ Private data                                          ]
		//[-------------------------------------------------------]
		private:
			AndroidKeyboardDevice		*m_pAndroidKeyboardDevice;		///< Android keyboard device instance, can be a null pointer
			AndroidSplitTouchPadDevice	*m_pAndroidSplitTouchPadDevice;	///< Android splitted touchscreen mouse device instance, can be a null pointer
			AndroidMouseDevice			*m_pAndroidMouseDevice;			///< Android mouse device instance, can be a null pointer


		};


		//[-------------------------------------------------------]
		//[ HID constructor                                       ]
		//[-------------------------------------------------------]
		HID::HID() :
			m_pHIDImpl(nullptr)
		{
			// Detect devices
			DetectDevices();
		}


		//[-------------------------------------------------------]
		//[ Bluetooth constructor                                 ]
		//[-------------------------------------------------------]
		Bluetooth::Bluetooth() :
			m_pBTImpl(nullptr)
		{
			// Detect devices
			DetectDevices();
		}
	#else
		#error "Unsupported platform"
	#endif








	//[-------------------------------------------------------]
	//[ PLInput::InputManager methods                         ]
	//[-------------------------------------------------------]
	void InputManager::DetectProvider(const std::string &sProvider, bool bReset)
	{
		// Check if the provider is already present
		Provider *pProvider = GetProvider(sProvider);
		if (!pProvider) {
			// TODO(co) Add provider factory
			if ("PLInput::HIDProvider" == sProvider)
			{
				pProvider = new HIDProvider(*this);
			}
			else if ("PLInput::BluetoothProvider" == sProvider)
			{
				pProvider = new BluetoothProvider(*this);
			}
			#if defined(WIN32)
				else if ("PLInput::RawInputProvider" == sProvider)
				{
					pProvider = new RawInputProvider(*this);
				}
				else if ("PLInput::LegacyJoystickProvider" == sProvider)
				{
					pProvider = new LegacyJoystickProvider(*this);
				}
			#elif defined(LINUX)
				#error "TODO(co) Implement me"
			#elif defined(APPLE)
				#error "TODO(co) Implement me"
			#elif defined(ANDROID)
				#error "TODO(co) Implement me"
			#elif
				#error "Unsupported platform"
			#endif

			// Add provider
			if (nullptr != pProvider)
			{
				m_lstProviders.push_back(pProvider);
				m_mapProviders.emplace(sProvider, pProvider);
			}
		}

		// Detect devices
		if (pProvider)
			pProvider->DetectDevices(bReset);
	}


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
} // PLInput
