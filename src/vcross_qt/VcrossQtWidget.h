/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2006 met.no

  Contact information:
  Norwegian Meteorological Institute
  Box 43 Blindern
  0313 OSLO
  NORWAY
  email: diana@met.no

  This file is part of Diana

  Diana is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  Diana is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with Diana; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/
#ifndef VCROSSQTWIDGET_H
#define VCROSSQTWIDGET_H

#include "diColour.h"
#include "VcrossQtManager.h"

//#define VCROSS_GLWIDGET 1
#ifdef VCROSS_GLWIDGET
#include <QtGui/QGLWidget>
#define VCROSS_GL(gl,nogl) gl
#else
#include <QtGui/QWidget>
#define VCROSS_GL(gl,nogl) nogl
#endif

class QKeyEvent;
class QMouseEvent;

namespace vcross {

/**
   \brief The widget for displaying vertical crossections.

   Handles widget paint/redraw events.
   Receives mouse and keybord events and initiates actions.
*/
#ifdef VCROSS_GLWIDGET // cannot use VCROSS_GL here because moc-qt4 does not understand
class QtWidget : public QGLWidget
#else
class QtWidget : public QWidget
#endif
{
  Q_OBJECT;

public:
  QtWidget(QtManager_p vcm, QWidget* parent = 0);
  ~QtWidget();

  void enableTimeGraph(bool on);

  bool saveRasterImage(const std::string& fname, const std::string& format, const int quality = -1);

  /** print using either given QPrinter (if USE_PAINTGL) or using the given printOptions */
  //void print(QPrinter* qprt, const printOptions& priop);

protected:
  virtual void paintEvent(QPaintEvent* event);
  virtual void resizeEvent(QResizeEvent* event);

  virtual void keyPressEvent(QKeyEvent *me);
  virtual void mousePressEvent(QMouseEvent* me);
  virtual void mouseMoveEvent(QMouseEvent* me);
  virtual void mouseReleaseEvent(QMouseEvent* me);

private:
  QtManager_p vcrossm;

  bool dorubberband;   // while zooming
  bool dopanning;

  int arrowKeyDirection;
  int firstx, firsty, mousex, mousey;

  bool timeGraph;
  bool startTimeGraph;

Q_SIGNALS:
  void timeChanged(int);
  void crossectionChanged(int);
};

} // namespace vcross

#endif // VCROSSQTWIDGET_H