#pragma once

/* ** EXPLANATION **

	Ray class handles GPU memory management and processing through OpenGL.

	Function Call(..) is implemented in main.cpp, and Ray class is implemented in Ray.cpp.

*/

namespace ray { 
	
	void Call(float outpos[3], float outtheta[3]);						// get inputs that GLFW get
	
	class Ray;

}

class ray::Ray {

	/*
		In this class:

			- Dynamically initialize and release Ray Units/Objects and process their ray calculations.

			- Statically initialize and release GPU environment for ray calculations.

	*/

	struct Table;														// contain ray units/objects

	Table* table;

public:

	Ray(void);															// init ray units/objects

	void Update(void);													// process ray calc for a frame
	
	~Ray(void);															// release ray units/objects


	// ** static *******************************

	static void Initialize(void);										// init Gpu memory and shaders for ray calc

	static void Release(void);											// release Gpu memory and shaders

};
