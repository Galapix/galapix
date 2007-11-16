// Little experimental epeg test app

#include <Epeg.h>
#include <iostream>

int main(int argc, char** argv)
{
  if (argc != 7)
    {
      printf("Usage: %s INFILE X Y W H OUTFILE\n", argv[0]);
      exit(0);
    }

  char* filename = argv[1];
  int x = atoi(argv[2]);
  int y = atoi(argv[3]);
  int w = atoi(argv[4]);
  int h = atoi(argv[5]);
  //char* out_filename = argv[6];

  std::cout << filename << " " << x << ", " << y << " - " << w << "x" << h << std::endl;

  Epeg_Image* img;

  img = epeg_file_open(filename);
  if (!img)
    {
      printf("Cannot open %s\n", filename);
      exit(-1);
    }


  const unsigned char* pixels = static_cast<const unsigned char*>(epeg_pixels_get(img, x, y, w, h));
  epeg_pixels_free(img, pixels);

  epeg_close(img);

  
  return 0;
}

/* EOF */
