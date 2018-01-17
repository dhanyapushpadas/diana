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

#ifndef diUtilities_h
#define diUtilities_h 1

#include <puTools/miTime.h>

#include <map>
#include <string>
#include <vector>

class Plot;
class Rectangle;

namespace diutil {

template<class C>
void delete_all_and_clear(C& container)
{
  for (typename C::iterator it=container.begin(); it!=container.end(); ++it)
    delete *it;
  container.clear();
}

typedef std::vector<std::string> string_v;

//! glob filenames, return matches, or empty if error
string_v glob(const std::string& pattern, int glob_flags, bool& error);

//! glob filenames, return matches, or empty if error
inline string_v glob(const std::string& pattern, int glob_flags=0)
{ bool error; return glob(pattern, glob_flags, error); }

//! return adjusted i so that it is between 0 and available; if repeat, return i % available
int find_index(bool repeat, int available, int i);

bool getFromFile(const std::string& filename, string_v& lines);
bool getFromHttp(const std::string &url, string_v& lines);

bool getFromAny(const std::string &url_or_filename, string_v& lines);

inline int float2int(float f)
{ return (int)(f > 0.0 ? f + 0.5 : f - 0.5); }

inline int ms2knots(float ff)
{ return (float2int(ff*3600.0/1852.0)); }

inline float knots2ms(float ff)
{ return (ff*1852.0/3600.0); }

/*! replace reftime by refhour and refoffset
  refoffset is 0 today, -1 yesterday etc independent of time of the day
*/
void replace_reftime_with_offset(std::string& pstr, const miutil::miDate& nowdate);

/*! Make list of numbers around 'number'.
 */
std::vector<std::string> numberList(float number, const float* enormal);

} // namespace diutil

#endif // diUtilities_h
