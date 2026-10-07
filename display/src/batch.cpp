// SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: LGPL-3.0-or-later

#include <wstdisplay/batch.hpp>

#include <algorithm>
#include <numeric>
#include <tuple>

namespace wstdisplay {

namespace {

constexpr size_t max_segment_vertices = 0x10000;

/** Order of commands within an unordered layer, groups equal state */
auto state_key(Canvas::Command const& cmd)
{
  return std::make_tuple(cmd.type, cmd.space, cmd.clip, cmd.material, cmd.texture, cmd.blend);
}

bool same_state(Batch const& batch, Canvas::Command const& cmd)
{
  return batch.type == CommandType::Geometry &&
    batch.texture == cmd.texture &&
    batch.material == cmd.material &&
    batch.blend == cmd.blend &&
    batch.clip == cmd.clip &&
    batch.space == cmd.space;
}

} // namespace

void
BatchList::clear()
{
  vertices.clear();
  indices.clear();
  batches.clear();
  order.clear();
  unordered.clear();
}

void
build_batches(Canvas const& canvas, BatchList& out)
{
  out.clear();

  auto const& commands = canvas.get_commands();
  auto const& vertices = canvas.get_vertices();
  auto const& indices = canvas.get_indices();

  out.order.resize(commands.size());
  std::iota(out.order.begin(), out.order.end(), 0u);

  // look up the layer mode once per command instead of in the comparator
  out.unordered.resize(commands.size());
  bool any_unordered = false;
  for (size_t i = 0; i < commands.size(); ++i) {
    bool const unordered = (i > 0 && commands[i].z == commands[i - 1].z) ?
      (out.unordered[i - 1] != 0) :
      canvas.is_layer_unordered(commands[i].z);
    out.unordered[i] = unordered ? 1 : 0;
    any_unordered |= unordered;
  }

  auto const by_z = [&commands](uint32_t a, uint32_t b) {
    return commands[a].z < commands[b].z;
  };

  // all commands with equal z share the layer mode, which keeps this
  // a strict weak ordering
  auto const by_z_and_state = [&commands, &out](uint32_t a, uint32_t b) {
    if (commands[a].z != commands[b].z) {
      return commands[a].z < commands[b].z;
    } else if (out.unordered[a]) {
      return state_key(commands[a]) < state_key(commands[b]);
    } else {
      return false;
    }
  };

  if (any_unordered) {
    std::stable_sort(out.order.begin(), out.order.end(), by_z_and_state);
  } else if (!std::is_sorted(out.order.begin(), out.order.end(), by_z)) {
    std::stable_sort(out.order.begin(), out.order.end(), by_z);
  }

  out.vertices.reserve(vertices.size());
  out.indices.reserve(indices.size());

  size_t segment_base = 0;
  for (uint32_t const idx : out.order)
  {
    Canvas::Command const& cmd = commands[idx];

    if (cmd.type != CommandType::Geometry) {
      out.batches.push_back(Batch{
          .texture = cmd.texture,
          .material = cmd.material,
          .object = cmd.object,
          .base_vertex = 0,
          .first_index = 0,
          .index_count = 0,
          .clip = cmd.clip,
          .blend = cmd.blend,
          .space = cmd.space,
          .type = cmd.type
        });
      continue;
    }

    if (cmd.index_count == 0) {
      continue;
    }

    // start a new segment when 16-bit indices would overflow
    if (out.vertices.size() - segment_base + cmd.vertex_count > max_segment_vertices) {
      segment_base = out.vertices.size();
    }

    uint32_t const local_base = static_cast<uint32_t>(out.vertices.size() - segment_base);
    uint32_t const first_index = static_cast<uint32_t>(out.indices.size());

    out.vertices.insert(out.vertices.end(),
                        vertices.begin() + cmd.first_vertex,
                        vertices.begin() + cmd.first_vertex + cmd.vertex_count);

    for (uint32_t i = 0; i < cmd.index_count; ++i) {
      out.indices.push_back(static_cast<uint16_t>(local_base + indices[cmd.first_index + i]));
    }

    if (!out.batches.empty() &&
        same_state(out.batches.back(), cmd) &&
        out.batches.back().base_vertex == segment_base &&
        out.batches.back().first_index + out.batches.back().index_count == first_index)
    {
      out.batches.back().index_count += cmd.index_count;
    }
    else
    {
      out.batches.push_back(Batch{
          .texture = cmd.texture,
          .material = cmd.material,
          .object = 0,
          .base_vertex = static_cast<uint32_t>(segment_base),
          .first_index = first_index,
          .index_count = cmd.index_count,
          .clip = cmd.clip,
          .blend = cmd.blend,
          .space = cmd.space,
          .type = CommandType::Geometry
        });
    }
  }
}

} // namespace wstdisplay

/* EOF */
