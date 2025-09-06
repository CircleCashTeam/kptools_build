# kptools_build
Build kptools with cmake build system

# Build
## Build kptools
### Build for ubuntu
- On ubuntu setup
```shell
sudo apt install build-essential cmake ninja-build patch git
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
```
- If you want link static set `-DPREFER_STATIC_LINKING=ON`
- If you want link system zlib set `-DUSE_SYSTEM_ZLIB=ON`