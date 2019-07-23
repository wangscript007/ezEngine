
#pragma once

#include <GameEngine/GameEngineDLL.h>
#include <GameEngine/Path/BezierPath.h>

#include <Core/World/Component.h>
#include <Core/World/ComponentManager.h>
#include <Core/World/Declarations.h>

class ezPathComponent;

typedef ezComponentManager<ezPathComponent, ezBlockStorageType::Compact> ezPathComponentManager;


struct EZ_GAMEENGINE_DLL ezPathKnot
{
  void SetLocalPosition(ezVec3 position);
  ezVec3 GetLocalPosition() const;

  ezResult Serialize(ezStreamWriter& writer) const;
  ezResult Deserialize(ezStreamReader& reader);

  ezVec3 m_Position;
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_GAMEENGINE_DLL, ezPathKnot);

/// \brief A simple path component 
class EZ_GAMEENGINE_DLL ezPathComponent : public ezComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezPathComponent, ezComponent, ezPathComponentManager);

public:
  ezPathComponent();
  ~ezPathComponent();

  ezArrayPtr<const ezPathKnot> GetKnots() const;

  const ezBezierPath& GetBezierPath() const;

protected:

  void UpdateBezierPath();

  ezUInt32 Knots_GetCount() const;
  ezPathKnot Knots_GetValue(ezUInt32 uiIndex) const;
  void Knots_SetValue(ezUInt32 uiIndex, ezPathKnot value);
  void Knots_Insert(ezUInt32 uiIndex, ezPathKnot value);
  void Knots_Remove(ezUInt32 uiIndex);

  ezDynamicArray<ezPathKnot> m_Knots;
  bool m_bClosedLoop = false;

  ezBezierPath m_BezierPath;
};
