// SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: LGPL-3.0-or-later

#include <wstsprite/sprite_data.hpp>

#include <algorithm>
#include <charconv>
#include <format>
#include <set>
#include <stdexcept>

#include <geom/offset.hpp>
#include <geom/origin.hpp>
#include <logmich/log.hpp>
#include <prio/reader_collection.hpp>
#include <prio/reader_document.hpp>
#include <prio/reader_mapping.hpp>
#include <prio/reader_object.hpp>
#include <wstdisplay/surface.hpp>
#include <wstdisplay/surface_manager.hpp>

namespace wstsprite {

/*{ Properties */

bool
Properties::has(std::string_view key) const
{
  return m_values.find(key) != m_values.end();
}

std::vector<std::string> const*
Properties::get(std::string_view key) const
{
  auto it = m_values.find(key);
  return it != m_values.end() ? &it->second : nullptr;
}

void
Properties::set(std::string key, std::vector<std::string> values)
{
  m_values[std::move(key)] = std::move(values);
}

std::optional<std::string>
Properties::get_string(std::string_view key) const
{
  auto const* v = get(key);
  if (!v || v->empty()) { return std::nullopt; }
  return v->front();
}

std::optional<float>
Properties::get_float(std::string_view key) const
{
  auto const* v = get(key);
  if (!v || v->empty()) { return std::nullopt; }
  try {
    return std::stof(v->front());
  } catch (std::exception const&) {
    return std::nullopt;
  }
}

std::optional<int>
Properties::get_int(std::string_view key) const
{
  auto const value = get_float(key);
  if (!value) { return std::nullopt; }
  return static_cast<int>(*value);
}

std::optional<bool>
Properties::get_bool(std::string_view key) const
{
  auto const value = get_string(key);
  if (!value) { return std::nullopt; }
  if (*value == "true" || *value == "#t" || *value == "1") { return true; }
  if (*value == "false" || *value == "#f" || *value == "0") { return false; }
  return std::nullopt;
}

std::optional<glm::vec2>
Properties::get_vec2(std::string_view key) const
{
  auto const* v = get(key);
  if (!v || v->size() < 2) { return std::nullopt; }
  try {
    return glm::vec2(std::stof((*v)[0]), std::stof((*v)[1]));
  } catch (std::exception const&) {
    return std::nullopt;
  }
}

/*} */

float
Action::get_total_duration() const
{
  float total = 0.0f;
  for (auto const& frame : frames) {
    total += frame.duration;
  }
  return total;
}

namespace {

/** Read any value of \a key as strings, regardless of its type */
std::vector<std::string> read_values(prio::ReaderMapping const& mapping, std::string const& key)
{
  try {
    std::vector<std::string> strings;
    if (mapping.read(key, strings)) {
      return strings;
    }
  } catch (std::exception const&) {}

  try {
    std::vector<float> numbers;
    if (mapping.read(key, numbers)) {
      std::vector<std::string> result;
      for (float const n : numbers) {
        result.push_back(std::format("{}", n));
      }
      return result;
    }
  } catch (std::exception const&) {}

  try {
    bool value = false;
    if (mapping.read(key, value)) {
      return { value ? "true" : "false" };
    }
  } catch (std::exception const&) {}

  return {};
}

/** Copy all keys of \a mapping that aren't in \a known into \a properties */
void read_properties(prio::ReaderMapping const& mapping, std::set<std::string, std::less<>> const& known,
                     Properties& properties)
{
  for (auto const& key : mapping.get_keys()) {
    if (!known.contains(key)) {
      std::vector<std::string> values = read_values(mapping, key);
      if (!values.empty()) {
        properties.set(key, std::move(values));
      }
    }
  }
}

/** Everything the readers need besides the mapping */
struct LoadContext
{
  std::filesystem::path directory;
  wstdisplay::SurfaceManager& surfaces;
  EventRegistry const* events;
};

/** (events (NAME (frame N) ...) ...), Pingus' (at-step N) is accepted too */
void read_events(prio::ReaderMapping const& mapping, LoadContext const& ctx, Action& action)
{
  prio::ReaderCollection collection;
  if (!mapping.read("events", collection)) {
    return;
  }

  for (auto const& obj : collection.get_objects()) {
    FrameEvent event;
    event.name = obj.get_name();
    prio::ReaderMapping const event_mapping = obj.get_mapping();
    if (!event_mapping.read("frame", event.frame)) {
      event_mapping.read("at-step", event.frame);
    }
    read_properties(event_mapping, {"frame", "at-step"}, event.properties);
    if (ctx.events) {
      ctx.events->resolve(event);
    }
    action.events.push_back(std::move(event));
  }
}

geom::origin origin_from_string(std::string_view text)
{
  if (text == "top_left") { return geom::origin::TOP_LEFT; }
  if (text == "top_center") { return geom::origin::TOP_CENTER; }
  if (text == "top_right") { return geom::origin::TOP_RIGHT; }
  if (text == "center_left") { return geom::origin::CENTER_LEFT; }
  if (text == "center") { return geom::origin::CENTER; }
  if (text == "center_right") { return geom::origin::CENTER_RIGHT; }
  if (text == "bottom_left") { return geom::origin::BOTTOM_LEFT; }
  if (text == "bottom_center") { return geom::origin::BOTTOM_CENTER; }
  if (text == "bottom_right") { return geom::origin::BOTTOM_RIGHT; }
  throw std::runtime_error("unknown origin: " + std::string(text));
}

geom::fsize max_frame_size(Action const& action)
{
  float w = 0.0f;
  float h = 0.0f;
  for (auto const& frame : action.frames) {
    w = std::max(w, frame.surface.get_width());
    h = std::max(h, frame.surface.get_height());
  }
  return geom::fsize(w, h);
}

void set_frame_duration(Action& action, float duration)
{
  for (auto& frame : action.frames) {
    frame.duration = duration;
  }
}

/*{ SuperTux: (supertux-sprite (actions (action ...) ...)) */

void read_supertux_action(SpriteData& data, prio::ReaderMapping const& mapping, LoadContext const& ctx)
{
  Action action;
  if (!mapping.read("name", action.name) && !data.get_actions().empty()) {
    throw std::runtime_error("actions need names if there is more than one");
  }

  float fps = 10.0f;
  bool const has_fps = mapping.read("fps", fps);

  bool const has_loops = mapping.read("loops", action.loops);
  int loop_frame = 1;
  if (mapping.read("loop-frame", loop_frame)) {
    action.loop_frame = std::max(0, loop_frame - 1);
  }

  std::vector<float> hitbox;
  mapping.read("hitbox", hitbox);
  if (!hitbox.empty() && hitbox.size() != 2 && hitbox.size() != 4) {
    throw std::runtime_error("hitbox needs 2 or 4 values");
  }

  std::string mirror_action;
  std::string clone_action;
  std::vector<std::string> images;
  prio::ReaderCollection surfaces_collection;

  if (mapping.read("mirror-action", mirror_action)) {
    int const idx = data.find_action(mirror_action);
    if (idx < 0) {
      throw std::runtime_error("mirror-action not found (must be defined before): " + mirror_action);
    }
    Action const& src = data.get_action(idx);
    action.frames = src.frames;
    action.hflip = !src.hflip;
    if (hitbox.empty()) {
      action.hitbox = src.hitbox;
      action.origin = src.origin;
    }
    if (!has_loops) {
      action.loops = src.loops;
      action.loop_frame = src.loop_frame;
    }
    if (!has_fps) {
      action.frames = src.frames;
    } else {
      set_frame_duration(action, fps > 0.0f ? 1.0f / fps : 0.1f);
    }
  } else if (mapping.read("clone-action", clone_action)) {
    int const idx = data.find_action(clone_action);
    if (idx < 0) {
      throw std::runtime_error("clone-action not found (must be defined before): " + clone_action);
    }
    std::string const name = action.name;
    action = data.get_action(idx);
    action.name = name;
    action.events.clear();
  } else {
    if (mapping.read("images", images)) {
      for (auto const& image : images) {
        action.frames.push_back(Frame{ctx.surfaces.get(ctx.directory / image), 0.1f});
      }
    } else if (mapping.read("surfaces", surfaces_collection)) {
      for (auto const& obj : surfaces_collection.get_objects()) {
        prio::ReaderMapping const surface_mapping = obj.get_mapping();
        // (surface (file ..)) or (surface (diffuse-texture (file ..)) (displacement-texture ..)),
        // displacement textures are not supported
        std::string file;
        if (!surface_mapping.read("file", file)) {
          prio::ReaderMapping diffuse;
          if (surface_mapping.read("diffuse-texture", diffuse)) {
            diffuse.read("file", file);
          }
        }
        if (file.empty()) {
          throw std::runtime_error("surface without file");
        }
        action.frames.push_back(Frame{ctx.surfaces.get(ctx.directory / file), 0.1f});
      }
    } else {
      throw std::runtime_error("action without images: " + action.name);
    }

    set_frame_duration(action, fps > 0.0f ? 1.0f / fps : 0.1f);
  }

  if (!hitbox.empty()) {
    // SuperTux draws the frame so that the hitbox offset lands on the object position
    action.origin = geom::fpoint(hitbox[0], hitbox[1]);
    geom::fsize const size = max_frame_size(action);
    float const w = hitbox.size() == 4 ? hitbox[2] : size.width() - hitbox[0];
    float const h = hitbox.size() == 4 ? hitbox[3] : size.height() - hitbox[1];
    action.hitbox = geom::frect(geom::fpoint(hitbox[0], hitbox[1]), geom::fsize(w, h));
  } else if (!action.hitbox) {
    action.hitbox = geom::frect(max_frame_size(action));
  }

  read_events(mapping, ctx, action);
  read_properties(mapping, {"name", "fps", "loops", "loop-frame", "hitbox", "mirror-action",
                            "clone-action", "images", "surfaces", "events"}, action.properties);

  data.add_action(std::move(action));
}

void read_supertux(SpriteData& data, prio::ReaderMapping const& mapping, LoadContext const& ctx)
{
  prio::ReaderCollection actions;
  if (!mapping.read("actions", actions)) {
    throw std::runtime_error("supertux-sprite without actions");
  }

  for (auto const& obj : actions.get_objects()) {
    if (obj.get_name() == "action") {
      read_supertux_action(data, obj.get_mapping(), ctx);
    }
  }
}

/*} */

/*{ Windstille: (sprite (actions (action ...) ...)) */

void read_windstille(SpriteData& data, prio::ReaderMapping const& mapping, LoadContext const& ctx)
{
  prio::ReaderCollection actions;
  if (!mapping.read("actions", actions)) {
    throw std::runtime_error("sprite without actions");
  }

  for (auto const& obj : actions.get_objects())
  {
    if (obj.get_name() != "action") {
      continue;
    }

    prio::ReaderMapping const action_mapping = obj.get_mapping();
    Action action;
    action.name = action_mapping.get<std::string>("name");
    if (action.name.empty()) {
      throw std::runtime_error("action without name");
    }

    float speed = 1.0f; // frames per second
    action_mapping.read("speed", speed);
    action_mapping.read("scale", action.scale);

    std::vector<float> offset;
    if (action_mapping.read("offset", offset) && offset.size() == 2) {
      // Windstille adds the offset to the draw position
      action.origin = geom::fpoint(-offset[0], -offset[1]);
    }

    std::vector<std::string> images;
    prio::ReaderMapping grid;
    if (action_mapping.read("images", images)) {
      for (auto const& image : images) {
        action.frames.push_back(Frame{ctx.surfaces.get(ctx.directory / image), 0.1f});
      }
    } else if (action_mapping.read("image-grid", grid)) {
      std::string const file = grid.get<std::string>("file");
      int const x_size = grid.get<int>("x-size", -1);
      int const y_size = grid.get<int>("y-size", -1);
      if (file.empty() || x_size <= 0 || y_size <= 0) {
        throw std::runtime_error("invalid image-grid");
      }
      for (auto const& surface : ctx.surfaces.load_grid(ctx.directory / file, geom::isize(x_size, y_size))) {
        action.frames.push_back(Frame{surface, 0.1f});
      }
    }

    if (action.frames.empty()) {
      throw std::runtime_error("action without images: " + action.name);
    }

    set_frame_duration(action, speed > 0.0f ? 1.0f / speed : 1.0f);
    read_events(action_mapping, ctx, action);
    read_properties(action_mapping, {"name", "speed", "scale", "offset", "images", "image-grid", "events"},
                    action.properties);
    data.add_action(std::move(action));
  }
}

/*} */

/*{ Pingus: (pingus-sprite (image ..) (array ..) ...), a single action */

void read_pingus(SpriteData& data, prio::ReaderMapping const& mapping, LoadContext const& ctx)
{
  std::string image;
  if (!mapping.read("image", image)) {
    throw std::runtime_error("pingus-sprite without image");
  }

  wstdisplay::Surface const sheet = ctx.surfaces.get(ctx.directory / image);

  std::vector<int> array{1, 1};
  mapping.read("array", array);
  if (array.size() != 2 || array[0] <= 0 || array[1] <= 0) {
    throw std::runtime_error("invalid array");
  }

  std::vector<int> position{0, 0};
  mapping.read("position", position);

  std::vector<int> size;
  mapping.read("size", size);
  geom::fsize const frame_size = (size.size() == 2) ?
    geom::fsize(static_cast<float>(size[0]), static_cast<float>(size[1])) :
    geom::fsize(sheet.get_width() / static_cast<float>(array[0]),
                sheet.get_height() / static_cast<float>(array[1]));

  int speed = 100; // milliseconds per frame
  mapping.read("speed", speed);

  bool loop = true;
  mapping.read("loop", loop);

  Action action;
  action.name = "default";
  action.loops = loop ? -1 : 1;

  for (int i = 0; i < array[0] * array[1]; ++i) {
    geom::fpoint const pos(static_cast<float>(position.size() == 2 ? position[0] : 0) +
                           frame_size.width() * static_cast<float>(i % array[0]),
                           static_cast<float>(position.size() == 2 ? position[1] : 0) +
                           frame_size.height() * static_cast<float>(i / array[0]));
    action.frames.push_back(Frame{sheet.region(geom::frect(pos, frame_size)),
                                  static_cast<float>(speed) / 1000.0f});
  }

  // Pingus places the origin anchor of the frame plus offset at the position
  std::string origin_name = "top_left";
  mapping.read("origin", origin_name);
  geom::foffset const anchor = geom::anchor_offset(frame_size, origin_from_string(origin_name));
  std::vector<float> offset{0.0f, 0.0f};
  mapping.read("offset", offset);
  if (offset.size() == 2) {
    action.origin = geom::fpoint(-anchor.x() - offset[0], -anchor.y() - offset[1]);
  }

  read_events(mapping, ctx, action);
  read_properties(mapping, {"image", "array", "position", "size", "speed", "loop", "origin", "offset", "events"},
                  action.properties);
  data.add_action(std::move(action));
}

/*} */

} // namespace

SpriteData::SpriteData() :
  m_actions()
{
}

int
SpriteData::find_action(std::string_view name) const
{
  for (size_t i = 0; i < m_actions.size(); ++i) {
    if (m_actions[i].name == name) {
      return static_cast<int>(i);
    }
  }
  return -1;
}

SpriteData
SpriteData::from_mapping(std::string_view name, prio::ReaderMapping const& mapping,
                         std::filesystem::path const& directory, wstdisplay::SurfaceManager& surfaces,
                         EventRegistry const* events)
{
  SpriteData data;
  LoadContext const ctx{directory, surfaces, events};

  if (name == "supertux-sprite") {
    read_supertux(data, mapping, ctx);
  } else if (name == "sprite") {
    read_windstille(data, mapping, ctx);
  } else if (name == "pingus-sprite") {
    read_pingus(data, mapping, ctx);
  } else {
    throw std::runtime_error("unknown sprite format: " + std::string(name));
  }

  if (data.get_actions().empty()) {
    throw std::runtime_error("sprite without actions");
  }

  return data;
}

SpriteData
SpriteData::from_file(std::filesystem::path const& filename, wstdisplay::SurfaceManager& surfaces,
                      EventRegistry const* events)
{
  try {
    prio::ReaderDocument const doc = prio::ReaderDocument::from_file(filename);
    return from_mapping(doc.get_name(), doc.get_mapping(), filename.parent_path(), surfaces, events);
  } catch (std::exception const& err) {
    throw std::runtime_error(filename.string() + ": " + err.what());
  }
}

SpriteData
SpriteData::from_surface(wstdisplay::Surface const& surface)
{
  SpriteData data;
  Action action;
  action.name = "default";
  action.frames.push_back(Frame{surface, 1.0f});
  data.add_action(std::move(action));
  return data;
}

} // namespace wstsprite

/* EOF */
