#pragma once

/// \file

#include <Foundation/Reflection/Implementation/AbstractProperty.h>
#include <Foundation/Reflection/Implementation/DynamicRTTI.h>
#include <Foundation/Reflection/Implementation/RTTI.h>
#include <Foundation/Types/Variant.h>

EZ_DECLARE_REFLECTABLE_TYPE(EZ_FOUNDATION_DLL, ezEnumBase);
EZ_DECLARE_REFLECTABLE_TYPE(EZ_FOUNDATION_DLL, ezBitflagsBase);

EZ_DECLARE_REFLECTABLE_TYPE(EZ_FOUNDATION_DLL, ezReflectedClass);

// *********************************************
// ***** Standard POD Types for Properties *****

EZ_DECLARE_REFLECTABLE_TYPE(EZ_FOUNDATION_DLL, bool);

EZ_DECLARE_REFLECTABLE_TYPE(EZ_FOUNDATION_DLL, float);
EZ_DECLARE_REFLECTABLE_TYPE(EZ_FOUNDATION_DLL, double);

EZ_DECLARE_REFLECTABLE_TYPE(EZ_FOUNDATION_DLL, ezInt8);
EZ_DECLARE_REFLECTABLE_TYPE(EZ_FOUNDATION_DLL, ezUInt8);
EZ_DECLARE_REFLECTABLE_TYPE(EZ_FOUNDATION_DLL, ezInt16);
EZ_DECLARE_REFLECTABLE_TYPE(EZ_FOUNDATION_DLL, ezUInt16);
EZ_DECLARE_REFLECTABLE_TYPE(EZ_FOUNDATION_DLL, ezInt32);
EZ_DECLARE_REFLECTABLE_TYPE(EZ_FOUNDATION_DLL, ezUInt32);
EZ_DECLARE_REFLECTABLE_TYPE(EZ_FOUNDATION_DLL, ezInt64);
EZ_DECLARE_REFLECTABLE_TYPE(EZ_FOUNDATION_DLL, ezUInt64);

// macros won't work with 'const char*'
using ezConstCharPtr = const char*;
EZ_DECLARE_REFLECTABLE_TYPE(EZ_FOUNDATION_DLL, ezConstCharPtr);

EZ_DECLARE_REFLECTABLE_TYPE(EZ_FOUNDATION_DLL, ezTime);
EZ_DECLARE_REFLECTABLE_TYPE(EZ_FOUNDATION_DLL, ezColor);
EZ_DECLARE_REFLECTABLE_TYPE(EZ_FOUNDATION_DLL, ezColorBaseUB);
EZ_DECLARE_REFLECTABLE_TYPE(EZ_FOUNDATION_DLL, ezColorGammaUB);
EZ_DECLARE_REFLECTABLE_TYPE(EZ_FOUNDATION_DLL, ezColorLinearUB);
EZ_DECLARE_REFLECTABLE_TYPE(EZ_FOUNDATION_DLL, ezVec2);
EZ_DECLARE_REFLECTABLE_TYPE(EZ_FOUNDATION_DLL, ezVec3);
EZ_DECLARE_REFLECTABLE_TYPE(EZ_FOUNDATION_DLL, ezVec4);
EZ_DECLARE_REFLECTABLE_TYPE(EZ_FOUNDATION_DLL, ezVec2I32);
EZ_DECLARE_REFLECTABLE_TYPE(EZ_FOUNDATION_DLL, ezVec3I32);
EZ_DECLARE_REFLECTABLE_TYPE(EZ_FOUNDATION_DLL, ezVec4I32);
EZ_DECLARE_REFLECTABLE_TYPE(EZ_FOUNDATION_DLL, ezVec2U32);
EZ_DECLARE_REFLECTABLE_TYPE(EZ_FOUNDATION_DLL, ezVec3U32);
EZ_DECLARE_REFLECTABLE_TYPE(EZ_FOUNDATION_DLL, ezVec4U32);
EZ_DECLARE_REFLECTABLE_TYPE(EZ_FOUNDATION_DLL, ezQuat);
EZ_DECLARE_REFLECTABLE_TYPE(EZ_FOUNDATION_DLL, ezMat3);
EZ_DECLARE_REFLECTABLE_TYPE(EZ_FOUNDATION_DLL, ezMat4);
EZ_DECLARE_REFLECTABLE_TYPE(EZ_FOUNDATION_DLL, ezTransform);
EZ_DECLARE_REFLECTABLE_TYPE(EZ_FOUNDATION_DLL, ezBasisAxis);
EZ_DECLARE_REFLECTABLE_TYPE(EZ_FOUNDATION_DLL, ezUuid);
EZ_DECLARE_REFLECTABLE_TYPE(EZ_FOUNDATION_DLL, ezAngle);

EZ_DECLARE_REFLECTABLE_TYPE(EZ_FOUNDATION_DLL, ezVariant);
EZ_DECLARE_REFLECTABLE_TYPE(EZ_FOUNDATION_DLL, ezString);
EZ_DECLARE_REFLECTABLE_TYPE(EZ_FOUNDATION_DLL, ezUntrackedString);
EZ_DECLARE_REFLECTABLE_TYPE(EZ_FOUNDATION_DLL, ezStringView);
EZ_DECLARE_REFLECTABLE_TYPE(EZ_FOUNDATION_DLL, ezDataBuffer);

// **********************************************************************
// ***** Various RTTI infos that can't be put next to their classes *****

EZ_DECLARE_REFLECTABLE_TYPE(EZ_FOUNDATION_DLL, ezTypeFlags);
EZ_DECLARE_REFLECTABLE_TYPE(EZ_FOUNDATION_DLL, ezPropertyFlags);
EZ_DECLARE_REFLECTABLE_TYPE(EZ_FOUNDATION_DLL, ezFunctionType);
EZ_DECLARE_REFLECTABLE_TYPE(EZ_FOUNDATION_DLL, ezVariantType);
EZ_DECLARE_REFLECTABLE_TYPE(EZ_FOUNDATION_DLL, ezPropertyCategory);
