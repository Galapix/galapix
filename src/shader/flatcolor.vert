uniform mat4 projection;
uniform mat4 modelview;

attribute vec2 position;

void main(void)
{
  gl_Position = projection * modelview * vec4(position, 0, 1);
}

/* EOF */
