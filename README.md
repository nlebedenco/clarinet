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
which matches the platform name used in VS project files but on the x86 NTCP we have `platform=x86` (at least for 
VS2017 and VS2019 installations) while VS project files expect Win32. In this case calling `cmake -A %platform%` will 
fail with VS generators.

A Visual Studio solution is created in the buildir after the first configuration which can be used to build any of the 
supported types (Debug, Release, MinSizeRel or RelWithDebInfo)
It's not currently possible to have cmake generating a single Visual Studio solution for both x64 and Win32 because the 
build folders need to be separate.

#### Shared vs Static build

The variable `BUILD_SHARED_LIBS` is used to control whether cmake will produce a shared lib or a static lib. 
By default, cmake produces a static lib. For a shared lib (.so/.dll) use:

```
cmake \path\to\clarinet -DBUILD_SHARED_LIBS=ON
``` 
 
Note that `BUILD_SHARED_LIBS` is defined as an option so it does not affect submodules which will always be linked 
statically unless otherwise noted.

#### Debug vs Release

The variable `CMAKE_BUILD_TYPE` is used to control whether the project must be built for Debug, Release, MinSizeRel or 
RelWithDebInfo in *single-config generators*. Default build type is Debug. A build type can be explicitly set a follows: 

```
cmake \path\to\clarinet -DCMAKE_BUILD_TYPE=Release
```

On *multi-config generators* such as the Visual Studio generator on Windows there is no need to reconfigure the build.
A config argument can be used alongside the build argument as follows:

```
cmake --build \path\to\build_dir --config Release`
```

#### Install

The variable `CMAKE_INSTALL_PREFIX` can be used to customize the install directory as follows:

```
cmake \path\to\clarinet -DCMAKE_INSTALL_PREFIX:PATH=\path\to\install_dir 
```

Note that setting `CMAKE_INSTALL_PREFIX` is not strictly required but recommended to avoid polluting the system. 
Also note that the default installation directory on Windows is `%ProgramFiles(x86)%\clarinet` which normally requires 
administrative privileges.

For DLL platforms (all Windows-based systems including Cygwin), the DLL is treated as RUNTIME and the import library is 
treated as an ARCHIVE target although it's not a static lib really.

Then the library and its dependencies can be instaled using:

```
cmake --install \path\to\build_dir
```

which is equivalent to:

```
cmake --build \path\to\build_dir --target install
```

#### Compiler Warnings

Additional compilation warnings are enabled if a file named `.devel` exists in either the project directory or the build 
directory.


## TODO

- Use cmocka for tests (check https://github.com/OlivierLDff/cmocka-cmake-example)
- Use travis for automatic build check
- Optionally run tests under Valgrind to detect memory leaks
- Use Coverity for static source analysis