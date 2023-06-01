#include "dsewindow.h"
#include "extractdialog.h"
#include "plugin/baseplugin.h"
#include <QMenu>
#include <QtDebug>

DSEWindow::DSEWindow(S2WPluginBase* plugin)
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
