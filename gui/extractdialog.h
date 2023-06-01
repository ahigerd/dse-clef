#ifndef D2W_EXTRACTDIALOG_H
#define D2W_EXTRACTDIALOG_H

#include <QDialog>
#include <vector>
#include <memory>
#include "dsefile.h"
class S2WContext;
class QLineEdit;
class QTableWidget;
class QCheckBox;
class QPushButton;
class QComboBox;
class QStackedWidget;
class QProgressBar;

class ExtractDialog : public QDialog
{
  Q_OBJECT
public:
  ExtractDialog(S2WContext* ctx, QWidget* parent = nullptr);

private slots:
  void browseSource();
  void browseTarget();
  void scan();
  void extract();
  void updateRomaji();
  void toggleSelected();
  void updateEnabled();
  void scanFinished();
  void updateData(int row, int column);

private:
  void scan(const QString& path);

  QLineEdit* txtSource;
  QLineEdit* txtTarget;
  QStackedWidget* stkScan;
  QPushButton* btnScan;
  QProgressBar* prgScan;
  QTableWidget* tblFiles;
  QCheckBox* chkRomaji;
  QCheckBox* chkTagsM3U;
  QComboBox* cboBank;
  QPushButton* btnExtract;
  bool scanning;

  struct ScanResult {
    ScanResult(S2WContext* ctx, const std::vector<uint8_t>& buffer, int offset);
    DSEFile dseFile;
    std::string originalName;
    std::string romajiFilename;
    std::string filename;
  };
  std::vector<std::unique_ptr<ScanResult>> scannedFiles;
  S2WContext* ctx;
};

#endif
