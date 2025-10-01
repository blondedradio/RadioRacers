<picture>
  <img src="./radio/branding.gif">
</picture>

<p></p>

<p align="right">RadioRacers <span style="font-size: 0.8rem !important">(this is for SEO)</span></p>

A [Dr. Robotnik's Ring Racers](https://www.kartkrew.org/) fork.
<br><small>Last updated for **v2.4 RC5**.</small>

This build &ndash; like _all_ software &ndash; is always a work in progress.</br>
Compatible with vanilla Ring Racers; all changes made so far are client-side.

> Many thanks to [GenericHeroGuy](https://github.com/GenericHeroGuy) for his work on [`pk3make.py`](https://github.com/GenericHeroGuy/ringracers-scripts), which is used to automate the building process for the assets.

## Features

Including, but not limited to:

<details>
<summary>Hudfeed</summary>
<img src="./radio/readme/hudfeed.gif">

A live feed that displays in-game events, including player interactions and race statistics, such as grades. 

Both its position within the HUD and content can be customized in the settings.
</details>

<details>
<summary>Emotes</summary>

<img src="./radio/readme/emotes.gif" width="500">

Support for chat emotes, both animated and static. A set of default emotes is included.

For details on customization, such as adding your own emotes and usability tips, check the [readme](./radio/emotes/README.md).
</details>

<details>
<summary>Peek</summary>

<img src="./radio/readme/peekaboo.gif" width="500">

In the Server Browser, you can "peek" into a server to view key details, such as the current level and connected players.
</details>

...and [more](https://github.com/blondedradio/RadioRacers/pulls).

## Getting Started
1. Get the [**latest copy**](https://www.kartkrew.org/) of Dr. Robontik's Ring Racers installed on your system.
2. Download the latest assets (`radioracers_assets.zip`) for this build [here](https://github.com/blondedradio/RadioRacers/releases/latest-radio-assets/).
3. Extract the `radioracers_assets.zip` into the ***same directory*** where you installed Ring Racers.
     - If you've installed Ring Racers in `C:\Games\Ring Racers`, then **that's** the folder you want to extract the zip file in.
4. [Compile](#compiling) the build (`ringracers_radioracers.exe`) and copy it into the ***same directory*** where you installed Ring Racers.
5. Run `ringracers_radioracers.exe`.

### Compiling
If you don't know how to compile, either attempt it yourself (good practice) or ask someone you trust to do it for you.

If you want to try it yourself, follow the [instructions](#building-from-source) in the original README to compile the build.<br/>
If you're on Windows 10 (or above), try following [this](https://blondedradio.github.io/rr-compile-windows-guide/) guide.

However, if you grab a build from — say — a random Discord channel, *please* encourage whoever shared it to include [MD5 hashes](https://linuxsecurity.com/features/what-are-checksums-why-should-you-be-using-them) with the executable. It's spooky out here.


---

Original README below.

---
  
# Dr. Robotnik's Ring Racers

<p align="center">
  <a href="https://www.kartkrew.org">
    <img src="docs/logo.png" width="404" style="image-rendering:pixelated" alt="Dr. Robotnik's Ring Racers logo">
  </a>
</p>

Dr. Robotnik's Ring Racers is a kart racing video game originally based on the 3D Sonic the Hedgehog fangame [Sonic Robo Blast 2](https://srb2.org/), itself based on a modified version of [Doom Legacy](http://doomlegacy.sourceforge.net/).

Ring Racers' source code is available to users under the GNU General Public License version 2.0 or higher.

## Links

- [Kart Krew Dev Website](https://www.kartkrew.org/)
- [Kart Krew Dev Discord](https://www.kartkrew.org/discord)
- [SRB2 Forums](https://mb.srb2.org/)

## Disclaimer

Dr. Robotnik's Ring Racers is a work of fan art made available for free without intent to profit or harm the intellectual property rights of the original works it is based on. Kart Krew Dev is in no way affiliated with SEGA Corporation. We do not claim ownership of any of SEGA's intellectual property used in Dr. Robotnik's Ring Racers.

# Development

## Building from Source

Ring Racers is built using a compatible C++ toolchain (GCC, MinGW, Clang and Apple Clang as of this writing), CMake, and Microsoft vcpkg. The compiler and runtime libraries must support the ISO C++17 standard and ISO C11 standard.

On Linux platforms, you will need the following libraries available on the system.

- libcurl
- zlib
- libpng
- libogg
- libvorbis
- libvpx
- libyuv
- SDL2
- libopus

On Windows and macOS, you will need to install [vcpkg] instead to build these dependencies alongside the game.

[vcpkg]: https://vcpkg.io/en/

To configure and build the game, there are [CMake presets] (declared in `CMakePresets.json`). These presets require the ninja build script tool in addition to cmake and your C++ toolchain. Here is a non-exhaustive list of them:

- ninja-debug: non-optimized, assertions enabled
- ninja-develop: optimized, assertions enabled
- ninja-release: optimized
- ninja-x86_mingw_static_vcpkg-debug
- ninja-x86_mingw_static_vcpkg-develop
- ninja-x86_mingw_static_vcpkg-release
- ninja-x64_osx_vcpkg-debug
- ninja-x64_osx_vcpkg-develop
- ninja-x64_osx_vcpkg-release
- ninja-arm64_osx_vcpkg-debug
- ninja-arm64_osx_vcpkg-develop
- ninja-arm64_osx_vcpkg-release

[CMake presets]: https://cmake.org/cmake/help/latest/manual/cmake-presets.7.html

These presets depend on the `VCPKG_ROOT` environment variable being specified before the first run of the `cmake` command. Their build directories are pre-configured as subdirectories of `build/`.

After all prerequisites are set-up, configure and build using the following commands, adjusting according to your target system:

    cmake --preset ninja-x86_mingw_static_vcpkg-develop
    cmake --build --preset ninja-x86_mingw_static_vcpkg-develop

## Contributing

We welcome external contributions from the community. If you are planning on making a large feature you intend to contribute to the project, please consider reaching out to us in the Kart Krew Dev public Discord server so we can coordinate with you.

Our primary source repository is [hosted on gitlab.com](https://gitlab.com/kart-krew-dev/ring-racers). The Github repository is a mirror of this. If you submit a Pull Request to the Github repository, please keep in mind that we do not consistently monitor that mirror and may not see your request.

All contributions must be made available under the GPL General Public License version 2.0, or public domain. Integrations for third party code must be made to code which is compatibly licensed.

