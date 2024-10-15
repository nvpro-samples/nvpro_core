## Table of Contents
- [bsdf_functions.h](#bsdf_functionsh)
- [bsdf_structs.h](#bsdf_structsh)
- [constants.h](#constantsh)
- [dh_comp.h](#dh_comph)
- [dh_hdr.h](#dh_hdrh)
- [dh_inspector.h](#dh_inspectorh)
- [dh_lighting.h](#dh_lightingh)
- [dh_scn_desc.h](#dh_scn_desch)
- [dh_sky.h](#dh_skyh)
- [dh_tonemap.h](#dh_tonemaph)
- [func.h](#funch)
- [ggx.h](#ggxh)
- [hdr_env_sampling.h](#hdr_env_samplingh)
- [light_contrib.h](#light_contribh)
- [pbr_mat_eval.h](#pbr_mat_evalh)
- [random.h](#randomh)
- [ray_util.h](#ray_utilh)
- [vertex_accessor.h](#vertex_accessorh)

## bsdf_functions.h
> Implements bidirectional scattering distribution functions (BSDFs) for
> physically-based rendering systems.

To use this code, include `bsdf_functions.h`. When a ray hits a surface:
* Create a `PbrMaterial` describing the material at that point
* Call `bsdfEvaluate()` to evaluate the scattering from one direction to
  another, or `bsdfSample()` to choose the next ray in the light path.
  See these functions' documentation for more information on their parameters
  and return values.

See the vk_gltf_renderer and vk_mini_samples/gltf_raytrace samples for examples
where these functions are used in a Monte Carlo path tracer.

The returned BSDF values and weights have the cosine term from the rendering
equation included; e.g. the Lambert lobe returns `max(0.0f, cos(N, k1)) / pi`.

Advanced users can use some of the other functions in this file to evaluate
and sample individual lobes, or subsets of lobes.

This file also provides `bsdfEvaluateSimple()` and `bsdfSampleSimple()`, which
implement a simpler and faster, though less fully-featured BSDF model. The
simple model only has diffuse, specular, and metallic lobes, while the full
model includes diffuse, transmission, specular, metal, sheen, and clearcoat
lobes (plus support for most glTF extensions).

Since GLSL doesn't have a distinction between public and private functions,
only functions that are part of the "public API" will have annotations to
appear in README.md.

This file (and files that it depends on) should be a GLSL/C++ polyglot, and
work so long as GLM has been included:

```cpp
#undef GLM_FORCE_XYZW_ONLY
#define GLM_FORCE_SWIZZLE
#include <glm/glm.hpp>
using namespace glm;

#include <nvvkhl/shaders/bsdf_functions.h>
```

#### Technical Notes

NVVKHL's BSDFs are based on [the glTF 2.0 specification](https://github.com/KhronosGroup/glTF) and
[the NVIDIA MDL SDK's BSDF implementations](https://github.com/NVIDIA/MDL-SDK/blob/203d5140b1dee89de17b26e828c4333571878629/src/mdl/jit/libbsdf/libbsdf.cpp).

The largest divergence from the above is that NVVKHL's BSDF model uses a
Fresnel term that depends only on the view and normal vectors, instead of the
half vector. This allows it to compute weights for lobes independently, while
BSDF code would normally need to sample a half vector for layer `i` to
determine the Fresnel weight for layer `i+1`. This can result in slightly
different glossy/diffuse blend weights (e.g. slightly differently shaped
highlights on leather surfaces).

All lobes are energy conserving (their integral over the sphere is at most 1)
and probability distribution functions (PDFs) integrate to 1 (except for areas
where the sampled direction results in an absorption event).

All lobes use single-scattering BSDFs. Multiple-scattering lobes are a
potential future improvement.

Most lobes use GGX normal distribution functions (NDFs) and the uncorrelated
Smith shadowing-maskiung function, except for the diffuse lobe (Lambert BRDF)
and the sheen lobe ([Conty and Kulla's "Charlie" sheen](https://blog.selfshadow.com/publications/s2017-shading-course/#course_content)
and a V-cavity shadowing-masking function).

### `DIRAC` Define
> Special PDF value returned by `bsdfSample()` to represent an infinite impulse
> or singularity.
### Function `absorptionCoefficient`
> Returns the absorption coefficient of the material.
### `LOBE_*` Defines
> Indices for lobe weights returned by `computeLobeWeights()`.
### Function `computeLobeWeights`
>Calculates the weights of the individual lobes inside the standard PBR material.

Returns an array which can be indexed using the `LOBE_*` defines. This can be
used to perform your own lobe sampling.

Note that `tint` will be changed if the material has iridescence! (It's
convenient to compute the iridescence factor here.) This means you should avoid
passing a material parameter to the `tint` field. Make a temporary instead:
```glsl
vec3 tint = mat.baseColor;
float[LOBE_COUNT] weights = computeLobeWeights(mat, dot(k1, mat.N), tint);
```
### Function `findLobe`
> Calculates the weights of the individual lobes inside the standard PBR material
> and randomly selects one.
### Function `computeDispersedIOR`
> Calculates the IOR at a given wavelength (in nanometers) given the base IOR
> and glTF dispersion factor.

See https://github.com/KhronosGroup/glTF/tree/0251c5c0cce8daec69bd54f29f891e3d0cdb52c8/extensions/2.0/Khronos/KHR_materials_dispersion.
### Function `wavelengthToRGB`
> Given a wavelength of light, returns an approximation to the linear RGB
> color of a D65 illuminant (sRGB whitepoint) sampled at a wavelength of `x`
> nanometers, using the CIE 2015 2-degree Standard Observer Color Matching Functions.

This is normalized so that
`sum(wavelengthToRGB(i), {i, WAVELENGTH_MIN, WAVELENGTH_MAX}) == vec3(1.)`,
which means that the values it returns are usually low. You'll need to multiply
by an appropriate normalization factor if you're randomly sampling it.

The colors here are clamped to only positive sRGB values, in case renderers
have problems with colors with negative sRGB components (i.e. are valid
colors but are out-of-gamut).
### Function `bsdfEvaluate`
> Evaluates the full BSDF model for the given material and set of directions.

You must provide `BsdfEvaluateData`'s `k1`, `k2`, and `xi` parameters.
(Evaluation is stochastic because this code randomly samples lobes depending on
`xi`; this is valid to do in a Monte Carlo path tracer.)

The diffuse lobe evaluation and the sum of the specular lobe evaluations
(including the cosine term from the rendering equation) will be returned in
`data.bsdf_diffuse` and `data.bsdf_glossy`. Additionally, the probability that
the sampling code will return this direction will be returned in `data.pdf`.
### Function `bsdfSample`

> Samples the full BSDF model for the given material and input direction.

You must provide `BsdfSampleData`'s `k1` and `xi` parameters. This function
will set the other parameters of `BsdfSampleData`.

There are two things you should check after calling this function:
* Is `data.event_type` equal to `BSDF_EVENT_ABSORB`? If so, the sampler sampled
  an output direction that would be absorbed by the material (e.g. it chose a
  reflective lobe but sampled a vector below the surface).
  The light path ends here.
* Is `data.pdf` equal to `DIRAC`? If so, this sampled a perfectly specular lobe.
  If you're using Multiple Importance Sampling weights, you should compute them
  as if `data.pdf` was infinity.

### Simple BSDF Model
The functions below are used to evaluate and sample the BSDF for a simple PBR material,
without any additional lobes like clearcoat, sheen, etc. and without the need of random numbers.
This is based on the metallic/roughness BRDF in Appendix B of the glTF specification.
### Function `bsdfEvaluateSimple`
> Evaluates the simple BSDF model using the given material and input and output directions.

You must provide `data.k1` and `data.k2`, but do not need to provide `data.xi`.
### Function `bsdfSampleSimple`
> Evaluates the simple BSDF model using the given material and input and output directions.

You must provide `data.k1` and `data.xi`. For one sample of pure reflection
(e.g. vk_mini_samples/ray_trace), use `data.xi == vec2(0,0)`.

After calling this function, you should check if `data.event_type` is
`BSDF_EVENT_ABSORB`. If so, the sampling code sampled a direction below the
surface, and the light path ends here (it should be treated as a reflectance of 0).

This code cannot currently return a PDF of `DIRAC`, but that might change in
the future.
### Function `bsdfSimpleAverageReflectance`
> Returns the approximate average reflectance of the Simple BSDF -- that is,
> `average_over_k2(f(k1, k2))` -- if GGX didn't lose energy.

This is useful for things like the variance reduction algorithm in
Tomasz Stachowiak's *Stochastic Screen-Space Reflections*; see also
Ray-Tracing Gems 1, chapter 32, *Accurate Real-Time Specular Reflections
with Radiance Caching*.

## bsdf_structs.h
### `BSDF_EVENT*` Defines
> These are the flags of the `BsdfSampleData::event_type` bitfield, which
> indicates the type of lobe that was sampled.

This terminology is based on McGuire et al., "A Taxonomy of Bidirectional
Scattering Distribution Function Lobes for Rendering Engineers",
https://casual-effects.com/research/McGuire2020BSDF/McGuire2020BSDF.pdf.
### struct `BsdfEvaluateData`
> Data structure for evaluating a BSDF. See the file for parameter documentation.
### struct `BsdfSampleData`
> Data structure for sampling a BSDF. See the file for parameter documentation.

## constants.h
Useful mathematical constants:
* `M_PI` (pi)
* `M_TWO_PI` (2*pi)
* `M_PI_2` (pi/2)
* `M_PI_4` (pi/4)
* `M_1_OVER_PI` (1/pi)
* `M_2_OVER_PI` (2/pi)
* `M_1_PI` (also 1/pi)
* `INFINITE` (infinity)

## dh_comp.h
### Device/Host Polyglot Overview
Files in nvvkhl named "*.h" are designed to be compiled by both C++ and GLSL
code, so that they can share structure and function definitions.
Not all functions will be available in both C++ and GLSL.
### `WORKGROUP_SIZE` Define
> The number of threads per workgroup in X and Y used by 2D compute shaders.

Generally, all nvvkhl compute shaders use the same workgroup size. (Workgroup
sizes of 128, 256, or 512 threads are generally good choices across GPUs.)
### Function `getGroupCounts`
> Returns the number of workgroups needed to cover `size` threads.

## dh_hdr.h
> Common structures used for HDR environment maps.

## dh_inspector.h
> Shader header for inspecting shader variables.

Prior to including this header either the `INSPECTOR_MODE_COMPUTE` or the
`INSPECTOR_MODE_FRAGMENT` macro must be defined, depending on the type of
shader to be inspected.

* If `INSPECTOR_MODE_COMPUTE` is defined, the shader must expose invocation information (e.g. `gl_LocalInvocationID`).
  This typically applies to compute, task and mesh shaders.
* If `INSPECTOR_MODE_FRAGMENT` is defined, the shader must expose fragment information (e.g. `gl_FragCoord`).
  This applies to fragment shaders.

You must also define the following macros:
* `INSPECTOR_DESCRIPTOR_SET`: the index of the descriptor set containing the
  Inspector buffers.
* `INSPECTOR_INSPECTION_DATA_BINDING`: the binding index of the buffer
  containing the inspection information, as provided by
  `ElementInspector::getComputeInspectionBuffer()`.
* `INSPECTOR_METADATA_BINDING`: the binding index of the buffer containing the
  inspection metadata, as provided by `ElementInspector::getComputeMetadataBuffer()`

You can also set `INSPECTOR_MODE_CUSTOM`,
`INSPECTOR_CUSTOM_INSPECTION_DATA_BINDING`, and
`INSPECTOR_CUSTOM_METADATA_BINDING` to inspect more than one variable
per thread.
### Function `inspect32BitValue`
> Use this to inspect a 32-bit value at a given index.
### Function `inspectCustom32BitValue`
> Use this to inspect a custom 32-bit value at a given index.

## dh_lighting.h
Common structures used for lights other than environment lighting.

## dh_scn_desc.h
Common structures used to store glTF scenes in GPU buffers.

## dh_sky.h
> Contains structures and functions for procedural sky models.

This file includes two sky models: a simple sky that is fast to compute, and
a more complex "physical sky" model based on a model from Mental Ray,
customized for nvpro-samples.
### Struct `SkySamplingResult`
> Contains the resulting direction, probability density function, and radiance
> from sampling the procedural sky.
### Struct `SkyPushConstant`
> Used by shaders that bake the procedural sky to a texture.
### Struct `SimpleSkyParameters`
> Parameters for the simple sky model.
### Function `initSimpleSkyParameters`
> Initializes `SimpleSkyParameters` with default values.
### Function `evalSimpleSky`
> Returns the radiance of the simple sky model in a given view direction.
### Function `sampleSimpleSky`
> Samples the simple sky model using two random values, returning a `SkySamplingResult`.
### Struct `PhysicalSkyParameters`
> Parameters for the physical sky model.
### Function `initPhysicalSkyParameters`
> Initializes `PhysicalSkyParameters` with default, realistic parameters.
### Function `evalPhysicalSky`
> Returns the radiance of the physical sky model in a given direction.
### Function `samplePhysicalSky`
> Samples the physical sky model using two random values, returning a `SkySamplingResult`.

## dh_tonemap.h
> Contains implementations for several local tone mappers.
### Struct `Tonemapper`
> Tonemapper settings.
### Struct `defaultTonemapper`
> Returns default tonemapper settings: filmic tone mapping, no additional color
> correction.
### Function `toSrgb`
> Converts a color from linear RGB to sRGB.
### Function `toLinear`
> Converts a color from sRGB to linear RGB.
### Function `tonemapFilmic`
> Filmic tonemapping operator by Jim Hejl and Richard Burgess-Dawson,
> approximating the Digital Fusion Cineon mode, but more saturated and with
> darker midtones. sRGB correction is built in.

http://filmicworlds.com/blog/filmic-tonemapping-operators/
### Function `tonemapUncharted`
> Tone mapping operator from Uncharted 2 by John Hable. sRGB correction is built in.

See: http://filmicworlds.com/blog/filmic-tonemapping-operators/
### Function `tonemapACES`
> An approximation by Stephen Hill to the Academy Color Encoding System's
> filmic curve for displaying HDR images on LDR output devices.

From https://github.com/TheRealMJP/BakingLab/blob/master/BakingLab/ACES.hlsl,
via https://knarkowicz.wordpress.com/2016/01/06/aces-filmic-tone-mapping-curve/
### Function `tonemapAgX`
> Benjamin Wrensch's approximation to the AgX tone mapping curve by Troy Sobotka.

From https://iolite-engine.com/blog_posts/minimal_agx_implementation
### Function `tonemapKhronosPBR`
> The Khronos PBR neutral tone mapper.

Adapted from https://github.com/KhronosGroup/ToneMapping/blob/main/PBR_Neutral/pbrNeutral.glsl
> Applies the given tone mapper and color grading settings to a given color.

Requires the UV coordinate so that it can apply vignetting.

## func.h
Useful utility functions for shaders.
### Function `saturate`
> Clamps a value to the range [0,1].
### Function `luminance`
> Returns the luminance of a linear RGB color, using Rec. 709 coefficients.
### Function `clampedDot`
> Takes the dot product of two values and clamps the result to [0,1].
### Function `orthonormalBasis`
> Builds an orthonormal basis: given only a normal vector, returns a
tangent and bitangent.

This uses the technique from "Improved accuracy when building an orthonormal
basis" by Nelson Max, https://jcgt.org/published/0006/01/02.

Any tangent-generating algorithm must produce at least one discontinuity
when operating on a sphere (due to the hairy ball theorem); this has a
small ring-shaped discontinuity at `normal.z == -0.99998796`.
### Function `makeFastTangent`
> Like `orthonormalBasis()`, but returns a tangent and tangent sign that matches
> the glTF convention.
### Function `rotate`
> Rotates the vector `v` around the unit direction `k` by an angle of `theta`.

At `theta == pi/2`, returns `cross(k, v) + k * dot(k, v)`. This means that
rotations are clockwise in right-handed coordinate systems and
counter-clockwise in left-handed coordinate systems.
### Function `getSphericalUv`
> Given a direction, returns the UV coordinate of an environment map for that
> direction using a spherical projection.
### Function `mixBary`
> Interpolates between 3 values, using the barycentric coordinates of a triangle.
### Function `cosineSampleHemisphere
> Samples a hemisphere using a cosine-weighted distribution.

See https://www.realtimerendering.com/raytracinggems/unofficial_RayTracingGems_v1.4.pdf,
section 16.6.1, "COSINE-WEIGHTED HEMISPHERE ORIENTED TO THE Z-AXIS".
### Function `powerHeuristic`
> The power heuristic for multiple importance sampling, with `beta = 2`.

See equation 9.13 of https://graphics.stanford.edu/papers/veach_thesis/thesis.pdf.

## ggx.h
Lower-level mathematical functions used for BSDFs. This file is named after the
GGX normal distribution, which is the basis for much of nvvkhl's BSDF code, but
includes Fresnel, shadowing-masking, sheen, and thin-film functions as well.
### Function `hvd_ggx_eval`
> Evaluates anisotropic GGX distribution on the non-projected hemisphere (i.e.
> +z is up)
### Function `hvd_ggx_sample_vndf`
> Samples Samples a visible (Smith-masked) half vector according to the anisotropic GGX distribution.

See Eric Heitz, "A Simpler and Exact Sampling Routine for the GGX Distribution of Visible Normals".

The input and output will be in local space:
`vec3(dot(T, k1), dot(B, k1), dot(N, k1))`.
### Function `ggx_smith_shadow_mask`
> The uncorrelated Smith shadowing-masking function.

Note that the joint Smith shadowing-masking function may be more realistic.
### Function `ior_fresnel`
> Fresnel equation for an equal mix of polarization.
### Function `hvd_sheen_eval`
> Evaluates the anisotropic sheen half-vector distribution on the non-projected hemisphere.
### Function `vcavities_shadow_mask`
> Cook-Torrance style v-cavities shadowing-masking term.
### Function `hvd_sheen_sample`
> Samples a half-vector according to an anisotropic sheen distribution.

## hdr_env_sampling.h
### Function `environmentSample`
> Randomly samples a direction from an environment map.

Inputs:
- `envSamplingData`, a buffer generated by the `HdrEnvDome` code containing
data for the alias method to work.
- hdrTexture: Used to look up the environment value.
- randVal: A uniform random vector in the range [0,1]^3.

Returns `vec4(vec3(radiance), pdf)`, and sets `toLight` to the
sampled direction.

See https://arxiv.org/pdf/1901.05423.pdf, Section 2.6, "The Alias Method".

## light_contrib.h
### Function `singleLightContribution`
> Returns an estimate of the contribution of a light to a given point on a surface.

Inputs:
- `light`: A `Light` structure; see dh_lighting.h.
- `surfacePos`: The surface position.
- `surfaceNormal`: The surface normal.
- `randVal`: Used to randomly sample points on area (disk) lights. This means
that the light contribution from an area light will be noisy.
### Function `singleLightContribution` (three-argument overload)
> Like `singleLightContribution` above, but without using random values.

Because this is equivalent to `singleLightContribution(..., vec2(0.0F))`, area
lights won't be noisy, but will have harder falloff than they would
otherwise have.

## pbr_mat_eval.h
This file takes the incoming `GltfShadeMaterial` (material uploaded in a buffer) and
evaluates it, basically sampling the textures, and returns the struct `PbrMaterial`
which is used by the BSDF functions to evaluate and sample the material.
### `MAT_EVAL_TEXTURE_ARRAY` Define
> The name of the array that contains all texture samplers.

Defaults to `texturesMap`; you can define this before including `pbr_mat_eval.h`
to use textures from a different array name.
### `NO_TEXTURES` Define
> Define this before including `pbr_mat_eval.h` to use a color of `vec4(1.0f)`
> for everything instead of reading textures.
### `MICROFACET_MIN_ROUGHNESS` Define
> Minimum roughness for microfacet models.

This protects microfacet code from dividing by 0, as well as from numerical
instability around roughness == 0. However, it also means even roughness-0
surfaces will be rendered with a tiny amount of roughness.

This value is ad-hoc; it could probably be lowered without issue.
### Function `evaluateMaterial`
> From the incoming `material` and `mesh` info, return a `PbrMaterial` struct
> for the BSDF system.

## random.h
Random number generation functions.

For even more hash functions, check out
[Jarzynski and Olano, "Hash Functions for GPU Rendering"](https://jcgt.org/published/0009/03/02/)
### Function `xxhash32`
> High-quality hash that takes 96 bits of data and outputs 32, roughly twice
> as slow as `pcg`.

You can use this to generate a seed for subsequent random number generators;
for instance, provide `uvec3(pixel.x, pixel.y, frame_number).

From https://github.com/Cyan4973/xxHash and https://www.shadertoy.com/view/XlGcRh.
### Function `pcg`
> Fast, reasonably good hash that updates 32 bits of state and outputs 32 bits.

This is a version of `pcg32i_random_t` from the
[PCG random number generator library](https://www.pcg-random.org/index.html),
which updates its internal state using a linear congruential generator and
outputs a hash using `pcg_output_rxs_m_xs_32_32`, a more complex hash.

There's a section of vk_mini_path_tracer on this random number generator
[here](https://nvpro-samples.github.io/vk_mini_path_tracer/#antialiasingandpseudorandomnumbergeneration/pseudorandomnumbergenerationinglsl).
### Function `rand`
> Generates a random float in [0, 1], updating an RNG state.

This can be used to generate many random numbers! Here's an example:

```glsl
uint seed = xxhash32(vec3(pixel.xy, frame_number));
for(int bounce = 0; bounce < 50; bounce++)
{
  ...
  BsdfSampleData sampleData;
  sampleData.xi = vec3(rand(seed), rand(seed), rand(seed));
  ...
}
```

## ray_util.h
### Function `offsetRay`
> Adjusts the origin `p` of the ray `p + t*n` so that it is unlikely to
> intersect with a triangle that passes through `p`, but tries not to
> noticeably affect visual results.

For a more sophisticated algorithm, see
"A Fast and Robust Method for Avoiding Self-Intersection" by
Carsten Wachter and Nikolaus Binder in Ray Tracing Gems volume 1.
### Function `pointOffset`
> Adjusts a position so that shadows match interpolated normals more closely.

This technique comes from ["Hacking the shadow terminator"](https://jo.dreggn.org/home/2021_terminator.pdf) by Johannes Hanika.

Inputs:
- `p`: Point of intersection on a triangle.
- `p[a..c]`: Positions of the triangle at each vertex.
- `n[a..c]`: Normals of the triangle at each vertex.
- `bary`: Barycentric coordinate of the hit position.

Returns the new position.

## vertex_accessor.h
Included in shaders to provide access to vertex data, so long as vertex
data follows a standard form.

Includes `getTriangleIndices`, and `getVertex*` and `getInterpolatedVertex*`
functions for all attributes.
