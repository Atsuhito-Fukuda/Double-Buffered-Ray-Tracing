
/*
    Invoke twice to calculate in both ray sides.

    Transform the concerned plane from model space to ray space.
*/

#version 400

const int PLELMSIZE = 3;

struct Plane {
    /* This structure contains a plane. */ vec4 vec[PLELMSIZE];         // (pos, normal, u-axis)
};

struct Unit {
    /* This structure contains an attribute data of a ray unit. */
    mat4 modelmat4; vec2 texscale, padding;                             // modelmat4: model matrix of a ray unit
};       


flat out uvec2 index;                                                   // output the concerned ray unit and plane indices
flat out Plane pl;                                                      // output the concerned plane in ray space
flat out int rayside;                                                   // output the ray side index of this invocation


layout(std140, row_major) uniform UboUnitBuffer { Unit unit[20]; };     // ubo unit buffer. stores ray unit attribute data
layout(std140) uniform UboPlaneBuffer { Plane mdpl[200]; };             // ubo plane buffer. stores ray unit plane data

uniform int plstart, unitindex, unitsegment;                            // plstart: the plane start index of a ray unit in the ubo plane buffer
                                                                        // unitindex: the ray unit index in the ubo unit buffer
                                                                        // unitsegment: the ray unit segment index of the fbo ray2 buffer
uniform mat4 viewmat4;                                                  // view matrix

layout(triangle_strip, max_vertices = 3) out;
layout(triangles, invocations = 2) in;                                  // invoke twice

void main() {

    const int RAYSIDELEN = 2;                                           // RAYSIDELEN: number of the ray sides

    
    int plindex = plstart + (gl_PrimitiveIDIn / 2);

    index = uvec2(unitindex, plindex);

    for (int i = 0; i < PLELMSIZE; ++i) { pl.vec[i] = viewmat4 * unit[unitindex].modelmat4 * mdpl[plindex].vec[i]; }

    rayside = gl_InvocationID;
    gl_Layer = unitsegment * RAYSIDELEN + gl_InvocationID;


    for(int vert = 0; vert < gl_in.length(); ++vert){
        gl_Position = gl_in[vert].gl_Position; EmitVertex();            // pass the input vertices processed by the previous stage
    }


    EndPrimitive();

}
