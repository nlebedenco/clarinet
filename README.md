# Clarinet

## Platforms

  - Windows >= 7 (tested on Windows 10.19042.1165)
  - Linux kernel >= 2.6. (tested Ubuntu 20.04.3)
  - macOS >= 10.6 (tested OSX 10.15 Catalina)
  
Other relatively UN\*X platforms such as Solarias and FreeBSD should be supported but are not tested.

  - Android: ?
  - iOS: ?
  - tvOS: ?
  - watchOS: ?
  - Orbis (PS4): ?
  - Durango (XboxOne): ?

## Dependencies

There are no imposed 3rparty runtime dependencies. The build option CLARINET_USE_STATIC_RT can be used to force link 
to the static system runtime lib in case even the dependency on a shared libc is not desired. 

### Windows

Winsock 2 is required and can only be dynamically loaded, there is no static option, much like user32.dll and 
kernel32.dll. 

### UN\*X

Some unix systems other than Linux may require specific shared network libraries such as libsocket (which requires 
libnsl), libnetwork and/or libxnet.

## Build

### Requirements

  - CMake >- 3.18
  - A C compiler that supports C99
  - A CXX compiler that supports C++11 (only to build tests)
  - Git >= 2.18
  - (optional) Visual Studio >= 2015 (for MSVC >= 1900 and test explorer)

As far as compilers go any of MSVC >= 1900, GCC >= 4.6 or CLANG >= 3.0 should do. 

[Git](https://git-scm.com/docs/git) is required to automatically download submodules and the test framework.

[Catch2](https://github.com/catchorg/Catch2) is automatically fetched from git and only used to build unit tests.

All submodules are configured to produce static libraries linked into the main target. Not only it simplifies 
installation, it also contributes to keep functionality consistent regardless of the state of the system we deploy 
into.

[MbedTLS](https://github.com/ARMmbed/mbedtls) requires Python3 to generate test code and sample programs in the 
development branch but not having python should be ok since we only build the target libraries.


### CMake

#### UN\*X

```
mkdir /path/to/build_dir 
cd /path/to/build_dir
cmake /path/to/clarinet
cmake --build .
```

Unlike on Windows, the loader on Unix systems never checks the current directory for shared objects. This is 
inconvenient because projects with executables linked against a shared library produced in a sub-project cannot be 
immediately executed after installation if the install prefix was customized. On the other hand, one generally does not 
want to pollute system folders with development libraries. A possible compromise in this case is to temporarily set 
$LD_LIBRARY_PATH to point to the devel libdir used by cmake to install the libraries. See the 
[ld.so(8)](https://man7.org/linux/man-pages/man8/ld.so.8.html) man page for more details. 

#### Windows

On windows you might have to open a Native Tools Command Prompt rather than a normal command prompt if cmake was 
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

Note that despite the native prompt having a `Platform` environment it cannot be reliably used. On the x64 native 
prompt, `platform=x64` which matches the platform name used in VS project files but on the x86 native prompt we have 
`platform=x86` (at least for VS2017 and VS2019 installations) while VS project files expect Win32. In this case calling
`cmake -A %platform%` will fail with VS generators.

A Visual Studio solution is created in the buildir after the first configuration which can be used to build any of the 
supported types (Debug, Release, MinSizeRel or RelWithDebInfo)
It's not currently possible to have cmake generating a single Visual Studio solution for both x64 and Win32 because the 
build folders need to be separate.

#### Options

Build options can be passed to cmake using `-D<OPTION>=<VALUE>`.

All available options can be listed with the following command:

```
cd /path/to/build_dir
cmake -LA -B .
```

Or if the help text is desired:

```
cd /path/to/build_dir
cmake -LAH -B .
```

Note that standard CMake options (with the exception of `BUILD_SHARED_LIBS` and `BUILD_TESTING`) are prefixed with 
`CMAKE` while all Clarinet options are prefixed with `CLARINET`. Also note there are options that are dependent on 
other options. For example, `CLARINET_ENABLE_TLS` will be automatically set to OFF if `CLARINET_ENABLE_TCP` is OFF.

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

The variable `CMAKE_INSTALL_PREFIX` can be used to customize the install directory during configuration as follows:

```
cmake \path\to\clarinet -DCMAKE_INSTALL_PREFIX:PATH=\path\to\install_dir 
```

If this is the top project than `CMAKE_INSTALL_PREFIX` is already set to `"{CMAKE_BINARY_DIR}/install` which is a 
safe location for development and there is no need to explicitly pass `-DCMAKE_INSTALL_PREFIX:PATH=` unless you are 
really trying to install the files in the underlying the system. Overriding `CMAKE_INSTALL_PREFIX` like this is aligned 
with the [principle of least astonishment](https://en.wikipedia.org/wiki/Principle_of_least_astonishment). Default 
installation directories will normally require admin priviledges. On unix the default `CMAKE_INSTALL_PREFIX` is `/` or 
`/usr/` and on Windows it is normally `%ProgramFiles(x86)%\${PROJECT_NAME}\`. So an unsuspecting developer could either 
pollute its own system with one or more potentially broken development binaries or receive unexpected access denied 
error messages. A developer that is effectively trying to install the library on the system could still see binaries 
copied to a different (albeit safer) location than the one expected but mindful installers already know about 
`CMAKE_INSTALL_PREFIX` and those who don't can easily realize the situation by looking at the installation output.

Note that for DLL platforms (all Windows-based systems including Cygwin), the DLL is treated as RUNTIME and the import 
library is treated as an ARCHIVE target (despite it not being a static lib really).

The library (and all dependencies) can be built and instaled using:

```
cmake --build \path\to\build_dir --target install
```

#### Compiler Warnings

Additional compilation warnings are enabled if a file named `.devel` exists in either the project directory or the build 
directory.

## Tests

The `tests` folder contains unit tests defined in C++ using [catch2](https://github.com/catchorg/Catch2). Each test 
target represents a set of cases grouped under a single executable which can be declared by a cmake file fragment named 
`<TESTSETNAME>.cmake`. The minimum content of such a fragment should be:

```
target_test(<TESTSETNAME>)
target_sources(<TESTSETNAME> PRIVATE src/<TESTSETNAME>.cpp)
```

where `<TESTSETNAME>` must be replaced by the name of the actual test set. This way test cases can be added or removed 
without affecting the CMakeLists.txt and will not interfere with other tests. Note test sets should normally be agnostic 
to execution order but they are guaranteed to be included in `NATURAL` order so contiguous digits are compared as whole 
numbers. For example: the following list 10.0 1.1 2.1 8.0 2.0 3.1 will be sorted to 1.1 2.0 2.1 3.1 8.0 10.0 as opposed 
to the lexicographical order of 1.1 10.0 2.0 2.1 3.1 8.0. For more details see 
[list(SORT ...)](https://cmake.org/cmake/help/latest/command/list.html#sort).

After building, all tests can be launched as follows:

```
cd \path\to\build_dir
ctest . -C <CONFIG> --output-on-failure
```

where `<CONFIG>` is the build type used. It may be ommited when using single-config generators but is required by 
multi-config generators such as the Visual Studio generator on Windows. See the 
[ctest(1)](https://cmake.org/cmake/help/latest/manual/ctest.1.html) documentation page for more details. 
The `--output-on-failure` argument is optional and provides extended information about failures (useful for debugging). 

The variable `BUILD_TESTING` is used to control whether tests are configured. Default value is ON. 

The variable `CLARINET_BUILD_TESTING` is used to confirm whether tests should be configured when `BUILD_TESTING` is ON. 
It defaults to ON when this is the top project and OFF otherwise. An additional variable is required because top 
projects may configure their own unit tests using the variable `BUILD_TESTING` but usually they will not be interested 
in building unit tests of sub-projects (dependencies). If for any reason a top project do want to build and run our unit 
tests (e.g. top project is only an agregator) then it can force `CLARINET_BUILD_TESTING=ON`.

The auto generated Visual Studio project *RUN_TESTS* or the makefile target *test* will run all tests when built but 
they do not depend on the tests themselves therefore will not automatically rebuild out-of-date tests. Visual Studio 
2015, 2017 and 2019 require the [Test Adapter Catch2](https://github.com/JohnnyHendriks/TestAdapter_Catch2) 
extension for tests to be discovered in the test explorer window. Running a test out of the test explorer windoe will 
rebuild if the executable is out-of-date.

## TODO

- Export a conscise .editorconfig
- Add feature to enable/disable ipv6 scope id: CLARINET_ENABLE_IPV6_SCOPE_ID/CLARINET_FEATURE_IPV6_SCOPE_ID dependent on 
  HAVE_SOCKADDR_IN6_SCOPE_ID. Adjust functions and tests accordingly.
- Support IPv4 broadcast and IPv4/IPv6 multicast in the udp interface
- Test cmake generator using "-T ClangCL" on windows to ensure all conditions on MSVC are supported by the clang-cl too
- Add C# wrapper
- Add python wrapper
- Support code sanitizers
- Use travis for automatic build check?
- Run tests with Valgrind to detect memory leaks?
- Code coverage (https://gitlab.kitware.com/cmake/community/-/wikis/doc/ctest/Coverage)
  - For gcc (https://discourse.cmake.org/t/guideline-for-code-coverage/167) 
    - Build with flags `-fprofile-arcs` `-ftest-coverage`. 
    - Run `ctest` normally to run all the tests. 
    - Run `ctest -T Coverage` to collect coverage results. 
    - The two ctest calls can be combined as `ctest -T Test -T Coverage`.
  - For msvc (https://github.com/OpenCppCoverage/OpenCppCoverage/issues/85)
    - Use [OpenCppCoverage](https://github.com/OpenCppCoverage) (free/GPL-3.0)
    - Simply run ctest through OpenCppCoverage with --cover_children:
    - Example: `OpenCppCoverage.exe --quiet --export_type cobertura:cobertura.xml --cover_children -- ctest -C Debug`
    - Obviously you probably need `--sources` and also want to set `--modules` to the path containing your binaries 
    (e.g. build path) to avoid it tracing ctest itself.
