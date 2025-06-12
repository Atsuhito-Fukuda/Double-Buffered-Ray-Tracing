
/*
	Render the results of the Ray Selection stage to the screen.

	Planes and Ray Unit attributes are re-read to calculate the intersection point and read the texel color.
*/ 

#version 330

const int PLELMSIZE = 3, PLPOS = 0, PLNORMAL = 1, PLUAXIS = 2;
const int RAYBUFFSIZE = 2, RAYPOS = 0, RAYDIR = 1;
const int UNITINDEX = 0, PLINDEX = 1;

struct Unit {
	/* This structure contains an attribute data of a ray unit. */
	mat4 modelmat4;														// model matrix of ray unit
	vec2 texscale, padding;												// texscale: scale factor of an image texture (use negative val to flip tex)
};

struct Plane{ 
	/* This structure contains a plane. */ vec4 vec[PLELMSIZE];			// (pos, normal, u-axis)
};


out vec4 outcolor;														// output the rendered color


layout(std140, row_major) uniform UboUnitBuffer { Unit unit[20]; };		// ubo unit buffer. stores ray unit attribute data
layout(std140) uniform UboPlaneBuffer { Plane mdpl[200]; };				// ubo plane buffer. stores ray unit plane data

uniform sampler2DArray depthbuffer;										// fbo selection depth buffer. stores ray distances to planes from the selection stage
uniform sampler2DArray indexbuffer;										// fbo selection index buffer. stores ray unit and plane indices from the selection stage

uniform sampler2D raybuffer[RAYBUFFSIZE];								// ray pos and dir buffers
uniform sampler2D img;													// image texture

uniform mat4 viewmat4;													// view matrix


vec4 Phong(vec3 pos, vec3 N);

void main() { 

	const float raydist = 1000.0f;										// maximum ray distance


	float depth = texelFetch(depthbuffer, ivec3(gl_FragCoord.xy, 0), 0).x;

	vec4 light; vec2 texcoord; {										// light: light intensity, texcoord: texel coord on a scaled image

		vec3 intersectpos; {											// ray intersection pos on the concerned plane

			vec4 ray[RAYBUFFSIZE]; for (int i = 0; i < RAYBUFFSIZE; ++i) { ray[i] = texelFetch(raybuffer[i], ivec2(gl_FragCoord.xy), 0); }

			intersectpos = ray[RAYPOS].xyz + depth * raydist * ray[RAYDIR].xyz;
		}

		Plane pl; vec2 texscale; {										// pl: plane transformed into ray space, texscale: scale factor of image tex
		
			uvec2 index = uvec2(texelFetch(indexbuffer, ivec3(gl_FragCoord.xy, 0), 0).xy);

			for (int i = 0; i < PLELMSIZE; ++i) { pl.vec[i] = viewmat4 * unit[index[UNITINDEX]].modelmat4 * mdpl[index[PLINDEX]].vec[i]; }	
			texscale = unit[index[UNITINDEX]].texscale;
		
		}


		texcoord = vec2(
			(dot(intersectpos - pl.vec[PLPOS].xyz, pl.vec[PLUAXIS].xyz / texscale.x) + 1.0f) / 2.0f,
			1.0f - (dot(intersectpos - pl.vec[PLPOS].xyz, cross(pl.vec[PLNORMAL].xyz, pl.vec[PLUAXIS].xyz)) / texscale.y + 1.0f) / 2.0f		// derive the v-axis from the normal and the u-axis
		);

		light = Phong(pl.vec[PLPOS].xyz, pl.vec[PLNORMAL].xyz);			// phong lighting

	}

	// output
	
	outcolor = int(depth != 1.0f) * light * vec4(texture(img, texcoord).xyz, 1.0f);
	
}


vec4 Phong(vec3 pos, vec3 N) {

	/* Simple Phong lighting. */

	const vec3 light_dir = vec3(-1.0f, 1.0f, -0.75f) / 1.6;

	const vec3 
		amb_light = vec3(0.7f, 0.74f, 0.75f),
		diff_light = vec3(0.5f, 0.57f, 0.60f),
		spec_light = vec3(0.2f, 0.2f, 0.2f);

	vec3
		L = -light_dir,
		V = normalize(-pos),
		R = reflect(-L,N);	

	return vec4(amb_light + diff_light * max(dot(N,L),0.0) + spec_light * pow(max(0.0,dot(R,V)), 1.0f), 1.0f);

}
