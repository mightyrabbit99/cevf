# C Event Framework (cevf)

Library for Event-driven programming in C

## Build and Install for Testing
create CMake build directory
```sh
cmake -DCMAKE_BUILD_TYPE=Debug -DBUILD_EXAMPLES=ON -B build
```
compile source code
```sh
cmake --build build
```

install to ./_install
```sh
cmake --install build --prefix _install
```

run the hello1 example
```sh
_install/bin/hello1 -s a
```
