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
#include <string.h> // for strcpy() etc.
#include <sys/stat.h>
#include "main.h"
#include "nv_helpers_gl/GLSLProgram.h"

char* GLSLProgram::incPaths[] = {"/"};

bool GLSLProgram::setIncludeFromFile(const char *includeName, const char* filename)
{
    char tmpstr[200];
    sprintf(tmpstr, "%s/%s", filename, includeName);
    char * incStr = readTextFile(tmpstr);
    if(!incStr)
        return false;
    sprintf(tmpstr, "/%s", includeName);
    glNamedStringARB(GL_SHADER_INCLUDE_ARB, strlen(tmpstr), tmpstr, strlen(incStr), incStr);
    return false;
}
void GLSLProgram::setIncludeFromString(const char *includeName, const char* str)
{
    char tmpstr[200];
    sprintf(tmpstr, "/%s", includeName);
    glNamedStringARB(GL_SHADER_INCLUDE_ARB, strlen(tmpstr), tmpstr, strlen(str), str);
}

GLSLProgram::GLSLProgram(const char*progName) : mProg(0)
{
    curVSName = NULL;
    curFSName = NULL;
    curGSName = NULL;
    curProgName = NULL;
    if(progName)
    {
        curProgName = (char*)malloc(strlen(progName)+1);
        strncpy(curProgName, progName, strlen(progName)+1);
    }
}

GLSLProgram::GLSLProgram(const char *vsource, const char *fsource)
{
    curVSName = NULL;
    curFSName = NULL;
    curGSName = NULL;
    compileProgram(vsource, 0, fsource);
}

GLSLProgram::GLSLProgram(const char *vsource, const char *gsource, const char *fsource,
                         GLenum gsInput, GLenum gsOutput, int maxVerts)
{
    mProg = compileProgram(vsource, gsource, fsource, gsInput, gsOutput, maxVerts);
}

void
GLSLProgram::setShaderNames(const char*ProgName, const char *VSName,const char *GSName,const char *FSName)
{
    if(VSName)
    {
        if(curVSName) free(curVSName);
        curVSName = (char*)malloc(strlen(VSName)+1);
        strncpy(curVSName, VSName, strlen(VSName)+1);
    }
    if(FSName)
    {
        if(curFSName) free(curFSName);
        curFSName = (char*)malloc(strlen(FSName)+1);
        strncpy(curFSName, FSName, strlen(FSName)+1);
    }
    if(GSName)
    {
        if(curGSName) free(curGSName);
        curGSName = (char*)malloc(strlen(GSName)+1);
        strncpy(curGSName, GSName, strlen(GSName)+1);
    }
    if(ProgName)
    {
        if(curProgName) free(curProgName);
        curProgName = (char*)malloc(strlen(ProgName)+1);
        strncpy(curProgName, ProgName, strlen(ProgName)+1);
    }
}

GLuint
GLSLProgram::compileProgramFromFiles(const char *vFilename,  const char *gFilename, const char *fFilename,
                           GLenum gsInput, GLenum gsOutput, int maxVerts)
{
    char *vsource = readTextFile(vFilename);
    char *gsource = 0;
	if (mProg) {
		glDeleteProgram(mProg);
	}
    if (gFilename) {
        gsource = readTextFile(gFilename);
    }
    char *fsource = readTextFile(fFilename);
    if(vsource)
    {
        GLSLProgram::setShaderNames(NULL, vFilename, gFilename, fFilename);
        mProg = compileProgram(vsource, gsource, fsource, gsInput, gsOutput, maxVerts);
        delete [] vsource;
        if (gsource) delete [] gsource;
        delete [] fsource;
        return mProg;
    }
    return 0;
}

GLSLProgram::~GLSLProgram()
{
	if (mProg) {
		glDeleteProgram(mProg);
	}
    if(curVSName) free(curVSName);
    if(curFSName) free(curFSName);
    if(curGSName) free(curGSName);
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
GLSLProgram::setUniform1i(const char *name, int x)
{
    GLint loc = glGetUniformLocation(mProg, name);
    if (loc >= 0) {
        glUniform1i(loc, x);
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
    bool bErrors = false;
    GLint success = 0;
    char temp[1024];
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);

    mProg = glCreateProgram();

    if(vsource)
    {
        glShaderSource(vertexShader, 1, &vsource, 0);
        glCompileShaderIncludeARB(vertexShader, 1, incPaths,NULL);
        glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
        if (!success) {
            glGetShaderInfoLog(vertexShader, 1024, 0, temp);
            LOGE("%s: Failed to compile VtxShader:\n%s\n", curVSName ? curVSName:"VSNoname", temp);
            glDeleteShader(vertexShader);
            vertexShader = 0;
            bErrors = true;
        }
        else
        {
            glAttachShader(mProg, vertexShader);
            glDeleteShader(vertexShader);
        }
    }
    // NOTE: had some issues using include paths with https://www.opengl.org/registry/specs/ARB/shading_language_include.txt
    glShaderSource(fragmentShader, 1, &fsource, 0);
    glCompileShaderIncludeARB(fragmentShader, 1, incPaths,NULL);
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(fragmentShader, 1024, 0, temp);
        LOGE("%s: Failed to compile FragShader:\n%s\n", curFSName ? curFSName:"VSNoname", temp);
        glDeleteShader(fragmentShader);
        fragmentShader = 0;
        bErrors = true;
    }
    else
    {
        glAttachShader(mProg, fragmentShader);
        glDeleteShader(fragmentShader);
    }
    if (gsource) {
        GLuint geomShader = glCreateShader(GL_GEOMETRY_SHADER);
        // NOTE: had some issues using include paths with https://www.opengl.org/registry/specs/ARB/shading_language_include.txt
        glShaderSource(geomShader, 1, &gsource, 0);
        glCompileShaderIncludeARB(geomShader, 1, incPaths,NULL);
        glGetShaderiv(geomShader, GL_COMPILE_STATUS, &success);
        if (!success) {
            glGetShaderInfoLog(geomShader, 1024, 0, temp);
            LOGE("%s: Failed to compile GShader:\n%s\n", curGSName ? curGSName:"VSNoname", temp);
            glDeleteShader(geomShader);
            geomShader = 0;
            bErrors = true;
        }
        else
        {
            glAttachShader(mProg, geomShader);
            glDeleteShader(geomShader);
        }

        //glProgramParameteri(mProg, GL_GEOMETRY_INPUT_TYPE, gsInput);
        //glProgramParameteri(mProg, GL_GEOMETRY_OUTPUT_TYPE, gsOutput); 
        //glProgramParameteri(mProg, GL_GEOMETRY_VERTICES_OUT, maxVerts); 
    }

    if(bErrors)
    {
        glDeleteProgram(mProg);
        mProg = 0;
        return 0;
    }
    glLinkProgram(mProg);

    // check if program linked
    glGetProgramiv(mProg, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(mProg, 1024, 0, temp);
        LOGE("%s: Failed to link program:\n%s\n", curProgName ?curProgName:"Noname", temp);
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
