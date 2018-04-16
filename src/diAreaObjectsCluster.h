/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2017-2018 met.no

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

#ifndef AREAOBJECTSCLUSTER_H
#define AREAOBJECTSCLUSTER_H

#include "diAreaObjects.h"
#include "diPlot.h"

class PlotModule;
class PlotElement;
class StaticPlot;

class AreaObjectsCluster
{
public:
  AreaObjectsCluster(PlotModule* pm);
  ~AreaObjectsCluster();

  void plot(DiGLPainter* gl, Plot::PlotOrder zorder);

  void addPlotElements(std::vector<PlotElement>& pel);

  bool enablePlotElement(const PlotElement& pe);

  ///put area into list of area objects
  void makeAreaObjects(std::string name, std::string areastring, int id);

  ///send command to right area object
  void areaObjectsCommand(const std::string& command, const std::string& dataSet,
      const std::vector<std::string>& data, int id);

private:
  PlotModule* plot_;

  typedef std::vector<AreaObjects> areaobjects_v;
  areaobjects_v vareaobjects;  //QED areas
};

#endif // AREAOBJECTSCLUSTER_H
