#version 410 core                                                        
uniform sampler2D tex;                                                     
                                                                            
out vec4 color;                                                              
                                                                            
in VS_OUT                                                                   
{                                                                            
	vec2 texcoord;                                                          
} fs_in;  

void main(void)
{
	float pixelSize = 1.0 / float(800);
	
	float dx = mod(fs_in.texcoord.x, pixelSize) - pixelSize*0.5;
	float dy = mod(fs_in.texcoord.y, pixelSize) - pixelSize*0.5;
	float tx = fs_in.texcoord.x - dx;
	float ty = fs_in.texcoord.y - dy;

	vec3 col = texture2D(tex, vec2(tx,ty)).rgb;
	float bright = 0.3333*(col.r+col.g+col.b);
	
	float dist = sqrt(dx*dx + dy*dy);
	float rad = bright * pixelSize * 0.8;
	float m = step(dist, rad);

	vec3 col2 = mix(vec3(0.0), vec3(1.0), m);
	color = vec4(col2, 1.0);
}