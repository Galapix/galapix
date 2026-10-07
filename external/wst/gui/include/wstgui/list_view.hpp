// SPDX-FileCopyrightText: 2005 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef HEADER_WINDSTILLE_GUI_LIST_VIEW_HPP
#define HEADER_WINDSTILLE_GUI_LIST_VIEW_HPP

#include "component.hpp"

namespace wstgui {

/** */
class ListView : public Component
{
private:
  struct Column {
    std::string title;
    float       width;

    Column()
      : title(),
        width()
    {}
  };

public:
  struct Item {
    std::vector<std::string> columns;

    Item(const std::string& el1,
         const std::string& el2,
         const std::string& el3)
      : columns()
    {
      columns.push_back(el1);
      columns.push_back(el2);
      columns.push_back(el3);
    }
  };

private:
  typedef std::vector<Item> Items;
  typedef std::vector<Column> Columns;
  Columns columns;
  Items   items;
  int     current_item;

public:
  ListView(Component* parent);
  ~ListView() override;

  void draw(wstdisplay::Canvas& canvas) override;
  void update(float delta, const Controller& controller) override;

  /** if width is -1 it will be automatically spaced */
  void add_column(const std::string& name, float width = -1);
  void add_item(const Item& item);

private:
  ListView (const ListView&);
  ListView& operator= (const ListView&);
};

} // namespace wstgui

#endif

/* EOF */
