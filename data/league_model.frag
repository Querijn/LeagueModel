//#version 120
precision mediump float;

varying vec2 UV;

uniform sampler2D u_Diffuse;

void main()
{
	gl_FragColor.rgb = texture2D(u_Diffuse, UV).rgb;
}