
#pragma once

#include <GameEngine/GameEngineDLL.h>

#include <Foundation/Math/Vec3.h>

class ezStreamWriter;
class ezStreamReader;

/// \brief A simple Bezier path class using cubic curves.
class EZ_GAMEENGINE_DLL ezBezierPath
{
public:
  ezBezierPath();
  ~ezBezierPath();

  struct Segment
  {
    ezVec3 p0;
    ezVec3 c0;
    ezVec3 c1;
    ezVec3 p1;

    ezVec3 EvaluateAt(float t) const;
  };

  static ezBezierPath FromKnots(ezArrayPtr<ezVec3> Knots, bool bClosed);

  ezArrayPtr<const Segment> GetSegments() const;

protected :
    ezDynamicArray<Segment> m_Segments;
};
