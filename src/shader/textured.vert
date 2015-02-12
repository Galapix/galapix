#version 330

uniform mat4 projection;
uniform mat4 modelview;

attribute vec2 position;
attribute vec2 texcoord;

varying vec2 texcoord_v;

void main(void)
{
  texcoord_v = texcoord + position*0.00001;
  //gl_Position = projection * modelview * (gl_Vertex*0.00001 + (vec4(position, 0, 0)));
  gl_Position = projection * modelview * gl_Vertex;
}

/* EOF */
