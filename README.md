````
    Copyright (c) 2013, NVIDIA CORPORATION. All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions
    are met:
     * Redistributions of source code must retain the above copyright
       notice, this list of conditions and the following disclaimer.
     * Neither the name of NVIDIA CORPORATION nor the names of its
       contributors may be used to endorse or promote products derived
       from this software without specific prior written permission.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ``AS IS'' AND ANY
    EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
    PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
    CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
    EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
    PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
    PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
    OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
    OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

````

# shared_sources repository

This folder is a repository of *shared source code* : all other samples might need to cherry-pick some code from here

It means that you must clone this repository prior to trying samples.

## Folders

* cmake: contains the 'find' files to search for cmake packages
* nv_helpers: contains various helpers to simplify the code of samples.
* nv_helpers_gl: contains various helpers for OpenGL
* (TODO) nv_helpers_d3d: contains various helpers for D3D
* (TODO) nv_helpers_cuda: contains helpers for CUDA
* nv_math: math library used by almost all the samples
* glew: the samples do take the glew.c prior to linking with its library. Easier and good enough. Taken from http://glew.sourceforge.net/
  if you want to take one from somewhere else, specify its base directory with GLEW_LOCATION

## files
* CMakeLists_include.txt: common file that every samples CMakeLists.txt will include
* main.h: no matter the kind of sample, the main part is shared. This is the include for all
* main_glfw.cpp: main file for samples building with glfw (option in cmake)
* main_win32.cpp: main file for samples building for win32
* resources.h; resources.rc; opengl.ico: typical Windows stuff
* (TODO)download_all script: run this script if you want to clone all the available samples

# shared_external folder
The samples do rely on few external tools in order to compile. You can find them [here](https://github.com/nvpro-samples/shared_external)





