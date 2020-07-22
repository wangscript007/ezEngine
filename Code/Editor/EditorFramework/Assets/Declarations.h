#pragma once

#include <Foundation/Strings/HashedString.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Types/Bitflags.h>
#include <Foundation/Types/Uuid.h>

class ezImage;
class ezAssetFileHeader;

struct ezAssetExistanceState
{
  enum Enum : ezUInt8
  {
    FileAdded,
    FileRemoved,
    FileModified,
    FileUnchanged,
  };
};

struct ezAssetDocumentFlags
{
  typedef ezUInt8 StorageType;

  enum Enum
  {
    None = 0,
    AutoTransformOnSave = EZ_BIT(0),   ///< Every time the document is saved, TransformAsset is automatically executed
    DisableTransform = EZ_BIT(1),      ///< If set, TransformAsset will not do anything
    OnlyTransformManually = EZ_BIT(2), ///< The asset transformation is not done, unless explicitly requested for this asset
    SupportsThumbnail = EZ_BIT(3),     ///< The asset supports thumbnail generation (InternalCreateThumbnail must be implemented).
    AutoThumbnailOnTransform =
      EZ_BIT(4), ///< Thumbnail is automatically generated by the asset transform, and does not need to be explicitly created.
    Default = None
  };

  struct Bits
  {
    StorageType AutoTransformOnSave : 1;
    StorageType DisableTransform : 1;
    StorageType OnlyTransformManually : 1;
    StorageType SupportsThumbnail : 1;
    StorageType AutoThumbnailOnTransform : 1;
  };
};

EZ_DECLARE_FLAGS_OPERATORS(ezAssetDocumentFlags);

struct ezTransformFlags
{
  typedef ezUInt8 StorageType;

  enum Enum
  {
    None = 0,
    TriggeredManually = EZ_BIT(0), ///< Transform triggered by user directly. Needs to be set to transform assets marked with
                                   ///< ezAssetDocumentFlags::Enum::OnlyTransformManually.
    ForceTransform = EZ_BIT(1),    ///< Will transform the asset regardless of its current transform state.
    Default = None
  };

  struct Bits
  {
    StorageType TriggeredManually : 1;
    StorageType ForceTransform : 1;
  };
};
EZ_DECLARE_FLAGS_OPERATORS(ezTransformFlags);

struct ezSubAssetData
{
  ezUuid m_Guid;
  ezHashedString m_sSubAssetsDocumentTypeName;
  ezString m_sName;
};

struct EZ_EDITORFRAMEWORK_DLL ezAssetDocumentTypeDescriptor : public ezDocumentTypeDescriptor
{
  ezAssetDocumentTypeDescriptor() = default;
  ~ezAssetDocumentTypeDescriptor() = default;

  ezString m_sResourceFileExtension;
  ezBitflags<ezAssetDocumentFlags> m_AssetDocumentFlags;
};
