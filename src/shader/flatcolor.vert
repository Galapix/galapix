#version 330

uniform mat4 projection;
uniform mat4 modelview;

void main(void)
{
  gl_Position = projection * modelview * gl_Vertex;
}

/* EOF */
