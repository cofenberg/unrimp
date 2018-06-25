AMD AGS v5.2.0 - May 23, 2018
	AMD GPU Services (AGS) library, used to add multi-indirect-draw support to the Direct3D 11 renderer backend for AMD GPUs
- License: "MIT"
- Online: https://github.com/GPUOpen-LibrariesAndSDKs/AGS_SDK
- Unlike NVIDIA with NvAPI, AMD doesn't ship "amd_ags_x64.dll" and "amd_ags_x86.dll" with its drivers so we need to manually provide them (I don't want to statically link those)
