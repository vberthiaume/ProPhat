# Pro**Phat**

[![](https://github.com/vberthiaume/ProPhat/actions/workflows/build_and_test.yml/badge.svg)](https://github.com/vberthiaume/ProPhat/actions)

A **phat** virtual synthesizer inspired by the Prophet REV2!

![image](https://github.com/vberthiaume/ProPhat/assets/3721265/09299357-186f-4edf-92af-c5df1645bcc9)

To build:
- With Visual Studio on Windows: `cmake -B Builds`
- with Xcode on Mac: `cmake -B Builds -G Xcode`
- with Visual Studio Code on Mac:
    - open the folder in Visual Studio Code
    - make sure your CMake extension is configured to use the `Builds` folder (`cmd/ctrl + ,`, then set `Cmake: Build Directory` to `${workspaceFolder}/Builds`)
    - cmd/ctrl + shift + P, then `CMake: Configure`
    - F7 to build
    - F5 to run
