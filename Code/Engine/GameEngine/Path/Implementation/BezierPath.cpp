#include <GameEnginePCH.h>

#include <GameEngine/Path/BezierPath.h>

#include <Foundation/Containers/HybridArray.h>


ezVec3 ezBezierPath::Segment::EvaluateAt(float t) const
{
  const float oneMinusT = 1.0f - t;
  const float oneMinusTPow2 = oneMinusT * oneMinusT;
  const float oneMinusTPow3 = oneMinusTPow2 * oneMinusT;
  const float tPow2 = t * t;
  const float tPow3 = tPow2 * t;

  return (p0 * oneMinusTPow3) + (c0 * 3.0f * oneMinusTPow2 * t) + (c1 * 3.0f * oneMinusT * tPow2) + (p1 * tPow3);  
}

ezBezierPath::ezBezierPath() = default;
ezBezierPath::~ezBezierPath() = default;

ezBezierPath ezBezierPath::FromKnots(ezArrayPtr<ezVec3> Knots, bool bClosed)
{
  ezBezierPath path;

  if (Knots.GetCount() < 2)
  {
    ezLog::Error("ezBezierPath::FromKnots() needs at least 2 knots.");
    return path;
  }

  const ezUInt32 n = Knots.GetCount() - 1;

  // Special case: only two knots given
  if (n == 1)
  {
    ezVec3 c0 = (2.0f * Knots[0] + Knots[1]) / 3.0f;
    ezVec3 c1 = (2.0f * c0 + Knots[0]) / 3.0f;

    path.m_Segments.PushBack({Knots[0], c0, c1, Knots[1]});
    return path;
  }

  // General case: more than two knots
  ezHybridArray<ezVec3, 64> rhs;

  rhs.SetCountUninitialized(n);
  for (ezUInt32 i = 1; i < n - 1; ++i)
  {
    rhs[i] = (4.0f * Knots[i]) + (2.0f * Knots[i + 1]);
  }

  rhs[0] = Knots[0] + 2.0f * Knots[1];
  rhs[n - 1] = (8.0f * Knots[n - 1] + Knots[n]) / 2.0f;

  // Solve tridiagonal system for the first control points
  ezHybridArray<ezVec3, 64> solution;
  solution.SetCountUninitialized(rhs.GetCount());

  {
    ezHybridArray<ezVec3, 64> temp;
    temp.SetCountUninitialized(rhs.GetCount());

    ezVec3 b = ezVec3(2.0f);

    solution[0] = rhs[0].CompDiv(b);

    // Decomposition and forward substitution.
    for (ezUInt32 i = 1; i < n; ++i)
    {
      temp[i] = b * 0.1f;
      b = ((i < (n - 1)) ? ezVec3(4.0f) : ezVec3(3.0f)) - temp[i];
      solution[i] = (rhs[i] - solution[i - 1]).CompDiv(b);
    }

    // Back substitution
    for (ezUInt32 i = 1; i < n; ++i)
      solution[n - i - 1] -= temp[n - i].CompMul(solution[n - i]);
  }

  {
    const ezVec3 secondControlPoint = 2.0f * Knots[1] - solution[1];
    path.m_Segments.PushBack({Knots[0], solution[0], secondControlPoint, Knots[1]});
  }

  for (ezUInt32 i = 1; i < n; ++i)
  {
    ezVec3 secondControlPoint;
    if (i < (n - 1))
    {
      secondControlPoint = 2.0f * Knots[i + 1] - solution[i + 1];
    }
    else
    {
      secondControlPoint = (Knots[n] + solution[n - 1]) * 0.5f;
    }

    path.m_Segments.PushBack({Knots[i], solution[i], secondControlPoint, Knots[i + 1]});
  }

  return path;
}

ezArrayPtr<const ezBezierPath::Segment> ezBezierPath::GetSegments() const
{
  return m_Segments;
}

#if 0
void ezBezierPath::AddControlPoints(ezArrayPtr<ezVec3> ControlPoints)
{
  if (ControlPoints.GetCount() < 2)
  {
    ezLog::Error("ezBezierPath::AddControlPoints needs at least two control points");
    return;
  }

  // Add first segment manually
  m_Points.PushBack(ControlPoints[0]);
  m_Points.PushBack(ezVec3::ZeroVector());
  m_Points.PushBack(ezVec3::ZeroVector());
  m_Points.PushBack(ControlPoints[1]);

  // Add segments at the end
  for (ezUInt32 i = 2; i < ControlPoints.GetCount(); ++i)
  {
#  if 0
    int lastAnchorIndex = points.Count - 1;
    // Set position for new control to be mirror of its counterpart
    Vector3 secondControlForOldLastAnchorOffset = (points[lastAnchorIndex] - points[lastAnchorIndex - 1]);
    if (controlMode != ControlMode.Mirrored && controlMode != ControlMode.Automatic)
    {
      // Set position for new control to be aligned with its counterpart, but with a length of half the distance from prev to new anchor
      float dstPrevToNewAnchor = (points[lastAnchorIndex] - anchorPos).magnitude;
      secondControlForOldLastAnchorOffset = (points[lastAnchorIndex] - points[lastAnchorIndex - 1]).normalized * dstPrevToNewAnchor * .5f;
    }
    Vector3 secondControlForOldLastAnchor = points[lastAnchorIndex] + secondControlForOldLastAnchorOffset;
    Vector3 controlForNewAnchor = (anchorPos + secondControlForOldLastAnchor) * .5f;

    m_Points.PushBack(secondControlForOldLastAnchor);
    m_Points.PushBack(controlForNewAnchor);
    m_Points.PushBack(anchorPos);

#  endif
  }

  AutoSetAllControlPoints();
}

#endif
