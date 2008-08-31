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

#include <iostream>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <jpeglib.h>
#include <setjmp.h>
#include "math/size.hpp"
#include "jpeg_memory_src.hpp"
#include "jpeg_memory_dest.hpp"
#include "jpeg.hpp"

jmp_buf setjmp_buffer;

void fatal_error_handler(j_common_ptr cinfo)
{
  std::cout << "Some jpeg error: " << std::endl;
  longjmp(setjmp_buffer, 1);
}

bool
JPEG::get_size(const std::string& filename, Size& size)
{
  FILE* in = fopen(filename.c_str(), "rb");
  if (!in)
    {
      // throw std::runtime_error("JPEG::get_size: Couldn't open " + filename);
      return false;
    }
  else
    {
      struct jpeg_decompress_struct  cinfo;
      struct jpeg_error_mgr jerr;

      cinfo.err = jpeg_std_error(&jerr);
      cinfo.err->error_exit = &fatal_error_handler;
      jpeg_create_decompress(&cinfo);
      jpeg_stdio_src(&cinfo, in);

      if (setjmp(setjmp_buffer))
        {
          return false;
        }
      else
        {
          jpeg_read_header(&cinfo, FALSE);

          size.width  = cinfo.image_width;
          size.height = cinfo.image_height;

          jpeg_destroy_decompress(&cinfo);

          fclose(in);

          return true;
        }
    }
}

SoftwareSurface
JPEG::load_from_file(const std::string& filename, int scale)
{
  assert(scale == 1 ||
         scale == 2 ||
         scale == 4 ||
         scale == 8);

  //std::cout << "-- JPEG::load(" << filename << ")" << std::endl;

  FILE* in = fopen(filename.c_str(), "rb");
  if (!in)
    throw std::runtime_error("JPEG::load_from_file: Couldn't open " + filename);

  struct jpeg_decompress_struct  cinfo;
  struct jpeg_error_mgr jerr;

  cinfo.err = jpeg_std_error(&jerr);
  cinfo.err->error_exit = &fatal_error_handler;

  jpeg_create_decompress(&cinfo);
  jpeg_stdio_src(&cinfo, in);

  if (setjmp(setjmp_buffer))
    {
      throw std::runtime_error("JPEG::load_from_file: ERROR: Couldn't open " + filename);
    }

  jpeg_read_header(&cinfo, FALSE);

  if (scale != 1)
    { // scale the image down by scale

      // by default all those values below are on 1
      cinfo.scale_num           = 1;
      cinfo.scale_denom         = scale;
   
      cinfo.do_fancy_upsampling = FALSE; /* TRUE=apply fancy upsampling */
      cinfo.do_block_smoothing  = FALSE; /* TRUE=apply interblock smoothing */
    }

  jpeg_start_decompress(&cinfo);

  SoftwareSurface surface(Size(cinfo.output_width,
                               cinfo.output_height));
  
  if (cinfo.output_components == 3)
    { // RGB Image
      JSAMPLE* scanlines[cinfo.output_height];

      for(JDIMENSION y = 0; y < cinfo.output_height; ++y)
        scanlines[y] = surface.get_row_data(y);

      while (cinfo.output_scanline < cinfo.output_height) 
        {
          jpeg_read_scanlines(&cinfo, &scanlines[cinfo.output_scanline], 
                              cinfo.output_height - cinfo.output_scanline);
        }
    }
  else if (cinfo.output_components == 1)
    { // Greyscale Image
      
    }
  else
    {
      std::ostringstream str;
      str << "JPEG: Unsupported color depth: " << cinfo.output_components;
      throw std::runtime_error(str.str());
    }

  jpeg_destroy_decompress(&cinfo);

  fclose(in); 

  //std::cout << "-- done" << std::endl;

  return surface;
}

SoftwareSurface
JPEG::load_from_mem(uint8_t* mem, int len)
{
  //std::cout << "JPEG::load(" << static_cast<void*>(mem) << ", " << len << ")" << std::endl;
  
  // -- Setup the read source
  struct jpeg_decompress_struct  cinfo;
  struct jpeg_error_mgr jerr;

  cinfo.err = jpeg_std_error(&jerr);
  cinfo.err->error_exit = &fatal_error_handler;

  jpeg_create_decompress(&cinfo);
  jpeg_memory_src(&cinfo, mem, len);

  if (setjmp(setjmp_buffer)) // FIXME: Is that error handling correct that way?
    {
      throw std::runtime_error("JPEG::load");
    }

  // -- Start Reading
  jpeg_read_header(&cinfo, FALSE);
  jpeg_start_decompress(&cinfo);

  // -- Copy the scanlines to a SoftwareSurface
  SoftwareSurface surface(Size(cinfo.image_width, 
                               cinfo.image_height));
  
  //std::cout << surface.get_width() << "x" << surface.get_height() << std::endl;

  if (cinfo.output_components == 3) // RGB Image
    { 
      JSAMPLE* scanlines[cinfo.image_height];

      for(JDIMENSION y = 0; y < cinfo.image_height; ++y)
        scanlines[y] = surface.get_row_data(y);

      while (cinfo.output_scanline < cinfo.output_height) 
        {
          jpeg_read_scanlines(&cinfo, &scanlines[cinfo.output_scanline], 
                              cinfo.image_height - cinfo.output_scanline);
        }
    }
  else if (cinfo.output_components == 1) // Greyscale Image
    { 
      assert(!"JPEG::load: grayscale handling not implemented");
    }
  else
    {
      std::ostringstream str;
      str << "JPEG: Unsupported color depth: " << cinfo.output_components;
      throw std::runtime_error(str.str());
    }

  // -- Cleanup
  jpeg_destroy_decompress(&cinfo);

  return surface; 
}

void
JPEG::save(const SoftwareSurface& surface, int quality, const std::string& filename)
{
  assert(!"Unfinished: JPEG::save(SoftwareSurface& surface, int quality, const std::string& filename)");

  struct jpeg_compress_struct cinfo;
  struct jpeg_error_mgr jerr;

  cinfo.err = jpeg_std_error(&jerr);

  jpeg_create_compress(&cinfo);

  //jpeg_stdio_dest(&cinfo, outfile);

  cinfo.image_width  = surface.get_width();
  cinfo.image_height = surface.get_height();

  cinfo.input_components = 3;		/* # of color components per pixel */
  cinfo.in_color_space   = JCS_RGB; 	/* colorspace of input image */

  jpeg_set_defaults(&cinfo);
  jpeg_set_quality(&cinfo, quality, TRUE /* limit to baseline-JPEG values */);
 
  jpeg_start_compress(&cinfo, TRUE);

  JSAMPROW row_pointer[surface.get_height()];
  
  for(int y = 0; y < surface.get_height(); ++y)
    row_pointer[y] = static_cast<JSAMPLE*>(surface.get_row_data(y));

  while(cinfo.next_scanline < cinfo.image_height)
    {
      jpeg_write_scanlines(&cinfo, row_pointer, surface.get_height());
    }
  
  jpeg_finish_compress(&cinfo);
  
  jpeg_destroy_compress(&cinfo);
}  

Blob
JPEG::save(const SoftwareSurface& surface, int quality)
{
  // std::cout << "JPEG::save(const SoftwareSurface& surface, int quality)" << std::endl;

  struct jpeg_compress_struct cinfo;
  struct jpeg_error_mgr jerr;

  cinfo.err = jpeg_std_error(&jerr);

  jpeg_create_compress(&cinfo);

  std::vector<uint8_t> data;
  jpeg_memory_dest(&cinfo, &data);

  cinfo.image_width  = surface.get_width();
  cinfo.image_height = surface.get_height();

  cinfo.input_components = 3;		/* # of color components per pixel */
  cinfo.in_color_space   = JCS_RGB; 	/* colorspace of input image */

  jpeg_set_defaults(&cinfo);
  jpeg_set_quality(&cinfo, quality, TRUE /* limit to baseline-JPEG values */);
 
  jpeg_start_compress(&cinfo, TRUE);

  JSAMPROW row_pointer[surface.get_height()];
  
  for(int y = 0; y < surface.get_height(); ++y)
    row_pointer[y] = static_cast<JSAMPLE*>(surface.get_row_data(y));

  while(cinfo.next_scanline < cinfo.image_height)
    {
      jpeg_write_scanlines(&cinfo, &row_pointer[cinfo.next_scanline], 
                           surface.get_height() - cinfo.next_scanline);
    }
  
  jpeg_finish_compress(&cinfo);
  
  jpeg_destroy_compress(&cinfo);

  // FIXME: This causes an unnecessary copy, should have a BlobImpl that is based on std::vector<>
  return Blob(data);
}

#if 0 
void 
JPEG::crop()
{
  struct jpeg_decompress_struct srcinfo;
  struct jpeg_compress_struct dstinfo;
  struct jpeg_error_mgr jsrcerr, jdsterr;

  jvirt_barray_ptr * src_coef_arrays;
  jvirt_barray_ptr * dst_coef_arrays;

  /* Initialize the JPEG decompression object with default error handling. */
  srcinfo.err = jpeg_std_error(&jsrcerr);
  jpeg_create_decompress(&srcinfo);

  /* Initialize the JPEG compression object with default error handling. */
  dstinfo.err = jpeg_std_error(&jdsterr);
  jpeg_create_compress(&dstinfo);

  jsrcerr.trace_level = jdsterr.trace_level;
  srcinfo.mem->max_memory_to_use = dstinfo.mem->max_memory_to_use;

  /* Specify data source for decompression */
  jpeg_stdio_src(&srcinfo, fp);

  /* Enable saving of extra markers that we want to copy */
  // jcopy_markers_setup(&srcinfo, copyoption);

  /* Read file header */
  jpeg_read_header(&srcinfo, TRUE);

  /* Any space needed by a transform option must be requested before
   * jpeg_read_coefficients so that memory allocation will be done right.
   */
  jtransform_request_workspace(&srcinfo, &transformoption);

  /* Read source file as DCT coefficients */
  src_coef_arrays = jpeg_read_coefficients(&srcinfo);

  /* Initialize destination compression parameters from source values */
  jpeg_copy_critical_parameters(&srcinfo, &dstinfo);

  /* Adjust destination parameters if required by transform options;
   * also find out which set of coefficient arrays will hold the output.
   */
  dstinfo->image_width = info->output_width;
  dstinfo->image_height = info->output_height;

  dst_coef_arrays = src_coef_arrays;

  jpeg_stdio_dest(&dstinfo, fp);

  /* Start compressor (note no image data is actually written here) */
  jpeg_write_coefficients(&dstinfo, dst_coef_arrays);

  /* Copy to the output file any extra markers that we want to preserve */
  jcopy_markers_execute(&srcinfo, &dstinfo, copyoption);

  // JDIMENSION x_crop_offset, JDIMENSION y_crop_offset,
   
  { // Crop
    JBLOCKARRAY src_buffer, dst_buffer;
    jpeg_component_info *compptr;

    /* We simply have to copy the right amount of data (the destination's
     * image size) starting at the given X and Y offsets in the source.
     */
    for (int ci = 0; ci < dstinfo->num_components; ci++)
      {
        compptr = dstinfo->comp_info + ci;
        JDIMENSION x_crop_blocks = x_crop_offset * compptr->h_samp_factor;
        JDIMENSION y_crop_blocks = y_crop_offset * compptr->v_samp_factor;

        for (JDIMENSION dst_blk_y = 0; dst_blk_y < compptr->height_in_blocks; dst_blk_y += compptr->v_samp_factor) 
          {
            dst_buffer = (*srcinfo->mem->access_virt_barray)
              ((j_common_ptr) srcinfo, dst_coef_arrays[ci], dst_blk_y,
               (JDIMENSION) compptr->v_samp_factor, TRUE);

            src_buffer = (*srcinfo->mem->access_virt_barray)
              ((j_common_ptr) srcinfo, src_coef_arrays[ci],
               dst_blk_y + y_crop_blocks,
               (JDIMENSION) compptr->v_samp_factor, FALSE);

            for (int offset_y = 0; offset_y < compptr->v_samp_factor; offset_y++) 
              {
                jcopy_block_row(src_buffer[offset_y] + x_crop_blocks,
                                dst_buffer[offset_y],
                                compptr->width_in_blocks);
              }
          }
      }
  }

  { // Cleanup
    /* Finish compression and release memory */
    jpeg_finish_compress(&dstinfo);
    jpeg_destroy_compress(&dstinfo);
    jpeg_finish_decompress(&srcinfo);
    jpeg_destroy_decompress(&srcinfo);

  }
}
#endif

  
/* EOF */
