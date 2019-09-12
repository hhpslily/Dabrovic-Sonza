#version 410 core                                                        
uniform sampler2D tex;                                                     
                                                                
out vec4 color;                                                              
                                                                            
in VS_OUT                                                                   
{                                                                            
	vec2 texcoord;                                                          
} fs_in;  

vec4 blur(sampler2D img, vec2 uv, vec2 resolution, vec2 direction) {
    vec4 color = vec4(0.0);
    vec2 off1 = vec2(1.411764705882353) * direction;
    vec2 off2 = vec2(3.2941176470588234) * direction;
    vec2 off3 = vec2(5.176470588235294) * direction;
    color += texture2D(img, uv) * 0.1964825501511404;
    color += texture2D(img, uv + (off1 / resolution)) * 0.2969069646728344;
    color += texture2D(img, uv - (off1 / resolution)) * 0.2969069646728344;
    color += texture2D(img, uv + (off2 / resolution)) * 0.09447039785044732;
    color += texture2D(img, uv - (off2 / resolution)) * 0.09447039785044732;
    color += texture2D(img, uv + (off3 / resolution)) * 0.010381362401148057;
    color += texture2D(img, uv - (off3 / resolution)) * 0.010381362401148057;
    return color;
}


vec4 quantization(sampler2D img, vec2 v, vec2 resolution){
	vec2 uv = v / resolution;
    vec2 d = 1.0 / resolution;
    vec3 c = texture2D(img, uv).xyz;

    float qn = floor(c.x * float(8) + 0.5) / float(8);
    float qs = smoothstep(-2.0, 2.0, 8 * (c.x - qn) * 100.0) - 0.5;
    float qc = qn + qs / float(8);

	return vec4( vec3(qc, c.yz), 1.0 );
}

vec4 DoG(sampler2D img, vec2 v, vec2 resolution){
	vec2 uv = v / resolution;
    float twoSigmaESquared = 2.0 * 1 * 1;
    float twoSigmaRSquared = 2.0 * 1.6 * 1.6;
    int halfWidth = int(ceil(2.0 * 1.6));

    vec2 sum = vec2(0.0);
    vec2 norm = vec2(0.0);

    for ( int i = -halfWidth; i <= halfWidth; ++i ) {
        for ( int j = -halfWidth; j <= halfWidth; ++j ) {
            float d = length(vec2(i,j));
            vec2 kernel = vec2( exp( -d * d / twoSigmaESquared ), 
                                exp( -d * d / twoSigmaRSquared ));
                
            vec2 L = texture2D(img, uv + vec2(i,j) / resolution).xx;

            norm += 2.0 * kernel;
            sum += kernel * L;
        }
    }
    sum /= norm;

    float H = 100.0 * (sum.x - 0.99 * sum.y);
    float edge = ( H > 0.0 )? 1.0 : 2.0 * smoothstep(-2.0, 2.0, 2 * H );
    return vec4(vec3(edge), 1.0);
}

void main(){
    //color = blur(tex, fs_in.texcoord, vec2(800, 600), vec2(1.0, 0.0)) + quantization(tex, fs_in.texcoord, vec2(800, 600)) + DoG(tex, fs_in.texcoord, vec2(800, 600));
	color = blur(tex, fs_in.texcoord, vec2(800, 600), vec2(1.0, 0.0));
}                                                                         