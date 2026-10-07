// SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

/// Linux XDG / Freedesktop thumbnail cache + optional Thumbnailer1 D-Bus client.
///
/// Separate from thumtoo's durable ladder / tile pipeline (Store, Client tiles).
/// Hosts such as dirtoo can use this for desktop-standard thumbnails without
/// coupling to Galapix-style 256² JPEG pyramids.
///
/// Specs:
///   https://specifications.freedesktop.org/thumbnail-spec/latest/
///   org.freedesktop.thumbnails.Thumbnailer1 (session bus)
///
/// Cache lookup always works. Queue()/signals require THUMTOO_HAVE_DBUS=1
/// (libdbus-1) and a running provider (e.g. tumbler).

#include "thumtoo/executor.hpp"

#include <cstdint>
#include <filesystem>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace thumtoo {

/// Freedesktop thumbnail "flavor" (subdirectory under thumbnails/).
enum class XdgThumbnailFlavor {
  Normal,    ///< 128 px (normal/)
  Large,     ///< 256 px (large/) — default for most UIs
  XLarge,    ///< 512 px (x-large/)
  XXLarge,   ///< 1024 px (xx-large/)
};

[[nodiscard]] const char* xdg_thumbnail_flavor_name(XdgThumbnailFlavor flavor) noexcept;

/// Absolute file:// URI with proper percent-encoding (MD5 key material).
[[nodiscard]] std::string xdg_file_uri(const std::filesystem::path& absolute_file);

/// Expected on-disk PNG path for @p absolute_file (may not exist yet).
[[nodiscard]] std::filesystem::path xdg_thumbnail_cache_path(
    const std::filesystem::path& absolute_file,
    XdgThumbnailFlavor flavor = XdgThumbnailFlavor::Large);

/// True if cache PNG exists and matches source Thumb::MTime / Thumb::Size
/// (or mtime fallback when chunks are missing).
[[nodiscard]] bool xdg_thumbnail_cache_is_fresh(
    const std::filesystem::path& absolute_file,
    const std::filesystem::path& cache_png);

/// Fresh cache hit, or nullopt.
[[nodiscard]] std::optional<std::filesystem::path> xdg_thumbnail_lookup(
    const std::filesystem::path& absolute_file,
    XdgThumbnailFlavor flavor = XdgThumbnailFlavor::Large);

/// Delete normal/large/x-large/xx-large/fail cache entries for @p file.
/// @return number of files removed.
[[nodiscard]] int xdg_thumbnail_remove_cache(const std::filesystem::path& absolute_file);

/// Built with libdbus-1 (Thumbnailer1 Queue available).
[[nodiscard]] constexpr bool xdg_thumbnail_dbus_built() noexcept {
#if defined(THUMTOO_HAVE_DBUS) && THUMTOO_HAVE_DBUS
  return true;
#else
  return false;
#endif
}

/// Result of a request (cache hit or daemon-generated).
struct XdgThumbnailReply {
  std::string uri;                         ///< file:// key that was requested
  std::optional<std::filesystem::path> path;  ///< PNG if ready
  std::string error;                       ///< non-empty on failure
  bool from_cache = false;
};

using XdgThumbnailCallback = std::function<void(XdgThumbnailReply reply)>;

/**
 * Session-bus client for org.freedesktop.thumbnails.Thumbnailer1.
 *
 * Not thread-safe for concurrent use of one instance. Callbacks run via the
 * supplied Executor (default: inline). Does not touch Client / Store / tiles.
 */
class XdgThumbnailer {
 public:
  explicit XdgThumbnailer(Executor executor = {});
  ~XdgThumbnailer();

  XdgThumbnailer(const XdgThumbnailer&) = delete;
  XdgThumbnailer& operator=(const XdgThumbnailer&) = delete;

  /// True if the Thumbnailer1 name is on the session bus (or was activated).
  [[nodiscard]] bool service_available() const;

  /**
   * Look up cache; on miss Queue() generation (when D-Bus is built).
   * @p mime_type e.g. "image/jpeg"; "application/octet-stream" is accepted.
   * @p force removes existing cache entries first.
   */
  void request(const std::filesystem::path& absolute_file,
               std::string mime_type = "application/octet-stream",
               XdgThumbnailFlavor flavor = XdgThumbnailFlavor::Large,
               XdgThumbnailCallback cb = {}, bool force = false);

  void request_many(const std::vector<std::filesystem::path>& files,
                    const std::vector<std::string>& mime_types,
                    XdgThumbnailFlavor flavor = XdgThumbnailFlavor::Large,
                    XdgThumbnailCallback cb = {}, bool force = false);

  /// Dequeue outstanding handles and drop pending callbacks.
  void cancel_all();

  /**
   * Dispatch D-Bus traffic until idle or @p timeout_ms elapses.
   * Hosts with an event loop should call this periodically; CLI can call
   * request_wait instead.
   */
  void process_events(int timeout_ms = 50);

  /// request() + process_events until reply or timeout (milliseconds).
  [[nodiscard]] XdgThumbnailReply request_wait(
      const std::filesystem::path& absolute_file,
      std::string mime_type = "application/octet-stream",
      XdgThumbnailFlavor flavor = XdgThumbnailFlavor::Large, int timeout_ms = 15000,
      bool force = false);

 private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};

}  // namespace thumtoo
