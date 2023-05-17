#ifndef NVPRO_CORE_NVH_CONTAINER_UTILS_HPP_
#define NVPRO_CORE_NVH_CONTAINER_UTILS_HPP_

#include <array>
#include <cassert>
#include <stddef.h>
#include <stdint.h>
#include <vector>

/// \nodoc (keyword to exclude this file from automatic README.md generation)

// constexpr array size functions for C and C++ style arrays.
// Truncated to 32-bits (with error checking) to support the common case in Vulkan.
template <typename T, size_t size>
constexpr uint32_t arraySize(const T (&)[size])
{
  constexpr uint32_t u32_size = static_cast<uint32_t>(size);
  static_assert(size == u32_size, "32-bit overflow");
  return u32_size;
}

template <typename T, size_t size>
constexpr uint32_t arraySize(const std::array<T, size>&)
{
  constexpr uint32_t u32_size = static_cast<uint32_t>(size);
  static_assert(size == u32_size, "32-bit overflow");
  return u32_size;
}

// Checked 32-bit array size function for vectors.
template <typename T, typename Allocator>
constexpr uint32_t arraySize(const std::vector<T, Allocator>& vector)
{
  auto     size     = vector.size();
  uint32_t u32_size = static_cast<uint32_t>(size);
  if(u32_size != size)
  {
    assert(!"32-bit overflow");
  }
  return u32_size;
}

namespace nvh {

//---- Hash Combination ----
// http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2014/n3876.pdf
template <typename T>
void hashCombine(std::size_t& seed, const T& val)
{
  seed ^= std::hash<T>()(val) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}
// Auxiliary generic functions to create a hash value using a seed
template <typename T, typename... Types>
void hashCombine(std::size_t& seed, const T& val, const Types&... args)
{
  hashCombine(seed, val);
  hashCombine(seed, args...);
}
// Optional auxiliary generic functions to support hash_val() without arguments
inline void hashCombine(std::size_t& seed) {}
// Generic function to create a hash value out of a heterogeneous list of arguments
template <typename... Types>
std::size_t hashVal(const Types&... args)
{
  std::size_t seed = 0;
  hashCombine(seed, args...);
  return seed;
}
//--------------

template <typename T>
std::size_t hashAligned32(const T& v)
{
  const uint32_t  size  = sizeof(T) / sizeof(uint32_t);
  const uint32_t* vBits = reinterpret_cast<const uint32_t*>(&v);
  std::size_t     seed  = 0;
  for(uint32_t i = 0u; i < size; i++)
  {
    hashCombine(seed, vBits[i]);
  }
  return seed;
}


// Generic hash function to use when using a struct aligned to 32-bit as std::map-like container key
// Important: this only works if the struct contains integral types, as it will not
// do any pointer chasing
template <typename T>
struct HashAligned32
{
  std::size_t operator()(const T& s) const { return hashAligned32(s); }
};

// Generic equal function to use when using a struct as std::map-like container key
// Important: this only works if the struct contains integral types, as it will not
// do any pointer chasing
template <typename T>
struct EqualMem
{
  bool operator()(const T& l, const T& r) const { return memcmp(&l, &r, sizeof(T)) == 0; }
};

}  // namespace nvh

#endif