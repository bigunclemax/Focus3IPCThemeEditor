#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QtCore>
#include <QtGui>
#include <QtWidgets>
#include <QtConcurrent/QtConcurrent>

#include <VbfFile.h>
#include <EifConverter.h>

#include "HeaderObjectsModel.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
	Q_OBJECT

public:
	explicit MainWindow(QWidget *parent = nullptr);
	~MainWindow() override;

private slots:
	void slotOpen();
    void slotClose();
    void slotSave();
    void slotSaveAs();
	void on_lw_itemSelectionChanged();
    void onProgressChanged(QPoint progress);

signals:
    void progressChanged(QPoint progress);

private:

    HeaderObjectsModel m_model;

    struct sPictureIPC {
        int index;
        std::string name;
        uint16_t palette_crc;
        uint8_t  type;
        uint16_t width;
        uint16_t height;
        unique_ptr<EIF::EifImageBase> eif;
        QPixmap image_pixmap;
        bool changed = false;
    };

    std::map<int, sPictureIPC> images;

    static QString eitTypeToString(uint8_t eif_t);

    void reloadGui();
    void enableGui(bool doEnable);

    int exportAll(const QString &dest_dir);

    QString packVBF(const QString &path);
    QString unpackVBF();
    QPair<int, QString> ReplacePicture(int picture_idx, const QString &new_picture_path);

    static int
    compressVector(const vector<uint8_t> &data, const char *data_name, vector<uint8_t> &compressed_data);

    static void
    CompressAndReplaceEIF(ImageSection &img_sec, int idx, const vector<uint8_t> &res_bin,
                          const string &res_name);

    void unpackFinished();
    void exportFinished();
    void replaceFinished();
    void packFinished();

    Ui::MainWindow *ui;
	QLabel *label{};
	QScrollArea *scrollArea;
    VbfFile vbf;
    QString vbfPath;
    QThread thread;
    QFuture<QString> future;
    QFuture<int> futureExport;
    QFutureWatcher<QString> watcherUnpack;
    QFutureWatcher<int> watcherExportAll;
    QFuture<QPair<int, QString>> futureReplace;
    QFutureWatcher<QPair<int, QString>> watcherReplace;
    QFuture<QString> futurePack;
    QFutureWatcher<QString> watcherPack;
};

#endif // MAINWINDOW_H
