#module math
float pi = 3.14159265;

float saturate(float v) { return clamp(v, 0.0, 1.0); }
vec3  saturate(vec3 v)  { return clamp(v, 0.0, 1.0); }
