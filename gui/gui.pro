isEmpty(BUILDPATH) {
  error("BUILDPATH must be set")
}
BUILDPATH = $$absolute_path($$BUILDPATH)
include($$BUILDPATH/../seq2wav/gui/gui.pri)

SOURCES += main.cpp ../plugins/s2wplugin.cpp

HEADERS += dsewindow.h   extractdialog.h   channelwidget.h
SOURCES += dsewindow.cpp extractdialog.cpp channelwidget.cpp
