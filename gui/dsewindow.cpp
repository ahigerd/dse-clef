#include "dsewindow.h"
#include "extractdialog.h"
#include "channelwidget.h"
#include "playercontrols.h"
#include "plugin/baseplugin.h"
#include <QMenu>
#include <QtDebug>

DSEWindow::DSEWindow(ClefPluginBase* plugin)
: MainWindow(plugin)
{
  resize(400, 300);
}

void DSEWindow::populateFileMenu(QMenu* fileMenu)
{
  fileMenu->addAction("Ex&tract...", this, SLOT(extract()));
}

void DSEWindow::extract()
{
  ExtractDialog* dlg = new ExtractDialog(m_plugin->context(), this);
  dlg->open();
}

QWidget* DSEWindow::createPluginWidget(QWidget* parent)
{
  ChannelWidget* cw = new ChannelWidget(parent);
  QObject::connect(controls, SIGNAL(bufferUpdated()), cw, SLOT(updateMeters()));
  return cw;
}
