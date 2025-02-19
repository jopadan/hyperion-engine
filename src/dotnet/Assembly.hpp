/* Copyright (c) 2024 No Tomorrow Games. All rights reserved. */

#ifndef HYP_DOTNET_ASSEMBLY_HPP
#define HYP_DOTNET_ASSEMBLY_HPP

#include <core/memory/UniquePtr.hpp>
#include <core/memory/RefCountedPtr.hpp>

#include <dotnet/interop/ManagedGuid.hpp>

namespace hyperion {

class HypClass;
struct HypData;

namespace dotnet {

class Class;
class Assembly;
class Method;

class ClassHolder
{
public:
    friend class Assembly;

    using InvokeMethodFunction = void(*)(ManagedGuid, ManagedGuid, const HypData **, HypData *);

    ClassHolder(Assembly *owner_assembly);
    ClassHolder(const ClassHolder &)                = delete;
    ClassHolder &operator=(const ClassHolder &)     = delete;
    ClassHolder(ClassHolder &&) noexcept            = delete;
    ClassHolder &operator=(ClassHolder &&) noexcept = delete;
    ~ClassHolder()                                  = default;

    bool CheckAssemblyLoaded() const;

    HYP_FORCE_INLINE Assembly *GetOwnerAssembly() const
        { return m_owner_assembly; }

    Class *NewClass(const HypClass *hyp_class, int32 type_hash, const char *type_name, uint32 type_size, TypeID type_id, Class *parent_class, uint32 flags);
    Class *FindClassByName(const char *type_name);
    Class *FindClassByTypeHash(int32 type_hash);

    HYP_FORCE_INLINE InvokeMethodFunction GetInvokeMethodFunction() const
        { return m_invoke_method_fptr; }

    HYP_FORCE_INLINE void SetInvokeMethodFunction(InvokeMethodFunction invoke_method_fptr)
        { m_invoke_method_fptr = invoke_method_fptr; }

    HYP_FORCE_INLINE InvokeMethodFunction GetInvokeGetterFunction() const
        { return m_invoke_getter_fptr; }

    HYP_FORCE_INLINE void SetInvokeGetterFunction(InvokeMethodFunction invoke_getter_fptr)
        { m_invoke_getter_fptr = invoke_getter_fptr; }

    HYP_FORCE_INLINE InvokeMethodFunction GetInvokeSetterFunction() const
        { return m_invoke_setter_fptr; }

    HYP_FORCE_INLINE void SetInvokeSetterFunction(InvokeMethodFunction invoke_setter_fptr)
        { m_invoke_setter_fptr = invoke_setter_fptr; }

private:
    Assembly                            *m_owner_assembly;

    HashMap<int32, UniquePtr<Class>>    m_class_objects;

    // Function pointer to invoke a managed method
    InvokeMethodFunction                m_invoke_method_fptr;
    InvokeMethodFunction                m_invoke_getter_fptr;
    InvokeMethodFunction                m_invoke_setter_fptr;
};

class Assembly
{
public:
    Assembly();
    Assembly(const Assembly &)                  = delete;
    Assembly &operator=(const Assembly &)       = delete;
    Assembly(Assembly &&) noexcept              = delete;
    Assembly &operator=(Assembly &&) noexcept   = delete;
    ~Assembly();

    HYP_FORCE_INLINE ManagedGuid &GetGuid()
        { return m_guid; }

    HYP_FORCE_INLINE const ManagedGuid &GetGuid() const
        { return m_guid; }

    HYP_FORCE_INLINE ClassHolder &GetClassObjectHolder()
        { return m_class_object_holder; }

    HYP_FORCE_INLINE const ClassHolder &GetClassObjectHolder() const
        { return m_class_object_holder; }

    HYP_FORCE_INLINE bool IsLoaded() const
        { return m_guid.IsValid(); }

    bool Unload();

private:
    ManagedGuid m_guid;
    ClassHolder m_class_object_holder;
};

} // namespace dotnet
} // namespace hyperion

#endif