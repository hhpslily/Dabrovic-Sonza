#version 410 core                                                        
uniform sampler2D tex;                                                     
                                                                            
out vec4 color;                                                              
                                                                            
in VS_OUT                                                                   
{                                                                            
	vec2 texcoord;                                                          
} fs_in;

void main(){

	float fx = fs_in.texcoord.x - mod(fs_in.texcoord.x, 1.0 / 80);
	float fy = fs_in.texcoord.y - mod(fs_in.texcoord.y, 1.0 / 60);
    
	vec3 col = texture2D(tex, vec2(fx, fy)).rgb;
	color = vec4(col, 1.0);	
}
