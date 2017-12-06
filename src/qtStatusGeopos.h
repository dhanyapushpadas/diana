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
#ifndef _qtStatusGeopos_h
#define _qtStatusGeopos_h

#include <qwidget.h>

class Controller;
class QComboBox;
class QLabel;

/**
 \brief Geoposition in status bar

 Widget in status bar for displaying the geographical position of the mouse pointer
 */
class StatusGeopos: public QWidget {
  Q_OBJECT

public:
  StatusGeopos(Controller* controller, QWidget* parent = 0);
  //! handle mouse position update
  void handleMousePos(int x, int y);

private:
  bool geographicMode();
  bool gridMode();
  bool areaMode();
  bool distMode();
  void setPosition(float, float);
  void undefPosition();

private:
  Controller* controller;
  QLabel* sylabel;
  QLabel* sxlabel;
  QComboBox* xybox;
  QLabel *latlabel;
  QLabel *lonlabel;
};

#endif
