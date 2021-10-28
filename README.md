# Clarinet

## Platforms

  - Windows >= 8 (tested on Windows 10.19042.1165)
  - Linux kernel >= 3.9 (tested Ubuntu 20.04.3)
  - macOS >= 10.12 (tested OSX 10.15 Catalina)
  - Android: ?
  - iOS: ?
  - tvOS: ?
  - watchOS: ?
  - Orbis (PS4): ?
  - Durango (XboxOne): ?
  
Other related platforms such as FreeBSD, AIX and Solaris should be compatible but are not actively supported.


## Dependencies

There are no dependencies on 3rparty dynamic libraries but system shared libraries may still be required. 

On Linux and BSD/Darwin librt and libresolv are required. On other Un\*x systems libsocket and libnsl may be required as 
well as libnetwork and/or libxnet.

On Windows, winsock 2.2 is required and there is no static variant. It can only be dynamically loaded much like other 
Windows system libraries such as user32.dll and kernel32.dll.

The build option CL_USE_STATIC_RT can be used to force link with the static system runtime libc. 

## Build

### Requirements

  - CMake >- 3.18
  - A C compiler that supports C99
  - A CXX compiler that supports C++11 (only to build tests)
  - Git >= 2.18
  - (optional) Visual Studio >= 2015 (for MSVC >= 1900 and test explorer)

As far as compilers go any of MSVC >= 1900, GCC >= 4.6 or CLANG >= 3.0 should do. 

#### Tools

[Git](https://git-scm.com/docs/git) is required to automatically download submodules and the test framework.

#### Submodules

[Catch2](https://github.com/catchorg/Catch2) is the unit testing framework. It is only used to build test programs.

[MbedTLS](https://github.com/ARMmbed/mbedtls) is a library that implements TLS and DTLS protocols. Python3 is required 
if you are interested in building MbedTLS tests and sample programs. By default, only the target libraries are built 
with static linking.

[libasr](https://github.com/OpenSMTPD/libasr) is a library used for asynchronous name resolution (NSS/DNS) on Un\*x 
systems. Windows 8+ already supports asynchronous name resolution via WSA and does not require an additional library.

All library submodules are configured to be statically linked into the main target. Not only it simplifies installation
but also contributes to keep functionality consistent regardless of the state of the system we deploy into.

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
immediately executed after installation if the prefix was customized. On the other hand, one generally does not 
want to pollute system folders with development libraries. A possible compromise in this case is to temporarily set 
$LD_LIBRARY_PATH to point to the devel libdir used by cmake to install the libraries. See the 
[ld.so(8)](https://man7.org/linux/man-pages/man8/ld.so.8.html) man page for more details. 

#### Windows

On Windows you might have to open a Native Tools Command Prompt rather than a normal command prompt if cmake was 
installed using the Visual Studio Installer. Mind that cmake will not automatically pick the target platform matching 
the NTCP used, so you still need to pass the target platform to cmake using `-A`. It also means you should be able to 
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

Note that standard CMake options (except for `BUILD_SHARED_LIBS` and `BUILD_TESTING`) are prefixed with 
`CMAKE` while all Clarinet options are prefixed with `CL`. Also note there are options that are dependent on 
other options. For example, `CL_ENABLE_TLS` will be automatically set to OFF if `CL_ENABLE_TCP` is OFF.

#### Shared vs Static build

The variable `BUILD_SHARED_LIBS` is used to control whether cmake will produce a shared lib or a static lib. 
By default, cmake produces a static lib. For a shared lib (.so/.dll) use:

```
cmake \path\to\clarinet -DBUILD_SHARED_LIBS=ON
``` 
 
Note that `BUILD_SHARED_LIBS` is defined as an option, so it does not affect submodules which will always be linked 
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

The variable `CMAKE_INSTALL_PREFIX` can be used to customize the installation directory during configuration as follows:

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
to execution order, but they are guaranteed to be included in `NATURAL` order so contiguous digits are compared as whole 
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

The variable `CL_BUILD_TESTING` is used to confirm whether tests should be configured when `BUILD_TESTING` is ON. 
It defaults to ON when this is the top project and OFF otherwise. An additional variable is required because top 
projects may configure their own unit tests using the variable `BUILD_TESTING` but usually they will not be interested 
in building unit tests of sub-projects (dependencies). If for any reason a top project do want to build and run our unit 
tests (e.g. top project is only an agregator) then it can force `CL_BUILD_TESTING=ON`.

The auto generated Visual Studio project *RUN_TESTS* or the makefile target *test* will run all tests when built, but 
they do not depend on the tests themselves therefore will not automatically rebuild out-of-date tests. Visual Studio 
2015, 2017 and 2019 require the [Test Adapter Catch2](https://github.com/JohnnyHendriks/TestAdapter_Catch2) 
extension for tests to be discovered in the test explorer window. Running a test out of the test explorer windoe will 
rebuild if the executable is out-of-date.

## TODO

- Export a conscise .editorconfig
- Implement Logging
- Add feature to enable/disable ipv6 scope id: CL_ENABLE_IPV6_SCOPE_ID/CL_FEATURE_IPV6_SCOPE_ID dependent on 
  HAVE_SOCKADDR_IN6_SCOPE_ID. Adjust functions and tests accordingly.
- Support IPv4 broadcast and IPv4/IPv6 multicast in the udp interface or define a bcast/mcast interface
- Support PMTUD on TCP depending on platform support (possibly only a matter of defining the right socket flags). May be
  unsafe due to the possibility of ICMP spoofing.
- Support PMTUD on UDP sockets with multiple destinations. This is not trivial because every destination has a different  
  path so not only the host will have to handle multiple concurrent MTU estimates it will also have to rely on the 
  socket error queue to determine which destination had a packet dropped due to MTU changes. The host will also have to 
  deal with ICMP blackholes and cannot let the operating system itself track the MTU because each destination might have 
  a different value, probably conflicting. Linux supports an error queue per destination using IP_RECVERR but the case 
  is not clear for Windows and BSD/Darwin.
- Support WebSockets (ws and wss) on top of TCP and TLS, client and server.
- Support custom memory allocator callbacks (malloc, free and nomem). Perhaps not worth the trouble unless we plan to 
  support embedded devices in which case libasr will have to be completely assimilated. Catch2 tests in C++ would also 
  have to be taken into account but custom memory allocators in C++11 is a pandora's box.
- Test cmake generator using "-T ClangCL" on Windows to ensure all conditions on MSVC are supported by the clang-cl too
- Add a C# wrapper
- Add a python wrapper 
- Support code sanitizers
- Use travis for automatic build checks?
- Run tests with Valgrind on Linux/WSL/macOS to detect memory leaks?
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
