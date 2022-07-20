/*
 * Copyright (c) 2021, NVIDIA CORPORATION.  All rights reserved.
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
 * SPDX-FileCopyrightText: Copyright (c) 2021 NVIDIA CORPORATION
 * SPDX-License-Identifier: Apache-2.0
 */


//////////////////////////////////////////////////////////////////////////
/**
\fn nvdx12::checkResult
\brief Returns true on critical error result, logs errors.

Use `HR_CHECK(result)` to automatically log filename/linenumber.
*/

#pragma once

#include <cassert>
#include <d3d12.h>

namespace nvdx12 {

bool checkResult(HRESULT hr, const char* message = nullptr);
bool checkResult(HRESULT hr, const char* file, int line);

#ifndef HR_CHECK
#define HR_CHECK(result) nvdx12::checkResult(result, __FILE__, __LINE__)
#endif

}  // namespace nvdx12
