![](.github/vera.jpg)

# VERA 

VERA is a C++/OpenGL framework for making visual creative apps for a big spectrum of surfaces:

- native Windows, MacOS, Linux app (through GLFW)
- RaspberryPi stand alone app (no required X11)
- any browser as WebGL/WASM (Emscripten WebAssembly)

VERA is the resultant of the merge of [ADA](https://github.com/patriciogonzalezvivo/ada) (a minimal library to create OpenGL ES apps) and [HILMA](https://github.com/patriciogonzalezvivo/hilma) (a 2D/3D geometry library) 

## 1. Install Dependencies

VERA is a C++ library that require installing the following dependencies depending the OS/Platform 

### Windows 

* [Visual Studio 2019 or higher](https://visualstudio.microsoft.com/vs/). Make sure to check "Desktop development with C++" and "Universal Windows Platform development" are installed
* A [git](https://gitforwindows.org/) client 
* CMake (through [Scoop](https://scoop.sh/) is recommended)


### MacOS

```bash
brew install glfw3 pkg-config
```

For video support (using FFMpeg library LIBAV), also do:

```bash
brew install ffmpeg --build-from-source
```

### Linux: Debian based distributions with X11 Window Managers (Ex: Ubuntu, Raspberry Pi OS, etc) 

```bash
sudo apt install git cmake xorg-dev libglu1-mesa-dev
```

For video support (using FFMpeg library LIBAV), also do:

```bash
sudo apt install ffmpeg libavcodec-dev libavcodec-extra libavfilter-dev libavfilter-extra libavdevice-dev libavformat-dev libavutil-dev libswscale-dev libv4l-dev libjpeg-dev libpng-dev libtiff-dev
```

### Linux: Debian based distributions with no X11 Window manager (Raspberry Pi OS)

```bash
sudo apt install git cmake libgbm-dev libdrm-dev libegl1-mesa-dev libgles2-mesa-dev
```

### Linux: Fedora distribution

```bash
sudo dnf install git gcc-c++ cmake mesa-libGLU-devel glfw-devel libXi-devel libXxf86vm-devel 
sudo yum install libXdamage-devel 

```

For video support (using FFMpeg library LIBAV), also do:
```bash
sudo dnf install ffmpeg ffmpeg-devel
```

### Linux: Arch distribution

```bash
sudo pacman -S glu glfw-x11
```

For video support (using FFMpeg library LIBAV), also do:

```bash
sudo pacman -S ffmpeg
```

### Emscripten WebAssembly project

Follow [emscripten installation instructions](https://emscripten.org/docs/getting_started/downloads.html#installation-instructions):

```bash
git clone https://github.com/emscripten-core/emsdk.git
cd emsdk
git pull
./emsdk install latest
./emsdk activate latest
source ./emsdk_env.sh
cd ..
```

## 2. Compiling a project made with VERA

VERA is a C++ library, so in order to try it you will need to do it through a project that use it. In this case we will download and compile [this HELLO WORLD project](https://github.com/patriciogonzalezvivo/vera_hello_world)

### For windows managers like MacOS, Windows or Linux (with X11 Window Manager) (all through GLFW) 

```bash
git clone --recursive https://github.com/patriciogonzalezvivo/vera_hello_world.git
cd vera_hello_world
mkdir build
cd build
cmake ..
make
./hello_3d_world
```

### For Linux with no X11 Window Manager

```bash
git clone --recursive https://github.com/patriciogonzalezvivo/vera_hello_world.git
cd vera_hello_world
mkdir build
cd build
cmake -DNO_X11=TRUE ..
make
./hello_3d_world
```

### As a Emscripten WebAssembly project

```bash
git clone --recursive https://github.com/patriciogonzalezvivo/vera_hello_world.git
cd vera_hello_world
cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE=$EMSDK/upstream/emscripten/cmake/Modules/Platform/Emscripten.cmake
make

python3 -m http.server 
```
Then open http://localhost:8000/
