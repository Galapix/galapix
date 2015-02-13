uniform mat4 projection;
uniform mat4 modelview;

attribute vec2 position;
attribute vec2 texcoord;

varying vec2 texcoord_v;

void main(void)
{
  texcoord_v = texcoord;
  gl_Position = projection * modelview * vec4(position, 0, 1);
}

/* EOF */
