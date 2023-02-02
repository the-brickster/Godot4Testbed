shader_type spatial;
render_mode skip_vertex_transform;

void vertex() {
	vec3 vecMod = vec3(0);
	vecMod.x = float(VERTEX_ID >> 0 & 0xFF);
	vecMod.y = float(VERTEX_ID >> 8  & 0xFF);
	vecMod.z = float(VERTEX_ID >> 16  & 0xFF);
	VERTEX = (MODELVIEW_MATRIX * vec4(vecMod,1.0)).xyz;
}

void fragment() {
	ALBEDO = vec3(0.5,0,0);
	ALPHA = 1.0;
}
