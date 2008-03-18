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
#include <qapplication.h>
#include <q3filedialog.h>
#include <q3toolbar.h>
#include <qtoolbutton.h>
#include <qcombobox.h>
#include <qpushbutton.h>
#include <qtToggleButton.h>
#include <qlayout.h>
#include <qfont.h>
#include <qmotifstyle.h>
#include <qtUtility.h>
#include <qtVcrossWindow.h>
//Added by qt3to4:
#include <QPixmap>
#include <diLocationPlot.h>
#include <qtVcrossWidget.h>
#include <qtVcrossDialog.h>
#include <qtVcrossSetupDialog.h>
#include <diVcrossManager.h>
#include <qtPrintManager.h>
#include <forover.xpm>
#include <bakover.xpm>


VcrossWindow::VcrossWindow()
  : Q3MainWindow( 0, "DIANA Vcross window")
{
#ifndef linux
  qApp->setStyle(new QMotifStyle);
#endif

  //HK ??? temporary create new VcrossManager here
  vcrossm = new VcrossManager();

  setCaption( tr("Diana Vertical Crossections") );

  QGLFormat fmt;
  fmt.setOverlay(false);
  fmt.setDoubleBuffer(true);
  fmt.setDirectRendering(false);
  //central widget
  vcrossw= new VcrossWidget(vcrossm, fmt, this, "Vcross");
  setCentralWidget(vcrossw);
  connect(vcrossw, SIGNAL(timeChanged(int)),SLOT(timeChangedSlot(int)));
  connect(vcrossw, SIGNAL(crossectionChanged(int)),SLOT(crossectionChangedSlot(int)));


  //tool bar and buttons
  vcToolbar = new Q3ToolBar("vcTool", this,Qt::DockTop, FALSE,"vcTool");
  setDockEnabled( vcToolbar, Qt::DockLeft, FALSE );
  setDockEnabled( vcToolbar, Qt::DockRight, FALSE );
  //tool bar and for selecting time and crossection
  tsToolbar = new Q3ToolBar("vctsTool", this,Qt::DockTop, FALSE,"vctsTool");
  setDockEnabled( tsToolbar, Qt::DockLeft, FALSE );
  setDockEnabled( tsToolbar, Qt::DockRight, FALSE );


  //button for model/field dialog-starts new dialog
  dataButton = new ToggleButton(vcToolbar,tr("Model/field").latin1());
  connect( dataButton, SIGNAL( toggled(bool)), SLOT( dataClicked( bool) ));

  //button for setup - starts setupdialog
  setupButton = new ToggleButton(vcToolbar,tr("Settings").latin1());
  connect( setupButton, SIGNAL( toggled(bool)), SLOT( setupClicked( bool) ));

  //button for timeGraph
  timeGraphButton = new ToggleButton(vcToolbar,tr("TimeGraph").latin1());
  connect( timeGraphButton, SIGNAL( toggled(bool)), SLOT( timeGraphClicked( bool) ));

  //button to print - starts print dialog
  QPushButton* printButton = NormalPushButton(tr("Print"),vcToolbar);
  connect( printButton, SIGNAL(clicked()), SLOT( printClicked() ));

  //button to save - starts save dialog
  QPushButton* saveButton = NormalPushButton(tr("Save"),vcToolbar);
  connect( saveButton, SIGNAL(clicked()), SLOT( saveClicked() ));

  //button for quit
  QPushButton * quitButton = NormalPushButton(tr("Quit"),vcToolbar);
  connect( quitButton, SIGNAL(clicked()), SLOT(quitClicked()) );

  //button for help - pushbutton
  QPushButton * helpButton = NormalPushButton(tr("Help"),vcToolbar);
  connect( helpButton, SIGNAL(clicked()), SLOT(helpClicked()) );


  tsToolbar->addSeparator();

  QToolButton *leftCrossectionButton= new QToolButton(QPixmap(bakover_xpm),
						      tr("previous crossections"), "",
						      this, SLOT(leftCrossectionClicked()),
						      tsToolbar, "vcCstepB" );
  leftCrossectionButton->setUsesBigPixmap(false);
  leftCrossectionButton->setAutoRepeat(true);

  //combobox to select crossection
  vector<miString> dummycross;
  dummycross.push_back("                        ");
  crossectionBox = ComboBox(tsToolbar, dummycross, true, 0);
  connect( crossectionBox, SIGNAL( activated(int) ),
		       SLOT( crossectionBoxActivated(int) ) );

  QToolButton *rightCrossectionButton= new QToolButton(QPixmap(forward_xpm),
						       tr("next crossection"), "",
						       this, SLOT(rightCrossectionClicked()),
						       tsToolbar, "vcCstepF" );
  rightCrossectionButton->setUsesBigPixmap(false);
  rightCrossectionButton->setAutoRepeat(true);
  
  tsToolbar->addSeparator();

  QToolButton *leftTimeButton= new QToolButton(QPixmap(bakover_xpm),
					       tr("previous timestep"), "",
					       this, SLOT(leftTimeClicked()),
					       tsToolbar, "vcTstepB" );
  leftTimeButton->setUsesBigPixmap(false);
  leftTimeButton->setAutoRepeat(true);
  
  //combobox to select time
  miTime tnow= miTime::nowTime();
  miTime tset= miTime(tnow.year(),tnow.month(),tnow.day(),0,0,0);
  vector<miString> dummytime;
  dummytime.push_back(tset.isoTime(false,false));
  timeBox = ComboBox(tsToolbar, dummytime, true, 0);
  connect( timeBox, SIGNAL( activated(int) ),
		    SLOT( timeBoxActivated(int) ) );

  QToolButton *rightTimeButton= new QToolButton(QPixmap(forward_xpm),
						tr("next timestep"), "",
						this, SLOT(rightTimeClicked()),
						tsToolbar, "vcTstepF" );
  rightTimeButton->setUsesBigPixmap(false);
  rightTimeButton->setAutoRepeat(true);

  //connected dialogboxes

  vcDialog = new VcrossDialog(this,vcrossm);
  connect(vcDialog, SIGNAL(VcrossDialogApply(bool)),SLOT(changeFields(bool)));
  connect(vcDialog, SIGNAL(VcrossDialogHide()),SLOT(hideDialog()));
  connect(vcDialog, SIGNAL(showdoc(const miString)),
	  SIGNAL(showdoc(const miString)));


  vcSetupDialog = new VcrossSetupDialog(this,vcrossm);
  connect(vcSetupDialog, SIGNAL(SetupApply()),SLOT(changeSetup()));
  connect(vcSetupDialog, SIGNAL(SetupHide()),SLOT(hideSetup()));
  connect(vcSetupDialog, SIGNAL(showdoc(const miString)),
	  SIGNAL(showdoc(const miString)));


  //inialize everything in startUp
  firstTime = true;
  active = false; 
#ifdef DEBUGPRINT
  cerr<<"VcrossWindow::VcrossWindow() finished"<<endl;
#endif
}


/***************************************************************************/

void VcrossWindow::dataClicked( bool on ){
  //called when the model button is clicked
  if( on ){
#ifdef DEBUGPRINT
    cerr << "Model/field button clicked on" << endl;
#endif
    vcDialog->show();
  } else {
#ifdef DEBUGPRINT
    cerr << "Model/field button clicked off" << endl;
#endif
    vcDialog->hide();
  }
}

/***************************************************************************/

void VcrossWindow::leftCrossectionClicked(){
  //called when the left Crossection button is clicked
  miString s= vcrossm->setCrossection(-1);
  crossectionChangedSlot(-1);
  vcrossw->updateGL();
}

/***************************************************************************/

void VcrossWindow::rightCrossectionClicked(){
  //called when the right Crossection button is clicked
  miString s= vcrossm->setCrossection(+1);
  crossectionChangedSlot(+1);
  vcrossw->updateGL();
}

/***************************************************************************/

void VcrossWindow::leftTimeClicked(){
  //called when the left time button is clicked
  miTime t= vcrossm->setTime(-1);
  //update combobox
  timeChangedSlot(-1);
  vcrossw->updateGL();
}

/***************************************************************************/

void VcrossWindow::rightTimeClicked(){
  //called when the right Crossection button is clicked
  miTime t= vcrossm->setTime(+1);
  timeChangedSlot(+1);
  vcrossw->updateGL();
}

/***************************************************************************/

bool VcrossWindow::timeChangedSlot(int diff){
  //called if signal timeChanged is emitted from graphics
  //window (qtVcrossWidget)
#ifdef DEBUGPRINT
  cerr << "timeChangedSlot(int) is called " << endl;
#endif
  int index=timeBox->currentItem();
  while(diff<0){
    if(--index < 0) {
      //set index to the last in the box !
      index=timeBox->count()-1;
    } 
    timeBox->setCurrentItem(index);
    diff++;
  }    
  while(diff>0){
    if(++index > timeBox->count()-1) {
      //set index to the first in the box !
      index=0;
    } 
    timeBox->setCurrentItem(index);
    diff--;
  }
  miTime t = vcrossm->getTime();
  miString tstring=t.isoTime(false,false);
  if (!timeBox->count()) return false;
  miString tbs=timeBox->currentText().latin1();
  if (tbs!=tstring){
    //search timeList
    int n = timeBox->count();
    for (int i = 0; i<n;i++){      
      if(tstring ==timeBox->text(i).latin1()){
	timeBox->setCurrentItem(i);
	tbs=timeBox->currentText().latin1();
	break;
      }
    }
  }
  if (tbs!=tstring){
    cerr << "WARNING! timeChangedSlot  time from vcrossm ="
	 << t    <<" not equal to timeBox text = " << tbs << endl
	 << "You should search through timelist!" << endl;
    return false;
  }

  emit setTime("vcross",t);

  return true;
}


/***************************************************************************/

bool VcrossWindow::crossectionChangedSlot(int diff){
#ifdef DEBUGPRINT
  cerr << "crossectionChangedSlot(int) is called " << endl;
#endif
  int index=crossectionBox->currentItem();
  while(diff<0){
    if(--index < 0) {
      //set index to the last in the box !
      index=crossectionBox->count()-1;
    } 
    crossectionBox->setCurrentItem(index);
    diff++;
  }    
  while(diff>0){
    if(++index > crossectionBox->count()-1) {
      //set index to the first in the box !
      index=0;
    } 
    crossectionBox->setCurrentItem(index);
    diff--;
  }
  //get current crossection
  miString s = vcrossm->getCrossection();
  if (!crossectionBox->count()) return false;
  //if no current crossection, use last crossection plotted
  if (s.empty()) s = vcrossm->getLastCrossection();
  miString sbs=crossectionBox->currentText().latin1();
  if (sbs!=s){
    int n = crossectionBox->count();
    for(int i = 0;i<n;i++){
      if (s==crossectionBox->text(i).latin1()){
	crossectionBox->setCurrentItem(i);
	sbs=miString(crossectionBox->currentText().latin1());
	break;
      }
    }
  }
  QString sq = s.c_str();
  if (sbs==s) { 
    emit crossectionChanged(sq); //name of current crossection (to mainWindow)
    return true;
  } else {
    //    cerr << "WARNING! crossectionChangedSlot  crossection from vcrossm ="
    // 	 << s    <<" not equal to crossectionBox text = " << sbs << endl;	
    //current or last crossection plotted is not in the list, insert it...
    crossectionBox->insertItem(sq,0);
    crossectionBox->setCurrentItem(0);
    return false;
  }
}


/***************************************************************************/

void VcrossWindow::printClicked(){
  printerManager pman;
  //called when the print button is clicked
  miString command= pman.printCommand();

  QPrinter qprt;
  fromPrintOption(qprt,priop);

  if (qprt.setup(this)){
    if (qprt.outputToFile()) {
      priop.fname= qprt.outputFileName().latin1();
    } else if (command.substr(0,4)=="lpr ") {
      priop.fname= "prt_" + miTime::nowTime().isoTime() + ".ps";
      priop.fname= priop.fname.replace(' ','_');
#ifdef linux
      command= "lpr -r " + command.substr(4,command.length()-4);
#else
      command= "lpr -r -s " + command.substr(4,command.length()-4);
#endif
    } else {
      priop.fname= "tmp_vcross.ps";
    }

    // fill printOption from qprinter-selections
    toPrintOption(qprt, priop);

    // start the postscript production
    QApplication::setOverrideCursor( Qt::waitCursor );

    vcrossw->startHardcopy(priop);
    vcrossw->updateGL();
    vcrossw->endHardcopy();
    vcrossw->updateGL();

    // if output to printer: call appropriate command
    if (!qprt.outputToFile()){
      priop.numcopies= qprt.numCopies();

      // expand command-variables
      pman.expandCommand(command, priop);

      system(command.c_str());
    }
    QApplication::restoreOverrideCursor();
  }
}

/***************************************************************************/

void VcrossWindow::saveClicked()
{
  static QString fname = "./"; // keep users preferred image-path for later
  QString s = Q3FileDialog::getSaveFileName(fname,
					   tr("Images (*.png *.xpm *.bmp *.eps);;All (*.*)"),
					   this, "save_file_dialog",
					   tr("Save plot as image") );
  

  if (!s.isNull()) {// got a filename
    fname= s;
    miString filename= s.latin1();
    miString format= "PNG";
    int quality= -1; // default quality

    // find format
    if (filename.contains(".xpm") || filename.contains(".XPM"))
      format= "XPM";
    else if (filename.contains(".bmp") || filename.contains(".BMP"))
      format= "BMP";
    else if (filename.contains(".eps") || filename.contains(".epsf")){
      // make encapsulated postscript
      // NB: not screendump!
      makeEPS(filename);
      return;
    }

    // do the save
    vcrossw->saveRasterImage(filename, format, quality);
  }
}


void VcrossWindow::makeEPS(const miString& filename)
{
  QApplication::setOverrideCursor( Qt::waitCursor );
  printOptions priop;
  priop.fname= filename;
  priop.colop= d_print::incolour;
  priop.orientation= d_print::ori_automatic;
  priop.pagesize= d_print::A4;
  priop.numcopies= 1;
  priop.usecustomsize= false;
  priop.fittopage= false;
  priop.drawbackground= true;
  priop.doEPS= true;

  vcrossw->startHardcopy(priop);
  vcrossw->updateGL();
  vcrossw->endHardcopy();
  vcrossw->updateGL();

  QApplication::restoreOverrideCursor();
}

/***************************************************************************/

void VcrossWindow::setupClicked(bool on){
  //called when the setup button is clicked
  if( on ){
    vcSetupDialog->start();
    vcSetupDialog->show();
  } else {
    vcSetupDialog->hide();
  }
}

/***************************************************************************/

void VcrossWindow::timeGraphClicked(bool on)
{
  //called when the timeGraph button is clicked
#ifdef DEBUGPRINT
    cerr << "TiemGraph button clicked on=" << on << endl;
#endif
  if (on && vcrossm->timeGraphOK()) {
    vcrossw->enableTimeGraph(true);
  } else if (on) {
    timeGraphButton->setOn(false);
  } else {
    vcrossm->disableTimeGraph();
    vcrossw->enableTimeGraph(false);
    vcrossw->updateGL();
  }
}

/***************************************************************************/

void VcrossWindow::quitClicked(){
  //called when the quit button is clicked
#ifdef DEBUGPRINT
  cerr << "quit clicked" << endl;
#endif
  vcToolbar->hide();
  tsToolbar->hide();
  dataButton->setOn(false);
  setupButton->setOn(false);

  // cleanup selections in dialog and data in memory
  vcDialog->cleanup();
  vcrossm->cleanup();

  crossectionBox->clear();
  timeBox->clear();

  active = false;
  emit VcrossHide();
  vector<miTime> t;
  emit emitTimes("vcross",t);
}

/***************************************************************************/

void VcrossWindow::helpClicked(){
  //called when the help button in Vcrosswindow is clicked
#ifdef DEBUGPRINT
  cerr << "help clicked" << endl;
#endif
  emit showdoc("ug_verticalcrosssections.html");
}


/***************************************************************************/

void VcrossWindow::MenuOK(){
  //obsolete - nothing happens here 
#ifdef DEBUGPRINT
  cerr << "VcrossWindow::MenuOK()" << endl;
#endif
}

/***************************************************************************/

void VcrossWindow::changeFields(bool modelChanged){
  //called when the apply button from model/field dialog is clicked
  //... or field is changed ?
#ifdef DEBUGPRINT
  cerr << "VcrossWindow::changeFields()" << endl;
#endif

  if (modelChanged) {

    //emit to MainWindow (updates crossectionPlot)
    emit crossectionSetChanged();

    //update combobox lists of crossections and time
    updateCrossectionBox();
    updateTimeBox();

    //get correct selection in comboboxes
    crossectionChangedSlot(0);
    timeChangedSlot(0);
  }

  vcrossw->updateGL();
}

/***************************************************************************/

void VcrossWindow::changeSetup(){
  //called when the apply from setup dialog is clicked
#ifdef DEBUGPRINT
  cerr << "VcrossWindow::changeSetup()" << endl;
#endif

  //###if (mapOptionsChanged) {
    // emit to MainWindow
    // (updates crossectionPlot colour etc., not data, name etc.)
    emit crossectionSetUpdate();
  //###}

  vcrossw->updateGL();
}

/***************************************************************************/

void VcrossWindow::hideDialog(){
  //called when the hide button (from model dialog) is clicked
#ifdef DEBUGPRINT
  cerr << "VcrossWindow::hideDialog()" << endl;
#endif
  vcDialog->hide();
  dataButton->setOn(false);
}

/***************************************************************************/

void VcrossWindow::hideSetup(){
  //called when the hide button (from setup dialog) is clicked
#ifdef DEBUGPRINT
  cerr << "VcrossWindow::hideSetup()" << endl;
#endif
  vcSetupDialog->hide();
  setupButton->setOn(false);
}

/***************************************************************************/

void VcrossWindow::getCrossections(LocationData& locationdata){
#ifdef DEBUGPRINT
  cerr <<"VcrossWindow::getCrossections()" << endl;
#endif

  vcrossm->getCrossections(locationdata);

  return;
}

/***************************************************************************/

void VcrossWindow::getCrossectionOptions(LocationData& locationdata){
#ifdef DEBUGPRINT
  cerr <<"VcrossWindow::getCrossectionOptions()" << endl;
#endif

  vcrossm->getCrossectionOptions(locationdata);

  return;
}

/***************************************************************************/

void VcrossWindow::updateCrossectionBox(){
  //update list of crossections in crossectionBox
#ifdef DEBUGPRINT
  cerr << "VcrossWindow::updateCrossectionBox" << endl;
#endif

  crossectionBox->clear();
  vector<miString> crossections= vcrossm->getCrossectionList();

  int n =crossections.size();
  for (int i=0; i<n; i++){
   crossectionBox->addItem(QString(crossections[i].c_str()));		
  }

}

/***************************************************************************/

void VcrossWindow::updateTimeBox(){
  //update list of times
#ifdef DEBUGPRINT
  cerr << "VcrossWindow::updateTimeBox" << endl;
#endif

  timeBox->clear();
  vector<miTime> times= vcrossm->getTimeList();

  int n =times.size();
  for (int i=0; i<n; i++){
    timeBox->addItem(QString(times[i].isoTime(false,false).cStr()));
  }

  emit emitTimes("vcross",times);
}

/***************************************************************************/

void VcrossWindow::crossectionBoxActivated(int index){

  //vector<miString> crossections= vcrossm->getCrossectionList();
  miString cbs=crossectionBox->currentText().latin1();
  //if (index>=0 && index<crossections.size()) {
  vcrossm->setCrossection(cbs);
  vcrossw->updateGL();
  QString sq = cbs.c_str();
  emit crossectionChanged(sq); //name of current crossection (to mainWindow)
  //}
}

/***************************************************************************/

void VcrossWindow::timeBoxActivated(int index){

  vector<miTime> times= vcrossm->getTimeList();

  if (index>=0 && index<times.size()) {
    vcrossm->setTime(times[index]);

    vcrossw->updateGL();
  }
}

/***************************************************************************/

bool VcrossWindow::changeCrossection(const miString& crossection){
#ifdef DEBUGPRINT
  cerr << "VcrossWindow::changeCrossection" << endl;
#endif
  vcrossm->setCrossection(crossection); //HK ??? should check if crossection exists ?
  vcrossw->updateGL();
  raise();
  if (crossectionChangedSlot(0))
    return true;
  else
    return false;
}


/***************************************************************************/

//void VcrossWindow::setFieldModels(const vector<miString>& fieldmodels){
//  vcrossm->setFieldModels(fieldmodels);
//  if (active) changeModel();
//}

/***************************************************************************/

void VcrossWindow::mainWindowTimeChanged(const miTime& t){
  if (!active) return; 
#ifdef DEBUGPRINT
  cerr << "vcrossWindow::mainWindowTimeChanged called with time " << t << endl;
#endif
  vcrossm->mainWindowTimeChanged(t);
  //get correct selection in comboboxes 
  crossectionChangedSlot(0);
  timeChangedSlot(0);
  vcrossw->updateGL();
}


/***************************************************************************/

void VcrossWindow::startUp(const miTime& t){
#ifdef DEBUGPRINT
  cerr << "vcrossWindow::startUp  t= " << t << endl;
#endif
  active = true;
  vcToolbar->show();
  tsToolbar->show();
  //do something first time we start Vertical crossections
  if (firstTime){
    //vector<miString> models;
    //define models for dialogs, comboboxes and crossectionplot
    //vcrossm->setSelectedModels(models, true,false);
    //vcDialog->setSelection();
    firstTime=false;
    // show default diagram without any data
    vcrossw->updateGL();
  }
  //changeModel();
  mainWindowTimeChanged(t);
}

/***************************************************************************/

vector<miString> VcrossWindow::writeLog(const miString& logpart)
{
  vector<miString> vstr;
  miString str;

  if (logpart=="window") {

    str= "VcrossWindow.size " + miString(this->width()) + " "
			      + miString(this->height());
    vstr.push_back(str);
    str= "VcrossWindow.pos "  + miString(this->x()) + " "
			      + miString(this->y());
    vstr.push_back(str);
    str= "VcrossDialog.pos "  + miString(vcDialog->x()) + " "
			      + miString(vcDialog->y());
    vstr.push_back(str);
    str= "VcrossSetupDialog.pos " + miString(vcSetupDialog->x()) + " "
			          + miString(vcSetupDialog->y());
    vstr.push_back(str);

    // printer name & options...
    if (priop.printer.exists()){
      str= "PRINTER " + priop.printer;
      vstr.push_back(str);
      if (priop.orientation==d_print::ori_portrait)
	str= "PRINTORIENTATION portrait";
      else
	str= "PRINTORIENTATION landscape";
      vstr.push_back(str);
    }

  } else if (logpart=="setup") {

    vstr= vcrossm->writeLog();

  } else if (logpart=="field") {

    vstr= vcDialog->writeLog();

  }

  return vstr;
}


void VcrossWindow::readLog(const miString& logpart, const vector<miString>& vstr,
			   const miString& thisVersion, const miString& logVersion,
			   int displayWidth, int displayHeight)
{

  if (logpart=="window") {

    vector<miString> tokens;
    int n= vstr.size();

    for (int i=0; i<n; i++) {
      tokens= vstr[i].split(' ');

      if (tokens.size()==3) {

        int x= atoi(tokens[1].c_str());
        int y= atoi(tokens[2].c_str());
        if (x>20 && y>20 && x<=displayWidth && y<=displayHeight) {
          if (tokens[0]=="VcrossWindow.size") this->resize(x,y);
        }
        if (x>=0 && y>=0 && x<displayWidth-20 && y<displayHeight-20) {
          if      (tokens[0]=="VcrossWindow.pos")      this->move(x,y);
          else if (tokens[0]=="VcrossDialog.pos")      vcDialog->move(x,y);
          else if (tokens[0]=="VcrossSetupDialog.pos") vcSetupDialog->move(x,y);
	}

      } else if (tokens.size()>=2) {

        if (tokens[0]=="PRINTER") {
          priop.printer=tokens[1];
        } else if (tokens[0]=="PRINTORIENTATION") {
	  if (tokens[1]=="portrait")
	    priop.orientation=d_print::ori_portrait;
	  else
	    priop.orientation=d_print::ori_landscape;
        }

      }
    }

  } else if (logpart=="setup") {

    vcrossm->readLog(vstr,thisVersion,logVersion);

  } else if (logpart=="field") {

    vcDialog->readLog(vstr,thisVersion,logVersion);

  }
}

/***************************************************************************/

bool VcrossWindow::close(bool alsoDelete){
  quitClicked();
  return true;
}
