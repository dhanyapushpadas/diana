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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <iostream>
#include <sstream>
#include <map>
#include <puTools/miString.h>
#include <puTools/miTime.h>
#include <diObsData.h>
#include <diVprofPlot.h>
#include <diObsBufr.h>
#include <algorithm>

using namespace std;
using namespace miutil;

const double bufrMissing = 1.6e+38;

bool ObsBufr::init(const miString& bufr_file, const miString& format)
{

  //	     cerr	 <<"ObsBufr::init:"<<bufr_file<<endl;
  obsTime = miTime(); //undef
  //   const int ibflen=4*512000;
  //   int ibuff[512000];
  const int ibflen = 200000;
  int ibuff[ibflen / 4];
  miString bufr_access = "r";
  int iunit = 20;
  int iret = 0;

  // open for read
  int len_bufr_file = bufr_file.length();
  int len_bufr_access = bufr_access.length();
  pbopen_(&iunit, bufr_file.cStr(), bufr_access.cStr(), &iret, len_bufr_file,
      len_bufr_access);

  if (iret != 0) {
    cerr << "ObsBufr::init: PBOPEN failed for " << bufr_file << "   iret="
        << iret << endl;
    return false;
  }

  bool next = true;

  while (next) { // get the next BUFR product
    int ilen;
    int iibflen = ibflen;
    pbbufr_(&iunit, ibuff, &iibflen, &ilen, &iret);

    if (iret == -1) {
      pbclose_(&iunit,&iret);
      return true; // EOF
    }

    if (iret != 0) {
      if (iret == -2)
        cerr << "ObsBufr::init: ERROR: File handling problem" << endl;
      if (iret == -3)
        cerr << "ObsBufr::init: ERROR: Array too small" << endl;
      pbclose_(&iunit,&iret);
      return false;
    }

    next = BUFRdecode(ibuff, ilen, format);
  }



  pbclose_(&iunit,&iret);

  //init returns false when BUFRdecode returns false
  //BUFRdecode returns false when vprof station is found. Todo: make this more intuitive
  return false;
}

bool ObsBufr::ObsTime(const miString& bufr_file, miTime& time)
{
  //   cerr <<"ObsBufr::ObsTime"<<endl;
  const int ibflen = 200000;
  int ibuff[ibflen / 4];
  //   const int ibflen=4*512000;
  //   int ibuff[512000];
  miString bufr_access = "r";
  int iunit = 20;
  int iret = 0;

  // open for read
  int len_bufr_file = bufr_file.length();
  int len_bufr_access = bufr_access.length();
  pbopen_(&iunit, bufr_file.cStr(), bufr_access.cStr(), &iret, len_bufr_file,
      len_bufr_access);
  if (iret != 0) {
    cerr << "PBOPEN failed for " << bufr_file << "   iret=" << iret << endl;
    return false;
  }

  int ilen;
  int iibflen = ibflen;
  pbbufr_(&iunit, ibuff, &iibflen, &ilen, &iret);

  if (iret != 0) {
    if (iret == -1)
      cerr << "EOF" << endl;
    if (iret == -2)
      cerr << "ERROR: File handling problem" << endl;
    if (iret == -3)
      cerr << "ERROR: Array too small" << endl;
    return false;
  }

  static int ksup[9];
  static int ksec0[3];
  static int ksec1[40];
  static int ksec2[4096];
  int kerr;

  //read sec 0-2
  bus012_(&ilen, ibuff, ksup, ksec0, ksec1, ksec2, &kerr);

  //close file
  pbclose_(&iunit,&iret);

  //  Convert messages with data category (BUFR table A) 0 and 1 only.
  //  0 = Surface data - land, 1 = Surface data - sea
  if (ksec1[5] > 1) {
    return false;
  }

  //HACK if year has only two digits, files from year 1971 to 2070 is assumed
  int year = ksec1[8];
  if (year < 1000) {
    year = (year > 70) ? year + 1900 : year + 2000;
  }
  time = miTime(year, ksec1[9], ksec1[10], ksec1[11], ksec1[12], 0);
  return true;

}

bool ObsBufr::readStationInfo(const vector<miString>& bufr_file,
    vector<miString>& namelist, vector<miTime>& timelist, vector<float>& latitudelist,
    vector<float>& longitudelist)
{

  id.clear();
  idmap.clear();
  id_time.clear();

  for( size_t i=0; i< bufr_file.size(); i++) {
    init(bufr_file[i], "stationInfo");
  }

  namelist = id;
  timelist = id_time;
  latitudelist = latitude;
  longitudelist = longitude;
  return true;

}

VprofPlot* ObsBufr::getVprofPlot(const vector<miString>& bufr_file,
    const miString& modelName,
    const miString& station,
    const miTime& time)
{

  index = izone = istation = 0;
  vplot = new VprofPlot;

  vplot->text.modelName = modelName;

  //if station(no)
  vector<miString> token = station.split("(");
  if (token.size() == 2)
    index = atoi(token[1].cStr());

  strStation = token[0];

  if (token[0].isInt()) {
    int ii = atoi(station.cStr());
    izone = ii / 1000;
    istation = ii - izone * 1000;
  }

  for( size_t i=0; i< bufr_file.size(); i++) {
    //init returns true when reaching end of file, returns false when station is found
    if (!init(bufr_file[i], "vprofplot") ) {
      return vplot;
    }
  }

  // No station found
  return NULL;

}

bool ObsBufr::BUFRdecode(int* ibuff, int ilen, const miString& format)
{
  //   cerr <<"  // Decode BUFR message into fully decoded form."<<endl;

  const int kelem = 40000; //length of subsection
  const int kvals = 360000;

  const int len_cnames = 64, len_cunits = 24, len_cvals = 80;

  static int ksup[9];
  static int ksec0[3];
  static int ksec1[40];
  static int ksec2[4096];
  static int ksec3[4];
  static int ksec4[2];

  static char cnames[kelem][len_cnames];
  static char cunits[kelem][len_cunits];
  static char cvals[kvals][len_cvals];
  static double values[kvals];

  static int ktdlst[kelem];
  static int ktdexp[kelem];
  int ktdlen;
  int ktdexl;
  int kerr;

  int kkvals = kvals;
  bus012_(&ilen, ibuff, ksup, ksec0, ksec1, ksec2, &kerr);
  if (kerr > 0)
    cerr << "ObsBufr: Error in BUS012: KERR=" << kerr << endl;
  int kxelem = kvals / ksup[5];
  if (kxelem > kelem)
    kxelem = kelem;

  bufrex_(&ilen, ibuff, ksup, ksec0, ksec1, ksec2, ksec3, ksec4, &kxelem,
      &cnames[0][0], &cunits[0][0], &kkvals, values, &cvals[0][0], &kerr,
      len_cnames, len_cunits, len_cvals);
  if (kerr > 0) {
    cerr << "ObsBufr::BUFRdecode: Error in BUFREX: KERR=" << kerr << endl;
    return true;
  }

  int nsubset = ksup[5];

  //  Convert messages with data category (BUFR table A) 0 and 1 only.
  //  0 = Surface data - land, 1 = Surface data - sea
  //  if (ksec1[5] > 1) {
  //    cerr <<ksec1[5]<<endl;
  //    return true;
  //  }

  //HACK if year has only two digits, files from year 1971 to 2070 is assumed
  if (obsTime.undef()) {
    int year = ksec1[8];
    if (year < 1000) {
      year = (year > 70) ? year + 1900 : year + 2000;
      obsTime = miTime(year, ksec1[9], ksec1[10], ksec1[11], ksec1[12], 0);
    }
  }

  // Return list of Data Descriptors from Section 3 of Bufr message, and
  // total/requested list of elements. BUFREX must have been called before BUSEL.


  for (int i = 1; i < nsubset + 1; i++) {

    busel2_(&i, &kxelem, &ktdlen, ktdlst, &ktdexl, ktdexp, &cnames[0][0],
        &cunits[0][0], &kerr);
    if (kerr > 0)
      cerr << "ObsBufr::init: Error in BUSEL: KERR=" << kerr << endl;

    if (format.downcase() == "obsplot") {

      ObsData & obs = oplot->getNextObs();

      if (oplot->getLevel() < -1) {
        if (!get_diana_data(ktdexl, ktdexp, values, cvals, len_cvals, i - 1,
            kxelem, obs) || !oplot->timeOK(obs.obsTime)){
          oplot->removeObs();
        }
      } else {
        if (!get_diana_data_level(ktdexl, ktdexp, values, cvals, len_cvals, i
            - 1, kxelem, obs, oplot->getLevel()) || !oplot->timeOK(obs.obsTime))
          oplot->removeObs();
      }
    } else if (format.downcase() == "vprofplot") {
      //will return without reading more subsets, fix later
      return !get_data_level(ktdexl, ktdexp, values, cvals, len_cvals, i - 1,
          kxelem, obsTime);
    } else if (format.downcase() == "stationinfo") {
      get_station_info(ktdexl, ktdexp, values, cvals, len_cvals, i - 1, kelem);
    }

  }

  return true;
}

bool ObsBufr::get_diana_data(int ktdexl, int *ktdexp, double* values,
    const char cvals[][80], int len_cvals, int subset, int kelem, ObsData &d)
{

  d.fdata.clear();

  // constants for changing to met.no units
  const double pa2hpa = 0.01;
  const double t0 = 273.15;

  int wmoBlock = 0;
  int wmoStation = 0;
  int year = 0;
  int month = 0;
  int day = 0;
  int hour = 0;
  int minute = 0;
  bool landStation = false;
  float heightOfSensor = -1.0;
  float depth = -1.0;
  int timePeriodMissing = -9999;
  int timePeriodHour = timePeriodMissing;
  int timePeriodMinute = timePeriodMissing;
  int timeDisplacement = 0;
  miString cloudStr;
  unsigned int selected_wind_vector_ambiguities = 0;
  unsigned int wind_speed_selection_count = 1;
  unsigned int wind_dir_selection_count = 1;

  bool skip_time = false; // Will be set to true if 008021 is encountered
  // with value 26, which means 'Time of last
  // known position', used for buoys. The date/time
  // descriptors following this will be skipped.

  d.CAVOK = false;
  d.xpos = -32767;
  d.ypos = -32767;

  for (int i = 0, j = kelem * subset; i < ktdexl; i++, j++) {
//cerr <<ktdexp[i]<<" : "<<values[j]<<endl;
    switch (ktdexp[i]) {
    //   8021  TIME SIGNIFICANCE
    case 8021:
      if (int(values[j]) == 26) // 'Time of last known position', present
        skip_time = true; // in buoy reports after observation time
      break;

      //   1001  WMO BLOCK NUMBER
    case 1001:
      wmoBlock = int(values[j]);
      d.zone = wmoBlock;
      break;

      //   1002  WMO STATION NUMBER
    case 1002:
      wmoStation = int(values[j]);
      landStation = true;
      break;

      //   1005 BUOY/PLATFORM IDENTIFIER
    case 1005:
      d.id = miString(values[j]);
      landStation = false;
      break;

      // Not used in diana
      //     case 1015:
      //       c=0;
      //       while (c<5 && int(cvals[i][c])!=0 && int(cvals[i][c])!=32) {
      // 	d.id += cvals[i][c];
      // 	c++;
      //       }
      //     break;

      //  1011  SHIP OR MOBILE LAND STATION IDENTIFIER, CCITTIA5 (ascii chars)
    case 1006:
    {
      if (! landStation) {
        int index = int(values[j]) / 1000 - 1;
        for (int k = 0; k < 6; k++) {
          d.id += cvals[index][k];
        }
        landStation = false;
      }
    }
    break;

    case 1011:
    {
      if ( !landStation ) {
        int index = int(values[j]) / 1000 - 1;
        for (int k = 0; k < 7; k++) {
          d.id += cvals[index][k];
        }
        landStation = false;
      }
    }
    break;

    case 1195:
    {
      int index = int(values[j]) / 1000 - 1;
      for (int k = 0; k < 5; k++) {
        d.id += cvals[index][k];
      }
      landStation = false;
    }
    break;

    case 1063:
    {
      int index = int(values[j]) / 1000 - 1;
      for (int k = 0; k < 4; k++) {
        d.metarId += cvals[index][k];
      }
      landStation = false;
    }
    break;

    //   1012  DIRECTION OF MOTION OF MOVING OBSERVING PLATFORM, DEGREE TRUE
    case 1012:
      if (values[j] > 0 && values[j] < bufrMissing)
        d.fdata["ds"] = values[j];
      break;

      //   1013  SPEED OF MOTION OF MOVING OBSERVING PLATFORM, M/S
    case 1013:
      if (values[j] < bufrMissing)
        d.fdata["vs"] = ms2code4451(values[j]);
      break;

      //   2001 TYPE OF STATION
    case 2001:
      if (values[j] < bufrMissing)
        d.fdata["auto"] = values[j];
      break;

      //   4001  YEAR
    case 4001:
      if (!skip_time && values[j]<bufrMissing )
        year = int(values[j]);
      break;

      //   4002  MONTH
    case 4002:
      if (!skip_time && values[j]<bufrMissing )
        month = int(values[j]);
      break;

      //   4003  DAY
    case 4003:
      if (!skip_time && values[j]<bufrMissing )
        day = int(values[j]);
      break;

      //   4004  HOUR
    case 4004:
      if (!skip_time && values[j]<bufrMissing )
        hour = int(values[j]);
      break;

      //   4005  MINUTE
    case 4005:
      if (!skip_time && values[j]<bufrMissing )
        minute = int(values[j]);
      break;

      //   5001  LATITUDE (HIGH ACCURACY),   DEGREE
      //   5002  LATITUDE (COARSE ACCURACY), DEGREE
    case 5001:
    case 5002:
      if (values[j]<bufrMissing ) {
        d.ypos = values[j];
      }
      break;

      //   6001  LONGITUDE (HIGH ACCURACY),   DEGREE
      //   6002  LONGITUDE (COARSE ACCURACY), DEGREE
    case 6001:
    case 6002:
      if (values[j] < bufrMissing) {
        d.xpos = values[j];
      }
      break;

      //   7001  HEIGHT OF STATION, M
    case 7030:
      if (values[j] < bufrMissing)
        d.fdata["stationHeight"] = values[j];
      break;

      //   7032  HEIGHT OF SENSOR, M
    case 7032:
      if (values[j] < bufrMissing)
        heightOfSensor = values[j];
      break;

      //   7063  Depth, M
    case 7063:
      if (values[j] < bufrMissing)
        depth = values[j];
      break;

      //   04024  Time period or displacement hour
    case 4024:
      if (ktdexp[i - 1] == 4024) { // Second occurrence, must be displacement
        if (values[j] < bufrMissing) {
          timeDisplacement = int(values[j]);
        } else {
          // Not sure what to do here; 0 looks like best guess
          timeDisplacement = 0;
        }
      } else { // First occurrence, must be time period
        timeDisplacement = 0; // resets time displacement to 'missing' value
        if (values[j] < bufrMissing) {
          timePeriodHour = int(values[j]);
        } else {
          timePeriodHour = timePeriodMissing;
        }
      }
      break;

      //   04025  Time period minute
    case 4025:
      if (values[j] < bufrMissing) {
        timePeriodMinute = int(values[j]);
      } else {
        timePeriodMinute = timePeriodMissing;
      }
      break;

      // 010008 GEOPOTENTIAL (20 bits), M**2/S**2
      // 010003 GEOPOTENTIAL (17 bits), M**2/S**2
    case 10008:
    case 10003:
      if (values[j] < bufrMissing) {
        d.fdata["HHH"] = values[j] / 9.8;
      }
      break;
      // 010009 GEOPOTENTIAL HEIGHT
    case 10009:
      if (values[j] < bufrMissing)
        d.fdata["HHH"] = values[j];
      break;

      //   10051  PRESSURE REDUCED TO MEAN SEA LEVEL, Pa->hPa
    case 10051:
    case 7004:
      if (values[j] < bufrMissing)
        d.fdata["PPPP"] = values[j] * pa2hpa;
      break;

      //010052 ALTIMETER SETTING (QNH), Pa->hPa
    case 10052:
      if (values[j] < bufrMissing)
        d.fdata["PHPHPHPH"] = values[j] * pa2hpa;
      break;

      //   10061  3 HOUR PRESSURE CHANGE, Pa->hpa
    case 10061:
      if (values[j] < bufrMissing)
        d.fdata["ppp"] = values[j] * pa2hpa;
      break;

      //   10063  CHARACTERISTIC OF PRESSURE TENDENCY
    case 10063:
      if (values[j] < bufrMissing)
        d.fdata["a"] = values[j];
      break;

      //   11011  WIND DIRECTION AT 10 M, DEGREE TRUE
    case 11011:
      if (values[j] < bufrMissing) {
        if ( selected_wind_vector_ambiguities ) {
          if ( selected_wind_vector_ambiguities == wind_dir_selection_count ) {
            d.fdata["dd"] = values[j];
            //cerr<<"dd:"<<values[j]<<endl;
          }
          wind_dir_selection_count++;
        } else {
          d.fdata["dd"] = values[j];
        }
      }
      break;

      // 011001 WIND DIRECTION
    case 11001:
      if (!d.fdata.count("dd") && values[j] < bufrMissing) {
        d.fdata["dd"] = values[j];
      }
      break;

      //   11012  WIND SPEED AT 10 M, m/s
    case 11012:
      if (values[j] < bufrMissing) {
        if ( selected_wind_vector_ambiguities ) {
        //cerr <<"wind_speed_selection_count:"<<wind_speed_selection_count<<endl;
          if ( selected_wind_vector_ambiguities == wind_speed_selection_count ) {
            d.fdata["ff"] = values[j];
          //  cerr<<"ff:"<<values[j]<<endl;
          }
          wind_speed_selection_count++;
        } else {
          d.fdata["ff"] = values[j];
        }
      }
      break;

    // 011002 WIND SPEED
    case 11002:
      if (!d.fdata.count("ff") && values[j] < bufrMissing )
        d.fdata["ff"] = values[j];
      break;

      // 011016 EXTREME COUNTERCLOCKWISE WIND DIRECTION OF A VARIABLE WIND
    case 11016:
      if (values[j] < bufrMissing)
        d.fdata["dndndn"] = values[j];
      break;

      // 011017 EXTREME CLOCKWISE WIND DIRECTIONOF A VARIABLE WIND
    case 11017:
      if (values[j] < bufrMissing)
        d.fdata["dxdxdx"] = values[j];
      break;

      // 011041 MAXIMUM WIND SPEED (GUSTS), m/s
    case 11041:
      if (values[j] < bufrMissing) {
        //        d.fdata["911ff"] = values[j];
        d.fdata["fmfm"] = values[j]; //metar
        if (timePeriodMinute == -10) {
          d.fdata["911ff_10"] = values[j];
        } else if (timePeriodMinute == -60) {
          d.fdata["911ff_60"] = values[j];
        } else if (timePeriodMinute == -180) {
          d.fdata["911ff_180"] = values[j];
        } else if (timePeriodMinute == -360) {
          d.fdata["911ff_360"] = values[j];
        }
      }
      break;

      // 011042 MAXIMUM WIND SPEED (10 MIN MEAN WIND), m/s
    case 11042:
      if (values[j] < bufrMissing) {
        //        d.fdata["fxfx"] = values[j];
        if (timePeriodMinute == -60) {
          d.fdata["fxfx_60"] = values[j];
        } else if (timePeriodMinute == -180) {
          d.fdata["fxfx_180"] = values[j];
        } else if (timePeriodMinute == -360) {
          d.fdata["fxfx_360"] = values[j];
        }
      }
      break;

      //   12104  DRY BULB TEMPERATURE AT 2M (16 bits), K->Celsius
      //   12004  DRY BULB TEMPERATURE AT 2M (12 bits), K->Celsius
    case 12104:
    case 12004:
      if (values[j] < bufrMissing)
        d.fdata["TTT"] = values[j] - t0;
      break;
      //   12101  TEMPERATURE/DRY BULB TEMPERATURE (16 bits), K->Celsius
      //   12001  TEMPERATURE/DRY BULB TEMPERATURE (12 bits), K->Celsius
    case 12101:
    case 12001:
      if (!d.fdata.count("TTT") && values[j] < bufrMissing){
        d.fdata["TTT"] = values[j] - t0;
      }
      break;

      //   12106  DEW POINT TEMPERATURE AT 2M (16 bits), K->Celsius
      //   12006  DEW POINT TEMPERATURE AT 2M (12 bits), K->Celsius
    case 12106:
    case 12006:
      if (values[j] < bufrMissing)
        d.fdata["TdTdTd"] = values[j] - t0;
      break;
      //   12103  DEW POINT TEMPERATURE (16 bits), K->Celsius
      //   12003  DEW POINT TEMPERATURE (12 bits), K->Celsius
    case 12103:
    case 12003:
      if (!d.fdata.count("TdTdTd") && values[j] < bufrMissing)
        d.fdata["TdTdTd"] = values[j] - t0;
      break;

      //   12014  MAX TEMPERATURE AT 2M, K->Celsius
    case 12014:
      if (values[j] < bufrMissing && hour == 18)
        d.fdata["TxTn"] = values[j] - t0;
      break;
      //   12111  MAX TEMPERATURE AT HEIGHT AND OVER PERIOD SPECIFIED (16 bits), K->Celsius
      //   12011  MAX TEMPERATURE AT HEIGHT AND OVER PERIOD SPECIFIED (12 bits), K->Celsius
    case 12111:
    case 12011:
      if (!d.fdata.count("TxTn") && values[j] < bufrMissing && hour
          == 18 && timePeriodHour == -12 && timeDisplacement == 0)
        d.fdata["TxTn"] = values[j] - t0;
      break;

      //   12015  MIN TEMPERATURE AT 2M, K->Celsius
    case 12015:
      if (values[j] < bufrMissing && hour == 6)
        d.fdata["TxTn"] = values[j] - t0;
      break;

      //   12112  MIN TEMPERATURE AT HEIGHT AND OVER PERIOD SPECIFIED (16 bits), K->Celsius
      //   12012  MIN TEMPERATURE AT HEIGHT AND OVER PERIOD SPECIFIED (12 bits), K->Celsius
    case 12112:
    case 12012:
      if (!d.fdata.count("TxTn") && values[j] < bufrMissing && hour
          == 6 && timePeriodHour == -12 && timeDisplacement == 0)
        d.fdata["TxTn"] = values[j] - t0;
      break;

      //   13013  Snow depth
    case 13013:
      if (values[j] < bufrMissing) {
        // note: -0.01 means a little (less than 0.005 m) snow
        //       -0.02 means snow cover not continuous
        if (values[j] < 0) {
          d.fdata["sss"] = 0.;
        } else {
          d.fdata["sss"] = values[j] * 100;
        }
      }
      break;
      //   20062 State of the ground (with or without snow)
    case 20062:
      if (!d.fdata.count("sss") && values[j] < 13) // Indicates no snow or partially snow cover
        d.fdata["sss"] = 0.;
      break;
      //   13218 Ind. for snow-depth-measurement - DNMI
    case 13218: //   0 -> MEASURED IN METRES
      //   1 -> 997     (less than 5mm)
      //   2 -> 998     (partially snow cover)
      //   3 -> 999     (measurement inaccurate or impossible)
      //   4 -> MISSING
      if (!d.fdata.count("sss") && (int(values[j]) == 1 || int(values[j]) == 2))
        d.fdata["sss"] = 0.;
      break;

      //13019 Total precipitation past 1 hour
    case 13019:
      if (values[j] < bufrMissing) {
        d.fdata["RRR_1"] = values[j];
      }
      break;

      //13020 Total precipitation past 3 hour
    case 13020:
      if (values[j] < bufrMissing) {
        d.fdata["RRR_3"] = values[j];
      }
      break;

      //13021 Total precipitation past 6 hour
    case 13021:
      if (values[j] < bufrMissing) {
        d.fdata["RRR_6"] = values[j];
      }
      break;

      //13022 Total precipitation past 12 hour
    case 13022:
      if (values[j] < bufrMissing) {
        d.fdata["RRR_12"] = values[j];
      }
      break;

      //13023 Total precipitation past 24 hour
    case 13023:
      if (values[j] < bufrMissing) {
        d.fdata["RRR_24"] = values[j];
      }
      break;

      // 13011 Total precipitation / total water equivalent of snow
    case 13011:
      if (values[j] < bufrMissing) {
        //        d.fdata["RRR_accum"] =-1 * timePeriodHour;
        //        d.fdata["RRR"] = values[j];
        if (timePeriodHour == -24 ) {
          d.fdata["RRR_24"] = values[j];
        } else if (timePeriodHour == -12 ) {
          d.fdata["RRR_12"] = values[j];
        } else if (timePeriodHour == -6 ) {
          d.fdata["RRR_6"] = values[j];
        } else if (timePeriodHour == -3 ) {
          d.fdata["RRR_3"] = values[j];
        } else if (timePeriodHour == -1 ) {
          d.fdata["RRR_1"] = values[j];
        }
      }
      break;

      //   20001  HORIZONTAL VISIBILITY M
    case 20001:
      if (values[j] > 0 && values[j] < bufrMissing) {
        d.fdata["VV"] = values[j];
        //Metar
        if (values[j] > 9999) //remove VVVV=0
          d.fdata["VVVV"] = 9999;
        else
          d.fdata["VVVV"] = values[j];
      }
      break;

      // 5021 BEARING OR AZIMUTH, DEGREE
    case 5021: //Metar
      if (values[j] < bufrMissing)
        d.fdata["Dv"] = values[j];
      break;

      //  20003  PRESENT WEATHER, CODE TABLE  20003
    case 20003:
      if (values[j] < 200) //values>200 -> w1w1, ignored here
        d.fdata["ww"] = values[j];
      break;

      //  20004 PAST WEATHER (1),  CODE TABLE  20004
    case 20004:
      if (values[j] > 2 && values[j] < 20)
        d.fdata["W1"] = values[j];
      break;

      //  20005  PAST WEATHER (2),  CODE TABLE  20005
    case 20005:
      if (values[j] > 2 && values[j] < 20)
        d.fdata["W2"] = values[j];
      break;

      //   Clouds

      // 020009 GENERAL WEATHER INDICATOR (TAF/METAR)
    case 20009:
      if (values[j] < bufrMissing)
        d.CAVOK = (int(values[j]) == 2);
      break;

      // 20013 HEIGHT OF BASE OF CLOUD (M)
    case 20013:
      if (values[j] < bufrMissing) {
        if (!d.fdata.count("h")) {
          d.fdata["h"] = height_of_clouds(values[j]);
        }
        cloudStr += cloudHeight(int(values[j]));
      }
      break;

      // 20010 CLOUD COVER (TOTAL %)
      // Note: 113 means "Sky obscured by fog and/other meteorological
      // phenomena" but Hirlam BUFR uses 109 for this. Corresponds to
      // N=9 in synop.
    case 20010:
      if (values[j] < bufrMissing) {
        if (values[j] == 109 || values[j] == 113) {
          d.fdata["N"] = 9;
        } else {
          d.fdata["N"] = values[j] / 12.5; //% -> oktas
        }
      }
      break;

      //20011  CLOUD AMOUNT
    case 20011:
      if (values[j] < bufrMissing) {
        if (i < 32)
          d.fdata["Nh"] = values[j];
        if (cloudStr.exists()) {
          d.cloud.push_back(cloudStr);
          cloudStr.clear();
        }
        cloudStr = cloudAmount(int(values[j]));
      }
      break;

      // 20012 CLOUD TYPE, CODE TABLE  20012
    case 20012:
      if (values[j] < bufrMissing) {
        cloud_type(d, values[j]);
        cloudStr += cloud_TCU_CB(int(values[j]));
      }
      break;

      // 020019 SIGNIFICANT RECENT WEATHER PHENOMENA, CCITTIA5
    case 20019: {
      int index = int(values[j]) / 1000 - 1;
      miString ww;
      for (int k = 0; k < 9; k++)
        ww += cvals[index][k];
      ww.trim();
      if (ww.exists())
        d.ww.push_back(ww);
    }
    break;

    // 020020 SIGNIFICANT RECENT WEATHER PHENOMENA, CCITTIA5
    case 20020: {
      int index = int(values[j]) / 1000 - 1;
      miString REww;
      for (int j = 0; j < 4; j++)
        REww += cvals[index][j];
      REww.trim();
      if (REww.exists())
        d.REww.push_back(REww);
    }
    break;

    case 21102:
      if ( values[j] < bufrMissing ) {
        selected_wind_vector_ambiguities = values[j];
        //cerr <<"selected_wind_vector_ambiguities:"<<selected_wind_vector_ambiguities<<endl;
      }
    break;

    // 022011 PERIOD OF WAVES, s
    case 22011:
      if (values[j] < bufrMissing)
        d.fdata["PwaPwa"] = values[j];
      break;

      // 022021 HEIGHT OF WAVES, m
    case 22021:
      if (values[j] < bufrMissing)
        d.fdata["HwaHwa"] = values[j];
      break;

      // Not used in synop plot
      //       // 022012 PERIOD OF WIND WAVES, s
      //     case 22012:
      //       if (values[j]<bufrMissing)
      // 	d.fdata["PwPw"] = values[j];
      //       break;

      //       // 022022 HEIGHT OF WIND WAVES, m
      //     case 22022:
      //       if (values[j]<bufrMissing)
      // 	d.fdata["HwHw"] = values[j];
      //       break;

      // 022013 PERIOD OF SWELL WAVES, s (first system of swell)
    case 22013:
      if (!d.fdata.count("Pw1Pw1") && values[j] < bufrMissing)
        d.fdata["Pw1Pw1"] = values[j];
      break;

      // 022023 HEIGHT OF SWELL WAVES, m
    case 22023:
      if (!d.fdata.count("Hw1Hw1") && values[j] < bufrMissing)
        d.fdata["Hw1Hw1"] = values[j];
      break;

      // 022003 DIRECTION OF SWELL WAVES, DEGREE
    case 22003:
      if (!d.fdata.count("dw1dw1") && values[j] > 0 && values[j] < bufrMissing)
        d.fdata["dw1dw1"] = values[j];
      break;

      //   22042  SEA/WATER TEMPERATURE, K->Celsius
    case 22042:
      if (values[j] < bufrMissing)
        d.fdata["TwTwTw"] = values[j] - t0;
      break;

      //   22043  SEA/WATER TEMPERATURE, K->Celsius
    case 22043:
      if (!d.fdata.count("TwTwTw") && values[j] < bufrMissing && depth < 1) //??
        d.fdata["TwTwTw"] = values[j] - t0;
      break;

      // 022061 STATE OF THE SEA, CODE TABLE  22061
    case 22061:
      if (values[j] < bufrMissing)
        d.fdata["s"] = values[j];
      break;

      // 022038 TIDE
    case 22038:
      if (values[j] < bufrMissing)
        d.fdata["TE"] = values[j];
      break;

    }
  }


  if (landStation) {
    ostringstream ostr;
    ostr << setw(2) << setfill('0') << wmoBlock << setw(3) << setfill('0')
            << wmoStation;
    d.id = ostr.str();
  } else {
    d.fdata["stationHeight"] = 0.0;
    d.zone = 99;
  }

  //Metar cloud
  if (cloudStr.exists()) {
    d.cloud.push_back(cloudStr);
    cloudStr.clear();
  }

  //PRESSURE TENDENCY - ppp may or may not include sign, if a>4 then ppp<0
  if(d.fdata.count("ppp") && d.fdata.count("a") && d.fdata["a"]>4 && d.fdata["ppp"]>0) {
    d.fdata["ppp"] *= -1;
  }

  //TIME
  if ( miTime::isValid(year, month, day, hour, minute, 0) ) {
    d.obsTime = miTime(year, month, day, hour, minute, 0);
  }

  //skip obs if xpos or ypos  or obsTime not ok
  if ( d.xpos > -32767 && d.ypos > -32767 && !d.obsTime.undef()) {
    return true;
  }

  return false;

}

bool ObsBufr::get_station_info(int ktdexl, int *ktdexp, double* values,
    const char cvals[][80], int len_cvals, int subset, int kelem)
{

  int wmoBlock = 0;
  int wmoStation = 0;
  int year = 0;
  int month = 0;
  int day = 0;
  int hour = 0;
  int minute = 0;
  miString station;
  bool landStation = true;
  int nn = 0; //what is nn used for??

  for (int i = 0, j = kelem * subset; i < ktdexl; i++, j++) {

    switch (ktdexp[i]) {

    //   1001  WMO BLOCK NUMBER
    case 1001:
      wmoBlock = int(values[j]);
      nn++;
      break;

      //   1002  WMO STATION NUMBER
    case 1002:
      wmoStation = int(values[j]);
      nn++;
      break;

      //  1011  SHIP OR MOBILE LAND STATION IDENTIFIER, CCITTIA5 (ascii chars)
    case 1011:
    case 1194:
    {
      if ( !landStation ) {
        int index = int(values[j]) / 1000 - 1;
        //miString station;
        for (int k = 0; k < 6; k++) {
          station+= cvals[index][k];
        }
        if (station.exists()) {
          landStation = false;
          nn += 2;
        }
      }
    }
    break;

    //   4001  YEAR
    case 4001:
      year = int(values[j]);
      break;

      //   4002  MONTH
    case 4002:
      month = int(values[j]);
      break;

      //   4003  DAY
    case 4003:
      day = int(values[j]);
      break;

      //   4004  HOUR
    case 4004:
      hour = int(values[j]);
      break;

      //   4005  MINUTE
    case 4005:
      minute = int(values[j]);
      break;

    //   5001  LATITUDE (HIGH ACCURACY),   DEGREE
    //   5002  LATITUDE (COARSE ACCURACY), DEGREE
    case 5001:
    case 5002:
      latitude.push_back(values[j]);
      nn++;
      break;

      //   6001  LONGITUDE (HIGH ACCURACY),   DEGREE
      //   6002  LONGITUDE (COARSE ACCURACY), DEGREE
    case 6001:
    case 6002:
      longitude.push_back(values[j]);
      nn++;
      break;

    }
  }

  ostringstream ostr;
  if (landStation) {
    ostr << setw(2) << setfill('0') << wmoBlock << setw(3) << setfill('0')
            << wmoStation;
    station = ostr.str();
  } else {
    ostr << station;
  }

  if (idmap.count(station)) {
    ostr << "(" << idmap[station] << ")";
    idmap[station]++;
  } else {
    idmap[station] = 1;
  }
  id.push_back(ostr.str());
  id_time.push_back( miTime(year, month, day, hour, minute, 0));
  return true;

}

bool ObsBufr::get_diana_data_level(int ktdexl, int *ktdexp, double* values,
    const char cvals[][80], int len_cvals, int subset, int kelem, ObsData &d,
    int level)
{
  //    cerr <<"get_diana_data"<<endl;
  d.fdata.clear();
  d.id.clear();
  d.zone = 0;

  // constants for changing to met.no units
  const double pa2hpa = 0.01;
  const double t0 = 273.15;

  int wmoBlock = 0;
  int wmoStation = 0;
  int year = 0;
  int month = 0;
  int day = 0;
  int hour = 0;
  int minute = 0;
  bool landStation = true;

  int levelmin = level - level/20;
  int levelmax = level + level/20;

  bool found = false;
  bool stop = false;
  bool replication = false;

  for (int i = 0, j = kelem * subset; i < ktdexl; i++, j++) {

    if (!replication // station info
        || found // right pressure level
        || ktdexp[i] == 7004  // next pressure level
        || ktdexp[i] == 7062) { // next sea level
      switch (ktdexp[i]) {

      //   031001 DELAYED DESCRIPTOR REPLICATION FACTOR [NUMERIC]
      case 31001:
        replication=true;
        break;

      //   1001  WMO BLOCK NUMBER
      case 1001:
        wmoBlock = int(values[j]);
        d.zone = wmoBlock;
        break;

        //   1002  WMO STATION NUMBER
      case 1002:
        wmoStation = int(values[j]);
        break;

        //   1005 BUOY/PLATFORM IDENTIFIER [NUMERIC]
      case 1005:
        d.id = miString(int(values[j]));
        landStation = false;
        break;

        //   001007 SATELLITE IDENTIFIER [CODE TABLE]
      case 1007:
      {
        if ( !landStation ) {
          int index = int(values[j]) / 1000 - 1;
          for (int k = 0; k < 4; k++) {
            d.id += cvals[index][k];
          }
          if (d.id.exists()) {
            landStation = false;
          }
        }
      }
      break;

      //  1011  SHIP OR MOBILE LAND STATION IDENTIFIER, CCITTIA5 (ascii chars)
      case 1011:
      {
        if ( !landStation ) {
          int index = int(values[j]) / 1000 - 1;
          for (int k = 0; k < 4; k++) {
            d.id += cvals[index][k];
          }
          if (d.id.exists()) {
            landStation = false;
          }
        }
      }
      break;

      //   4001  YEAR
      case 4001:
        year = int(values[j]);
        break;

        //   4002  MONTH
      case 4002:
        month = int(values[j]);
        break;

        //   4003  DAY
      case 4003:
        day = int(values[j]);
        break;

        //   4004  HOUR
      case 4004:
        if (values[j] < bufrMissing) {
          hour = int(values[j]);
        }
        break;

        //   4005  MINUTE
      case 4005:
        if (values[j] < bufrMissing) {
          minute = int(values[j]);
        }
        break;

        //   5001  LATITUDE (HIGH ACCURACY),   DEGREE
        //   5002  LATITUDE (COARSE ACCURACY), DEGREE
      case 5001:
      case 5002:
        d.ypos = values[j];
        break;

        //   6001  LONGITUDE (HIGH ACCURACY),   DEGREE
        //   6002  LONGITUDE (COARSE ACCURACY), DEGREE
      case 6001:
      case 6002:
        d.xpos = values[j];
        break;

        //   7001  HEIGHT OF STATION, M
      case 7001:
        if (values[j] < bufrMissing)
          d.fdata["stationHeight"] = values[j];
        break;

        //   007004  PRESSURE, Pa->hPa
      case 7004:
        if (found) {
          stop = true;
        } else {
          if (values[j] < bufrMissing) {
            if (int(values[j] * pa2hpa) > levelmin && int(values[j] * pa2hpa) < levelmax) {
              found = true;
              d.fdata["PPPP"] = values[j] * pa2hpa;
            }
          }
        }
        break;

        //   007062  DEPTH BELOW SEA/WATER SURFACE [M]
      case 7062:
        if (found) {
          stop = true;
        } else {
          if (values[j] < bufrMissing) {
            if (int(values[j]) > levelmin && int(values[j]) < levelmax) {
              found = true;
              d.fdata["depth"] = values[j];
            }
          }
        }
        break;

        // 011001 WIND DIRECTION
      case 11001:
        if (values[j] < bufrMissing) {
          d.fdata["dd"] = values[j];
        }
        break;

        // 011002 WIND SPEED
      case 11002:
        if (values[j] < bufrMissing) {
          d.fdata["ff"] = values[j];
        }
        break;

        //   12101  TEMPERATURE/DRY BULB TEMPERATURE (16 bits), K->Celsius
        //   12001  TEMPERATURE/DRY BULB TEMPERATURE (12 bits), K->Celsius
      case 12101:
      case 12001:
        if (values[j] < bufrMissing){
          d.fdata["TTT"] = values[j] - t0;
        }
        break;

        //   12103  DEW POINT TEMPERATURE (16 bits), K->Celsius
        //   12003  DEW POINT TEMPERATURE (12 bits), K->Celsius
      case 12103:
      case 12003:
        if (values[j] < bufrMissing)
          d.fdata["TdTdTd"] = values[j] - t0;
        break;

        //   10008 GEOPOTENTIAL (20 bits), M**2/S**2
        //   10003 GEOPOTENTIAL (17 bits), M**2/S**2
      case 10008:
      case 10003:
        if (values[j] < bufrMissing)
          d.fdata["HHH"] = values[j] / 9.8;
        break;
        //   10009 GEOPOTENTIAL HEIGHT
      case 10009:
        if (values[j] < bufrMissing)
          d.fdata["HHH"] = values[j];
        break;

        // 022011  PERIOD OF WAVES [S]
      case 22011:
        if (values[j] < bufrMissing)
          d.fdata["PwaPwa"] = values[j];
        break;

        // 022021  HEIGHT OF WAVES [M]
      case 22021:
        if (values[j] < bufrMissing)
          d.fdata["HwaHwa"] = values[j];
        break;

        // 22043 SEA/WATER TEMPERATURE [K]
      case 22043:
        if (values[j] < bufrMissing)
          d.fdata["TTTT"] = values[j] - t0;
        break;

        // 22062 SALINITY [PART PER THOUSAND]
      case 22062:
        if (values[j] < bufrMissing)
          d.fdata["SSSS"] = values[j];
        break;

      }
    }

    // right pressure level found and corresponding parameters read
    if (stop)
      break;
  }

  //right pressure level not found
  if (!found){
    return false;
  }
  if (landStation) {
    ostringstream ostr;
    ostr << setw(2) << setfill('0') << wmoBlock << setw(3) << setfill('0')
                << wmoStation;
    d.id = ostr.str();
  } else {
    d.fdata["stationHeight"] = 0.0;
    d.zone = 99;
  }

  //TIME
  d.obsTime = miTime(year, month, day, hour, minute, 0);

  return true;
}

bool ObsBufr::get_data_level(int ktdexl, int *ktdexp, double* values,
    const char cvals[][80], int len_cvals, int subset, int kelem, miTime time)
{
  // constants for changing to met.no units
  const double pa2hpa = 0.01;
  const double t0 = 273.15;
  const float rad = 3.141592654 / 180.;
  const double ms2knots = 3600.0 / 1852.0;

  //  int wmoBlock = 0;
  //  int wmoStation = 0;
  miString station;
  int year = 0;
  int month = 0;
  int day = 0;
  int hour = 0;
  int minute = 0;
  float p = 0, tt = -30000, td = -30000;
  float fff, ddd;
  int dd, ff, bpart;
  float lat = 0., lon = 0.;
  //  bool landStation = true;
  int ffmax = -1, kmax = -1;

  static int ii = 0;

  bool ok = false;
  bool found = false;

  for (int i = 0, j = kelem * subset; i < ktdexl; i++, j++) {

    if (ktdexp[i] < 7000 // station info
        || ok // pressure ok
        || ktdexp[i] == 7004) { // next pressure level

      if (ktdexp[i] == 7004) { //new pressure level, save data
        if(!found)   {
          return false;
        }

        if (p > 0. && p < 1300.) {
          if (tt > -30000.) {
            vplot->ptt.push_back(p);
            vplot->tt.push_back(tt);
            if (td > -30000.) {
              vplot->ptd.push_back(p);
              vplot->td.push_back(td);
              vplot->pcom.push_back(p);
              vplot->tcom.push_back(tt);
              vplot->tdcom.push_back(td);
              td = -31000.;
            }
            tt = -31000.;
          }
          if (dd >= 0 && dd <= 360 && ff >= 0) {
            vplot->puv.push_back(p);
            vplot->dd.push_back(dd);
            vplot->ff.push_back(ff);
            // convert to east/west and north/south component
            fff = float(ff);
            ddd = (float(dd) + 90.) * rad;
            vplot->uu.push_back(fff * cosf(ddd));
            vplot->vv.push_back(-fff * sinf(ddd));
            vplot->sigwind.push_back(bpart);
            if (ff > ffmax) {
              ffmax = ff;
              kmax = vplot->sigwind.size() - 1;
            }
            dd = ff = -1;
            bpart = 1;
          }
        }
      }

      switch (ktdexp[i]) {

      //   1001  WMO BLOCK NUMBER
      case 1001:
      {
        if (izone != int(values[j])) {
          return false;
        }
        found = true;
      }
      break;

      //   1002  WMO STATION NUMBER
      case 1002:
      {
        if (istation != int(values[j])) {
          return false;
        }
        if (index != ii) {
          ii++;
          return false;
        }
        ii = 0;
        found = true;
      }
      break;



      // 1011  SHIP OR MOBILE LAND STATION IDENTIFIER, CCITTIA5 (ascii chars)
      case 1006:
      case 1011:
      case 1194:

      {
        if ( !found ) {
          station.clear();
          int iindex = int(values[j]) / 1000 - 1;
          for (int k = 0; k < 6; k++) {
            station += cvals[iindex][k];
          }
          strStation.trim();
          station.trim();
          if( strStation != station ) {
            return false;
          }
          if( index != ii){
            ii++;
            return false;
          }
          if(station.exists()){
            ii=0;
          }
          found = true;
        }
      }
      break;

      //   4001  YEAR
      case 4001:
        year = int(values[j]);
        break;

        //   4002  MONTH
      case 4002:
        month = int(values[j]);
        break;

        //   4003  DAY
      case 4003:
        day = int(values[j]);
        break;

        //   4004  HOUR
      case 4004:
        hour = int(values[j]);
        break;

        //   4005  MINUTE
      case 4005:
        minute = int(values[j]);
        break;

        //   5001  LATITUDE (HIGH ACCURACY),   DEGREE
        //   5002  LATITUDE (COARSE ACCURACY), DEGREE
      case 5001:
      case 5002:{
        lat = values[j];
      }
      break;

      //   6001  LONGITUDE (HIGH ACCURACY),   DEGREE
      //   6002  LONGITUDE (COARSE ACCURACY), DEGREE
      case 6001:
      case 6002:
      {
        lon = values[j];
      }
      break;

      //   10051  PRESSURE REDUCED TO MEAN SEA LEVEL, Pa->hPa
      case 7004:
        if (values[j] < bufrMissing) {
          p = int(values[j] * pa2hpa);
          ok = (p > 0. && p < 1300.);
        } else {
          p=-1;
         ok=false;
        }
        break;

        //   VERTICAL SOUNDING SIGNIFICANCE
      case 8001:
        if (values[j] > 31. && values[j] < 64)
          bpart = 0;
        else
          bpart = 1;
        break;

        //   11001  WIND DIRECTION
      case 11001:
        if (values[j] < bufrMissing)
          dd = int(values[j]);
        break;

        //   11002  WIND SPEED
      case 11002:
        if (values[j] < bufrMissing)
          ff = int(values[j] * ms2knots + 0.5); //should be done elsewhere
        break;

        //   12101  TEMPERATURE/DRY BULB TEMPERATURE (16 bits), K->Celsius
        //   12001  TEMPERATURE/DRY BULB TEMPERATURE (12 bits), K->Celsius
      case 12101:
      case 12001:
        if (values[j] < bufrMissing)
          tt = values[j] - t0;
        break;

        //   12103  DEW POINT TEMPERATURE (16 bits), K->Celsius
        //   12003  DEW POINT TEMPERATURE (12 bits), K->Celsius
      case 12103:
      case 12003:
        if (values[j] < bufrMissing)
          td = values[j] - t0;
        break;

      }
    }

    // right pressure level found and corresponding parameters read
  }

  //right pressure level not found
  if (p > 0. && p < 1300.) {
    if (tt > -30000.) {
      vplot->ptt.push_back(p);
      vplot->tt.push_back(tt);
      if (td > -30000.) {
        vplot->ptd.push_back(p);
        vplot->td.push_back(td);
        vplot->pcom.push_back(p);
        vplot->tcom.push_back(tt);
        vplot->tdcom.push_back(td);
      }
    }
    if (dd >= 0 && dd <= 360 && ff >= 0) {
      vplot->puv.push_back(p);
      vplot->dd.push_back(dd);
      vplot->ff.push_back(ff);
      // convert to east/west and north/south component
      fff = float(ff);
      ddd = (float(dd) + 90.) * rad;
      vplot->uu.push_back(fff * cosf(ddd));
      vplot->vv.push_back(-fff * sinf(ddd));
      vplot->sigwind.push_back(bpart);
      if (ff > ffmax) {
        ffmax = ff;
        kmax = vplot->sigwind.size() - 1;
      }
    }
  }

  vplot->text.posName = strStation;
  vplot->text.posName.trim();
  vplot->text.prognostic = false;
  vplot->text.forecastHour = 0;
  vplot->text.validTime = miTime(year, month, day, hour, minute, 0);
  vplot->text.latitude = lat;
  vplot->text.longitude = lon;
  vplot->text.kindexFound = false;

  if (kmax >= 0)
    vplot->sigwind[kmax] = 3;

  vplot->prognostic = false;
  int l1 = vplot->ptt.size();
  int l2 = vplot->puv.size();
  vplot->maxLevels = (l1 > l2) ? l1 : l2;

  return true;
}

miString ObsBufr::cloudAmount(int i)
{
  if (i == 0)
    return "SKC";
  if (i == 1)
    return "NSC";
  if (i == 8)
    return "O/";
  if (i == 11)
    return "S/";
  if (i == 12)
    return "B/";
  if (i == 13)
    return "F/";
  return "";
}

miString ObsBufr::cloudHeight(int i)
{
  ostringstream cl;
  i /= 30;
  cl << setw(3) << setfill('0') << i;
  return miString(cl.str());
}

miString ObsBufr::cloud_TCU_CB(int i)
{
  if (i == 3)
    return " TCU";
  if (i == 9)
    return " CB";
  return "";
}

float ObsBufr::height_of_clouds(double height)
{
  if (height > 2500)
    return 9.0;
  if (height > 2000)
    return 8.0;
  if (height > 1500)
    return 7.0;
  if (height > 1000)
    return 6.0;
  if (height > 600)
    return 5.0;
  if (height > 300)
    return 4.0;
  if (height > 200)
    return 3.0;
  if (height > 100)
    return 2.0;
  if (height > 50)
    return 1.0;
  return 9.0; // when there are no clouds -> height = 0
}

void ObsBufr::cloud_type(ObsData& d, double v)
{
  int type = int(v) / 10;
  float value = float(int(v) % 10);

  if (value < 1)
    return;

  if (type == 1)
    d.fdata["Ch"] = value;
  if (type == 2)
    d.fdata["Cm"] = value;
  if (type == 3)
    d.fdata["Cl"] = value;

}

float ObsBufr::ms2code4451(float v)
{
  if (v < 1852.0 / 3600.0)
    return 0.0;
  if (v < 5 * 1852.0 / 3600.0)
    return 1.0;
  if (v < 10 * 1852.0 / 3600.0)
    return 2.0;
  if (v < 15 * 1852.0 / 3600.0)
    return 3.0;
  if (v < 20 * 1852.0 / 3600.0)
    return 4.0;
  if (v < 25 * 1852.0 / 3600.0)
    return 5.0;
  if (v < 30 * 1852.0 / 3600.0)
    return 6.0;
  if (v < 35 * 1852.0 / 3600.0)
    return 7.0;
  if (v < 40 * 1852.0 / 3600.0)
    return 8.0;
  if (v < 25 * 1852.0 / 3600.0)
    return 9.0;
  return 9.0;
}
