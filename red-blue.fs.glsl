#version 410 core                                                        
uniform sampler2D tex;                                                     
                                                                            
out vec4 color;                                                              
                                                                            
in VS_OUT                                                                   
{                                                                            
	vec2 texcoord;                                                          
} fs_in;                                                                      
                                                                             
void main(void)                                                             
{                                                                             
	vec4 texture_color_Left = texture(tex,fs_in.texcoord-vec2(0.005,0.0));		
	vec4 texture_color_Right = texture(tex,fs_in.texcoord+vec2(0.005,0.0));		
	vec4 texture_color = vec4(texture_color_Left.r*0.29+texture_color_Left.g*0.58+texture_color_Left.b*0.114,texture_color_Right.g,texture_color_Right.b,0.0); 
	color = texture_color;		
}                                                                          