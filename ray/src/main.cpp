
/* ** PROJECT EXPLANATION **

    This project demonstrates the "Double Buffered Ray Tracing" method, processing the Ray2 and Selection calculations for Ray Units/Objects.

    Rays are used as camera rays in this demonstration.

    The results of the calculations appear as Ray Units/Objects drawn on the screen.

    Libraries as GLFW, GLEW, OpenGL are referenced.


    Edit shaders in directory '/src/sh' to modify the ray calculations.

    Type, number, parameters of Ray Units/Objects can be edited in the Ray class constructor in Ray.cpp.


    (An overview of the processing flow and buffer definitions is provided in a separate PDF ("RayCalcWorkflow.pdf").)

*/

#include <stdio.h>
#include <stdlib.h>

#include <glew.h>
#include <glfw3.h>

#define SUCCESS 1

// *****************************************
//  main
// *****************************************

#include "Ray.h"                                                        // namespace ray and class ray::Ray are declared here


static int Initialize(void);

static int Update(double time); 

static void Release(void);


int main(void) {

    /*
        Initialize GLEW, GLFW and OpenGL for ray calculations.
    
        Initialize Ray Units/Objects and process their calculations.
    */

    // ** Initialize ***************************

    if(Initialize()) {                                                  // init GLFW, GLEW, OPENGL and input callbacks

        ray::Ray::Initialize();                                         // init Gpu memory and shaders for ray calc

        {
            ray::Ray ray;                                               // init ray units/objects

            // ** Update *******************************

            printf("# Running while Loop.\n");

            while (Update(glfwGetTime())) {

                ray.Update();                                           // process ray calc for a frame

            }
    
            // ** Release ******************************

        }                                                               // release ray units/objects

        ray::Ray::Release();                                            // release Gpu memory and shaders
    
    }

    Release();                                                          // release OpenGL and GLFW


    return 0;

}



static GLFWwindow* window = NULL;

// *****************************************
//  Initialize
// *****************************************

#include "constant.h"


static void error_callback(int error, const char* description);

static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);

static void cursor_callback(GLFWwindow* window, double xpos, double ypos);


static int Initialize(void) {

    /*
        Initialize GLFW, GLEW and callbacks to get input information.
    */

    // ** Initialize GLFW **********************

    glfwSetErrorCallback(error_callback);

    if (!glfwInit()) { printf("Error: GLFW Init failure.\n"); return !SUCCESS; }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);                      // MacOS supports up to OpenGL 4.1 and the code is implemented accordingly

    static GLFWwindow* id = 0; id = glfwCreateWindow(PIXELS_W, PIXELS_H, "window", NULL, NULL);
    window = id;

    if (!id) { printf("Error: GLFW create window failure.\n"); return !SUCCESS; }

    glfwMakeContextCurrent(id);

    glfwSwapInterval(1);                                                // enable vertical sync

    glfwSetKeyCallback(id, key_callback);
    glfwSetCursorPosCallback(id, cursor_callback);

    // ** Initialize GLEW **********************

    glewExperimental = GL_TRUE;

    if (glewInit() != GLEW_OK) { printf("Error: GLEW Init failure.\n"); return !SUCCESS; }


    return SUCCESS;

}


// *****************************************
//  Update
// *****************************************

static const int MAXDISPTIME = 1000;

static double prevtime = -MAXDISPTIME;

static int Update(double time) {

    /* Update GLFW and process inputs. */

    if (glfwWindowShouldClose(window)) return !SUCCESS;

    glfwSwapBuffers(window);                                            // update with vert sync ON by default
    glfwPollEvents();

    double difftime = (time - prevtime) * 1000.0;

    char str[6] = "---"; if (difftime < MAXDISPTIME) snprintf(str, sizeof(str), "%05.1lf", difftime);
    printf("\r%s", str);                                                // output elapsed time (vert sync is ON by default)
    

    prevtime = time;


    return SUCCESS;

}


// *****************************************
//  Release
// *****************************************

static void Release(void) {

    /* Release GLFW. */

    glfwDestroyWindow(window); window = NULL;

    glfwTerminate();

}


static  float theta[3] = {}, pos[3] = {};                               // theta: cam orientation, pos: cam pos

// *****************************************
//  ray::Call(..)
// *****************************************

/* 
    The function is declared in Ray.h and implemented here.
*/

void ray::Call(float outpos[3], float outtheta[3]) {

    /* Output values for camera positions and orientations. */

    outtheta[0] = theta[0]; outtheta[1] = theta[1]; outtheta[2] = theta[2];

    outpos[0] = pos[0]; outpos[1] = pos[1]; outpos[2] = pos[2];

}


// *****************************************

// *****************************************

static void error_callback(int errcode, const char* description)
    { printf("GLFW Error: %s (%d)\n", description, errcode);  glfwSetWindowShouldClose(window, 1); }


// *****************************************

// *****************************************

static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {

    /* Determine camera positions from key inputs. */

    const float v = 2.1f;                                               // arbitrary value

    float tmp[3] = {}; {

        if (action == GLFW_PRESS || action == GLFW_REPEAT)
        {
            tmp[0] = v * ((key == GLFW_KEY_RIGHT) - (key == GLFW_KEY_LEFT));
            tmp[1] = v * ((key == GLFW_KEY_UP) - (key == GLFW_KEY_DOWN));
            tmp[2] = v * ((key == GLFW_KEY_LEFT_ALT) - (key == GLFW_KEY_LEFT_CONTROL));
        }

    }

    pos[0] += tmp[0]; pos[1] += tmp[1]; pos[2] += tmp[2];

}

static void cursor_callback(GLFWwindow* window, double xpos, double ypos) {

    /* Determine camera orientations from mouse inputs. */

    float tmp[3] = { 5.0f * ((float)ypos / PIXELS_H - 0.5f), 0.0f, 5.0f * ((float)xpos / PIXELS_W - 0.5f) };

    theta[0] = tmp[0]; theta[1] = tmp[1]; theta[2] = tmp[2];

}
