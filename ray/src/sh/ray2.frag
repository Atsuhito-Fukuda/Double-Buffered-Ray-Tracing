
/*
	Calculate the intersection distance (depth) from the concerned ray to the input plane.
*/

#version 330

const int PLELMSIZE = 3, PLPOS = 0, PLNORMAL = 1;
const int RAYBUFFSIZE = 2, RAYPOS = 0, RAYDIR = 1;

struct Plane{
	/* This structure contains a plane. */ vec4 vec[PLELMSIZE];			// (pos, normal, u-axis)
};


out uvec2 outindex;														// output the concerned ray unit and plane indices


flat in uvec2 index;													// input the concerned ray unit and plane indices
flat in Plane pl;														// input the concerned plane
flat in int rayside;													// input the ray side index of this invocation (0:incident, 1:opposite)

uniform sampler2D raybuffer[RAYBUFFSIZE];								// ray pos and dir buffers


void main() {

	const float raydist = 1000.0f;										// maximum ray distance


	outindex = index;

	float sdist, cosine; {												// sdist: signed distance to the plane from the ray side of this invocation
																		// cosine: cos of the angle between the plane normal and the ray dir vec for this ray side invocation

		vec4 ray[RAYBUFFSIZE]; for (int i = 0; i < RAYBUFFSIZE; ++i) { ray[i] = texelFetch(raybuffer[i], ivec2(gl_FragCoord.xy), 0); }

		sdist = dot(pl.vec[PLNORMAL].xyz, (ray[RAYPOS].xyz - pl.vec[PLPOS].xyz) / raydist + rayside * ray[RAYDIR].xyz);
		cosine = dot(pl.vec[PLNORMAL].xyz, ((1 - rayside) * 1.0f + rayside * (-1.0f)) * ray[RAYDIR].xyz);
	}

	gl_FragDepth = clamp(-sdist / min(cosine, -1.0e-4f) , 0.0f, 1.0f);	// clamped between 0.0 and 1.0

}
