# ImPlus

Extensions and add-ons for Dear ImGui

## Dependencies

- fontconfig
- freetype

On MacOS:

```bash
brew install freetype fontconfig
```

On Linux:

```bash
sudo apt install libfontconfig-dev libfreetype-dev libpulse-dev libsndio-dev libdrm-dev libgbm-dev libasound2-dev libx11-dev libxrandr-dev libxinerama-dev libxcursor-dev libxi-dev libgl-dev libxkbcommon-dev libusb-1.0-0-dev
```


## Features

- Font support
  - Loading default fonts provided by host desktop environment
  - Overlays (fallbacks) for additional languages and font icons
  - Runtime UI zoom in/out with automatic font map updates

- Extended host backend support
  - GLFW hosts
  - SDL2 hosts
  - SDL3 hosts
  - Native Win32 hosts with DX11
  - Manage location and state of your host window

- Keyboard accelerators/shortcuts

- Extended main menus
  - Front and trail item placement
  - Overflow submenu for items that don't fit
  - Simple to use

- Toolbars
  - Buttons, dropdowns, separators, text labels
  - Overflow submenu for items that don't fit
  - Front, trail and overflow item placement
  - Item layout, icon/text visibility, description tooltips

- Buttons and button bars
- Tooltips
- Splitters
- List views
- Column views
- Property list views
- Banners
- Dialog boxes
