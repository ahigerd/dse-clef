#ifndef D2W_DSEWINDOW_H
#define D2W_DSEWINDOW_H

#include "mainwindow.h"

class DSEWindow : public MainWindow
{
  Q_OBJECT
public:
  DSEWindow(ClefPluginBase* plugin);

public slots:
  void extract();

protected:
  virtual void populateFileMenu(QMenu* fileMenu);
  virtual QWidget* createPluginWidget(QWidget* parent);
};

#endif
