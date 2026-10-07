# xdgcpp

A straightforward implementation of the
[XDG Base Directory Specification](https://specifications.freedesktop.org/basedir-spec/latest/)
in C++17.

## Dependencies

- A C++17 compiler with `<filesystem>` support

No third-party libraries are required. The optional unit-test suite is
self-contained (a few macros in `src/xdg_test.cpp`).

## Build

```bash
cmake -B build -DXDG_BUILD_TESTS=ON
cmake --build build
ctest --test-dir build --output-on-failure
cmake --install build
```

Nix:

```bash
nix build
nix flake check
```

## Quick integration (installed package)

```cpp
#include <xdg.h>

#include <iostream>

int main()
{
    std::cout << xdg::data().home() << std::endl;
    std::cout << xdg::config().home() << std::endl;
    std::cout << xdg::cache().home() << std::endl;
    std::cout << xdg::runtime().dir() << std::endl;
    return 0;
}
```

```cmake
find_package(xdgcpp REQUIRED)
target_link_libraries(myapp PRIVATE xdgcpp::xdgcpp)
```

Or via pkg-config: `pkg-config --cflags --libs xdgcpp`.

## Use as a subdirectory

xdgcpp is designed to be embedded with `add_subdirectory` (for example as a
git submodule or a vendored copy).  Install rules are skipped automatically
when the project is not the top-level CMake project.

```cmake
# in your top-level CMakeLists.txt
add_subdirectory(third_party/xdgcpp)   # or wherever you placed it

target_link_libraries(myapp PRIVATE xdgcpp::xdgcpp)
# #include <xdg.h> works via the target's INTERFACE include directories
```

You can still enable tests of the embedded copy if desired:

```cmake
set(XDG_BUILD_TESTS ON CACHE BOOL "" FORCE)
add_subdirectory(third_party/xdgcpp)
```

## Complete integration (testable interface)

The interface `xdg::BaseDirSpecification` can be used so that interaction
with the XDG paths is mockable in unit tests.

```cpp
#include <xdg.h>

class MyClass
{
public:
    explicit MyClass(const std::shared_ptr<xdg::BaseDirSpecification>& bds)
        : bds{bds}
    {
    }

    void do_something()
    {
        auto path = bds->config().home();
        // ...
    }

private:
    std::shared_ptr<xdg::BaseDirSpecification> bds;
};
```

## License

GNU Lesser General Public License v3.0 or later (LGPL-3.0-or-later).
