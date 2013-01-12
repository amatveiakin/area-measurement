#include <cassert>

#include <QPainter>

#include "paint_utils.h"


TextDrawer::TextDrawer() :
  painter_(0),
  text_(),
  pos_(),
  workDone_(true)
{
}

TextDrawer::TextDrawer(QPainter& painter, const QString& text, QPoint pos) :
  painter_(&painter),
  text_(text),
  pos_(pos),
  workDone_(false)
{
}

TextDrawer::TextDrawer(const TextDrawer& other) :
  painter_(other.painter_),
  text_(other.text_),
  pos_(other.pos_),
  workDone_(other.workDone_)
{
  other.workDone_ = true;
}

TextDrawer::~TextDrawer()
{
  drawText();
}

TextDrawer& TextDrawer::operator=(const TextDrawer &other)
{
  assert(workDone_);
  painter_ = other.painter_;
  text_ = other.text_;
  pos_ = other.pos_;
  workDone_ = other.workDone_;
  return *this;
}

void TextDrawer::drawText()
{
  if (!painter_ || workDone_)
    return;
  painter_->setPen(Qt::black);
  painter_->drawText(pos_, text_);
  workDone_ = true;
}


TextDrawer drawTextWithBackground(QPainter& painter, const QString& text, QPoint pos)
{
  QRect textRect = painter.fontMetrics().boundingRect(text);
  painter.fillRect(textRect.translated(pos), QColor(255, 255, 255, 160));
  painter.setPen(QColor(255, 255, 255, 200));
  painter.drawText(pos + QPoint( 1,  1), text);
  painter.drawText(pos + QPoint( 1, -1), text);
  painter.drawText(pos + QPoint(-1,  1), text);
  painter.drawText(pos + QPoint(-1, -1), text);
  return TextDrawer(painter, text, pos);
}

void drawFramed(QPainter& painter, const QList<QRect>& objects, int frameThickness,
                QColor objectsColor, QColor frameColor)
{
  foreach (const QRect& rect, objects)
    painter.fillRect(rect.adjusted(-frameThickness, -frameThickness, frameThickness, frameThickness), frameColor);
  foreach (const QRect& rect, objects)
    painter.fillRect(rect, objectsColor);
}
