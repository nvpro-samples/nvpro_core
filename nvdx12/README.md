# Helpers nvdx12

Table of Contents

- [base_dx12.hpp](#base_dx12hpp)
- [context_dx12.hpp](#context_dx12hpp)
- [error_dx12.hpp](#error_dx12hpp)
_____

# base_dx12.hpp

<a name="base_dx12hpp"></a>
## class nvdx12::DeviceUtils
  Utility class for simple creation of pipeline states, root signatures,
and buffers.



## function nvdx12::transitionBarrier
  Short-hand function to create a transition barrier



_____

# context_dx12.hpp

<a name="context_dx12hpp"></a>
## class nvdx12::Context
  Container class for a basic DX12 app, consisting of a DXGI factory, a DX12
device, and a command queue.
 


## struct nvdx12::ContextCreateInfo
  Properties for context initialization.



_____

# error_dx12.hpp

<a name="error_dx12hpp"></a>
## function nvdx12::checkResult
> Returns true on critical error result, logs errors.

Use `HR_CHECK(result)` to automatically log filename/linenumber.



