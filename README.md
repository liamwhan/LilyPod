![LilyPod Logo](misc/icon_512x512.png)

# LilyPod - Simple audio file tool
This is a lightweight utility that makes it easy for non-technological folk to add an intro and outro sound to the beginning and end of a 48kz 16-bit signed PCM Wave file.

## Build instructions
### All platforms
- Clone the project
- Pull `imgui` submodule:
  - `git submodule update --init`

Then follow the platform specific instructions below:

### Windows
#### Prerequisites
1. Visual Studio Community 2017 or above with the C++ development workload or The Visual Studio Build Tools 2019 package
- Install the Visual Studio C++ workload (so that the C/C++ compiler cl.exe is installed and vcvarsall.bat is available)
- Locate `vcvarsall.bat` on your system (I recommend using [Everything](https://www.voidtools.com/en-au/) to find files easily, it's amazing, super-fast and free)
     - Microsoft changes the location of vcvarsall.bat all the time but on my machine it's located at `C:\Program Files (x86)\Microsoft Visual Studio\2019\Professional\VC\Auxiliary\Build\vcvarsall.bat`
- Update build.bat line 4 with the full path to `vcvarsall.bat` (don't forget quotes and be careful not to delete the `x64` argument that is passed to it)
- Run `build.bat` for Debug build
- Run `build.bat Release` for Release build
- Profit

### MacOS
The MacOS build uses the GLFW (an OpenGL wrapper library)  / Metal backend for Dear Imgui so `libglfw` is a dependency, additionally due to subprocess execution restrictions and code-signing woes, we use `ffmpeg-kit` (via CocoaPods) for conversion tasks like converting MKVs to WAV files and changing the sample rate of non 48k WAV files etc.

#### Setup instructions
- Install Xcode
- Install homebrew
```bash
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
```
- Install libglfw 3.3.4
```bash
brew install glfw@3.3.4
```
- Install CocoaPods (use the default MacOS version of Ruby/Gem)
```bash
sudo gem install cocoapods
```
- Use Cocoapods to install project dependencies (`ffmpeg-kit`)
```bash
# From repo root
cd code/Xcode/LilyPod
pod install
```
- Open workspace in Xcode
```bash
#From repo root
open code/Xcode/LilyPod.xcworkspace
```

## Release instructions
### Windows
There is a Visual Studio project that creates an MSI installer for Windows Release builds.

1. Build the project in Release mode first - run `build.bat Release`
2. Open `wav_installer.sln` in Visual Studio
3. Select the `wav_installer` project in the VS Project Explorer and hit F4 to open the Properties window (don't right-click and select Properties as this opens up the property pages which is not what you want)
4. Update the version number and click Yes to let Visual Studio automatically change the Product Code<sup>Ɨ</sup>
5. Build the project in Release mode
6. Get the generated `LilyPod Audio Utility Windows.msi` installer from the `wav_installer/Release/` directory and distribute


> <sup>Ɨ</sup> Changing the product code allows the installer to update existing installations of the utility in Windows, if you don't do this the installer will fail on systems where the utility is already installed with an error saying "Uninstall the existing version first"

### MacOS
1. Open the Xcode project Build and then and Product > Archive.
2. In order for users to be able to install the output Application archive, the archive will need to be submitted and notarised by Apple which is described below but is a painful and involved process so expect some frustration getting it working.
3. Once the app is notarised you can zip and distribute the generated package (MacOS applications are folders so you'll probably have to zip it to send it anywhere and if you want to email it, it'll need to be a tar.gz or most Email services will refuse to attach it)

----
#### Notarisation and Code Signing
After archiving XCode will open the Archives window displaying all of the successful app archives on your local machine. 

You can then have XCode validate the Archive locally before submission to App Store Connect for notarisation. Some common code signing or entitlements issues will be identified in the validation, but not all of them.

Included Libraries need to be code signed with the same identitiy that the app is signed with. In our case, we are distributing the app directly to users, not through the App Store, so we want to use the Developer ID code signing identity (see [step 1 below](#1-get-the-available-code-signing-identities-on-your-machine))

----
#### 1. Get the available code signing identities on your machine:
```bash
$ security find-identity -v -p codesigning
```
The output will look something like this:
```
  1) MLGAV9UA20E6MAHS83RBOQ8CGNBAXU6KAOFGSPAC "Apple Development: JOHN Smith (XYZABC123)"
  2) C8M1IJKXAKE076NZ9J7XENL9PMDN2EYF9ODXOFND "Apple Development: JOHN Smith (XYZABC123)"
  3) Q4UDRR9S70QNB6POBJZC0QBPWCBZBJ5E2XPLQOMF "Apple Distribution: JOHN Smith (XYZABC123)"
  4) WCBIIMV6L0ECTZM7BMY6CAYQFHX3L5KKKIFEHMBG "Developer ID Application: JOHN Smith (XYZABC123)"
     4 valid identities found
```


You want the quoted human readable identifier eg. `"Developer ID Application: JOHN Smith (TEAM_HASH)"` as you'll be pasting this into `codesign`.

----
#### 2. Sign the glfw library
You only need to do this once, as `codesign` modifies the binary when it signs it, and if you accidentally sign it with the wrong identity you can just sign it again (that's what the `-f` flag does) with the correct identity.

The below example uses the Developer ID identity installed on my Macbook, which matches the one that Xcode will use when we Distribute the app.
```bash
cd /usr/local/Cellar/glfw/3.3.4/lib
codesign -f -s "Developer ID Application: JOHN SMITH (XYZABC123)" libglfw.3.3.dylib
```

The linker Search paths in the Xcode project are configured to look in the `/usr/local/Cellar/glfw/3.3.4/lib/` directory for the `libglfw` dynamic library and the linker is instructed to compile against library with *Other Linker Flags* entry `-lglfw`, now the we've signed the binary with our Developer ID signature, we're ready to submit the application to Apple for notarisation. 

----

#### 3. Submit the application package for notarisation
1. Xcode > Product > Archive
2. Select latest Archive in Archives window and Validate App
3. Once Validated click Distribute App
4. For distribution type select `Developer ID (distribute directly to customers)`
5. Accept all the prompts (accept automatic signing - it will resign the package)
6. Upload to Apple for notarisation
7. After upload completes leave the window open that says "Awaiting notarisation", it will update once the app has passed validation and been notarised (usually takes 5 to 10 mins)
8. Export the notarised app and send it to users.

----

## Development
### General
The podcast utility is designed using a C/C++ architecture sometimes called the [**Unity** or **unified** build](https://en.wikipedia.org/wiki/Unity_build).

In practice, this means that there is a thin OS/Platform layer (in this case `win32_wav.cpp` for Win32 or `main.cc` for MacOS) for each OS platform) and the non-OS-specific code is defined in a shared "Application" layer (in this case `wav.cpp` & `wav.h`) that essentially acts as a service to the OS/Platform layer.

More specifically there are a set of OS/Platform-specific functions like `PlatformReadEntireFile` that are forward declared as external in the Application layer which are actually defined in the OS/Platform specific source code files. 

### Dependencies/Libraries
The only 3rd-party dependency this utility has is [Dear ImGui](https://github.com/ocornut/imgui) which itself has no external dependencies.

Dear ImGui is a slim, [immediate-mode GUI](https://caseymuratori.com/blog_0001) library that is x-plat and renderer agnostic and allows for lightweight, performant GUIs where asthetics and presentation are lower priority that rapid development.


In addition to Dear ImGui, each of the platforms imports the OS specific headers required for that platform (i.e. `windows.h` on Win32) but no further dependencies are imported.

### Features 

- Splices arbitrary files together to simplify the process of tasks like adding intro/outro music to podcast audio files
- Supports both mono and stereo files (mono sounds are naively converted to stereo files at load-time)
- Simple automatic leading silence detection and removal


### Limitations
- The application currently on supports writing 16-bit signed PCM 48000hz WAV files, it will coerce other formats to the output format on read.

#### TODOs
- Normalization / DeEssing / Compression / EQ