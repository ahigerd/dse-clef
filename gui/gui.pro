isEmpty(BUILDPATH) {
  error("BUILDPATH must be set")
}
BUILDPATH = $$absolute_path($$BUILDPATH)
include($$BUILDPATH/../libclef/gui/gui.pri)

SOURCES += main.cpp ../plugins/clefplugin.cpp

HEADERS += dsewindow.h   extractdialog.h   channelwidget.h
SOURCES += dsewindow.cpp extractdialog.cpp channelwidget.cpp
