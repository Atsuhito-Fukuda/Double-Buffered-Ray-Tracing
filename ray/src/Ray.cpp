
/* ** EXPLANATION **

    Ray class handles GPU memory management and processing through OpenGL.

    Ray calculations and renderings are processed through (FBO) frame buffers.

*/

#include <iostream>

#include <glew.h>
#include <glfw3.h>

#include "Ray.h"                                                        // namespace ray, class ray::Ray declared here

#include "constant.h"


using namespace ray;


#define MAXUNITBUFFSIZE 20                                              // max size of (ubo) units
#define MAXPLANEBUFFSIZE 200                                            // max size of (ubo) plane buffers

#define  MAXOBJECTSIZE 5                                                // max size of ray objects per table
#define MAXUNITSIZE 3                                                   // max size of ray units per ray object

// *****************************************
//  Table
// *****************************************

struct Unit {

    /* This structure contains planes which construct a Ray Unit. */

    int plstart = 0, plsize = 0;                                        // plstart: plane start index of ray unit in (ubo) plane buffer

};

struct Object {

    /* This structure contains Ray Units and attribute data which construct a Ray Object. */

    void(*updatemat4)(float[4][4]) = nullptr;                           // func to update model matrix

    float modelmat4[4][4] = {};                                         // model matrix of the ray object

    int unitstart = 0, unitsize = 0;                                    // unitstart: ray unit start index in (ubo) unit buffer
    Unit unit[MAXUNITSIZE] = {};

};

struct Ray::Table {

    /* This structure contains Ray Objects. */

    int objectsize = 0; Object object[MAXOBJECTSIZE] = {};

};


// *****************************************
//  Update
// *****************************************

static void GL_UnitBuffer_Reset(int unitstartindex, const float modelmat4[4][4]);

static void GL_UnitBuffer_Update(void);

static void GL_Ray2Buffer_Initialize(int unitstartindex);

static void GL_Ray2Buffer_Update(const Unit* unit);

static void GL_Ray2Buffer_Release(void);

static void GL_SelectionBuffer_Reset(void);

static void GL_SelectionBuffer_Update(void);

static void GL_DrawBuffer_Update(void);

static bool GL_CheckError(void);


static void SetNewFrame(void);

static void SetViewmat4(void);

static bool IsFirstUpdError(void);


void Ray::Update(void) {

    /*
        Update Ray Object's movements and process Ray2 Calculations for each Unit, Selection Calculations for each Object, and finally render to screen.
    */

    SetNewFrame();                                                      // advance to the next frame.

    SetViewmat4();                                                      // calc view matrix

    // ** update ray object's movements ********

    for (int n = 0; n < table->objectsize; ++n) {

        table->object[n].updatemat4(table->object[n].modelmat4);        // update ray object's model matrix

    }

    // upload ray unit's model matrices to Gpu

    for (int n = 0; n < table->objectsize; ++n) {

        GL_UnitBuffer_Reset(table->object[n].unitstart, table->object[n].modelmat4);    // init for uploading model matrices

        for (int m = 0; m < table->object[n].unitsize; ++m) {

            GL_UnitBuffer_Update();                                                     // upload the ray object's model matrix for the ray unit's matrix

        }

    }

    // ** ray2 and selection calc **************

    GL_SelectionBuffer_Reset();                                         // init for selction calc

    // process through all ray objects/units

    for (int n = 0; n < table->objectsize; ++n) {

        GL_Ray2Buffer_Initialize(table->object[n].unitstart);           // init for ray2 calc

        for (int m = 0; m < table->object[n].unitsize; ++m) {

            GL_Ray2Buffer_Update(&table->object[n].unit[m]);            // init and process ray2 calc for a ray unit 

        }

        GL_Ray2Buffer_Release();                                        // release for ray2 calc

        GL_SelectionBuffer_Update();                                    // process selection calc for a ray object

    }

    // ** render to screen *********************    

    GL_DrawBuffer_Update();


    if (GL_CheckError() && IsFirstUpdError()) { std::cout << "\rError: Error has been confirmed in update process.\n."; }

}


// *****************************************
//  Constructor 
// *****************************************

#include <vector>
#include "Unit.h"

namespace data { struct Unit; }

struct data::Unit {
    /*
        This structure contains the attribute data of a Ray Unit to be passed to uploading process.
        (The data structure is the same as in (UBO) Unit Buffer.)
    */
    float modelmat4[4][4] = {};                                         // model matrix of ray unit
    float texscale[2] = { 1.0f, 1.0f }, padding[2] = {};                // texscale: scale factor of image texture (use negative val to flip tex)
};


static void GL_PlaneBuffer_Reset(const float(*plbuff)[POINTS_PER_UNIT], unsigned int plbufflines);

static void GL_UnitBuffer_Reset(const data::Unit* unitbuff, unsigned int unitbuffsize);


static int getplbuffindex(void);

static void setnewplbuffindex(int increase);

static int getunitbuffindex(void);

static void setunitbuffindex(int increase);

static void updatemat4_default(float modelmat4[4][4]);

static void updatemat4(float modelmat4[4][4]);


Ray::Ray(void) : table(new Table) {

    /*
        Initialize and upload Ray Units/Objects from arbitrary plane and attribute data.
    */

    struct Mat4 { /* This structure contains a 4x4 matrix. */ float mat4[4][4] = {}; };

    struct Unit {
        /* This structure contains planes and attribute data which construct a Ray Unit. */
        int linesize = 0; float(*buff)[POINTS_PER_UNIT] = nullptr; data::Unit att = {};
    };

    struct Object {
        /* This structure contains Ray Units and attribute data which construct a Ray Object. */
        Unit unit[MAXUNITSIZE] = {}; Mat4 modelmat = {}; void(*updatemat4)(float[4][4]) = nullptr;
    };

    const Mat4 initalmodelmat4 = { 1.0f, 0.0f, 0.0f, 0.0f,  0.0f, 1.0f, 0.0f, 3.0f,  0.0f, 0.0f, 1.0f, -1.2f,  0.0f, 0.0f, 0.0f, 1.0f };


    Object object[MAXOBJECTSIZE] = {                                        // arbitrary unit/object data to init and upload

        { // - object 0 -
            {
                { sizeof(cubeunit) / POINTS_PER_UNIT / sizeof(float), cubeunit, { {}, { 1.0f, 1.0f }, } },

                { sizeof(suboctahedronunit) / POINTS_PER_UNIT / sizeof(float), suboctahedronunit, { {}, { 1.0f, -1.0f }, } },
            },

            initalmodelmat4, updatemat4
        },

        { // - object 1 -
            {
                { sizeof(largecubeunit) / POINTS_PER_UNIT / sizeof(float), largecubeunit, { {}, { 100.0f, 100.0f }, } },

                { sizeof(largesubcubeunit) / POINTS_PER_UNIT / sizeof(float), largesubcubeunit, { {}, { -100.0f, 100.0f }, } },
            },

            initalmodelmat4, updatemat4_default
        },

    };

    int objectsize = 0;
    std::vector<float> plbuff; std::vector<data::Unit> unitbuff;            // buffs for uploading ray unit plane and attribute data to Gpu

    unitbuff.reserve(MAXUNITBUFFSIZE); plbuff.reserve(MAXPLANEBUFFSIZE * POINTS_PER_UNIT);

    // process through ray unit/object data

    for (int n = 0; n < MAXOBJECTSIZE; ++n) {

        if (object[n].unit[0].linesize == 0) break;


        int unitsize = 0;
        std::vector<data::Unit> tmpunitbuff; std::vector<float> tmpplbuff;  // segmented buffs for uploading ray unit plane and attribute data

        tmpplbuff.reserve(MAXPLANEBUFFSIZE * POINTS_PER_UNIT); tmpunitbuff.reserve(MAXUNITSIZE);

        for (int m = 0; m < MAXUNITSIZE; ++m) {

            if (object[n].unit[m].linesize == 0) break;

            table->object[n].unit[m].plsize = object[n].unit[m].linesize;

            table->object[n].unit[m].plstart = getplbuffindex();

            setnewplbuffindex(object[n].unit[m].linesize);                  // increase the plane buffer index


            for (int i = 0; i < POINTS_PER_UNIT * object[n].unit[m].linesize; ++i) { tmpplbuff.push_back(((float*)object[n].unit[m].buff)[i]); }

            tmpunitbuff.push_back(object[n].unit[m].att);

            ++unitsize;

        }

        table->object[n].unitsize = unitsize;

        table->object[n].unitstart = getunitbuffindex();

        setunitbuffindex(unitsize);                                         // increase the unit buffer index


        for (const auto& elm : tmpunitbuff) { unitbuff.push_back(elm); }

        for (auto elm : tmpplbuff) { plbuff.push_back(elm); }


        for (int u = 0; u < 16; ++u) { table->object[n].modelmat4[u / 4][u % 4] = object[n].modelmat.mat4[u / 4][u % 4]; }

        table->object[n].updatemat4 = object[n].updatemat4;


        ++objectsize;

    }

    table->objectsize = objectsize;

    // upload the data to Gpu

    if (plbuff.size() > POINTS_PER_UNIT * MAXPLANEBUFFSIZE) { std::cout << "Error: Plane buffer size error.\n"; }

    GL_PlaneBuffer_Reset(
        (float(*)[POINTS_PER_UNIT])plbuff.data(),                           // upload ray unit plane data
        (unsigned int)plbuff.size() / POINTS_PER_UNIT
    );

    if (unitbuff.size() > MAXUNITBUFFSIZE) { std::cout << "Error: Ray unit buffer size error.\n"; }

    GL_UnitBuffer_Reset(
        unitbuff.data(),                                                    // upload ray unit attribute data
        (unsigned int)unitbuff.size()
    );


    if (GL_CheckError()) { std::cout << "Error: Error has been confirmed in constructor.\n."; }

}


// *****************************************
//  Destructor
// *****************************************

Ray::~Ray(void) { delete table; }


// *****************************************
//  Initialize
// *****************************************

static void GL_LoadShader(void);

static void GL_LoadUbo(void);

static void GL_LoadFbo(void);

static void GL_LoadCamRay(void);

static void GL_LoadImgData(void);

static void GL_LoadScreenRender(void);


void Ray::Initialize(void) {

    /*
        Initialize shaders, UBO buffers, FBO frame buffers, camera ray data, image data and screen rendering.
    */


    glEnable(GL_DEPTH_TEST);

    // ** Initialize shaders *******************

    GL_LoadShader();

    // ** Initialize ubo buffers ***************

    GL_LoadUbo();

    // ** Initialize fbo frame buffers *********

    GL_LoadFbo();

    // ** Initialize camera ray ****************

    GL_LoadCamRay();

    // ** Initialize image data ****************

    GL_LoadImgData();

    // ** Initialize screen rendering **********

    GL_LoadScreenRender();


    if (GL_CheckError()) { std::cout << "Error: Error has been confirmed in initialization process.\n."; }

}


// *****************************************
//  Release
// *****************************************

static void GL_UnLoadScreenRender(void);

static void GL_UnLoadImgData(void);

static void GL_UnLoadCamRay(void);

static void GL_UnLoadFbo(void);

static void GL_UnLoadUbo(void);

static void GL_UnLoadShader(void);


void Ray::Release(void) {

    /*
        Release shaders, UBO buffers, FBO frame buffers, camera ray data, image data and screen rendering.
    */

    // ** Release screen rendering **********  

    GL_UnLoadScreenRender();

    // ** Release image data ****************

    GL_UnLoadImgData();

    // ** Release camera ray ****************

    GL_UnLoadCamRay();

    // ** Release fbo frame buffers *********

    GL_UnLoadFbo();

    // ** Release ubo buffers ***************

    GL_UnLoadUbo();

    // ** Release shaders *******************

    GL_UnLoadShader();


    if (GL_CheckError()) { std::cout << "Error: Error has been confirmed in release process.\n."; }

}



//*************************************************************

//  Environment

//*************************************************************

static GLuint

    dummyvao = 0,                                                               // dummy vao opengl object (needed to render)

    imgtex = 0,                                                                 // textures

    camraypostex = 0, camraydirtex = 0,

    ray2fbo = 0, selectfbo = 0,                                                 // fbo frame buffers

    ubounitbuff = 0, uboplbuff = 0,                                             // ubo buffers

    ray2initprgm = 0, ray2prgm = 0, selectprgm = 0, drawprgm = 0;               // shader programs



static float ubomodelmat4[4][4] = {};                                   // retained model matrix of a ray object
static int ubooffset = 0, ubounitstartindex = 0;                        // ubooffset: an offset of the ubo unit buff
                                                                        // ubounitstartindex: a start index in the ubo unit buff

static void GL_UnitBuffer_Reset(int unitstartindex, const float modelmat4[4][4]) {

    /* Initialize for uploading model matrices to the UBO Unit Buffer. */

    ubounitstartindex = unitstartindex;
    ubooffset = 0;

    for (int i = 0; i < 16; ++i) { ubomodelmat4[i / 4][i % 4] = modelmat4[i / 4][i % 4]; }

}

static void GL_UnitBuffer_Update(void) {

    /* Upload a model matrix to the UBO Unit buffer. */

    glBindBuffer(GL_UNIFORM_BUFFER, ubounitbuff);
    glBufferSubData(
        GL_UNIFORM_BUFFER,
        sizeof(data::Unit) * (ubooffset + ubounitstartindex),
        sizeof(data::Unit::modelmat4),
        ubomodelmat4
    );

    glBindBuffer(GL_UNIFORM_BUFFER, NULL);


    ++ubooffset;

}

static int ray2offset = 0, ray2unitstartindex = 0;                      // ray2offset: num of calls for ray2 calc
                                                                        // ray2unitstartindex: a start index in the ubo unit buff

static void GL_Ray2Buffer_Initialize(int unitstartindex) {

    /* Initialize the Ray2 FBO Frame Buffer before processing Ray2 Calculations for each Ray Unit. */

    glBindFramebuffer(GL_FRAMEBUFFER, ray2fbo);

    glBindVertexArray(dummyvao);                                        // dummy (needed to render)

    glClearDepth(1.0f);                                                 // clear fbo ray2 depth buff to 1.0
    glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

    ray2unitstartindex = unitstartindex;
    ray2offset = 0;

}

static const float (*GetViewmat4(void))[4][4];

static void GL_Ray2Buffer_Update(const Unit* unit) {

    /* Initialize a Ray Unit Segment of the Ray2 FBO Frame Buffer and process a Ray2 Calculation for a Ray Unit. */

    #define  UNIFORMSIZE 3

    struct Uniform {
        /* This structure contains an Uniform GLSL int variable. */
        int val = 0; const char* name = nullptr;
    };

    {
        glDepthFunc(GL_ALWAYS);                                                             // init a ray unit segment of the fbo ray2 buff

        glUseProgram(ray2initprgm);

        glUniform1i(glGetUniformLocation(ray2initprgm, "unitsegment"), ray2offset);         // specify a ray unit segment of the fbo ray2 buff
        glDrawArrays(GL_TRIANGLES, 0, 6);                                                   // init a ray unit segment of the fbo ray2 depth buff to 0.0

        glUseProgram(NULL);
    }

    // process ray2 calc

    glDepthFunc(GL_GEQUAL);

    glUseProgram(ray2prgm);

    Uniform uniform[UNIFORMSIZE] = {
        { unit->plstart, "plstart" },
        { ray2offset + ray2unitstartindex, "unitindex" },
        { ray2offset, "unitsegment" }                                                       // specify a ray unit segment of the fbo ray2 buff
    };

    for (int i = 0; i < UNIFORMSIZE; ++i) { glUniform1i(glGetUniformLocation(ray2prgm, uniform[i].name), uniform[i].val); }

    glUniformMatrix4fv(glGetUniformLocation(ray2prgm, "viewmat4"), 1, GL_TRUE, (const GLfloat*)GetViewmat4());

    glDrawArrays(GL_TRIANGLES, 0, 6 * unit->plsize);

    glUseProgram(NULL);


    ++ray2offset;

}

static void GL_Ray2Buffer_Release(void) {
    glBindVertexArray(NULL);
    glBindFramebuffer(GL_FRAMEBUFFER, NULL);
}

static void GL_SelectionBuffer_Reset(void) {

    /* Initialize the Selection FBO Frame Buffer before processing Selection Calculations for each Ray Object. */

    glBindFramebuffer(GL_FRAMEBUFFER, selectfbo);
    glBindVertexArray(dummyvao);                                        // dummy (needed to render)

    glClearDepth(1.0f);
    glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);


    glBindVertexArray(NULL);
    glBindFramebuffer(GL_FRAMEBUFFER, NULL);

}

static void GL_SelectionBuffer_Update(void) {

    /* Process a Selection calculation for a Ray Object. */

    glBindFramebuffer(GL_FRAMEBUFFER, selectfbo);
    glBindVertexArray(dummyvao);                                        // dummy (needed to render)

    glDepthFunc(GL_LESS);
    glUseProgram(selectprgm);

    glDrawArrays(GL_TRIANGLES, 0, 6);


    glUseProgram(NULL);

    glBindVertexArray(NULL);
    glBindFramebuffer(GL_FRAMEBUFFER, NULL);

}

static void GL_DrawBuffer_Update(void) {

    /* Render the results to the screen. */

    glBindVertexArray(dummyvao);                                        // dummy (needed to render)
    glUseProgram(drawprgm);

    glClearDepth(1.0f);
    glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

    glDepthFunc(GL_ALWAYS);

    glUniformMatrix4fv(glGetUniformLocation(drawprgm, "viewmat4"), 1, GL_TRUE, (const GLfloat*)GetViewmat4());

    glDrawArrays(GL_TRIANGLES, 0, 6);


    glUseProgram(NULL);
    glBindVertexArray(NULL);

}

static void GL_PlaneBuffer_Reset(const float (*plbuff)[POINTS_PER_UNIT], unsigned int plbufflines) {

    /* Upload Ray Unit plane data to the UBO Plane Buffer.  */

    glBindBuffer(GL_UNIFORM_BUFFER, uboplbuff);
    glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(float) * POINTS_PER_UNIT * plbufflines, plbuff);


    glBindBuffer(GL_UNIFORM_BUFFER, NULL);

}

static void GL_UnitBuffer_Reset(const data::Unit* unitbuff, unsigned int unitbuffsize) {

    /* Upload Ray Unit attribute data to UBO Unit Buffer.  */

    glBindBuffer(GL_UNIFORM_BUFFER, ubounitbuff);
    glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(data::Unit) * unitbuffsize, unitbuff);


    glBindBuffer(GL_UNIFORM_BUFFER, NULL);

}


// *****************************************

// *****************************************

#include <cstring>
#include <string>

#define MAXSHADERTYPE 3
#define PROGRAMSIZE 4

struct FileRead {

    /* This structure contains file text. */

    std::string str;

    FileRead(void);
    FileRead(const char* file);                                         // read a file

};

static bool GL_CheckLinkError(GLuint id);


static const char* const& getlval(const char* cchar);


static void GL_LoadShader(void) {

    /*
        Initialize the shaders and upload them to GPU.
    */

    #define MAXCHARSIZE 32

    struct Shader {
        /* This structure contains a list of shaders to be compiled and linked together. */
        GLuint* id = nullptr; const char file[MAXSHADERTYPE][MAXCHARSIZE] = {};
    };

    const GLenum shadertype[MAXSHADERTYPE] = { GL_VERTEX_SHADER, GL_GEOMETRY_SHADER, GL_FRAGMENT_SHADER };


    Shader prgmlist[PROGRAMSIZE] = {                                    // shaders for initializing ray2 calc. ray2 calc, selection calc and drawing (screen rendering)
        { &ray2initprgm, { "src/sh/common.vert", "src/sh/ray2init.geom", "src/sh/ray2init.frag" } },
        { &ray2prgm, { "src/sh/common.vert", "src/sh/ray2.geom", "src/sh/ray2.frag" } },
        { &selectprgm, { "src/sh/common.vert", "src/sh/common.geom", "src/sh/select.frag" } },
        { &drawprgm, { "src/sh/common.vert", "src/sh/common.geom", "src/sh/draw.frag" } }
    };

    // process through shaders

    for (int i = 0; i < PROGRAMSIZE; ++i) {

        static GLuint prgmid = 0; prgmid = glCreateProgram();
        *prgmlist[i].id = prgmid;

        GLuint shidlist[MAXSHADERTYPE] = {}; for (int j = 0; j < MAXSHADERTYPE; ++j) {      // shader ids
        
            static GLuint shid = 0; shid = glCreateShader(shadertype[j]);
            shidlist[j] = shid;

            glAttachShader(prgmid, shid);
        }

        FileRead frlist[MAXSHADERTYPE] = {}; for (int j = 0; j < MAXSHADERTYPE; ++j) {      // shader file strings
            frlist[j] = FileRead(prgmlist[i].file[j]);
        }

        for (int j = 0; j < MAXSHADERTYPE; ++j) {

            glShaderSource(shidlist[j], 1, &getlval(frlist[j].str.data()), nullptr);
            glCompileShader(shidlist[j]);
        }

        glLinkProgram(prgmid);


        if (GL_CheckLinkError(prgmid)) {                                // check link error

            char* shlist = new char[MAXSHADERTYPE * MAXCHARSIZE + 1](); {

                char (*str)[1 + MAXCHARSIZE] = new char[MAXSHADERTYPE][1 + MAXCHARSIZE]();
                for (int j = 0; j < MAXSHADERTYPE; ++j) { snprintf(str[j], sizeof(str[j]), " %s", prgmlist[i].file[j]); }

                int shlistptr = 0;

                size_t len[MAXSHADERTYPE]; for (int j = 0; j < MAXSHADERTYPE; ++j) { len[j] = std::strlen(str[j]); }

                for (int j = 0; j < MAXSHADERTYPE; ++j) {
                    for (int k = 0; k < len[j]; ++k) { shlist[shlistptr++] = str[j][k]; }
                }


                delete[] str;
            }

            GLchar* buff = new GLchar[512](); glGetProgramInfoLog(prgmid, 512 - 1, nullptr, buff);
            std::cout << "Shader Link Error:" << shlist << ":\n" << buff << "\n";


            delete[] buff; delete[] shlist;
        }

    }

}

static void GL_UnLoadShader(void) {

    /*
        Release the shaders on GPU.
    */

    GLuint* prgmlist[PROGRAMSIZE] = { &ray2initprgm, &ray2prgm, &selectprgm, &drawprgm };

    for (int n = 0; n < PROGRAMSIZE; ++n) {

        GLsizei attlen = 0; GLuint att[MAXSHADERTYPE] = {};
        glGetAttachedShaders(*prgmlist[n], MAXSHADERTYPE, &attlen, att);


        for (int m = 0; m < attlen; ++m) { glDeleteShader(att[m]); }

        glDeleteProgram(*prgmlist[n]); *prgmlist[n] = 0;

    }

}


#define UBOSIZE 2

static int getuboindex(void);

static void setnewuboindex(void);


static void GL_LoadUbo(void) {

    /*
        Initialize the UBO buffers on GPU.
    */

    #define BINDSHADERSIZE 2

    struct Ubo {
        /* This structure contains an UBO buffer to be allocated in GPU. */
        GLuint* id = nullptr; int datasize = 0; const char* name = nullptr;
    };


    Ubo ubolist[UBOSIZE] = {                                                           // ubo buffers for unit/plane buffers
        { &ubounitbuff, sizeof(data::Unit) * MAXUNITBUFFSIZE, "UboUnitBuffer" },
        { &uboplbuff, sizeof(float) * POINTS_PER_UNIT * MAXPLANEBUFFSIZE, "UboPlaneBuffer" }
    };

    // process through ubo buffers

    for (int n = 0; n < UBOSIZE; ++n) {

        static GLuint id = 0; glGenBuffers(1, &id);
        *ubolist[n].id = id;

        glBindBuffer(GL_UNIFORM_BUFFER, id);

        setnewuboindex();                                                               // set a new ubo binding point 
        glBindBufferBase(GL_UNIFORM_BUFFER, getuboindex(), id);
        glBufferData(GL_UNIFORM_BUFFER, ubolist[n].datasize, NULL, GL_DYNAMIC_DRAW);

        GLuint bindshader[BINDSHADERSIZE] = { ray2prgm, drawprgm };                    // shaders in which the above ubo buffers are read
        const char* tmpname = ubolist[n].name;

        for (int m = 0; m < BINDSHADERSIZE; ++m) { glUniformBlockBinding(bindshader[m], glGetUniformBlockIndex(bindshader[m], tmpname), getuboindex()); }


        glBindBuffer(GL_UNIFORM_BUFFER, NULL);
    }

}

static void GL_UnLoadUbo(void) {

    /*
        Release the UBO buffers on GPU.
    */

    GLuint* ubolist[UBOSIZE] = { &ubounitbuff, &uboplbuff };

    for (int n = 0; n < UBOSIZE; ++n) { glDeleteBuffers(1, ubolist[n]); *ubolist[n] = 0; }

}


#define FBOSIZE 2
#define TEXTSIZE 2

static int gettextindex(void);

static void setnewtextindex(void);


static void GL_LoadFbo(void) {

    /*
        Initialize the FBO frame buffers on GPU.
    */

    #define RAYSIDELEN 2

    struct Fbo {
        /* This structure contains a FBO frame buffer to be allocated in GPU. */
        GLuint* id = nullptr; GLuint passshader = 0; int layersize = 0;
    };

    struct Texture {
        /* This structure defines a texture buffer attached to a FBO frame buffer. */
        GLenum attachment = 0; GLint internalformat = 0; GLenum format = 0, type = 0;  const char* name = nullptr;
    };

    const GLenum ColorAttachments = GL_COLOR_ATTACHMENT0;

    const Texture texdeflist[TEXTSIZE] = {                              // tex buff defs for depth/index buffs to be attached to fbo frame buffs
        { GL_DEPTH_ATTACHMENT, GL_DEPTH_COMPONENT32F, GL_DEPTH_COMPONENT, GL_FLOAT, "depthbuffer" },
        { GL_COLOR_ATTACHMENT0, GL_RG16F, GL_RG, GL_HALF_FLOAT, "indexbuffer" }
    };


    Fbo fbolist[FBOSIZE] = {                                            // fbo frame buffers for ray2 and selection calculations
        { &ray2fbo, selectprgm, RAYSIDELEN * MAXUNITSIZE },
        { &selectfbo, drawprgm, 1 }
    };

    // process through fbo frame buffers

    for (int n = 0; n < FBOSIZE; ++n) {

        static GLuint id = 0; glGenFramebuffers(1, &id);
        *fbolist[n].id = id;

        glBindFramebuffer(GL_FRAMEBUFFER, id);

        glDrawBuffers(1, &ColorAttachments);

        GLuint tmppassshader = fbolist[n].passshader; int tmplayersize = fbolist[n].layersize;

        for (int m = 0; m < TEXTSIZE; ++m) {

            setnewtextindex();                                                  // set a new texture unit index
            glActiveTexture(GL_TEXTURE0 + gettextindex());

            static GLuint texID = 0; glGenTextures(1, &texID);
            glBindTexture(GL_TEXTURE_2D_ARRAY, texID);

            glFramebufferTexture(GL_FRAMEBUFFER, texdeflist[m].attachment, texID, 0);

            glTexImage3D(                                                       // attach depth/index tex buffs to each fbo frame buffer
                GL_TEXTURE_2D_ARRAY, 0, texdeflist[m].internalformat, PIXELS_W, PIXELS_H,
                tmplayersize, 0, texdeflist[m].format, texdeflist[m].type, nullptr
            );
            glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAX_LEVEL, 0);      // mip 0

            glUseProgram(tmppassshader);

            glUniform1i(glGetUniformLocation(tmppassshader, texdeflist[m].name), gettextindex());


            glUseProgram(NULL);

        }


        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
            { std::cout << "Error: FBO frame buffer initialize error.\n"; }


        glBindFramebuffer(GL_FRAMEBUFFER, NULL);

    }

}

static void GL_UnLoadFbo(void) {

    /*
        Release the FBO frame buffers on GPU.
    */

    const GLenum att[TEXTSIZE] = { GL_DEPTH_ATTACHMENT, GL_COLOR_ATTACHMENT0 };


    GLuint* fbolist[FBOSIZE] = { &ray2fbo, &selectfbo };

    for (int n = 0; n < FBOSIZE; ++n) {

        glBindFramebuffer(GL_FRAMEBUFFER, *fbolist[n]);

        for (int m = 0; m < TEXTSIZE; ++m) {

            static GLint id = 0; glGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER, att[m], GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME, &id);

            GLuint uid = id; glDeleteTextures(1, &uid);
        }


        glDeleteFramebuffers(1, fbolist[n]); *fbolist[n] = 0;

        glBindFramebuffer(GL_FRAMEBUFFER, NULL);

    }

}


#define RAYBUFFSIZE 2

static void makecamray(float screenraydir[PIXELS_H][PIXELS_W][4], float screenraypos[PIXELS_H][PIXELS_W][4]);


static void GL_LoadCamRay(void) {

    /*
        Initialize the camera rays and upload the data to GPU.
    */

    #define BINDSHADERSIZE 2

    struct Texture {
        /* This structure defines a texture buffer. */
        GLuint* id = nullptr; const float (*screen)[PIXELS_W][4] = nullptr;
    };

    const char* raybuffname[RAYBUFFSIZE] = { "raybuffer[0]", "raybuffer[1]" };


    float
        (*screenraydir)[PIXELS_W][4] = new float[PIXELS_H][PIXELS_W][4]{},
        (*screenraypos)[PIXELS_W][4] = new float[PIXELS_H][PIXELS_W][4]{};  // init screen pos and dir buffs for cam rays
    makecamray(screenraydir, screenraypos);

    Texture texlist[RAYBUFFSIZE] = {
        { &camraypostex, screenraypos }, { &camraydirtex, screenraydir }
    };

    // process through cam ray texts

    int raytexindex[RAYBUFFSIZE] = {}; for (int n = 0; n < RAYBUFFSIZE; ++n) {

        setnewtextindex();
        static int texindex = gettextindex();
        raytexindex[n] = texindex;

        glActiveTexture(GL_TEXTURE0 + texindex);

        static GLuint id = 0; glGenTextures(1, &id);
        *texlist[n].id = id;

        glBindTexture(GL_TEXTURE_2D, id);

        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, PIXELS_W, PIXELS_H, 0, GL_RGBA, GL_FLOAT, texlist[n].screen);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);            // mip 0

    }

    GLuint raytexbindshader[BINDSHADERSIZE] = { ray2prgm, drawprgm };       // shaders in which the above texture buffers are read

    for (int n = 0; n < BINDSHADERSIZE; ++n) {

        glUseProgram(raytexbindshader[n]);

        GLuint tmpraytexbindshader = raytexbindshader[n];
        int tmpraytexindex[2] = {}; for (int m = 0; m < 2; ++m) { tmpraytexindex[m] = raytexindex[m]; }

        for (int m = 0; m < RAYBUFFSIZE; ++m) {
            glUniform1iv(glGetUniformLocation(tmpraytexbindshader, raybuffname[m]), 1, &tmpraytexindex[m]);
        }


        glUseProgram(NULL);

    }


    delete[] screenraypos; delete[] screenraydir;

}

static void GL_UnLoadCamRay(void) {

    /* Release camera ray data on Gpu. */

    GLuint* texlist[RAYBUFFSIZE] = { &camraypostex, &camraydirtex };

    for (int n = 0; n < RAYBUFFSIZE; ++n) { glDeleteTextures(1, texlist[n]); *texlist[n] = 0; }

}


struct Image {

    /* This structure defines image data. */

    std::vector<unsigned char> buff; int channels = 0, height = 0, width = 0;

    Image(const char* file);                                            // read an image file

};


static void GL_LoadImgData(void) {

    /*
        Initialize the image for mapping and upload the data to GPU.
    */

    #define TEXFILTERSIZE 2

    const int filterlist[TEXFILTERSIZE] = { GL_TEXTURE_MIN_FILTER, GL_TEXTURE_MAG_FILTER };


    setnewtextindex();
    glActiveTexture(GL_TEXTURE0 + gettextindex());

    static GLuint id = 0; glGenTextures(1, &id);
    imgtex = id;

    glBindTexture(GL_TEXTURE_2D, id);

    Image img("src/pic3.png");                                          // the image is assumed to be in rgb/rgba format
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, img.width, img.height, 0, (img.channels == 4) ? GL_RGBA : GL_RGB, GL_UNSIGNED_BYTE, img.buff.data());

    for (int n = 0; n < TEXFILTERSIZE; ++n) { glTexParameteri(GL_TEXTURE_2D, filterlist[n], GL_LINEAR); }

    glUseProgram(drawprgm);
    glUniform1i(glGetUniformLocation(drawprgm, "img"), gettextindex());


    glUseProgram(NULL);

}

static void GL_UnLoadImgData(void) {
    /* Release the image data for mapping on Gpu. */
    glDeleteTextures(1, &imgtex); imgtex = 0;
}


static void GL_LoadScreenRender(void) {

    /* Initialize for screen rendering. */

    static GLuint id = 0; glGenVertexArrays(1, &id);
    dummyvao = id;                                                      // dummy vao opengl object

    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);                               // set BG color to black transparent

}

static void GL_UnLoadScreenRender(void) {
    /* Release for screen rendering. */
    glDeleteVertexArrays(1, &dummyvao); dummyvao = 0;
}

static unsigned int frame = 0;                                          // the current frame

static float GetFrame(void) {
    /* Return the current game time.  */
    return frame * 16.6667f;                                            // assuming 60 fps
}

static void SetNewFrame(void) { /* Tick one frame. */ ++frame; }

static bool firstupderror = true;

static bool IsFirstUpdError(void) { bool res = firstupderror; firstupderror = false; return res; }

static float viewmat4[4][4] = {};                                       // view matrix

static const float (*GetViewmat4(void))[4][4]{
    /* Return the view matrix. */
    return &viewmat4;
}

static void makeviewmat4(float outviewmat4[4][4], const float theta[3], const float pos[3]);

static void SetViewmat4(void) {

    /* Calculate and retain the view matrix. */

    float pos[3] = {}, theta[3] = {}; Call(pos, theta);                 // get cam pos and orientation

    float tmpmat4[4][4] = {}; makeviewmat4(tmpmat4, theta, pos);


    for (int i = 0; i < 16; ++i) { viewmat4[i / 4][i % 4] = tmpmat4[i / 4][i % 4]; }

}

static bool GL_CheckError(void) {

    /* Check if an error has occurred. */

    bool res = false; while (glGetError() != GL_NO_ERROR) { res = true; }

    return res;

}

static bool GL_CheckLinkError(GLuint id) {
    /* Check if a shader link error has occurred. */
    GLint status; glGetProgramiv(id, GL_LINK_STATUS, &status); return (status != GL_TRUE);
};


// *****************************************

// *****************************************

#include <fstream>

FileRead::FileRead(void) {}

FileRead::FileRead(const char* file) {

    /* Read a text file. */

    std::ifstream shaderfile(file);

    if (!shaderfile.is_open()) {
        std::cout << "CompileShader Error: cannot open file. (\"" << file << "\")\n"; return;
    }

    long long size;
    shaderfile.seekg(0, std::ios::end);
    size = shaderfile.tellg();

    std::string tmp(size, '\0');
    shaderfile.seekg(0, std::ios::beg);
    shaderfile.read(&tmp[0], size);


    this->str = tmp;

}

#include <SOIL.h>

Image::Image(const char* file) {

    /* Read an image file. */

    unsigned char* soilimagebuff = nullptr; int width = 0, height = 0, channels = 0;
    soilimagebuff = SOIL_load_image(file, &width, &height, &channels, SOIL_LOAD_AUTO);

    if (!soilimagebuff) { std::cout << "Error: Soil failed to load texture.\n"; return; }

    this->width = width; this->height = height; this->channels = channels;

    this->buff.reserve(width * height * channels);
    for (unsigned char* ptr = soilimagebuff; ptr < soilimagebuff + width * height * channels; ++ptr) { this->buff.push_back(*ptr); }

    SOIL_free_image_data(soilimagebuff);

}

static int plbuffindex = 0;                                             // current index of the ubo plane buffer

static int getplbuffindex(void) { return plbuffindex; }

static void setnewplbuffindex(int increase) {
    /* Increase the current index of the ubo plane buffer to write. */
    plbuffindex += increase;
}

static int unitbuffindex = 0;

static int getunitbuffindex(void) { return unitbuffindex; }

static void setunitbuffindex(int increase) {
    /* Increase the current index of the ubo unit buffer to write. */
    unitbuffindex += increase;
}

static void updatemat4_default(float modelmat4[4][4]) { /* Change nothing. */ }

#include <cmath>

static void updatemat4(float modelmat4[4][4]) {

    /* The ray object rotates around the local Z axis. */

    float tmpmat4[4][4] = {};  for (int i = 0; i < 16; ++i) { tmpmat4[i / 4][i % 4] = modelmat4[i / 4][i % 4]; }

    float time = GetFrame();
    tmpmat4[0][0] = std::cos(-time / 800.0f); tmpmat4[0][1] = -std::sin(-time / 800.0f);
    tmpmat4[1][0] = std::sin(-time / 800.0f); tmpmat4[1][1] = std::cos(-time / 800.0f);


    for (int i = 0; i < 16; ++i) { modelmat4[i / 4][i % 4] = tmpmat4[i / 4][i % 4]; }

}

static const char* contentschar = nullptr;                              // to convert an rvalue to an lvalue

static const char* const& getlval(const char* cchar) {
    /* Get lvalue reference. */ return (contentschar = cchar);
}

static int uboindex = 0;                                                // ubo binding point

static int getuboindex(void) { return uboindex; }

static void setnewuboindex(void) { /* Increment the ubo binding point. */ ++uboindex; }

static int textindex = 0;                                               // texture unit index

static int gettextindex(void) { return textindex; }

static void setnewtextindex(void) { /* Increment the texture unit index. */ ++textindex; }

float dot3(const float x[3], const float y[3]) {
    /* Dot product of 3 dim vectors. */
    float res = 0.0f; for (int i = 0; i < 3; ++i) { res += y[i] * x[i]; } return res;
}

static void makecamray(float screenraydir[PIXELS_H][PIXELS_W][4], float screenraypos[PIXELS_H][PIXELS_W][4]) {

    /* Make the camera ray data. */

    #define RAYBUFFELMSIZE 4 * PIXELS_W * PIXELS_H

    const float d = 2.0f / PIXELS_W * 1.0f;                             // angle of view = PI / 4 rad (= 1.0f)


    float* tmpray = new float[RAYBUFFELMSIZE] {};
    for (int n = 0; n < RAYBUFFELMSIZE / 4; ++n) {

        int wn = n % PIXELS_W, hn = n / PIXELS_W;

        float* tmprayelm = tmpray + (wn * 4 + hn * PIXELS_W * 4); {

            float tmp[4] = { d * (-PIXELS_W / 2.0f + wn + 0.5f), 1.0f, d * (-PIXELS_H / 2.0f + hn + 0.5f), 0.0f };

            for (int m = 0; m < 4; ++m) { tmprayelm[m] = tmp[m]; }

        }

    }


    // output screen ray pos buff

    for (int n = 0; n < RAYBUFFELMSIZE; ++n) { ((float*)screenraypos)[n] = tmpray[n]; }

    // output screen ray dir buff

    float (*ttmpray)[PIXELS_W][4] = (float (*)[PIXELS_W][4])tmpray;

    for (int i = 0; i < PIXELS_H; ++i) {

        for (int j = 0; j < PIXELS_W; ++j) {

            float tmp[4] = {}; for (int k = 0; k < 4; ++k) { tmp[k] = ttmpray[i][j][k]; }

            float length = std::sqrt(dot3(tmp, tmp));
            for (int k = 0; k < 3; ++k) { tmp[k] /= length; }


            for (int k = 0; k < 4; ++k) { screenraydir[i][j][k] = tmp[k]; }

        }

    }


    delete[] tmpray;

}

static void makeviewmat4(float outviewmat4[4][4], const float theta[3], const float pos[3]) {

    /* Get the camera position and orientation and make a view matrix. */

    const float theta_start = -2.0f * 3.141592f * 20.0f / 360.0f;       // arbitrary val


    float rot[3][3] = {                                                 // Rx(-(theta[0] + theta_start)) * Rz(-theta[2])
        { std::cos(theta[2]), std::sin(theta[2]), 0.0f },
        { -std::cos(theta[0] + theta_start) * std::sin(theta[2]), std::cos(theta[0] + theta_start) * std::cos(theta[2]), std::sin(theta[0] + theta_start) },
        { std::sin(theta[0] + theta_start) * std::sin(theta[2]), -std::sin(theta[0] + theta_start) * std::cos(theta[2]), std::cos(theta[0] + theta_start) }
    };

    // make a 4x4 view matrix from the rot matrix and the pos vector

    float tmpmat4[4][4] = {};
    for (int i = 0; i < 3; ++i) { tmpmat4[i][3] = -dot3(pos, rot[i]); }
    for (int i = 0; i < 9; ++i) { tmpmat4[i / 3][i % 3] = rot[i / 3][i % 3]; }


    for (int i = 0; i < 16; ++i) { outviewmat4[i / 4][i % 4] = tmpmat4[i / 4][i % 4]; }

}
