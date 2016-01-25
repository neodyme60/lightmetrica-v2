/*
    Lightmetrica - A modern, research-oriented renderer

    Copyright (c) 2015 Hisanari Otsu

    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the "Software"), to deal
    in the Software without restriction, including without limitation the rights
    to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
    copies of the Software, and to permit persons to whom the Software is
    furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included in
    all copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
    THE SOFTWARE.
*/

#include <pch.h>
#include <lightmetrica/component.h>

LM_NAMESPACE_BEGIN

struct CreateAndReleaseFuncs
{
    CreateFuncPointerType createFunc;
    ReleaseFuncPointerType releaseFunc;
};

/*
    Implementation of ComponentFactory.
    Note that in this class the error message cannot be output
    with logger framework (e.g., using LM_LOG_ERROR, etc.)
    because the registration of creation/release function is processed 
    in static initialization phase, so LM_LOG_INIT is not yet be called.
*/
class ComponentFactoryImpl
{
public:

    static ComponentFactoryImpl& Instance()
    {
        static ComponentFactoryImpl instance;
        return instance;
    }

public:

    auto Register(const std::string& key, CreateFuncPointerType createFunc, ReleaseFuncPointerType releaseFunc) -> void
    {
        // Check if already registered
        if (funcMap.find(key) != funcMap.end())
        {
            // Note that in this class the error message cannot be output
            // with logger framework(e.g., using LM_LOG_ERROR, etc.)
            // because the registration of creation / release function is processed
            // in static initialization phase, so LM_LOG_INIT is not yet be called.
            std::cerr << "Failed to register [ " << key << " ]. Already registered." << std::endl;
        }

        funcMap[key] = CreateAndReleaseFuncs{createFunc, releaseFunc};
    }

    auto Create(const char* key) -> Component*
    {
        auto it = funcMap.find(key);
        if (it == funcMap.end())
        {
            return nullptr;
        }

        auto* p = it->second.createFunc();
        p->createFunc = it->second.createFunc;
        p->releaseFunc = it->second.releaseFunc;
        return p;
    }

    auto ReleaseFunc(const char* key) -> ReleaseFuncPointerType
    {
        auto it = funcMap.find(key);
        return it == funcMap.end() ? nullptr : it->second.releaseFunc;
    }

private:

    using FuncMap = std::unordered_map<std::string, CreateAndReleaseFuncs>;
    FuncMap funcMap;

};

auto ComponentFactory_Register(const char* key, CreateFuncPointerType createFunc, ReleaseFuncPointerType releaseFunc) -> void { ComponentFactoryImpl::Instance().Register(key, createFunc, releaseFunc); }
auto ComponentFactory_Create(const char* key) -> Component* { return ComponentFactoryImpl::Instance().Create(key); }
auto ComponentFactory_ReleaseFunc(const char* key) -> ReleaseFuncPointerType { return ComponentFactoryImpl::Instance().ReleaseFunc(key); }

LM_NAMESPACE_END