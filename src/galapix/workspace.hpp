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

#ifndef HEADER_GALAPIX_GALAPIX_WORKSPACE_HPP
#define HEADER_GALAPIX_GALAPIX_WORKSPACE_HPP

#include <functional>
#include <iosfwd>
#include <memory>
#include <string>
#include <vector>

#include <entt/entity/registry.hpp>
#include <wstdisplay/fwd.hpp>

#include "galapix/components.hpp"
#include "galapix/image_tiles.hpp"
#include "galapix/layouter/layouter.hpp"
#include "math/rect.hpp"
#include "math/vector2f.hpp"
#include "util/url.hpp"

namespace galapix {

class ImageOpener;
class SizeProbeSession;

/** The Workspace houses all the images, the selection and the layout.

    Images are entities in an EnTT registry with the components of
    galapix/components.hpp; the selection is the Selected tag. The
    registry is only touched on the main thread. m_order is the display
    order (later images are drawn on top), which sorting and the
    layouters use as well. */
class Workspace final
{
public:
  Workspace();
  ~Workspace();

  // ---------------------------------------------
  // Images

  /** Add an image; \a provider may be empty while its size is unknown
      (see set_tile_provider()). */
  entt::entity add_image(URL const& url, TileProviderPtr provider);

  /** Attach the tile provider once the size is known. Keeps the image's
      displayed size where the layout put it. */
  void set_tile_provider(entt::entity image, TileProviderPtr provider);

  bool valid(entt::entity image) const { return m_registry.valid(image); }

  entt::registry& registry() { return m_registry; }
  entt::registry const& registry() const { return m_registry; }

  /** All images in display order */
  std::vector<entt::entity> const& images() const { return m_order; }

  /** Images fully inside \a rect, in display order */
  std::vector<entt::entity> get_images(Rectf const& rect) const;

  /** Topmost image at \a pos, entt::null if none */
  entt::entity get_image(Vector2f const& pos) const;

  Rectf get_image_rect(entt::entity image) const;
  URL get_url(entt::entity image) const;

  // ---------------------------------------------
  // Order and layout

  void sort();
  void sort_reverse();
  void random_shuffle();

  void layout_spiral();
  void layout_tight(float aspect_w, float aspect_h);
  void layout_aspect(float aspect_w, float aspect_h);
  void layout_vertical();
  void layout_random();
  void solve_overlaps();

  // ---------------------------------------------
  // Selection

  bool is_selected(entt::entity image) const;
  void select(entt::entity image);
  void deselect(entt::entity image);
  void select_images(std::vector<entt::entity> const& images);
  void clear_selection();
  bool selection_empty() const;

  /** Selected images in display order */
  std::vector<entt::entity> get_selection() const;
  Rectf get_selection_rect() const;
  Vector2f get_selection_center() const;

  void move_selection(Vector2f const& rel);
  /** Scale the selected images by \a factor around the selection center */
  void scale_selection(float factor);
  void set_selection_angle(float angle);

  /** Delete all images that aren't in the selection */
  void isolate_selection();

  /** Delete all images in the selection */
  void delete_selection();

  void clear();

  // ---------------------------------------------
  // Frame

  /** Visibility and tiles: visible images publish tile demand and upload
      textures, images leaving the view withdraw it. */
  void prepare_tiles(wstdisplay::Device& device, Rectf const& cliprect, float zoom);

  /** Record the visible images and the selection marks. */
  void draw(wstdisplay::Canvas& canvas, Rectf const& cliprect, float zoom);

  void update(float delta);

  /** Optional background thumtoo size probe (viewer may open before sizes are known). */
  void set_size_probe(std::shared_ptr<SizeProbeSession> probe);
  std::shared_ptr<SizeProbeSession> size_probe() const { return m_size_probe; }
  void tick_size_probe();

  // ---------------------------------------------
  // Debug and status

  /** Drop all tile cells and textures */
  void clear_cache();
  /** Give back the GPU textures of images that are not visible */
  void cache_cleanup();

  void print_info(Rectf const& rect) const;
  void print_images(Rectf const& rect) const;

  /** Aggregate tile state across all images (for status): outstanding
      requests, cells waiting for upload, cells, uploaded textures. */
  void tile_load_stats(int& out_requests, int& out_uploads, int& out_cache_entries,
                      int* out_ready_surfaces = nullptr) const;

  /** Print the tile status line of visible images that are not complete. */
  void print_tile_status(int limit = 24) const;

  /** Count visible images by tile phase (thumtoo::lod::TileSession::Phase). */
  void tile_phase_stats(int& out_loading, int& out_complete, int& out_degraded,
                        int& out_error) const;

  // ---------------------------------------------
  // Files

  /** Add the images of a .galapix workspace file, opened with \a opener,
      at their saved position and scale */
  void load(std::string const& filename, ImageOpener const& opener);
  void save(std::ostream& out) const;

  Rectf get_bounding_rect() const;

private:
  void relayout();
  void apply_layout(Layouter& layouter);
  void apply_layout(std::vector<LayoutItem> const& items);
  std::vector<LayoutItem> layout_items() const;
  void destroy_if(std::function<bool (entt::entity)> const& pred);

private:
  entt::registry m_registry;
  std::vector<entt::entity> m_order;
  LayouterPtr m_layouter;
  std::shared_ptr<SizeProbeSession> m_size_probe;

private:
  Workspace (Workspace const&) = delete;
  Workspace& operator= (Workspace const&) = delete;
};

} // namespace galapix

#endif

/* EOF */
