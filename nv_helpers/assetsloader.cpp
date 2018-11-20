/* Copyright (c) 2014-2018, NVIDIA CORPORATION. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *  * Neither the name of NVIDIA CORPORATION nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdint.h>

#include "assetsloader.hpp"
#include "nvprint.hpp"  // for LOGI

#include <string>

#if defined( WIN32 )

#include <stdio.h>

static std::vector<std::string> s_searchPath;

bool AssetLoaderInit( void* )
{
  return true;
}

bool AssetLoaderShutdown()
{
  s_searchPath.clear();
  return true;
}

bool AssetLoaderAddSearchPath( const char* path )
{
  std::vector<std::string>::iterator src = s_searchPath.begin();

  if( !s_searchPath.empty() )
    while( src != s_searchPath.end() )
    {
      if( !( *src ).compare( path ) )
        return true;
      src++;
    }

  s_searchPath.push_back( path );
  return true;
}

bool AssetLoaderRemoveSearchPath( const char* path )
{
  std::vector<std::string>::iterator src = s_searchPath.begin();

  while( src != s_searchPath.end() )
  {
    if( !( *src ).compare( path ) )
    {
      s_searchPath.erase( src );
      return true;
    }
    src++;
  }
  return true;
}

std::vector<std::string> AssetLoaderGetSearchPath()
{
  return s_searchPath;
}


static FILE* openFile( const char* filePath, const char* mode, std::string& filenameFound )
{
  FILE* fp = NULL;
  // loop N times up the hierarchy, testing at each level
  std::string upPath;
  std::string fullPath;
  for( int32_t i = 0; i < 10; i++ )
  {
    std::vector<std::string>::iterator src     = s_searchPath.begin();
    bool                               looping = true;
    while( looping )
    {
      fullPath.assign( upPath );  // reset to current upPath.
      if( src != s_searchPath.end() )
      {
        //sprintf_s(fullPath, "%s%s/assets/%s", upPath, *src, filePath);
        fullPath.append( *src );
        fullPath.append( "/" );  //fullPath.append("/assets/");
        src++;
      }
      else
      {
        //sprintf_s(fullPath, "%sassets/%s", upPath, filePath);
        //fullPath.append("assets/");
        looping = false;
      }
      fullPath.append( filePath );

#ifdef DEBUG
      LOGI("Trying to open %s\n", fullPath.c_str() );
#endif
      if( ( fopen_s( &fp, fullPath.c_str(), mode ) != 0 ) || ( fp == NULL ) )
        fp = NULL;
      else
        looping = false;
    }

    filenameFound = fullPath;

    if( fp )
      break;

    upPath.append( "../" );
  }

  return fp;
}

std::string AssetLoaderFindFile( const std::string& filename )
{
  std::string filenameFound;
  FILE*       fp = openFile( filename.c_str(), "rb", filenameFound );
  if( fp )
    fclose( fp );
  return filenameFound;
}


bool AssetLoaderFileExists( const char* filePath )
{
  std::string filenameFound;
  FILE*       fp = openFile( filePath, "rb", filenameFound );
  if( !fp )
    return false;
  fclose( fp );
  return true;
}

char* AssetLoaderRead( const char* filePath, size_t& length, const char* mode, std::string& filenameFound )
{
  FILE* fp = openFile( filePath, mode, filenameFound );

  if( !fp )
  {
    LOGE( "Error opening file '%s'\n", filePath );
    return NULL;
  }

  fseek( fp, 0, SEEK_END );
  length = ftell( fp );
  fseek( fp, 0, SEEK_SET );

  char* data   = new char[length + 1];
  length       = fread( data, 1, length, fp );
  data[length] = '\0';

  fclose( fp );

#ifdef DEBUG
  LOGI( "Read file '%s', %d bytes\n", filePath, length );
#endif
  return data;
}

char* AssetLoaderRead( const char* filePath, size_t& length )
{
  std::string filenameFound;
  return AssetLoaderRead( filePath, length, "rb", filenameFound );
}

bool AssetLoaderFree( char* asset )
{
  delete[] asset;
  return true;
}

#elif defined( LINUX ) || defined( MACOSX )  // have mac and linux share ftm.

#include <stdio.h>
#include <vector>

static std::vector<std::string> s_searchPath;

bool AssetLoaderInit( void* )
{
  return true;
}

bool AssetLoaderShutdown()
{
  s_searchPath.clear();
  return true;
}

bool AssetLoaderAddSearchPath( const char* path )
{
  std::vector<std::string>::iterator src = s_searchPath.begin();

  while( src != s_searchPath.end() )
  {
    if( !( *src ).compare( path ) )
      return true;
    src++;
  }

  s_searchPath.push_back( path );
  return true;
}

bool AssetLoaderRemoveSearchPath( const char* path )
{
  std::vector<std::string>::iterator src = s_searchPath.begin();

  while( src != s_searchPath.end() )
  {
    if( !( *src ).compare( path ) )
    {
      s_searchPath.erase( src );
      return true;
    }
    src++;
  }
  return true;
}


static FILE* openFile( const char* filePath, const char* mode, std::string& filenameFound )
{
  FILE* fp = NULL;
  // loop N times up the hierarchy, testing at each level
  std::string upPath;
  std::string fullPath;
  for( int32_t i = 0; i < 10; i++ )
  {
    std::vector<std::string>::iterator src     = s_searchPath.begin();
    bool                               looping = true;
    while( looping )
    {
      fullPath.assign( upPath );  // reset to current upPath.
      if( src != s_searchPath.end() )
      {
        //sprintf_s(fullPath, "%s%s/assets/%s", upPath, *src, filePath);
        fullPath.append( *src );
        //fullPath.append("/assets/");
        src++;
      }
      else
      {
        //sprintf_s(fullPath, "%sassets/%s", upPath, filePath);
        //fullPath.append("assets/");
        looping = false;
      }
      fullPath.append( filePath );

#ifdef DEBUG
      LOGI( "Trying to open %s\n", fullPath.c_str() );
#endif
      fp = fopen( fullPath.c_str(), mode );
      if( fp != NULL )
        looping = false;
    }

    filenameFound = fullPath;

    if( fp )
      break;

    upPath.append( "../" );
  }

  return fp;
}

bool AssetLoaderFileExists( const char* filePath )
{
  std::string filenameFound;
  FILE*       fp = openFile( filePath, "rb", filenameFound );
  if( !fp )
    return false;
  fclose( fp );
  return true;
}

char* AssetLoaderRead( const char* filePath, size_t& length, const char* mode, std::string& filenameFound )
{
  FILE* fp = openFile( filePath, mode, filenameFound );

  if( !fp )
  {
    LOGE( "Error opening file '%s'\n", filePath );
    return NULL;
  }

  fseek( fp, 0, SEEK_END );
  length = ftell( fp );
  fseek( fp, 0, SEEK_SET );

  char* data   = new char[length + 1];
  length       = fread( data, 1, length, fp );
  data[length] = '\0';

  fclose( fp );

#ifdef DEBUG
  LOGI("Read file '%s', %d bytes\n", filePath, length );
#endif
  return data;
}

char* AssetLoaderRead( const char* filePath, size_t& length )
{
  std::string filenameFound;
  return AssetLoaderRead( filePath, length, "rb", filenameFound );
}

bool AssetLoaderFree( char* asset )
{
  delete[] asset;
  return true;
}

#else

#error "No asset loader library defined for this platform!"

#endif
