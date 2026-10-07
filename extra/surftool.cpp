// surf - Software surface library
// Copyright (C) 2008-2020 Ingo Ruhnke <grumbel@gmail.com>
//
// This program is free software: you can redistribute it and/or modify it
// under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or (at your
// option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
// or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public
// License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this program. If not, see <http://www.gnu.org/licenses/>.

#include <filesystem>
#include <format>
#include <functional>
#include <iostream>
#include <memory>
#include <stack>
#include <variant>

#include <geom/io.hpp>
#include <surf/blendfunc.hpp>
#include <surf/channel.hpp>
#include <surf/io.hpp>
#include <surf/surf.hpp>

using surf::SoftwareSurface;

namespace {

class Context;

using ContextCommand = std::variant<std::function<void(Context& ctx)>,
                                    std::filesystem::path>;

class Context
{
public:
  struct FileInfo {
    int index = 0;
    std::filesystem::path filename = {};
  };

public:
  Context() :
    m_verbose(false),
    m_file_info(),
    m_blendfunc(surf::BlendFunc::COPY),
    m_stack()
  {}

  void clear() {
    m_blendfunc = surf::BlendFunc::COPY;
    m_stack.clear();
  }

  void eval(ContextCommand const& cmd) {
    if (cmd.index() == 0) {
      std::get<0>(cmd)(*this);
    } else {
      push(SoftwareSurface::from_file(std::get<1>(cmd)));
    }
  }

  size_t idx(int n) const {
    if (n >= 0) {
      return n;
    } else {
      return static_cast<int>(m_stack.size()) + n;
    }
  }

  SoftwareSurface& get(int n) {
    return *m_stack[idx(n)];
  }

  SoftwareSurface& top() {
    if (m_stack.empty()) {
      throw std::runtime_error("can't top(), stack empty");
    }
    return *m_stack.back();
  }

  void drop(int n) {
    m_stack.erase(m_stack.begin() + idx(n));
  }

  SoftwareSurface pop() {
    if (m_verbose) {
      std::cout << "pop surface\n";
    }

    if (m_stack.empty()) {
      throw std::runtime_error("can't pop(), stack empty");
    }

    SoftwareSurface sur;
    if (m_stack.back().use_count() == 1) {
      sur = std::move(*m_stack.back());
    } else {
      // FIXME: potentially useless copy
      sur = *m_stack.back();
    }
    m_stack.pop_back();
    return sur;
  }

  void push(SoftwareSurface&& img) {
    if (m_verbose) {
      std::cout << "push surface\n";
    }
    m_stack.emplace_back(std::make_shared<SoftwareSurface>(std::move(img)));
  }

  void push_ref(int n) {
    if (m_verbose) {
      std::cout << "push_ref surface\n";
    }
    m_stack.push_back(m_stack[idx(n)]);
  }

  void set_blendfunc(surf::BlendFunc func) {
    if (m_verbose) {
      std::cout << "set_blendfunc: " << int(func) << "\n";
    }

    m_blendfunc = func;
  }

  surf::BlendFunc blendfunc() const {
    return m_blendfunc;
  }

  FileInfo const& file_info() const {
    return m_file_info;
  }

  void set_file_info(int v, std::filesystem::path filename) {
    m_file_info.index = v;
    m_file_info.filename = std::move(filename);
  }

  void message(std::string const& msg) { // NOLINT
    if (m_verbose) {
      std::cout << msg << std::endl;
    }
  }

  void set_verbose(bool v) { m_verbose = v; }
  //bool verbose() const { return m_verbose; }

private:
  bool m_verbose;
  FileInfo m_file_info;
  surf::BlendFunc m_blendfunc;
  std::vector<std::shared_ptr<SoftwareSurface>> m_stack;
};

struct Options
{
  bool verbose = false;
  std::vector<ContextCommand> foreach_commands = {};
  std::vector<ContextCommand> commands = {};
};

void print_usage(int argc, char** argv)
{
  std::cout
    << argv[0] << " ( [IMGFILE] [OPTION] )...\n"
    << "\n"
    << "General Options:\n"
    << " -h, --help      Display this text\n"
    << " -v, --verbose   Be verbose\n"
    << "\n"
    << "File Commands:\n"
    << "  --foreach                 Process each image individually\n"
    << "  -o, --output FILE         Output filename\n"
    << "  -O, --output-pattern PAT  Output filename with pattern\n"
    //<< "  --output-dir DIR     Output directory\n"
    << "\n"
    << "Image Commands:\n"
    << "  --create WxH COLOR   Create a new empty image\n"
    << "  --fill COLOR         Fill the image with COLOR\n"
    << "  --checkerboard SIZE COLOR\n"
    << "                       Fill the image with a checkerboard\n"
    << "  --invert             Invert the image\n"
    << "  --gamma VALUE        Apply gamma correction\n"
    << "  --brightness VALUE   Adjust brightness\n"
    << "  --contrast VALUE     Adjust contrast\n"
    << "  --halve              Scale to halve the size\n"
    << "  --scale WxH{!><}     Resize the image\n"
    << "  --crop WxH[+X+Y]     Crop the image\n"
    << "  --transform ROT      Rotate or flip the image\n"
    << "  --threshold VALUE    Apply the given threshold\n"
    << "  --grayscale          Convert to grayscale\n"
    << "  --hsv H:S:V          Apply hue/saturation/value\n"
    << "  --convert FORMAT     Convert internal format to FORMAT\n"
    << "  --blit POS           Blit image\n"
    << "  --blendfunc FUNC     Switch blendfunc to FUNC\n"
    << "  --blend POS          Blend image\n"
    << "  --blend-scaled RECT  Blit image scaled\n"
    << "  --multiply VALUE     Multiply the image by value\n"
    << "  --add VALUE          Add value to pixels\n"
    << "  --split              Split image into channels\n"
    << "\n"
    << "Stack Commands:\n"
    << "  --dup                Duplicate the top image\n"
    << "  --drop               Remove the top image\n"
    << "  --dupi INDEX         Duplicate INDEX\n"
    << "  --dropi INDEX        Remove INDEX\n"
    << "  --movei INDEX        Move INDEX to top\n"
    << "  --swap               Swap the two top most items\n"
    << "  --withi INDEX        Push a view on INDEX on the top\n"
    << "\n";
}

std::string to_positional_format(std::string fmt)
{
  // name → argument position
  static constexpr std::pair<std::string_view, int> mapping[] = {
    {"index",    0},
    {"dirname",  1},
    {"basename", 2},
    {"path",     3},
    {"stem",     4},
    {"ext",      5},
  };

  for (auto const& [name, index] : mapping) {
    const std::string from = "{" + std::string(name);
    const std::string to   = "{" + std::to_string(index);

    std::size_t pos = 0;
    while ((pos = fmt.find(from, pos)) != std::string::npos) {
      // Only replace if it is a real placeholder:
      // {name}  or  {name:...}
      const char next = (pos + from.size() < fmt.size()) ? fmt[pos + from.size()] : '\0';
      if (next == '}' || next == ':') {
        fmt.replace(pos, from.size(), to);
        pos += to.size();
      } else {
        pos += from.size(); // false match, skip
      }
    }
  }
  return fmt;
}

Options parse_args(int argc, char** argv)
{
  Options opts;

  if (argc == 1) {
    print_usage(argc, argv);
    exit(EXIT_SUCCESS);
  }

  for (int i = 1; i < argc; ++i) {
    auto next_arg = [&]{
      if (++i >= argc) {
        std::ostringstream msg;
        msg << argv[i - 1] << " needs an argument";
        throw std::runtime_error(msg.str());
      }
      return std::string_view(argv[i]);
    };

    if (argv[i][0] == '-') {
      std::string_view opt = argv[i];
      if (opt == "--help" || opt == "-h") {
        print_usage(argc, argv);
        exit(EXIT_SUCCESS);
      } else if (opt == "-v" || opt == "--verbose") {
        opts.verbose = true;
      } else if (opt == "--dup") {
        opts.commands.emplace_back([](Context& ctx) {
          SoftwareSurface img(ctx.top());
          ctx.push(std::move(img));
        });
      } else if (opt == "--drop") {
        opts.commands.emplace_back([](Context& ctx) {
          ctx.pop();
        });
      } else if (opt == "--swap") {
        opts.commands.emplace_back([](Context& ctx) {
          SoftwareSurface one = ctx.pop();
          SoftwareSurface two = ctx.pop();
          ctx.push(std::move(one));
          ctx.push(std::move(two));
        });
      } else if (opt == "--dupi") {
        int idx = std::stoi(std::string(next_arg()));
        opts.commands.emplace_back([idx](Context& ctx) {
          ctx.message(std::format("dup {}", idx));
          SoftwareSurface copy(ctx.get(idx));
          ctx.push(std::move(copy)); // NOLINT
        });
      } else if (opt == "--dropi") {
        int idx = std::stoi(std::string(next_arg()));
        opts.commands.emplace_back([idx](Context& ctx) {
          ctx.message(std::format("drop {}", idx));
          ctx.drop(idx);
        });
      } else if (opt == "--movei") {
        int idx = std::stoi(std::string(next_arg()));
        opts.commands.emplace_back([idx](Context& ctx) {
          ctx.message(std::format("move {}", idx));
          SoftwareSurface sur = ctx.get(idx);
          ctx.drop(idx);
          ctx.push(std::move(sur));
        });
      } else if (opt == "--withi") {
        int idx = std::stoi(std::string(next_arg()));
        opts.commands.emplace_back([idx](Context& ctx) {
          ctx.message(std::format("view {}", idx));
          ctx.push_ref(idx);
        });
      } else if (opt == "--create") {
        surf::PixelFormat format = surf::pixelformat_from_string(next_arg());
        geom::isize const size = geom::isize_from_string(std::string(next_arg()));
        surf::Color const color = surf::Color::from_string(next_arg());
        opts.commands.emplace_back([format, size, color](Context& ctx) {
          ctx.push(SoftwareSurface::create(format, size, color));
        });
      } else if (opt == "--invert") {
        opts.commands.emplace_back([](Context& ctx) {
          surf::apply_invert(ctx.top());
        });
      } else if (opt == "--gamma") {
        std::string_view arg = next_arg();
        float value = std::stof(std::string(arg));
        opts.commands.emplace_back([value](Context& ctx) {
          surf::apply_gamma(ctx.top(), value);
        });
      } else if (opt == "--multiply") {
        std::string_view arg = next_arg();
        float value = std::stof(std::string(arg));
        opts.commands.emplace_back([value](Context& ctx) {
          surf::apply_multiply(ctx.top(), value);
        });
      } else if (opt == "--add") {
        std::string_view arg = next_arg();
        float value = std::stof(std::string(arg));
        opts.commands.emplace_back([value](Context& ctx) {
          surf::apply_add(ctx.top(), value);
        });
      } else if (opt == "--brightness") {
        std::string_view arg = next_arg();
        float value = std::stof(std::string(arg));
        opts.commands.emplace_back([value](Context& ctx) {
          surf::apply_brightness(ctx.top(), value);
        });
      } else if (opt == "--contrast") {
        std::string_view arg = next_arg();
        float value = std::stof(std::string(arg));
        opts.commands.emplace_back([value](Context& ctx) {
          surf::apply_contrast(ctx.top(), value);
        });
      } else if (opt == "--threshold") {
        std::string_view arg = next_arg();
        float rthreshold = 0.5f;
        float gthreshold = 0.5f;
        float bthreshold = 0.5f;
        if (sscanf(argv[i], " %f, %f, %f ", &rthreshold, &gthreshold, &bthreshold) != 3) {
          rthreshold = gthreshold = bthreshold = std::stof(std::string(arg));
        }
        opts.commands.emplace_back([rthreshold, gthreshold, bthreshold](Context& ctx) {
          surf::apply_threshold(ctx.top(), surf::Color(rthreshold, gthreshold, bthreshold));
        });
      } else if (opt == "--hsv") {
        next_arg();
        float hue = 0.0f;
        float saturation = 0.5f;
        float value = 1.0f;
        if (sscanf(argv[i], " %f, %f, %f ", &hue, &saturation, &value) != 3) {
          throw std::invalid_argument("invalid argument");
        }
        opts.commands.emplace_back([hue, saturation, value](Context& ctx) {
          surf::apply_hsv(ctx.top(), hue, saturation, value);
        });
      } else if (opt == "--blendfunc") {
        std::string_view arg = next_arg();
        surf::BlendFunc blendfunc = surf::BlendFunc_from_string(arg);
        opts.commands.emplace_back([blendfunc](Context& ctx) {
          ctx.set_blendfunc(blendfunc);
        });
      } else if (opt == "--grayscale") {
        opts.commands.emplace_back([](Context& ctx) {
          surf::apply_grayscale(ctx.top());
        });
      } else if (opt == "--convert") {
        std::string_view arg = next_arg();
        auto format = surf::pixelformat_from_string(arg);
        opts.commands.emplace_back([format](Context& ctx) {
          ctx.top() = surf::convert(ctx.top(), format);
        });
      } else if (opt == "--blit") {
        std::string_view pos_str = next_arg();
        geom::ipoint pos = geom::ipoint_from_string(std::string(pos_str));

        opts.commands.emplace_back([pos](Context& ctx) {
          auto img = ctx.pop();
          blit(img, ctx.top(), pos);
        });
      } else if (opt == "--blend-scaled") {
        std::string_view rect_str = next_arg();
        geom::irect rect = geom::irect_from_string(std::string(rect_str));

        opts.commands.emplace_back([rect](Context& ctx) {
          auto img = ctx.pop();
          blend_scaled(ctx.blendfunc(), img, ctx.top(), rect);
        });
      } else if (opt == "--blend") {
        std::string_view pos_str = next_arg();
        geom::ipoint pos = geom::ipoint_from_string(std::string(pos_str));

        opts.commands.emplace_back([pos](Context& ctx) {
          auto img = ctx.pop();
          blend(ctx.blendfunc(), img, ctx.top(), pos);
        });
      } else if (opt == "--scale") {
        std::string_view arg = next_arg();
        char op = ' ';
        if (arg.back() == '!' || arg.back() == '<' || arg.back() == '>') {
          op = arg.back();
          arg = arg.substr(0, arg.length() - 1);
        }

        geom::isize desired_size = geom::isize_from_string(std::string(arg));
        opts.commands.emplace_back([desired_size, op](Context& ctx) {
          geom::isize size = desired_size;
          if (op == '!') {
            // nop
          } else if (op == '<') {
            size = geom::grow_to_fit(ctx.top().get_size(), desired_size);
          } else if (op == '>') {
            size = geom::shrink_to_fit(ctx.top().get_size(), desired_size);
          } else {
            size = geom::resize_to_fit(ctx.top().get_size(), desired_size);
          }
          ctx.top() = surf::scale(ctx.top(), size);
        });
      } else if (opt == "--halve") {
        opts.commands.emplace_back([](Context& ctx) {
          ctx.top() = surf::halve(ctx.top());
        });
      } else if (opt == "--crop") {
        std::string_view arg = next_arg();
        geom::irect rect = geom::irect_from_string(std::string(arg));
        opts.commands.emplace_back([rect](Context& ctx) {
          ctx.top() = surf::crop(ctx.top(), rect);
        });
      } else if (opt == "--transform") {
        std::string_view arg = next_arg();
        surf::Transform const transf = surf::transform_from_string(arg);
        opts.commands.emplace_back([transf](Context& ctx) {
          ctx.top() = surf::transform(ctx.top(), transf);
        });
      } else if (opt == "--fill") {
        std::string_view arg = next_arg();
        surf::Color const color = surf::Color::from_string(arg);
        opts.commands.emplace_back([color](Context& ctx) {
          surf::fill(ctx.top(), color);
        });
      } else if (opt == "--checkerboard") {
        auto size = geom::isize_from_string(std::string(next_arg()));
        auto color = surf::Color::from_string(next_arg());

        opts.commands.emplace_back([size, color](Context& ctx) {
          surf::fill_checkerboard(ctx.top(), size, color);
        });
      } else if (opt == "--offset") {
        std::string_view arg = next_arg();
        geom::ipoint const offset = geom::ipoint_from_string(std::string(arg));
        opts.commands.emplace_back([offset](Context& ctx) {
          surf::apply_offset(ctx.top(), geom::ioffset(offset));
        });
      } else if (opt == "--split") {
        opts.commands.emplace_back([](Context& ctx) {
          auto res = surf::split_channel(ctx.pop());
          for (auto&& item : res) {
            ctx.push(std::move(item));
          }
        });
      } else if (opt == "--join-rgba") {
        opts.commands.emplace_back([](Context& ctx) {
          SoftwareSurface alpha = ctx.pop();
          SoftwareSurface blue = ctx.pop();
          SoftwareSurface green = ctx.pop();
          SoftwareSurface red = ctx.pop();

          ctx.push(surf::join_channel({red, green, blue, alpha}));
        });
      } else if (opt == "--join-rgb") {
        opts.commands.emplace_back([](Context& ctx) {
          SoftwareSurface blue = ctx.pop();
          SoftwareSurface green = ctx.pop();
          SoftwareSurface red = ctx.pop();

          ctx.push(surf::join_channel({red, green, blue}));
        });
      } else if (opt == "--foreach") {
        opts.foreach_commands = std::move(opts.commands);
        opts.commands.clear();
      } else if (opt == "-o" || opt == "--output") {
        std::filesystem::path output_filename = next_arg();
        opts.commands.emplace_back([output_filename](Context& ctx) {
          surf::save(ctx.top(), output_filename);
        });
      } else if (opt == "-O" || opt == "--output-pattern") {
        std::string output_pattern = to_positional_format(std::string(next_arg()));
        opts.commands.emplace_back([output_pattern](Context& ctx) {
          auto const index = ctx.file_info().index;                                    // 0
          std::string const dirname = ctx.file_info().filename.parent_path().string(); // 1
          std::string const basename = ctx.file_info().filename.filename().string();   // 2
          std::string const path = ctx.file_info().filename.string();                  // 3
          std::string const stem = ctx.file_info().filename.stem().string();           // 4
          std::string const ext = ctx.file_info().filename.extension().string();       // 5

          std::string output_filename = std::vformat(
            output_pattern,
            std::make_format_args(
              index,    // 0
              dirname,  // 1
              basename, // 2
              path,     // 3
              stem,     // 4
              ext       // 5
              ));
          ctx.message(std::format("saving {}", output_filename));
          surf::save(ctx.top(), output_filename);
        });
      } else {
        print_usage(argc, argv);
        throw std::invalid_argument(std::format("unknown option: {}", opt));
      }
    } else { // rest argument
      std::filesystem::path input_filename = argv[i];
      opts.commands.emplace_back(input_filename);
    }
  }

  return opts;
}

void run(int argc, char** argv)
{
  Options opts = parse_args(argc, argv);

  Context ctx;
  ctx.set_verbose(opts.verbose);
  if (opts.foreach_commands.empty()) {
    for (auto&& cmd : opts.commands) {
      ctx.eval(cmd);
    }
  } else {
    int index = 0;
    for (auto&& foreachcmd : opts.foreach_commands) {
      if (foreachcmd.index() != 1) {
        throw std::runtime_error(std::format("invalid --foreach command: {}", foreachcmd.index()));
      } else {
        ctx.clear();
        ctx.set_file_info(index, std::get<1>(foreachcmd));
        ctx.eval(foreachcmd);
        for (auto&& cmd : opts.commands) {
          ctx.eval(cmd);
        }
      }
      index += 1;
    }
  }
}

} // namespace

int main(int argc, char** argv)
{
  try {
    run(argc, argv);
  } catch (std::exception const& err) {
    std::cerr << "error: " << err.what() << std::endl;
    exit(EXIT_FAILURE);
  }
  return 0;
}

/* EOF */
