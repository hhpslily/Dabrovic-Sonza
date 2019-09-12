#version 420 core                                                        
                                                     
uniform int mode;
uniform float width;
uniform float height;
uniform float mouse_x = 0.5f;
uniform float time;
float PI = 3.14159265;
layout (binding = 0) uniform sampler2D tex;
layout (binding = 1) uniform sampler2D noiseMap;
layout (binding = 2) uniform sampler2D mask;

out vec4 color;                                                              
                                                                            
in VS_OUT                                                                   
{                                                                            
  vec2 texcoord;   

} fs_in;

float snow(vec2 uv,float scale)
{
	float w = smoothstep(1.0, 0.0, -uv.y * (scale/10.0));
	if(w < 0.1) return 0.;
	uv += time/scale;
	uv.y += time*0.1 / scale;
	uv.x += sin(uv.y + time * 0.5) / scale;
	uv *= scale;
	vec2 s = floor(uv),f=fract(uv),p;
	float k=3.,d;
	p = 0.5 + 0.35 * sin(11.0 * fract(sin((s+p+scale)*mat2(7,3,6,5))*5.0))-f;
	d=length(p);
	k=min(d,k);
	k=smoothstep(0.0, k, sin(f.x+f.y) * 0.01);
    return k*w;
}


vec4 Blur()
{
	vec4 color = vec4(0);
	int n = 0;
	int half_size = 3;
	for(int i = -half_size; i <= half_size; ++i){
		for(int j = -half_size; j <= half_size; ++j){
			vec4 c = texture(tex, fs_in.texcoord + vec2(i, j) / vec2(width, height));
			color += c;
			n++;
		}
	}
	color /= n;
	return color;
}

vec4 Quantization()
{
	float nbins = 8.0;
	vec3 texture_color = texture2D(tex, fs_in.texcoord).rgb;
	texture_color = floor(texture_color * nbins) / nbins;
	return vec4(texture_color, 1);
}

vec4 DoG(void)
{
	float sigma_e = 2.0f;
	float sigma_r = 2.8f;
	float phi = 3.4f;
	float tau = 0.99f;
	float twoSigmaESquared = 2.0 * sigma_e * sigma_e;
	float twoSigmaRSquared = 2.0 * sigma_r * sigma_r;
	int halfWidth = int(ceil(2.0 * sigma_r));
	vec2 img_size = vec2(1024, 768);
			
	vec2 sum = vec2(0);
	vec2 norm = vec2(0);
	int kernel_count = 0;
	for(int i = -halfWidth; i <= halfWidth; ++i){
		for(int j = -halfWidth; j <= halfWidth; ++j){
			float d = length(vec2(i, j));
			vec2 kernel = vec2(exp(-d * d / twoSigmaESquared),
								exp(-d * d / twoSigmaRSquared));
			vec4 c = texture(tex, fs_in.texcoord + vec2(i, j) / img_size);
			vec2 L = vec2(0.299 * c.r + 0.587 * c.g + 0.114 * c.b);
			norm += 2.0 * kernel;
			sum += kernel * L;
		}
	}
	sum /= norm;
	float H = 100.0 * (sum.x - tau * sum.y);
	float edge = (H > 0.0) ? 1.0 : 2.0 * smoothstep(-2.0, 2.0, phi * H);
	return vec4(edge, edge, edge, 1.0);
}

void main(void) {

  vec4 vec4Sum = vec4(0.0);

    if(mode == 5){ // fish eye

      float aperture = 178.0;
      float apertureHalf = 0.5 * aperture * (PI / 180.0);
      float maxFactor = sin(apertureHalf);

      vec2 uv;
      vec2 xy = 2.0 * fs_in.texcoord - 1.0;
      float d = length(xy);
      if (d < (2.0-maxFactor))
      {
        d = length(xy * maxFactor);
        float z = sqrt(1.0 - d * d);
        float r = atan(d, z) / PI;
        float phi = atan(xy.y, xy.x);
  
        uv.x = r * cos(phi) + 0.5;
        uv.y = r * sin(phi) + 0.5;
      }
      else
      {
        uv = fs_in.texcoord;
      }
      vec4 c = texture2D(tex, uv);
      vec4Sum = c;

	}
	else if(mode == 9){ // Swirl

		float radius = 200.0;
		float angle = 0.8;
		vec2 center = vec2(width/2, height/2);
		
		vec2 texSize = vec2(width, height);
		vec2 tc = fs_in.texcoord * texSize;
		tc -= vec2(width/2, height/2);
		float dist = length(tc);

		if (dist < radius) 
		{
			float percent = (radius - dist) / radius;
			float theta = percent * percent * angle * 8.0;
			float s = sin(theta);
			float c = cos(theta);
			tc = vec2(dot(tc, vec2(c, -s)), dot(tc, vec2(s, c)));
		}
		tc += center;
		vec3 color = texture2D(tex, tc / texSize).rgb;
		vec4Sum = vec4(color, 1.0);

	}
	else if(mode == 11){ //night view
			
		vec4 finalColor;

		if (fs_in.texcoord.x < 1) {
			vec2 uv;               
			uv.x = 0.4*sin(time*50.0);                                 
			uv.y = 0.4*cos(time*50.0);   
			float m = texture2D(mask, fs_in.texcoord).r;
			vec3 n = texture2D(noiseMap, (fs_in.texcoord * 3.5) + uv).rgb;
			vec3 c = texture2D(tex, fs_in.texcoord + (n.xy*0.005)).rgb;
  
			float lum = dot(vec3(0.30, 0.59, 0.11), c);
			if (lum < 0.2)
				c *= 4; 
  
			vec3 visionColor = vec3(0.1, 0.95, 0.2);
			finalColor.rgb = (c + (n*0.2)) * visionColor * m;
		}
		else
		{
			finalColor = texture2D(tex, fs_in.texcoord);
		}

		vec4Sum = vec4(finalColor.rgb, 1.0);
	}
	else if (fs_in.texcoord.x < mouse_x - 0.002)
    {
		if(mode == 0){ // abstraction
			
			vec4Sum = Blur() * Quantization() * DoG();	
		}
		else if(mode == 1){ // Laplacian
			
			float w = 1 / float(width);
			float h = 1 / float(height);

			vec4 offset0 = texture2D(tex, fs_in.texcoord + vec2(-w,-h));
			vec4 offset1 = texture2D(tex, fs_in.texcoord + vec2(0,-h));
			vec4 offset2 = texture2D(tex, fs_in.texcoord + vec2(w,-h));
			vec4 offset3 = texture2D(tex, fs_in.texcoord + vec2(-w,0));
			vec4 offset4 = texture2D(tex, fs_in.texcoord + vec2(0,0));
			vec4 offset5 = texture2D(tex, fs_in.texcoord + vec2(w,0));
			vec4 offset6 = texture2D(tex, fs_in.texcoord + vec2(-w,h));
			vec4 offset7 = texture2D(tex, fs_in.texcoord + vec2(0,h));
			vec4 offset8 = texture2D(tex, fs_in.texcoord + vec2(w,h));

			vec4 color = -(offset0 + offset1 + offset2 + offset3 + offset5 + offset6 + offset7 + offset8) +  8 * offset4;
			vec4Sum = vec4(color.rgb, 1);

		}
		else if(mode == 2){ // Sharpen
			
			float w = 1 / float(width);
			float h = 1 / float(height);

			vec4 offset0 = texture2D(tex, fs_in.texcoord + vec2(-w,-h));
			vec4 offset1 = texture2D(tex, fs_in.texcoord + vec2(0,-h));
			vec4 offset2 = texture2D(tex, fs_in.texcoord + vec2(w,-h));
			vec4 offset3 = texture2D(tex, fs_in.texcoord + vec2(-w,0));
			vec4 offset4 = texture2D(tex, fs_in.texcoord + vec2(0,0));
			vec4 offset5 = texture2D(tex, fs_in.texcoord + vec2(w,0));
			vec4 offset6 = texture2D(tex, fs_in.texcoord + vec2(-w,h));
			vec4 offset7 = texture2D(tex, fs_in.texcoord + vec2(0,h));
			vec4 offset8 = texture2D(tex, fs_in.texcoord + vec2(w,h));

			vec4 color = -(offset0 + offset1 + offset2 + offset3 + offset5 + offset6 + offset7 + offset8) +  8 * offset4;
			vec4Sum = vec4(color.rgb, 1) + texture2D(tex, fs_in.texcoord);

		}
		else if(mode == 3){ // pixelation

		  float fx = fs_in.texcoord.x - mod(fs_in.texcoord.x, 1.0 / (width / 5));
		  float fy = fs_in.texcoord.y - mod(fs_in.texcoord.y, 1.0 / (height / 5));
    
		  vec3 col = texture2D(tex, vec2(fx, fy)).rgb;
		  vec4Sum = vec4(col, 1.0); 

		}
		else if(mode == 4){ // red blue stereo

		  vec4 texture_color_Left = texture(tex,fs_in.texcoord-vec2(0.005,0.0));    
		  vec4 texture_color_Right = texture(tex,fs_in.texcoord+vec2(0.005,0.0));   
		  vec4 texture_color = vec4(texture_color_Left.r*0.29+texture_color_Left.g*0.58+texture_color_Left.b*0.114,
		                            texture_color_Right.g,texture_color_Right.b,0.0); 
		  vec4Sum = texture_color;  

		}
		else if(mode == 6){ // halftone

		  float pixelSize = 1.0 / 800;
  
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
		  vec4Sum = vec4(col2, 1.0);

		}
		else if(mode == 7){ // water color
			 
			float gradientStep = 0.0015;
			float advectStep = 0.0055;
			float flipHeightMap = 0.7;
			float expand = 1;
			vec4 advectMatrix = vec4 (0.1);
			vec4 cxp = texture2D(noiseMap, vec2(fs_in.texcoord.x + gradientStep, flipHeightMap*fs_in.texcoord.y));
			vec4 cxn = texture2D(noiseMap, vec2(fs_in.texcoord.x - gradientStep, flipHeightMap*fs_in.texcoord.y));
			vec4 cyp = texture2D(noiseMap, vec2(fs_in.texcoord.x, flipHeightMap*(fs_in.texcoord.y + gradientStep)));
			vec4 cyn = texture2D(noiseMap, vec2(fs_in.texcoord.x, flipHeightMap*(fs_in.texcoord.y - gradientStep)));

			vec3 grey = vec3(.3, .59, .11);
			float axp = dot(grey, cxp.xyz);
			float axn = dot(grey, cxn.xyz);
			float ayp = dot(grey, cyp.xyz);
			float ayn = dot(grey, cyn.xyz);

			vec2 grad = vec2(axp - axn, ayp - ayn);
			vec2 newtc = fs_in.texcoord + advectStep * normalize(advectMatrix.xy * grad) * expand;

			vec4Sum.xyz = texture2D(tex, newtc).rgb;
		}
		else if(mode == 8){ // black and white

			vec4 Color = texture2D( tex, fs_in.texcoord );
			vec3 lum = vec3(0.299, 0.587, 0.114);
			vec4Sum = vec4( vec3(dot( Color.rgb, lum)), Color.a);

		}
		else if(mode == 10){ // Sin Wave

			vec2 uv = fs_in.texcoord;
			uv.x = uv.x + 0.05 * (sin(uv.y * 2 * PI + time));
			vec4Sum = texture2D(tex, uv);

		}
		else if(mode == 12){ //snow
			vec2 uv = (fs_in.texcoord*2 - vec2(width, height)) / min(width,height); 
			vec3 finalColor = vec3(0);
			float c = smoothstep(1.,0.3,clamp(uv.y*.3+.8,0.,.75));
			c+=snow(uv,30.0)*.3;
			c+=snow(uv,20.0)*.5;
			c+=snow(uv,15.0)*.8;
			c+=snow(uv,10.0);
			c+=snow(uv,8.0);
			c+=snow(uv,6.0);
			c+=snow(uv,5.0);
			finalColor=(vec3(c));
			vec4Sum = vec4(finalColor,1) * texture2D(tex, fs_in.texcoord);
		}
		else if(mode == 13){ //bloom
			vec4 blur = Blur();
			vec4Sum = texture2D(tex, fs_in.texcoord) * 1.3 + blur * 1.2 / 256;
		}
    }
    else if (fs_in.texcoord.x > mouse_x + 0.002)
    {
        vec4Sum = texture2D(tex, fs_in.texcoord);
    }
    else
    {
        vec4Sum = vec4(1.0, 0.0, 0.0, 1.0);
    }
    color = vec4Sum;
}