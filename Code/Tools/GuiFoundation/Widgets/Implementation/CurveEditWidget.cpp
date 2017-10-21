#include <PCH.h>
#include <GuiFoundation/Widgets/CurveEditWidget.moc.h>
#include <Foundation/Math/Color.h>
#include <Foundation/Math/Color8UNorm.h>
#include <GuiFoundation/Widgets/GridBarWidget.moc.h>
#include <QPainter>
#include <qevent.h>
#include <QRubberBand>

//////////////////////////////////////////////////////////////////////////

static void AdjustGridDensity(double& fFinestDensity, double& fRoughDensity, ezUInt32 uiWindowWidth, double fOrthoDimX, ezUInt32 uiMinPixelsForStep)
{
  const double fMaxStepsFitInWindow = (double)uiWindowWidth / (double)uiMinPixelsForStep;

  const double fStartDensity = fFinestDensity;

  ezInt32 iFactor = 1;
  double fNewDensity = fFinestDensity;
  ezInt32 iFactors[2] = { 5, 2 };
  ezInt32 iLastFactor = 0;

  while (true)
  {
    const double fStepsAtDensity = fOrthoDimX / fNewDensity;

    if (fStepsAtDensity < fMaxStepsFitInWindow)
      break;

    iFactor *= iFactors[iLastFactor];
    fNewDensity = fStartDensity * iFactor;

    iLastFactor = (iLastFactor + 1) % 2;
  }

  fFinestDensity = fStartDensity * iFactor;

  iFactor *= iFactors[iLastFactor];
  fRoughDensity = fStartDensity * iFactor;
}

static void ComputeGridExtentsX2(const QRectF& viewportSceneRect, double fGridStops, double& out_fMinX, double& out_fMaxX)
{
  out_fMinX = ezMath::Floor((double)viewportSceneRect.left(), fGridStops);
  out_fMaxX = ezMath::Ceil((double)viewportSceneRect.right(), fGridStops);
}

static void ComputeGridExtentsY2(const QRectF& viewportSceneRect, double fGridStops, double& out_fMinY, double& out_fMaxY)
{
  out_fMinY = ezMath::Floor((double)viewportSceneRect.top(), fGridStops);
  out_fMaxY = ezMath::Ceil((double)viewportSceneRect.bottom(), fGridStops);
}

//////////////////////////////////////////////////////////////////////////

ezQtCurveEditWidget::ezQtCurveEditWidget(QWidget* parent)
  : QWidget(parent)
{
  setFocusPolicy(Qt::FocusPolicy::ClickFocus);
  setMouseTracking(true);

  m_SceneTranslation = QPointF(-8, 8);
  m_SceneToPixelScale = QPointF(40, -40);

  m_ControlPointBrush.setColor(QColor(200, 150, 0));
  m_ControlPointBrush.setStyle(Qt::BrushStyle::SolidPattern);

  m_SelectedControlPointBrush.setColor(QColor(220, 200, 50));
  m_SelectedControlPointBrush.setStyle(Qt::BrushStyle::SolidPattern);

  m_TangentLinePen.setCosmetic(true);
  m_TangentLinePen.setColor(QColor(100, 100, 255));
  m_TangentLinePen.setStyle(Qt::PenStyle::DashLine);

  m_TangentHandleBrush.setColor(QColor(100, 100, 255));
  m_TangentHandleBrush.setStyle(Qt::BrushStyle::SolidPattern);
}

void ezQtCurveEditWidget::SetCurves(const ezCurve1DAssetData* pCurveEditData)
{
  m_Curves.Clear();
  m_Curves.Reserve(pCurveEditData->m_Curves.GetCount());

  for (ezUInt32 i = 0; i < pCurveEditData->m_Curves.GetCount(); ++i)
  {
    auto& data = m_Curves.ExpandAndGetRef();

    pCurveEditData->ConvertToRuntimeData(i, data);
  }

  // make sure the selection does not contain points that got deleted
  for (ezUInt32 i = 0; i < m_SelectedCPs.GetCount(); )
  {
    if (m_SelectedCPs[i].m_uiCurve >= m_Curves.GetCount() ||
      m_SelectedCPs[i].m_uiPoint >= m_Curves[m_SelectedCPs[i].m_uiCurve].GetNumControlPoints())
    {
      m_SelectedCPs.RemoveAt(i);
    }
    else
    {
      ++i;
    }
  }

  m_CurvesSorted = m_Curves;
  m_CurveExtents.SetCount(m_Curves.GetCount());

  for (ezUInt32 i = 0; i < m_CurvesSorted.GetCount(); ++i)
  {
    ezCurve1D& curve = m_CurvesSorted[i];

    curve.SortControlPoints();
    curve.CreateLinearApproximation();

    curve.QueryExtents(m_CurveExtents[i].x, m_CurveExtents[i].y);
  }

  ComputeSelectionRect();

  update();
}

QPoint ezQtCurveEditWidget::MapFromScene(const QPointF& pos) const
{
  double x = pos.x() - m_SceneTranslation.x();
  double y = pos.y() - m_SceneTranslation.y();
  x *= m_SceneToPixelScale.x();
  y *= m_SceneToPixelScale.y();

  return QPoint((int)x, (int)y);
}

QPointF ezQtCurveEditWidget::MapToScene(const QPoint& pos) const
{
  double x = pos.x();
  double y = pos.y();
  x /= m_SceneToPixelScale.x();
  y /= m_SceneToPixelScale.y();

  return QPointF(x, y) + m_SceneTranslation;
}


void ezQtCurveEditWidget::ClearSelection()
{
  m_selectionBRect = QRectF();

  if (!m_SelectedCPs.IsEmpty())
  {
    m_SelectedCPs.Clear();
    update();
  }
}

bool ezQtCurveEditWidget::IsSelected(const ezSelectedCurveCP& cp) const
{
  for (const auto& other : m_SelectedCPs)
  {
    if (other.m_uiCurve == cp.m_uiCurve && other.m_uiPoint == cp.m_uiPoint)
      return true;
  }

  return false;
}

void ezQtCurveEditWidget::SetSelection(const ezSelectedCurveCP& cp)
{
  m_SelectedCPs.Clear();
  m_SelectedCPs.PushBack(cp);

  ComputeSelectionRect();
}

void ezQtCurveEditWidget::ToggleSelected(const ezSelectedCurveCP& cp)
{
  SetSelected(cp, !IsSelected(cp));

  ComputeSelectionRect();
}

void ezQtCurveEditWidget::SetSelected(const ezSelectedCurveCP& cp, bool set)
{
  if (!set)
  {
    for (ezUInt32 i = 0; i < m_SelectedCPs.GetCount(); ++i)
    {
      if (m_SelectedCPs[i].m_uiCurve == cp.m_uiCurve && m_SelectedCPs[i].m_uiPoint == cp.m_uiPoint)
      {
        m_SelectedCPs.RemoveAt(i);
        return;
      }
    }
  }
  else
  {
    if (!IsSelected(cp))
    {
      m_SelectedCPs.PushBack(cp);
    }
  }

  ComputeSelectionRect();
}

bool ezQtCurveEditWidget::GetSelectedTangent(ezInt32& out_iCurve, ezInt32& out_iPoint, bool& out_bLeftTangent) const
{
  out_iCurve = m_iSelectedTangentCurve;
  out_iPoint = m_iSelectedTangentPoint;
  out_bLeftTangent = m_bSelectedTangentLeft;
  return (out_iCurve >= 0);
}

QRectF ezQtCurveEditWidget::ComputeViewportSceneRect() const
{
  const QPointF topLeft = MapToScene(rect().topLeft());
  const QPointF bottomRight = MapToScene(rect().bottomRight());

  return QRectF(topLeft, bottomRight);
}

void ezQtCurveEditWidget::paintEvent(QPaintEvent* e)
{
  QPainter painter(this);
  painter.fillRect(rect(), palette().base());
  painter.translate(0.5, 0.5);

  painter.setRenderHint(QPainter::Antialiasing, true);

  const QRectF viewportSceneRect = ComputeViewportSceneRect();

  double fFineGridDensity = 0.01;
  double fRoughGridDensity = 0.01;
  AdjustGridDensity(fFineGridDensity, fRoughGridDensity, rect().width(), viewportSceneRect.width(), 20);

  RenderVerticalGrid(&painter, viewportSceneRect, fRoughGridDensity);

  if (m_pGridBar)
  {
    m_pGridBar->SetConfig(viewportSceneRect, fRoughGridDensity, fFineGridDensity, [this](const QPointF& pt) -> QPoint
    {
      return MapFromScene(pt);
    });
  }

  RenderSideLinesAndText(&painter, viewportSceneRect);

  PaintCurveSegments(&painter);
  PaintSelectedTangentLines(&painter);
  PaintControlPoints(&painter);
  PaintSelectedTangentHandles(&painter);
  PaintSelectedControlPoints(&painter);
  PaintMultiSelectionSquare(&painter);
}

void ezQtCurveEditWidget::mousePressEvent(QMouseEvent* e)
{
  QWidget::mousePressEvent(e);
  m_LastMousePos = e->pos();

  if (m_State != EditState::None)
    return;

  if (e->button() == Qt::RightButton)
  {
    m_State = EditState::RightClick;
    return;
  }

  if (e->buttons() == Qt::LeftButton) // nothing else pressed
  {
    const ClickTarget clickedOn = DetectClickTarget(e->pos());

    if (clickedOn == ClickTarget::Nothing || clickedOn == ClickTarget::SelectedPoint)
    {
      if (e->modifiers() == Qt::NoModifier)
      {
        m_scaleStartPoint = MapToScene(e->pos());

        switch (WhereIsPoint(e->pos()))
        {
        case ezQtCurveEditWidget::SelectArea::Center:
          m_State = EditState::DraggingPoints;
          break;
        case ezQtCurveEditWidget::SelectArea::Top:
          m_scaleReferencePoint = m_selectionBRect.topLeft();
          m_State = EditState::ScaleUpDown;
          break;
        case ezQtCurveEditWidget::SelectArea::Bottom:
          m_scaleReferencePoint = m_selectionBRect.bottomRight();
          m_State = EditState::ScaleUpDown;
          break;
        case ezQtCurveEditWidget::SelectArea::Left:
          m_State = EditState::ScaleLeftRight;
          m_scaleReferencePoint = m_selectionBRect.topRight();
          break;
        case ezQtCurveEditWidget::SelectArea::Right:
          m_State = EditState::ScaleLeftRight;
          m_scaleReferencePoint = m_selectionBRect.topLeft();
          break;
        }
      }

      if (m_State == EditState::None)
      {
        m_State = EditState::MultiSelect;

        ezSelectedCurveCP cp;
        if (PickCpAt(e->pos(), 8, cp))
        {
          if (e->modifiers().testFlag(Qt::ControlModifier))
          {
            ToggleSelected(cp);
          }
          else if (e->modifiers().testFlag(Qt::ShiftModifier))
          {
            SetSelected(cp, true);
          }
          else if (e->modifiers().testFlag(Qt::AltModifier))
          {
            SetSelected(cp, false);
          }
          else
          {
            if (clickedOn == ClickTarget::Nothing)
              SetSelection(cp);

            m_State = EditState::DraggingPoints;
          }
        }
      }

      EZ_ASSERT_DEBUG(!m_bBegunChanges, "Invalid State");

      if (m_State == EditState::DraggingPoints)
      {
        emit BeginOperationEvent("Drag Points");
        m_bBegunChanges = true;
      }
      else if (m_State == EditState::ScaleLeftRight)
      {
        emit BeginOperationEvent("Scale Points Left / Right");
        m_bBegunChanges = true;
      }
      else if (m_State == EditState::ScaleUpDown)
      {
        emit BeginOperationEvent("Scale Points Up / Down");
        m_bBegunChanges = true;
      }

      update();
    }
    else if (clickedOn == ClickTarget::TangentHandle)
    {
      m_State = EditState::DraggingTangents;
      emit BeginOperationEvent("Drag Tangents");
      EZ_ASSERT_DEBUG(!m_bBegunChanges, "Invalid State");
      m_bBegunChanges = true;
    }
  }

  if (m_State == EditState::MultiSelect && m_pRubberband == nullptr)
  {
    m_multiSelectionStart = e->pos();
    m_multiSelectRect = QRect();
    m_pRubberband = new QRubberBand(QRubberBand::Shape::Rectangle, this);
    m_pRubberband->setGeometry(QRect(m_multiSelectionStart, QSize()));
    m_pRubberband->hide();
  }
}

void ezQtCurveEditWidget::mouseReleaseEvent(QMouseEvent* e)
{
  QWidget::mouseReleaseEvent(e);

  if (e->button() == Qt::RightButton)
  {
    if (m_State == EditState::Panning)
      m_State = EditState::None;

    if (m_State == EditState::RightClick)
    {
      m_State = EditState::None;

      ContextMenuEvent(mapToGlobal(e->pos()), MapToScene(e->pos()));
    }
  }

  if (e->button() == Qt::LeftButton &&
    (m_State == EditState::DraggingPoints ||
      m_State == EditState::DraggingTangents ||
      m_State == EditState::MultiSelect))
  {
    m_State = EditState::None;
    m_iSelectedTangentCurve = -1;
    m_iSelectedTangentPoint = -1;

    if (m_bBegunChanges)
    {
      m_bBegunChanges = false;
      emit EndOperationEvent(true);
    }

    update();
  }

  if (m_State != EditState::MultiSelect && m_pRubberband)
  {
    delete m_pRubberband;
    m_pRubberband = nullptr;

    if (!m_multiSelectRect.isEmpty())
    {
      ezDynamicArray<ezSelectedCurveCP> change;
      ExecMultiSelection(change);
      m_multiSelectRect = QRect();

      if (e->modifiers().testFlag(Qt::AltModifier))
      {
        CombineSelection(m_SelectedCPs, change, false);
      }
      else if (e->modifiers().testFlag(Qt::ShiftModifier) || e->modifiers().testFlag(Qt::ControlModifier))
      {
        CombineSelection(m_SelectedCPs, change, true);
      }
      else
      {
        m_SelectedCPs = change;
      }

      ComputeSelectionRect();
      update();
    }
  }

  if (e->buttons() == Qt::NoButton)
  {
    unsetCursor();

    m_State = EditState::None;
    m_iSelectedTangentCurve = -1;
    m_iSelectedTangentPoint = -1;

    if (m_bBegunChanges)
    {
      m_bBegunChanges = false;
      emit EndOperationEvent(true);
    }

    update();
  }
}

void ezQtCurveEditWidget::mouseMoveEvent(QMouseEvent* e)
{
  QWidget::mouseMoveEvent(e);
  Qt::CursorShape cursor = Qt::ArrowCursor;

  const QPoint diff = e->pos() - m_LastMousePos;
  const double moveX = (double)diff.x() / m_SceneToPixelScale.x();
  const double moveY = (double)diff.y() / m_SceneToPixelScale.y();

  if (m_State == EditState::RightClick || m_State == EditState::Panning)
  {
    m_State = EditState::Panning;
    cursor = Qt::ClosedHandCursor;

    m_SceneTranslation.setX(m_SceneTranslation.x() - moveX);
    m_SceneTranslation.setY(m_SceneTranslation.y() - moveY);

    update();
  }

  if (m_State == EditState::DraggingPoints)
  {
    MoveControlPointsEvent(moveX, moveY);
    //cursor = Qt::SizeAllCursor;
  }

  if (m_State == EditState::DraggingTangents)
  {
    MoveTangentsEvent(moveX, moveY);
  }

  if (m_State == EditState::MultiSelect && m_pRubberband)
  {
    m_multiSelectRect = QRect(m_multiSelectionStart, e->pos()).normalized();
    m_pRubberband->setGeometry(m_multiSelectRect);
    m_pRubberband->show();
  }

  if (m_State == EditState::None && !m_selectionBRect.isEmpty())
  {
    switch (WhereIsPoint(e->pos()))
    {
    case ezQtCurveEditWidget::SelectArea::Center:
      //cursor = Qt::SizeAllCursor;
      break;
    case ezQtCurveEditWidget::SelectArea::Top:
    case ezQtCurveEditWidget::SelectArea::Bottom:
      cursor = Qt::SizeVerCursor;
      break;
    case ezQtCurveEditWidget::SelectArea::Left:
    case ezQtCurveEditWidget::SelectArea::Right:
      cursor = Qt::SizeHorCursor;
      break;
    }
  }

  if (m_State == EditState::ScaleLeftRight)
  {
    cursor = Qt::SizeHorCursor;

    const QPointF wsPos = MapToScene(e->pos());
    const QPointF norm = m_scaleReferencePoint - m_scaleStartPoint;
    const QPointF wsDiff = m_scaleReferencePoint - wsPos;

    ScaleControlPointsEvent(m_scaleReferencePoint, wsDiff.x() / norm.x(), 1);
  }

  if (m_State == EditState::ScaleUpDown)
  {
    cursor = Qt::SizeVerCursor;

    const QPointF wsPos = MapToScene(e->pos());
    const QPointF norm = m_scaleReferencePoint - m_scaleStartPoint;
    const QPointF wsDiff = m_scaleReferencePoint - wsPos;

    ScaleControlPointsEvent(m_scaleReferencePoint, 1, wsDiff.y() / norm.y());
  }

  setCursor(cursor);
  m_LastMousePos = e->pos();
}

void ezQtCurveEditWidget::mouseDoubleClickEvent(QMouseEvent* e)
{
  QWidget::mouseDoubleClickEvent(e);

  if (e->button() == Qt::LeftButton)
  {
    ezSelectedCurveCP cp;
    if (PickCpAt(e->pos(), 15, cp))
    {
      SetSelection(cp);
    }
    else
    {
      const QPointF epsilon = MapToScene(QPoint(15, 15)) - MapToScene(QPoint(0, 0));
      const QPointF scenePos = MapToScene(e->pos());

      emit DoubleClickEvent(scenePos, epsilon);
    }
  }
}

void ezQtCurveEditWidget::wheelEvent(QWheelEvent* e)
{
  const QPointF ptAt = MapToScene(mapFromGlobal(e->globalPos()));
  QPointF posDiff = m_SceneTranslation - ptAt;

  if (e->delta() > 0)
  {
    m_SceneToPixelScale.setX(m_SceneToPixelScale.x() * 1.2);
    m_SceneToPixelScale.setY(m_SceneToPixelScale.y() * 1.2);
    posDiff.setX(posDiff.x() * (1.0 / 1.2));
    posDiff.setY(posDiff.y() * (1.0 / 1.2));
  }
  else
  {
    m_SceneToPixelScale.setX(m_SceneToPixelScale.x() * (1.0 / 1.2));
    m_SceneToPixelScale.setY(m_SceneToPixelScale.y() * (1.0 / 1.2));
    posDiff.setX(posDiff.x() * 1.2);
    posDiff.setY(posDiff.y() * 1.2);
  }

  m_SceneTranslation = ptAt + posDiff;

  update();
}


void ezQtCurveEditWidget::keyPressEvent(QKeyEvent* e)
{
  QWidget::keyPressEvent(e);

  if (e->key() == Qt::Key_Escape)
  {
    ClearSelection();
  }

  if (e->key() == Qt::Key_Delete)
  {
    emit DeleteControlPointsEvent();
  }
}

void ezQtCurveEditWidget::PaintCurveSegments(QPainter* painter) const
{
  painter->save();
  painter->setBrush(Qt::NoBrush);

  QPen pen;
  pen.setCosmetic(true);
  pen.setStyle(Qt::PenStyle::SolidLine);

  for (ezUInt32 curveIdx = 0; curveIdx < m_CurvesSorted.GetCount(); ++curveIdx)
  {
    const ezCurve1D& curve = m_CurvesSorted[curveIdx];
    const ezColorGammaUB curveColor = curve.GetCurveColor();

    pen.setColor(QColor(curveColor.r, curveColor.g, curveColor.b));
    painter->setPen(pen);

    QPainterPath path;

    const ezUInt32 numCps = curve.GetNumControlPoints();
    for (ezUInt32 cpIdx = 1; cpIdx < numCps; ++cpIdx)
    {
      const ezCurve1D::ControlPoint& cpPrev = curve.GetControlPoint(cpIdx - 1);
      const ezCurve1D::ControlPoint& cpThis = curve.GetControlPoint(cpIdx);

      const QPointF startPt = QPointF(cpPrev.m_Position.x, cpPrev.m_Position.y);
      const QPointF endPt = QPointF(cpThis.m_Position.x, cpThis.m_Position.y);
      const QPointF tangent1 = QPointF(cpPrev.m_RightTangent.x, cpPrev.m_RightTangent.y);
      const QPointF tangent2 = QPointF(cpThis.m_LeftTangent.x, cpThis.m_LeftTangent.y);
      const QPointF ctrlPt1 = startPt + tangent1;
      const QPointF ctrlPt2 = endPt + tangent2;

      path.moveTo(MapFromScene(startPt));
      path.cubicTo(MapFromScene(ctrlPt1), MapFromScene(ctrlPt2), MapFromScene(endPt));
    }

    painter->drawPath(path);
  }

  painter->restore();
}

void ezQtCurveEditWidget::PaintControlPoints(QPainter* painter) const
{
  painter->save();
  painter->setBrush(m_ControlPointBrush);
  painter->setPen(Qt::NoPen);

  for (ezUInt32 curveIdx = 0; curveIdx < m_Curves.GetCount(); ++curveIdx)
  {
    const ezCurve1D& curve = m_Curves[curveIdx];

    const ezUInt32 numCps = curve.GetNumControlPoints();
    for (ezUInt32 cpIdx = 0; cpIdx < numCps; ++cpIdx)
    {
      const ezCurve1D::ControlPoint& cp = curve.GetControlPoint(cpIdx);

      const QPointF ptPos = MapFromScene(QPointF(cp.m_Position.x, cp.m_Position.y));

      painter->drawEllipse(ptPos, 3.5, 3.5);
    }
  }

  painter->restore();
}

void ezQtCurveEditWidget::PaintSelectedControlPoints(QPainter* painter) const
{
  painter->save();
  painter->setBrush(m_SelectedControlPointBrush);
  painter->setPen(Qt::NoPen);

  for (const auto& cpSel : m_SelectedCPs)
  {
    const ezCurve1D& curve = m_Curves[cpSel.m_uiCurve];
    const ezCurve1D::ControlPoint& cp = curve.GetControlPoint(cpSel.m_uiPoint);

    const QPointF ptPos = MapFromScene(QPointF(cp.m_Position.x, cp.m_Position.y));

    painter->drawEllipse(ptPos, 4.5, 4.5);
  }

  painter->restore();
}

void ezQtCurveEditWidget::PaintSelectedTangentLines(QPainter* painter) const
{
  painter->save();
  painter->setBrush(Qt::NoBrush);
  painter->setPen(m_TangentLinePen);

  ezHybridArray<QLine, 50> lines;

  for (const auto& cpSel : m_SelectedCPs)
  {
    const ezCurve1D& curve = m_Curves[cpSel.m_uiCurve];
    const ezCurve1D::ControlPoint& cp = curve.GetControlPoint(cpSel.m_uiPoint);

    ezVec2 leftHandlePos = cp.m_Position + cp.m_LeftTangent;
    ezVec2 rightHandlePos = cp.m_Position + cp.m_RightTangent;

    const QPoint ptPos = MapFromScene(QPointF(cp.m_Position.x, cp.m_Position.y));
    const QPoint ptPosLeft = MapFromScene(QPointF(leftHandlePos.x, leftHandlePos.y));
    const QPoint ptPosRight = MapFromScene(QPointF(rightHandlePos.x, rightHandlePos.y));

    if (m_CurveExtents[cpSel.m_uiCurve].x != cp.m_Position.x)
    {
      QLine& l1 = lines.ExpandAndGetRef();
      l1.setLine(ptPos.x(), ptPos.y(), ptPosLeft.x(), ptPosLeft.y());
    }

    if (m_CurveExtents[cpSel.m_uiCurve].y != cp.m_Position.x)
    {
      QLine& l2 = lines.ExpandAndGetRef();
      l2.setLine(ptPos.x(), ptPos.y(), ptPosRight.x(), ptPosRight.y());
    }
  }

  painter->drawLines(lines.GetData(), lines.GetCount());

  painter->restore();
}

void ezQtCurveEditWidget::PaintSelectedTangentHandles(QPainter* painter) const
{
  painter->save();
  painter->setBrush(m_TangentHandleBrush);
  painter->setPen(Qt::NoPen);

  for (const auto& cpSel : m_SelectedCPs)
  {
    const ezCurve1D& curve = m_Curves[cpSel.m_uiCurve];
    const ezCurve1D::ControlPoint& cp = curve.GetControlPoint(cpSel.m_uiPoint);

    ezVec2 leftHandlePos = cp.m_Position + cp.m_LeftTangent;
    ezVec2 rightHandlePos = cp.m_Position + cp.m_RightTangent;

    const QPointF ptPosLeft = MapFromScene(QPointF(leftHandlePos.x, leftHandlePos.y));
    const QPointF ptPosRight = MapFromScene(QPointF(rightHandlePos.x, rightHandlePos.y));

    if (m_CurveExtents[cpSel.m_uiCurve].x != cp.m_Position.x)
      painter->drawRect(QRectF(ptPosLeft.x() - 4.5, ptPosLeft.y() - 4.5, 9, 9));

    if (m_CurveExtents[cpSel.m_uiCurve].y != cp.m_Position.x)
      painter->drawRect(QRectF(ptPosRight.x() - 4.5, ptPosRight.y() - 4.5, 9, 9));
  }

  painter->restore();
}

void ezQtCurveEditWidget::PaintMultiSelectionSquare(QPainter* painter) const
{
  if (m_selectionBRect.isEmpty())
    return;

  painter->save();
  painter->setPen(Qt::NoPen);

  QColor col = palette().highlight().color();
  col.setAlpha(100);
  painter->setBrush(col);

  const QPoint tl = MapFromScene(m_selectionBRect.topLeft());
  const QPoint br = MapFromScene(m_selectionBRect.bottomRight());
  QRectF r = QRect(tl, br);
  r.adjust(-4.5, +4.5, +3.5, -5.5);

  painter->drawRect(r);

  col.setAlpha(255);
  QPen pen(col);
  pen.setStyle(Qt::PenStyle::SolidLine);
  pen.setCosmetic(true);
  pen.setWidth(1);
  pen.setCapStyle(Qt::PenCapStyle::SquareCap);
  painter->setPen(pen);

  painter->drawLine(tl.x() - 10, tl.y(), tl.x() - 10, br.y());
  painter->drawLine(br.x() + 10, tl.y(), br.x() + 10, br.y());
  painter->drawLine(tl.x(), br.y() - 10, br.x(), br.y() - 10);
  painter->drawLine(tl.x(), tl.y() + 10, br.x(), tl.y() + 10);

  painter->restore();
}

void ezQtCurveEditWidget::RenderVerticalGrid(QPainter* painter, const QRectF& viewportSceneRect, double fRoughGridDensity)
{
  double lowX, highX;
  ComputeGridExtentsX2(viewportSceneRect, fRoughGridDensity, lowX, highX);

  const int iy = rect().bottom();

  // render grid lines
  {
    QPen pen(palette().light(), 1.0f);
    pen.setCosmetic(true);
    painter->setPen(pen);

    ezHybridArray<QLine, 100> lines;

    for (double x = lowX; x <= highX; x += fRoughGridDensity)
    {
      const int ix = MapFromScene(QPointF(x, x)).x();

      QLine& l = lines.ExpandAndGetRef();
      l.setLine(ix, 0, ix, iy);
    }

    painter->drawLines(lines.GetData(), lines.GetCount());
  }
}

void ezQtCurveEditWidget::RenderSideLinesAndText(QPainter* painter, const QRectF& viewportSceneRect)
{
  double fFineGridDensity = 0.01;
  double fRoughGridDensity = 0.01;
  AdjustGridDensity(fFineGridDensity, fRoughGridDensity, rect().width(), viewportSceneRect.width(), 20);

  painter->save();

  const ezInt32 iFineLineLength = 10;
  const ezInt32 iRoughLineLength = 20;

  QRect areaRect = rect();
  areaRect.setRight(areaRect.left() + 20);

  // render fine grid stop lines
  {
    double lowY, highY;
    ComputeGridExtentsY2(viewportSceneRect, fFineGridDensity, lowY, highY);

    if (lowY > highY)
      ezMath::Swap(lowY, highY);

    QPen pen(palette().light(), 1.0f);
    pen.setCosmetic(true);
    painter->setPen(pen);

    ezHybridArray<QLine, 100> lines;

    for (double y = lowY; y <= highY; y += fFineGridDensity)
    {
      const QPoint pos = MapFromScene(QPointF(0, y));

      QLine& l = lines.ExpandAndGetRef();
      l.setLine(0, pos.y(), iFineLineLength, pos.y());
    }

    painter->drawLines(lines.GetData(), lines.GetCount());
  }

  // render rough grid stop lines
  {
    double lowY, highY;
    ComputeGridExtentsY2(viewportSceneRect, fRoughGridDensity, lowY, highY);

    if (lowY > highY)
      ezMath::Swap(lowY, highY);

    QPen pen(palette().light(), 1.0f);
    pen.setCosmetic(true);
    painter->setPen(pen);

    ezHybridArray<QLine, 100> lines;

    for (double y = lowY; y <= highY; y += fRoughGridDensity)
    {
      const QPoint pos = MapFromScene(QPointF(0, y));

      QLine& l = lines.ExpandAndGetRef();
      l.setLine(0, pos.y(), iRoughLineLength, pos.y());
    }

    painter->drawLines(lines.GetData(), lines.GetCount());
  }

  // Grid Stop Value Text
  {
    double lowY, highY;
    ComputeGridExtentsY2(viewportSceneRect, fRoughGridDensity, lowY, highY);

    if (lowY > highY)
      ezMath::Swap(lowY, highY);

    QTextOption textOpt(Qt::AlignCenter);
    QRectF textRect;

    painter->setPen(palette().buttonText().color());

    ezStringBuilder tmp;

    for (double y = lowY; y <= highY; y += fRoughGridDensity)
    {
      const QPoint pos = MapFromScene(QPointF(0, y));

      textRect.setRect(0, pos.y() - 15, areaRect.width(), 15);
      tmp.Format("{0}", ezArgF(y));

      painter->drawText(textRect, tmp.GetData(), textOpt);
    }
  }

  painter->restore();
}

bool ezQtCurveEditWidget::PickCpAt(const QPoint& pos, float fMaxPixelDistance, ezSelectedCurveCP& out_Result) const
{
  const ezVec2 at((float)pos.x(), (float)pos.y());
  float fMaxDistSqr = ezMath::Square(fMaxPixelDistance);

  out_Result.m_uiCurve = 0xFFFF;

  for (ezUInt32 uiCurve = 0; uiCurve < m_Curves.GetCount(); ++uiCurve)
  {
    const ezCurve1D& curve = m_Curves[uiCurve];

    for (ezUInt32 uiCP = 0; uiCP < curve.GetNumControlPoints(); ++uiCP)
    {
      const auto& cp = curve.GetControlPoint(uiCP);

      const QPoint diff = MapFromScene(cp.m_Position) - pos;
      const ezVec2 fDiff(diff.x(), diff.y());

      const float fDistSqr = fDiff.GetLengthSquared();
      if (fDistSqr <= fMaxDistSqr)
      {
        fMaxDistSqr = fDistSqr;
        out_Result.m_uiCurve = uiCurve;
        out_Result.m_uiPoint = uiCP;
      }
    }
  }

  return out_Result.m_uiCurve != 0xFFFF;
}

static inline ezVec2 ToVec(const QPoint& pt)
{
  return ezVec2(pt.x(), pt.y());
}

ezQtCurveEditWidget::ClickTarget ezQtCurveEditWidget::DetectClickTarget(const QPoint& pos)
{
  const ezVec2 vScreenPos(pos.x(), pos.y());
  float fMinDistSQR = ezMath::Square(15);
  ezInt32 iBestCurve = -1;
  ezInt32 iBestCP = -1;
  ezInt32 iBestComp = -1;

  for (ezUInt32 i = 0; i < m_SelectedCPs.GetCount(); ++i)
  {
    const auto& cpSel = m_SelectedCPs[i];
    const auto& cp = m_Curves[cpSel.m_uiCurve].GetControlPoint(cpSel.m_uiPoint);

    const ezVec2 ptPos = ToVec(MapFromScene(cp.m_Position));
    const ezVec2 ptLeft = ToVec(MapFromScene(cp.m_Position + cp.m_LeftTangent));
    const ezVec2 ptRight = ToVec(MapFromScene(cp.m_Position + cp.m_RightTangent));

    {
      const float distSQR = (ptPos - vScreenPos).GetLengthSquared();
      if (distSQR < fMinDistSQR)
      {
        fMinDistSQR = distSQR;
        iBestCurve = cpSel.m_uiCurve;
        iBestCP = cpSel.m_uiPoint;
        iBestComp = 0;
      }
    }
    {
      const float distSQR = (ptLeft - vScreenPos).GetLengthSquared();
      if (distSQR < fMinDistSQR)
      {
        fMinDistSQR = distSQR;
        iBestCurve = cpSel.m_uiCurve;
        iBestCP = cpSel.m_uiPoint;
        iBestComp = 1;
      }
    }
    {
      const float distSQR = (ptRight - vScreenPos).GetLengthSquared();
      if (distSQR < fMinDistSQR)
      {
        fMinDistSQR = distSQR;
        iBestCurve = cpSel.m_uiCurve;
        iBestCP = cpSel.m_uiPoint;
        iBestComp = 2;
      }
    }
  }

  m_iSelectedTangentCurve = -1;
  m_iSelectedTangentPoint = -1;
  m_bSelectedTangentLeft = false;

  if (iBestComp > 0)
  {
    m_iSelectedTangentCurve = iBestCurve;
    m_iSelectedTangentPoint = iBestCP;
    m_bSelectedTangentLeft = (iBestComp == 1);

    return ClickTarget::TangentHandle;
  }

  if (iBestComp == 0)
    return ClickTarget::SelectedPoint;

  return ClickTarget::Nothing;
}

void ezQtCurveEditWidget::ExecMultiSelection(ezDynamicArray<ezSelectedCurveCP>& out_Selection)
{
  out_Selection.Clear();

  for (ezUInt32 uiCurve = 0; uiCurve < m_Curves.GetCount(); ++uiCurve)
  {
    const ezCurve1D& curve = m_Curves[uiCurve];

    for (ezUInt32 uiCP = 0; uiCP < curve.GetNumControlPoints(); ++uiCP)
    {
      const auto& cp = curve.GetControlPoint(uiCP);

      const QPoint cpPos = MapFromScene(cp.m_Position);

      if (m_multiSelectRect.contains(cpPos))
      {
        auto& sel = out_Selection.ExpandAndGetRef();
        sel.m_uiCurve = uiCurve;
        sel.m_uiPoint = uiCP;
      }
    }
  }
}

bool ezQtCurveEditWidget::CombineSelection(ezDynamicArray<ezSelectedCurveCP>& inout_Selection, const ezDynamicArray<ezSelectedCurveCP>& change, bool add)
{
  bool bChange = false;

  for (ezUInt32 i = 0; i < change.GetCount(); ++i)
  {
    const auto& cp = change[i];

    if (!add)
    {
      bChange |= inout_Selection.Remove(cp);
    }
    else if (!inout_Selection.Contains(cp))
    {
      inout_Selection.PushBack(cp);
      bChange = true;
    }
  }

  return bChange;
}

void ezQtCurveEditWidget::ComputeSelectionRect()
{
  m_selectionBRect = QRectF();

  if (m_SelectedCPs.GetCount() < 2)
    return;

  ezBoundingBox bbox;
  bbox.SetInvalid();

  for (const auto& cpSel : m_SelectedCPs)
  {
    const ezCurve1D& curve = m_Curves[cpSel.m_uiCurve];
    const ezCurve1D::ControlPoint& cp = curve.GetControlPoint(cpSel.m_uiPoint);

    bbox.ExpandToInclude(ezVec3(cp.m_Position.x, cp.m_Position.y, cp.m_Position.x));
  }

  if (bbox.IsValid())
  {
    m_selectionBRect.setCoords(bbox.m_vMin.x, bbox.m_vMin.y, bbox.m_vMax.x, bbox.m_vMax.y);
    m_selectionBRect.normalized();
  }
}

ezQtCurveEditWidget::SelectArea ezQtCurveEditWidget::WhereIsPoint(QPoint pos) const
{
  if (m_selectionBRect.isEmpty())
    return SelectArea::None;

  const QPoint tl = MapFromScene(m_selectionBRect.topLeft());
  const QPoint br = MapFromScene(m_selectionBRect.bottomRight());
  QRect selectionRectSS = QRect(tl, br);
  selectionRectSS.adjust(-4.5, +4.5, +3.5, -5.5);

  const QRect barTop(selectionRectSS.left(), selectionRectSS.bottom() - 10, selectionRectSS.width(), 10);
  const QRect barBottom(selectionRectSS.left(), selectionRectSS.top(), selectionRectSS.width(), 10);
  const QRect barLeft(selectionRectSS.left() - 10, selectionRectSS.top(), 10, selectionRectSS.height());
  const QRect barRight(selectionRectSS.right(), selectionRectSS.top(), 10, selectionRectSS.height());

  if (barTop.contains(pos))
    return SelectArea::Top;

  if (barBottom.contains(pos))
    return SelectArea::Bottom;

  if (barLeft.contains(pos))
    return SelectArea::Left;

  if (barRight.contains(pos))
    return SelectArea::Right;

  if (selectionRectSS.contains(pos))
    return SelectArea::Center;

  return SelectArea::None;
}


