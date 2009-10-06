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

#ifndef HEADER_GALAPIX_MATH_QUAD_TREE_HPP
#define HEADER_GALAPIX_MATH_QUAD_TREE_HPP

#include <vector>
#include <memory>
#include <boost/scoped_ptr.hpp>

#include "rect.hpp"

/** 
    +----+----+
    | nw | ne |
    |----+----+
    | sw | se |
    +----+----+
*/
template<class C>
class QuadTreeNode
{
private:
  struct Object {
    Rectf rect;
    C    data;

    Object() :
      rect(), 
      data()
    {}
  };

  typedef std::vector<Object> Items;

  Rectf    bounding_rect;
  Vector2f center;
  Items    items;
  int      depth;

  boost::scoped_ptr<QuadTreeNode<C> > nw;
  boost::scoped_ptr<QuadTreeNode<C> > ne;
  boost::scoped_ptr<QuadTreeNode<C> > sw;
  boost::scoped_ptr<QuadTreeNode<C> > se;

public:
  QuadTreeNode(int depth_, const Rectf& bounding_rect_)
    : bounding_rect(bounding_rect_),
      center(bounding_rect.get_center()),
      items(),
      depth(depth_),
      nw(0), ne(0), sw(0), se(0)
  {
  }

  void add(const Rectf& rect, const C& c)
  {
    if (depth > 8) // FIXME: max depth, don't hardcode this
      {
        Object obj;
        obj.rect = rect;
        obj.data = c;
        items.push_back(obj);
      }
    else
      {
        if ((rect.left   < center.x && rect.right  > center.x) ||
            (rect.top    < center.y && rect.bottom > center.x))
          { // Rectf doesn't fit in any quadrant
            Object obj;
            obj.rect = rect;
            obj.data = c;
            items.push_back(obj);
          }
        else
          {
            if (rect.right <= center.x)
              { // west
                if(rect.top >= center.y)
                  { // south
                    if (!sw.get())
                      sw.reset(new QuadTreeNode(depth+1, Rectf(bounding_rect.left, bounding_rect.top,
                                                                                       center.x, center.y)));
                    sw->add(rect, c);
                  }
                else // (rect.top < center.y)
                  { // north
                    if (!nw.get())
                      nw.reset(new QuadTreeNode(depth+1, Rectf(bounding_rect.left, bounding_rect.top,
                                                                                       center.x, center.y)));
                    nw->add(rect, c);
                  }
              }
            else // (rect.right > center.x)
              { // east
                if(rect.top >= center.y)
                  { // south
                    if (!se.get())
                      se.reset(new QuadTreeNode(depth+1, Rectf(bounding_rect.left, bounding_rect.top,
                                                                                       center.x, center.y)));
                    se->add(rect, c);
                  }
                else // (rect.top < center.y)
                  { // north
                    if (!ne.get())
                      ne.reset(new QuadTreeNode(depth+1, Rectf(bounding_rect.left, bounding_rect.top,
                                                                                       center.x, center.y)));
                    ne->add(rect, c);
                  }
              }
          }
      }
  }

  void get_items_at(const Rectf& rect, std::vector<C>& out_items) const
  {
    // Check all overlapping items
    for(typename Items::const_iterator i = items.begin(); i != items.end(); ++i)
      {
        if (i->rect.is_overlapped(rect))
          {
            out_items.push_back(i->data);
          }
      }

    // If rect overlaps with the given quadrant, recursivly check the quadrant
    if (nw.get() && 
        rect.left < center.x &&
        rect.top  < center.y)
      nw->get_items_at(rect, out_items);
    
    if (ne.get() && 
        rect.right > center.x &&
        rect.top   < center.y)
      ne->get_items_at(rect, out_items);
    
    if (sw.get() &&
        rect.left   < center.x &&
        rect.bottom > center.y)
      sw->get_items_at(rect, out_items);

    if (se.get() &&
        rect.right  > center.x &&
        rect.bottom > center.y)
      se->get_items_at(rect, out_items);
  }
};

template<class C>
class QuadTree 
{
private:
  boost::scoped_ptr<QuadTreeNode<C> > main_node;

public: 
  QuadTree(const Rectf& bounding_rect) :
    main_node(new QuadTreeNode<C>(0, bounding_rect))
  {
  }

  void add(const Rectf& rect, const C& c)
  {
    main_node->add(rect, c);
  }

  std::vector<C> get_items_at(const Rectf& rect)
  {
    std::vector<C> items;
    main_node->get_items_at(rect, items);
    return items;
  }
};

#endif

/* EOF */
