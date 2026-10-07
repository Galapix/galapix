// Copyright (C) 2015 Thomas Voß <thomas.voss.bochum@gmail.com>
//               2026 Ingo Ruhnke <grumbel@gmail.com>
//
// This library is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published
// by the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

#include <xdg.h>

#include <cstdlib>
#include <exception>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

// ---------------------------------------------------------------------------
// Minimal test harness (no third-party dependency)
// ---------------------------------------------------------------------------

namespace
{
int g_failures = 0;
int g_checks = 0;

struct TestCase
{
    const char* name;
    void (*fn)();
};

std::vector<TestCase>& registry()
{
    static std::vector<TestCase> tests;
    return tests;
}

struct Registrar
{
    Registrar(const char* name, void (*fn)())
    {
        registry().push_back({name, fn});
    }
};

#define TEST(name)                                                             \
    void name();                                                               \
    static Registrar registrar_##name(#name, &name);                           \
    void name()

void fail(const char* file, int line, const std::string& msg)
{
    ++g_failures;
    std::cerr << file << ":" << line << ": check failed: " << msg << "\n";
}

#define CHECK(expr)                                                            \
    do {                                                                       \
        ++g_checks;                                                            \
        if (!(expr))                                                           \
            fail(__FILE__, __LINE__, #expr);                                   \
    } while (0)

template <typename A, typename B>
void check_equal(const char* file, int line, const A& a, const B& b,
                 const char* a_str, const char* b_str)
{
    ++g_checks;
    if (!(a == b))
    {
        std::ostringstream os;
        os << a_str << " == " << b_str << "  (" << a << " != " << b << ")";
        fail(file, line, os.str());
    }
}

#define CHECK_EQUAL(a, b) check_equal(__FILE__, __LINE__, (a), (b), #a, #b)

#define CHECK_THROW(expr, exctype)                                             \
    do {                                                                       \
        ++g_checks;                                                            \
        bool threw = false;                                                    \
        try                                                                    \
        {                                                                      \
            (void)(expr);                                                      \
        }                                                                      \
        catch (const exctype&)                                                 \
        {                                                                      \
            threw = true;                                                      \
        }                                                                      \
        catch (...)                                                            \
        {                                                                      \
            fail(__FILE__, __LINE__,                                           \
                 std::string(#expr) + " threw unexpected exception type");    \
            break;                                                             \
        }                                                                      \
        if (!threw)                                                            \
            fail(__FILE__, __LINE__,                                           \
                 std::string(#expr) + " did not throw " + #exctype);           \
    } while (0)

// Clear every XDG-related variable so tests start from a known state.
void clear_xdg_env()
{
    ::unsetenv("XDG_DATA_HOME");
    ::unsetenv("XDG_DATA_DIRS");
    ::unsetenv("XDG_CONFIG_HOME");
    ::unsetenv("XDG_CONFIG_DIRS");
    ::unsetenv("XDG_STATE_HOME");
    ::unsetenv("XDG_CACHE_HOME");
    ::unsetenv("XDG_RUNTIME_DIR");
}

void set_home(const char* value)
{
    ::setenv("HOME", value, 1);
}
} // namespace

// ---------------------------------------------------------------------------
// XDG_DATA_HOME
// ---------------------------------------------------------------------------

TEST(XdgDataHomeIgnoresRelativeDirectoryFromEnv)
{
    clear_xdg_env();
    set_home("/tmp");
    ::setenv("XDG_DATA_HOME", "tmp", 1);
    CHECK_EQUAL(xdg::BaseDirSpecification::create()->data().home(),
                "/tmp/.local/share");
    CHECK_EQUAL(xdg::data().home(), "/tmp/.local/share");
}

TEST(XdgDataHomeReturnsDefaultValueForEmptyEnv)
{
    clear_xdg_env();
    set_home("/tmp");
    ::setenv("XDG_DATA_HOME", "", 1);
    CHECK_EQUAL(xdg::BaseDirSpecification::create()->data().home(),
                "/tmp/.local/share");
    CHECK_EQUAL(xdg::data().home(), "/tmp/.local/share");
}

TEST(XdgDataHomeReturnsDefaultValueWhenUnset)
{
    clear_xdg_env();
    set_home("/tmp");
    CHECK_EQUAL(xdg::data().home(), "/tmp/.local/share");
}

TEST(XdgDataHomeUsesAbsoluteEnvValue)
{
    clear_xdg_env();
    set_home("/tmp");
    ::setenv("XDG_DATA_HOME", "/custom/data", 1);
    CHECK_EQUAL(xdg::BaseDirSpecification::create()->data().home(),
                "/custom/data");
    CHECK_EQUAL(xdg::data().home(), "/custom/data");
}

TEST(XdgDataHomeThrowsWhenHomeMissing)
{
    clear_xdg_env();
    ::unsetenv("HOME");
    CHECK_THROW(xdg::data().home(), std::runtime_error);
}

TEST(XdgDataHomeThrowsWhenHomeRelative)
{
    clear_xdg_env();
    set_home("relative-home");
    CHECK_THROW(xdg::data().home(), std::runtime_error);
}

// ---------------------------------------------------------------------------
// XDG_DATA_DIRS
// ---------------------------------------------------------------------------

TEST(XdgDataDirsCorrectlyTokenizesEnv)
{
    clear_xdg_env();
    ::setenv("XDG_DATA_DIRS", "/tmp:/var", 1);
    auto dirs = xdg::BaseDirSpecification::create()->data().dirs();
    CHECK_EQUAL(dirs.size(), 2u);
    CHECK_EQUAL(dirs[0], "/tmp");
    CHECK_EQUAL(dirs[1], "/var");
    dirs = xdg::data().dirs();
    CHECK_EQUAL(dirs.size(), 2u);
}

TEST(XdgDataDirsIgnoresRelativeEntries)
{
    clear_xdg_env();
    ::setenv("XDG_DATA_DIRS", "/tmp:tmp:/usr/share", 1);
    auto dirs = xdg::data().dirs();
    CHECK_EQUAL(dirs.size(), 2u);
    CHECK_EQUAL(dirs[0], "/tmp");
    CHECK_EQUAL(dirs[1], "/usr/share");
}

TEST(XdgDataDirsFallsBackWhenAllRelative)
{
    clear_xdg_env();
    ::setenv("XDG_DATA_DIRS", "tmp:relative", 1);
    auto dirs = xdg::data().dirs();
    CHECK_EQUAL(dirs.size(), 2u);
    CHECK_EQUAL(dirs[0], "/usr/local/share");
    CHECK_EQUAL(dirs[1], "/usr/share");
}

TEST(XdgDataDirsReturnsDefaultValueForEmptyEnv)
{
    clear_xdg_env();
    ::setenv("XDG_DATA_DIRS", "", 1);
    auto dirs = xdg::data().dirs();
    CHECK_EQUAL(dirs.size(), 2u);
    CHECK_EQUAL(dirs[0], "/usr/local/share");
    CHECK_EQUAL(dirs[1], "/usr/share");

    dirs = xdg::BaseDirSpecification::create()->data().dirs();
    CHECK_EQUAL(dirs.size(), 2u);
    CHECK_EQUAL(dirs[0], "/usr/local/share");
    CHECK_EQUAL(dirs[1], "/usr/share");
}

TEST(XdgDataDirsReturnsDefaultValueWhenUnset)
{
    clear_xdg_env();
    auto dirs = xdg::data().dirs();
    CHECK_EQUAL(dirs.size(), 2u);
    CHECK_EQUAL(dirs[0], "/usr/local/share");
    CHECK_EQUAL(dirs[1], "/usr/share");
}

TEST(XdgDataDirsIgnoresEmptyComponents)
{
    clear_xdg_env();
    ::setenv("XDG_DATA_DIRS", "/tmp::/usr/share:", 1);
    auto dirs = xdg::data().dirs();
    CHECK_EQUAL(dirs.size(), 2u);
    CHECK_EQUAL(dirs[0], "/tmp");
    CHECK_EQUAL(dirs[1], "/usr/share");
}

TEST(XdgDataDirsSingleAbsoluteEntry)
{
    clear_xdg_env();
    ::setenv("XDG_DATA_DIRS", "/only/one", 1);
    auto dirs = xdg::data().dirs();
    CHECK_EQUAL(dirs.size(), 1u);
    CHECK_EQUAL(dirs[0], "/only/one");
}

// ---------------------------------------------------------------------------
// XDG_CONFIG_HOME
// ---------------------------------------------------------------------------

TEST(XdgConfigHomeIgnoresRelativeDirectoryFromEnv)
{
    clear_xdg_env();
    set_home("/tmp");
    ::setenv("XDG_CONFIG_HOME", "tmp", 1);
    CHECK_EQUAL(xdg::BaseDirSpecification::create()->config().home(),
                "/tmp/.config");
    CHECK_EQUAL(xdg::config().home(), "/tmp/.config");
}

TEST(XdgConfigHomeReturnsDefaultValueForEmptyEnv)
{
    clear_xdg_env();
    set_home("/tmp");
    ::setenv("XDG_CONFIG_HOME", "", 1);
    CHECK_EQUAL(xdg::BaseDirSpecification::create()->config().home(),
                "/tmp/.config");
    CHECK_EQUAL(xdg::config().home(), "/tmp/.config");
}

TEST(XdgConfigHomeReturnsDefaultValueWhenUnset)
{
    clear_xdg_env();
    set_home("/tmp");
    CHECK_EQUAL(xdg::config().home(), "/tmp/.config");
}

TEST(XdgConfigHomeUsesAbsoluteEnvValue)
{
    clear_xdg_env();
    ::setenv("XDG_CONFIG_HOME", "/custom/config", 1);
    CHECK_EQUAL(xdg::config().home(), "/custom/config");
}

// ---------------------------------------------------------------------------
// XDG_CONFIG_DIRS
// ---------------------------------------------------------------------------

TEST(XdgConfigDirsCorrectlyTokenizesEnv)
{
    clear_xdg_env();
    ::setenv("XDG_CONFIG_DIRS", "/tmp:/etc", 1);
    CHECK_EQUAL(xdg::BaseDirSpecification::create()->config().dirs().size(),
                2u);
    CHECK_EQUAL(xdg::config().dirs().size(), 2u);
}

TEST(XdgConfigDirsIgnoresRelativeEntries)
{
    clear_xdg_env();
    ::setenv("XDG_CONFIG_DIRS", "/tmp:tmp:/etc/xdg", 1);
    auto dirs = xdg::config().dirs();
    CHECK_EQUAL(dirs.size(), 2u);
    CHECK_EQUAL(dirs[0], "/tmp");
    CHECK_EQUAL(dirs[1], "/etc/xdg");
}

TEST(XdgConfigDirsFallsBackWhenAllRelative)
{
    clear_xdg_env();
    ::setenv("XDG_CONFIG_DIRS", "tmp:relative", 1);
    auto dirs = xdg::config().dirs();
    CHECK_EQUAL(dirs.size(), 1u);
    CHECK_EQUAL(dirs[0], "/etc/xdg");
}

TEST(XdgConfigDirsReturnsDefaultValueForEmptyEnv)
{
    clear_xdg_env();
    ::setenv("XDG_CONFIG_DIRS", "", 1);
    auto dirs = xdg::config().dirs();
    CHECK_EQUAL(dirs.size(), 1u);
    CHECK_EQUAL(dirs[0], "/etc/xdg");
    dirs = xdg::BaseDirSpecification::create()->config().dirs();
    CHECK_EQUAL(dirs[0], "/etc/xdg");
}

TEST(XdgConfigDirsReturnsDefaultValueWhenUnset)
{
    clear_xdg_env();
    auto dirs = xdg::config().dirs();
    CHECK_EQUAL(dirs.size(), 1u);
    CHECK_EQUAL(dirs[0], "/etc/xdg");
}

// ---------------------------------------------------------------------------
// XDG_STATE_HOME
// ---------------------------------------------------------------------------

TEST(XdgStateHomeIgnoresRelativeDirectoryFromEnv)
{
    clear_xdg_env();
    set_home("/tmp");
    ::setenv("XDG_STATE_HOME", "tmp", 1);
    CHECK_EQUAL(xdg::BaseDirSpecification::create()->state().home(),
                "/tmp/.local/state");
    CHECK_EQUAL(xdg::state().home(), "/tmp/.local/state");
}

TEST(XdgStateHomeReturnsDefaultValueForEmptyEnv)
{
    clear_xdg_env();
    set_home("/tmp");
    ::setenv("XDG_STATE_HOME", "", 1);
    CHECK_EQUAL(xdg::BaseDirSpecification::create()->state().home(),
                "/tmp/.local/state");
    CHECK_EQUAL(xdg::state().home(), "/tmp/.local/state");
}

TEST(XdgStateHomeReturnsDefaultValueWhenUnset)
{
    clear_xdg_env();
    set_home("/tmp");
    CHECK_EQUAL(xdg::state().home(), "/tmp/.local/state");
}

TEST(XdgStateHomeUsesAbsoluteEnvValue)
{
    clear_xdg_env();
    ::setenv("XDG_STATE_HOME", "/custom/state", 1);
    CHECK_EQUAL(xdg::state().home(), "/custom/state");
}

// ---------------------------------------------------------------------------
// XDG_CACHE_HOME
// ---------------------------------------------------------------------------

TEST(XdgCacheHomeIgnoresRelativeDirectoryFromEnv)
{
    clear_xdg_env();
    set_home("/tmp");
    ::setenv("XDG_CACHE_HOME", "tmp", 1);
    CHECK_EQUAL(xdg::BaseDirSpecification::create()->cache().home(),
                "/tmp/.cache");
    CHECK_EQUAL(xdg::cache().home(), "/tmp/.cache");
}

TEST(XdgCacheHomeReturnsDefaultValueForEmptyEnv)
{
    clear_xdg_env();
    set_home("/tmp");
    ::setenv("XDG_CACHE_HOME", "", 1);
    CHECK_EQUAL(xdg::BaseDirSpecification::create()->cache().home(),
                "/tmp/.cache");
    CHECK_EQUAL(xdg::cache().home(), "/tmp/.cache");
}

TEST(XdgCacheHomeReturnsDefaultValueWhenUnset)
{
    clear_xdg_env();
    set_home("/tmp");
    CHECK_EQUAL(xdg::cache().home(), "/tmp/.cache");
}

TEST(XdgCacheHomeUsesAbsoluteEnvValue)
{
    clear_xdg_env();
    ::setenv("XDG_CACHE_HOME", "/custom/cache", 1);
    CHECK_EQUAL(xdg::cache().home(), "/custom/cache");
}

// ---------------------------------------------------------------------------
// XDG_RUNTIME_DIR
// ---------------------------------------------------------------------------

TEST(XdgRuntimeDirThrowsForRelativeDirectoryFromEnv)
{
    clear_xdg_env();
    ::setenv("XDG_RUNTIME_DIR", "tmp", 1);
    CHECK_THROW(xdg::BaseDirSpecification::create()->runtime().dir(),
                std::runtime_error);
    CHECK_THROW(xdg::runtime().dir(), std::runtime_error);
}

TEST(XdgRuntimeDirThrowsForEmptyEnv)
{
    clear_xdg_env();
    ::setenv("XDG_RUNTIME_DIR", "", 1);
    CHECK_THROW(xdg::BaseDirSpecification::create()->runtime().dir(),
                std::runtime_error);
    CHECK_THROW(xdg::runtime().dir(), std::runtime_error);
}

TEST(XdgRuntimeDirThrowsWhenUnset)
{
    clear_xdg_env();
    CHECK_THROW(xdg::runtime().dir(), std::runtime_error);
}

TEST(XdgRuntimeDirUsesAbsoluteEnvValue)
{
    clear_xdg_env();
    ::setenv("XDG_RUNTIME_DIR", "/run/user/1000", 1);
    CHECK_EQUAL(xdg::runtime().dir(), "/run/user/1000");
    CHECK_EQUAL(xdg::BaseDirSpecification::create()->runtime().dir(),
                "/run/user/1000");
}

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------

int main()
{
    int failed_cases = 0;
    for (const auto& tc : registry())
    {
        const int before = g_failures;
        try
        {
            tc.fn();
        }
        catch (const std::exception& e)
        {
            ++g_failures;
            std::cerr << "Test " << tc.name
                      << " threw uncaught exception: " << e.what() << "\n";
        }
        catch (...)
        {
            ++g_failures;
            std::cerr << "Test " << tc.name
                      << " threw uncaught non-std exception\n";
        }
        if (g_failures != before)
        {
            ++failed_cases;
            std::cerr << "FAIL " << tc.name << "\n";
        }
    }

    const int total = static_cast<int>(registry().size());
    if (failed_cases == 0)
    {
        std::cout << "All " << total << " test cases passed (" << g_checks
                  << " checks).\n";
        return 0;
    }

    std::cerr << failed_cases << " of " << total << " test cases failed ("
              << g_failures << " failed checks of " << g_checks << ").\n";
    return 1;
}
