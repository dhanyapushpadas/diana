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

#include "bdiana_main.h"

#include "diMainPaintable.h"
#include "diQuickMenues.h"
#include "export/DianaImageSource.h"

#define MILOGGER_CATEGORY "diana.bdiana"
#include <miLogger/miLogging.h>

BdianaMain::BdianaMain()
    : keeparea(false)
    , use_time(USE_LASTTIME)
{
}

BdianaMain::~BdianaMain()
{
}

bool BdianaMain::MAKE_CONTROLLER()
{
  if (controller)
    return true;

  controller.reset(new Controller);

  if (controller->parseSetup()) {
    return true;
  } else {
    METLIBS_LOG_ERROR("ERROR, an error occured while main_controller parsed setup");
    return false;
  }
}

void BdianaMain::commands(std::vector<std::string>& pcom)
{
  if (updateCommandSyntax(pcom))
    METLIBS_LOG_WARN("The plot commands are outdated, please update!");

  //  if (verbose)
  //    METLIBS_LOG_INFO("- sending plotCommands");
  controller->plotCommands(makeCommands(pcom));
}

ImageSource* BdianaMain::imageSource()
{
  if (!imageSource_) {
    if (!controller)
      MAKE_CONTROLLER();
    paintable_.reset(new MainPaintable(controller.get()));
    imageSource_.reset(new DianaImageSource(paintable_.get()));
  }
  return imageSource_.get();
}

bool BdianaMain::hasCutout()
{
  return static_cast<DianaImageSource*>(imageSource())->isAnnotationsOnly();
}

void BdianaMain::setAnnotationsOnly(bool ao)
{
  static_cast<DianaImageSource*>(imageSource())->setAnnotationsOnly(ao);
}

QRectF BdianaMain::cutout()
{
  return static_cast<DianaImageSource*>(imageSource())->annotationsCutout();
}

typedef std::vector<miutil::miTime> times_t;
typedef std::map<std::string, times_t> type_times_t;

static type_times_t::const_iterator find_times(const type_times_t& times, const std::string& key)
{
  type_times_t::const_iterator it = times.find(key);
  if (it == times.end() || it->second.empty())
    return times.end();

  return it;
}

miutil::miTime BdianaMain::getTime()
{
  if (use_time == USE_REFERENCETIME)
    return controller->getFieldReferenceTime();

  type_times_t times;
  controller->getPlotTimes(times);
  type_times_t::const_iterator it = find_times(times, "fields");
  if (it == times.end())
    it = find_times(times, "satellites");
  if (it == times.end())
    it = find_times(times, "observations");
  if (it == times.end())
    it = find_times(times, "objects");
  if (it == times.end())
    it = find_times(times, "products");
  if (it == times.end())
    return miutil::miTime();

  if (use_time == USE_NOWTIME) {
    const miutil::miTime now = miutil::miTime::nowTime();
    const times_t& fieldtimes = it->second;
    // select closest to now without overstepping
    const int n = fieldtimes.size();
    for (int i = 0; i < n; i++) {
      if (fieldtimes[i] >= now)
        return (i > 0 ? fieldtimes[i - 1] : fieldtimes[i]);
    }
  } else if (use_time == USE_FIRSTTIME) {
    return it->second.front();
  } else {
    return it->second.back();
  }
}

void BdianaMain::setTime(const miutil::miTime& time)
{
  controller->setPlotTime(time);
}
