// SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef HEADER_WSTDISPLAY_RESOURCE_POOL_HPP
#define HEADER_WSTDISPLAY_RESOURCE_POOL_HPP

#include <cstdint>
#include <memory>
#include <vector>

#include "handle.hpp"

namespace wstdisplay {

/** Storage for objects of one type that are referred to by Handle<T>,
    used by Device and usable for other handle based managers. Objects
    are heap allocated so references stay valid while other objects
    are inserted. destroy() only marks an object, it is deleted and
    its handles become stale on the next collect(). */
template<typename T>
class ResourcePool final
{
public:
  ResourcePool() :
    m_slots(),
    m_free(),
    m_pending(),
    m_count(0)
  {}

  Handle<T> insert(std::unique_ptr<T> object)
  {
    uint32_t index;
    if (!m_free.empty()) {
      index = m_free.back();
      m_free.pop_back();
    } else {
      index = static_cast<uint32_t>(m_slots.size());
      m_slots.push_back(Slot{});
    }

    Slot& slot = m_slots[index];
    slot.object = std::move(object);
    m_count += 1;
    return Handle<T>{index, slot.generation};
  }

  T* find(Handle<T> handle) const
  {
    if (handle.index < m_slots.size()) {
      Slot const& slot = m_slots[handle.index];
      if (slot.generation == handle.generation) {
        return slot.object.get();
      }
    }
    return nullptr;
  }

  void destroy(Handle<T> handle)
  {
    if (find(handle) == nullptr) {
      return;
    }

    Slot& slot = m_slots[handle.index];
    if (!slot.pending) {
      slot.pending = true;
      m_pending.push_back(handle.index);
    }
  }

  /** Mark all objects for deletion, unlike clear() their handles are
      reliably stale after the next collect() */
  void destroy_all()
  {
    for (size_t index = 0; index < m_slots.size(); ++index) {
      Slot& slot = m_slots[index];
      if (slot.object && !slot.pending) {
        slot.pending = true;
        m_pending.push_back(static_cast<uint32_t>(index));
      }
    }
  }

  /** Delete the pending objects, returns true if anything was deleted */
  bool collect()
  {
    if (m_pending.empty()) {
      return false;
    }

    // deleting an object can destroy further resources, e.g. a
    // framebuffer its texture, so work on a copy of the list
    std::vector<uint32_t> pending;
    pending.swap(m_pending);

    for (uint32_t const index : pending) {
      Slot& slot = m_slots[index];
      std::unique_ptr<T> object = std::move(slot.object);
      slot.pending = false;
      slot.generation += 1;
      if (slot.generation == 0) {
        slot.generation = 1;
      }
      m_free.push_back(index);
      m_count -= 1;
      object.reset();
    }
    return true;
  }

  /** Delete everything and forget the generations, only for the
      final shutdown as old handles may match new objects afterwards */
  void clear()
  {
    // see collect(), objects may destroy other objects while going away
    std::vector<Slot> slots;
    slots.swap(m_slots);
    slots.clear();
    m_free.clear();
    m_pending.clear();
    m_count = 0;
  }

  size_t count() const { return m_count; }

private:
  struct Slot
  {
    std::unique_ptr<T> object = {};
    /** Starts at 1, a handle with generation 0 is null */
    uint32_t generation = 1;
    bool pending = false;
  };

  std::vector<Slot> m_slots;
  std::vector<uint32_t> m_free;
  std::vector<uint32_t> m_pending;
  size_t m_count;

public:
  ResourcePool(ResourcePool const&) = delete;
  ResourcePool& operator=(ResourcePool const&) = delete;
};

} // namespace wstdisplay

#endif

/* EOF */
