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

#include "workspace.hpp"

#include <boost/bind.hpp>

#include "database_thread.hpp"
#include "math/rect.hpp"
#include "math/quad_tree.hpp"
#include "math.hpp"
#include "file_entry.hpp"
#include "file_reader.hpp"
#include "display/framebuffer.hpp"

Workspace::Workspace()
{
  next_pos = Vector2i(0, 0);
  row_width = 20;
  progress = 0.0f;
}

std::vector<Image>
Workspace::get_images(const Rectf& rect) const
{
  std::vector<Image> result;
  for(Images::const_iterator i = images.begin(); i != images.end(); ++i)
    {
      if (rect.contains(i->get_image_rect()))
        {
          result.push_back(*i);
        }
    }  
  return result;
}

Image
Workspace::get_image(const Vector2f& pos) const
{
  for(Images::const_reverse_iterator i = images.rbegin(); i != images.rend(); ++i)
    {
      if (i->overlaps(pos))
        return *i;
    }  
  return Image();
}

void
Workspace::add_image(const URL& url, const Vector2f& pos, float scale)
{
  DatabaseThread::current()->request_file(url, boost::bind(&Workspace::receive_file, this, _1));
  image_requests.push_back(ImageRequest(url, pos, scale));
}

void
Workspace::add_image(const FileEntry& file_entry, const Vector2f& pos, float scale)
{
  Image image(file_entry);
  images.push_back(image);
  image.set_scale(scale);
  image.set_pos(pos);
}

struct ImageRequestFinder {
  std::string str;

  ImageRequestFinder(const std::string& str_)
    : str(str_)
  {}

  bool operator()(const ImageRequest& lhs) const {
    return str == lhs.url.str();
  }
};

void
Workspace::add_image(const FileEntry& file_entry)
{
  Image image(file_entry);
  images.push_back(image);

  if (!image_requests.empty())
    {
      ImageRequests::iterator i = std::find_if(image_requests.begin(), image_requests.end(), 
                                               ImageRequestFinder(file_entry.get_url().str()));
      if (i != image_requests.end())
        {
          image.set_pos(i->pos);
          image.set_scale(i->scale);
          return;
        }
    }

  {
    image.set_scale(Math::min(1000.0f / static_cast<float>(file_entry.get_width()),
                              1000.0f / static_cast<float>(file_entry.get_height())));

    image.set_pos(next_pos * 1024.0f);

    // FIXME: Ugly, instead we should relayout once a second or so
    next_pos.x += 1;
    if (next_pos.x >= row_width)
      {
        next_pos.x  = 0;
        next_pos.y += 1;
      }
  }
}

void
Workspace::layout_vertical()
{
  float spacing = 10.0f;
  next_pos = Vector2f(0.0f, 0.0f);
  for(Images::iterator i = images.begin(); i != images.end(); ++i)
    {
      i->set_target_scale(1.0f);
      i->set_target_pos(next_pos);
      next_pos.y += static_cast<float>(i->get_original_height()) + spacing;
    }
  
  progress = 0.0f;
}

void
Workspace::layout_aspect(float aspect_w, float aspect_h)
{
  if (!images.empty())
    {
      int w = int(Math::sqrt(aspect_w * static_cast<float>(images.size()) / aspect_h));
      
      row_width = w;

      for(int i = 0; i < int(images.size()); ++i)
        {
          float target_scale = Math::min(1000.0f / static_cast<float>(images[i].get_original_width()),
                                         1000.0f / static_cast<float>(images[i].get_original_height()));

          images[i].set_target_scale(target_scale);

          if ((i/w) % 2 == 0)
            {
              images[i].set_target_pos(Vector2f(static_cast<float>(i % w) * 1024.0f,
                                                static_cast<float>(i / w) * 1024.0f));
            }
          else
            {
              images[i].set_target_pos(Vector2f(static_cast<float>(w - (i % w)-1) * 1024.0f,
                                                static_cast<float>(i / w)         * 1024.0f));
            }

          next_pos = Vector2i(i % w, i / w);
        }
    }

  progress = 0.0f;
}

void
Workspace::layout_tight(float aspect_w, float aspect_h)
{
  float spacing = 24.0f;

  float width = 0;  
  // calculate the total width 
  for(Images::iterator i = images.begin(); i != images.end(); ++i)
    {
      const float scale = (1000.0f + spacing) / static_cast<float>(i->get_original_height());
      width += static_cast<float>(i->get_original_width()) * scale;
    }

  width /= Math::sqrt(width / ((aspect_w / aspect_h) * (1000.0f + spacing)));

  Vector2f pos(0.0f, 0.0f);
  Vector2f last_pos(0.0f, 0.0f);
  bool go_right = true;
  
  for(Images::iterator i = images.begin(); i != images.end(); ++i)
    {
      Image& image = *i;

      const float scale = 1000.0f / static_cast<float>(image.get_original_height());
      image.set_target_scale(scale);

      if (go_right)
        {
          if (pos.x + (static_cast<float>(image.get_original_width())*scale) > width)
            {
              pos.x = last_pos.x;
              pos.y += 1000.0f + spacing;   
              
              go_right = false;

              last_pos = pos;
              image.set_target_pos(pos + Vector2f(static_cast<float>(image.get_original_width()),
                                                  static_cast<float>(image.get_original_height())) * scale / 2.0f);
            }
          else
            {
              last_pos = pos;
              image.set_target_pos(pos + Vector2f(static_cast<float>(image.get_original_width()),
                                                  static_cast<float>(image.get_original_height())) * scale / 2.0f);
            
              pos.x += static_cast<float>(image.get_original_width()) * scale + spacing;
            }
        }
      else
        {
          if (pos.x - (static_cast<float>(image.get_original_width()) * scale) < 0)
            {
              //pos.x = 0;
              pos.y += 1000.0f + spacing;   
              go_right = true;

              last_pos = pos;
              image.set_target_pos(pos + Vector2f(static_cast<float>(image.get_original_width()),
                                                  static_cast<float>(image.get_original_height())) * scale / 2.0f);

              pos.x += static_cast<float>(image.get_original_width()) * scale + spacing;
            }
          else
            {
              pos.x -= static_cast<float>(image.get_original_width()) * scale + spacing;

              last_pos = pos;
              image.set_target_pos(pos + Vector2f(static_cast<float>(image.get_original_width()),
                                                  static_cast<float>(image.get_original_height())) * scale / 2.0f);
            }
        }
    }

  // Cause a smooth relayout
  progress = 0.0f;
}

void
Workspace::layout_random()
{
  int width = static_cast<int>(Math::sqrt(float(images.size())) * 1500.0f);
  for(Images::iterator i = images.begin(); i != images.end(); ++i)
    {
      i->set_target_pos(Vector2f(static_cast<float>(rand()%width), static_cast<float>(rand()%width)));
      i->set_target_scale(static_cast<float>(rand()%1000) / 1000.0f + 0.25f); // FIXME: Make this relative to image size
    }
  progress = 0.0f;
}

void
Workspace::build_quad_tree()
{
  if (!images.empty())
    {
      Rectf rect = get_bounding_rect();

      std::cout << "QuadTree: " << rect << std::endl;

      quad_tree = std::auto_ptr<QuadTree<Image> >(new QuadTree<Image>(rect));
      
      for(Images::iterator i = images.begin(); i != images.end(); ++i)
        {
          quad_tree->add(i->get_image_rect(), *i);
        }
    }
}

void
Workspace::clear_quad_tree()
{
  quad_tree = std::auto_ptr<QuadTree<Image> >();
}

void
Workspace::draw(const Rectf& cliprect, float scale)
{
  if (quad_tree.get())
    {
      Images current_images = quad_tree->get_items_at(cliprect);
      for(Images::iterator i = current_images.begin(); i != current_images.end(); ++i)
        {
          i->draw(cliprect, scale);
        }
    }
  else
    {
      //std::cout << Math::clamp(1, static_cast<int>(1.0f / scale), 32) << " -> " << scale << std::endl;
        
      for(Images::iterator i = images.begin(); i != images.end(); ++i)
        {
          i->draw(cliprect, scale);
        }
    }

  for(Selection::iterator i = selection.begin(); i != selection.end(); ++i)
    {
      i->draw_mark();
    }
}

void
Workspace::update(float delta)
{
  while (!file_queue.empty())
    {
      const FileEntry& entry = file_queue.front();
      add_image(entry);
      file_queue.pop();
    }

  if (progress != 1.0f)
    {
      progress += delta * 2.0f;

      if (progress > 1.0f)
        progress = 1.0f;

      for(Images::iterator i = images.begin(); i != images.end(); ++i)
        {
          i->update_pos(progress);
        }
    }
}

struct ImageSorter
{
  bool operator()(const Image& lhs, const Image& rhs)
  {
    return lhs.get_url() < rhs.get_url();
  }
};

void
Workspace::sort()
{
  std::sort(images.begin(), images.end(), ImageSorter());
}

void
Workspace::random_shuffle()
{
  std::random_shuffle(images.begin(), images.end());
}

void
Workspace::clear_cache()
{
  for(Images::iterator i = images.begin(); i != images.end(); ++i)
    {
      i->clear_cache();
    }  
}

void
Workspace::cache_cleanup()
{
  for(Images::iterator i = images.begin(); i != images.end(); ++i)
    {
      i->cache_cleanup();
    }   
}

void
Workspace::print_info(const Rectf& rect)
{
  std::cout << "-------------------------------------------------------" << std::endl;
  std::cout << "Workspace Info:" << std::endl;
  for(Images::iterator i = images.begin(); i != images.end(); ++i)
  {
    if (i->overlaps(rect))
    {
      i->print_info();
      std::cout << std::endl;
    }
  }
  std::cout << "  Number of Images: " << images.size() << std::endl;
  std::cout << "-------------------------------------------------------" << std::endl;
}

void
Workspace::print_images(const Rectf& rect)
{
  std::cout << "-- Visible images --------------------------------------" << std::endl;
  for(Images::iterator i = images.begin(); i != images.end(); ++i)
    {
      if (i->overlaps(rect))
        {
          std::cout << i->get_url() << " "
                    << i->get_original_width() << "x" << i->get_original_height()
                    << std::endl;
        }
    }
  std::cout << "--------------------------------------------------------" << std::endl;
}

void
Workspace::set_row_width(int w)
{
  row_width = w;
}

void
Workspace::select_images(const std::vector<Image>& lst)
{
  selection.clear();
  selection.add_images(lst);
}

bool
Workspace::selection_clicked(const Vector2f& pos) const
{
  for(Selection::const_iterator i = selection.begin(); i != selection.end(); ++i)
    {
      if (i->overlaps(pos))
        return true;
    }
  return false;
}

void
Workspace::clear_selection()
{
  selection.clear();
}

void
Workspace::clear()
{
  clear_quad_tree();
  selection.clear();
  images.clear();
}

void
Workspace::move_selection(const Vector2f& rel)
{
  for(Selection::iterator i = selection.begin(); i != selection.end(); ++i)
    {
      i->set_pos(i->get_pos() + rel);
    }
}

void
Workspace::isolate_selection()
{
  images = selection.get_images();
  selection.clear();
}

struct ImagesMemberOf
{
  Selection selection;

  ImagesMemberOf(const Selection& selection_)
    : selection(selection_)
  {}

  bool operator()(const Image& image)
  {
    for(Selection::iterator i = selection.begin(); i != selection.end(); ++i)
      {
        if (*i == image)
          return true;
      }
    return false;
  }
};

void
Workspace::delete_selection()
{
  images.erase(std::remove_if(images.begin(), images.end(), ImagesMemberOf(selection)),
               images.end());
  selection.clear();
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
      for(std::vector<Image>::iterator i = images.begin(); i != images.end(); ++i)
        {
          for(std::vector<Image>::iterator j = i+1; j != images.end(); ++j)
            {
              Rectf irect = i->get_image_rect();
              Rectf jrect = j->get_image_rect();

              if (irect.is_overlapped(jrect))
                {
                  num_overlappings += 1;
                  
                  Rectf clip = irect.clip_to(jrect);

                  // FIXME: This only works if one rect isn't completly within the other
                  if (clip.get_width() > clip.get_height())
                    {
                      i->set_pos(i->get_pos() - Vector2f(0.0f, clip.get_height()/2 + 16.0f));
                      j->set_pos(j->get_pos() + Vector2f(0.0f, clip.get_height()/2 + 16.0f));
                    }
                  else
                    {
                      i->set_pos(i->get_pos() - Vector2f(clip.get_width()/2 + 16.0f, 0.0f));
                      j->set_pos(j->get_pos() + Vector2f(clip.get_width()/2 + 16.0f, 0.0f));
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
  
  for(Images::iterator i = images.begin(); i != images.end(); ++i)
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

      for(std::vector<FileReader>::iterator i = image_sections.begin(); i != image_sections.end(); ++i)
        {
          if (i->get_name() == "image")
            {
              URL      url;
              Vector2f pos;
              float    scale = 0.5f;

              i->read_url("url", url);
              i->read_vector2f("pos", pos);
              i->read_float("scale", scale);

              std::cout << url << " " << pos << " " << scale << std::endl;

              add_image(url, pos, scale);
            }
        }
    }
}

Rectf
Workspace::get_bounding_rect() const
{
  if (images.empty())
    {
      return Rectf();
    }
  else
    {
      Rectf rect = images.front().get_image_rect();

      std::cout << images.front().get_url() << " " << images.front().get_pos() << " " 
                << images.front().get_scale() << " "
                << images.front().get_scaled_width() << " "
                << images.front().get_scaled_height()
                << std::endl;

      for(Images::const_iterator i = images.begin()+1; i != images.end(); ++i)
        {
          const Rectf& image_rect = i->get_image_rect(); 
          
          rect.left   = Math::min(rect.left,   image_rect.left);
          rect.right  = Math::max(rect.right,  image_rect.right);
          rect.top    = Math::min(rect.top,    image_rect.top);
          rect.bottom = Math::max(rect.bottom, image_rect.bottom);

          if (isnan(rect.left) ||
              isnan(rect.right) ||
              isnan(rect.top) ||
              isnan(rect.bottom))
            {
              std::cout << i->get_url() << " " << i->get_pos() << " " << image_rect << std::endl;
              assert(0);
            }
        }
  
      return rect;
    }
}

bool
Workspace::is_animated() const
{
  return progress != 1.0f;
}

void
Workspace::receive_file(const FileEntry& entry)
{
  file_queue.push(entry);
}

/* EOF */
