//===========================================
//  Lumina Desktop Source Code
//  Copyright (c) 2016, Ken Moore
//  Available under the 3-clause BSD license
//  See the LICENSE file for full details
//===========================================
#include "mainWindow.h"
#include "ui_mainWindow.h"
#include "globals.h"

#include "pages/getPage.h"

//=============
//      PUBLIC
//=============
mainWindow::mainWindow() : QMainWindow(), ui(new Ui::mainWindow()){
  ui->setupUi(this);
  cpage = "somerandomjunktostartwith";
  //Need to insert a spacer action in the toolbar
  QWidget *tmp = new QWidget(this);
    tmp->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
  ui->toolBar->insertWidget(ui->actionSave, tmp); //after the save button
  backShortcut = new QShortcut(Qt::Key_Escape, this);
    connect(backShortcut, SIGNAL(activated()), this, SLOT(on_actionBack_triggered()) );
  setupIcons();
  loadMonitors();
  changePage(""); //load the default main page
}

mainWindow::~mainWindow(){

}

//==============
//  PUBLIC SLOTS
//==============
void mainWindow::slotSingleInstance(){
  this->showNormal(); //just in case it is hidden/minimized
}

void mainWindow::setupIcons(){
  this->setWindowIcon( LXDG::findIcon("preferences-desktop") );
  ui->actionSave->setIcon( LXDG::findIcon("document-save","") );
  ui->actionBack->setIcon( LXDG::findIcon("go-previous-view","") );
  ui->actionMonitor->setIcon(LXDG::findIcon("preferences-desktop-display","") );
}

void mainWindow::loadMonitors(){
  if(ui->actionMonitor->menu()==0){ 
    ui->actionMonitor->setMenu( new QMenu(this) );
    ui->actionMonitor->setWhatsThis("0");
    connect( ui->actionMonitor->menu(), SIGNAL(triggered(QAction*)), this, SLOT(changeMonitor(QAction*)) );
    QToolButton *b = static_cast<QToolButton*>(ui->toolBar->widgetForAction(ui->actionMonitor));
    b->setPopupMode(QToolButton::InstantPopup);
  }
  int cnum = ui->actionMonitor->whatsThis().toInt();
  ui->actionMonitor->menu()->clear();
  QList<QScreen*> SL = QApplication::screens();
  for(int i=0; i<SL.length(); i++){
    QAction *tmp = ui->actionMonitor->menu()->addAction( QString("%1: %2").arg(QString::number(i), SL[i]->name()) );
      tmp->setWhatsThis(QString::number(i));
      if(i==cnum || (i==0 && cnum>= SL.length()) ){
        ui->actionMonitor->setText( tmp->text() );
        ui->actionMonitor->setWhatsThis(tmp->whatsThis() );
      }
  }
  
}

//=============
//      PRIVATE
//=============
void mainWindow::changePage(QString id){
  PageWidget *page = 0;
  if(id!=cpage){
    page =  GetNewPage(id, this);
    if(page==0){ return; }
    qDebug() << "Changing page:" << id;
    cpage = id;
    QWidget *old = this->centralWidget();
    this->setCentralWidget(page);
    if(old!=0 && old!=ui->centralwidget){ old->disconnect(); old->deleteLater(); }
    //Connect the new page
    connect(page, SIGNAL(HasPendingChanges(bool)), this, SLOT(pageCanSave(bool)) );
    connect(page, SIGNAL(ChangePageTitle(QString)), this, SLOT(pageSetTitle(QString)) );
    connect(page, SIGNAL(ChangePage(QString)), this, SLOT(page_change(QString)) );
    page->setFocus();
    ui->toolBar->setVisible( !cpage.isEmpty() );
  }else{
    //No change in page (some other refresh)
    // just re-use the current widget
    page = static_cast<PageWidget*>(this->centralWidget());
  }
  //Now load the new page
  page->LoadSettings(ui->actionMonitor->whatsThis().toInt()); //need to make this show the current screen as needed
  //Now update this UI a bit based on page settings
  ui->actionMonitor->setVisible( page->needsScreenSelector() && ui->actionMonitor->menu()->actions().length()>1 );
  
  this->showNormal();
}
//================
//  PRIVATE SLOTS
//================
//Page signal handling
void mainWindow::pageCanSave(bool save){
  ui->actionSave->setVisible(save);
  ui->actionSave->setEnabled(save);
}

void mainWindow::pageSetTitle(QString title){
  this->setWindowTitle(title);
}

bool mainWindow::page_change(QString id){
  if(ui->actionSave->isEnabled()){
    //unsaved changed available - prompt to save first
    QMessageBox::StandardButton result = QMessageBox::question(this, tr("Unsaved Changes"), tr("This page currently has unsaved changes, do you wish to save them now?"),  QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel, QMessageBox::No);
    if(result == QMessageBox::Yes){ on_actionSave_triggered(); }
    else if(result == QMessageBox::Cancel){ return false; } //stop now
  }
  changePage(id);
  return true;
}

void mainWindow::on_actionSave_triggered(){
  pageCanSave(false); //disable for the moment (page might re-enable later)
  static_cast<PageWidget*>(this->centralWidget())->SaveSettings();

}

void mainWindow::on_actionBack_triggered(){
  if(cpage.isEmpty()){ this->close(); } //main menu - go ahead and close it
  else{ page_change(""); } //Use the interactive wrapper (check for save state, etc).
}

void mainWindow::changeMonitor(QAction *act){
  QString oldWT = ui->actionMonitor->whatsThis();
  //Update the current selection shown on the button
  ui->actionMonitor->setWhatsThis( act->whatsThis() );
  //Now prompt the current page to re-load settings
  if( page_change(cpage) ){  ui->actionMonitor->setText(act->text()); }
  else{ ui->actionMonitor->setWhatsThis(oldWT); } //cancelled - go back to old setting
}
