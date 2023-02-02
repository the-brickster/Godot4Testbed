shader_type spatial;
render_mode depth_draw_never,cull_disabled,unshaded;
uniform sampler2D DEPTH_TEXTURE : hint_depth_texture;
uniform sampler2D SCREEN_TEXTURE : hint_screen_texture;
uniform float zNear = 0.05;
uniform float zFar = 100.0;


void vertex(){
	POSITION = vec4(VERTEX,1.0);
	
}

void fragment() {
	float depth = texture(DEPTH_TEXTURE, SCREEN_UV).x;
	vec3 ndc = vec3(SCREEN_UV, depth)* 2.0 - 1.0;
	vec4 view = INV_PROJECTION_MATRIX * vec4(ndc, 1.0);
	view.xyz /= view.w;
	float linear_depth = view.z;
	float scale_depth = (zFar*zNear)/(zFar + (linear_depth * (zNear - zFar) ));
	vec2 screen_uv = FRAGCOORD.xy/VIEWPORT_SIZE;
	vec4 screen_albedo = texture(SCREEN_TEXTURE,SCREEN_UV);
	ALBEDO = screen_albedo.xyz*vec3(0.25,0.75,1.0);
	ALBEDO = vec3(scale_depth);
//	ALPHA = 0.0;

}
