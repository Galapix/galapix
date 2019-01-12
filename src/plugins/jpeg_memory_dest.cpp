/*
**  Galapix - an image viewer for large image collections
**  Copyright (C) 2008 Ingo Ruhnke <grumbel@gmail.com>
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

#include "plugins/jpeg_memory_dest.hpp"

#define OUTPUT_BUF_SIZE 4096

namespace {

struct jpeg_memory_destination_mgr
{
  struct jpeg_destination_mgr pub;

  JOCTET buffer[OUTPUT_BUF_SIZE];
  std::vector<uint8_t>* data;
};

void jpeg_memory_init_destination(j_compress_ptr cinfo)
{
  struct jpeg_memory_destination_mgr* mgr = (struct jpeg_memory_destination_mgr*)cinfo->dest;

  cinfo->dest->next_output_byte = mgr->buffer;
  cinfo->dest->free_in_buffer   = OUTPUT_BUF_SIZE;
}

boolean jpeg_memory_empty_output_buffer(j_compress_ptr cinfo)
{
  struct jpeg_memory_destination_mgr* mgr = (struct jpeg_memory_destination_mgr*)cinfo->dest;

  // This function always gets OUTPUT_BUF_SIZE bytes,
  // cinfo->dest->free_in_buffer *must* be ignored
  for(size_t i = 0; i < OUTPUT_BUF_SIZE; ++i)
  { // FIXME: Little slow maybe?
    mgr->data->push_back(mgr->buffer[i]);
  }

  cinfo->dest->next_output_byte = mgr->buffer;
  cinfo->dest->free_in_buffer   = OUTPUT_BUF_SIZE;

  return TRUE;
}

void jpeg_memory_term_destination(j_compress_ptr cinfo)
{
  struct jpeg_memory_destination_mgr* mgr = (struct jpeg_memory_destination_mgr*)cinfo->dest;
  size_t datacount = OUTPUT_BUF_SIZE - cinfo->dest->free_in_buffer;

  for(size_t i = 0; i < datacount; ++i)
  { // FIXME: Little slow maybe?
    mgr->data->push_back(mgr->buffer[i]);
  }
}

} // namespace

void jpeg_memory_dest(j_compress_ptr cinfo, std::vector<uint8_t>* data)
{
  if (cinfo->dest == nullptr)
  {     /* first time for this JPEG object? */
    cinfo->dest = (struct jpeg_destination_mgr*)
      (*cinfo->mem->alloc_small)((j_common_ptr) cinfo, JPOOL_PERMANENT,
                                 sizeof(struct jpeg_memory_destination_mgr));
  }

  cinfo->dest->init_destination    = jpeg_memory_init_destination;
  cinfo->dest->empty_output_buffer = jpeg_memory_empty_output_buffer;
  cinfo->dest->term_destination    = jpeg_memory_term_destination;

  struct jpeg_memory_destination_mgr* mgr = (struct jpeg_memory_destination_mgr*)cinfo->dest;
  mgr->data = data;
}

/* EOF */
