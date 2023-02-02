shader_type spatial;
render_mode skip_vertex_transform,depth_draw_always;
#include "res://shaders/intersection.gdshaderinc"

uniform vec2 viewport_pos = vec2(0,0);

varying mat4 inverse_view_projection_matrix;
varying vec3 aabb_center;
varying vec3 aabb_radius;
varying vec3 box_min;
varying vec3 box_max;
varying mat4 view_projection_matrix;


//Fast Quadric Proj: "GPU-Based Ray-Casting of Quadratic Surfaces" http://dl.acm.org/citation.cfm?id=2386396
void quadricProj(in vec3 osPosition, in float voxSize, in mat4 objectToScreenMatrix, in vec2 halfScreenSize, inout vec4 position, inout float pointSize) {

    vec4 quadricMat = vec4(1.0, 1.0, 1.0, -1.0);
//	float sphereRadius = (voxSize*0.5f) * 1.5;
    float sphereRadius = (voxSize*0.5f) * 1.732051;
//    float sphereRadius = (voxSize) * 3.0;
    vec4 sphereCenter = vec4(osPosition.xyz, 1.0);
    mat4 modelViewProj = transpose(objectToScreenMatrix);
    mat4 matT = mat4( mat3(modelViewProj[0].xyz, modelViewProj[1].xyz, modelViewProj[3].xyz) * sphereRadius);
    matT[0].w = dot(sphereCenter, modelViewProj[0]);
    matT[1].w = dot(sphereCenter, modelViewProj[1]);
    matT[2].w = dot(sphereCenter, modelViewProj[3]);
    mat4 matD = mat4(matT[0] * quadricMat, matT[1] * quadricMat, matT[2] * quadricMat,vec4(0.0));
    vec4 eqCoefs =
    vec4(dot(matD[0], matT[2]), dot(matD[1], matT[2]), dot(matD[0], matT[0]), dot(matD[1], matT[1]))
    / dot(matD[2], matT[2]);
    vec4 outPosition = vec4(eqCoefs.x, eqCoefs.y, 0.0, 1.0);
    vec2 AABB = sqrt(eqCoefs.xy*eqCoefs.xy - eqCoefs.zw);
    AABB *= halfScreenSize * 2.0f;
    position.xy = outPosition.xy * position.w;
    pointSize = min(AABB.x, AABB.y);
	
}

void vertex() {
	vec3 vec_mod = vec3(0);
	vec_mod.x = float(VERTEX_ID >> 0 & 0xFF);
	vec_mod.y = float(VERTEX_ID >> 8  & 0xFF);
	vec_mod.z = float(VERTEX_ID >> 16  & 0xFF);
	vec4 out_pos = (MODELVIEW_MATRIX * vec4(vec_mod,1.0));
	
	mat4 MVP = PROJECTION_MATRIX * MODELVIEW_MATRIX;
	view_projection_matrix = MVP;
	vec2 half_screen_size = VIEWPORT_SIZE/2.0f;
	float vox_size = 0.25f;

	quadricProj(vec_mod,vox_size,MVP,half_screen_size,out_pos,POINT_SIZE);


	VERTEX = (MODELVIEW_MATRIX * vec4(vec_mod,1.0f)).xyz;
	
	inverse_view_projection_matrix = inverse(PROJECTION_MATRIX * VIEW_MATRIX);
	aabb_center = vec_mod;
	aabb_radius = vec3(vox_size*0.5f);
	box_max = aabb_center+aabb_radius;
	box_min = aabb_center-aabb_radius;
}


// Almost. The 'p' is assumed to be in NDC space, yes. Now, since the uninverted (projection * view) matrix transforms
// from "world" space to homogeneous clip space, we want the opposite and therefore transform NDC (which is just synonymous
// to homogeneous clip space with w=1) into "homogeneous world space" and then to actual 3D world space by perspective division.
// This essentially gives the world-space 3D coordinates of the point on the near clipping plane for the current/respective
// fragment/pixel. Then we convert that position into a direction by subtracting the world-space camera position from it.
//layout (location = 1) out vec4 texCoordOut;

void fragment() {
	
	vec2 p = 2.0f*vec2(FRAGCOORD.xy)/(VIEWPORT_SIZE - viewport_pos) - vec2(1.0);
	vec3 ray_origin = CAMERA_POSITION_WORLD;
	vec4 rdh = inverse_view_projection_matrix * vec4(p,-1.0,1.0);
	vec3 ray_dir = rdh.xyz/rdh.w - ray_origin;
	vec3 inv_ray_dir = safeInverseV3(ray_dir);
	
//	bool intersection = ourHitAABox(aabb_center,aabb_radius,ray_origin,ray_dir,inv_ray_dir);
	ALBEDO = vec3(0.5f,0,0);
//	if(!intersection)
//		ALBEDO = vec3(0.1,0.5,0.8);
	vec2 result = intersectAABB(ray_origin,ray_dir,box_min,box_max);
    bool rayIntersectionTest = result.y >= result.x;
	if(rayIntersectionTest == false)
		discard;

	vec3 pos = ray_dir* result.x  ;
	vec4 PCLIP = (view_projection_matrix ) * vec4(pos,1.0);
//	float ndc_depth = PCLIP.z/PCLIP.w;
//	DEPTH = (ndc_depth + 1.0)/2.0;
	//Due to godot 4 vulkan shenanigans this is what will work
	float ndc_depth = (PCLIP.z / PCLIP.w) * 2.0 - 1.0;
	DEPTH = ndc_depth;
//	texCoordOut = vec4(1.0);
	
	
}
