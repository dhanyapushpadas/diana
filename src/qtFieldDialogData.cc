/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2018 met.no

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

#include "qtFieldDialogData.h"

#include "diFieldPlotManager.h"

DianaFieldDialogData::DianaFieldDialogData(FieldPlotManager* fpm)
    : fpm_(fpm)
{
}

FieldModelGroupInfo_v DianaFieldDialogData::getFieldModelGroups()
{
  return fpm_->getFieldModelGroups();
}

void DianaFieldDialogData::getFieldPlotGroups(const std::string& modelName, const std::string& refTime, bool predefinedPlots, FieldPlotGroupInfo_v& vfgi)
{
  fpm_->getFieldPlotGroups(modelName, refTime, predefinedPlots, vfgi);
}

std::string DianaFieldDialogData::getFieldGlobalAttribute(const std::string& modelName, const std::string& refTime, const std::string& attribute)
{
  const std::map<std::string, std::string> globalAttributes = fpm_->getFieldGlobalAttributes(modelName, refTime);
  std::map<std::string, std::string>::const_iterator it = globalAttributes.find(attribute);
  if (it != globalAttributes.end())
    return it->second;
  else
    return std::string();
}

std::string DianaFieldDialogData::getBestFieldReferenceTime(const std::string& model, int refOffset, int refHour)
{
  return fpm_->getBestFieldReferenceTime(model, refOffset, refHour);
}

plottimes_t DianaFieldDialogData::getFieldTime(std::vector<FieldRequest>& request)
{
  return fpm_->getFieldTime(request);
}

std::set<std::string> DianaFieldDialogData::getFieldReferenceTimes(const std::string& model)
{
  return fpm_->getFieldReferenceTimes(model);
}

void DianaFieldDialogData::getSetupFieldOptions(std::map<std::string, miutil::KeyValue_v>& fieldOptions)
{
  fpm_->getSetupFieldOptions(fieldOptions);
}
