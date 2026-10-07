# TODO

## Audit against XDG Base Directory Specification (v0.8)

Spec: https://specifications.freedesktop.org/basedir-spec/latest/

### Spec conformance (done)

1. Relative paths ignored (not thrown).
2. Multi-value dirs: keep absolute entries; empty after filter → defaults.
3. `XDG_RUNTIME_DIR` still throws when unset/empty/relative (secure fallback out of scope).
4. `HOME` must be absolute when defaults are used.

### Test / CI (done)

- Self-contained unit-test suite covers unset/empty/relative/absolute for every variable.
- CMake registers the suite with CTest.
- Flake exposes `checks.xdgcpp` and runs tests via `doCheck`.

### CMake / packaging audit (done)

Fixed issues:

1. **xdgcpp-config.cmake.in typo** (`xdgpp` → proper `@PACKAGE_INIT@` + targets include).
2. **Static library destinations**: `ARCHIVE` + `LIBRARY` → `${CMAKE_INSTALL_LIBDIR}` (not `lib/xdgcpp/`).
3. **pkg-config `-L`** now matches the actual archive location.
4. **Removed obsolete `export(PACKAGE)`**.
5. **Namespaced alias** `xdgcpp::xdgcpp` for consumers.
6. **`add_subdirectory` support**:
   - Install / export rules run only when this project is top-level
     (`CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME`).
   - Parent can `add_subdirectory(...)` and `target_link_libraries(... xdgcpp::xdgcpp)`.
   - `BUILD_INTERFACE` include path points at the source `include/` directory.
7. **xdgcpp-info** installed to `bin/` but not exported in the package targets.
8. **Package version file** generated for `find_package` version checks.
9. **README** updated (no boost::filesystem; documents find_package and subdirectory).
10. **Flake** uses `lib.cleanSource`, has `meta`, keeps tests on the check path.

Verified locally:

- `find_package(xdgcpp)` + `target_link_libraries(... xdgcpp::xdgcpp)` builds and runs.
- `add_subdirectory(xdgcpp)` + same link line builds and runs; parent `cmake --install` does not install the embedded library.
- `pkg-config --cflags --libs xdgcpp` returns correct flags.

### Remaining / future

- [ ] Optional `std::optional` for `runtime().dir()` in a future major version.
- [ ] Windows path-separator support if ever ported.
