//
// Copyright 2016 Pixar
//
// Licensed under the Apache License, Version 2.0 (the "Apache License")
// with the following modification; you may not use this file except in
// compliance with the Apache License and the following modification to it:
// Section 6. Trademarks. is deleted and replaced with:
//
// 6. Trademarks. This License does not grant permission to use the trade
//    names, trademarks, service marks, or product names of the Licensor
//    and its affiliates, except as required to comply with Section 4(c) of
//    the License and to reproduce the content of the NOTICE file.
//
// You may obtain a copy of the Apache License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the Apache License with the above modification is
// distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied. See the Apache License for the specific
// language governing permissions and limitations under the Apache License.
//
#include "pxr/pxr.h"
#include "pxr/base/arch/env.h"
#include "pxr/base/arch/error.h"

#include <memory>
#include <regex>

#include <stdlib.h>

PXR_NAMESPACE_OPEN_SCOPE

bool
ArchHasEnv(const std::string &name)
{
#if defined(ARCH_OS_WINDOWS)
    size_t requiredSize;
    getenv_s(&requiredSize, nullptr, 0, name.c_str());
    return requiredSize != 0;
#else
    return static_cast<bool>(getenv(name.c_str()));
#endif
}

std::string
ArchGetEnv(const std::string &name)
{
#if defined(ARCH_OS_WINDOWS)
    size_t requiredSize;

    getenv_s(&requiredSize, nullptr, 0, name.c_str());
    if (requiredSize) {
        std::unique_ptr<char[]> buffer(new char[requiredSize]);
        getenv_s(&requiredSize, buffer.get(), requiredSize, name.c_str());
        return std::string(buffer.get());
    }

#else
    if (const char* const result = getenv(name.c_str())) {
        return std::string(result);
    }
#endif

    return std::string();
}

bool
ArchSetEnv(const std::string &name, const std::string &value, bool overwrite)
{
    // NOTE: Setting environment variables must be externally synchronized
    //       with other sets and gets to avoid race conditions.

#if defined(ARCH_OS_WINDOWS)
    if (!overwrite) {
        size_t requiredSize;
        getenv_s(&requiredSize, nullptr, 0, name.c_str());
        if (requiredSize) {
            // Already exists.
            return true;
        }
    }
    if (_putenv_s(name.c_str(), value.c_str()) == 0) {
#else
    if (setenv(name.c_str(), value.c_str(), overwrite ? 1 : 0) == 0) {
#endif
        return true;
    }

    return false;
}

bool ArchRemoveEnv(const std::string &name)
{
#if defined(ARCH_OS_WINDOWS)
    return _putenv_s(name.c_str(), "") == 0;
#else
    return unsetenv(name.c_str()) == 0;
#endif
}

std::string
ArchExpandEnvironmentVariables(const std::string& value)
{
#if defined(ARCH_OS_WINDOWS)
    static std::regex regex("\\%([^\\%]+)\\%");
#else
    static std::regex regex("\\$\\{([^}]+)\\}");
#endif

    std::string result = value;

    std::smatch match;
    while (std::regex_search(result, match, regex)) {
        // NOTE: g++'s standard library's replace() wants non-const iterators
        //       in violation of the standard.  We work around this by using
        //       indexes.
        const std::string::size_type pos   = match[0].first  - result.begin();
        const std::string::size_type count = match[0].second - match[0].first;
        result.replace(pos, count, ArchGetEnv(match[1].str()));
    }

    return result;
}

PXR_NAMESPACE_CLOSE_SCOPE
