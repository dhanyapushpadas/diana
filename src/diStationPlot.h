/*
  Diana - A Free Meteorological Visualisation Tool

  $Id$

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
#ifndef StationPlot_h
#define StationPlot_h

#include <vector>
#include <deque>
#include <iostream>
#include <diPlot.h>
#include <diField/diPlotOptions.h>
#include <puTools/miString.h>
#include <puDatatypes/miCoordinates.h>
#include <diCommonTypes.h>
#include <diField/diColour.h>
#include <diField/diArea.h>
#include <diImageGallery.h>

using namespace std;

  /**
     \brief text and alignment for one stationPlot point
  */
struct stationText{
  miutil::miString text;
  Alignment hAlign;
};


  /**
     \brief properties for a stationplot point

       coordinates, image and text to be shown etc.
  */
struct Station{
  miutil::miString name;
  float lat;
  float lon;
  miutil::miString image;
  miutil::miString image2;
  bool isVisible;
  bool isSelected;
  bool edit;
  int dd,ff,north;
  int alpha;
  float scale;
  Colour colour;
  vector <stationText> vsText;
};


/**
   \brief Plot pickable stations/positions on the map

   Contains data for a set of stations.
   Stations can be displayed by an image or a simple drawing
   with or without name/text. Misc options to set/change "state"
   of each position in the set.
*/
class StationPlot: public Plot{

public:
  //constructors
  StationPlot(){}
  /// constructor with station longitudes and latitudes
  StationPlot(const vector <float> & lons, const vector <float> & lats);
  /// constructor with station names,longitudes and latitudes
  StationPlot(const vector <miutil::miString> & names,const vector <float> & lons,
	      const vector <float> & lats);
  /// constructor with station names,longitudes,latitudes and images
  StationPlot(const vector <miutil::miString> & names,const vector <float> & lons,
	      const vector <float> & lats, const vector <miutil::miString> images);
  StationPlot(const miutil::miString& commondesc,
	      const miutil::miString& common,
	      const miutil::miString& description,
	      int from,
	      const  vector<miutil::miString>& data);
  //destructor
  ~StationPlot();


  /// plot stations
  bool plot();
  bool plot(const int){return false;}
  /// hide stations
  void hide();
  // show stations
  void show();
  /// unselects all stations
  void unselect();
  /// returns true if stationplot visible
  bool isVisible();
  /// change stationplot projection
  bool changeProjection();
  /// find stations in position x and y
  vector<miutil::miString> findStation(int x, int y, bool add=false);
  /// set station with name station to selected<br> if add is false, unselect all stations first
  void setSelectedStation(miutil::miString station, bool add=false);
  /// set station number i to selected<br> if add is false, unselect all stations first
  void setSelectedStation(int i, bool add=false);
  /// get annotation
  void getStationPlotAnnotation(miutil::miString &str,Colour &col);
  /// set annotation
  void setStationPlotAnnotation(miutil::miString &str);
  /// set name/plotname
  void setName(miutil::miString nm);
  /// return name
  miutil::miString getName(){return name;}
  /// set id to i
  void setId(int i){id = i;}
  /// return id
  int getId(){return id;}
  int getPriority(){return priority;}
  /**
   * Get image scale for a station
   * @param i Index of station
   * @return Current image scale for station i
   */
  float getImageScale(int i);
  /// set normal and selected image to im1
  void setImage(miutil::miString im1);
  /// set normal image to im1, selected image to im2
  void setImage(miutil::miString im1,miutil::miString im2);
  /// set new scale for all images
  void setImageScale(float new_scale);
  /// clears all text
  void clearText();
  /// if normal=true write name on all plotted stations, if selected=true write name on all selected stations
  void setUseStationName(bool normal, bool selected);
  void setIcon(miutil::miString icon){iconName = icon;}
  miutil::miString getIcon(){return iconName;}
  void setEditStations(const vector<miutil::miString>& );
  bool getEditStation(int step, miutil::miString& name, int& id,
		      vector<miutil::miString>& stations, bool& updateArea);
  bool stationCommand(const miutil::miString& Command,
		      vector<miutil::miString>& data,
		      const miutil::miString& misc="");
  bool stationCommand(const miutil::miString& Command);
  miutil::miString stationRequest(const miutil::miString& Command);

  friend bool operator==(const StationPlot& lhs, const StationPlot& rhs)
  { return (lhs.stations.size()==rhs.stations.size()) ; }
  friend bool operator>(const StationPlot& lhs, const StationPlot& rhs)
  { return (lhs.stations.size()>rhs.stations.size()); }
  friend bool operator<(const StationPlot& lhs, const StationPlot& rhs)
  { return (lhs.stations.size()<rhs.stations.size()); }


private:

  enum thingToPlot{redRectangle,greyRectangle,redCross,
		   yellowCircle,whiteRectangle,redCircle};

  vector <Station*> stations; //stations, name, lon, lat etc...

  //  void addStation(const miutil::miString names);
  void addStation(const float lon, const float lat,
		  const miutil::miString name="",
		  const miutil::miString image="",
		  int alpha=255, float scale=1.0);
  void defineCoordinates();
  void init();
  void plotStation(int i);
  void glPlot(thingToPlot tp,float x, float y, float w, float h);
  void plotWind(int i, float x, float y,
		bool classic=false, float scale=1);

  vector <float> xplot; //x-positions to plot in current projection
  vector <float> yplot; //y-positions to plot in current projection

  bool visible;
  miutil::miString annotation;
  miutil::miString name; //f.ex. "vprof"
  int id;
  int priority;
  bool useImage;
  bool useStationNameNormal,useStationNameSelected;
  bool showText;
  int textSize;
  Colour textColour;
  miutil::miString textStyle;
  miutil::miString imageNormal,imageSelected;
  miutil::miString iconName;
  int editIndex; //last selected editStation
  int index; //last selected

  float pi;
  GLuint circle;
  ImageGallery ig;


  static miutil::miString ddString[16]; // NN�,N�,�N�,�,�S� etc.
 };

#endif






