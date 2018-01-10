/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2017 met.no

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

#include "bdiana_vprof.h"

#include <puTools/miStringFunctions.h>

#include "diVprofOptions.h"
#include "diVprofPaintable.h"
#include "export/PaintableImageSource.h"

#define MILOGGER_CATEGORY "diana.bdiana"
#include <miLogger/miLogging.h>

BdianaVprof::BdianaVprof()
{
}

BdianaVprof::~BdianaVprof()
{
}

void BdianaVprof::MAKE_VPROF()
{
  if (!manager) {
    manager.reset(new VprofManager());
    manager->init();
  }
}

void BdianaVprof::set_options(const std::vector<std::string>& opts)
{
  int n = opts.size();
  for (int i = 0; i < n; i++) {
    std::string line = opts[i];
    miutil::trim(line);
    if (line.empty())
      continue;
    std::string upline = miutil::to_upper(line);

    if (upline == "OBSERVATION.ON")
      vprof_plotobs = true;
    else if (upline == "OBSERVATION.OFF")
      vprof_plotobs = false;
    else if (miutil::contains(upline, "MODELS=") || miutil::contains(upline, "MODEL=") || miutil::contains(upline, "STATION=")) {
      std::vector<std::string> vs = miutil::split(line, "=");
      if (vs.size() > 1) {
        std::string key = miutil::to_upper(vs[0]);
        std::string value = vs[1];
        if (key == "STATION") {
          if (miutil::contains(value, "\""))
            miutil::remove(value, '\"');
          stations = miutil::split(value, ",");
        } else if (key == "MODELS" || key == "MODEL") {
          vprof_models = miutil::split(value, 0, ",");
        }
      }
    } else {
      // assume plot-options
      vprof_options.push_back(line);
      vprof_optionschanged = true;
    }
  }
}

void BdianaVprof::set_vprof(const std::vector<std::string>& pcom)
{
  // extract options for plot
  set_options(pcom);

  if (vprof_optionschanged)
    manager->getOptions()->readOptions(vprof_options);

  vprof_optionschanged = false;
  manager->setSelectedModels(vprof_models);
  manager->setModel();
}

ImageSource* BdianaVprof::imageSource()
{
  if (!imageSource_) {
    paintable_.reset(new VprofPaintable(manager.get()));
    imageSource_.reset(new PaintableImageSource(paintable_.get()));
  }
  return imageSource_.get();
}

miutil::miTime BdianaVprof::getTime()
{
  return manager->getTime();
}

void BdianaVprof::setTime(const miutil::miTime& time)
{
  manager->setTime(time);
}