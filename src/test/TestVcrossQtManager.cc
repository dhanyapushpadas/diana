/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2015 met.no

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

#include "TestVcrossQtManager.h"
#include <vcross_v2/VcrossQtManager.h>

#define MILOGGER_CATEGORY "diana.test.VcrossQtManager"
#include "miLogger/miLogging.h"

#include <gtest/gtest.h>

//#define DEBUG_MESSAGES
#ifdef DEBUG_MESSAGES
#include <log4cpp/Category.hh>
#define configureLogging()                                              \
  milogger::LoggingConfig lc("kjlhlkjH");                               \
  log4cpp::Category::getRoot().setPriority(log4cpp::Priority::DEBUG)
#else
#define configureLogging() /* empty */
#endif // !DEBUG_MESSAGES

namespace vcross {
namespace test {

ManagerSlots::ManagerSlots(vcross::QtManager* manager)
{
  connect(manager, SIGNAL(fieldChangeBegin(bool)),
      this, SLOT(onFieldChangeBegin(bool)));
  connect(manager, SIGNAL(fieldAdded(int)),
      this, SLOT(onFieldAdded(int)));
  connect(manager, SIGNAL(fieldRemoved(int)),
      this, SLOT(onFieldRemoved(int)));
  connect(manager, SIGNAL(fieldOptionsChanged(int)),
      this, SLOT(onFieldOptionsChanged(int)));
  connect(manager, SIGNAL(fieldVisibilityChanged(int)),
      this, SLOT(onFieldVisibilityChanged(int)));
  connect(manager, SIGNAL(fieldChangeEnd()),
      this, SLOT(onFieldChangeEnd()));

  connect(manager, SIGNAL(crossectionListChanged()),
      this, SLOT(onCrossectionListChanged()));
  connect(manager, SIGNAL(crossectionIndexChanged(int)),
      this, SLOT(onCrossectionIndexChanged(int)));
  connect(manager, SIGNAL(timeListChanged()),
      this, SLOT(onTimeListChanged()));
  connect(manager, SIGNAL(timeIndexChanged(int)),
      this, SLOT(onTimeIndexChanged(int)));

  reset();
}

void ManagerSlots::reset()
{
  beginScript = end = false;

  added.clear();
  removed.clear();
  options.clear();
  visibility.clear();

  cslist = csindex = timelist = timeindex = 0;
}

void ManagerSlots::onFieldChangeBegin(bool fromScript)
{
  beginScript = fromScript;
}

void ManagerSlots::onFieldAdded(int index)
{
  added.push_back(index);
}

void ManagerSlots::onFieldRemoved(int index)
{
  removed.push_back(index);
}

void ManagerSlots::onFieldOptionsChanged(int index)
{
  options.push_back(index);
}

void ManagerSlots::onFieldVisibilityChanged(int index)
{
  visibility.push_back(index);
}

void ManagerSlots::onFieldChangeEnd()
{
  end = true;
}

void ManagerSlots::onCrossectionListChanged()
{
  cslist += 1;
}

void ManagerSlots::onCrossectionIndexChanged(int)
{
  csindex += 1;
}

void ManagerSlots::onTimeListChanged()
{
  timelist += 1;
}

void ManagerSlots::onTimeIndexChanged(int)
{
  timeindex += 1;
}


} // namespace test
} // namespace vcross

namespace {
typedef std::vector<std::string> string_v;

const char AROME1_FILE[] = "arome_vprof.nc";
const vcross::QtManager::vctime_t AROME1_RT("2014-10-20 00:00:00");

const char AROME2_FILE[] = "arome_[yyyymmdd]_[HH]_vc.nc";
const char* AROME2_RTT[4] = {
  "2015-02-01 12:00:00", "2015-02-01 18:00:00",
  "2015-02-02 00:00:00", "2015-02-02 06:00:00"
};

}

TEST(TestVcrossQtManager, Script)
{
  configureLogging();

  string_v sources;
  sources.push_back("m=MODEL f=" TEST_SRCDIR "/"
      + std::string(AROME1_FILE) + " t=netcdf");

  string_v computations;
  computations.push_back("vc_surface_altitude  = convert_unit(altitude,m)");
  computations.push_back("vc_surface_altitude  = height_above_msl_from_surface_geopotential(surface_geopotential)");
  computations.push_back("tk = identity(air_temperature_ml)");
  computations.push_back("tk = tk_from_th(air_potential_temperature_ml)");
  computations.push_back("ff_normal     = normal(x_wind_ml, y_wind_ml)");
  computations.push_back("ff_tangential = tangential(x_wind_ml, y_wind_ml)");

  string_v plots;
  plots.push_back("name=Temp(K) plot=CONTOUR(tk)  colour=red  line.interval=1.");
  plots.push_back("name=Vind    plot=WIND(ff_tangential,ff_normal) colour=blue");

  vcross::QtManager manager;
  manager.parseSetup(sources, computations, plots);

  vcross::test::ManagerSlots ms(&manager);

  const vcross::QtManager::vctime_v reftimes = manager.getModelReferenceTimes("MODEL");
  ASSERT_EQ(1, reftimes.size());

  manager.addField(vcross::QtManager::PlotSpec("MODEL", AROME1_RT, "Temp(K)"),
      "colour=red line.interval=0.2", 0);
  EXPECT_EQ(1, ms.added.size());
  EXPECT_EQ(1, ms.cslist);
  EXPECT_EQ(1, ms.csindex);
  ms.reset();


  string_v select;
  select.push_back("VCROSS model=MODEL field=Vind colour=blue");
  manager.selectFields(select);

  EXPECT_TRUE(ms.beginScript);
  EXPECT_EQ(1, ms.removed.size());
  EXPECT_EQ(1, ms.added.size());
  EXPECT_EQ(0, ms.cslist); // we have the same model with the same set of crossections and times
  EXPECT_EQ(0, ms.csindex);
  EXPECT_TRUE(ms.end);
  ms.reset();

  { int idx = manager.getCrossectionIndex(), n = manager.getCrossectionCount();
    ASSERT_EQ(3, n);
    idx = (idx + 1) % n; // make sure we actually change the index
    manager.setCrossectionIndex(idx);
    EXPECT_EQ(1, ms.csindex);
    ms.reset();
  }

  manager.updateField(0, "colour=green");
  EXPECT_EQ(1, ms.options.size());
  ms.reset();
}

TEST(TestVcrossQtManager, Reftime)
{
  configureLogging();

  string_v sources;
  sources.push_back("m=MODEL f=" TEST_SRCDIR "/"
      + std::string(AROME2_FILE) + " t=netcdf");

  string_v computations;
  computations.push_back("vc_surface_altitude  = convert_unit(altitude,m)");
  computations.push_back("vc_surface_altitude  = height_above_msl_from_surface_geopotential(surface_geopotential)");
  computations.push_back("tk = identity(air_temperature_ml)");
  computations.push_back("tk = tk_from_th(air_potential_temperature_ml)");
  computations.push_back("ff_normal     = normal(x_wind_ml, y_wind_ml)");
  computations.push_back("ff_tangential = tangential(x_wind_ml, y_wind_ml)");

  string_v plots;
  plots.push_back("name=Temp(K) plot=CONTOUR(tk)  colour=red  line.interval=1.");
  plots.push_back("name=Vind    plot=WIND(ff_tangential,ff_normal) colour=blue");

  vcross::QtManager manager;
  manager.parseSetup(sources, computations, plots);

  vcross::test::ManagerSlots ms(&manager);

  const vcross::QtManager::vctime_v reftimes = manager.getModelReferenceTimes("MODEL");
  ASSERT_EQ(4, reftimes.size());

  const vcross::QtManager::vctime_t AROME2_RT1(AROME2_RTT[1]);
  manager.addField(vcross::QtManager::PlotSpec("MODEL", AROME2_RT1, "Temp(K)"),
      "colour=red line.interval=0.2", 0);
  EXPECT_EQ(1, ms.added.size());
  EXPECT_EQ(1, ms.cslist);
  EXPECT_EQ(1, ms.csindex);
  ms.reset();
  EXPECT_EQ(AROME2_RTT[1], manager.getReftimeAt(0).isoTime());


  string_v select;
  select.push_back("VCROSS model=MODEL refhour=12 field=Vind colour=blue");
  manager.selectFields(select);

  EXPECT_TRUE(ms.beginScript);
  EXPECT_EQ(1, ms.removed.size());
  EXPECT_EQ(1, ms.added.size());
  EXPECT_EQ(0, ms.cslist); // we have the same model with the same set of crossections and times
  EXPECT_EQ(0, ms.csindex);
  EXPECT_TRUE(ms.end);
  ms.reset();

  { int idx = manager.getCrossectionIndex(), n = manager.getCrossectionCount();
    ASSERT_EQ(2, n);
    idx = (idx + 1) % n; // make sure we actually change the index
    manager.setCrossectionIndex(idx);
    EXPECT_EQ(1, ms.csindex);
    ms.reset();
  }

  manager.updateField(0, "colour=green");
  EXPECT_EQ(1, ms.options.size());
  ms.reset();

  EXPECT_EQ(AROME2_RTT[0], manager.getReftimeAt(0).isoTime());
}
