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

#ifndef DIGRIDAREAMANAGER_H_
#define DIGRIDAREAMANAGER_H_
#include <map>
#include <list>
#include <diMapMode.h>
#include <diGridConverter.h>
#include <diProjectablePolygon.h>
#include <miString.h>

#ifndef NOLOG4CXX
#include <log4cxx/logger.h>
#else
#include <miLogger/logger.h>
#endif

using namespace std;

class GridArea;
class Point;
enum cursortype;

/**
	\brief Manager for GridAreas
	
	Manages a collection of GridAreas.
	Maintains a current area.
	Modifies the current area depending on the state (paint-mode). 
*/
class GridAreaManager {
	
public:
    enum PaintMode{SELECT_MODE,DRAW_MODE,INCLUDE_MODE,CUT_MODE,MOVE_MODE};
    
private:
#ifndef NOLOG4CXX
    log4cxx::LoggerPtr logger;
#endif
	map<miString,GridArea> gridAreas;
	list< map<miString,GridArea> > history; 
	int maxHistoryLength;
	miString currentId;
	mapMode mapmode;
	GridConverter gc;   // gridconverter class
	float first_x;
	float first_y; 
	float newx,newy;
	PaintMode paintMode;
	bool changeCursor;
        Area base_proj;
	cursortype getCurrentCursor();
	bool selectArea(Point p);
	void saveHistory();
	void updateSelectedArea();

public:
	GridAreaManager(); 
	~GridAreaManager();
	/// Replace all Grid-areas with specified areas
	bool setGridAreas(map<miString,Polygon> newAreas, Area currentProj );
	/// Returns current polygon
	ProjectablePolygon getCurrentPolygon();
	bool inDrawing;
	/// Setting current paint mode
	void setPaintMode(PaintMode mode);
	/// Returns current paint mode
	PaintMode getPaintMode() const;
	/// Adding new empty area with specified id (returns true if added)
	bool addArea(miString id);
	/// Adding specified area with specified id. (returns true if added)
	bool addArea(miString id, ProjectablePolygon area, bool overwrite);
	/// Change area id (returns true if success, false if old ID not found)
	bool changeAreaId(miString oldId, miString newId);
	/// Get area with specified id (returns empty polygon if ID not found)
	ProjectablePolygon getArea(miString id);
	/// Removes area with specified id (returns true if removed)
	bool removeArea(miString id);
	/// Removes current area (returns true if removed)
	bool removeCurrentArea();
	/// Handle mouse event
	void sendMouseEvent(const mouseEvent& me, EventResult& res, float x, float y);
	/// Handle keyboard event
	void sendKeyboardEvent(const keyboardEvent& me, EventResult& res);
	/// Plotting current areas
	bool plot();
	/// true if undo is possible (history available)
	bool isUndoPossible();
	/// Sets selected area to specified id. Returns false if id not found.
	bool setCurrentArea(miString id);
	/// True if current area exist
	bool hasCurrentArea();
	/// True if selected area is not empty (defined area selected) 
	bool isAreaSelected();
	/// True if an empty area is selected. (returns false if no area selected) 
	bool isEmptyAreaSelected();
	/// Perform undo. Returns true if success
	bool undo();
	/// Returns selected id;
	miString getCurrentId();
	/// Returns current mode as string
	miString getModeAsString();
	/// Set plot enabled / disabled. Returns false if area id not found.
	bool setEnabled(miString id, bool enabled);
	/// Clear / Remove all areas. No current area.
	void clear();
	/// Set projection of active field
	void setBaseProjection(Area proj){base_proj = proj;}
	/// Get number of Areas
	int getAreaCount(){ return gridAreas.size(); }
        /// set the list of Points which are actually affected by the mask
        void setActivePoints(vector<Point>);
};


#endif /*DIGRIDAREAMANAGER_H_*/
