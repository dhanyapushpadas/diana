
#include "WebMapDialog.h"

#include "WebMapManager.h"
#include "WebMapService.h"

#include <QAction>
#include <QSortFilterProxyModel>
#include <QStringList>
#include <QStringListModel>

#include <sstream>

#include "wmsclient/webmap_dialog.ui.h"

#define MILOGGER_CATEGORY "diana.WebMapDialog"
#include <miLogger/miLogging.h>

WebMapDialog::WebMapDialog(QWidget *parent, Controller *ctrl)
  : DataDialog(parent, ctrl)
  , ui(new Ui_WebMapDialog)
  , mAddSelectedService(0)
{
  METLIBS_LOG_SCOPE();
  setupUi();
  m_action = new QAction("Web Maps", this);
  m_action->setCheckable(true);

  initializeAddServicePage(true);
}

WebMapDialog::~WebMapDialog()
{
}

void WebMapDialog::setupUi()
{
  METLIBS_LOG_SCOPE();
  ui->setupUi(this);

  mServicesModel = new QStringListModel(this);
  mServicesFilter = new QSortFilterProxyModel(this);
  mServicesFilter->setFilterCaseSensitivity(Qt::CaseInsensitive);
  mServicesFilter->setSourceModel(mServicesModel);
  ui->listAddServices->setModel(mServicesFilter);
  connect(ui->editAddServiceFilter, SIGNAL(textChanged(const QString&)),
      this, SLOT(onAddServicesFilter(const QString&)));

  connect(ui->listAddServices->selectionModel(), SIGNAL(selectionChanged(const QItemSelection &, const QItemSelection&)),
      this, SLOT(checkAddComplete()));
  connect(ui->listAddServices, SIGNAL(activated(const QModelIndex&)),
      this, SLOT(onAddNext()));

  mLayersModel = new QStringListModel(this);
  mLayersFilter = new QSortFilterProxyModel(this);
  mLayersFilter->setFilterCaseSensitivity(Qt::CaseInsensitive);
  mLayersFilter->setSourceModel(mLayersModel);
  ui->listAddLayers->setModel(mLayersFilter);
  connect(ui->editAddLayerFilter, SIGNAL(textChanged(const QString&)),
      this, SLOT(onAddLayersFilter(const QString&)));

  connect(ui->listAddLayers->selectionModel(), SIGNAL(selectionChanged(const QItemSelection &, const QItemSelection&)),
      this, SLOT(checkAddComplete()));
  connect(ui->listAddLayers, SIGNAL(activated(const QModelIndex&)),
      this, SLOT(onAddNext()));

  connect(ui->buttonNextAdd, SIGNAL(clicked()),
      this, SLOT(onAddNext()));
  connect(ui->buttonBack, SIGNAL(clicked()),
      this, SLOT(onAddBack()));
  connect(ui->buttonRestart, SIGNAL(clicked()),
      this, SLOT(onAddRestart()));
}

void WebMapDialog::initializeAddServicePage(bool forward)
{
  METLIBS_LOG_SCOPE();
  ui->stackAdd->setCurrentIndex(AddServicePage);

  if (mAddSelectedService) {
    disconnect(mAddSelectedService, SIGNAL(refreshStarting()),
        this, SLOT(onServiceRefreshStarting()));
    disconnect(mAddSelectedService, SIGNAL(refreshFinished()),
        this, SLOT(onServiceRefreshFinished()));
    mAddSelectedService = 0;
  }

  if (forward) {
    ui->editAddServiceFilter->clear();
    QStringList services;
    if (WebMapManager* wmm = WebMapManager::instance()) {
      for (int i=0; i<wmm->getServiceCount(); ++i) {
        const WebMapService* wms = wmm->getService(i);
        services << QString::fromStdString
            ((wms->title().empty()) ? wms->identifier() : wms->title());
      }
    }
    METLIBS_LOG_DEBUG(LOGVAL(services.size()));
    mServicesModel->setStringList(services);
  }

  ui->buttonRestart->setEnabled(false);
  ui->buttonBack->setEnabled(false);
  checkAddComplete();
}

void WebMapDialog::onAddServicesFilter(const QString& text)
{
  mServicesFilter->setFilterFixedString(text);
}

bool WebMapDialog::isAddServiceComplete()
{
  return (not ui->listAddServices->selectionModel()->selectedIndexes().isEmpty());
}

WebMapService* WebMapDialog::selectedAddService() const
{
  METLIBS_LOG_SCOPE();
  WebMapManager* wmm = WebMapManager::instance();
  if (!wmm)
    return 0;

  const QModelIndexList si = ui->listAddServices->selectionModel()->selectedIndexes();
  if (si.size() != 1)
    return 0;

  const int i = mServicesFilter->mapToSource(si.at(0)).row();
  if (i >= wmm->getServiceCount())
    return 0;

  return wmm->getService(i);
}

void WebMapDialog::initializeAddLayerPage(bool forward)
{
  METLIBS_LOG_SCOPE();
  ui->stackAdd->setCurrentIndex(AddLayerPage);

  if (forward) {
    assert(!mAddSelectedService);
    mAddSelectedService = selectedAddService();
    if (mAddSelectedService) {
      connect(mAddSelectedService, SIGNAL(refreshStarting()),
          this, SLOT(onServiceRefreshStarting()));
      connect(mAddSelectedService, SIGNAL(refreshFinished()),
          this, SLOT(onServiceRefreshFinished()));
      mAddSelectedService->refresh();
      ui->labelAddChooseLayer->setText(tr("Choose layer in service %1:")
          .arg(QString::fromStdString(mAddSelectedService->identifier())));
    }
    ui->editAddLayerFilter->clear();

    updateAddLayers();
  }

  ui->buttonRestart->setEnabled(true);
  ui->buttonBack->setEnabled(true);
  checkAddComplete();
}

void WebMapDialog::onServiceRefreshStarting()
{
  mLayersModel->setStringList(QStringList());
  checkAddComplete();
}

void WebMapDialog::onServiceRefreshFinished()
{
  updateAddLayers();
}

void WebMapDialog::updateAddLayers()
{
  QStringList layers;
  if (mAddSelectedService) {
    for (int i=0; i < (int)mAddSelectedService->countLayers(); ++i)
      layers << QString::fromStdString(mAddSelectedService->layer(i)->title());
  }
  mLayersModel->setStringList(layers);
  checkAddComplete();
}

void WebMapDialog::onAddLayersFilter(const QString& text)
{
  mLayersFilter->setFilterFixedString(text);
}

bool WebMapDialog::isAddLayerComplete()
{
  return (not ui->listAddLayers->selectionModel()->selectedIndexes().isEmpty());
}

void WebMapDialog::checkAddComplete()
{
  METLIBS_LOG_SCOPE();
  const int page = ui->stackAdd->currentIndex();
  if (page == AddServicePage) {
    ui->buttonNextAdd->setText(tr("Next"));
    ui->buttonNextAdd->setEnabled(isAddServiceComplete());
  } else if (page == AddLayerPage) {
    ui->buttonNextAdd->setText(tr("Add"));
    ui->buttonNextAdd->setEnabled(isAddLayerComplete());
  }
}

const WebMapLayer* WebMapDialog::selectedAddLayer() const
{
  const WebMapService* wms = selectedAddService();
  const QModelIndexList si = ui->listAddLayers->selectionModel()->selectedIndexes();
  if (!wms || si.size() != 1)
    return 0;

  const int i = mLayersFilter->mapToSource(si.at(0)).row();
  if (i >= (int)wms->countLayers())
    return 0;
  return wms->layer(i);
}

void WebMapDialog::onAddNext()
{
  METLIBS_LOG_SCOPE();
  const int page = ui->stackAdd->currentIndex();
  if (page == AddServicePage) {
    if (isAddServiceComplete())
      initializeAddLayerPage(true);
  } else if (page == AddLayerPage) {
    addSelectedLayer();
    ui->listAddLayers->selectionModel()->clear();
  }
}

void WebMapDialog::onAddBack()
{
  METLIBS_LOG_SCOPE();
  const int page = ui->stackAdd->currentIndex();
  if (page == AddLayerPage)
    initializeAddServicePage(false);
}

void WebMapDialog::onAddRestart()
{
  initializeAddLayerPage(true);
}

void WebMapDialog::addSelectedLayer()
{
  const WebMapLayer* layer = selectedAddLayer();
  if (!isAddLayerComplete() || !mAddSelectedService || !layer)
    return;

  std::ostringstream add;
  add << "WEBMAP webmap.service=" << mAddSelectedService->identifier()
      << " webmap.layer=" << layer->identifier()
      << " style.alpha_scale=0.75";
  mOk.push_back(add.str());
  Q_EMIT applyData();
}

void WebMapDialog::onModifyApply()
{
}

void WebMapDialog::onModifyLayerSelected()
{
}

std::string WebMapDialog::name() const
{
  static const std::string WEBMAP = "WEBMAP";
  return WEBMAP;
}

void WebMapDialog::updateDialog()
{
}

std::vector<std::string> WebMapDialog::getOKString()
{
  return mOk;
}

void WebMapDialog::putOKString(const std::vector<std::string>& ok)
{
  mOk = ok;
}

void WebMapDialog::updateTimes()
{
}