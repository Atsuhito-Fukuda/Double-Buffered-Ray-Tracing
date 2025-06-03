
/* Pass through. */

#version 330

layout(triangle_strip, max_vertices = 3) out;
layout(triangles) in;

void main() {

    for(int vert = 0; vert < gl_in.length(); ++vert){
        gl_Position = gl_in[vert].gl_Position; EmitVertex();            // pass the input vertices processed in the previous stage.
    }

    EndPrimitive();
}
