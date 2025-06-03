
/*
    Invoke twice to initialize in both ray sides.
*/

#version 400

uniform int unitsegment;                                                // the ray unit segment index of the fbo ray2 buffer


layout(triangle_strip, max_vertices = 3) out;
layout(triangles, invocations = 2) in;                                  // invoke twice

void main() { 

    const int RAYSIDELEN = 2;                                           // RAYSIDELEN: number of the ray sides

    gl_Layer = unitsegment * RAYSIDELEN + gl_InvocationID;

    for(int vert = 0; vert < gl_in.length(); ++vert){
        gl_Position = gl_in[vert].gl_Position; EmitVertex();            // pass the input vertices processed in the previous stage.
    }

    EndPrimitive();

}
