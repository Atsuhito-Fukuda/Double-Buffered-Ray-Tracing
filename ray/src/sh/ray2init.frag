
/*
    Initialize the FBO Ray2 depth buffer to 0.0 and index buffer to (-1, -1).
*/

#version 330

out uvec2 outindex;

void main() { outindex = uvec2(-1, -1); gl_FragDepth = 0.0f; }
