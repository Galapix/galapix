// Galapix - an image viewer for large image collections
// Copyright (C) 2008-2026 Ingo Ruhnke <grumbel@gmail.com>
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

#include "galapix/workspace.hpp"

#include <algorithm>
#include <iostream>
#include <random>

#include <logmich/log.hpp>
#include <strut/numeric_less.hpp>
#include <surf/color.hpp>
#include <wstdisplay/canvas.hpp>

#include "galapix/image_opener.hpp"
#include "galapix/layouter/overlap_solver.hpp"
#include "galapix/layouter/random_layouter.hpp"
#include "galapix/layouter/regular_layouter.hpp"
#include "galapix/layouter/spiral_layouter.hpp"
#include "galapix/layouter/tight_layouter.hpp"
#include "galapix/size_probe_session.hpp"
#include "util/reader.hpp"

namespace galapix {

namespace {

Rectf
union_rect(Rectf const& a, Rectf const& b)
{
  return geom::frect(std::min(a.left(),   b.left()),
                     std::min(a.top(),    b.top()),
                     std::max(a.right(),  b.right()),
                     std::max(a.bottom(), b.bottom()));
}

/** S-expression string literal */
std::string
quoted(std::string const& text)
{
  std::string out = "\"";
  for (char const c : text) {
    if (c == '"' || c == '\\') {
      out += '\\';
    }
    out += c;
  }
  out += '"';
  return out;
}

} // namespace

Workspace::Workspace() :
  m_registry(),
  m_order(),
  m_layouter(),
  m_size_probe()
{
}

Workspace::~Workspace()
{
}

// ---------------------------------------------------------------------------
// Images

entt::entity
Workspace::add_image(URL const& url, TileProviderPtr provider)
{
  entt::entity const image = m_registry.create();
  m_registry.emplace<Source>(image, url);
  m_registry.emplace<Transform>(image);
  m_registry.emplace<ImageSize>(image);
  m_order.push_back(image);
  set_tile_provider(image, std::move(provider));
  return image;
}

void
Workspace::set_tile_provider(entt::entity image, TileProviderPtr provider)
{
  auto& transform = m_registry.get<Transform>(image);
  auto& size = m_registry.get<ImageSize>(image);

  float const old_size = static_cast<float>(std::max(size.width, size.height));

  m_registry.remove<Tiles>(image);
  if (provider) {
    size = ImageSize{provider->get_size().width(), provider->get_size().height()};
    auto tiles = std::make_unique<ImageTiles>(provider);
    m_registry.emplace<Tiles>(image, std::move(provider), std::move(tiles));
  } else {
    size = ImageSize{};
  }

  if (m_registry.all_of<ScaleIsNative>(image)) {
    m_registry.remove<ScaleIsNative>(image);
    return;
  }

  // Keep the displayed size (only really right for regular layouts)
  float const new_size = static_cast<float>(std::max(size.width, size.height));
  transform.scale *= old_size / new_size;
}

std::vector<entt::entity>
Workspace::get_images(Rectf const& rect) const
{
  std::vector<entt::entity> result;
  for (entt::entity const image : m_order) {
    if (geom::contains(rect, get_image_rect(image))) {
      result.push_back(image);
    }
  }
  return result;
}

entt::entity
Workspace::get_image(Vector2f const& pos) const
{
  for (auto it = m_order.rbegin(); it != m_order.rend(); ++it) {
    if (geom::contains(get_image_rect(*it), geom::fpoint(pos))) {
      return *it;
    }
  }
  return entt::null;
}

Rectf
Workspace::get_image_rect(entt::entity image) const
{
  auto const [transform, size] = m_registry.get<Transform, ImageSize>(image);
  return image_rect(transform, size);
}

URL
Workspace::get_url(entt::entity image) const
{
  return m_registry.get<Source>(image).url;
}

// ---------------------------------------------------------------------------
// Order and layout

std::vector<LayoutItem>
Workspace::layout_items() const
{
  std::vector<LayoutItem> items;
  items.reserve(m_order.size());
  for (entt::entity const image : m_order) {
    auto const [transform, size] = m_registry.get<Transform, ImageSize>(image);
    items.push_back(LayoutItem{size, transform});
  }
  return items;
}

void
Workspace::apply_layout(std::vector<LayoutItem> const& items)
{
  for (size_t i = 0; i < m_order.size(); ++i) {
    m_registry.get<Transform>(m_order[i]) = items[i].transform;
  }
}

void
Workspace::apply_layout(Layouter& layouter)
{
  std::vector<LayoutItem> items = layout_items();
  layouter.layout(items);
  apply_layout(items);
}

void
Workspace::relayout()
{
  if (m_layouter) {
    apply_layout(*m_layouter);
  }
}

void
Workspace::sort()
{
  std::sort(m_order.begin(), m_order.end(),
            [this](entt::entity lhs, entt::entity rhs) {
              return strut::numeric_less(get_url(lhs).str(), get_url(rhs).str());
            });
  relayout();
}

void
Workspace::sort_reverse()
{
  std::sort(m_order.rbegin(), m_order.rend(),
            [this](entt::entity lhs, entt::entity rhs) {
              return strut::numeric_less(get_url(lhs).str(), get_url(rhs).str());
            });
  relayout();
}

void
Workspace::random_shuffle()
{
  std::shuffle(m_order.begin(), m_order.end(), std::mt19937{std::random_device{}()});
  relayout();
}

void
Workspace::layout_vertical()
{
  // FIXME: Move this into it's own Layouter class
  float const spacing = 10.0f;
  Vector2f next_pos(0.0f, 0.0f);
  std::vector<LayoutItem> items = layout_items();
  for (LayoutItem& item : items) {
    item.transform.scale = 1.0f;
    item.transform.pos = next_pos;
    next_pos = Vector2f(next_pos.x(),
                        next_pos.y() + static_cast<float>(item.size.height) + spacing);
  }
  apply_layout(items);
}

void
Workspace::layout_aspect(float aspect_w, float aspect_h)
{
  m_layouter = std::make_shared<RegularLayouter>(aspect_w, aspect_h);
  relayout();
}

void
Workspace::layout_spiral()
{
  m_layouter = std::make_shared<SpiralLayouter>();
  relayout();
}

void
Workspace::layout_tight(float aspect_w, float aspect_h)
{
  m_layouter = std::make_shared<TightLayouter>(aspect_w, aspect_h);
  relayout();
}

void
Workspace::layout_random()
{
  m_layouter = std::make_shared<RandomLayouter>();
  relayout();
}

void
Workspace::solve_overlaps()
{
  std::vector<LayoutItem> items = layout_items();
  solve_image_overlaps(items, /*gap=*/16.0f);
  apply_layout(items);
}

// ---------------------------------------------------------------------------
// Selection

bool
Workspace::is_selected(entt::entity image) const
{
  return m_registry.all_of<Selected>(image);
}

void
Workspace::select(entt::entity image)
{
  m_registry.emplace_or_replace<Selected>(image);
}

void
Workspace::deselect(entt::entity image)
{
  m_registry.remove<Selected>(image);
}

void
Workspace::select_images(std::vector<entt::entity> const& images)
{
  clear_selection();
  for (entt::entity const image : images) {
    select(image);
  }
}

void
Workspace::clear_selection()
{
  m_registry.clear<Selected>();
}

bool
Workspace::selection_empty() const
{
  return m_registry.view<Selected>().empty();
}

std::vector<entt::entity>
Workspace::get_selection() const
{
  std::vector<entt::entity> result;
  for (entt::entity const image : m_order) {
    if (is_selected(image)) {
      result.push_back(image);
    }
  }
  return result;
}

Rectf
Workspace::get_selection_rect() const
{
  std::vector<entt::entity> const selection = get_selection();
  if (selection.empty()) {
    return {};
  }

  Rectf rect = get_image_rect(selection.front());
  for (entt::entity const image : selection) {
    rect = union_rect(rect, get_image_rect(image));
  }
  return rect;
}

Vector2f
Workspace::get_selection_center() const
{
  auto view = m_registry.view<Selected, Transform const>();
  Vector2f pos(0.0f, 0.0f);
  int count = 0;
  for (auto [image, transform] : view.each()) {
    pos = pos.as_vec() + transform.pos.as_vec();
    count += 1;
  }
  return count == 0 ? Vector2f() : Vector2f(pos.as_vec() / static_cast<float>(count));
}

void
Workspace::move_selection(Vector2f const& rel)
{
  for (auto [image, transform] : m_registry.view<Selected, Transform>().each()) {
    transform.pos = transform.pos.as_vec() + rel.as_vec();
  }
}

void
Workspace::scale_selection(float factor)
{
  Vector2f const center = get_selection_center();
  for (auto [image, transform] : m_registry.view<Selected, Transform>().each()) {
    transform.scale *= factor;
    transform.pos = center.as_vec() + (transform.pos.as_vec() - center.as_vec()) * factor;
  }
}

void
Workspace::set_selection_angle(float angle)
{
  for (auto [image, transform] : m_registry.view<Selected, Transform>().each()) {
    transform.angle = angle;
  }
}

void
Workspace::destroy_if(std::function<bool (entt::entity)> const& pred)
{
  std::vector<entt::entity> kept;
  kept.reserve(m_order.size());
  for (entt::entity const image : m_order) {
    if (pred(image)) {
      m_registry.destroy(image);
    } else {
      kept.push_back(image);
    }
  }
  m_order.swap(kept);
}

void
Workspace::isolate_selection()
{
  destroy_if([this](entt::entity image) { return !is_selected(image); });
  clear_selection();
}

void
Workspace::delete_selection()
{
  destroy_if([this](entt::entity image) { return is_selected(image); });
}

void
Workspace::clear()
{
  m_registry.clear();
  m_order.clear();
}

// ---------------------------------------------------------------------------
// Frame

void
Workspace::prepare_tiles(wstdisplay::Device& device, Rectf const& cliprect, float zoom)
{
  for (entt::entity const image : m_order)
  {
    Rectf const rect = get_image_rect(image);
    Tiles* const tiles = m_registry.try_get<Tiles>(image);

    if (geom::intersects(rect, cliprect))
    {
      m_registry.emplace_or_replace<Visible>(image);
      if (tiles) {
        tiles->tiles->update(device, rect, m_registry.get<Transform>(image).scale, cliprect, zoom);
      }
    }
    else if (m_registry.all_of<Visible>(image))
    {
      m_registry.remove<Visible>(image);
      if (tiles) {
        tiles->tiles->hide();
      }
    }
  }
}

void
Workspace::draw(wstdisplay::Canvas& canvas, Rectf const& cliprect, float zoom)
{
  for (entt::entity const image : m_order)
  {
    if (!m_registry.all_of<Visible>(image)) {
      continue;
    }

    Rectf const rect = get_image_rect(image);
    if (Tiles const* tiles = m_registry.try_get<Tiles>(image)) {
      tiles->tiles->draw(canvas, rect, m_registry.get<Transform>(image).scale, zoom);
    } else {
      // size not known yet
      canvas.fill_rect(rect, surf::Color::from_rgb888(255, 255, 0));
    }
  }

  for (entt::entity const image : get_selection()) {
    canvas.draw_rect(get_image_rect(image), surf::Color::from_rgb888(255, 255, 255), 1.0f / zoom);
  }
}

void
Workspace::update(float delta)
{
}

void
Workspace::set_size_probe(std::shared_ptr<SizeProbeSession> probe)
{
  m_size_probe = std::move(probe);
}

void
Workspace::tick_size_probe()
{
  if (m_size_probe) {
    m_size_probe->tick(*this);
  }
}

// ---------------------------------------------------------------------------
// Debug and status

void
Workspace::clear_cache()
{
  for (auto [image, tiles] : m_registry.view<Tiles>().each()) {
    tiles.tiles->clear();
  }
}

void
Workspace::cache_cleanup()
{
  for (auto [image, tiles] : m_registry.view<Tiles>(entt::exclude<Visible>).each()) {
    tiles.tiles->release_textures();
  }
}

void
Workspace::print_info(Rectf const& rect) const
{
  std::cout << "-------------------------------------------------------" << std::endl;
  std::cout << "Workspace Info:" << std::endl;
  for (entt::entity const image : get_images(rect)) {
    Tiles const* tiles = m_registry.try_get<Tiles>(image);
    std::cout << "  Image: " << get_url(image) << std::endl;
    std::cout << "    " << (tiles ? tiles->tiles->status_line() : std::string("no size yet")) << std::endl;
  }
  std::cout << "  Number of Images: " << m_order.size() << std::endl;
  std::cout << "-------------------------------------------------------" << std::endl;
}

void
Workspace::print_images(Rectf const& rect) const
{
  std::cout << "-- Visible images --------------------------------------" << std::endl;
  for (entt::entity const image : get_images(rect)) {
    ImageSize const& size = m_registry.get<ImageSize>(image);
    std::cout << get_url(image) << " " << size.width << "x" << size.height << std::endl;
  }
  std::cout << "--------------------------------------------------------" << std::endl;
}

void
Workspace::print_tile_status(int limit) const
{
  std::cout << "=== tiles (visible, not complete) ===\n";
  int n = 0;
  for (entt::entity const image : m_order) {
    Tiles const* tiles = m_registry.try_get<Tiles>(image);
    if (!tiles || !m_registry.all_of<Visible>(image)) {
      continue;
    }
    if (tiles->tiles->stats().phase == thumtoo::lod::TileSession::Phase::Complete) {
      continue;
    }
    if (n++ >= limit) {
      std::cout << "  ...\n";
      break;
    }
    std::cout << "  " << get_url(image) << ": " << tiles->tiles->status_line() << "\n";
  }
}

void
Workspace::tile_load_stats(int& out_requests, int& out_uploads, int& out_cache_entries,
                           int* out_ready_surfaces) const
{
  out_requests = 0;
  out_uploads = 0;
  out_cache_entries = 0;
  int ready = 0;
  for (auto [image, tiles] : m_registry.view<Tiles const>().each()) {
    ImageTiles::Stats const stats = tiles.tiles->stats();
    out_requests += stats.queued;
    out_uploads += stats.pending_uploads;
    out_cache_entries += stats.cells;
    ready += stats.textures;
  }
  if (out_ready_surfaces) {
    *out_ready_surfaces = ready;
  }
}

void
Workspace::tile_phase_stats(int& out_loading, int& out_complete, int& out_degraded,
                            int& out_error) const
{
  using Phase = thumtoo::lod::TileSession::Phase;

  out_loading = 0;
  out_complete = 0;
  out_degraded = 0;
  out_error = 0;
  for (auto [image, tiles] : m_registry.view<Tiles const, Visible const>().each()) {
    switch (tiles.tiles->stats().phase) {
      case Phase::Idle: break;
      case Phase::Loading: ++out_loading; break;
      case Phase::Complete: ++out_complete; break;
      case Phase::Degraded: ++out_degraded; break;
      case Phase::Error: ++out_error; break;
    }
  }
}

// ---------------------------------------------------------------------------
// Files

void
Workspace::save(std::ostream& out) const
{
  // FIXME: Rewrite with FileWriter
  out << "(galapix-workspace\n"
      << "  (images\n";

  for (entt::entity const image : m_order)
  {
    Transform const& transform = m_registry.get<Transform>(image);
    out  << "    (image (url   " << quoted(get_url(image).str()) << ")\n"
         << "           (pos   " << transform.pos.x() << " " << transform.pos.y() << ")\n"
         << "           (scale " << transform.scale << ")"
         << ")\n";
  }

  out << "  ))\n\n";
  out << ";; EOF ;;" << std::endl;
}

void
Workspace::load(std::string const& filename, ImageOpener const& opener)
{
  auto doc = ReaderDocument::from_file(filename);

  if (doc.get_name() != "galapix-workspace")
  {
    std::cout << "Error: Unknown file format: " << doc.get_name() << std::endl;
    return;
  }

  ReaderCollection collection;
  doc.get_mapping().read("images", collection);

  for (auto& i: collection.get_objects())
  {
    if (i.get_name() == "image")
    {
      ReaderMapping image_reader = i.get_mapping();

      URL      url;
      Vector2f pos;
      float    scale = 0.5f;

      image_reader.read("url", url);
      image_reader.read("pos", pos);
      image_reader.read("scale", scale);

      entt::entity const image = opener.open(*this, url);
      if (image != entt::null) {
        Transform& transform = m_registry.get<Transform>(image);
        transform.pos = pos;
        transform.scale = scale;
        if (!m_registry.all_of<Tiles>(image)) {
          // size probe still running
          m_registry.emplace<ScaleIsNative>(image);
        }
      }
    }
  }
}

Rectf
Workspace::get_bounding_rect() const
{
  if (m_order.empty()) {
    return {};
  }

  Rectf rect = get_image_rect(m_order.front());
  for (entt::entity const image : m_order) {
    rect = union_rect(rect, get_image_rect(image));
  }
  return rect;
}

} // namespace galapix

/* EOF */
