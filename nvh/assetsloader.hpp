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

// TAGRELEASE: PUBLIC

#ifndef NV_ASSET_LOADER_H
#define NV_ASSET_LOADER_H

#include <stdint.h>
#include <string>
#include <vector>

/// \file
/// Cross-platform binary asset loader.
/// Cross-platform binary asset loader.  Requires files
/// to be located in a subdirectory of the application's source
/// tree named "assets" in order to be able to find them.  This
/// is enforced so that assets will be automatically packed into
/// the application's APK on Android (ANT defaults to packing the
/// tree under "assets" into the binary assets of the APK).
///
/// On platforms that use file trees for storage (Windows and
/// Linux), the search method for finding each file passed in as the
/// partial path <filepath> is as follows:
/// - Start at the application's current working directory
/// - Do up to 10 times:
///     -# For each search path <search> in the search list:
///          -# Try to open <currentdir>/<search>/<filepath>
///          -# If it is found, return it
///          -# Otherwise, move to next path in <search> and iterate
///     -# Change directory up one level and iterate
///
/// On Android, the file opened is always <filepath>, since the "assets"
/// directory is known (it is the APK's assets).

/// Opaque file handle type - do not cast to platform equivalents!
typedef void* AssetFilePtr;

/// Seek "whence" type: these must match the POSIX variants;
/// they are not to be changed
enum AssetSeekBase
{
  NV_SEEK_SET = 0,  /// SEEK_SET
  NV_SEEK_CUR = 1,  /// SEEK_CUR
  NV_SEEK_END = 2   /// SEEK_END
};


/// Initializes the loader at application start.
/// Initializes the loader.  In most cases, the platform-specific
/// application framework or main loop should make this call.  It
/// requires a different argument on each platform
/// param[in] platform a platform-specific context pointer used by
/// the implementation
/// - On Android, this should be the app's AssetManager instance
/// - On Windows and Linux, this is currently ignored and should be NULL
///
/// \return true on success and false on failure
bool AssetLoaderInit( void* platform );

/// Shuts down the system
/// \return true on success and false on failure
bool AssetLoaderShutdown();

/// Adds a search path for finding the root of the assets tree.
/// Adds a search path to be prepended to "assets" when searching
/// for the correct assets tree.  Note that this must be a relative
/// path, and it is not used directly to find the file.  It is only
/// used on path-based platforms (Linux and Windows) to find the
/// "assets" directory.
/// \param[in] The relative path to add to the set of paths used to
/// find the "assets" tree.  See the package description for the
/// file search methods
/// \return true on success and false on failure
bool        AssetLoaderAddSearchPath( const char* path );
inline bool AssetLoaderAddSearchPath( const std::string& path )
{
  return AssetLoaderAddSearchPath( path.c_str() );
}

/// Removes a search path from the lists.
/// \param[in] the path to remove
/// \return true on success and false on failure (not finding the path
/// on the list is considered success)
bool        AssetLoaderRemoveSearchPath( const char* path );
inline bool AssetLoaderRemoveSearchPath( const std::string& path )
{
  return AssetLoaderRemoveSearchPath( path.c_str() );
}


/// Returns the list of search paths
std::vector<std::string> AssetLoaderGetSearchPath();


/// Search for a filename on all search path
/// \return The full path to the file, empty if nothing found
std::string AssetLoaderFindFile( const std::string& filename );


/// Reads an asset file as a block.
/// Reads an asset file, returning a pointer to a block of memory
/// that contains the entire file, along with the length.  The block
/// is null-terminated for safety
/// \param[in] filePath the partial path (below "assets") to the file
/// \param[out] length the length of the file in bytes
/// \return a pointer to a null-terminated block containing the contents
/// of the file or NULL on error.  This block should be freed with a call
/// to #AssetLoaderFree when no longer needed.  Do NOT delete the block
/// with free or delete[]
char*        AssetLoaderRead( const char* filePath, size_t& length );
char*        AssetLoaderRead( const char* filePath, size_t& length, const char* mode, std::string& filenameFound );
inline char* AssetLoaderRead( const std::string& path, size_t& length )
{
  return AssetLoaderRead( path.c_str(), length );
}
inline char* AssetLoaderRead( const std::string& path, size_t& length, const char* mode, std::string& filenameFound )
{
  return AssetLoaderRead( path.c_str(), length, mode, filenameFound );
}

/// Frees a block returned from #AssetLoaderRead.
/// \param[in] asset a pointer returned from #AssetLoaderRead
/// \return true on success and false on failure
bool AssetLoaderFree( char* asset );

/// Returns a boolean indicating whether the desired file exists
/// \param[in] filePath the partial path to the file to be tested
/// \return true if file exists and is readable, false otherwise
bool        AssetLoaderFileExists( const char* filePath );
inline bool AssetLoaderFileExists( const std::string& path )
{
  return AssetLoaderFileExists( path.c_str() );
}

#if 0
/// Opens an asset file as binary read-only
/// \param[in] name the partial path to the file to be opened for reading
/// \return nonzero opaque file handle if the file could be opened for reading, NULL
/// if the file could not be opened
AssetFilePtr AssetLoaderOpenFile(const char* name);
inline AssetFilePtr AssetLoaderOpenFile(const std::string &path) { return AssetLoaderOpenFile(path.c_str()); }

/// Closes an open file
/// \param[in] fp the opaque handle to the open file
void AssetLoaderCloseFile(AssetFilePtr fp);


/// Returns the total size of an open file in bytes
/// \param[in] fp the opaque handle to the open file
/// \return the size of the file in bytes or zero on error
int32_t AssetFileGetSize(AssetFilePtr fp);

/// Read from the file
/// Note that unlike AssetLoaderRead, the result is not automatically
/// null-terminated.
/// \param[in] file the opaque handle to the open file
/// \param[in] size bytes to read
/// \param[in] data the destination pointer, allocated by the called
/// \return the number of bytes actually read
int32_t AssetFileRead(AssetFilePtr file, int32_t size, void* data);

/// Seek the file pointer
/// \param[in] fp the opaque handle to the open file
/// \param[in] offset the signed offset from the base in bytes
/// \param[in] whence the base of the seek (start, current or end)
/// \return zero on success, non-zero otherwise
int32_t AssetFileSeek(AssetFilePtr fp, int32_t offset, AssetSeekBase whence);

/// Returns the total size of an open file in bytes (64 bit)
/// \param[in] fp the opaque handle to the open file
/// \return the size of the file in bytes or zero on error
int64_t AssetFileGetSize64(AssetFilePtr fp);

/// Read from the file (64 bit)
/// Note that unlike AssetLoaderRead, the result is not automatically
/// null-terminated.
/// \param[in] file the opaque handle to the open file
/// \param[in] size bytes to read
/// \param[in] data the destination pointer, allocated by the called
/// \return the number of bytes actually read
int64_t AssetFileRead64(AssetFilePtr file, int32_t size, void* data);

/// Seek the file pointer (64 bit)
/// \param[in] fp the opaque handle to the open file
/// \param[in] offset the signed offset from the base in bytes
/// \param[in] whence the base of the seek (start, current or end)
/// \return zero on success, non-zero otherwise
int64_t AssetFileSeek64(AssetFilePtr fp, int64_t offset, AssetSeekBase whence);
#endif

/// Load the text in the given file and return it as an STL string
/// \param[in] fileName the path and filename of the file to be opened
/// \return A string containing the file text
inline std::string AssetLoadTextFile( const std::string& fileName )
{
  std::string fileNameFound;
  std::string result;
  size_t      len;
  char*       content = AssetLoaderRead( fileName, len, "rt", fileNameFound );

  if( content )
  {
    result = std::string( const_cast<const char*>( content ) );
    AssetLoaderFree( content );
  }

  return result;
}

inline std::string AssetLoadTextFile( const std::string& fileName, std::string& fileNameFound )
{
  std::string result;
  size_t      len;
  char*       content = AssetLoaderRead( fileName, len, "rt", fileNameFound );

  if( content )
  {
    result = std::string( const_cast<const char*>( content ) );
    AssetLoaderFree( content );
  }

  return result;
}

/// Load the data in the given file and return it as an STL string
/// \param[in] fileName the path and filename of the file to be opened
/// \return A string containing the file data
inline std::string AssetLoadBinaryFile( const std::string& fileName )
{
  std::string fileNameFound;
  std::string result;
  size_t      len;
  char*       content = AssetLoaderRead( fileName, len, "rb", fileNameFound );

  if( content )
  {
    result = std::string( const_cast<const char*>( content ), len );
    AssetLoaderFree( content );
  }

  return result;
}

#endif
