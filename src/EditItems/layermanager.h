/*
  Diana - A Free Meteorological Visualisation Tool

  $Id$

  Copyright (C) 2013 met.no

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

#ifndef EDITITEMSLAYERMANAGER_H
#define EDITITEMSLAYERMANAGER_H

#include <QObject>
//#define QT_SHAREDPOINTER_TRACK_POINTERS
#include <QSharedPointer>
#include <QList>
#include <EditItems/drawingitembase.h>

class DrawingManager;

namespace EditItems {

class LayerGroup;
class Layer;

class LayerManager : public QObject
{
  Q_OBJECT
public:
  LayerManager();
  ~LayerManager();
  bool isEmpty() const;
  void clear();
  QList<QSharedPointer<Layer> > selectedLayers() const;
  int selectedLayersItemCount() const;
  QSet<QSharedPointer<DrawingItemBase> > itemsInSelectedLayers(bool = false) const;
  bool selectedLayersContainItem(const QSharedPointer<DrawingItemBase> &) const;
  bool selectItem(const QSharedPointer<DrawingItemBase> &, bool = false, bool = true);
  bool selectItem(int, bool = false, bool = true);
  bool deselectItem(const QSharedPointer<DrawingItemBase> &, bool = true);
  bool deselectAllItems(bool = true);
  void addToLayerGroup(QSharedPointer<LayerGroup> &, const QList<QSharedPointer<Layer> > &);
  void addToLayerGroup(QSharedPointer<LayerGroup> &, const QSharedPointer<Layer> &);
  QSharedPointer<LayerGroup> addToNewLayerGroup(const QList<QSharedPointer<Layer> > &, const QString & = QString());
  QSharedPointer<LayerGroup> addToNewLayerGroup(const QSharedPointer<Layer> &, const QString & = QString());
  QSharedPointer<LayerGroup> createNewLayerGroup(const QString &) const;
  QSharedPointer<Layer> createNewLayer(const QString & = QString()) const;
  QSharedPointer<Layer> createDuplicateLayer(const QList<QSharedPointer<Layer> > &, const DrawingManager *) const;
  void mergeLayers(const QList<QSharedPointer<Layer> > &, const QSharedPointer<Layer> &) const;
  const QList<QSharedPointer<LayerGroup> > &layerGroups() const;
  const QList<QSharedPointer<Layer> > &orderedLayers() const;
  QSharedPointer<LayerGroup> findLayerGroup(const QString &) const;
  QSharedPointer<LayerGroup> findLayerGroup(const QSharedPointer<Layer> &) const;
  QSharedPointer<Layer> findLayer(const QString &) const;
  bool findItem(int, QSharedPointer<DrawingItemBase> &, QSharedPointer<Layer> &) const;
  bool findLayer(const QSharedPointer<DrawingItemBase> &, QSharedPointer<Layer> &) const;
  void removeLayer(const QSharedPointer<Layer> &);
  void moveLayer(const QSharedPointer<Layer> &, const QSharedPointer<Layer> &);
  void removeItem(const QSharedPointer<DrawingItemBase> &, bool = true);

private:
  QList<QSharedPointer<LayerGroup> > layerGroups_;
  // the layers of all layer groups organized in a single, ordered list:
  QList<QSharedPointer<Layer> > orderedLayers_;

  QString createUniqueLayerGroupName(const QString &) const;
  void ensureUniqueLayerGroupName(const QSharedPointer<LayerGroup> &) const;

  QString createUniqueLayerName(const QString &) const;
  void ensureUniqueLayerName(const QSharedPointer<Layer> &) const;

  void selectItem(const QSharedPointer<DrawingItemBase> &, QSharedPointer<Layer> &, bool, bool = true);
};

} // namespace

#endif // EDITITEMSLAYERMANAGER_H
