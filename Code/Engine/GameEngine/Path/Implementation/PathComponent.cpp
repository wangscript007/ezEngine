
#include <GameEnginePCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>

#include <GameEngine/Path/PathComponent.h>

// clang-format off
EZ_BEGIN_STATIC_REFLECTED_TYPE(ezPathKnot, ezNoBase, 1, ezRTTIDefaultAllocator<ezPathKnot>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ACCESSOR_PROPERTY("LocalPosition", GetLocalPosition, SetLocalPosition)->AddAttributes(new ezSuffixAttribute(" m")),
  }
  EZ_END_PROPERTIES;

  EZ_BEGIN_ATTRIBUTES
  {
    new ezTransformManipulatorAttribute("LocalPosition"),
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_STATIC_REFLECTED_TYPE
// clang-format on


void ezPathKnot::SetLocalPosition(ezVec3 position)
{
  m_Position = position;
}

ezVec3 ezPathKnot::GetLocalPosition() const
{
  return m_Position;
}

static const ezTypeVersion s_PathControlPointVersion = 1;
ezResult ezPathKnot::Serialize(ezStreamWriter& writer) const
{
  writer.WriteVersion(s_PathControlPointVersion);

  writer << m_Position;

  return EZ_SUCCESS;
}

ezResult ezPathKnot::Deserialize(ezStreamReader& reader)
{
  /*auto version = */ reader.ReadVersion(s_PathControlPointVersion);

  reader >> m_Position;

  return EZ_SUCCESS;
}

// clang-format off
EZ_BEGIN_COMPONENT_TYPE(ezPathComponent, 1, ezComponentMode::Static)
{
    EZ_BEGIN_PROPERTIES
    {
      EZ_MEMBER_PROPERTY("ClosedLoop", m_bClosedLoop),
      EZ_ARRAY_ACCESSOR_PROPERTY("Knots", Knots_GetCount, Knots_GetValue, Knots_SetValue, Knots_Insert, Knots_Remove),
    }
    EZ_END_PROPERTIES;
}
EZ_END_COMPONENT_TYPE
// clang-format on


ezPathComponent::ezPathComponent() = default;
ezPathComponent::~ezPathComponent() = default;

ezArrayPtr<const ezPathKnot> ezPathComponent::GetKnots() const
{
  return m_Knots.GetArrayPtr();
}

const ezBezierPath& ezPathComponent::GetBezierPath() const
{
  return m_BezierPath;
}

void ezPathComponent::UpdateBezierPath()
{
  ezHybridArray<ezVec3, 64> pathKnots;

  for (const auto& knot : m_Knots)
  {
    pathKnots.PushBack(knot.m_Position);
  }

  m_BezierPath = ezBezierPath::FromKnots(pathKnots, m_bClosedLoop);
}

ezUInt32 ezPathComponent::Knots_GetCount() const
{
  return m_Knots.GetCount();
}

ezPathKnot ezPathComponent::Knots_GetValue(ezUInt32 uiIndex) const
{
  return m_Knots[uiIndex];
}

void ezPathComponent::Knots_SetValue(ezUInt32 uiIndex, ezPathKnot value)
{
  m_Knots[uiIndex] = value;

  UpdateBezierPath();
}

void ezPathComponent::Knots_Insert(ezUInt32 uiIndex, ezPathKnot value)
{
  m_Knots.Insert(value, uiIndex);

  UpdateBezierPath();
}

void ezPathComponent::Knots_Remove(ezUInt32 uiIndex)
{
  m_Knots.RemoveAtAndCopy(uiIndex);

  UpdateBezierPath();
}
