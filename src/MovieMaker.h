/*
 Diana - A Free Meteorological Visualisation Tool

 $Id: qtMainWindow.h 477 2008-05-06 09:53:22Z lisbethb $

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

#ifndef MOVIEMAKER_H_
#define MOVIEMAKER_H_

#include <string>

class QImage;

struct AVFormatContext;
struct AVFrame;
struct AVStream;

struct OutputCtx {
  const char *fileName;
  AVFormatContext *outputCtx;
  AVStream *videoStream;
  AVFrame *frame;

  short *videoBuffer;
};


class MovieMaker {
public:
  /**
   * Constructor. Sets filename to save the finished animation to, and
   * which quality it will be saved in.
   */
  MovieMaker(const std::string &filename, const std::string &format,
             float delay);

  ~MovieMaker();

  std::string outputFile() const;
  std::string outputFormat() const;

  bool addImage(const QImage &image);

private:
  float delay;
  std::string g_strOutputVideoFile;
  std::string g_strOutputVideoFormat;
  std::string g_strInputImageFile;

  OutputCtx outputVideo;

  bool makeVideoFrame(const QImage *image);
  bool addVideoStream(OutputCtx *output);
  bool openVideoEncoder(OutputCtx *output);
  bool initOutputStream(OutputCtx *output);
  void closeVideoEncoder(OutputCtx *output);
  void endOutputStream(OutputCtx *output);
  bool writeVideoFrame(OutputCtx *output);
};

#endif /*MOVIEMAKER_H_*/
