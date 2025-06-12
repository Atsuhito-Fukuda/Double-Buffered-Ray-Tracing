
/*
	Layer 0, tests if the two rays in the Ray2 stage have captured actual regions.

	In Layer 1, the actual region of the Ray Primary Unit gets subtracted by the regions of the Ray Subordinate Units.

	Layer 2, outputs a result from the captured actual region(s).
*/

#version 330

struct Cell {
	/* This structure contains an actual region, captured from both ray sides. */
	float depth[2]; vec2 index[2];										// depth: intersection distance, index: ray unit and plane indices
};


out vec2 outindex;	 													// output the final ray unit and plane indices for subsequent rendering


uniform sampler2DArray depthbuffer;										// fbo ray2 depth buffer. stores ray distances to planes from the ray2 stage
uniform sampler2DArray indexbuffer;										// fbo ray2 index buffer. stores ray unit and plane indices from the ray2 stage


void main() { 

	const int RAYSIDELEN = 2, UNITSEGSIZE = 3;							// RAYSIDELEN: number of the ray sides
																		// UNITSEGSIZE: number of ray unit segments contained in an ray object segment of the fbo ray2 buffer
	const int PRMCELL = 0;												// index of the ray primary unit cell
	
	const Cell defcell = Cell(float[](1.0f, 0.0f), vec2[](vec2(-1, -1), vec2(-1, -1)));		// initial val of the cell


	// (-- layer 0: capture test --) test if the two rays have captured actual regions of the ray units

	Cell cell[UNITSEGSIZE]; for(int i = 0; i < UNITSEGSIZE; ++i) {

		float depth[RAYSIDELEN]; for (int n = 0; n < RAYSIDELEN; ++n) { depth[n] = texelFetch(depthbuffer, ivec3(gl_FragCoord.xy, i * RAYSIDELEN + n), 0).x; }

		bool D = depth[0] + depth[1] < 1.0f;
		
		for (int n = 0; n < RAYSIDELEN; ++n) { cell[i].depth[n] = int(D) * depth[n] + (1 - int(D)) * defcell.depth[n]; }
		for (int n = 0; n < RAYSIDELEN; ++n) { cell[i].index[n] = int(D) * texelFetch(indexbuffer, ivec3(gl_FragCoord.xy, i * RAYSIDELEN + n), 0).xy + (1 - int(D)) * defcell.index[n]; }
	
	}

	// (-- layer 1: subtraction --) starting from the ray primary unit, re-capture actual regions by subtracting each ray subordinate unit

	Cell actcell = cell[PRMCELL];										// store a subtracted and re-captured actual region
	
	for(int i = PRMCELL + 1; i < UNITSEGSIZE; ++i){

		Cell subcell; {
			for (int n = 0; n < RAYSIDELEN; ++n) { subcell.depth[n] = 1.0f - cell[i].depth[(n + 1) % RAYSIDELEN]; }
			for (int n = 0; n < RAYSIDELEN; ++n) { subcell.index[n] = cell[i].index[(n + 1) % RAYSIDELEN]; }
		}

		int tmpptr = 0; Cell tmpcell[RAYSIDELEN];						// temporarily store the results of subtracting a ray sub unit from an actual region
		for(int n = 0; n < RAYSIDELEN; ++n) {

			bool Dm[RAYSIDELEN]; for (int m = 0; m < RAYSIDELEN; ++m){ Dm[m] = (actcell.depth[m] < subcell.depth[m]) ^^ (bool(n) ^^ bool(m)); }

			float depth[RAYSIDELEN]; for (int m = 0; m < RAYSIDELEN; ++m){ depth[m] = int(Dm[m]) * actcell.depth[m] + (1 - int(Dm[m])) * subcell.depth[m]; }

			for (int m = 0; m < RAYSIDELEN; ++m){ tmpcell[tmpptr].depth[m] = depth[m]; }
			for (int m = 0; m < RAYSIDELEN; ++m){ tmpcell[tmpptr].index[m] = int(Dm[m]) * actcell.index[m] + (1 - int(Dm[m])) * subcell.index[m]; }
				
			bool D = depth[0] + depth[1] < 1.0f;						// test if an actual region is captured

			tmpptr += int(D);
		
		}

		bool D = tmpptr > 0;											// move the closest result from the re-captured regions

		for (int n = 0; n < RAYSIDELEN; ++n){ actcell.depth[n] = int(D) * tmpcell[0].depth[n] + (1 - int(D)) * defcell.depth[n]; }
		for (int n = 0; n < RAYSIDELEN; ++n){ actcell.index[n] = int(D) * tmpcell[0].index[n] + (1 - int(D)) * defcell.index[n]; }

	}
	
	// (-- layer 2: output --) output an arbitrary result from the captured actual region(s)

	gl_FragDepth = actcell.depth[0];									// simply output
	outindex = actcell.index[0];

}
