#include "extractdialog.h"
#include "s2wcontext.h"
#include "dseutil.h"
#include "mojibake.h"
#include <set>
#include <QMetaObject>
#include <QStackedWidget>
#include <QGridLayout>
#include <QLabel>
#include <QLineEdit>
#include <QToolButton>
#include <QPushButton>
#include <QCheckBox>
#include <QComboBox>
#include <QTableWidget>
#include <QHeaderView>
#include <QProgressBar>
#include <QMessageBox>
#include <QFileDialog>
#include <QDir>
#include <QFileInfo>
#include <QThread>

#if QT_CONFIG(cxx11_future)
#define qThreadCreate QThread::create
#else
namespace {
template <typename FN>
class QThreadRunner : public QThread
{
public:
  QThreadRunner(FN fn) : QThread(nullptr), fn(fn) {}

  void run() { fn(); }

  FN fn;
};
}
#define qThreadCreate new QThreadRunner
#endif

ExtractDialog::ScanResult::ScanResult(S2WContext* ctx, const std::vector<uint8_t>& buffer, int offset)
: dseFile(ctx, buffer, offset)
{
  filename = dseFile.originalFilename();
  if (!filename.size()) {
    filename = std::to_string(offset);
  }
  size_t dotPos = filename.find('.');
  if (dotPos != std::string::npos) {
    filename = filename.substr(0, dotPos);
  }
  if (!isUtf8(filename)) {
    std::string sjisFilename = tryShiftJIS(filename);
    if (sjisFilename.size() == 0 || !isUtf8(sjisFilename)) {
      filename = forceSafeAscii(filename);
    } else {
      filename = sjisFilename;
    }
  }
  originalName = filename;
}

ExtractDialog::ExtractDialog(S2WContext* ctx, QWidget* parent)
: QDialog(parent), scanning(false), ctx(ctx)
{
  setAttribute(Qt::WA_DeleteOnClose);

  QGridLayout* layout = new QGridLayout(this);
  layout->setColumnStretch(1, 1);

  QLabel* lblSource = new QLabel(tr("S&ource:"), this);
  txtSource = new QLineEdit(this);
  lblSource->setBuddy(txtSource);
  QToolButton* btnSource = new QToolButton(this);
  btnSource->setText("...");
  layout->addWidget(lblSource, 0, 0);
  layout->addWidget(txtSource, 0, 1);
  layout->addWidget(btnSource, 0, 2);

  QLabel* lblTarget = new QLabel(tr("&Target:"), this);
  txtTarget = new QLineEdit(this);
  lblTarget->setBuddy(txtTarget);
  QToolButton* btnTarget = new QToolButton(this);
  btnTarget->setText("...");
  layout->addWidget(lblTarget, 1, 0);
  layout->addWidget(txtTarget, 1, 1);
  layout->addWidget(btnTarget, 1, 2);

  stkScan = new QStackedWidget(this);
  btnScan = new QPushButton(tr("&Scan"), this);
  btnScan->setDefault(true);
  prgScan = new QProgressBar(this);
  stkScan->addWidget(btnScan);
  stkScan->addWidget(prgScan);
  layout->addWidget(stkScan, 2, 0, 1, 3);

  tblFiles = new QTableWidget(this);
  tblFiles->setSelectionMode(QTableWidget::ExtendedSelection);
  tblFiles->setSelectionBehavior(QTableWidget::SelectRows);
  tblFiles->setColumnCount(4);
  tblFiles->setHorizontalHeaderLabels({ "", tr("Filename"), tr("Title"), tr("Bank") });
  tblFiles->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
  tblFiles->horizontalHeader()->resizeSection(1, 200);
  tblFiles->horizontalHeader()->resizeSection(2, 200);
  tblFiles->horizontalHeader()->setStretchLastSection(true);
  tblFiles->verticalHeader()->hide();
  layout->addWidget(tblFiles, 3, 0, 1, 3);
  layout->setRowStretch(3, 1);

  QToolButton* btnToggle = new QToolButton(this);
  btnToggle->setText("\u00d7");
  layout->addWidget(btnToggle, 4, 2);

  chkRomaji = new QCheckBox(tr("Transliterate &romaji"), this);
  layout->addWidget(chkRomaji, 4, 1);

  chkTagsM3U = new QCheckBox(tr("&Generate !tags.m3u"), this);
  layout->addWidget(chkTagsM3U, 5, 1);

  QLabel* lblBank = new QLabel(tr("Main &Bank:"), this);
  cboBank = new QComboBox(this);
  lblBank->setBuddy(cboBank);
  layout->addWidget(lblBank, 6, 0);
  layout->addWidget(cboBank, 6, 1, 1, 2);

  btnExtract = new QPushButton(tr("E&xtract"), this);
  layout->addWidget(btnExtract, 7, 0, 1, 3);

  QPushButton* btnClose = new QPushButton(tr("Close"), this);
  layout->addWidget(btnClose, 8, 0, 1, 3);

  QObject::connect(btnSource, SIGNAL(clicked()), this, SLOT(browseSource()));
  QObject::connect(btnTarget, SIGNAL(clicked()), this, SLOT(browseTarget()));
  QObject::connect(btnScan, SIGNAL(clicked()), this, SLOT(scan()));
  QObject::connect(btnToggle, SIGNAL(clicked()), this, SLOT(toggleSelected()));
  QObject::connect(btnExtract, SIGNAL(clicked()), this, SLOT(extract()));
  QObject::connect(btnClose, SIGNAL(clicked()), this, SLOT(reject()));
  QObject::connect(txtSource, SIGNAL(textChanged(QString)), this, SLOT(updateEnabled()));
  QObject::connect(txtTarget, SIGNAL(textChanged(QString)), this, SLOT(updateEnabled()));
  QObject::connect(tblFiles, SIGNAL(cellChanged(int,int)), this, SLOT(updateEnabled()));
  QObject::connect(tblFiles, SIGNAL(cellChanged(int,int)), this, SLOT(updateData(int,int)));
  QObject::connect(chkTagsM3U, SIGNAL(clicked()), this, SLOT(updateEnabled()));
  QObject::connect(chkRomaji, SIGNAL(clicked()), this, SLOT(updateRomaji()));

  updateEnabled();
  resize(600, sizeHint().height());
}

void ExtractDialog::browseSource()
{
  QString path = QFileDialog::getOpenFileName(
    this,
    tr("Select Game"),
    QString(),
    "Nintendo DS Games (*.nds);;All Files (*)"
  );
  if (path.isEmpty()) {
    return;
  }
  txtSource->setText(path);
  if (txtTarget->text().isEmpty()) {
    QDir dir(path);
    dir.cdUp();
    txtTarget->setText(dir.absolutePath());
  }
}

void ExtractDialog::browseTarget()
{
  QString path = QFileDialog::getExistingDirectory(
    this,
    tr("Select Target Folder")
  );
  if (path.isEmpty()) {
    return;
  }
  txtTarget->setText(path);
}

void ExtractDialog::scan()
{
  scan(txtSource->text());
}

void ExtractDialog::extract()
{
  if (!scannedFiles.size()) {
    scan(txtSource->text());
    if (!scannedFiles.size()) {
      return;
    }
  }
  bool makeM3U = chkTagsM3U->isChecked();
  QFile m3u;
  QDir dir(txtTarget->text());
  if (makeM3U) {
    m3u.setFileName(dir.absoluteFilePath("!tags.m3u"));
    if (!m3u.open(QIODevice::WriteOnly)) {
      QMessageBox::critical(this, tr("dse2wav"), tr("Unable to open file for writing:\n%1").arg(m3u.fileName()));
      return;
    }
    QFileInfo src(txtSource->text());
    m3u.write(QStringLiteral("#@ALBUM %1\n").arg(src.completeBaseName()).toUtf8());
    if (cboBank->currentIndex() >= 0) {
      m3u.write(QStringLiteral("#@BANK %1\n").arg(cboBank->currentText()).toUtf8());
    }
  }
  int numRows = tblFiles->rowCount();
  for (int i = 0; i < numRows; i++) {
    QTableWidgetItem* item = tblFiles->item(i, 0);
    if (!item || !item->data(Qt::CheckStateRole).toBool()) {
      continue;
    }
    item = tblFiles->item(i, 1);
    if (!item || item->text().isEmpty()) {
      continue;
    }
    QFile dest(dir.absoluteFilePath(item->text()));
    if (!dest.open(QIODevice::WriteOnly)) {
      QMessageBox::critical(this, tr("dse2wav"), tr("Unable to open file for writing:\n%1").arg(dest.fileName()));
      return;
    }
    const DSEFile* dseFile = &scannedFiles[i]->dseFile;
    const std::vector<uint8_t>& data = dseFile->getData();
    dest.write((const char*)data.data(), data.size());
    dest.close();
    if (makeM3U) {
      std::string extension = magicString(dseFile->magic()).substr(0, 3);
      if (extension == "smd" || extension == "sad") {
        QTableWidgetItem* title = tblFiles->item(i, 2);
        QTableWidgetItem* pair = extension == "smd" ? tblFiles->item(i, 3) : nullptr;
        m3u.write("\n");
        if (title && !title->text().isEmpty()) {
          m3u.write(QStringLiteral("#%TITLE %1\n").arg(title->text()).toUtf8());
        }
        if (pair && !pair->text().isEmpty()) {
          m3u.write(QStringLiteral("#%PAIR %1\n").arg(pair->text()).toUtf8());
        }
        m3u.write(QStringLiteral("%1\n").arg(item->text()).toUtf8());
      }
    }
  }
  if (makeM3U) {
    m3u.close();
  }
  accept();
}

void ExtractDialog::updateEnabled()
{
  bool noSource = txtSource->text().isEmpty() || !QFileInfo::exists(txtSource->text());
  bool noTarget = txtTarget->text().isEmpty() || !QFileInfo(txtTarget->text()).isDir();
  btnScan->setEnabled(!noSource);
  btnExtract->setEnabled(!noSource && !noTarget);
  cboBank->setEnabled(chkTagsM3U->isChecked());
}

void ExtractDialog::scan(const QString& path)
{
  scannedFiles.clear();
  setEnabled(false);
  prgScan->setValue(0);
  stkScan->setCurrentWidget(prgScan);

  QThread* worker = qThreadCreate([this, path]{
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) {
      // TODO: error
      return;
    }
    std::vector<uint8_t> buffer;
    {
      QByteArray qbuf = f.readAll();
      buffer = std::vector<uint8_t>(qbuf.begin(), qbuf.end());
    }
    int end = buffer.size() - 64;
    QMetaObject::invokeMethod(prgScan, "setRange", Qt::QueuedConnection, Q_ARG(int, 0), Q_ARG(int, end));
    std::set<std::string> usedNames;
    for (int i = 0; i < end; i++) {
      if (buffer[i] == 0) {
        continue;
      }
      uint64_t magic64 = parseIntBE<uint64_t>(buffer, i);
      if (!isValidMagic64(magic64) || magic64 & 0xFFFF) {
        continue;
      }
      try {
        std::unique_ptr<ScanResult> scanResult(new ScanResult(ctx, buffer, i));
        std::string filename = scanResult->filename;
        std::string baseRomaji = transliterateRomaji(filename);
        std::string baseFilename = filename;
        std::string extension = "." + magicString(scanResult->dseFile.magic()).substr(0, 3);
        filename += extension;
        char counter = '1';
        while (usedNames.count(filename)) {
          filename = baseFilename + "_" + counter + extension;
          ++counter;
        }
        usedNames.insert(filename);
        if (baseRomaji != baseFilename) {
          std::string romajiFilename = baseRomaji + extension;
          char counter = '1';
          while (usedNames.count(romajiFilename)) {
            romajiFilename = baseRomaji + "_" + counter + extension;
            ++counter;
          }
          usedNames.insert(romajiFilename);
          scanResult->romajiFilename = romajiFilename;
        }
        scanResult->filename = filename;
        scannedFiles.push_back(std::move(scanResult));
      } catch (std::exception& err) {
        // consume errors silently because they indicate that there wasn't a (good) file there
        if (std::string(err.what()).find("unknown file type") == std::string::npos) {
          std::cout << "With " << magicString(magic64) << " header: " << err.what() << std::endl;
        }
      }
      QMetaObject::invokeMethod(prgScan, "setValue", Qt::QueuedConnection, Q_ARG(int, i));
    }
  });
  QObject::connect(worker, SIGNAL(finished()), this, SLOT(scanFinished()));
  QObject::connect(worker, SIGNAL(finished()), worker, SLOT(deleteLater()));
  worker->start();
}

void ExtractDialog::scanFinished()
{
  setEnabled(true);
  stkScan->setCurrentWidget(btnScan);
  tblFiles->clearContents();
  cboBank->clear();
  if (!scannedFiles.size()) {
    btnScan->setDefault(true);
    btnExtract->setDefault(false);
    return;
  }
  btnScan->setDefault(false);
  btnExtract->setDefault(true);
  tblFiles->setRowCount(scannedFiles.size());
  int rowIndex = 0;
  for (const auto& row : scannedFiles) {
    QTableWidgetItem* check = new QTableWidgetItem();
    check->setFlags(check->flags() | Qt::ItemIsUserCheckable);
    check->setData(Qt::CheckStateRole, Qt::Checked);
    tblFiles->setItem(rowIndex, 0, check);
    tblFiles->setItem(rowIndex, 1, new QTableWidgetItem());
    std::string extension = magicString(scannedFiles[rowIndex]->dseFile.magic()).substr(0, 3);
    if (extension == "smd" || extension == "sad") {
      tblFiles->setItem(rowIndex, 2, new QTableWidgetItem(QString::fromStdString(row->originalName)));
    } else {
      tblFiles->setItem(rowIndex, 2, new QTableWidgetItem());
    }
    tblFiles->setItem(rowIndex, 3, new QTableWidgetItem());
    rowIndex++;
  }
  updateRomaji();
}

void ExtractDialog::toggleSelected()
{
  std::set<int> rows;
  for (const auto& item : tblFiles->selectedItems()) {
    rows.insert(item->row());
  }
  for (int row : rows) {
    QTableWidgetItem* item = tblFiles->item(row, 0);
    item->setData(Qt::CheckStateRole, item->data(Qt::CheckStateRole).toBool() ? Qt::Unchecked : Qt::Checked);
  }
}

void ExtractDialog::updateRomaji()
{
  int rowIndex = 0;
  bool romaji = chkRomaji->isChecked();
  int bankIndex = cboBank->currentIndex();
  cboBank->clear();
  std::set<std::string> names;
  for (const auto& row : scannedFiles) {
    names.insert((romaji && row->romajiFilename.size()) ? row->romajiFilename : row->filename);
    cboBank->addItem(QString::fromStdString(row->filename));
  }
  cboBank->setCurrentIndex(bankIndex);
  for (const auto& row : scannedFiles) {
    std::string fn = (romaji && row->romajiFilename.size()) ? row->romajiFilename : row->filename;
    tblFiles->setItem(rowIndex, 1, new QTableWidgetItem(QString::fromStdString(fn)));
    QString pair;
    if (fn.substr(fn.size() - 4) == ".smd") {
      std::string pairPath = fn.substr(0, fn.size() - 4) + ".swd";
      if (names.count(pairPath)) {
        pair = QString::fromStdString(pairPath);
      }
    }
    tblFiles->setItem(rowIndex, 3, new QTableWidgetItem(pair));
    rowIndex++;
  }
}

void ExtractDialog::updateData(int row, int column)
{
  if (column != 1) {
    return;
  }
  cboBank->setItemText(row, tblFiles->item(row, 1)->text());
}
