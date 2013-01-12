#ifndef PAINT_UTILS_H
#define PAINT_UTILS_H

#include <QColor>
#include <QList>
#include <QRect>

class QPainter;

class TextDrawer
{
public:
  TextDrawer();
  TextDrawer(QPainter& painter, const QString& text, QPoint pos);
  TextDrawer(const TextDrawer &other);
  ~TextDrawer();
  TextDrawer& operator=(const TextDrawer &other);
  void drawText();  // The text will be drawn exactly once

private:
  QPainter* painter_;
  QString text_;
  QPoint pos_;
  mutable bool workDone_;  // TODO: Fix the dirty trick (with move constructor)
};

TextDrawer drawTextWithBackground(QPainter& painter, const QString& text, QPoint pos);

void drawFramed(QPainter& painter, const QList<QRect>& objects, int frameThickness,
                QColor objectsColor, QColor frameColor);

#endif // PAINT_UTILS_H
