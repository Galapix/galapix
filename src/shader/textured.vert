//attribute vec4 coord;

void main(void)
{
  //gl_Position = coord;
  gl_TexCoord[0] = gl_MultiTexCoord0;
  gl_Position = ftransform();
}

/* EOF */
