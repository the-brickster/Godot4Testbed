struct Box{
	vec3 center;
	vec3 radius;
	vec3 invRadius;
	mat3 rotation;
};

float safeInverse(float x) { return (x == 0.0) ? 1e12 : (1.0 / x); }
vec3 safeInverseV3(vec3 v) { return vec3(safeInverse(v.x), safeInverse(v.y), safeInverse(v.z)); }


// Just determines whether the ray hits the axis-aligned box.
// invRayDirection is guaranteed to be finite for all elements.
bool ourHitAABox(vec3 boxCenter, vec3 boxRadius, vec3 rayOrigin, vec3 rayDirection, vec3 invRayDirection) {
    rayOrigin -= boxCenter;
    vec3 distanceToPlane = (-boxRadius * sign(rayDirection) - rayOrigin) * invRayDirection;

    return bool(float(abs(rayOrigin.x) < boxRadius.x) *
                float(abs(rayOrigin.y) < boxRadius.y) *
                float(abs(rayOrigin.z) < boxRadius.z) +
                (float(distanceToPlane.x >= 0.0) * float(abs(rayOrigin.y + rayDirection.y * distanceToPlane.x) < boxRadius.y) * float(abs(rayOrigin.z + rayDirection.z * distanceToPlane.x) < boxRadius.z)) +
                (float(distanceToPlane.y >= 0.0) * float(abs(rayOrigin.z + rayDirection.z * distanceToPlane.y) < boxRadius.z) * float(abs(rayOrigin.x + rayDirection.x * distanceToPlane.y) < boxRadius.x)) +
                (float(distanceToPlane.z >= 0.0) * float(abs(rayOrigin.x + rayDirection.x * distanceToPlane.z) < boxRadius.x) * float(abs(rayOrigin.y + rayDirection.y * distanceToPlane.z) < boxRadius.y)));

}

vec2 intersectAABB(vec3 rayOrigin, vec3 rayDir, vec3 boxMin, vec3 boxMax) {
    vec3 tMin = (boxMin - rayOrigin) / rayDir;
    vec3 tMax = (boxMax - rayOrigin) / rayDir;
    vec3 t1 = min(tMin, tMax);
    vec3 t2 = max(tMin, tMax);
    float tNear = max(max(t1.x, t1.y), t1.z);
    float tFar = min(min(t2.x, t2.y), t2.z);
    return vec2(tNear, tFar);
}