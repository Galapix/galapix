// Galapix - an image viewer for large image collections
// Copyright (C) 2008-2019 Ingo Ruhnke <grumbel@gmail.com>
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

#include "plugins/jpeg_memory_src.hpp"

#include <jerror.h>

namespace {

struct jpeg_memory_source_mgr {
  struct jpeg_source_mgr pub;   /* public fields */

  const uint8_t* data;
  int      len;
};

void jpeg_memory_init_source(j_decompress_ptr cinfo)
{
  cinfo->src->next_input_byte = nullptr;
  cinfo->src->bytes_in_buffer = 0;
}

void jpeg_memory_term_source(j_decompress_ptr)
{
  // nothing to do destruct the source
}

boolean jpeg_memory_fill_input_buffer(j_decompress_ptr cinfo)
{
  if (cinfo->src->next_input_byte != nullptr)
  {
    (cinfo)->err->msg_code = JERR_INPUT_EOF;
    (*(cinfo)->err->error_exit)(reinterpret_cast<j_common_ptr>(cinfo));
    return FALSE;
  }
  else
  {
    struct jpeg_memory_source_mgr* mgr = reinterpret_cast<struct jpeg_memory_source_mgr*>(cinfo->src);

    cinfo->src->next_input_byte = mgr->data;
    cinfo->src->bytes_in_buffer = static_cast<size_t>(mgr->len);

    return TRUE;
  }
}

void jpeg_memory_skip_input_data(j_decompress_ptr cinfo, long num_bytes)
{
  cinfo->src->next_input_byte = cinfo->src->next_input_byte + static_cast<unsigned long>(num_bytes);
  cinfo->src->bytes_in_buffer = cinfo->src->bytes_in_buffer - static_cast<unsigned long>(num_bytes);

  struct jpeg_memory_source_mgr* mgr = reinterpret_cast<struct jpeg_memory_source_mgr*>(cinfo->src);

  if (cinfo->src->next_input_byte >= &mgr->data[mgr->len])
  {
    (cinfo)->err->msg_code = JERR_INPUT_EOF;
    (*(cinfo)->err->error_exit)(reinterpret_cast<j_common_ptr>(cinfo));
  }
}

} // namespace

void jpeg_memory_src(j_decompress_ptr cinfo, const uint8_t* data, int len)
{
  if (cinfo->src == nullptr)
  {
    cinfo->src = static_cast<struct jpeg_source_mgr*>((*cinfo->mem->alloc_small)(reinterpret_cast<j_common_ptr>(cinfo),
                                                                      JPOOL_PERMANENT,
                                                                      sizeof(struct jpeg_memory_source_mgr)));
  }

  cinfo->src->init_source       = jpeg_memory_init_source;
  cinfo->src->fill_input_buffer = jpeg_memory_fill_input_buffer;
  cinfo->src->skip_input_data   = jpeg_memory_skip_input_data;
  cinfo->src->resync_to_restart = jpeg_resync_to_restart; /* use default method */
  cinfo->src->term_source       = jpeg_memory_term_source;

  cinfo->src->bytes_in_buffer = 0; /* forces fill_input_buffer on first read */
  cinfo->src->next_input_byte = nullptr; /* until buffer loaded */

  struct jpeg_memory_source_mgr* mgr = reinterpret_cast<struct jpeg_memory_source_mgr*>(cinfo->src);
  mgr->data = data;
  mgr->len  = len;
}

/* EOF */
