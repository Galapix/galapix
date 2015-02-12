//attribute vec4 coord;
 
void main(void)
{
  //gl_Position = coord;
  gl_Position = ftransform();
}

/* EOF */
