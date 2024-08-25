/*
 * Copyright (c) 2022-2024, NVIDIA CORPORATION.  All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * SPDX-FileCopyrightText: Copyright (c) 2022-2024, NVIDIA CORPORATION.
 * SPDX-License-Identifier: Apache-2.0
 */


#ifndef DH_SKY_H
#define DH_SKY_H 1

#ifdef __cplusplus
#define OUT_TYPE(T) T&
#define INOUT_TYPE(T) T&
#else
#define OUT_TYPE(T) out T
#define INOUT_TYPE(T) inout T
#endif

#ifndef M_PIf
#define M_PIf 3.1415926535f
#endif

#ifdef __cplusplus
namespace nvvkhl_shaders {
using glm::acos;
using glm::clamp;
using glm::degrees;
using glm::dot;
using glm::fract;
using glm::mat3;
using glm::mat4;
using glm::max;
using glm::min;
using glm::mix;
using glm::radians;
using glm::smoothstep;
using glm::vec2;
using glm::vec3;
#else
#define static
#define inline
#endif  // __cplusplus


#ifndef WORKGROUP_SIZE
#define WORKGROUP_SIZE 16  // Grid size used by compute shaders
#endif

// clang-format off
#ifdef __cplusplus // Descriptor binding helper for C++ and GLSL
#define START_BINDING(a) enum a {
#define END_BINDING() }
#else
#define START_BINDING(a)  const uint
#define END_BINDING()
#endif

START_BINDING(SkyBindings)
eSkyOutImage = 0,
eSkyParam = 1
END_BINDING();
// clang-format on

// Struct to hold the sampling result
struct SkySamplingResult
{
  vec3  direction;  // Direction to the sampled light
  float pdf;        // Probability Density Function value
  vec3  radiance;   // Light intensity
};

// Used by the sky shaders
struct SkyPushConstant
{
  mat4 mvp;
};

struct SimpleSkyParameters
{
  vec3  directionToLight;
  float angularSizeOfLight;

  vec3  sunColor;
  float glowSize;

  vec3  skyColor;
  float glowIntensity;

  vec3  horizonColor;
  float horizonSize;

  vec3  groundColor;
  float glowSharpness;

  vec3  directionUp;
  float sunIntensity;

  vec3  lightRadiance;
  float brightness;
};


/** @DOC_START
# Function initSimplSkyParameters
>  Initializes the sky shader parameters with default values
@DOC_END */
inline SimpleSkyParameters initSimpleSkyParameters()
{
  SimpleSkyParameters p;
  p.directionToLight   = vec3(0.0F, 0.707F, 0.707F);
  p.angularSizeOfLight = 0.059F;
  p.sunColor           = vec3(1.0F, 1.0F, 1.0F);
  p.sunIntensity       = 0.01093f;
  p.skyColor           = vec3(0.17F, 0.37F, 0.65F);
  p.horizonColor       = vec3(0.50F, 0.70F, 0.92F);
  p.groundColor        = vec3(0.62F, 0.59F, 0.55F);
  p.directionUp        = vec3(0.F, 1.F, 0.F);
  p.horizonSize        = 0.5F;    // +/- degrees
  p.glowSize           = 0.091F;  // degrees, starting from the edge of the light disk
  p.glowIntensity      = 0.9F;    // [0-1] relative to light intensity
  p.glowSharpness      = 4.F;     // [1-10] is the glow power exponent
  p.brightness         = 1.0F;    // [0-1] overall brightness
  p.lightRadiance      = vec3(1.0F, 1.0F, 1.0F);

  return p;
}

/** @DOC_START
# Function proceduralSky
>  Return the color of the procedural sky shader
@DOC_END  */
inline vec3 evalSimpleSky(SimpleSkyParameters params, vec3 direction)
{
  float angularSizeOfPixel = 0.0F;

  vec3 skyColor     = params.skyColor * params.brightness;
  vec3 horizonColor = params.horizonColor * params.brightness;
  vec3 groundColor  = params.groundColor * params.brightness;

  // Sky
  float elevation   = asin(clamp(dot(direction, params.directionUp), -1.0F, 1.0F));
  float top         = smoothstep(0.F, params.horizonSize, elevation);
  float bottom      = smoothstep(0.F, params.horizonSize, -elevation);
  vec3  environment = mix(mix(horizonColor, groundColor, bottom), skyColor, top);

  // Sun
  float angleToLight    = acos(clamp(dot(direction, params.directionToLight), 0.0F, 1.0F));
  float halfAngularSize = params.angularSizeOfLight * 0.5F;
  float lightIntensity =
      clamp(1.0F - smoothstep(halfAngularSize - angularSizeOfPixel * 2.0F, halfAngularSize + angularSizeOfPixel * 2.0F, angleToLight),
            0.0F, 1.0F);
  lightIntensity = pow(lightIntensity, 4.0F);
  float glowInput =
      clamp(2.0F * (1.0F - smoothstep(halfAngularSize - params.glowSize, halfAngularSize + params.glowSize, angleToLight)),
            0.0F, 1.0F);
  float glowIntensity = params.glowIntensity * pow(glowInput, params.glowSharpness);
  vec3  sunLight      = max(lightIntensity, glowIntensity) * params.lightRadiance;

  return environment + sunLight;
}

inline SkySamplingResult sampleSimpleSky(SimpleSkyParameters params, vec2 randVal)
{
  SkySamplingResult result;

  // Constants
  const float sunWeight = 0.95f;  // 95.0% weight for the sun
  const float skyWeight = 1.0f - sunWeight;

  // Decide whether to sample the sun or the sky
  if(randVal.x < sunWeight)
  {
    float sunAngularRadius = params.angularSizeOfLight * 0.5f;

    // Sample the sun
    float theta = sunAngularRadius * sqrt(randVal.y);
    float phi   = 2.0f * M_PIf * randVal.x;

    vec3  sunLocalDir;
    float sinTheta = sin(theta);
    sunLocalDir.x  = sinTheta * cos(phi);
    sunLocalDir.y  = sinTheta * sin(phi);
    sunLocalDir.z  = cos(theta);

    // Transform sun_local_dir to world space
    vec3 up    = vec3(0, 0, 1);
    vec3 right = cross(up, params.directionToLight);
    up         = cross(params.directionToLight, right);

    result.direction = sunLocalDir.x * right + sunLocalDir.y * up + sunLocalDir.z * params.directionToLight;

    // Compute the PDF
    result.pdf = sunWeight;  // / (M_TWO_PI * (1.0f - cos(angular_size)));

    // Compute the light intensity (assuming uniform sun disk)
    result.radiance = params.lightRadiance;
  }
  else
  {
    // Sample the sky (simple uniform sampling of the upper hemisphere)
    float cosTheta = sqrt(1.0f - randVal.y);
    float sinTheta = sqrt(randVal.y);
    float phi      = 2.0f * M_PIf * randVal.x;

    result.direction = vec3(sinTheta * cos(phi), cosTheta, sinTheta * sin(phi));

    // Ensure the sampled direction is in the upper hemisphere
    if(dot(result.direction, params.directionUp) < 0.0f)
    {
      result.direction.y = -result.direction.y;
    }

    // Compute the PDF (uniform sampling of hemisphere)
    result.pdf = skyWeight;  // / M_TWO_PI;

    // Compute sky color (simplified version of proceduralSky function)
    float elevation = asin(clamp(dot(result.direction, params.directionUp), -1.0f, 1.0f));
    float t         = smoothstep(0.0f, params.horizonSize, elevation);
    result.radiance = mix(params.horizonColor, params.skyColor, t);
  }

  return result;
}


////////////////////////////////////////////////////////////////////////
// Physical Sky
////////////////////////////////////////////////////////////////////////
struct PhysicalSkyParameters
{
  vec3  rgbUnitConversion;
  float multiplier;

  float haze;
  float redblueshift;
  float saturation;
  float horizonHeight;

  vec3  groundColor;
  float horizonBlur;

  vec3  nightColor;
  float sunDiskIntensity;

  vec3  sunDirection;
  float sunDiskScale;

  float sunGlowIntensity;
  int   yIsUp;
};


// Function to initialize SunAndSky with default values
inline PhysicalSkyParameters initPhysicalSkyParameters()
{
  PhysicalSkyParameters ss;

  // Default RGB unit conversion (assuming standard sRGB primaries)
  ss.rgbUnitConversion = vec3(1.0f / 80000.0f);

  // Default multiplier (overall intensity scaling)
  ss.multiplier = 0.10f;

  // Default atmospheric conditions
  ss.haze         = 0.1f;
  ss.redblueshift = 0.1f;
  ss.saturation   = 1.0f;

  // Default horizon settings
  ss.horizonHeight = 0.0f;
  ss.horizonBlur   = 0.3f;

  // Default ground color (neutral gray)
  ss.groundColor = vec3(0.4f);

  // Default night sky settings
  ss.nightColor = vec3(0.0f, 0.0f, 0.01f);

  // Default sun settings
  ss.sunDiskIntensity = 1.0f;
  ss.sunDiskScale     = 1.0f;
  ss.sunGlowIntensity = 1.0f;

  // Default sun direction (45 degrees above horizon, facing south)
  float elevation = radians(45.0f);
  ss.sunDirection = normalize(vec3(0.0f, sin(elevation), cos(elevation)));

  // Default coordinate system (Y is up)
  ss.yIsUp = 1;

  return ss;
}


/*helper functions for sun_and_sky*/

// Function to calculate the luminance of an RGB color
inline float rgbLuminance(vec3 rgb)
{
  return 0.2126f * rgb.x + 0.7152f * rgb.y + 0.0722f * rgb.z;  // Rec. 709 luminance coefficients
}

// Function to transform local coordinates (x, y, z) to a direction vector aligned with in_main
inline vec3 localCoordsToDir(vec3 mainVec, float x, float y, float z)
{
  vec3 u =
      normalize(abs(mainVec.x) < abs(mainVec.y) ? vec3(0.0f, -mainVec.z, mainVec.y) : vec3(mainVec.z, 0.0f, -mainVec.x));
  vec3 v = cross(mainVec, u);
  return x * u + y * v + z * mainVec;
}


inline vec2 squareToDisk(float inX, float inY)
{
  float localX = 2.0f * inX - 1.0f;
  float localY = 2.0f * inY - 1.0f;
  if(localX == 0.0f && localY == 0.0f)
    return vec2(0.0f);

  float r, phi;
  if(localX > -localY)
  {
    if(localX > localY)
    {
      r   = localX;
      phi = (M_PIf / 4.0f) * (1.0f + localY / localX);
    }
    else
    {
      r   = localY;
      phi = (M_PIf / 4.0f) * (3.0f - localX / localY);
    }
  }
  else
  {
    if(localX < localY)
    {
      r   = -localX;
      phi = (M_PIf / 4.0f) * (5.0f + localY / localX);
    }
    else
    {
      r   = -localY;
      phi = (M_PIf / 4.0f) * (7.0f - localX / localY);
    }
  }
  return vec2(r, phi);
}

// Function to calculate a diffuse reflection direction based on an input normal and sample
inline vec3 reflectionDirDiffuseX(vec3 inNormal, vec2 inSample)
{
  vec2  rPhi = squareToDisk(inSample.x, inSample.y);   // Map the 2D sample point to a disk
  float x    = rPhi.x * cos(rPhi.y);                   // Calculate the x component of the reflection direction
  float y    = rPhi.x * sin(rPhi.y);                   // Calculate the y component of the reflection direction
  float z    = sqrt(max(0.0f, 1.0f - x * x - y * y));  // Calculate the z component, ensuring it is non-negative
  return localCoordsToDir(inNormal, x, y, z);  // Convert the local direction to the world space direction using the normal
}

// Function to calculate the color of the sun based on its direction and atmospheric turbidity
inline vec3 calcSunColor(vec3 sunDir, float turbidity)
{
  if(sunDir.z <= 0.0f)
    return vec3(0.0f);

  vec3 ko         = vec3(12.0f, 8.5f, 0.9f);       // Optical depth constants for ozone
  vec3 wavelength = vec3(0.610f, 0.550f, 0.470f);  // Wavelength of light (in micrometers) for RGB channels
  vec3 solRad = vec3(1.0f, 0.992f, 0.911f) * (127500.0f / 0.9878f);  // Solar radiance adjusted for atmospheric effects

  float m = 1.0f / (sunDir.z + 0.15f * pow(93.885f - degrees(acos(sunDir.z)), -1.253f));  // Optical air mass calculation (simplified relative air mass formula)
  float beta = 0.04608f * turbidity - 0.04586f;  // Beta coefficient for Rayleigh scattering based on turbidity
  vec3  ta   = exp(-m * beta * pow(wavelength, vec3(-1.3f)));        // Attenuation due to aerosol (Rayleigh) scattering
  vec3  to   = exp(-m * ko * 0.0035f);                               // Attenuation due to ozone absorption
  vec3  tr   = exp(-m * 0.008735f * pow(wavelength, vec3(-4.08f)));  // Attenuation due to Rayleigh scattering

  return tr * ta * to * solRad;  // Calculate the final sun color based on the combined effects
}

// Function to calculate the color of the sky based on the sun's direction and atmospheric turbidity
inline vec3 skyColorXyz(vec3 inDir, vec3 inSunPos, float inTurbidity, float inLuminance)
{
  vec3  xyz;
  float A, B, C, D, E;
  float cosGamma = dot(inSunPos, inDir);
  if(cosGamma > 1.0f)
  {
    cosGamma = 2.0f - cosGamma;
  }
  float gamma       = acos(cosGamma);
  float cosTheta    = inDir.z;
  float cosThetaSun = inSunPos.z;
  float thetaSun    = acos(cosThetaSun);
  float t2          = inTurbidity * inTurbidity;
  float ts2         = thetaSun * thetaSun;
  float ts3         = ts2 * thetaSun;
  // determine x and y at zenith
  float zenithX = ((+0.001650f * ts3 - 0.003742f * ts2 + 0.002088f * thetaSun + 0) * t2
                   + (-0.029028f * ts3 + 0.063773f * ts2 - 0.032020f * thetaSun + 0.003948f) * inTurbidity
                   + (+0.116936f * ts3 - 0.211960f * ts2 + 0.060523f * thetaSun + 0.258852f));
  float zenithY = ((+0.002759f * ts3 - 0.006105f * ts2 + 0.003162f * thetaSun + 0) * t2
                   + (-0.042149f * ts3 + 0.089701f * ts2 - 0.041536f * thetaSun + 0.005158f) * inTurbidity
                   + (+0.153467f * ts3 - 0.267568f * ts2 + 0.066698f * thetaSun + 0.266881f));
  xyz.y         = inLuminance;
  // TODO: Preetham/Utah

  // use flags (see above)
  A = -0.019257f * inTurbidity - (0.29f - pow(cosThetaSun, 0.5f) * 0.09f);
  B = -0.066513f * inTurbidity + 0.000818f;
  C = -0.000417f * inTurbidity + 0.212479f;
  D = -0.064097f * inTurbidity - 0.898875f;
  E = -0.003251f * inTurbidity + 0.045178f;

  float x = (((1.f + A * exp(B / cosTheta)) * (1.f + C * exp(D * gamma) + E * cosGamma * cosGamma))
             / ((1 + A * exp(B / 1.0f)) * (1 + C * exp(D * thetaSun) + E * cosThetaSun * cosThetaSun)));

  A = -0.016698f * inTurbidity - 0.260787f;
  B = -0.094958f * inTurbidity + 0.009213f;
  C = -0.007928f * inTurbidity + 0.210230f;
  D = -0.044050f * inTurbidity - 1.653694f;
  E = -0.010922f * inTurbidity + 0.052919f;

  float y = (((1 + A * exp(B / cosTheta)) * (1 + C * exp(D * gamma) + E * cosGamma * cosGamma))
             / ((1 + A * exp(B / 1.0f)) * (1 + C * exp(D * thetaSun) + E * cosThetaSun * cosThetaSun)));

  float localSaturation = 1.0f;

  x = zenithX * ((x * localSaturation) + (1.0f - localSaturation));
  y = zenithY * ((y * localSaturation) + (1.0f - localSaturation));

  // convert chromaticities x and y to CIE
  xyz.x = (x / y) * xyz.y;
  xyz.z = ((1.0f - x - y) / y) * xyz.y;
  return xyz;
}

inline float skyLuminance(vec3 inDir, vec3 inSunPos, float inTurbidity)
{
  float cosGamma    = clamp(dot(inSunPos, inDir), 0.0f, 1.0f);
  float gamma       = acos(cosGamma);
  float cosTheta    = inDir.z;
  float cosThetaSun = inSunPos.z;
  float thetaSun    = acos(cosThetaSun);

  float A = 0.178721f * inTurbidity - 1.463037f;
  float B = -0.355402f * inTurbidity + 0.427494f;
  float C = -0.022669f * inTurbidity + 5.325056f;
  float D = 0.120647f * inTurbidity - 2.577052f;
  float E = -0.066967f * inTurbidity + 0.370275f;

  return (((1 + A * exp(B / cosTheta)) * (1 + C * exp(D * gamma) + E * cosGamma * cosGamma))
          / ((1 + A * exp(B / 1.0f)) * (1 + C * exp(D * thetaSun) + E * cosThetaSun * cosThetaSun)));
}


inline vec3 calcSkyColor(vec3 inSunDir, vec3 inDir, float inTurbidity)
{
  float thetaSun  = acos(inSunDir.z);
  float chi       = (4.0f / 9.0f - inTurbidity / 120.0f) * (M_PIf - 2.0f * thetaSun);
  float luminance = 1000.0f * ((4.0453f * inTurbidity - 4.9710f) * tan(chi) - 0.2155f * inTurbidity + 2.4192f);
  luminance *= skyLuminance(inDir, inSunDir, inTurbidity);

  vec3 xyz = skyColorXyz(inDir, inSunDir, inTurbidity, luminance);
  vec3 envColor = vec3(3.241f * xyz.x - 1.537f * xyz.y - 0.499f * xyz.z, -0.969f * xyz.x + 1.876f * xyz.y + 0.042f * xyz.z,
                       0.056f * xyz.x - 0.204f * xyz.y + 1.057f * xyz.z);
  return envColor * M_PIf;
}

inline vec3 calcSkyIrradiance(vec3 inDataSunDir, float inDataSunDirHaze)
{
  vec3 colSum        = vec3(0.0f);
  vec3 nuStateNormal = vec3(0.0f, 0.0f, 1.0f);

  for(float u = 0.1f; u < 1.0f; u += 0.2f)
  {
    for(float v = 0.1f; v < 1.0f; v += 0.2f)
    {
      vec3 diff = reflectionDirDiffuseX(nuStateNormal, vec2(u, v));
      colSum += calcSkyColor(inDataSunDir, diff, inDataSunDirHaze);
    }
  }
  return colSum / 25.0f;
}

inline float tweakSaturation(float inSaturation, float inHaze)
{
  if(inSaturation > 1.0f)
    return 1.0f;

  float lowSat    = inSaturation * inSaturation * inSaturation;  // Compute the low saturation value
  float localHaze = clamp((inHaze - 2.0f) / 15.0f, 0.0f, 1.0f);  // Normalize the local haze value
  localHaze *= localHaze * localHaze;                            // Adjust haze to the power of 3
  return mix(inSaturation, lowSat, localHaze);  // Blend the input saturation with the low saturation value based on local haze
}


inline vec3 tweakVector(vec3 dir, int yIsUp, float horizHeight)
{
  vec3 outDir = dir;
  if(yIsUp == 1)
  {
    outDir = vec3(dir.x, dir.z, dir.y);
  }
  if(horizHeight != 0)
  {
    outDir.z -= horizHeight;
    outDir = normalize(outDir);
  }
  return outDir;
}


inline vec3 tweakColor(vec3 tint, float saturation, float redness)
{
  float intensity = rgbLuminance(tint);
  vec3  outTint   = (saturation <= 0.0f) ? vec3(intensity) : mix(vec3(intensity), tint, saturation);
  outTint *= vec3(1.0f + redness, 1.0f, 1.0f - redness);
  return max(outTint, vec3(0.0f));
}


inline vec2 calcPhysicalScale(float sunDiskScale, float sunGlowIntensity, float sunDiskIntensity)
{
  float sunAngularRadius = 0.00465f;
  float sunDiskRadius    = sunAngularRadius * sunDiskScale;
  float sunGlowRadius    = sunDiskRadius * 10.0f;

  /* We calculate the integral of the glow intensity function */
  float glowFuncIntegral = sunGlowIntensity
                           * ((4.0f * M_PIf) - (24.0f * M_PIf) / (sunGlowRadius * sunGlowRadius)
                              + (24.0f * M_PIf) * sin(sunGlowRadius) / (sunGlowRadius * sunGlowRadius * sunGlowRadius));


  /* Calculate the target sun disk intensity integral (the value towards which
    we must scale to attain a physically-scaled sun intensity */
  float targetSundiskIntegral = sunDiskIntensity * M_PIf;

  /* Subtract the glow integral from the target disk integral,
    limiting the glow power to 50% of the sun disk */
  float skySunglowScale = 1.0f;
  float maxGlowIntegral = 0.5f * targetSundiskIntegral;
  if(glowFuncIntegral > maxGlowIntegral)
  {
    skySunglowScale *= maxGlowIntegral / glowFuncIntegral;
    targetSundiskIntegral -= maxGlowIntegral;
  }
  else
  {
    targetSundiskIntegral -= glowFuncIntegral;
  }

  float sundiskArea            = 2 * M_PIf * (1 - cos(sunDiskRadius));
  float targetSundiskIntensity = targetSundiskIntegral / sundiskArea;

  /* Calculate the actual sun disk intensity, before scaling is applied */
  float actualSundiskIntegral = 1.0f * sundiskArea;
  /* approximation! needs to be re-calculated from the integral of the function */
  float actualSundiskIntensity = sunDiskIntensity * 100.0f * actualSundiskIntegral / sundiskArea;
  /* Apply the proper scaling to get to the target value */
  return vec2((targetSundiskIntensity == 0.0f) ? 0.0f : targetSundiskIntensity / actualSundiskIntensity, skySunglowScale);
}


inline float nightBrightnessAdjustment(vec3 sunDir)
{
  float lmt = 0.30901699437494742410229341718282f;  // ~ cos(72°) or 18° below the horizon
  if(sunDir.z <= -lmt)
    return 0.0f;
  float factor = (sunDir.z + lmt) / lmt;
  factor *= factor;
  factor *= factor;
  return factor;
}


inline vec3 evalPhysicalSky(PhysicalSkyParameters ss, vec3 inDirection)
{
  if(ss.multiplier <= 0.0f)
    return vec3(0.0f);

  vec3  result          = vec3(0.0f);
  float factor          = 1.0f;
  float nightFactor     = 1.0f;
  vec3  outColor        = vec3(0.0f);
  vec3  rgbScale        = ss.rgbUnitConversion * ss.multiplier;
  float heightAdjusted  = (ss.horizonHeight + ss.horizonBlur) / 10.0f;
  vec3  dir             = tweakVector(inDirection, ss.yIsUp, heightAdjusted);
  float localHaze       = max(2.0f, 2.0f + ss.haze);
  float localSaturation = tweakSaturation(ss.saturation, localHaze);

  // Calculate the "downness" of the direction (how much it points downward)
  float downness = dir.z;
  vec3  realDir  = dir;
  if(dir.z < 0.001f)
  {
    dir.z = 0.001f;
    dir   = normalize(dir);
  }

  // Adjust the sun direction similarly to the input direction
  vec3 sunDir     = ss.sunDirection;
  sunDir          = tweakVector(sunDir, ss.yIsUp, heightAdjusted);
  vec3 realSunDir = sunDir;
  if(sunDir.z < 0.001f)
  {
    // Adjust brightness for night time
    factor   = nightBrightnessAdjustment(sunDir);
    sunDir.z = 0.001f;
    sunDir   = normalize(sunDir);
  }

  // Calculate the sun and sky color
  vec3 tint         = (factor > 0.0f) ? calcSkyColor(sunDir, dir, localHaze) * factor : vec3(0.0f);
  vec3 dataSunColor = calcSunColor(sunDir, (downness > 0) ? localHaze : 2.0f);

  // Add the sun disk and glow if enabled
  if(ss.sunDiskIntensity > 0.0f && ss.sunDiskScale > 0.0f)
  {
    float sunAngle  = acos(dot(realDir, realSunDir));
    float sunRadius = 0.00465f * ss.sunDiskScale * 10.0f;
    if(sunAngle < sunRadius)
    {
      vec2  scales    = calcPhysicalScale(ss.sunDiskScale, ss.sunGlowIntensity, ss.sunDiskIntensity);
      float sunFactor = pow((1.0f - sunAngle / sunRadius) * 10.0f / 10.0f, 3.0f) * 2.0f * ss.sunGlowIntensity * scales.y
                        + smoothstep(8.5f, 9.5f + (localHaze / 50.0f), (1.0f - sunAngle / sunRadius) * 10.0f) * 100.0f
                              * ss.sunDiskIntensity * scales.x;
      tint += dataSunColor * sunFactor;
    }
  }
  outColor = tint * rgbScale;

  // Add ground color if the direction is pointing downward
  if(downness <= 0.0f)
  {
    vec3 irrad     = calcSkyIrradiance(sunDir, 2.0f);
    vec3 downColor = ss.groundColor * (irrad + dataSunColor * sunDir.z) * rgbScale;
    downColor *= factor;
    float horBlur = ss.horizonBlur / 10.0f;
    if(horBlur > 0.0f)
    {
      // Blend between sky and ground color at the horizon
      float dness = smoothstep(0.0f, 1.0f, -downness / horBlur);
      outColor    = mix(outColor, downColor, dness);
      nightFactor = 1.0f - dness;
    }
    else
    {
      outColor    = downColor;
      nightFactor = 0.0f;
    }
  }

  // Adjusting the color based on the saturation and red-blue shift
  outColor = tweakColor(outColor, localSaturation, ss.redblueshift);
  result   = outColor * M_PIf;

  // Adding the night sky color
  if(nightFactor > 0.0f)
  {
    vec3 night = ss.nightColor * nightFactor;
    result     = max(result, night);
  }

  return result;
}

// Sampling the sun and sky model
inline SkySamplingResult samplePhysicalSky(PhysicalSkyParameters ss, vec2 randomSample)
{
  SkySamplingResult result;

  // Constants
  const float epsilon          = 1e-5f;
  const float sunAngularRadius = max(0.00465f * ss.sunDiskScale, epsilon);
  // Use first-degree Taylor series expansion around 0 for better precision
  const float sunSolidAngle = (sunAngularRadius < 0.001f) ? M_PIf * sunAngularRadius * sunAngularRadius :
                                                            2.0f * M_PIf * (1.0f - cos(sunAngularRadius));

  // Approximation for sun probability
  float sunElevation = ss.sunDirection.z;
  float sunProb = (ss.sunDiskScale > epsilon) ? clamp(ss.sunDiskIntensity * sunElevation * 0.5f + 0.5f, 0.1f, 0.9f) : 0.0f;

  // Improved MIS weighting
  float misWeight;

  // Decide whether to sample sun or sky
  if(randomSample.x < sunProb)
  {
    // Sample the sun
    float theta = sunAngularRadius * sqrt(randomSample.y);
    float phi   = 2.0f * M_PIf * fract(randomSample.x / sunProb);

    vec3  sunLocalDir;
    float sinTheta = sin(theta);
    sunLocalDir.x  = sinTheta * cos(phi);
    sunLocalDir.y  = sinTheta * sin(phi);
    sunLocalDir.z  = cos(theta);

    // Transform sun_local_dir to world space
    vec3 up    = vec3(0, 0, 1);
    vec3 right = normalize(cross(up, ss.sunDirection));
    up         = cross(ss.sunDirection, right);

    result.direction = sunLocalDir.x * right + sunLocalDir.y * up + sunLocalDir.z * ss.sunDirection;

    result.pdf = sunProb / sunSolidAngle;
    // MIS weight calculation
    float skyPdf = (1.0f - sunProb) * max(result.direction.z, 0.001f) / M_PIf;
    misWeight    = result.pdf / (result.pdf + skyPdf);
  }
  else
  {
    // Sample the sky using cosine-weighted hemisphere sampling
    float z        = sqrt(randomSample.y);
    float phi      = 2.0f * M_PIf * fract((randomSample.x - sunProb) / (1.0f - sunProb));
    float sqrtTerm = sqrt(1.0f - z * z);
    float x        = cos(phi) * sqrtTerm;
    float y        = sin(phi) * sqrtTerm;

    result.direction = vec3(x, y, z);

    // Ensure the sampled direction is above the horizon
    if(result.direction.z < 0.001f)
    {
      result.direction.z = 0.001f;
      result.direction   = normalize(result.direction);
    }

    result.pdf = (1.0f - sunProb) * result.direction.z / M_PIf;
    // MIS weight calculation
    float sunPdf = (dot(result.direction, ss.sunDirection) > cos(sunAngularRadius)) ? sunProb / sunSolidAngle : 0.0f;
    misWeight    = result.pdf / (result.pdf + sunPdf);
  }

  // Evaluate the sky model
  result.radiance = evalPhysicalSky(ss, result.direction) * misWeight;
  return result;
}

#ifdef __cplusplus
}  // namespace nvvkhl_shaders
#endif


#endif  // DH_SKY_H
