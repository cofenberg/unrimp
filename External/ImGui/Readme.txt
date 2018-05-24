ImGui 1.61 WIP (latest commit 037d5a7e98537b9eb14ca1bbd8a8f8ddd2c9dd62 - April 20, 2018)
- License: "MIT License"
- Online: https://github.com/ocornut/imgui
- Directly compiled and linked inside renderer runtime, hence "src" sub-directory

Changes made:
- "imgui.cpp" -> "SettingsHandlerWindow_WriteAll()" -> TODO(co) Changed this from integer to float since "SettingsHandlerWindow_ReadLine()" is reading floats
