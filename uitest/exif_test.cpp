#include <stdio.h>
#include <stdlib.h>
#include <libexif/exif-data.h>

/*
  Default       =   1
  1  = The 0th row is at the visual top    of the image, and the 0th column is the visual left-hand side.
  2  = The 0th row is at the visual top    of the image, and the 0th column is the visual right-hand side.
  3  = The 0th row is at the visual bottom of the image, and the 0th column is the visual right-hand side.
  4  = The 0th row is at the visual bottom of the image, and the 0th column is the visual left-hand side.
  5  = The 0th row is the visual left-hand  side of the image, and the 0th column is the visual top.
  6  = The 0th row is the visual right-hand side of the image, and the 0th column is the visual top.
  7  = The 0th row is the visual right-hand side of the image, and the 0th column is the visual bottom.
  8  = The 0th row is the visual left-hand  side of the image, and the 0th column is the visual bottom.
  Other = reserved
 */

enum 
{
  kRot0       = 1, // top - left
  kRot0Flip   = 2,
  kRot180     = 3,
  kRot180Flip = 4,
  kRot270Flip = 5,
  kRot90      = 6, // right - top
  kRot90Flip  = 7,
  kRot270     = 8  // left - bottom
};

int
get_orientation (ExifData *exif_data)
{
  ExifEntry *entry;
  
  ExifByteOrder byte_order = exif_data_get_byte_order(exif_data);

  /* get orientation and rotate image accordingly if necessary */
  if ((entry = exif_content_get_entry (exif_data->ifd[EXIF_IFD_0],
                                       EXIF_TAG_ORIENTATION)))
    {
      return exif_get_short(entry->data, byte_order);
    }
  return 0;
}

int main(int argc, char** argv)
{
  for(int i = 1; i < argc; ++i)
  {
    ExifData* ed = exif_data_new_from_file(argv[i]);

    if (!ed) 
    {
      printf("File not readable or no EXIF data in file %s\n", argv[i]);
      return EXIT_FAILURE;
    }
    else
    {
      ExifEntry* entry = exif_content_get_entry(ed->ifd[EXIF_IFD_0], EXIF_TAG_ORIENTATION);
      if (!entry) 
      {
        printf("Couldn't read entry\n");
      }
      else
      {
        char buf[1024];
   
        /* Get the contents of the tag in human-readable form */
        int v = get_orientation(ed);
        exif_entry_get_value(entry, buf, sizeof(buf));
   
        /* Don't bother printing it if it's entirely blank */
        if (*buf) 
        {
          printf("%s: %d \"%s\"\n", 
                 exif_tag_get_name_in_ifd(EXIF_TAG_ORIENTATION, EXIF_IFD_0), 
                 v,
                 buf);
        }
      }
    }
  }
  return EXIT_SUCCESS;
}

/* EOF */
