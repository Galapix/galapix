// Copyright (C) 2015 Thomas Voß <thomas.voss.bochum@gmail.com>
//               2022 Ingo Ruhnke <grumbel@gmail.com>
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

#include <algorithm>
#include <cstdlib>
#include <iterator>
#include <stdexcept>
#include <string>
#include <vector>

namespace fs = std::filesystem;

namespace
{

// Split a colon-separated PATH-style string into components.
// Empty components (from leading/trailing/double colons) are preserved
// so the caller can decide to ignore them.
std::vector<std::string> path_split(const std::string& path)
{
    std::vector<std::string> result;
    std::size_t end = path.find(':');

    std::size_t start = 0;
    while (end != std::string::npos)
    {
        result.push_back(path.substr(start, end - start));
        start = end + 1;
        end = path.find(':', start);
    }

    result.push_back(path.substr(start, end - start));
    return result;
}

// Return true if the path is absolute (and therefore a candidate for use).
// Relative paths and the empty path are invalid per the XDG spec and must
// be ignored.
bool is_valid_xdg_path(const fs::path& p)
{
    return p.is_absolute();
}

namespace env
{
std::string get(const std::string& key, const std::string& default_value)
{
    if (const char* value = std::getenv(key.c_str()))
        return value;
    return default_value;
}

std::string get_or_throw(const std::string& key)
{
    if (const char* value = std::getenv(key.c_str()))
        return value;
    throw std::runtime_error{key + " not set in environment"};
}

constexpr const char* xdg_data_home{"XDG_DATA_HOME"};
constexpr const char* xdg_data_dirs{"XDG_DATA_DIRS"};
constexpr const char* xdg_config_home{"XDG_CONFIG_HOME"};
constexpr const char* xdg_config_dirs{"XDG_CONFIG_DIRS"};
constexpr const char* xdg_cache_home{"XDG_CACHE_HOME"};
constexpr const char* xdg_state_home{"XDG_STATE_HOME"};
constexpr const char* xdg_runtime_dir{"XDG_RUNTIME_DIR"};
}

// Require that HOME is set and absolute; used when constructing defaults.
fs::path home_or_throw()
{
    fs::path home{env::get_or_throw("HOME")};
    if (!is_valid_xdg_path(home))
        throw std::runtime_error{"HOME must be an absolute path"};
    return home;
}

namespace impl
{
class BaseDirSpecification : public xdg::BaseDirSpecification
{
public:
    static const BaseDirSpecification& instance()
    {
        static const BaseDirSpecification spec;
        return spec;
    }

    const xdg::Data& data() const override
    {
        return data_;
    }

    const xdg::Config& config() const override
    {
        return config_;
    }

    const xdg::State& state() const override
    {
        return state_;
    }

    const xdg::Cache& cache() const override
    {
        return cache_;
    }

    const xdg::Runtime& runtime() const override
    {
        return runtime_;
    }

private:
    xdg::Data data_;
    xdg::Config config_;
    xdg::State state_;
    xdg::Cache cache_;
    xdg::Runtime runtime_;
};
}
}

fs::path xdg::Data::home() const
{
    auto v = env::get(env::xdg_data_home, "");
    fs::path p{v};
    if (v.empty() || !is_valid_xdg_path(p))
        return home_or_throw() / ".local" / "share";
    return p;
}

std::vector<fs::path> xdg::Data::dirs() const
{
    auto v = env::get(env::xdg_data_dirs, "");
    if (v.empty())
        return {fs::path{"/usr/local/share"}, fs::path{"/usr/share"}};

    std::vector<fs::path> result;
    for (const auto& token : path_split(v))
    {
        fs::path p{token};
        if (is_valid_xdg_path(p))
            result.push_back(std::move(p));
    }

    // Spec: if the variable is not set or empty → defaults.
    // After discarding relative/empty entries an empty list is treated the
    // same way (common, practical reading of the "ignore invalid" rule).
    if (result.empty())
        return {fs::path{"/usr/local/share"}, fs::path{"/usr/share"}};
    return result;
}

fs::path xdg::Config::home() const
{
    auto v = env::get(env::xdg_config_home, "");
    fs::path p{v};
    if (v.empty() || !is_valid_xdg_path(p))
        return home_or_throw() / ".config";
    return p;
}

std::vector<fs::path> xdg::Config::dirs() const
{
    auto v = env::get(env::xdg_config_dirs, "");
    if (v.empty())
        return {fs::path{"/etc/xdg"}};

    std::vector<fs::path> result;
    for (const auto& token : path_split(v))
    {
        fs::path p{token};
        if (is_valid_xdg_path(p))
            result.push_back(std::move(p));
    }

    if (result.empty())
        return {fs::path{"/etc/xdg"}};
    return result;
}

fs::path xdg::State::home() const
{
    auto v = env::get(env::xdg_state_home, "");
    fs::path p{v};
    if (v.empty() || !is_valid_xdg_path(p))
        return home_or_throw() / ".local" / "state";
    return p;
}

fs::path xdg::Cache::home() const
{
    auto v = env::get(env::xdg_cache_home, "");
    fs::path p{v};
    if (v.empty() || !is_valid_xdg_path(p))
        return home_or_throw() / ".cache";
    return p;
}

fs::path xdg::Runtime::dir() const
{
    auto v = env::get(env::xdg_runtime_dir, "");
    fs::path p{v};
    // Relative or empty is treated as unset.  The specification recommends
    // a secure fallback + warning, but constructing one is outside the scope
    // of this library; callers must handle the missing directory.
    if (v.empty() || !is_valid_xdg_path(p))
        throw std::runtime_error{"Runtime directory not set"};
    return p;
}

std::shared_ptr<xdg::BaseDirSpecification> xdg::BaseDirSpecification::create()
{
    return std::make_shared<impl::BaseDirSpecification>();
}

const xdg::Data& xdg::data()
{
    return impl::BaseDirSpecification::instance().data();
}

const xdg::Config& xdg::config()
{
    return impl::BaseDirSpecification::instance().config();
}

const xdg::State& xdg::state()
{
    return impl::BaseDirSpecification::instance().state();
}

const xdg::Cache& xdg::cache()
{
    return impl::BaseDirSpecification::instance().cache();
}

const xdg::Runtime& xdg::runtime()
{
    return impl::BaseDirSpecification::instance().runtime();
}
