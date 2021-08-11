ImGui 1.84 WIP (latest commit c7529c8ea8ef36e344d00cb38e1493b465ce6090 - August 10, 2021)
- License: "MIT License"
- Online: https://github.com/ocornut/imgui
- Directly compiled and linked inside renderer runtime
- Modified "imgui_internal.h" to be able to overwrite the file system functions "ImFileOpen()", "ImFileClose()", "ImFileGetSize()", "ImFileRead()" and "ImFileWrite()" when using "IMGUI_DISABLE_DEFAULT_FILE_FUNCTIONS"
