/*
 * Copyright 1993-2010 NVIDIA Corporation.  All rights reserved.
 *
 * Please refer to the NVIDIA end user license agreement (EULA) associated
 * with this source code for terms and conditions that govern your use of
 * this software. Any use, reproduction, disclosure, or distribution of
 * this software and related documentation outside the terms of the EULA
 * is strictly prohibited.
 *
 */
 
 // Simple class to contain GLSL shaders/programs

#ifndef GLSL_PROGRAM_H
#define GLSL_PROGRAM_H

#include <GL/glew.h>
#include <stdio.h>

class GLSLProgram
{
public:
    // construct program from strings
    GLSLProgram(const char*progName=NULL);
	GLSLProgram(const char *vsource, const char *fsource);
    GLSLProgram(const char *vsource, const char *gsource, const char *fsource,
                GLenum gsInput = GL_POINTS, GLenum gsOutput = GL_TRIANGLE_STRIP, int maxVerts=4);

	~GLSLProgram();

	void enable();
	void disable();

	void setUniform1f(const GLchar *name, GLfloat x);
	void setUniform2f(const GLchar *name, GLfloat x, GLfloat y);
    void setUniform2fv(const GLchar *name, float *v) { setUniformfv(name, v, 2, 1); }
    void setUniform3f(const GLchar *name, float x, float y, float z);
    void setUniform3fv(const GLchar *name, float *v) { setUniformfv(name, v, 3, 1); }
    void setUniform4f(const GLchar *name, float x, float y=0.0f, float z=0.0f, float w=0.0f);
	void setUniformfv(const GLchar *name, GLfloat *v, int elementSize, int count=1);
    void setUniformMatrix4fv(const GLchar *name, GLfloat *m, bool transpose);

	void setUniform1i(const GLchar *name, GLint x);
	void setUniform2i(const GLchar *name, GLint x, GLint y);
    void setUniform3i(const GLchar *name, int x, int y, int z);

	void bindTexture(const GLchar *name, GLuint tex, GLenum target, GLint unit);
	void bindImage  (const GLchar *name, GLint unit, GLuint tex, GLint level, GLboolean layered, GLint layer, GLenum access, GLenum format);

	inline GLuint getProgId() { return mProg; }
	
    GLuint compileProgram(const char *vsource, const char *gsource, const char *fsource,
                          GLenum gsInput = GL_POINTS, GLenum gsOutput = GL_TRIANGLE_STRIP, int maxVerts=4);
    GLuint compileProgramFromFiles(const char *vFilename,  const char *gFilename, const char *fFilename,
                       GLenum gsInput = GL_POINTS, GLenum gsOutput = GL_TRIANGLE_STRIP, int maxVerts=4);
    void setShaderNames(const char*ProgName, const char *VSName=NULL,const char *GSName=NULL,const char *FSName=NULL);
    static bool setIncludeFromFile(const char *includeName, const char* filename);
    static void setIncludeFromString(const char *includeName, const char* str);
private:
    static char *readTextFile(const char *filename);
    char *curVSName, *curFSName, *curGSName, *curProgName;

	GLuint mProg;
    static char* incPaths[];
};

#endif