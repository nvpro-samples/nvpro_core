## Table of Contents
- [bsdf_functions.h](#bsdf_functionsh)
- [bsdf_structs.h](#bsdf_structsh)
- [dh_comp.h](#dh_comph)
- [dh_inspector.h](#dh_inspectorh)
- [dh_sky.h](#dh_skyh)
- [dh_tonemap.h](#dh_tonemaph)

## bsdf_functions.h
### Function absorptionCoefficient

>  Compute the absorption coefficient of the material
### Function bsdfEvaluate

>  Evaluate the BSDF for the given material
### Function bsdfSample

>  Sample the BSDF for the given material

## bsdf_structs.h
### struct BsdfEvaluateData

>  Data structure for evaluating a BSDF
### struct BsdfSampleData

>  Data structure for sampling a BSDF

## dh_comp.h
### Function getGroupCounts

>  Returns the number of workgroups needed to cover the size

This function is used to calculate the number of workgroups needed to cover a given size. It is used in the compute shader to calculate the number of workgroups needed to cover the size of the image.

## dh_inspector.h
### Function inspect32BitValue

>  Inspect a 32-bit value at a given index
### Function inspectCustom32BitValue

>  Inspect a 32-bit value at a given index

## dh_sky.h
### Function initSkyShaderParameters

>  Initializes the sky shader parameters with default values
### Function proceduralSky

>  Return the color of the procedural sky shader

## dh_tonemap.h
### Function tonemapFilmic

>  Filmic tonemapping operator
http://filmicworlds.com/blog/filmic-tonemapping-operators/
### Function gammaCorrection

>  Gamma correction
see https://www.teamten.com/lawrence/graphics/gamma/
### Function tonemapUncharted

>  Uncharted tone map
see: http://filmicworlds.com/blog/filmic-tonemapping-operators/
