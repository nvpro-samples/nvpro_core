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
* (TODO)install_shared_external script: run this 

# shared_external folder
the samples do rely on few external tools in order to compile.

Like every GitHub application/samples, the user might have to specify where to find them.
However, finding proper builds and source code is know to be quite painful.

We propose a zip file containing all the necessary external tool for the samples to build properly. These tools don't belong to NVIDIA, so we decided to keep them outside of GitHub.

You can find the zip at this location: https://developer.nvidia.com/samples
* download it and unzip it to the folder where you cloned the samples and 'shared_sources' folder
* the zip file contains a folder called "shared_external"
* samples will be able to automatically locate these tools if in "shared_external"
* (TODO)//install_shared_external// script should help you to gather this zip file

The tools that are needed and available in the zip file are:
* AntTweakBar: if you already have it, you can set ANTTWEAKBAR_LOCATION env. variable or set it in cmake
* glew: if you already have it, you can set GLEW_LOCATION env. variable or set it in cmake
* NSight NVPMAPI: needed only if you checked the option for the samples to be decorated with custom markers
* NvAPI: optional special NVIDIA library to access specific features. few samples might use it, as examples on how to use nvAPI
* SvCMFCUI: a simple UI quite convenient to quickly get some widgets in a sample. No all samples do need it. If not available, the sample will run without any UI widgets
* zlib: essentially needed for big meshes of *bk3d* format. the folder "shared_resources" contains these files under *.bk3d.gz. If you don't install zlib, the samples using these meshes will require you to unzip these meshes (unzip all *.bk3d.gz to *.bk3d ) prior to running the samoples. If zlib is available, the sample will be able to read directly the bk3d.gz files


