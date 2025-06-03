
/* Render the triangles that cover the screen. */

#version 330

void main(void){

	const float triangle[] = float[](-1.0f,1.0f,  1.0f,-1.0f,  1.0f,1.0f,  -1.0f,-1.0f,  1.0f,-1.0f,  -1.0f,1.0f);

	gl_Position = vec4(triangle[2 * (gl_VertexID % 6)], triangle[2 * (gl_VertexID % 6) + 1], 0.0f, 1.0f);

}
