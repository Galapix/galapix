// SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef HEADER_WSTSYSTEM_SIGNAL_HPP
#define HEADER_WSTSYSTEM_SIGNAL_HPP

#include <algorithm>
#include <cstdint>
#include <functional>
#include <memory>
#include <utility>
#include <vector>

namespace wstsystem {

namespace detail {

struct SignalStateBase
{
  virtual ~SignalStateBase() = default;
  virtual void disconnect(uint64_t id) = 0;
};

} // namespace detail

/** Returned by Signal::connect(), disconnects the slot when asked.
    Discarding it leaves the slot connected. It doesn't keep the
    signal alive and does nothing once the signal is gone. */
class Connection final
{
public:
  Connection() = default;
  Connection(std::weak_ptr<detail::SignalStateBase> state, uint64_t id) :
    m_state(std::move(state)),
    m_id(id)
  {}

  void disconnect()
  {
    if (auto state = m_state.lock()) {
      state->disconnect(m_id);
    }
    m_state.reset();
  }

private:
  std::weak_ptr<detail::SignalStateBase> m_state = {};
  uint64_t m_id = 0;
};

template<typename Signature>
class Signal;

/** A list of callbacks that are called in the order they were
    connected. Slots connected or disconnected while the signal is
    emitted take effect with the next emission. */
template<typename... Args>
class Signal<void (Args...)> final
{
public:
  using Slot = std::function<void (Args...)>;

public:
  Signal() :
    m_state(std::make_shared<State>())
  {}

  Signal(Signal&&) noexcept = default;
  Signal& operator=(Signal&&) noexcept = default;

  Connection connect(Slot slot)
  {
    uint64_t const id = ++m_state->next_id;
    m_state->slots.push_back(Entry{id, std::make_shared<Slot>(std::move(slot))});
    return Connection(m_state, id);
  }

  void operator()(Args... args) const
  {
    // a copy, so slots can change the connections while being called
    std::vector<Entry> const slots = m_state->slots;
    for (Entry const& entry : slots) {
      (*entry.slot)(args...);
    }
  }

  void clear() { m_state->slots.clear(); }
  bool empty() const { return m_state->slots.empty(); }
  size_t size() const { return m_state->slots.size(); }

private:
  struct Entry
  {
    uint64_t id;
    std::shared_ptr<Slot> slot;
  };

  struct State final : public detail::SignalStateBase
  {
    std::vector<Entry> slots = {};
    uint64_t next_id = 0;

    void disconnect(uint64_t id) override
    {
      std::erase_if(slots, [id](Entry const& entry) { return entry.id == id; });
    }
  };

  std::shared_ptr<State> m_state;

public:
  Signal(Signal const&) = delete;
  Signal& operator=(Signal const&) = delete;
};

} // namespace wstsystem

#endif

/* EOF */
