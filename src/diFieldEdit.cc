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
//#define TESTICECONC

//#define DEBUGCLASSES

//#define DEBUGREDRAW

#include <diFieldEdit.h>
#include <diFortranUnit.h>
#include <diPlotModule.h>
#include <milib.h>
#include <math.h>
#include <stdio.h>


// static class members
editType FieldEdit::editstate= edit_value;
int      FieldEdit::def_influencetype= 0;
float    FieldEdit::def_ecellipse= 0.88;
bool     FieldEdit::drawExtraLines= false;


FieldEdit::FieldEdit()
  : editfield(0), editfieldplot(0), numundo(0), active(false),
    editStarted(false), operationStarted(false), discontinuous(false),
    justDoneUndoRedo(false), convertpos(false), odata(0), specset(false),
    workfield(0),posx(0),posy(0),showArrow(false),
    drawIsoline(false), numUndefReplaced(0),
    minValue(fieldUndef), maxValue(fieldUndef),
    lastFileWritten(""), showNumbers(false), numbersDisplayed(false)
{
#ifdef DEBUGPRINT
  cerr << "FieldEdit constructor" << endl;
#endif
}


FieldEdit::~FieldEdit()
{
#ifdef DEBUGPRINT
  cerr << "FieldEdit destructor" << endl;
#endif
  cleanup();
}


// Assignment operator
FieldEdit& FieldEdit::operator=(const FieldEdit &rhs){
#ifdef DEBUGPRINT
  cerr << "FieldEdit assignment operator" << endl;
#endif
  if (this == &rhs) return *this;

  // first delete
  if (editfield) delete editfield;
  if (workfield) delete workfield;
  if (editfieldplot) delete editfieldplot;
  if (odata) delete[] odata;

  // odata
  if ( rhs.nx!=0 && rhs.ny!=0 && rhs.odata!=0 ){
    nx = rhs.nx;
    ny = rhs.ny;
    fsize = nx*ny;
    odata = new float[fsize];
    for ( int i=0; i<fsize; i++ )
      odata[i] = rhs.odata[i];
  }

  // editfield
  editfield= new Field;
  if ( rhs.editfield != 0 )
    *(editfield)= *(rhs.editfield);
  miTime tprod= editfield->validFieldTime;
  vector<Field*> vf;
  vf.push_back(editfield);

  // fieldPlot
  editfieldplot= new FieldPlot();
  editfieldplot->setData(vf, tprod);
  editfieldplot->setColourMode(true);

  // all the other members...
  showNumbers=rhs.showNumbers;
  numbersDisplayed=rhs.numbersDisplayed;

  specset=rhs.specset;
  areaspec=rhs.areaspec;
  areaminimize=rhs.areaminimize;
  minValue=rhs.minValue;
  maxValue=rhs.maxValue;

  lastFileWritten=rhs.lastFileWritten;

  for (int i=0; i<8; i++) metnoFieldFileIdentSpec[i]=rhs.metnoFieldFileIdentSpec[i];
  for (int i=0; i<20; i++) metnoFieldFileIdent[i]=rhs.metnoFieldFileIdent[i];

  undofields.clear();
  numundo=0;

  active=rhs.active;
  editStarted=rhs.editStarted;
  operationStarted=rhs.operationStarted;

  def_rcircle=rhs.def_rcircle;
  def_axellipse=rhs.def_axellipse;
  def_ayellipse=rhs.def_ayellipse;

  maparea=rhs.maparea;
  posx=rhs.posx;
  posy=rhs.posy;
  influencetype=rhs.influencetype;
  rcircle=rhs.rcircle;
  axellipse=rhs.axellipse;
  ayellipse=rhs.ayellipse;
  ecellipse=rhs.ecellipse;
  orcircle=rhs.orcircle;
  oaxellipse=rhs.oaxellipse;
  oayellipse=rhs.oayellipse;
  oecellipse=rhs.oecellipse;
  rcirclePlot=rhs.rcirclePlot;
  axellipsePlot=rhs.axellipsePlot;
  ayellipsePlot=rhs.ayellipsePlot;
  xArrow=rhs.xArrow;
  yArrow=rhs.yArrow;
  showArrow=rhs.showArrow;

  xfirst=rhs.xfirst;
  yfirst=rhs.yfirst;
  xrefpos=rhs.xrefpos;
  yrefpos=yrefpos;
  deltascale=rhs.deltascale;
  xprev=rhs.xprev;
  yprev=rhs.yprev;
  lineincrement=rhs.lineincrement;
  convertpos=rhs.convertpos;
  justDoneUndoRedo=rhs.justDoneUndoRedo;
  firstInfluence=rhs.firstInfluence;
  currentInfluence=rhs.currentInfluence;
  i1ed=rhs.i1ed;
  i2ed=rhs.i2ed;
  j1ed=rhs.j1ed;
  j2ed=rhs.j2ed;
  i1edp=rhs.i1edp;
  i2edp=rhs.i2edp;
  j1edp=rhs.j1edp;
  j2edp=rhs.j2edp;
  numUndefReplaced=rhs.numUndefReplaced;
  xline=rhs.xline;
  yline=rhs.yline;
  fline=rhs.fline;
  lineinterval=rhs.lineinterval;
  numsmooth=rhs.numsmooth;
  brushValue=rhs.brushValue;
  brushReplaceUndef=rhs.brushReplaceUndef;
  classLineValue=rhs.classLineValue;
  isoline=rhs.isoline;
  lockedValue=rhs.lockedValue;
  discontinuous=rhs.discontinuous;
  drawIsoline=rhs.drawIsoline;

  return *this;
}

void FieldEdit::cleanup()
{
  if (editfieldplot) delete editfieldplot;
  editfieldplot= 0;

  if (workfield && workfield!=editfield) delete workfield;
  workfield= 0;
  editfield= 0;

  int n= undofields.size();
  for (int i=0; i<n; i++) {
    delete[] undofields[i].data[0];
    delete[] undofields[i].data[1];
  }
  undofields.clear();
  numundo= 0;

  // annet (odata,xline,yline,isoline,.....) ????????????????
  if (odata) delete[] odata;
  odata= 0;

  lockedValue.clear();
}


bool FieldEdit::changedEditField(){

  if (!editfield) return false;
  if (!editfield->data) return false;
  if (numundo==0) return false;

  return true;
}


void FieldEdit::getFieldSize(int &nx, int &ny) {

  if (editfield) {
    nx= editfield->nx;
    ny= editfield->ny;
  } else {
    nx= ny= 0;
  }
}


void FieldEdit::setSpec(const EditProduct& ep, int fnum) {

  metnoFieldFileIdentSpec[0]= ep.producer;
  metnoFieldFileIdentSpec[1]= ep.gridnum;
  metnoFieldFileIdentSpec[2]= 1;
  metnoFieldFileIdentSpec[3]= 0;
  metnoFieldFileIdentSpec[4]= ep.fields[fnum].vcoord;
  metnoFieldFileIdentSpec[5]= ep.fields[fnum].param;
  metnoFieldFileIdentSpec[6]= ep.fields[fnum].level;
  metnoFieldFileIdentSpec[7]= ep.fields[fnum].level2;
  areaspec=     ep.area;
  areaminimize= ep.areaminimize;
  minValue=     ep.fields[fnum].minValue;
  maxValue=     ep.fields[fnum].maxValue;

  specset= true;
}


bool FieldEdit::prepareEditFieldPlot(const miString& fieldname,
				     const miTime& tprod){

  if (!editfield) return false;

  editfield->numSmoothed= 0;

  editfield->validFieldTime= tprod;

  metnoFieldFileIdent[0]= editfield->producer;
  metnoFieldFileIdent[1]= editfield->gridnum;

  if (specset) {
    for (int i=0; i<2; i++)
      if (metnoFieldFileIdentSpec[i]>0)
	metnoFieldFileIdent[i]= metnoFieldFileIdentSpec[i];
    for (int i=2; i<8; i++)
      metnoFieldFileIdent[i]= metnoFieldFileIdentSpec[i];
  } else {
    metnoFieldFileIdent[2]= 1;  // analysis (datatype=1 forecastHour=0(
    metnoFieldFileIdent[3]= 0;
  }

  metnoFieldFileIdent[11]= tprod.year();
  metnoFieldFileIdent[12]= tprod.month()*100 + tprod.day();
  metnoFieldFileIdent[13]= tprod.hour() *100 + tprod.min();

  // text for plot etc.
  miString text, fulltext;
  text = "ANALYSE " + fieldname;

  miString sclock= editfield->validFieldTime.isoClock();
  miString shour=  sclock.substr(0,2);
  miString smin=   sclock.substr(3,2);
  if (smin=="00")
    fulltext = text + " " + editfield->validFieldTime.isoDate()
                    + " " + shour + " UTC";
  else
    fulltext = text + " " + editfield->validFieldTime.isoDate()
		    + " " + shour + ":" + smin + " UTC";

  editfield->name=     fieldname;
  editfield->text=     text;
  editfield->fulltext= fulltext;

  vector<Field*> vf;
  vf.push_back(editfield);

  if (editfieldplot) delete editfieldplot;
  editfieldplot= new FieldPlot();
  editfieldplot->setData(vf, tprod);
  editfieldplot->setColourMode(true);

  return true;
}


void FieldEdit::makeWorkfield()
{
  if (workfield && workfield!=editfield) delete workfield;
  workfield= 0;

  if (!editfield->allDefined) {
    int nx= editfield->nx;
    int ny= editfield->ny;
    int nundef= 0;
    float avg= 0.0;
    for (int i=0; i<nx*ny; i++) {
      if (editfield->data[i]!=fieldUndef)
	avg+=editfield->data[i];
      else
	nundef++;
    }
    if (nundef>0 && nundef<nx*ny) {
      workfield= new Field;
      *(workfield)= *(editfield);
      float *f= workfield->data;
      avg/=float(nx*ny-nundef);
      int *indx= new int[nundef];
      int n1a,n1b,n2a,n2b,n3a,n3b,n4a,n4b,n5a,n5b;
      int i,j,ij,n;
      n=0;
      n1a=n;
      for (j=1; j<ny-1; j++)
	for (i=1; i<nx-1; i++)
	  if (f[j*nx+i]==fieldUndef) indx[n++]= j*nx+i;
      n1b=n;
      n2a=n;
      i=0;
      for (j=1; j<ny-1; j++)
	if (f[j*nx+i]==fieldUndef) indx[n++]= j*nx+i;
      n2b=n;
      n3a=n;
      i=nx-1;
      for (j=1; j<ny-1; j++)
	if (f[j*nx+i]==fieldUndef) indx[n++]= j*nx+i;
      n3b=n;
      n4a=n;
      j=0;
      for (i=0; i<nx; i++)
	if (f[j*nx+i]==fieldUndef) indx[n++]= j*nx+i;
      n4b=n;
      n5a=n;
      j=ny-1;
      for (i=0; i<nx; i++)
	if (f[j*nx+i]==fieldUndef) indx[n++]= j*nx+i;
      n5b=n;

      for (i=0; i<nx*ny; i++)
        if (f[i]==fieldUndef) f[i]= avg;

      float cor= 1.6;
      float error;

      // much faster than in the DNMI MIPOM ocean model,
      // where method was found to fill undefined values with
      // rather sensible values...

      for (int loop=0; loop<100; loop++) {

	for (n=n1a; n<n1b; n++) {
	  ij=indx[n];
	  error= (f[ij-nx]+f[ij-1]+f[ij+1]+f[ij+nx])*0.25-f[ij];
	  f[ij]+=(error*cor);
	}
	for (n=n2a; n<n2b; n++) {
	  ij=indx[n];
	  f[ij]=f[ij+1];
        }
	for (n=n3a; n<n3b; n++) {
	  ij=indx[n];
	  f[ij]=f[ij-1];
        }
	for (n=n4a; n<n4b; n++) {
	  ij=indx[n];
	  f[ij]=f[ij+nx];
	}
 	for (n=n5a; n<n5b; n++) {
	  ij=indx[n];
	  f[ij]=f[ij-nx];
	}

      }

      if (minValue!=fieldUndef) {
 	for (n=n1a; n<n5b; n++) {
	  ij=indx[n];
	  if (f[ij]<minValue) f[ij]= minValue;
	}
      }
      if (maxValue!=fieldUndef) {
 	for (n=n1a; n<n5b; n++) {
	  ij=indx[n];
	  if (f[ij]>maxValue) f[ij]= maxValue;
	}
      }

      delete[] indx;

      workfield->allDefined= true;
    }
  }

  // if no workfield made (not needed or impossible), just set pointer
  if (!workfield) workfield= editfield;
}


bool FieldEdit::OLDreadEditfield(const miString& filename)
{
  FILE *pfile;

  int oldlength= filename.length() - 2;
  miString oldfilename= filename.substr(0 ,oldlength);

  if ((pfile= fopen(oldfilename.c_str(), "rb")) == NULL) {
    cerr << "OPEN ERROR (READ) " << oldfilename << endl;
    return false;
  }

  float gspec[Projection::speclen];
  int   gtype;

  editfield= new Field;

  bool ok;
  int nw,nx,ny;
  nw= fread(metnoFieldFileIdent,2,20,pfile);
  ok= (nw==20);

  if (ok) {
    nw= fread(&gtype,4,1,pfile);
    ok= (nw==1);
  }
  if (ok) {
    nw= fread(gspec,4,6,pfile);
    ok= (nw==6);
  }

  if (ok) {
    nx= metnoFieldFileIdent[ 9];
    ny= metnoFieldFileIdent[10];
    if (nx<1 || ny<1) {
      cerr << "ERROR: EditField size:" << nx << " " << ny << endl;
      ok= false;
    }
  }

  if (ok) {
    Projection p(gtype, gspec);
    Rectangle r(0., 0., float(nx-1), float(ny-1));
    editfield->area.setP(p);
    editfield->area.setR(r);
    editfield->nx= nx;
    editfield->ny= ny;
    delete[] editfield->data;
    editfield->data= new float[nx*ny];
    nw= fread(editfield->data,4,nx*ny,pfile);
    ok= (nw==nx*ny);
    if (ok) {
      int i=0;
      while (i<nx*ny && editfield->data[i]!=fieldUndef) i++;
      editfield->allDefined= (i==nx*ny);
    }
  }

  fclose(pfile);

  if (ok) return true;

  cerr << "ERROR READ " << oldfilename << endl;
  delete editfield;
  editfield= 0;
  fclose(pfile);

  return false;
}


bool FieldEdit::readEditfield(const miString& filename)
{
  // read a DNMI field file

  FortranUnit funit;

  if (!funit.Lock()) {
    cerr << "No available fortran file unitnumber" << endl;
    return false;
  }

  bool found= false;

  const int maxinh= 8;
  short int inh[maxinh][16];
  int   nfoundq, ifoundq[maxinh];
  int   iunit, irequest, iexist, iend, ioerr;
  int   ninh= maxinh;

  int   mode  = 1;
  int   ipack = 2;
  float fscale= 1.0;
  float fdum=   0.0;
  short idfile[32];
  int   ierror= 1;

  // open the DNMI field file
  mrfelt(mode,filename.c_str(),funit.Num(),&inh[0][0],ipack,1,&fdum,1.0,
         32,idfile,&ierror);
  if (ierror) {
    cerr << "Something went wrong during opening.." << endl;
    funit.Unlock();
    return false;
  }

  // initialize qfelt (always smart...)
  qfelt(0,0,0,ninh,&inh[0][0],ifoundq,&nfoundq,&iend,&ierror,&ioerr);

  iunit=    funit.Num();
  irequest= 11;
  iexist  = 1;


  for (int i=0; i<16; ++i) inh[0][i]=-32767;

  // find existing fields
  qfelt(iunit,irequest,iexist,ninh,&inh[0][0],ifoundq,&nfoundq,
        &iend,&ierror,&ioerr);
  if (ierror!=0) cerr << "QFELT ERROR,IOERROR: "
                      << ierror << " " << ioerr << endl;

  for (int n=0; n<nfoundq; n++) {

    bool read= true;
    if (specset) {
      read= (inh[n][10]==metnoFieldFileIdentSpec[4] &&
	     inh[n][11]==metnoFieldFileIdentSpec[5] &&
             inh[n][12]==metnoFieldFileIdentSpec[6] &&
	     inh[n][13]==metnoFieldFileIdentSpec[7]);
    }

    if (read) {
      mode=2;
      int ldata=  ifoundq[n] + 20 + 100;
      int lfield= ifoundq[n];
      short int *idata= new short int[ldata];
      float *field= new float[lfield];
      mrfelt(mode,filename.c_str(),funit.Num(),&inh[n][0],
	     ipack,lfield,field,fscale,
             ldata,idata,&ierror);
      if (ierror) {
        cerr << "Something went wrong during reading.." << endl;
	delete[] field;
      } else {
        editfield= new Field;
	editfield->data= field;
        for (int i=0; i<20; i++) metnoFieldFileIdent[i]= idata[i];
	float gspec[Projection::speclen];
	int   gtype;
        gridpar(+1,ldata,idata,&gtype,&nx,&ny,gspec,&ierror);
        if (ierror) {
          cerr << "FieldEdit::readEditfield GRIDPAR ERROR" << endl;
	  delete editfield;
        } else {
          Projection p(gtype, gspec);
          Rectangle r(0., 0., float(nx-1), float(ny-1));
          editfield->area.setP(p);
          editfield->area.setR(r);
          editfield->nx= nx;
          editfield->ny= ny;
          found= true;
	  int i=0;
	  while (i<nx*ny && editfield->data[i]!=fieldUndef) i++;
	  editfield->allDefined= (i==nx*ny);
        }
      }
      delete[] idata;
      if (found) break;
    }
  }

  // close the DNMI field file
  mode= 3;
  mrfelt(mode,filename.c_str(),funit.Num(),&inh[0][0],ipack,1,&fdum,1.0,
         1,idfile,&ierror);

  funit.Unlock();

  return found;
}


void FieldEdit::setData(const vector<Field*>& vf,
                        const miString& fieldname,
			const miTime& tprod) {

  cleanup();

  if (vf.size()==0) return;

  editfield= new Field;

  // copy the Field object
  *(editfield)= *(vf[0]);

  if (specset && metnoFieldFileIdentSpec[1]>0) {
    int gridnum= metnoFieldFileIdentSpec[1];
    miString demands= "fine.interpolation";
    if (areaminimize) demands+= " minimize.area";
    cerr << "Interpolation to analysis grid" << endl;
    if (!editfield->changeGrid(areaspec,demands,gridnum))
      cerr << "   specification/interpolation failure!!!!" << endl;
  }

  prepareEditFieldPlot(fieldname,tprod);
}


void FieldEdit::setConstantValue(float value) {

  if (!editfield) return;

  int fsize= editfield->nx * editfield->ny;
  for (int i=0; i<fsize; ++i)
    editfield->data[i]= value;

  editfield->allDefined= (value!=fieldUndef);
}


bool FieldEdit::readEditFieldFile(const miString& filename,
                                  const miString& fieldname,
				  const miTime& tprod){

  cleanup();

  if (!readEditfield(filename)) {
    //if (!OLDreadEditfield(filename)) return false;
//########################################################
    if (!OLDreadEditfield(filename)) {
      miString filename2= filename + "00";
      if (!OLDreadEditfield(filename2)) return false;
    }
//########################################################
  }

  prepareEditFieldPlot(fieldname,tprod);

  return true;
}


void FieldEdit::OLDwriteEditFieldFile(const miString& filename){

  if (!editfield) return;
  if (!editfield->data) return;

  // all gridspec may not be in ident... use area
  Projection p= editfield->area.P();
  float gspec[Projection::speclen];
  p.Gridspecstd(gspec);
  int gtype= p.Gridtype();

  FILE *pfile;

  if ((pfile= fopen(filename.c_str(), "wb+")) == NULL) {
    cerr << "OPEN ERROR (WRITE) " << filename << endl;
    return;
  }

  bool ok;
  int nw;
  nw= fwrite(metnoFieldFileIdent,2,20,pfile);
  ok= (nw==20);
  if (ok) {
    nw= fwrite(&gtype,4,1,pfile);
    ok= (nw==1);
  }
  if (ok) {
    nw= fwrite(gspec,4,6,pfile);
    ok= (nw==6);
  }

  if (ok) {
    int nx= editfield->nx;
    int ny= editfield->ny;

    nw= fwrite(editfield->data,4,nx*ny,pfile);
    ok= (nw==nx*ny);
  }

  fclose(pfile);

  if (!ok) {
    cerr << "ERROR WRITE " << filename << endl;
    return;
  }

  int n= undofields.size();
  for (int i=0; i<n; i++) {
    delete[] undofields[i].data[0];
    delete[] undofields[i].data[1];
  }
  undofields.clear();
  numundo=0;
}


bool FieldEdit::writeEditFieldFile(const miString& filename,
				   bool returndata,
				   short int** fdata, int& fdatalength) {

  // create and write a DNMI field file,
  // and possibly return data for database

  if (!editfield) return false;
  if (!editfield->data) return false;

//   cerr << "FieldEdit::writeEditFieldFile" << endl;
//   for ( int i=0; i<8; i++)
//     cerr << " metnoFieldFileIdentSpec[" << i << "]=" << metnoFieldFileIdentSpec[i] << endl;
//   for ( int i=0; i<20; i++)
//     cerr << " metnoFieldFileIdent[" << i << "]=" << metnoFieldFileIdent[i] << endl;

  // all gridspec may not be in ident... use area
  Projection p= editfield->area.P();
  float gspec[Projection::speclen];
  p.Gridspecstd(gspec);
  int gtype= p.Gridtype();
  nx= editfield->nx;
  ny= editfield->ny;

  int ierror;
  int ldata = 20 + nx*ny + 100;
  short int *idata= new short int[ldata];

  for (int i=0; i<20; i++) idata[i]= metnoFieldFileIdent[i];

  gridpar(-1,ldata,idata,&gtype,&nx,&ny,gspec,&ierror);
  if (ierror) {
    cerr << "FieldEdit::writeEditFieldFile GRIDPAR ERROR" << endl;
    delete[] idata;
    return false;
  }

  // always creating a DNMI field file when writing the first time
  // (avoiding misc. testing of possibly existing file and contents...
  //  the file is meant to be in a portable transfer format)

  FortranUnit funit;

  if (!funit.Lock()) {
    cerr << "No available unitnumber" << endl;
    return false;
  }

  if (filename != lastFileWritten) {

    int itype= 999;
    int ltime= 5;
    int itime[5];
    itime[0]= editfield->validFieldTime.year();
    itime[1]= editfield->validFieldTime.month();
    itime[2]= editfield->validFieldTime.day();
    itime[3]= editfield->validFieldTime.hour();
    itime[4]= editfield->validFieldTime.min();
    int icode= 0;
    int lspec= 3;
    short ispec[3];
    ispec[0]= metnoFieldFileIdent[0];
    ispec[1]= metnoFieldFileIdent[1];
    ispec[2]= 1;
    int lopt=1;
    int iopt=0;

    crefelt(filename.c_str(), funit.Num(), itype, ltime, itime,
	    icode, lspec, ispec, lopt, &iopt, &ierror);
    if (ierror) {
      cerr << "FieldEdit::writeEditFieldFile CREFELT ERROR" << endl;
      delete[] idata;
      funit.Unlock();
      return false;
    }
  }

  // use automatic (best) scaling unless discontinuous/classes
  const PlotOptions poptions= editfieldplot->getPlotOptions();
  discontinuous= poptions.discontinuous!=0;

  if (discontinuous){
    idata[19]= 0;
  } else {
    idata[19]= -32767;
  }

  int mode= 10;
  int ipack= 2;
  float fscale= 1.0;

  mwfelt(mode, filename.c_str(), funit.Num(),
	 ipack, nx*ny, editfield->data, fscale,
	 ldata, idata, &ierror);
  if (ierror) {
    mode= 0; // now printing error message
    mwfelt(mode, filename.c_str(), funit.Num(),
	   ipack, nx*ny, editfield->data, fscale,
	   ldata, idata, &ierror);
    if (ierror) {
      cerr << "FieldEdit::writeEditFieldFile MWFELT ERROR above" << endl;
      delete[] idata;
      funit.Unlock();
      return false;
    }
  }

  funit.Unlock();

  int n= undofields.size();
  for (int i=0; i<n; i++) {
    delete[] undofields[i].data[0];
    delete[] undofields[i].data[1];
  }
  undofields.clear();
  numundo=0;

  if (returndata) {
    *fdata= idata;
    // correct used datalength (gridpar/mwfelt)
    int lgeom= 0;
    int gtype= idata[8];
    if (gtype>1000) lgeom= gtype%1000;
    fdatalength= 20 + nx*ny + lgeom;
  } else {
    delete[] idata;
  }

  lastFileWritten= filename;

  cerr<<"Wrote field to file "<<filename<<endl;

  return true;
}


void FieldEdit::activate() {

  operationStarted= false;

  if (!editfield) return;
  if (!editfieldplot) return;

  if (!editStarted) {

    // set a small default influence size
    Rectangle fr= splot.getPlotSize();
    float pwidth,pheight;
    splot.getPhysSize(pwidth,pheight);
    float d= 20. * (fr.x2-fr.x1)/pwidth;

    def_rcircle=   d;
    def_axellipse= d;
    def_ayellipse= 0.;

    editStarted= true;
  }

  influencetype= def_influencetype;
  rcircle=       def_rcircle;
  axellipse=     def_axellipse;
  ayellipse=     def_ayellipse;
  ecellipse=     def_ecellipse;
  rcirclePlot=   rcircle;
  axellipsePlot= axellipse;
  ayellipsePlot= ayellipse;

  convertpos= false;
  justDoneUndoRedo= false;

  if (!workfield) makeWorkfield();

  active= true;
}


bool FieldEdit::notifyEditEvent(const EditEvent& ee)
{
  //
  // Receive an editevent
  // old and new point, editshape, edittype and eventorder
  //
  // See diDrawingTypes.h for structs and enum-definitions
  //
  // Return true if repaint (of overlay) is needed
  //
#ifdef DEBUGREDRAW
  cerr<<"FieldEdit::notifyEditEvent  type,order: "
      <<ee.type<<" "<<ee.order<<endl;
#endif

  // do not exit if only setup/default information from dialog
  bool existing= true;
  if (!editfield) existing= false;
  else if (!editfield->data) existing= false;

  if (!workfield) existing= false;

  bool change= false;
  bool repaint= true;

  bool haveDoneUndoRedo= justDoneUndoRedo;

  if (justDoneUndoRedo) {
    setFieldInfluence(currentInfluence, false);
    justDoneUndoRedo= false;
  }

  if (ee.type!=edit_pos) {

    int uindex= -1;
    int pindex= -1;

    if (ee.type==edit_size) {

      // right mouse button pressed
      if (ee.order==start_event) {
        posx= xfirst= ee.x;
        posy= yfirst= ee.y;
	influencetype= def_influencetype;
	ecellipse=     def_ecellipse;
      } else {
        float dx= ee.x - xfirst;
        float dy= ee.y - yfirst;
        rcirclePlot=   def_rcircle=   sqrtf(dx*dx+dy*dy);
        axellipsePlot= def_axellipse= dx;
        ayellipsePlot= def_ayellipse= dy;
      }

    } else if (ee.type==edit_inspection && existing) {

      // middle mouse button pressed
      float gx= ee.x;
      float gy= ee.y;
      maparea = splot.getMapArea();
      if (maparea.P()!=editfield->area.P()) {
        int npos= 1;
        if (!gc.getPoints(maparea,editfield->area,npos,&gx,&gy)) {
	  cerr << "EDIT: getPoints error" << endl;
	  return false;
        }
      }
      float fv;
      int interpoltype= 0;
      if (!editfield->interpolate(1,&gx,&gy,&fv,interpoltype))
	fv=fieldUndef;
      if (fv!=fieldUndef) {
//####################################################
        const PlotOptions poptions= editfieldplot->getPlotOptions();
        discontinuous= poptions.discontinuous!=0;
//####################################################
        nx= editfield->nx;
        ny= editfield->ny;
        int nsmooth=0;
	isoline= findIsoLine(gx,gy,fv,nsmooth,nx,ny,workfield->data,discontinuous);
	if (!isoline.x.size()) {
	  repaint= false;
        } else {
          drawIsoline= true;
        }
      }

    } else if (ee.type==edit_circle) {

      def_influencetype= influencetype= 0;

    } else if (ee.type==edit_square) {

      def_influencetype= influencetype= 3;

    } else if (ee.type==edit_ellipse1) {

      def_influencetype= influencetype= 1;

    } else if (ee.type==edit_ellipse2) {

      def_influencetype= influencetype= 2;

    } else if (ee.type==edit_ecellipse) {

      def_ecellipse= ecellipse= ee.x;

    } else if (ee.type==edit_exline_on) {

      drawExtraLines= true;
      repaint= false;

    } else if (ee.type==edit_exline_off) {

      drawExtraLines= false;
      repaint= false;

    } else if (ee.type==edit_value        ||
    	       ee.type==edit_move         ||
	       ee.type==edit_gradient     ||
               ee.type==edit_line         ||
               ee.type==edit_line_smooth  ||
               ee.type==edit_line_limited ||
               ee.type==edit_line_limited_smooth ||
               ee.type==edit_smooth) {

      if (def_influencetype==3)
        def_influencetype= influencetype= 0;  // as in dialog ??????

      editstate= ee.type;

    } else if (ee.type==edit_replace_undef) {

//    if (def_influencetype==3)
//      def_influencetype= influencetype= 0;  // as in dialog ??????

      editstate= ee.type;

    } else if (ee.type==edit_class_line || ee.type==edit_class_copy ||
	       ee.type==edit_class_value) {

//    if (def_influencetype==1 || def_influencetype==2)
//      def_influencetype= influencetype= 0;  // as in dialog ??????

      editstate= ee.type;
      if (ee.type==edit_class_value) brushValue= ee.x;
      brushReplaceUndef= false;

//  } else if (ee.type==edit_number_change || ee.type==edit_number_fixed ||
//	       ee.type==edit_number_undef) {
    } else if (ee.type==edit_set_undef) {

//   if (def_influencetype==1 || def_influencetype==2)
//      def_influencetype= influencetype= 0;  // as in dialog ??????

      editstate= ee.type;

    } else if (ee.type==edit_show_numbers_on) {

      showNumbers= true;
      repaint= false;

    } else if (ee.type==edit_show_numbers_off) {

      showNumbers= false;
      repaint= false;

    } else if (!existing) {

      repaint= false;

    } else if (ee.type==edit_undo) {

      if (numundo>0) {
        uindex= --numundo;
        pindex= 0;
      }
      // return true if there are more undo cases left
      if (numundo==0) repaint= false;

    } else if (ee.type==edit_redo) {

      if (numundo<undofields.size()) {
        uindex= numundo++;
        pindex= 1;
      }
      // return true if there are more redo cases left
      if (numundo==undofields.size()) repaint= false;

    } else if (ee.type==edit_lock_value) {

      lockedValue.insert(ee.x);

      // due to current EditDialog...
      editstate= edit_class_value;
      brushValue= ee.x;
      brushReplaceUndef= false;

    } else if (ee.type==edit_open_value) {

      lockedValue.erase(ee.x);

      // due to current EditDialog...
      editstate= edit_class_value;
      brushValue= ee.x;
      brushReplaceUndef= false;

    }

    if (uindex>=0 && pindex>=0) {
      // undo (pindex=0) or redo (pindex=1)
      int u= uindex;
      int p= pindex;
      int i,j, n=0;
      if (workfield==editfield ||
          undofields[u].editstate==edit_replace_undef ||
          undofields[u].editstate==edit_set_undef) {
        for (j=undofields[u].j1; j<undofields[u].j2; ++j)
          for (i=undofields[u].i1; i<undofields[u].i2; ++i)
	    editfield->data[j*nx+i]=undofields[u].data[p][n++];
      } else {
        for (j=undofields[u].j1; j<undofields[u].j2; ++j) {
          for (i=undofields[u].i1; i<undofields[u].i2; ++i) {
	    if (editfield->data[j*nx+i]!=fieldUndef)
	      editfield->data[j*nx+i]=undofields[u].data[p][n];
	    workfield->data[j*nx+i]=undofields[u].data[p][n++];
	  }
	}
      }

      // save influence as shown in dialog
      if (!haveDoneUndoRedo) currentInfluence= getFieldInfluence(false);
      // set cursor pos and influence
      if (ee.type==edit_redo) setFieldInfluence(undofields[u].influence, true);
      else if (u>0)           setFieldInfluence(undofields[u-1].influence, true);
      else                    setFieldInfluence(firstInfluence, true);
      justDoneUndoRedo= true;
    }

  } else if (editStarted) {      // edit_pos event

    if ((editstate!=edit_value && editstate!=edit_gradient) || ee.order==start_event) {
      posx= ee.x;
      posy= ee.y;
    }

    float gx= ee.x;
    float gy= ee.y;

    if (convertpos) {
      int npos= 1;
      if (!gc.getPoints(maparea,editfield->area,npos,&gx,&gy)) {
        cerr << "EDIT: getPoints error" << endl;
        return false;
      }
    }

    if (ee.order==start_event) {

      if (numundo==0) firstInfluence= getFieldInfluence(true);

      const PlotOptions poptions= editfieldplot->getPlotOptions();
      lineinterval= poptions.lineinterval;
      discontinuous= poptions.discontinuous!=0;

      if (discontinuous && workfield!=editfield) {
        // could not prevent making workfield earlier (?)
        delete workfield;
        workfield= editfield;
        minValue= maxValue= fieldUndef;
      }

      // tar vare paa originalverdier
      nx= editfield->nx;
      ny= editfield->ny;
      fsize= nx*ny;
      if (!odata) odata= new float[fsize];

      if (editstate==edit_replace_undef ||
	  editstate==edit_set_undef) {
        for (int i=0; i<fsize; ++i) odata[i]= editfield->data[i];
      } else {
        for (int i=0; i<fsize; ++i) odata[i]= workfield->data[i];
      }

      i1ed = nx;  i2ed = -1;
      j1ed = ny;  j2ed = -1;
      i1edp = nx;  i2edp = -1;
      j1edp = ny;  j2edp = -1;
      numUndefReplaced= 0;

      // NOTE: values are on the map (not converted to the field)
      influencetype= def_influencetype;
      rcircle=       def_rcircle;
      axellipse=     def_axellipse;
      ayellipse=     def_ayellipse;
      ecellipse=     def_ecellipse;
      // keep map coordinates for plotting (may be changed later)
      rcirclePlot=   rcircle;
      axellipsePlot= axellipse;
      ayellipsePlot= ayellipse;
      showArrow=     false;

      maparea = splot.getMapArea();

      convertpos= (maparea.P() != editfield->area.P());

      if (convertpos) {
        float rx[3] = { gx, gx-axellipse*0.5, gx+axellipse*0.5 };
        float ry[3] = { gy, gy-ayellipse*0.5, gy+ayellipse*0.5 };
        int npos= 3;
        if (!gc.getPoints(maparea,editfield->area,npos,rx,ry)) {
	  cerr << "EDIT: getPoints error" << endl;
	  return false;
        }
        gx= rx[0];
        gy= ry[0];
        // WARNING: Convertion may cause a different influence area
        //          on the field data than shown (circle/ellipse).
        //          Problem occurring when scaling in x and y directions
        //          get very different (circle on map is not a circle
        //          in the field, ellipse ???).
        //          Cannot avoid this while showing an image.
        axellipse = rx[2]-rx[1];
        ayellipse = ry[2]-ry[1];
        rcircle= sqrtf(axellipse*axellipse+ayellipse*ayellipse);
      }

      // in field coordinates
      xfirst= xprev= gx;
      yfirst= yprev= gy;
      orcircle=   rcircle;
      oaxellipse= axellipse;
      oayellipse= ayellipse;
      oecellipse= ecellipse;

      fline= fieldUndef;

      numsmooth= 0;

      if (editstate==edit_value) {
        yrefpos= ee.y;
        Rectangle rf= maparea.R();
        // changing along the map height
//      deltascale = 4.0 * lineinterval / rf.height();
        deltascale = 2.0 * lineinterval / rf.height();
        convertpos= false;

      } else if (editstate==edit_gradient) {
        xrefpos= ee.x;
        yrefpos= ee.y;
        Rectangle rf= maparea.R();
        // changing along the map height
//      deltascale = 40.0 * lineinterval / rf.height();
        deltascale = 10.0 * lineinterval / rf.height();
        convertpos= false;

      } else if (editstate==edit_line         ||
      		 editstate==edit_line_smooth  ||
	         editstate==edit_line_limited ||
	         editstate==edit_line_limited_smooth) {
        Rectangle rec= maparea.R();
        float rx[4] = { rec.x1, rec.x2, rec.x1, rec.x2 };
        float ry[4] = { rec.y1, rec.y2, rec.y2, rec.y1 };
        if (convertpos) {
          int npos= 4;
          if (!gc.getPoints(maparea,editfield->area,npos,rx,ry)) {
	    cerr << "EDIT: getPoints error" << endl;
	    return false;
          }
        }
        float avg= (sqrtf((rx[0]-rx[1])*(rx[0]-rx[1])+(ry[0]-ry[1])*(ry[0]-ry[1]))
		   +sqrtf((rx[2]-rx[3])*(rx[2]-rx[3])+(ry[2]-ry[3])*(ry[2]-ry[3])))*0.5;
        int nsmooth= int(200./avg + 0.5);
        if (nsmooth<0) nsmooth= 0;
        if (nsmooth>8) nsmooth= 8; // ??????????????????????????
        lineincrement= 1./float(nsmooth+1);
        xline.clear();
        yline.clear();
        int interpoltype= 0;
        if (!editfield->interpolate(1,&gx,&gy,&fline,interpoltype))
	  fline=fieldUndef;
        if (fline==fieldUndef) return false;
	isoline= findIsoLine(gx,gy,fline,nsmooth,nx,ny,workfield->data,false);
	if (!isoline.x.size()) {
	  fline= fieldUndef;
	  return false;
        }
        xline.push_back(gx);
        yline.push_back(gy);
        // only circle influence, radius changing during operation
	influencetype= 0;
        if (editstate==edit_line_smooth ||
            editstate==edit_line_limited_smooth) numsmooth= 2;
	else                                     numsmooth= 0;

      } else if (editstate==edit_smooth) {
	// ellipse with focus line not used
        if (influencetype==2) influencetype= 1;
        editSmooth(gx, gy);
        change= true;

      } else if (editstate==edit_replace_undef) {
	brushReplaceUndef= true;
        editBrush(gx, gy);
        change= false;  // everything is done

      } else if (editstate==edit_class_line) {
        xline.clear();
        yline.clear();
        int interpoltype= 0; // just find a value to start the line
        if (!editfield->interpolate(1,&gx,&gy,&fline,interpoltype))
	  fline=fieldUndef;
        if (fline==fieldUndef) {
#ifdef DEBUGCLASSES
          cerr<<"fline==fieldUndef"<<endl;
#endif
          return false;
        }
#ifdef DEBUGCLASSES
        cerr<<"fline= "<<fline<<endl;
#endif
        int nsmooth= 0;
	isoline= findIsoLine(gx,gy,fline,nsmooth,nx,ny,workfield->data,true);
	if (!isoline.x.size()) {
	  fline= fieldUndef;
#ifdef DEBUGCLASSES
	  cerr<<"NO isoLine found"<<endl;
#endif
	  return false;
        }
        xline.push_back(gx);
        yline.push_back(gy);
        classLineValue= fieldUndef;  // not determined yet
	influencetype= -1;    // do not draw any influence
	numsmooth= 0;

      } else if (editstate==edit_class_copy) {
        int interpoltype= 2;
        if (!editfield->interpolate(1,&gx,&gy,&brushValue,interpoltype))
	  brushValue=fieldUndef;
        if (brushValue==fieldUndef) return false;
        editBrush(gx,gy);
        change= true;

      } else if (editstate==edit_class_value) {

        editBrush(gx,gy);
        change= true;
//
//    } else if (editstate==edit_number_change) {
//
//      return false;
//
//    } else if (editstate==edit_number_fixed) {
//
//      return false;

      } else if (editstate==edit_set_undef) {

        brushValue= fieldUndef;
        brushReplaceUndef= false;
        editBrush(gx,gy);
        change= false;    // everything is done

      }

      operationStarted= true;

    } else if (!operationStarted) {

      repaint= false;

    } else if (ee.order==stop_event) {

      isoline.x.clear();
      isoline.y.clear();
      xline.clear();
      yline.clear();
      convertpos= false;

      if (i1ed<i2ed && j1ed<j2ed) {
        if (numundo<undofields.size()) {
          // after undo (and not complete redo)
	  for (int n=numundo; n<undofields.size(); ++n) {
	    delete[] undofields[n].data[0];
	    delete[] undofields[n].data[1];
	  }
	  undofields.resize(numundo);
        }
        if (editstate!=edit_replace_undef || numUndefReplaced>0) {
          UndoField uf;
          uf.editstate= editstate;
          uf.i1= i1ed;
          uf.i2= i2ed;
          uf.j1= j1ed;
          uf.j2= j2ed;
          uf.data[0]= new float[(i2ed-i1ed)*(j2ed-j1ed)];
          uf.data[1]= new float[(i2ed-i1ed)*(j2ed-j1ed)];
          int i, j, n=0;
          for (j=j1ed; j<j2ed; ++j)
            for (i=i1ed; i<i2ed; ++i)
	      uf.data[0][n++]= odata[j*nx+i];
	  float* udata= (editstate==edit_replace_undef ||
	  		 editstate==edit_set_undef) ?
				editfield->data : workfield->data;
          n=0;
          for (j=j1ed; j<j2ed; ++j)
            for (i=i1ed; i<i2ed; ++i)
	      uf.data[1][n++]= udata[j*nx+i];
          // will show (extended) influence during undo/redo
          uf.influence= getFieldInfluence(true);
          undofields.push_back(uf);
          numundo++;

        }
      } else {
        repaint= false;
      }

      i1ed = nx;  i2ed = -1;
      j1ed = ny;  j2ed = -1;

      // reset to show standard influence
      influencetype= def_influencetype;
      rcircle=       def_rcircle;
      axellipse=     def_axellipse;
      ayellipse=     def_ayellipse;
      ecellipse=     def_ecellipse;
      rcirclePlot=   rcircle;
      axellipsePlot= axellipse;
      ayellipsePlot= ayellipse;
      showArrow=     false;

      operationStarted= false;

    } else if (editstate==edit_value) {

      float delta = (gy - yrefpos) * deltascale;

      editValue(xfirst,yfirst,delta);

      change= true;

    } else if (editstate==edit_move) {

      rcircle=   orcircle;
      axellipse= oaxellipse;
      ayellipse= oayellipse;
      ecellipse= oecellipse;

      editMove(gx,gy);

      if (orcircle!=rcircle || oaxellipse!=axellipse
			    || oayellipse!=ayellipse) {
        if (influencetype==0) {
          rcirclePlot = def_rcircle * (rcircle/orcircle);
        } else if (convertpos) {
	  // may be not the best we could do...
          float rx[2] = { gx-axellipse*0.5, gx+axellipse*0.5 };
          float ry[2] = { gy-ayellipse*0.5, gy+ayellipse*0.5 };
          int npos= 2;
          if (!gc.getPoints(editfield->area,maparea,npos,rx,ry)) {
	    cerr << "EDIT: getPoints error" << endl;
	    return false;
          }
          axellipsePlot= rx[1]-rx[0];
          ayellipsePlot= ry[1]-ry[0];
        } else {
          axellipsePlot= axellipse;
          ayellipsePlot= ayellipse;
        }
      }

      change= true;

    } else if (editstate==edit_gradient) {

      float deltax = (gx - xrefpos) * deltascale;
      float deltay = (gy - yrefpos) * deltascale;

      editGradient(xfirst,yfirst,deltax,deltay);

      showArrow= true;
      xArrow= ee.x;
      yArrow= ee.y;

      change= true;

    } else if ((editstate==edit_line         ||
    		editstate==edit_line_smooth  ||
	        editstate==edit_line_limited ||
	        editstate==edit_line_limited_smooth) && fline!=fieldUndef) {

      float dx= gx - xfirst;
      float dy= gy - yfirst;
      float ds= sqrtf(dx*dx+dy*dy);
      int   np= int(ds/lineincrement + 0.5);
      if (np<=1) {
        xline.push_back(gx);
        yline.push_back(gy);
      } else {
        dx= dx/float(np);
        dy= dy/float(np);
        for (int j=0; j<np; j++) {
	  xfirst+=dx;
	  yfirst+=dy;
          xline.push_back(xfirst);
          yline.push_back(yfirst);
        }
      }
      xfirst= gx;
      yfirst= gy;

      if (editstate==edit_line || editstate==edit_line_smooth)
        editLine();
      else
        editLimitedLine();

      rcirclePlot= def_rcircle * (rcircle/orcircle);

      // stop line(-move) if outside the field area or near undefined
      int interpoltype= 0;
      float fv;
      if (editfield->interpolate(1,&gx,&gy,&fv,interpoltype)) {
	if (fv==fieldUndef) fline=fieldUndef;
      }
      change= true;

    } else if (editstate==edit_smooth) {

      editSmooth(gx, gy);
      change= true;

    } else if (editstate==edit_replace_undef) {

      brushReplaceUndef= true;
      editBrush(gx,gy);
      change= false;  // everything is done

    } else if (editstate==edit_class_line && fline!=fieldUndef) {

      xline.push_back(gx);
      yline.push_back(gy);
      editClassLine();
      change= true;
      // stop line(-move) if outside the field area or near undefined
      int interpoltype= 2;
      float fv;
      if (editfield->interpolate(1,&gx,&gy,&fv,interpoltype)) {
	if (fv==fieldUndef) fline=fieldUndef;
      }

    } else if (editstate==edit_class_copy ||
	       editstate==edit_class_value) {

      editBrush(gx,gy);
      change= true;
//
// } else if (editstate==edit_number_change) {
//
//    return false;
//
//  } else if (editstate==edit_number_fixed) {
//
//    return false;

    } else if (editstate==edit_set_undef) {

      editBrush(gx,gy);
      change= true;

    }

    xprev= gx;
    yprev= gy;

  } else {

    repaint= false;

  }

  if (change) {

    if (minValue!=fieldUndef) {
      int i,j,ij;
      for (j=j1ed; j<j2ed; j++) {
        for (i=i1ed; i<i2ed; i++) {
	  ij= j*nx+i;
	  if (workfield->data[ij]<minValue)
	    workfield->data[ij]= minValue;
        }
      }
    }
    if (maxValue!=fieldUndef) {
      int i,j,ij;
      for (j=j1ed; j<j2ed; j++) {
        for (i=i1ed; i<i2ed; i++) {
	  ij= j*nx+i;
	  if (workfield->data[ij]>maxValue)
	    workfield->data[ij]= maxValue;
        }
      }
    }
    if (lockedValue.size()>0 && odata) {
      set<float>::iterator pend= lockedValue.end();
      int i,j,ij;
      for (j=j1ed; j<j2ed; j++) {
        for (i=i1ed; i<i2ed; i++) {
	  ij= j*nx+i;
	  if (workfield->data[ij]!=odata[ij]) {
	    if (lockedValue.find(odata[ij])!=pend)
	      workfield->data[ij]= odata[ij];
	  }
        }
      }
    }
    if (workfield!=editfield) {
      if (i1edp>i1ed) i1edp=i1ed;
      if (i2edp<i2ed) i2edp=i2ed;
      if (j1edp>j1ed) j1edp=j1ed;
      if (j2edp<j2ed) j2edp=j2ed;
      int i,j,ij;
      for (j=j1edp; j<j2edp; j++) {
        for (i=i1edp; i<i2edp; i++) {
	  ij= j*nx+i;
	  if (editfield->data[ij]!=fieldUndef)
	    editfield->data[ij]= workfield->data[ij];
        }
      }
      i1edp= i1ed;
      i2edp= i2ed;
      j1edp= j1ed;
      j2edp= j2ed;
    }

  }

  return repaint;
}


void FieldEdit::setFieldInfluence(const FieldInfluence& fi,
				  bool geo) {

  float rx[2] = { fi.posx, fi.posx + fi.axellipse };
  float ry[2] = { fi.posy, fi.posy + fi.ayellipse };
  if (geo) {
    int npos= 2;
    Area maparea = splot.getMapArea();
    gc.geo2xy(maparea,npos,rx,ry);
  }
  float dx= rx[1] - rx[0];
  float dy= ry[1] - ry[0];

  posx= rx[0];
  posy= ry[0];
  def_influencetype= fi.influencetype;
  def_rcircle=       sqrtf(dx*dx+dy*dy);
  def_axellipse=     dx;
  def_ayellipse=     dy;
  def_ecellipse=     fi.ecellipse;

  influencetype=                def_influencetype;
  rcircle=       rcirclePlot=   def_rcircle;
  axellipse=     axellipsePlot= def_axellipse;
  ayellipse=     ayellipsePlot= def_ayellipse;
  ecellipse=                    def_ecellipse;
}


FieldInfluence FieldEdit::getFieldInfluence(bool geo) {

  FieldInfluence fi;
  float dx,dy;

//if (influencetype==0 && rcirclePlot!=def_rcircle) {
    float scale= rcirclePlot / def_rcircle;
    dx= scale * def_axellipse;
    dy= scale * def_ayellipse;
//} else {
//  dx= axellipsePlot;
//  dy= ayellipsePlot;
//}

  float rx[3]= { posx, posx - dx*0.5, posx + dx*0.5 };
  float ry[3]= { posy, posy - dy*0.5, posy + dy*0.5 };
  if (geo) {
    int npos=3;
    Area maparea = splot.getMapArea();
    gc.xy2geo(maparea,npos,rx,ry);
  }
  dx= rx[2]-rx[1];
  dy= ry[2]-ry[1];

  fi.posx= rx[0];
  fi.posy= ry[0];
  fi.influencetype= influencetype;
  fi.rcircle=       sqrtf(dx*dx+dy*dy);
  fi.axellipse=     dx;
  fi.ayellipse=     dy;
  fi.ecellipse=     ecellipse;

  return fi;
}


void FieldEdit::editValue(float px, float py, float delta) {

  int   n, i, j, i1, i2, j1, j2, ij;
  float r;

  if (influencetype==0) r= rcircle;
  else                  r= sqrtf(axellipse*axellipse+ayellipse*ayellipse);

  i1= int(px-r+1.);  if (i1<0)  i1=0;   i1ed=i1;
  i2= int(px+r+1.);  if (i2>nx) i2=nx;  i2ed=i2;
  j1= int(py-r+1.);  if (j1<0)  j1=0;   j1ed=j1;
  j2= int(py+r+1.);  if (j2>ny) j2=ny;  j2ed=j2;

  if (i1>=i2 || j1>=j2) {
    i1ed = nx;  i2ed = -1;
    j1ed = ny;  j2ed = -1;
    return;
  }

  float *weight = new float[(i2-i1)*(j2-j1)];

  editWeight(px, py, i1, i2, j1, j2, weight);

  n=0;

  for (j=j1; j<j2; ++j) {
    ij= j*nx+i1-1;
    for (i=i1; i<i2; ++i) {
      ij++;
      workfield->data[ij]= odata[ij] + delta*weight[n++];
    }
  }

  delete[] weight;
}


void FieldEdit::editMove(float px, float py) {

  int   n,  m, i, j, i1, i2, j1, j2;
  float r, rmin, dxmv, dymv, dmv, wmin;

  if (influencetype==0) r= rcircle;
  else                  r= sqrtf(axellipse*axellipse+ayellipse*ayellipse);

  dxmv = xfirst - px;
  dymv = yfirst - py;
  dmv  = sqrtf(dxmv*dxmv+dymv*dymv);
  rmin = dmv * 2.5;  // ...............hmmmm..........
  if (r<rmin) {
    r=rmin;
    if (influencetype==0) rcircle=r;
  }
  if (influencetype!=0) {
    axellipse= r*dxmv/dmv; // changes the rotation !!!!! ....hmmmm....
    ayellipse= r*dymv/dmv;
  }

  // reset previous move
  for (j=j1ed; j<j2ed; ++j)
    for (i=i1ed; i<i2ed; ++i)
      workfield->data[j*nx+i]= odata[j*nx+i];

  i1= int(px-r+1.);  if (i1<0)  i1=0;   i1ed=i1;
  i2= int(px+r+1.);  if (i2>nx) i2=nx;  i2ed=i2;
  j1= int(py-r+1.);  if (j1<0)  j1=0;   j1ed=j1;
  j2= int(py+r+1.);  if (j2>ny) j2=ny;  j2ed=j2;

  if (i1>=i2 || j1>=j2) {
    i1ed = nx;  i2ed = -1;
    j1ed = ny;  j2ed = -1;
    return;
  }

  int npos= (i2-i1)*(j2-j1);
  float *weight = new float[npos];
  float *xpos   = new float[npos];
  float *ypos   = new float[npos];
  float *zpos   = new float[npos];

  // computing weight
  editWeight(px, py, i1, i2, j1, j2, weight);

  wmin= 0.000001;

  int nposw=0;
  n=-1;
  for (j=j1; j<j2; ++j) {
    for (i=i1; i<i2; ++i) {
      n++;
      if (weight[n]>wmin) {
	xpos[nposw]=float(i)+dxmv*weight[n];
	ypos[nposw]=float(j)+dymv*weight[n];
	nposw++;
      }
    }
  }

  // bessel (4x4 points) interpolation and extrapolation at boundaries
  int interpoltype=101;
//#####################################################
#ifdef TESTICECONC
  interpoltype=100;
#endif
//#####################################################
  if (workfield->interpolate(nposw,xpos,ypos,zpos,interpoltype)) {
    m=0;
    n=0;
    for (j=j1; j<j2; ++j)
      for (i=i1; i<i2; ++i)
        if (weight[n++]>wmin) workfield->data[j*nx+i]= zpos[m++];
  }

  delete[] weight;
  delete[] xpos;
  delete[] ypos;
  delete[] zpos;
}


void FieldEdit::editGradient(float px, float py, float deltax, float deltay) {

  int   n, i, j, i1, i2, j1, j2, ij;
  float r;

  if (influencetype==0) r= rcircle;
  else                  r= sqrtf(axellipse*axellipse+ayellipse*ayellipse);

  i1= int(px-r+1.);  if (i1<0)  i1=0;   i1ed=i1;
  i2= int(px+r+1.);  if (i2>nx) i2=nx;  i2ed=i2;
  j1= int(py-r+1.);  if (j1<0)  j1=0;   j1ed=j1;
  j2= int(py+r+1.);  if (j2>ny) j2=ny;  j2ed=j2;

  if (i1>=i2 || j1>=j2) {
    i1ed = nx;  i2ed = -1;
    j1ed = ny;  j2ed = -1;
    return;
  }

  float *weight = new float[(i2-i1)*(j2-j1)];

  editExpWeight(px, py, i1, i2, j1, j2, weight, true);

  n=0;

  for (j=j1; j<j2; ++j) {
    ij= j*nx+i1-1;
    for (i=i1; i<i2; ++i) {
      ij++;
      workfield->data[ij]= odata[ij] - (deltax*(float(j)-py)
				       -deltay*(float(i)-px))*weight[n++];
    }
  }

  delete[] weight;
}


void FieldEdit::editLine()
{
  int n, i, j, ij, i1, i2, j1, j2, k;
  float r, px, py, dx, dy, dxmv, dymv, dmv;

  int npos= xline.size();

  if (npos<3) return;

  int npline= isoline.x.size();
  if (npline<2) return;

  // reset previous operation before interpolation !!!!!
  for (j=j1ed; j<j2ed; ++j)
    for (i=i1ed; i<i2ed; ++i)
      workfield->data[j*nx+i]= odata[j*nx+i];

  float *xpos = new float[npos];
  float *ypos = new float[npos];

  for (i=0; i<npos; ++i) {
    xpos[i]= xline[i];
    ypos[i]= yline[i];
  }

  float *hxpos = new float[npos];
  float *hypos = new float[npos];
  hxpos[0]     = xpos[0];
  hypos[0]     = ypos[0];
  hxpos[npos-1]= xpos[npos-1];
  hypos[npos-1]= ypos[npos-1];
  int nmax=4;
  for (n=0; n<nmax; ++n) {
    for (i=1; i<npos-1; ++i) {
      hxpos[i]= (xpos[i-1]+xpos[i]+xpos[i+1])*0.333333333;
      hypos[i]= (ypos[i-1]+ypos[i]+ypos[i+1])*0.333333333;
    }
    for (i=1; i<npos-1; ++i) {
      xpos[i]= (hxpos[i-1]+hxpos[i]+hxpos[i+1])*0.333333333;
      ypos[i]= (hypos[i-1]+hypos[i]+hypos[i+1])*0.333333333;
    }
  }
  delete[] hxpos;
  delete[] hypos;

  const float wmin= 0.00001;

  float d2,d2min;
  float d2max= (float(nx)*float(nx)+float(ny)*float(ny))*2.;

  float *weightmax= new float[nx*ny];
  float *dxmax=     new float[nx*ny];
  float *dymax=     new float[nx*ny];
  float *weight=    new float[nx*ny];

  for (i=0; i<nx*ny; i++) weightmax[i]= 0.0;

  for (n=0; n<npos; n++) {

    px= xpos[n];
    py= ypos[n];
    d2min= d2max;
    k= 0;
    for (i=0; i<npline; i++) {
      dx= isoline.x[i] - px;
      dy= isoline.y[i] - py;
      d2= dx*dx+dy*dy;
      if (d2<d2min) {
	d2min= d2;
	k= i;
      }
    }
    dxmv= isoline.x[k] - px;
    dymv= isoline.y[k] - py;
    dmv= sqrtf(dxmv*dxmv+dymv*dymv);
    r  = dmv * 2.5;  // ...............hmmmm..........

    i1= int(px-r+1.);  if (i1<0)  i1=0;
    i2= int(px+r+1.);  if (i2>nx) i2=nx;
    j1= int(py-r+1.);  if (j1<0)  j1=0;
    j2= int(py+r+1.);  if (j2>ny) j2=ny;

    if (i1ed>i1) i1ed=i1;
    if (i2ed<i2) i2ed=i2;
    if (j1ed>j1) j1ed=j1;
    if (j2ed<j2) j2ed=j2;

    rcircle= r;

    // computing weight
    editWeight(px, py, i1, i2, j1, j2, weight);

    int nposw= -1;

    for (j=j1; j<j2; ++j) {
      for (i=i1; i<i2; ++i) {
        nposw++;
	ij= j*nx+i;
        if (weight[nposw]>weightmax[ij] && weight[nposw]>wmin) {
	  weightmax[ij]= weight[nposw];
	  dxmax[ij]=dxmv;
	  dymax[ij]=dymv;
        }
      }
    }
  }

  i1= i1ed;
  i2= i2ed;
  j1= j1ed;
  j2= j2ed;

  int nposw= (i2-i1)*(j2-j1);
  float *xposw  = new float[nposw];
  float *yposw  = new float[nposw];
  float *zposw  = new float[nposw];
  nposw=0;

  for (j=j1; j<j2; ++j) {
    for (i=i1; i<i2; ++i) {
      ij= j*nx+i;
      if (weightmax[ij]>wmin) {
	xposw[nposw]=float(i)+dxmax[ij]*weightmax[ij];
	yposw[nposw]=float(j)+dymax[ij]*weightmax[ij];
	nposw++;
      }
    }
  }

  // bessel (4x4 points) interpolation and extrapolation at boundaries
  int interpoltype=101;
//#####################################################
#ifdef TESTICECONC
  interpoltype=100;
#endif
//#####################################################
  if (workfield->interpolate(nposw,xposw,yposw,zposw,interpoltype)) {
    nposw=0;
    for (j=j1; j<j2; ++j) {
      for (i=i1; i<i2; ++i) {
        ij= j*nx+i;
        if (weightmax[ij]>wmin) workfield->data[ij]= zposw[nposw++];
      }
    }
  }

  delete[] xposw;
  delete[] yposw;
  delete[] zposw;
  delete[] xpos;
  delete[] ypos;

  if (numsmooth>0) {
    // smooth the result, only part of field changed
    // (reusing allocated arrays from above)
    if (i1>0)  i1--;
    if (i2<nx) i2++;
    if (j1>0)  j1--;
    if (j2<ny) j2++;
    n= -1;
    for (j=j1; j<j2; ++j) {
      for (i=i1; i<i2; ++i) {
	n++;
	ij= j*nx+i;
	dxmax[n]= workfield->data[ij];
	if (weightmax[ij]>wmin) dymax[n]= 1.;
	else                    dymax[n]= 0.;
      }
    }

    int iudef= 0;
    int nsmoothf=-numsmooth;
    int mx= i2-i1;
    int my= j2-j1;
    quicksmooth(nsmoothf, iudef, mx, my, dxmax, dymax, weightmax, weight);

    n= 0;
    for (j=j1; j<j2; ++j)
      for (i=i1; i<i2; ++i) workfield->data[j*nx+i]= dxmax[n++];
  }

  delete[] weightmax;
  delete[] dxmax;
  delete[] dymax;
  delete[] weight;
}


void FieldEdit::editLimitedLine() {

  int n, i, j, ij, i1, i2, j1, j2, k;
  float r, px, py, dx, dy, dxmv, dymv, dmv;

  int npos= xline.size();

  if (npos<3) return;

  int npline= isoline.x.size();
  if (npline<2) return;

  // reset previous operation before interpolation !!!!!
  for (j=j1ed; j<j2ed; ++j)
    for (i=i1ed; i<i2ed; ++i)
      workfield->data[j*nx+i]= odata[j*nx+i];

  float *xpos = new float[npos];
  float *ypos = new float[npos];
  float *zpos = new float[npos];

  for (i=0; i<npos; ++i) {
    xpos[i]= xline[i];
    ypos[i]= yline[i];
  }

  float *hxpos = new float[npos];
  float *hypos = new float[npos];
  hxpos[0]     = xpos[0];
  hypos[0]     = ypos[0];
  hxpos[npos-1]= xpos[npos-1];
  hypos[npos-1]= ypos[npos-1];
  int nmax=4;
  for (n=0; n<nmax; ++n) {
    for (i=1; i<npos-1; ++i) {
      hxpos[i]= (xpos[i-1]+xpos[i]+xpos[i+1])*0.333333333;
      hypos[i]= (ypos[i-1]+ypos[i]+ypos[i+1])*0.333333333;
    }
    for (i=1; i<npos-1; ++i) {
      xpos[i]= (hxpos[i-1]+hxpos[i]+hxpos[i+1])*0.333333333;
      ypos[i]= (hypos[i-1]+hypos[i]+hypos[i+1])*0.333333333;
    }
  }
  delete[] hxpos;
  delete[] hypos;

  int interpoltyp=101;
  if (!workfield->interpolate(npos,xpos,ypos,zpos,interpoltyp)) {
    delete[] xpos;
    delete[] ypos;
    delete[] zpos;
    return;
  }

  const float wmin= 0.00001;

  float d2,d2min,pz,w,wf,f;
  float d2max= (float(nx)*float(nx)+float(ny)*float(ny))*2.;

  float flow=  isoline.value - lineinterval * 0.95;
  float fhigh= isoline.value + lineinterval * 0.95;

  float *weightmax= new float[nx*ny];
  float *dxmax=     new float[nx*ny];
  float *dymax=     new float[nx*ny];
  float *weight=    new float[nx*ny];

  for (i=0; i<nx*ny; i++) weightmax[i]= 0.0;

  for (n=0; n<npos; n++) {

    px= xpos[n];
    py= ypos[n];
    d2min= d2max;
    k= 0;
    for (i=0; i<npline; i++) {
      dx= isoline.x[i] - px;
      dy= isoline.y[i] - py;
      d2= dx*dx+dy*dy;
      if (d2<d2min) {
	d2min= d2;
	k= i;
      }
    }
    dxmv= isoline.x[k] - px;
    dymv= isoline.y[k] - py;
    dmv= sqrtf(dxmv*dxmv+dymv*dymv);
    r  = dmv * 2.5;  // ...............hmmmm..........

    i1= int(px-r+1.);  if (i1<0)  i1=0;
    i2= int(px+r+1.);  if (i2>nx) i2=nx;
    j1= int(py-r+1.);  if (j1<0)  j1=0;
    j2= int(py+r+1.);  if (j2>ny) j2=ny;

    if (i1ed>i1) i1ed=i1;
    if (i2ed<i2) i2ed=i2;
    if (j1ed>j1) j1ed=j1;
    if (j2ed<j2) j2ed=j2;

    rcircle= r;

    // computing weight
    editWeight(px, py, i1, i2, j1, j2, weight);

    pz= zpos[n];

    int nposw= -1;

    for (j=j1; j<j2; ++j) {
      for (i=i1; i<i2; ++i) {
        nposw++;
	ij= j*nx+i;
	w= weight[nposw];
	f= workfield->data[ij];
	if (f>flow && f<fhigh) {
	  if (f<pz) wf= (f-flow)/(pz-flow);
	  else      wf= (fhigh-f)/(fhigh-pz);
	  if (w>wf) w=wf;
          if (w>weightmax[ij] && w>wmin) {
	    weightmax[ij]= w;
	    dxmax[ij]=dxmv;
	    dymax[ij]=dymv;
          }
	}
      }
    }
  }

  i1= i1ed;
  i2= i2ed;
  j1= j1ed;
  j2= j2ed;

  int nposw= (i2-i1)*(j2-j1);
  float *xposw  = new float[nposw];
  float *yposw  = new float[nposw];
  float *zposw  = new float[nposw];
  nposw=0;

  for (j=j1; j<j2; ++j) {
    for (i=i1; i<i2; ++i) {
      ij= j*nx+i;
      if (weightmax[ij]>wmin) {
	xposw[nposw]=float(i)+dxmax[ij]*weightmax[ij];
	yposw[nposw]=float(j)+dymax[ij]*weightmax[ij];
	nposw++;
      }
    }
  }

  // bessel (4x4 points) interpolation and extrapolation at boundaries
  int interpoltype=101;
//#####################################################
#ifdef TESTICECONC
  interpoltype=100;
#endif
//#####################################################
  if (workfield->interpolate(nposw,xposw,yposw,zposw,interpoltype)) {
    nposw= -1;
    for (j=j1; j<j2; ++j) {
      for (i=i1; i<i2; ++i) {
        ij= j*nx+i;
        if (weightmax[ij]>wmin) {
	  nposw++;
	  if (zposw[nposw]>flow && zposw[nposw]<fhigh)
	    workfield->data[ij]= zposw[nposw];
	}
      }
    }
  }

  delete[] xposw;
  delete[] yposw;
  delete[] zposw;
  delete[] xpos;
  delete[] ypos;
  delete[] zpos;

  if (numsmooth>0) {
    // smooth the result, only part of field changed
    // (reusing allocated arrays from above)
    if (i1>0)  i1--;
    if (i2<nx) i2++;
    if (j1>0)  j1--;
    if (j2<ny) j2++;
    n= -1;
    for (j=j1; j<j2; ++j) {
      for (i=i1; i<i2; ++i) {
	n++;
	ij= j*nx+i;
	dxmax[n]= workfield->data[ij];
	if (weightmax[ij]>wmin) dymax[n]= 1.;
	else                    dymax[n]= 0.;
      }
    }

    int iudef= 0;
    int nsmoothf=-numsmooth;
    int mx= i2-i1;
    int my= j2-j1;
    quicksmooth(nsmoothf, iudef, mx, my, dxmax, dymax, weightmax, weight);

    n= 0;
    for (j=j1; j<j2; ++j)
      for (i=i1; i<i2; ++i) workfield->data[j*nx+i]= dxmax[n++];
  }

  delete[] weightmax;
  delete[] dxmax;
  delete[] dymax;
  delete[] weight;
}


IsoLine FieldEdit::findIsoLine(float xpos, float ypos, float value,
			       int nsmooth, int nx, int ny, float *z,
			       bool drawBorders) {
  //
  // Method as in plotting function "contour_line"
  //
  // should maybe know the line-smoothing used when plotting ?????????
  //

  IsoLine isoline;

  const float d2max= float(nx)*float(nx)+float(ny)*float(ny);

  int i,j,ij,ijp,ijstart,k,kk,kcmin,m,n,numend,ic;
  float frac,dx,dy,d2min,xh,yh;
  int iabs[5],iab[5];
  bool search,closed,ended;
  int iabeq= 1;
  if (value<0.) iabeq= -1;

  i= int(xpos);
  j= int(ypos);
  ijstart= j*nx+i;

  const int markused[4][2]= { 0,0, 1,1, nx,0, 0,1 };
  const int ijadd[5]= { 0, 1, nx+1, nx, 0 };
  const int ijnext[4]= { -nx, 1, +nx, -1 };
  const int  knext[4]= {   2, 3,   0,  1 };

  const float dxside[4]= { 0., 1.,  1.,  0. };
  const float dyside[4]= { 0., 0.,  1.,  1. };
  const float dxfrac[4]= { 1., 0., -1.,  0. };
  const float dyfrac[4]= { 0., 1.,  0., -1. };

  for (k=0; k<4; k++) {
    ijp= ijstart+ijadd[k];
    if      (z[ijp]==fieldUndef) iabs[k]= 99;
    else if (z[ijp]>value)       iabs[k]= +1;
    else if (z[ijp]<value)       iabs[k]= -1;
    else                         iabs[k]= iabeq;
  }

  iabs[4]= iabs[0];

  int kc[4],kcstart[4];
  int nc,   ncstart= 0;

  for (k=0; k<4; k++)
    if (iabs[k]+iabs[k+1]==0) kcstart[ncstart++]= k;

  // ncstart==0: should not happen (unless bad start pos and/or value...)
  // ncstart==1: may occur due to undefined points
  // ncstart==2: the normal (one line crossing the start square)
  // ncstart==3: impossible
  // ncstart==4: saddle point (two lines crossing the start square)

  if (ncstart<1 || ncstart>2) return isoline;

  const int nbitwd= sizeof(int)*8;

  int bit[nbitwd];
  int liused= (2*nx*ny+nbitwd-1)/nbitwd;
  int *iused= new int[liused];
  for (i=0; i<liused; ++i) iused[i]= 0;
  for (i=0; i<nbitwd; ++i) bit[i] = 1 << i;
  int ibit,iwrd;

  kk= kcstart[0];
  ij= ijstart;

  float zleft=  z[ij+ijadd[kk]];
  float zright= z[ij+ijadd[kk+1]];
//###############################################################################
#ifdef DEBUGCLASSES
  if (drawBorders) cerr<<"findIsoLine  zleft,zright: "<<zleft<<" "<<zright<<endl;
#endif
//###############################################################################

  vector<float> xtmp;
  vector<float> ytmp;

  // exit pos from start square
  if (!drawBorders)
    frac= (value             - z[ij+ijadd[kk]]) /
          (z[ij+ijadd[kk+1]] - z[ij+ijadd[kk]]);
  else
    frac= 0.5f;
  i= ij%nx;
  j= ij/nx;
  xtmp.push_back(float(i) + dxside[kk] + frac*dxfrac[kk]);
  ytmp.push_back(float(j) + dyside[kk] + frac*dyfrac[kk]);
  // mark side as used
  ibit= (ij+markused[kk][0])*2 + markused[kk][1];
  iwrd= ibit/nbitwd;
  ibit%=nbitwd;
  iused[iwrd] = iused[iwrd] | bit[ibit];

  for (k=0; k<4; k++) iab[k]= iabs[k];

  numend= 0;
  closed= false;
  search= true;

  while (search) {

    ended= false;

    // next square to search for an exit from
    if (kk>=0) {
      ij= ij + ijnext[kk];
      kk= knext[kk];
    } else {
      ij= ijstart;
    }
    i= ij%nx;
    j= ij/nx;

    if (i>=0 && i<nx-1 && j>=0 && j<ny-1) {

      if (kk==0) {
        iab[0]= iab[3];
        iab[1]= iab[2];
        iab[2]= 0;
        iab[3]= 0;
      } else if (kk==1) {
        iab[1]= iab[0];
        iab[2]= iab[3];
        iab[0]= 0;
        iab[3]= 0;
      } else if (kk==2) {
        iab[2]= iab[1];
        iab[3]= iab[0];
        iab[0]= 0;
        iab[1]= 0;
      } else if (kk==3) {
        iab[0]= iab[1];
        iab[3]= iab[2];
        iab[1]= 0;
        iab[2]= 0;
      } else if (kk<0) {
	for (k=0; k<4; k++) iab[k]= iabs[k];
      }

      for (k=0; k<4; k++) {
	if (iab[k]==0) {
          ijp= ij+ijadd[k];
          if      (z[ijp]==fieldUndef) iab[k]= 99;
          else if (z[ijp]>value)       iab[k]= +1;
          else if (z[ijp]<value)       iab[k]= -1;
          else                         iab[k]= iabeq;
        }
      }
      iab[4]= iab[0];

      nc= 0;
      for (k=0; k<4; k++) {
        if (iab[k]+iab[k+1]==0) {
          // check if side is used before
          ibit= (ij+markused[k][0])*2 + markused[k][1];
          iwrd= ibit/nbitwd;
          ibit%=nbitwd;
	  if ((iused[iwrd] & bit[ibit]) == 0) kc[nc++]= k;
	}
      }

      if (nc>1) {
	// saddle point
	if (!drawBorders) {
	  // choosing exit side closest start pos...hmmm...
	  d2min= d2max;
	  kcmin= kc[0];
	  for (ic=0; ic<nc; ic++) {
	    kk= kc[ic];
            frac= (value             - z[ij+ijadd[kk]]) /
	          (z[ij+ijadd[kk+1]] - z[ij+ijadd[kk]]);
            dx= float(i) + dxside[kk] + frac*dxfrac[kk] - xpos;
            dy= float(j) + dyside[kk] + frac*dyfrac[kk] - ypos;
            if (d2min>dx*dx+dy*dy) {
	      d2min= dx*dx+dy*dy;
	      kcmin= kk;
	    }
	  }
	} else {
	  kcmin= -1;
	  for (ic=0; ic<nc; ic++) {
	    kk= kc[ic];
	    if (z[ij+ijadd[kk]]==zleft &&
	        z[ij+ijadd[kk+1]]==zright) kcmin= kk;
	  }
	  if (kcmin<0) nc= 0;
	}
	kc[0]= kcmin;
      }
      if (nc>0) {
	kk= kc[0];
	if (!drawBorders)
          frac= (value             - z[ij+ijadd[kk]]) /
	        (z[ij+ijadd[kk+1]] - z[ij+ijadd[kk]]);
	else
	  frac= 0.5f;
        xtmp.push_back(float(i) + dxside[kk] + frac*dxfrac[kk]);
        ytmp.push_back(float(j) + dyside[kk] + frac*dyfrac[kk]);
	// mark side as used
        ibit= (ij+markused[kk][0])*2 + markused[kk][1];
        iwrd= ibit/nbitwd;
        ibit%=nbitwd;
        iused[iwrd] = iused[iwrd] | bit[ibit];
      } else if (ij==ijstart) {
	// found a closed line
	closed= true;
	search= false;
      } else {
	// no exit found
	ended= true;
      }
    } else {
      // outside grid
      ended= true;
    }

    if (ended) {
      // no exit found from current square, or outside grid
      numend++;
      if (numend==1) {
	// turn line and search in the other direction
        m= xtmp.size();
	n= m-1;
	m/=2;
        for (i=0; i<m; ++i) {
          xh= xtmp[i];
          yh= ytmp[i];
          xtmp[i]= xtmp[n-i];
          ytmp[i]= ytmp[n-i];
          xtmp[n-i]= xh;
          ytmp[n-i]= yh;
	}
	xh= zleft;
	zleft= zright;
	zright= xh;
	// signal to go back to start square
	kk= -1;
      } else {
	search= false;
      }
    }
  }   // end search

  delete[] iused;

  if (xtmp.size()<2) return isoline;

  if (!drawBorders && nsmooth>0) {

    // smooth line (insert linesmoothed points)
    int nfirst,nlast;
    k= xtmp.size();
    n= k;
    if (closed) {
      n= k+1;
      m= n + (n-1)*nsmooth;
      n= k+3;
      nfirst= 1;
      nlast=  n-2;
    } else {
      n= k;
      m= n + (n-1)*nsmooth;
      nfirst= 0;
      nlast=  n-1;
    }

    float *xp= new float[n];
    float *yp= new float[n];
    float *xs= new float[m];
    float *ys= new float[m];

    for (i=0; i<k; ++i) {
      xp[i]= xtmp[i];
      yp[i]= ytmp[i];
    }
    if (closed) {
      for (i=0; i<3; ++i) {
        xp[k+i]= xtmp[i];
        yp[k+i]= ytmp[i];
      }
    }

    m= smoothline(n,xp,yp,nfirst,nlast,nsmooth,xs,ys);

    for (i=0; i<m; ++i) {
      isoline.x.push_back(xs[i]);
      isoline.y.push_back(ys[i]);
    }

    delete[] xp;
    delete[] yp;
    delete[] xs;
    delete[] ys;

  } else if (drawBorders) {

    int npos= xtmp.size();
//#############################################################
#ifdef DEBUGCLASSES
    cerr<<"---------------------------------------------"<<endl;
    cerr<<"Before drawBorders linefix  npos= "<<npos
        <<"  closed= "<<closed<<endl;
    int n1= (npos<6) ? npos : 6;
    int n2= (npos<6) ? 0 : npos-6;
    cerr<<"start x:";
    for (i=0; i<n1; i++)
      cerr<<setw(7)<<setprecision(2)<<setiosflags(ios::fixed)<<xtmp[i];
    cerr<<endl;
    cerr<<"      y:";
    for (i=0; i<n1; i++)
      cerr<<setw(7)<<setprecision(2)<<setiosflags(ios::fixed)<<ytmp[i];
    cerr<<endl;
    cerr<<"  end x:";
    for (i=n2; i<npos; i++)
      cerr<<setw(7)<<setprecision(2)<<setiosflags(ios::fixed)<<xtmp[i];
    cerr<<endl;
    cerr<<"      y:";
    for (i=n2; i<npos; i++)
      cerr<<setw(7)<<setprecision(2)<<setiosflags(ios::fixed)<<ytmp[i];
    cerr<<endl;
#endif
//#############################################################
//  xtmp.push_back(0.0f);
//  ytmp.push_back(0.0f);
//  if (!closed) {
//    for (i=npos; i>0; i--) {
//	xtmp[i+1]= xtmp[i];
//	ytmp[i+1]= ytmp[i];
//    }
//    npos++;
//  } else {
//    npos++;
//    xtmp[npos]= xtmp[2];
//    ytmp[npos]= ytmp[2];
//  }
    if (closed) {
      xtmp.push_back(xtmp[0]);
      ytmp.push_back(ytmp[0]);
      npos++;
    }
    for (n=1; n<npos-1; n++) {
      if (fabsf(xtmp[n+1]-xtmp[n])<0.1 ||
          fabsf(ytmp[n+1]-ytmp[n])<0.1) {
        xtmp[n]= (xtmp[n]+xtmp[n+1])*0.5;
        ytmp[n]= (ytmp[n]+ytmp[n+1])*0.5;
      } else {
	i= int((xtmp[n]+xtmp[n+1])*0.5);
	j= int((ytmp[n]+ytmp[n+1])*0.5);
	xtmp[n]= float(i)+0.5f;
	ytmp[n]= float(j)+0.5f;
      }
    }
    isoline.x = xtmp;
    isoline.y = ytmp;
//#############################################################
#ifdef DEBUGCLASSES
    npos= xtmp.size();
    cerr<<"----------------"<<endl;
    cerr<<"After drawBorders linefix  npos= "<<npos
        <<"  closed= "<<closed<<endl;
    n1= (npos<6) ? npos : 6;
    n2= (npos<6) ? 0 : npos-6;
    cerr<<"start x:";
    for (i=0; i<n1; i++)
      cerr<<setw(7)<<setprecision(2)<<setiosflags(ios::fixed)<<xtmp[i];
    cerr<<endl;
    cerr<<"      y:";
    for (i=0; i<n1; i++)
      cerr<<setw(7)<<setprecision(2)<<setiosflags(ios::fixed)<<ytmp[i];
    cerr<<endl;
    cerr<<"  end x:";
    for (i=n2; i<npos; i++)
      cerr<<setw(7)<<setprecision(2)<<setiosflags(ios::fixed)<<xtmp[i];
    cerr<<endl;
    cerr<<"      y:";
    for (i=n2; i<npos; i++)
      cerr<<setw(7)<<setprecision(2)<<setiosflags(ios::fixed)<<ytmp[i];
    cerr<<endl;
    cerr<<"---------------------------------------------"<<endl;
#endif
//#############################################################

  } else {

    isoline.x = xtmp;
    isoline.y = ytmp;
    if (closed) {
      isoline.x.push_back(xtmp[0]);
      isoline.y.push_back(ytmp[0]);
    }

  }

  isoline.value=  value;
  isoline.vleft=  zleft;
  isoline.vright= zright;
  isoline.closed= closed;

  return isoline;
}


int FieldEdit::smoothline(int npos, float x[], float y[],
			  int nfirst, int nlast, int ismooth,
                          float xsmooth[], float ysmooth[])
{
  // Smooth line, make and return spline through points.
  //
  //  input:
  //     x(n),y(n), n=1,npos:   x and y in "window" coordinates
  //     x(nfrst),y(nfsrt):     first point
  //     x(nlast),y(nlast):     last  point
  //     ismooth:               number of points spline-interpolated
  //                            between each pair of input points
  //
  //  method: 'hermit interpolation'
  //     nfirst=0:      starting condition for spline = relaxed
  //     nfirst>0:      starting condition for spline = clamped
  //     nlast<npos-1:  ending   condition for spline = clamped
  //     nlast=npos-1:  ending   condition for spline = relaxed
  //        relaxed  -  second derivative is zero
  //        clamped  -  derivatives computed from nearest points

  int   ndivs, n, ns, i;
  float rdivs, xl1, yl1, s1, xl2, yl2, s2, dx1, dy1, dx2, dy2;
  float c32, c42, c31, c41, fx1, fx2, fx3, fx4, fy1, fy2, fy3, fy4;
  float tstep, t, t2, t3;

  if (npos<3 || nfirst<0 || nfirst>=nlast
      || nlast>npos-1 || ismooth<1) {
    nfirst = (nfirst>0)     ? nfirst : 0;
    nlast  = (nlast<npos-1) ? nlast  : npos-1;
    ns = 0;
    for (n=nfirst; n<=nlast; ++n) {
      xsmooth[ns] = x[n];
      ysmooth[ns] = y[n];
      ++ns;
    }
    return ns;
  }

  ndivs = ismooth;
  rdivs = 1./float(ismooth+1);

  n = nfirst;
  if (n > 0)
    {
      xl1 = x[n]-x[n-1];
      yl1 = y[n]-y[n-1];
      s1  = sqrtf(xl1*xl1+yl1*yl1);
      xl2 = x[n+1]-x[n];
      yl2 = y[n+1]-y[n];
      s2  = sqrtf(xl2*xl2+yl2*yl2);
      dx2 = (xl1*(s2/s1)+xl2*(s1/s2))/(s1+s2);
      dy2 = (yl1*(s2/s1)+yl2*(s1/s2))/(s1+s2);
    }
  else
    {
      xl2 = x[n+1]-x[n];
      yl2 = y[n+1]-y[n];
      s2  = sqrtf(xl2*xl2+yl2*yl2);
      dx2 = xl2/s2;
      dy2 = yl2/s2;
    }

  xsmooth[0] = x[nfirst];
  ysmooth[0] = y[nfirst];
  ns = 0;

  for (n=nfirst+1; n<=nlast; ++n)
    {
      xl1 = xl2;
      yl1 = yl2;
      s1  = s2;
      dx1 = dx2;
      dy1 = dy2;

      if (n < npos-1) {
        xl2 = x[n+1]-x[n];
        yl2 = y[n+1]-y[n];
        s2  = sqrtf(xl2*xl2+yl2*yl2);
        dx2 = (xl1*(s2/s1)+xl2*(s1/s2))/(s1+s2);
	dy2 = (yl1*(s2/s1)+yl2*(s1/s2))/(s1+s2);
      }
      else {
        dx2 = xl1/s1;
        dy2 = yl1/s1;
      }

      // four spline coefficients for x and y
      c32 =  1./s1;
      c42 =  c32*c32;
      c31 =  c42*3.;
      c41 =  c42*c32*2.;
      fx1 =  x[n-1];
      fx2 =  dx1;
      fx3 =  c31*xl1-c32*(2.*dx1+dx2);
      fx4 = -c41*xl1+c42*(dx1+dx2);
      fy1 =  y[n-1];
      fy2 =  dy1;
      fy3 =  c31*yl1-c32*(2.*dy1+dy2);
      fy4 = -c41*yl1+c42*(dy1+dy2);

      // make 'ismooth' straight lines, from point 'n-1' to point 'n'

      tstep = s1*rdivs;
      t = 0.;

      for (i=0; i<ndivs; ++i) {
        t += tstep;
        t2 = t*t;
	t3 = t2*t;
        ns++;
        xsmooth[ns] = fx1 + fx2*t + fx3*t2 + fx4*t3;
	ysmooth[ns] = fy1 + fy2*t + fy3*t2 + fy4*t3;
      }

      ns++;
      xsmooth[ns] = x[n];
      ysmooth[ns] = y[n];
    }

  ns++;

  return ns;
}


void FieldEdit::editSmooth(float px, float py) {

  int   i, j, i1, i2, j1, j2, mx, my, size;
  float r;

  // reset previous smooth
  for (j=j1ed; j<j2ed; ++j)
    for (i=i1ed; i<i2ed; ++i)
      workfield->data[j*nx+i]= odata[j*nx+i];

  if (influencetype==0) r= rcircle;
  else                  r= sqrtf(axellipse*axellipse+ayellipse*ayellipse);

  i1= int(px-r+1.);  if (i1<0)  i1=0;   i1ed=i1;
  i2= int(px+r+1.);  if (i2>nx) i2=nx;  i2ed=i2;
  j1= int(py-r+1.);  if (j1<0)  j1=0;   j1ed=j1;
  j2= int(py+r+1.);  if (j2>ny) j2=ny;  j2ed=j2;

  if (i1>=i2 || j1>=j2) {
    i1ed = nx;  i2ed = -1;
    j1ed = ny;  j2ed = -1;
    return;
  }

  mx = i2-i1;
  my = j2-j1;
  size = mx*my;

  float *weight = new float[size];
  float *newfm  = new float[size];
  float *fmask  = new float[size];
  float *worku1 = new float[size];
  float *worku2 = new float[size];

  editWeight(px, py, i1, i2, j1, j2, weight);

  for (i=0; i<size; ++i)
    if (weight[i]>0.) fmask[i]= 1.;
    else              fmask[i]= 0.;

  for (j=0; j<my; j++)
    for (i=0; i<mx; i++)
      newfm[j*mx+i]= workfield->data[(j1+j)*nx+i1+i];

  int nsmoothf=-1;
  bool allDefined= true;

  quicksmooth(nsmoothf, allDefined, mx, my, newfm, fmask, worku1, worku2);

  for (j=0; j<my; j++)
    for (i=0; i<mx; i++)
      workfield->data[(j1+j)*nx+i1+i]= newfm[j*mx+i];

  delete[] weight;
  delete[] newfm;
  delete[] fmask;
  delete[] worku1;
  delete[] worku2;
}


void FieldEdit::editBrush(float px, float py)
{
  int i1,i2,j1,j2;
  float r;
  if (influencetype==0 || influencetype==3)  // circle or square
    r= rcircle;
  else                                       // ellipse
    r= sqrtf(axellipse*axellipse+ayellipse*ayellipse);

  int infl= influencetype;

  if (r>0.6f) {
    i1= int(px-r+1.);
    i2= int(px+r+1.);
    j1= int(py-r+1.);
    j2= int(py+r+1.);
  } else {
    // set a single point and avoid not hitting any gridpoint
    infl= -1;
    i1= int(px+0.5);
    i2= i1+1;
    j1= int(py+0.5);
    j2= j1+1;
  }
  if (i1<0)  i1=0;
  if (i2>nx) i2=nx;
  if (j1<0)  j1=0;
  if (j2>ny) j2=ny;

  if (i1>=i2 || j1>=j2) return;

  if (i1ed>i1) i1ed=i1;
  if (i2ed<i2) i2ed=i2;
  if (j1ed>j1) j1ed=j1;
  if (j2ed<j2) j2ed=j2;

  // steps necessary when cursor has moved a long distance since previous position
  float dx= fabsf(px - xprev);
  float dy= fabsf(py - yprev);
  int nstep;
  if (dx>dy)
    nstep= int(dx)+1;
  else
    nstep= int(dy)+1;

  float value;
  float *fdata, *repdata;

  if (brushReplaceUndef) {
    fdata=   editfield->data;
    repdata= workfield->data;
  } else {
    value= brushValue;
    fdata= (value==fieldUndef) ? editfield->data : workfield->data;
  }

  for (int n=0; n<nstep; n++) {

    float xm= xprev + (px-xprev)*float(n+1)/float(nstep);
    float ym= yprev + (py-yprev)*float(n+1)/float(nstep);

    if (r>0.6f) {
      i1= int(xm-r+1.);
      i2= int(xm+r+1.);
      j1= int(ym-r+1.);
      j2= int(ym+r+1.);
    } else {
      // set a single point and avoid not hitting any gridpoint
      i1= int(xm+0.5);
      i2= i1+1;
      j1= int(ym+0.5);
      j2= j1+1;
    }
    if (i1<0)  i1=0;
    if (i2>nx) i2=nx;
    if (j1<0)  j1=0;
    if (j2>ny) j2=ny;

#ifdef DEBUGCLASSES
    cerr<<"BRUSH n,i1,i2,j1,j2,r: "
        <<n<<" "<<i1<<" "<<i2<<" "<<j1<<" "<<j2<<" "<<r<<endl;
#endif

    if (infl<0) {

      if (brushReplaceUndef) {

        if (i1<i2 && j1<j2 && fdata[j1*nx+i1]==fieldUndef) {
	  fdata[j1*nx+i1]= repdata[j1*nx+i1];
	  numUndefReplaced++;
	}

      } else {

        if (i1<i2 && j1<j2) fdata[j1*nx+i1]= value;

      }

    } else if (influencetype==0) {

      // circle
      float dy2,s2, r2=r*r;

      if (brushReplaceUndef) {

        for (int j=j1; j<j2; j++) {
          dy= float(j)-ym;
          dy2= dy*dy;
          for (int i=i1; i<i2; i++) {
	    if (fdata[j*nx+i]==fieldUndef) {
              dx= float(i)-xm;
              s2= dx*dx+dy2;
              if (s2<r2) {
		fdata[j*nx+i]= repdata[j*nx+i];
		numUndefReplaced++;
	      }
	    }
          }
        }

      } else {

        for (int j=j1; j<j2; j++) {
          dy= float(j)-ym;
          dy2= dy*dy;
          for (int i=i1; i<i2; i++) {
            dx= float(i)-xm;
            s2= dx*dx+dy2;
            if (s2<r2) fdata[j*nx+i]= value;
          }
        }

      }

    } else if (influencetype==3) {

      if (brushReplaceUndef) {

        // square
        for (int j=j1; j<j2; j++)
          for (int i=i1; i<i2; i++)
	    if (fdata[j*nx+i]==fieldUndef) {
	      fdata[j*nx+i]= repdata[j*nx+i];
	      numUndefReplaced++;
	    }

      } else {

        // square
        for (int j=j1; j<j2; j++)
          for (int i=i1; i<i2; i++)
	    fdata[j*nx+i]= value;

      }

    } else {

      // ellipse (same border for the two types)
      float e = ecellipse;
      float a = sqrtf(axellipse*axellipse+ayellipse*ayellipse);
      float b = a*sqrtf(1.-e*e);
      float c = 2.*sqrtf(a*a-b*b);
      float ecos= axellipse/a;
      float esin= ayellipse/a;
      float ex,ey,fx1,fy1,fx2,fy2;
      ex= -c*0.5;
      ey= 0.;
      fx1= px + ex*ecos - ey*esin;
      fy1= py + ex*esin + ey*ecos;
      ex= c*0.5;
      ey= 0.;
      fx2= px + ex*ecos - ey*esin;
      fy2= py + ex*esin + ey*ecos;

      float dx,dy,dy2,s2,s,gcos,gsin,rcos,smax2;
      float en= 0.5*c/a;
      float en2= en*en, b2= b*b;

      if (brushReplaceUndef) {

        for (int j=j1; j<j2; j++) {
          dy=float(j)-py;
          dy2=dy*dy;
          for (int i=i1; i<i2; i++) {
	    if (fdata[j*nx+i]==fieldUndef) {
              dx=float(i)-px;
              s2=dx*dx + dy2;
	      s= sqrtf(s2);
	      gcos= dx/s;
	      gsin= dy/s;
	      rcos= gcos*ecos + gsin*esin; // cos(angle between a-axis and gridpos)
	      smax2= b2/(1.-en2*rcos*rcos);
              if (s2<smax2) {
		fdata[j*nx+i]= repdata[j*nx+i];
		numUndefReplaced++;
	      }
	    }
          }
        }

      } else {

        for (int j=j1; j<j2; j++) {
          dy=float(j)-py;
          dy2=dy*dy;
          for (int i=i1; i<i2; i++) {
            dx=float(i)-px;
            s2=dx*dx + dy2;
	    s= sqrtf(s2);
	    gcos= dx/s;
	    gsin= dy/s;
	    rcos= gcos*ecos + gsin*esin; // cos(angle between a-axis and gridpos)
	    smax2= b2/(1.-en2*rcos*rcos);
            if (s2<smax2) fdata[j*nx+i]= value;
          }
        }

      }

    }
  }
}


void FieldEdit::editClassLine()
{
  float px, py, dx, dy, d2, d2min;
  int k;
#ifdef DEBUGCLASSES
  cerr<<"FieldEdit::editClassLine xline.size(),isoline.x.size(): "
      <<xline.size()<<"  "<<isoline.x.size()<<endl;
#endif

  int npos= xline.size();
  if (npos<2) return;

  int npline= isoline.x.size();
  if (npline<2) return;

  if (classLineValue==fieldUndef) {
    int interpoltype= 0;
    float fv= fieldUndef;
    px= xline[npos-1];
    py= yline[npos-1];
    editfield->interpolate(1,&px,&py,&fv,interpoltype);
    if (fv==fieldUndef || fabsf(fv-fline)<0.5) {
#ifdef DEBUGCLASSES
      cerr<<"!!!!!!!!!!!!!!! NO classLine !!!!!!!!!!!!!!!!!"<<endl;
#endif
      return;
    }

    // fline is the first interpolated value, fv the second
    // check if going towards higher or lower value
    if (fv>fline)
      classLineValue= (isoline.vleft<isoline.vright) ?
      			isoline.vleft : isoline.vright;
    else
      classLineValue= (isoline.vleft>isoline.vright) ?
      			isoline.vleft : isoline.vright;
  }

  // reset previous operation
  for (int j=j1ed; j<j2ed; ++j)
    for (int i=i1ed; i<i2ed; ++i)
      workfield->data[j*nx+i]= odata[j*nx+i];

  i1ed = nx;  i2ed = -1;
  j1ed = ny;  j2ed = -1;

  float xp[2]= { xline[0], xline[npos-1] };
  float yp[2]= { yline[0], yline[npos-1] };
  int nearp[2];

  for (int n=0; n<2; n++) {
    px= xp[n];
    py= yp[n];
    d2min= +1.e+35;
    k= 0;
    for (int i=0; i<npline; i++) {
      dx= isoline.x[i] - px;
      dy= isoline.y[i] - py;
      d2= dx*dx+dy*dy;
      if (d2<d2min) {
	d2min= d2;
	k= i;
      }
    }
    nearp[n]= k;
  }

  // make a closed line surrouding the field area to be replace
  // (no need to put first point as last point too)
  vector<float> vx,vy;

  // skip lineparts that is inside a single gridsquare
  int li=0;
  int lj=0;
  int i,j;

  for (int n=0; n<npos; n++) {
    i= int(xline[n]);  if (xline[n]<0) i--;
    j= int(yline[n]);  if (yline[n]<0) j--;
    if (i!=li || j!=lj || n==0 || n==npos-1) {
      li= i;
      lj= j;
      vx.push_back(xline[n]);
      vy.push_back(yline[n]);
    }
  }

  if (nearp[1]<=nearp[0]) {
    for (int n=nearp[1]; n<=nearp[0]; n++) {
      vx.push_back(isoline.x[n]);
      vy.push_back(isoline.y[n]);
    }
  } else {
    for (int n=nearp[1]; n>=nearp[0]; n--) {
      vx.push_back(isoline.x[n]);
      vy.push_back(isoline.y[n]);
    }
  }

  replaceInsideLine(vx,vy,classLineValue);
}


void FieldEdit::replaceInsideLine(const vector<float>& vx,
				  const vector<float>& vy,
				  float replaceValue)
{
  const int nbitwd= sizeof(int)*8;

  int m= vx.size();
  if (m<3 || vy.size()!=m) return;  // programmers error!

  float x1=1.e+35, x2=-1.e+35;
  float y1=1.e+35, y2=-1.e+35;

  for (int n=0; n<m; n++) {
    if(x1>vx[n]) x1= vx[n];
    if(x2<vx[n]) x2= vx[n];
    if(y1>vy[n]) y1= vy[n];
    if(y2<vy[n]) y2= vy[n];
  }

  int i1= int(x1)+1;  if (i1<0)  i1=0;
  int i2= int(x2)+1;  if (i2>nx) i2=nx;
  int j1= int(y1)+1;  if (j1<0)  j1=0;
  int j2= int(y2)+1;  if (j2>ny) j2=ny;

  if (i1>=i2 || j1>=j2) return;

  if (i1ed>i1) i1ed=i1;
  if (i2ed<i2) i2ed=i2;
  if (j1ed>j1) j1ed=j1;
  if (j2ed<j2) j2ed=j2;

  int mx= i2-i1;
  int my= j2-j1;
  int lmark= (mx*my+nbitwd-1)/nbitwd;
  int *mark= new int[lmark];
  for (int i=0; i<lmark; i++) mark[i]=0;
//###########################################################
#ifdef DEBUGCLASSES
  int *umark= new int[lmark];
  for (int i=0; i<lmark; i++) umark[i]=0;
#endif
//###########################################################

  int bit[nbitwd];
  int bitclear[nbitwd];

  for (int i=0; i<nbitwd; i++) {
    bit[i] = 1 << i;
    bitclear[i] = ~bit[i];
  }

  int i,j,jp1,jp2,jpp1,jpp2,im,jm,iwrd,ibit,mm;
  float px,py;

  j2--;

  x2= vx[m-1];
  y2= vy[m-1];

  for (int n=0; n<m; n++) {
    x1= x2;
    y1= y2;
    x2= vx[n];
    y2= vy[n];
    if (y1<y2) {
      jpp1= int(y1)+1;
      jpp2= int(y2);
    } else {
      jpp1= int(y2)+1;
      jpp2= int(y1);
    }
    if (jpp1<j1) jpp1= j1;
    if (jpp2>j2) jpp2= j2;

    // mark on left side of gridpoints
    for (j=jpp1; j<=jpp2; j++) {
      py= float(j);
      px= x1 + (x2-x1)*(py-y1)/(y2-y1);
      im= (px>0) ? int(px)-i1+1 : int(px)-i1;
      if (im<mx) {
        if (im<0) im= 0;
        jm= j-j1;
//###########################################################
#ifdef DEBUGCLASSES
        cerr<<"  MARK im,jm: "<<im<<" "<<jm
	    <<"  n,x1,y1,x2,y2: "<<setw(2)<<n<<" "
	    <<setw(5)<<setprecision(2)<<setiosflags(ios::fixed)<<x1-i1<<" "
	    <<setw(5)<<setprecision(2)<<setiosflags(ios::fixed)<<y1-j1<<" "
	    <<setw(5)<<setprecision(2)<<setiosflags(ios::fixed)<<x2-i1<<" "
	    <<setw(5)<<setprecision(2)<<setiosflags(ios::fixed)<<y2-j1<<"  px,py: "
	    <<setw(5)<<setprecision(2)<<setiosflags(ios::fixed)<<px-i1<<" "
	    <<setw(5)<<setprecision(2)<<setiosflags(ios::fixed)<<py-j1<<endl;
#endif
//###########################################################
        ibit= jm*mx+im;
        iwrd= ibit/nbitwd;
        ibit= ibit%nbitwd;
        if ((mark[iwrd] & bit[ibit]) == 0)
          mark[iwrd]|=bit[ibit];
        else
//###########################################################
	{
//###########################################################
          mark[iwrd]&=bitclear[ibit];
//###########################################################
#ifdef DEBUGCLASSES
          umark[iwrd]|=bit[ibit];
#endif
	}
//###########################################################
      }
    }
  }

//#############################################################
#ifdef DEBUGCLASSES
  if (m<31) {
    cerr<<"--------------------------------------"<<endl;
    const int PW=6, PR=2;
    int nppp=m;
    int nsss=10;
    for (int ii1=0; ii1<nppp; ii1+=nsss) {
      int ii2= (ii1+nsss)<nppp ? ii1+nsss : nppp;
      cerr<<"  x:";
      for (int ii=ii1; ii<ii2; ii++)
        cerr<<" "<<setw(PW)<<setprecision(PR)<<setiosflags(ios::fixed)
            <<vx[ii]-i1;
      cerr<<endl;
      cerr<<"  y:";
      for (int ii=ii1; ii<ii2; ii++)
        cerr<<" "<<setw(PW)<<setprecision(PR)<<setiosflags(ios::fixed)
            <<vy[ii]-j1;
      cerr<<endl;
    }
  }
  cerr<<"---------- mx,my: "<<mx<<" "<<my<<endl;
  if (mx<40 && my<40) {
    for (jm=my-1; jm>=0; jm--) {
      miString str;
      mm=0;
      for (im=0; im<mx; im++) {
        ibit= jm*mx+im;
        iwrd= ibit/nbitwd;
        ibit= ibit%nbitwd;
        if ((umark[iwrd] & bit[ibit]) == 0) {
          if ((mark[iwrd] & bit[ibit]) != 0) str+="X.";
          else                               str+=" .";
	} else {
          if ((mark[iwrd] & bit[ibit]) != 0) str+="W.";
          else                               str+="U.";
	}
      }
      cerr<<"  "<<str<<endl;
    }
  }
#endif
//#############################################################

  // update the field
  for (jm=0; jm<my; jm++) {
    j=j1+jm;
    mm=0;
    for (im=0; im<mx; im++) {
      ibit= jm*mx+im;
      iwrd= ibit/nbitwd;
      ibit= ibit%nbitwd;
      if ((mark[iwrd] & bit[ibit]) != 0) mm=(mm+1)%2;
      if (mm==1) workfield->data[j*nx+im+i1]= replaceValue;
    }
  }

  delete[] mark;
}


void FieldEdit::editWeight(float px, float py,
			   int i1, int i2, int j1, int j2, float *weight) {

  // influencetype==0: circle with radius rcircle
  //                   weights computed relative to centre of the circle.
  // influencetype==1: ellipse with major halfaxis=(axellipse,ayellipse)
  //                   and eccentricity=ecellipse,
  //                   weights computed relative to centre of ellipse.
  // influencetype==2: ellipse with major halfaxis=(axellipse,ayellipse)
  //                   and eccentricity=ecellipse,
  //                   weights computed relative to the line between the
  //                   focus points of the ellipse.
  //
  // NOTE; weight array only covering the influence area (i1,i2,j1,j2).

  int n,i,j;
  n= 0;

  if (influencetype==0) {

    float dx, dy, dy2, s2;
    float r= rcircle;
    float r2 = r*r;

    for (j=j1; j<j2; ++j) {
      dy=float(j)-py;
      dy2=dy*dy;
      for (i=i1; i<i2; ++i) {
        dx=float(i)-px;
        s2=dx*dx + dy2;
        if (s2<r2) weight[n++]= (r-sqrtf(s2))/r;  // linear weight
        //######## weight[n++]= (r2-s2)/(r2+s2);  // another weight
	else       weight[n++]= 0.;
      }
    }

  } else {

    // ellipse
    float e = ecellipse;
    float a = sqrtf(axellipse*axellipse+ayellipse*ayellipse);
    float b = a*sqrtf(1.-e*e);
    float c = 2.*sqrtf(a*a-b*b);
    float ecos= axellipse/a;
    float esin= ayellipse/a;
    float ex,ey,fx1,fy1,fx2,fy2;
    ex= -c*0.5;
    ey= 0.;
    fx1= px + ex*ecos - ey*esin;
    fy1= py + ex*esin + ey*ecos;
    ex= c*0.5;
    ey= 0.;
    fx2= px + ex*ecos - ey*esin;
    fy2= py + ex*esin + ey*ecos;

    if (influencetype==1) {

      float dx,dy,dy2,s,gcos,gsin,rcos,smax;
      float en= 0.5*c/a;
      float en2= en*en, b2= b*b;

      for (j=j1; j<j2; ++j) {
        dy=float(j)-py;
        dy2=dy*dy;
        for (i=i1; i<i2; ++i) {
          dx=float(i)-px;
          s=sqrtf(dx*dx + dy2);
	  gcos= dx/s;
	  gsin= dy/s;
	  rcos= gcos*ecos + gsin*esin; // cos(angle between a-axis and gridpos)
	  smax= sqrtf(b2/(1.-en2*rcos*rcos));
          if (s<smax) weight[n++]= (smax-s)/smax;  // linear weight
	  else        weight[n++]= 0.;
        }
      }

    } else {

      float x,y,dx1,dy1,dy12,dx2,dy2,dy22,s;
      float smax= a*2. - c;

      for (j=j1; j<j2; ++j) {
        y= float(j);
        dy1 = y - fy1;
        dy12= dy1*dy1;
        dy2 = y - fy2;
        dy22= dy2*dy2;
        for (i=i1; i<i2; ++i) {
          x= float(i);
          dx1= x - fx1;
          dx2= x - fx2;
          s= sqrtf(dx1*dx1 + dy12) + sqrtf(dx2*dx2 + dy22) - c;
          if (s<smax) weight[n++]= (smax-s)/smax;  // linear weight
	  else        weight[n++]= 0.;
        }
      }
    }
  }
}


void FieldEdit::editExpWeight(float px, float py,
			      int i1, int i2, int j1, int j2, float *weight,
			      bool gradients) {

  // influencetype==0: circle with radius rcircle
  //                   weights computed relative to centre of the circle.
  // influencetype==1: ellipse with major halfaxis=(axellipse,ayellipse)
  //                   and eccentricity=ecellipse,
  //                   weights computed relative to centre of ellipse.
  // influencetype==2: ellipse with major halfaxis=(axellipse,ayellipse)
  //                   and eccentricity=ecellipse,
  //                   weights computed relative to the line between the
  //                   focus points of the ellipse.
  //
  // NOTE; weight array only covering the influence area (i1,i2,j1,j2).

  int n,i,j;
  n= 0;

  if (influencetype==0) {

    float dx, dy, dy2, s2;
    float r= rcircle;
    float r2 = r*r;
    float bexp= r*0.3;
    float bbexp= 1.0/(2.0*bexp*bexp);
    float scale= (gradients) ? 1.0/(bexp*bexp) : 1.0;

    for (j=j1; j<j2; ++j) {
      dy=float(j)-py;
      dy2=dy*dy;
      for (i=i1; i<i2; ++i) {
        dx=float(i)-px;
        s2=dx*dx + dy2;
        if (s2<r2) weight[n++]= scale * exp(-s2*bbexp); // exponential weight
	else       weight[n++]= 0.;
      }
    }

  } else {

    // ellipse
    float e = ecellipse;
    float a = sqrtf(axellipse*axellipse+ayellipse*ayellipse);
    float b = a*sqrtf(1.-e*e);
    float c = 2.*sqrtf(a*a-b*b);
    float ecos= axellipse/a;
    float esin= ayellipse/a;
    float ex,ey,fx1,fy1,fx2,fy2;
    ex= -c*0.5;
    ey= 0.;
    fx1= px + ex*ecos - ey*esin;
    fy1= py + ex*esin + ey*ecos;
    ex= c*0.5;
    ey= 0.;
    fx2= px + ex*ecos - ey*esin;
    fy2= py + ex*esin + ey*ecos;

    if (influencetype==1) {

      float dx,dy,dy2,s,gcos,gsin,rcos,smax;
      float en= 0.5*c/a;
      float en2= en*en, b2= b*b;
      float bexp,bbexp;

      for (j=j1; j<j2; ++j) {
        dy=float(j)-py;
        dy2=dy*dy;
        for (i=i1; i<i2; ++i) {
          dx=float(i)-px;
          s=sqrtf(dx*dx + dy2);
	  gcos= dx/s;
	  gsin= dy/s;
	  rcos= gcos*ecos + gsin*esin; // cos(angle between a-axis and gridpos)
	  smax= sqrtf(b2/(1.-en2*rcos*rcos));
          if (s<smax) {
	    bexp= smax*0.3;
	    bbexp= 1.0/(2.0*bexp*bexp);
            if (gradients)
              weight[n++]= exp(-s*s*bbexp)/(bexp*bexp);  // exponential weight
            else
              weight[n++]= exp(-s*s*bbexp);  // exponential weight
	  } else {
	    weight[n++]= 0.;
	  }
        }
      }

    } else {

      float x,y,dx1,dy1,dy12,dx2,dy2,dy22,s;
      float smax= a*2. - c;
      float bexp= smax*0.3;
      float bbexp=1.0/(2.0*bexp*bexp);
      float scale= (gradients) ? 1.0/(bexp*bexp) : 1.0;

      for (j=j1; j<j2; ++j) {
        y= float(j);
        dy1 = y - fy1;
        dy12= dy1*dy1;
        dy2 = y - fy2;
        dy22= dy2*dy2;
        for (i=i1; i<i2; ++i) {
          x= float(i);
          dx1= x - fx1;
          dx2= x - fx2;
          s= sqrtf(dx1*dx1 + dy12) + sqrtf(dx2*dx2 + dy22) - c;
          if (s<smax) weight[n++]= scale * exp(-s*s*bbexp);  // exponential weight
	  else        weight[n++]= 0.;
        }
      }
    }
  }
}


bool FieldEdit::quicksmooth(int nsmooth, bool allDefined, int nx, int ny,
		            float *data, float *work,
	                    float *worku1, float *worku2)
{
   //  Low-bandpass filter, removing short wavelengths
   //  (not a 2nd or 4th order Shapiro filter)
   //
   //  G.J.Haltiner, Numerical Weather Prediction,
   //                   Objective Analysis,
   //                       Smoothing and filtering
   //
   //  input:   nsmooth       - no. of iterations (1,2,3,...),
   //                           nsmooth<0 => '-nsmooth' iterations and
   //                           using input work as mask
   //           work[nx*ny]   - a work matrix (size as the field),
   //                           or input mask if nsmooth<0
   //                           (0.0=not smooth, 1.0=full smooth)
   //           worku1[nx*ny] - a work matrix (only used if !allDefined or nsmooth<0)
   //           worku2[nx*ny] - a work matrix (only used if !allDefined or nsmooth<0)

   const float s = 0.25;
   int   size = nx * ny;
   int   i, j, n, i1, i2;

   if (nx<3 || ny<3) return false;

   if (nsmooth==0) return true;

   if (allDefined && nsmooth>0) {

      for (n=0; n<nsmooth; n++) {

	 // loop extended, reset below
	 for (i=1; i<size-1; ++i)
	    work[i] = data[i] + s * (data[i-1] + data[i+1] - 2.*data[i]);

	 i1 = 0;
         i2 = nx - 1;
	 for (j=0; j<ny; ++j, i1+=nx, i2+=nx) {
	    work[i1] = data[i1];
	    work[i2] = data[i2];
	 }

	 // loop extended, reset below
	 for (i=nx; i<size-nx; ++i)
	    data[i] = work[i] + s * (work[i-nx] + work[i+nx] - 2.*work[i]);

         i2 = size - nx;
	 for (i1=0; i1<nx; ++i1, ++i2) {
	    data[i1] = work[i1];
	    data[i2] = work[i2];
	 }

      }

   } else {

      if (!allDefined) {
         // loops extended, no problem
         for (i=1; i<size-1; ++i)
	    worku1[i]= (data[i-1]!=fieldUndef && data[i]  !=fieldUndef
	                                      && data[i+1]!=fieldUndef) ? s : 0.;
         for (i=nx; i<size-nx; ++i)
	    worku2[i]= (data[i-nx]!=fieldUndef && data[i]   !=fieldUndef
	                                       && data[i+nx]!=fieldUndef ) ? s : 0.;
         if (nsmooth<0) {
            for (i=1;  i<size-1;  ++i) worku1[i] = worku1[i]*work[i];
            for (i=nx; i<size-nx; ++i) worku2[i] = worku2[i]*work[i];
	 }
      } else {
         for (i=1;  i<size-1;  ++i) worku1[i] = s*work[i];
         for (i=nx; i<size-nx; ++i) worku2[i] = s*work[i];
      }

      if (nsmooth<0) nsmooth= -nsmooth;

      for (n=0; n<nsmooth; n++) {

	 // loop extended, reset below
	 for (i=1; i<size-1; ++i)
	    work[i] = data[i] + worku1[i] * (data[i-1] + data[i+1] - 2.*data[i]);

	 i1 = 0;
         i2 = nx - 1;
	 for (j=0; j<ny; ++j, i1+=nx, i2+=nx) {
	    work[i1] = data[i1];
	    work[i2] = data[i2];
	 }

	 // loop extended, reset below
	 for (i=nx; i<size-nx; ++i)
	    data[i] = work[i] + worku2[i] * (work[i-nx] + work[i+nx] - 2.*work[i]);

	 i2 = size - nx;
	 for (i1=0; i1<nx; ++i1, ++i2) {
	    data[i1] = work[i1];
	    data[i2] = work[i2];
	 }

      }

   }

   return true;
}


void FieldEdit::drawInfluence()
{

  // draw influence
  //   shape==0: circle
  //   shape==1: ellipse
  //   shape==2: ellipse with line between focus points
  //   shape==3: square

  const float drot=5.0*3.141592654/180.;
  float d,x,y,rot;
  int   i;

  // draw centre box
  Rectangle fr= splot.getPlotSize();
  float pwidth,pheight;
  splot.getPhysSize(pwidth,pheight);
  d= 5.0 * (fr.x2-fr.x1)/pwidth;

  glColor4f(0.0,1.0,1.0,0.5);
  glLineWidth(3.0);
  glBegin(GL_LINE_LOOP);
  glVertex2f(posx-d,posy-d);
  glVertex2f(posx+d,posy-d);
  glVertex2f(posx+d,posy+d);
  glVertex2f(posx-d,posy+d);
  glEnd();

  Colour col= splot.getBackContrastColour();
  glColor3ubv(col.RGB());
  glLineWidth(2.0);

  if (influencetype==0) {

    // circle
    glBegin(GL_LINE_LOOP);
    for (i=0; i<72; ++i) {
      rot= float(i)*drot;
      x= posx + rcirclePlot * cosf(rot);
      y= posy + rcirclePlot * sinf(rot);
      glVertex2f(x,y);
    }
    glEnd();

  } else if (influencetype==1 || influencetype==2) {

    // ellipse
    float e=  ecellipse;
    float dx= axellipsePlot;
    float dy= ayellipsePlot;
    float a= sqrtf(dx*dx+dy*dy);
    float b= a*sqrtf(1.-e*e);
    float c= 2.*sqrtf(a*a-b*b);
    float ecos= dx/a;
    float esin= dy/a;
    float ex,ey;
    glBegin(GL_LINE_LOOP);
    x= posx + 0.*ecos - b*esin;  // avoiding sqrt(-0.00000001)=nan
    y= posy + 0.*esin + b*ecos;
    glVertex2f(x,y);
    for (i=1; i<36; ++i) {
      ey= b*float(18-i)/18.;
      ex= a*sqrtf(1.-(ey*ey)/(b*b));
      x= posx + ex*ecos - ey*esin;
      y= posy + ex*esin + ey*ecos;
      glVertex2f(x,y);
    }
    x= posx + 0.*ecos + b*esin;  // avoiding sqrt(-0.00000001)=nan
    y= posy + 0.*esin - b*ecos;
    glVertex2f(x,y);
    for (i=35; i>0; --i) {
      ey= b*float(18-i)/18.;
      ex= -a*sqrtf(1.-(ey*ey)/(b*b));
      x= posx + ex*ecos - ey*esin;
      y= posy + ex*esin + ey*ecos;
      glVertex2f(x,y);
    }
    glEnd();
    if (influencetype==2) {
      // show line between focus points
      glBegin(GL_LINES);
      ex= -c*0.5;
      ey= 0.;
      x= posx + ex*ecos - ey*esin;
      y= posy + ex*esin + ey*ecos;
      glVertex2f(x,y);
      ex= c*0.5;
      ey= 0.;
      x= posx + ex*ecos - ey*esin;
      y= posy + ex*esin + ey*ecos;
      glVertex2f(x,y);
      glEnd();
    }

  } else if (influencetype==3) {

    // square
    glBegin(GL_LINE_LOOP);
    glVertex2f(posx-rcirclePlot,posy-rcirclePlot);
    glVertex2f(posx+rcirclePlot,posy-rcirclePlot);
    glVertex2f(posx+rcirclePlot,posy+rcirclePlot);
    glVertex2f(posx-rcirclePlot,posy+rcirclePlot);
    glEnd();
  }

  if (showArrow) {
    float dx= (xArrow-posx)*0.15;
    float dy= (yArrow-posy)*0.15;
    glLineWidth(3.0);
    glBegin(GL_LINES);
    glVertex2f(posx,posy);
    glVertex2f(xArrow,yArrow);
    glVertex2f(xArrow-dx-dy*0.667,yArrow-dy+dx*0.667);
    glVertex2f(xArrow,yArrow);
    glVertex2f(xArrow-dx+dy*0.667,yArrow-dy-dx*0.667);
    glVertex2f(xArrow,yArrow);
    glEnd();
    glLineWidth(1.0);
  }

  if ((drawExtraLines || drawIsoline) && isoline.x.size()>1) {
    glLineWidth(3.0);
    glBegin(GL_LINE_STRIP);
    int n= isoline.x.size();
    if (maparea.P()!=editfield->area.P()) {
      float *xplot= new float[n];
      float *yplot= new float[n];
      for (int i=0; i<n; ++i) {
        xplot[i]= isoline.x[i];
        yplot[i]= isoline.y[i];
      }
      if (!gc.getPoints(editfield->area,maparea,n,xplot,yplot)) n=0;
      for (int i=0; i<n; ++i)
        glVertex2f(xplot[i],yplot[i]);
      delete[] xplot;
      delete[] yplot;
    } else {
      for (int i=0; i<n; ++i)
        glVertex2f(isoline.x[i],isoline.y[i]);
    }
    glEnd();
    if (drawIsoline) {
      isoline.x.clear();
      isoline.y.clear();
      drawIsoline= false;
    }
  }
  if (drawExtraLines && xline.size()>1) {
    glLineWidth(1.0);
    glBegin(GL_LINE_STRIP);
    int n= xline.size();
    if (maparea.P()!=editfield->area.P()) {
      float *xplot= new float[n];
      float *yplot= new float[n];
      for (int i=0; i<n; ++i) {
        xplot[i]= xline[i];
        yplot[i]= yline[i];
      }
      if (!gc.getPoints(editfield->area,maparea,n,xplot,yplot)) n=0;
      for (int i=0; i<n; ++i)
        glVertex2f(xplot[i],yplot[i]);
      delete[] xplot;
      delete[] yplot;
    } else {
      for (int i=0; i<n; ++i)
        glVertex2f(xline[i],yline[i]);
    }
    glEnd();
  }
//###############################################################
/**************************************************************
  if (drawExtraLines && i1edp<i2edp && j1edp<j2edp &&
      maparea.P()==editfield->area.P()) {
    glLineWidth(1.0);
    glBegin(GL_LINE_LOOP);
    glVertex2f(float(i1edp)-0.5f,float(j1edp)-0.5f);
    glVertex2f(float(i2edp)-0.5f,float(j1edp)-0.5f);
    glVertex2f(float(i2edp)-0.5f,float(j2edp)-0.5f);
    glVertex2f(float(i1edp)-0.5f,float(j2edp)-0.5f);
    glEnd();
  }
**************************************************************/
/**************************************************************
  maparea = splot.getMapArea();
  if (lockedValue.size()>0 && maparea.P()==editfield->area.P()) {
    Rectangle rec= maparea.R();
    nx= editfield->nx;
    ny= editfield->ny;
    int i1= int(rec.x1)+1; if (i1<0)  i1=0;
    int i2= int(rec.x2)+1; if (i2>nx) i2=nx;
    int j1= int(rec.y1)+1; if (j1<0)  j1=0;
    int j2= int(rec.y2)+1; if (j2>ny) j2=ny;
    float d=0.25f;
    glColor3f(0.0f,0.0f,0.0f);
    glLineWidth(2.0);
    glBegin(GL_LINES);
    set<float>::iterator pend= lockedValue.end();
    for (int j=j1; j<j2; j++) {
      for (int i=i1; i<i2; i++) {
        if (lockedValue.find(editfield->data[j*nx+i])!=pend) {
          float px= float(i);
          float py= float(j);
          glVertex2f(px-d,py-d);
          glVertex2f(px+d,py+d);
          glVertex2f(px-d,py+d);
          glVertex2f(px+d,py-d);
        }
      }
    }
    glEnd();
  }
**************************************************************/
//###############################################################
}


bool FieldEdit::plot(bool showinfluence)
{
#ifdef DEBUGREDRAW
  if (active) cerr<<"Plot active editfield"<<endl;
  else        cerr<<"Plot deactivated editfield"<<endl;
#endif
  if (editfieldplot->getUndefinedPlot())
    editfieldplot->plotUndefined();

  bool res= editfieldplot->plot();
  if (active && showinfluence) drawInfluence();

  if (active && showinfluence && showNumbers)
    numbersDisplayed= editfieldplot->plotNumbers();
  else
    numbersDisplayed= false;

  return res;
}

bool FieldEdit::getAnnotations(vector<miString>& anno)
{
  return editfieldplot->getAnnotations(anno);
}
