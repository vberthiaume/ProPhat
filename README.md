# Pro**Phat**

[![](https://github.com/vberthiaume/ProPhat/actions/workflows/build_and_test.yml/badge.svg)](https://github.com/vberthiaume/ProPhat/actions)

A **phat** virtual synthesizer inspired by the Prophet REV2!

![image](https://github.com/vberthiaume/ProPhat/assets/3721265/09299357-186f-4edf-92af-c5df1645bcc9)

## Configure and build using Visual Studio or Xcode:
- make sure you got all the submodules: `git submodule update --init --recursive`
- configure cmake: `cmake -S . -B Builds`
- then to build
    - with MSVC/Visual Studio on Windows: `cmake -B Builds`
    - with Xcode on Mac: `cmake -B Builds -G Xcode`
- the build artefacts should be in `/Builds/ProPhat_artefacts/` 🚀
## Configure and build with Visual Studio Code on Mac:
- make sure you got all the submodules: `git submodule update --init --recursive`
- open the folder in Visual Studio Code: `code .`
- make sure your CMake extension is configured to use the `Builds` folder (`cmd/ctrl + ,`, then set `Cmake: Build Directory` to `${workspaceFolder}/Builds`)
- cmd/ctrl + shift + P, then `CMake: Configure`
- F7 to build
- F5 to run
## Configure and build with Visual Studio Code on Ubuntu 24.04:
- same as for mac right above, but make sure to install all dependencies like this: `sudo apt-get update && sudo apt install libasound2-dev libx11-dev libxinerama-dev libxext-dev libfreetype6-dev libwebkit2gtk-4.1-dev libglu1-mesa-dev xvfb ninja-build ladspa-sdk libcurl4-openssl-dev libxcomposite-dev libxcursor-dev libxrandr-dev mesa-common-dev libjack-dev sccache`


