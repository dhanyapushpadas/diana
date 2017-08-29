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
#ifndef diPlotModule_h
#define diPlotModule_h

#include "diPlot.h"
#include "diPlotCommand.h"
#include "diDrawingTypes.h"
#include "diMapMode.h"
#include "diDisplayObjects.h"

#include <puTools/miTime.h>

#include <vector>
#include <set>
#include <deque>
#include <memory>

class ObsPlot;
class MapPlot;
class AnnotationPlot;
class AreaObjectsCluster;
class FieldManager;
class FieldPlotManager;
class FieldPlotCluster;
struct LocationData;
class LocationPlot;
class Manager;
class ObsManager;
class ObsPlotCluster;
class SatManager;
class StationManager;
class ObjectManager;
class EditManager;
class DrawingManager;
class Field;
class TrajectoryPlot;
class MeasurementsPlot;
class StationPlot;

class QMouseEvent;

/**

 \brief Main plot engine

 The main plot engine, combining various plot layers into one map view
 - reroutes plot info strings to appropriate handlers
 - reroutes mouse and keyboard events
 - all plot layers stored within

 */

class PlotModule {
private:

  ObsManager *obsm;       // observation manager
  SatManager *satm;       // raster-data manager
  StationManager *stam;       // raster-data manager
  ObjectManager *objm;    // met.objects
  EditManager *editm;     // editing/drawing manager
  FieldPlotManager *fieldplotm;   // field plot manager

  std::unique_ptr<ObsPlotCluster> obsplots_;   // observation plots
  std::unique_ptr<FieldPlotCluster> fieldplots_; // field plots
  std::vector<MapPlot*> vmp;   // vector of map plots
  std::vector<TrajectoryPlot*> vtp; // vector of trajectory plots
  std::vector<MeasurementsPlot*> vMeasurementsPlot; // vector of measurements plots
  std::vector<AnnotationPlot*> vap; // vector of annotation plots
  std::vector<AnnotationPlot*> obsVap; //display obs annotation
  std::vector<AnnotationPlot*> objectVap; //display object label
  std::vector<AnnotationPlot*> editVap;   //edit object labels
  std::unique_ptr<AreaObjectsCluster> areaobjects_;

  PlotCommand_cpv annotationCommands;  //orig. strings from setup

  bool mapDefinedByUser; // map area set by user
  bool mapDefinedByData; // for initial maps with fields or sat.
  bool mapDefinedByView; // for initial maps without any data

  Area previousrequestedarea;

  bool showanno;        // show standard annotations

  std::unique_ptr<StaticPlot> staticPlot_;

  DiCanvas* mCanvas;

  std::vector<LocationPlot*> locationPlots; // location (vcross,...) to be plotted

  // event-handling
  float oldx, oldy;
  float newx, newy;
  float startx, starty;
  Area myArea;
  std::deque<Area> areaQ;
  int areaIndex;
  bool areaSaved;
  bool dorubberband;
  bool keepcurrentarea;

  void plotUnder(DiGLPainter* gl);
  void plotOver(DiGLPainter* gl);

  static PlotModule *self;

  /// delete all data vectors
  void cleanup();

  /// handles fields plot info strings
  void prepareFields(const PlotCommand_cpv&);
  /// handles area info strings
  void prepareArea(const PlotCommand_cpv&);
  /// handles map plot info strings
  void prepareMap(const PlotCommand_cpv&);
  /// handles stations plot info strings
  void prepareStations(const PlotCommand_cpv&);
  /// handles trajectory plot info strings
  void prepareTrajectory(const PlotCommand_cpv&);
  /// handles annotation plot info strings
  void prepareAnnotation(const PlotCommand_cpv&);

  /// receive rectangle in pixels
  void setMapAreaFromPhys(const Rectangle& phys);

  double getEntireWindowDistances(const bool horizontal);

  double getArea(const float& flat1, const float& flat2, const float& flat3,
      const float& flat4, const float& flon1, const float& flon2,
      const float& flon3, const float& flon4);
  double calculateArea(double hLSide, double hUSide, double vLSide,
      double vRSide, double diag);

  /// create a Rectangle from staticPlot phys size
  Rectangle getPhysRectangle() const;

  void setMapArea(const Area& area);

  void callManagersChangeProjection();
  void defineMapArea();
  void PlotAreaSetup();

public:
  PlotModule();
  ~PlotModule();

  void setCanvas(DiCanvas* canvas);

  /// the main plot routine (plot for underlay, plot for overlay)
  void plot(DiGLPainter* gl, bool under = true, bool over = true);

  /// split plot info strings and reroute them to appropriate handlers
  void preparePlots(const PlotCommand_cpv&);

  /// get annotations
  const std::vector<AnnotationPlot*>& getAnnotations();

  /// plot annotations
  std::vector<Rectangle> plotAnnotations(DiGLPainter* gl);

  /// get annotations from all plots
  void setAnnotations();

  /// update all plot objects, returning true if successful
  bool updatePlots();

  /// toggle conservative map area
  void keepCurrentArea(bool b)
  {
    keepcurrentarea = b;
  }

  /// get static maparea in plot superclass
  const Area& getMapArea()
    { return staticPlot_->getMapArea(); }

  /// get plotwindow rectangle
  const Rectangle& getPlotSize()
    { return staticPlot_->getPlotSize(); }

  /// get the size of the plot window
  void getPlotWindow(int &width, int &height);
  /// new size of plotwindow
  void setPlotWindow(int, int);
  /// return latitude,longitude from physical x,y
  bool PhysToGeo(const float, const float, float&, float&);
  /// return physical x,y from physical latitude,longitude
  bool GeoToPhys(const float, const float, float&, float&);
  /// return map x,y from physical x,y
  void PhysToMap(const float, const float, float&, float&);
  /// return field grid x,y from map x,y if field defined and map proj = field proj
  bool MapToGrid(const float, const float, float&, float&);

  double getWindowDistances(const float& x, const float& y,
      const bool horizontal);
  double getMarkedArea(const float& x, const float& y);
  double getWindowArea();

  /// set managers
  void setManagers(FieldManager*, FieldPlotManager*, ObsManager*, SatManager*,
      StationManager*, ObjectManager*, EditManager*);
  Manager *getManager(const std::string &name);

  /// return current plottime
  const miutil::miTime& getPlotTime() const;

  /// return data times (fields,images, observations, objects and editproducts)
  void getPlotTimes(std::map<std::string, std::vector<miutil::miTime> >& times);
  ///returns union or intersection of plot times from all pinfos
  void getCapabilitiesTime(std::set<miutil::miTime>& okTimes,
      const PlotCommand_cpv& pinfos, bool allTimes = true);

  /// set plottime (forwarded to staticPlot_)
  void setPlotTime(const miutil::miTime&);

  ObsPlotCluster* obsplots() const
    { return obsplots_.get(); }

  /// Update ObsPlots if data files have changed
  void updateObs();

  AreaObjectsCluster* areaobjects();

  // locationPlot (vcross,...)
  void putLocation(const LocationData& locationdata);
  void updateLocation(const LocationData& locationdata);
  void deleteLocation(const std::string& name);
  void setSelectedLocation(const std::string& name,
      const std::string& elementname);
  std::string findLocation(int x, int y, const std::string& name);

  // Trajectories
  /// handles trajectory plot info strings
  void trajPos(const std::vector<std::string>&);

  bool startTrajectoryComputation();
  void stopTrajectoryComputation();
  // print trajectory positions to file
  bool printTrajectoryPositions(const std::string& filename);

  // Measurements (distance, velocity)
  void measurementsPos(const std::vector<std::string>&);

  //show or hide all annotations (for fields, observations, satellite etc.)
  void showAnnotations(bool on)
  {
    showanno = on;
  }
  /// mark editable annotationPlot if x,y inside plot
  bool markAnnotationPlot(int, int);
  /// get text of marked and editable annotationPlot
  std::string getMarkedAnnotation();
  /// change text of marked and editable annotationplot
  void changeMarkedAnnotation(std::string text, int cursor = 0, int sel1 = 0,
      int sel2 = 0);
  /// delete marked and editable annotation
  void DeleteMarkedAnnotation();
  /// start editing annotations
  void startEditAnnotation();
  /// stop editing annotations
  void stopEditAnnotation();
  /// go to next element in annotation being edited
  void editNextAnnoElement();
  /// go to last element in annotation being edited
  void editLastAnnoElement();
  /// return vector std::strings with edited annotation for product prodname
  PlotCommand_cpv writeAnnotations(const std::string& prodname);
  /// put info from saved edit labels into new annotation
  void updateEditLabels(const PlotCommand_cpv& productLabelCommands,
      const std::string& productName, bool newProduct);

  void deleteAllEditAnnotations();

  //Objects
  ///objects follow main plot time
  void setObjAuto(bool autoF);

  /// push a new area onto the area stack
  void areaInsert(bool);

  enum ChangeAreaCommand {
    CA_HISTORY_PREVIOUS,
    CA_HISTORY_NEXT,
    CA_DEFINE_MYAREA,
    CA_RECALL_MYAREA,
    CA_RECALL_F5,
    CA_RECALL_F6,
    CA_RECALL_F7,
    CA_RECALL_F8,
  };

  /// respond to shortcuts to move to predefined areas
  void changeArea(ChangeAreaCommand ca);

  /// zoom to specified rectangle
  void setMapAreaFromMap(const Rectangle& rectangle);
  /// zoom out (about 1.3 times)
  void zoomOut();

  // plotelements methods
  /// return PlotElement data (for the speedbuttons)
  std::vector<PlotElement> getPlotElements();
  /// enable one PlotElement
  void enablePlotElement(const PlotElement& pe);

  // keyboard/mouse events
  /// send one mouse event
  void sendMouseEvent(QMouseEvent* me, EventResult& res);

  enum AreaNavigationCommand {
    ANAV_HOME,
    ANAV_TOGGLE_DIRECTION,
    ANAV_PAN_LEFT,
    ANAV_PAN_RIGHT,
    ANAV_PAN_DOWN,
    ANAV_PAN_UP,
    ANAV_ZOOM_OUT,
    ANAV_ZOOM_IN
  };
  void areaNavigation(AreaNavigationCommand anav, EventResult& res);

  // return settings formatted for log file
  std::vector<std::string> writeLog();
  // read settings from log file data
  void readLog(const std::vector<std::string>& vstr,
      const std::string& thisVersion, const std::string& logVersion);

  // Miscellaneous get methods
  FieldPlotCluster* fieldplots()
    { return fieldplots_.get(); }

  typedef std::map<std::string, Manager*> managers_t;
  managers_t managers;

  static PlotModule *instance()
    { return self; }

  StaticPlot* getStaticPlot() const
    { return staticPlot_.get(); }
};

#endif
