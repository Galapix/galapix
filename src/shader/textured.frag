uniform sampler2D tex;
varying vec2 texcoord_v;

void main(void)
{
  gl_FragColor = texture2D(tex, texcoord_v);
}

/* EOF */
