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
 
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include "main.h"
#include "GLSLProgram.h"

GLSLProgram::GLSLProgram() : mProg(0)
{
}

GLSLProgram::GLSLProgram(const char *vsource, const char *fsource)
{
    compileProgram(vsource, 0, fsource);
}

GLSLProgram::GLSLProgram(const char *vsource, const char *gsource, const char *fsource,
                         GLenum gsInput, GLenum gsOutput, int maxVerts)
{
    mProg = compileProgram(vsource, gsource, fsource, gsInput, gsOutput, maxVerts);
}

void
GLSLProgram::loadFromFiles(const char *vFilename,  const char *gFilename, const char *fFilename,
                           GLenum gsInput, GLenum gsOutput, int maxVerts)
{
    char *vsource = readTextFile(vFilename);
    char *gsource = 0;
    if (gFilename) {
        gsource = readTextFile(gFilename);
    }
    char *fsource = readTextFile(fFilename);

    mProg = compileProgram(vsource, gsource, fsource, gsInput, gsOutput, maxVerts);

    delete [] vsource;
    if (gsource) delete [] gsource;
    delete [] fsource;
}

GLSLProgram::~GLSLProgram()
{
	if (mProg) {
		glDeleteProgram(mProg);
	}
}

void
GLSLProgram::enable()
{
	glUseProgram(mProg);
}

void
GLSLProgram::disable()
{
	glUseProgram(0);
}

void
GLSLProgram::setUniform1f(const char *name, float value)
{
    GLint loc = glGetUniformLocation(mProg, name);
    if (loc >= 0) {
        glUniform1f(loc, value);
    } else {
#if _DEBUG
        LOGE("Error setting parameter '%s'\n", name);
#endif
    }
}

void
GLSLProgram::setUniform2f(const char *name, float x, float y)
{
    GLint loc = glGetUniformLocation(mProg, name);
    if (loc >= 0) {
        glUniform2f(loc, x, y);
    } else {
#if _DEBUG
        LOGE("Error setting parameter '%s'\n", name);
#endif
    }
}

void
GLSLProgram::setUniform2i(const char *name, int x, int y)
{
    GLint loc = glGetUniformLocation(mProg, name);
    if (loc >= 0) {
        glUniform2i(loc, x, y);
    } else {
#if _DEBUG
        LOGE("Error setting parameter '%s'\n", name);
#endif
    }
}

void
GLSLProgram::setUniform3f(const char *name, float x, float y, float z)
{
    GLint loc = glGetUniformLocation(mProg, name);
    if (loc >= 0) {
        glUniform3f(loc, x, y, z);
    } else {
#if _DEBUG
        LOGE("Error setting parameter '%s'\n", name);
#endif
    }
}

void
GLSLProgram::setUniform4f(const char *name, float x, float y, float z, float w)
{
    GLint loc = glGetUniformLocation(mProg, name);
    if (loc >= 0) {
        glUniform4f(loc, x, y, z, w);
    } else {
#if _DEBUG
        LOGE("Error setting parameter '%s'\n", name);
#endif
    }
}

void
GLSLProgram::setUniformMatrix4fv(const GLchar *name, GLfloat *m, bool transpose)
{
    GLint loc = glGetUniformLocation(mProg, name);
    if (loc >= 0) {
        glUniformMatrix4fv(loc, 1, transpose, m);
    } else {
#if _DEBUG
        LOGE("Error setting parameter '%s'\n", name);
#endif
    }
}

void
GLSLProgram::setUniformfv(const GLchar *name, GLfloat *v, int elementSize, int count)
{
	GLint loc = glGetUniformLocation(mProg, name);
	if (loc == -1) {
#ifdef _DEBUG
        LOGE("Error setting parameter '%s'\n", name);
#endif
		return;
	}

    switch (elementSize) {
		case 1:
			glUniform1fv(loc, count, v);
			break;
		case 2:
			glUniform2fv(loc, count, v);
			break;
		case 3:
			glUniform3fv(loc, count, v);
			break;
		case 4:
			glUniform4fv(loc, count, v);
			break;
	}
}

void
GLSLProgram::setUniform3i(const char *name, int x, int y, int z)
{
    GLint loc = glGetUniformLocation(mProg, name);
    if (loc >= 0) {
        glUniform3i(loc, x, y, z);
    } else {
#if _DEBUG
        LOGE("Error setting parameter '%s'\n", name);
#endif
    }
}
void
GLSLProgram::bindTexture(const char *name, GLuint tex, GLenum target, GLint unit)
{
    GLint loc = glGetUniformLocation(mProg, name);
    if (loc >= 0) {
        glActiveTexture(GL_TEXTURE0 + unit);
        glBindTexture(target, tex);
        glUseProgram(mProg);
        glUniform1i(loc, unit);
        glActiveTexture(GL_TEXTURE0);
    } else {
#if _DEBUG
        LOGE("Error binding texture '%s'\n", name);
#endif
    }
}
void
GLSLProgram::bindImage(const char *name, GLint unit, GLuint tex, GLint level, GLboolean layered, GLint layer, GLenum access, GLenum format)
{
    GLint loc = glGetUniformLocation(mProg, name);
    if (loc >= 0) {
        glBindImageTexture(unit, tex, level, layered, layer, access, format);
        glUseProgram(mProg);
        glUniform1i(loc, unit);
    } else {
#if _DEBUG
        LOGE("Error binding texture '%s'\n", name);
#endif
    }
}


GLuint
GLSLProgram::compileProgram(const char *vsource, const char *gsource, const char *fsource,
                            GLenum gsInput, GLenum gsOutput, int maxVerts)
{
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);

    mProg = glCreateProgram();

    if(vsource)
    {
        glShaderSource(vertexShader, 1, &vsource, 0);
        glCompileShader(vertexShader);
        glAttachShader(mProg, vertexShader);
    }
    glShaderSource(fragmentShader, 1, &fsource, 0);
    glCompileShader(fragmentShader);

    glAttachShader(mProg, fragmentShader);
    if (gsource) {
        GLuint geomShader = glCreateShader(GL_GEOMETRY_SHADER_EXT);
        glShaderSource(geomShader, 1, &gsource, 0);
        glCompileShader(geomShader);
        glAttachShader(mProg, geomShader);

        glProgramParameteriEXT(mProg, GL_GEOMETRY_INPUT_TYPE_EXT, gsInput);
        glProgramParameteriEXT(mProg, GL_GEOMETRY_OUTPUT_TYPE_EXT, gsOutput); 
        glProgramParameteriEXT(mProg, GL_GEOMETRY_VERTICES_OUT_EXT, maxVerts); 
    }

    glLinkProgram(mProg);

    // check if program linked
    GLint success = 0;
    glGetProgramiv(mProg, GL_LINK_STATUS, &success);

    if (!success) {
        char temp[1024];
        glGetProgramInfoLog(mProg, 1024, 0, temp);
        LOGE("Failed to link program:\n%s\n", temp);
        glDeleteProgram(mProg);
        mProg = 0;
        return 0;
    }

    return mProg;
}

char *
GLSLProgram::readTextFile(const char *filename)
{
    if (!filename) return 0;

	FILE *fp = 0;
	if (!(fp = fopen(filename, "r")))
	{
		LOGE("Cannot open \"%s\" for read!\n", filename);
		return 0;
	}

    fseek(fp, 0L, SEEK_END);     // seek to end of file
    long size = ftell(fp);       // get file length
    rewind(fp);                  // rewind to start of file

	char * buf = new char[size+1];

	size_t bytes;
	bytes = fread(buf, 1, size, fp);

	buf[bytes] = 0;

	fclose(fp);
	return buf;
}
