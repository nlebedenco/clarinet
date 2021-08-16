# clarinet

## Build

### CMake

#### Un*x

```
mkdir /path/to/build_dir 
cd /path/to/build_dir
cmake /path/to/clarinet
cmake --build .
```

#### Windows

On windows you might have to open a Native Tools Command Prompt (NTCP) rather than a normal command prompt if cmake was 
installed using the Visual Studio Installer. Mind that cmake will not automatically pick the target platform matching 
the NTCP used so you still need to pass the target platform to cmake using `-A`. It also means you should be able to 
build x86 binaries from a x64 NTCP and vice-versa without problems.

For Windows x64:

```
mkdir \path\to\build_dir 
cd \path\to\build_dir
cmake \path\to\clarinet
cmake -A x64 \path\to\clarinet 
cmake --build .
```

For Windows x86:

```
mkdir \path\to\build_dir 
cd \path\to\build_dir
cmake \path\to\clarinet
cmake -A Win32 \path\to\clarinet 
cmake --build .
```

Note that despite the NTCP having a `Platform` environment it cannot be reliably used. On the x64 NTCP, `platform=x64` 
which matches the platform name used in VS project files but on the x86 NTCP `platform=x86` (at least for VS2017 and 
VS2019 installations) while the VS project files generated expect Win32 so calling `cmake -A %platform%` will normally 
fail on a x86 NTCP.

It's not currently possible to have cmake generating a single Visual Studio solution for both x64 and Win32 because the 
build folders need to be separate.

#### Shared vs Static Lib

The variable `BUILD_SHARED_LIBS` controls if cmake will produce a shared lib or a static lib. By default, CMake 
produces a static lib. For a a shared lib use:

`cmake \path\to\build\_dir -DBUILD_SHARED_LIBS=ON`
 
Note that `BUILD_SHARED_LIBS` is not transitive and does not affect dependencies which will always be linked statically 
linked unless otherwise noted.

