/*
**  Galapix - an image viewer for large image collections
**  Copyright (C) 2008 Ingo Ruhnke <grumbel@gmx.de>
**
**  This program is free software: you can redistribute it and/or modify
**  it under the terms of the GNU General Public License as published by
**  the Free Software Foundation, either version 3 of the License, or
**  (at your option) any later version.
**
**  This program is distributed in the hope that it will be useful,
**  but WITHOUT ANY WARRANTY; without even the implied warranty of
**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**  GNU General Public License for more details.
**
**  You should have received a copy of the GNU General Public License
**  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "galapix/workspace.hpp"

#include <algorithm>
#include <iostream>

#include "database/file_entry.hpp"
#include "galapix/database_thread.hpp"
#include "galapix/layouter/random_layouter.hpp"
#include "galapix/layouter/regular_layouter.hpp"
#include "galapix/layouter/spiral_layouter.hpp"
#include "galapix/layouter/tight_layouter.hpp"
#include "util/file_reader.hpp"
#include "util/log.hpp"
#include "util/string_util.hpp"

Workspace::Workspace() :
  m_images(),
  m_selection(Selection::create()),
  m_layouter()
{
}

ImageCollection
Workspace::get_images(const Rectf& rect) const
{
  ImageCollection result;
  for(const auto& i: m_images)
  {
    if (rect.contains(i->get_image_rect()))
    {
      result.add(i);
    }
  }
  return result;
}

WorkspaceItemPtr
Workspace::get_image(const Vector2f& pos) const
{
  for(ImageCollection::const_reverse_iterator i = m_images.rbegin(); i != m_images.rend(); ++i)
  {

    if ((*i)->get_image_rect().contains(pos))
    {
      return *i;
    }
  }
  return WorkspaceItemPtr();
}

void
Workspace::add_image(const WorkspaceItemPtr& image)
{
  m_images.add(image);  
}

void
Workspace::layout_vertical()
{
  // FIXME: Move this into it's own Layouter class
  float spacing = 10.0f;
  Vector2f next_pos(0.0f, 0.0f);
  for(auto& i: m_images)
  {
    i->set_scale(1.0f);
    i->set_pos(next_pos);
    next_pos.y += static_cast<float>(i->get_original_height()) + spacing;
  }
}

void
Workspace::layout_aspect(float aspect_w, float aspect_h)
{
  m_layouter.reset(new RegularLayouter(aspect_w, aspect_h));
  relayout();
}

void
Workspace::layout_spiral()
{
  m_layouter.reset(new SpiralLayouter());
  relayout();
}

void
Workspace::layout_tight(float aspect_w, float aspect_h)
{
  m_layouter.reset(new TightLayouter(aspect_w, aspect_h));
  relayout();
}

void
Workspace::layout_random()
{
  m_layouter.reset(new RandomLayouter);
  relayout();
}

void
Workspace::draw(const Rectf& cliprect, float zoom)
{
  for(auto& i: m_images)
  {
    if (i->get_image_rect().is_overlapped(cliprect))
    {  
      if (!i->is_visible())
      {
        i->on_enter_screen();
      }

      i->draw(cliprect, zoom);
    }
    else
    {
      if (i->is_visible())
      {
        i->on_leave_screen();
      }
    }
  }

  for(auto& i: *m_selection)
  {
    i->draw_mark();
  }
}

void
Workspace::update(float delta)
{
}

void
Workspace::relayout()
{
  if (m_layouter)
  {
    m_layouter->layout(m_images);
  } 
}

void
Workspace::sort()
{
  std::sort(m_images.begin(), m_images.end(), 
            [](const WorkspaceItemPtr& lhs, const WorkspaceItemPtr& rhs) {
              return StringUtil::numeric_less(lhs->get_url().str(), rhs->get_url().str());
            });
}

void
Workspace::sort_reverse()
{
  std::sort(m_images.rbegin(), m_images.rend(), 
            [](const WorkspaceItemPtr& lhs, const WorkspaceItemPtr& rhs) {
              return StringUtil::numeric_less(lhs->get_url().str(), rhs->get_url().str());
            });
  relayout();
}

void
Workspace::random_shuffle()
{
  std::random_shuffle(m_images.begin(), m_images.end());
  relayout();
}

void
Workspace::clear_cache()
{
  for(auto& i: m_images)
  {
    i->clear_cache();
  }  
}

void
Workspace::cache_cleanup()
{
  for(auto& i: m_images)
  {
    i->cache_cleanup();
  }   
}

void
Workspace::print_info(const Rectf& rect)
{
  std::cout << "-------------------------------------------------------" << std::endl;
  std::cout << "Workspace Info:" << std::endl;
  for(auto& img: get_images(rect))
  {
    img->print_info();
  }
  std::cout << "  Number of Images: " << m_images.size() << std::endl;
  std::cout << "-------------------------------------------------------" << std::endl;
}

void
Workspace::print_images(const Rectf& rect)
{
  std::cout << "-- Visible images --------------------------------------" << std::endl;
  for(auto& img: get_images(rect))
  {
    std::cout << img->get_url() << " "
              << img->get_original_width() << "x" << img->get_original_height()
              << std::endl;
  }
  std::cout << "--------------------------------------------------------" << std::endl;
}

void
Workspace::select_images(const ImageCollection& lst)
{
  m_selection->clear();
  m_selection->add_images(lst);
}

bool
Workspace::selection_clicked(const Vector2f& pos) const
{
  for(auto& i: *m_selection)
  {
    if (i->get_image_rect().contains(pos))
      return true;
  }
  return false;
}

void
Workspace::clear_selection()
{
  m_selection->clear();
}

void
Workspace::clear()
{
  m_selection->clear();
  m_images.clear();
}

void
Workspace::move_selection(const Vector2f& rel)
{
  for(auto& i: *m_selection)
  {
    i->set_pos(i->get_pos() + rel);
  }
}

void
Workspace::isolate_selection()
{
  m_images = m_selection->get_images();
  m_selection->clear();
}

void
Workspace::delete_selection()
{
  m_images.erase(std::remove_if(m_images.begin(), m_images.end(), 
                                [this](const WorkspaceItemPtr& image)->bool{
                                  for(auto& i: *m_selection)
                                  {
                                    if (i == image)
                                      return true;
                                  }
                                  return false;
                                }),
                 m_images.end());
  m_selection->clear();
}

void
Workspace::solve_overlaps()
{
  // FIXME: This function is extremely slow and doesn't work
  // dynamically (no animation on position setting)
  int num_overlappings = 1;

  while(num_overlappings)
  {
    num_overlappings = 0;
    // Use QuadTree to make this fast
    for(auto i = m_images.begin(); i != m_images.end(); ++i)
    {
      for(auto j = i+1; j != m_images.end(); ++j)
      {
        Rectf irect = (*i)->get_image_rect();
        Rectf jrect = (*j)->get_image_rect();

        if (irect.is_overlapped(jrect))
        {
          num_overlappings += 1;
                  
          Rectf clip = irect.clip_to(jrect);

          // FIXME: This only works if one rect isn't completly within the other
          if (clip.get_width() > clip.get_height())
          {
            (*i)->set_pos((*i)->get_pos() - Vector2f(0.0f, clip.get_height()/2 + 16.0f));
            (*j)->set_pos((*j)->get_pos() + Vector2f(0.0f, clip.get_height()/2 + 16.0f));
          }
          else
          {
            (*i)->set_pos((*i)->get_pos() - Vector2f(clip.get_width()/2 + 16.0f, 0.0f));
            (*j)->set_pos((*j)->get_pos() + Vector2f(clip.get_width()/2 + 16.0f, 0.0f));
          }
        }
      }
    }
    std::cout << "NumOverlappings: " << num_overlappings << std::endl; 
  }
}

void
Workspace::save(std::ostream& out)
{
  // FIXME: Rewrite with FileWriter
  out << "(galapix-workspace\n"
      << "  (images\n";
  
  for(const auto& i: m_images)
  {
    // FIXME: Must escape the filename!
    out  << "    (image (url   \"" << i->get_url() << "\")\n"
         << "           (pos   " << i->get_pos().x << " " << i->get_pos().y << ")\n"
         << "           (scale " << i->get_scale() << ")"
      // << "           (angle " << i->get_angle() << ")"
      // << "           (alpha " << i->get_alpha() << ")"
         << ")\n";
  }

  out << "  ))\n\n";
  out << ";; EOF ;;" << std::endl;
}

void
Workspace::load(const std::string& filename)
{
  FileReader reader = FileReader::parse(filename);
  
  if (reader.get_name() != "galapix-workspace")
  {
    std::cout << "Error: Unknown file format: " << reader.get_name() << std::endl;      
  }
  else
  {
    //clear();

    std::vector<FileReader> image_sections = reader.read_section("images").get_sections();

    for(auto& i: image_sections)
    {
      if (i.get_name() == "image")
      {
        URL      url;
        Vector2f pos;
        float    scale = 0.5f;

        i.read_url("url", url);
        i.read_vector2f("pos", pos);
        i.read_float("scale", scale);

        std::cout << url << " " << pos << " " << scale << std::endl;

        ImagePtr image(new Image(url));
        image->set_pos(pos);
        image->set_scale(scale);
        add_image(WorkspaceItemPtr(new Image(url)));
      }
    }
  }
}

Rectf
Workspace::get_bounding_rect() const
{
  if (m_images.empty())
  {
    return Rectf();
  }
  else
  {
    Rectf rect = m_images.front()->get_image_rect();

    for(ImageCollection::const_iterator i = m_images.begin()+1; i != m_images.end(); ++i)
    {
      const Rectf& image_rect = (*i)->get_image_rect(); 
          
      if (isnan(image_rect.left) ||
          isnan(image_rect.right) ||
          isnan(image_rect.top) ||
          isnan(image_rect.bottom))
      {
        std::cout << "NAN ERROR: " << (*i)->get_url() << " " << (*i)->get_pos() << " " << image_rect << std::endl;
        //assert(0);
      }
      else
      {
        rect.left   = Math::min(rect.left,   image_rect.left);
        rect.right  = Math::max(rect.right,  image_rect.right);
        rect.top    = Math::min(rect.top,    image_rect.top);
        rect.bottom = Math::max(rect.bottom, image_rect.bottom);
      }
    }
  
    return rect;
  }
}

/* EOF */
