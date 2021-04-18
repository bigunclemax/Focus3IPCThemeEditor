#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "version.h"

#include <miniz_zip.h>
#include <CRC.h>
#include <filesystem>

namespace fs = std::filesystem;

MainWindow::MainWindow(QWidget *parent) :
	QMainWindow(parent),
	ui(new Ui::MainWindow)
{
	ui->setupUi(this);

	ui->statusBar->addPermanentWidget(ui->label_Width);
    ui->statusBar->addPermanentWidget(ui->label_Height);
    ui->statusBar->addPermanentWidget(ui->label_Type);
    ui->statusBar->addWidget(ui->label_Status);
    ui->menuBar->addAction("About", this, [this]() {

        QString about_str = QString(R"about(
Version: %1%4
Date: %2

%3)about").arg(GIT_DESCRIBE, GIT_COMMIT_DATE_ISO8601, GIT_AUTHOR_EMAIL, GIT_IS_DIRTY ? "-dirty" : "");

        QMessageBox::about(this, "Ford focus mk 3.* IPC theme editor", about_str);
    });

    connect(&watcherUnpack, &QFutureWatcher<int>::finished, this, &MainWindow::unpackFinished);
    connect(&watcherExportAll, &QFutureWatcher<int>::finished, this, &MainWindow::exportFinished);
    connect(this, &MainWindow::progressChanged, this, &MainWindow::onProgressChanged);

	connect(ui->actionOpen, &QAction::triggered, this, &MainWindow::slotOpen);
    connect(ui->actionClose, &QAction::triggered, this, &MainWindow::slotClose);
    connect(ui->actionSave, &QAction::triggered, this, &MainWindow::slotSave);
    connect(ui->actionSave_As, &QAction::triggered, this, &MainWindow::slotSaveAs);
    connect(ui->pushButton_exportAll, QOverload<bool>::of(&QPushButton::clicked),[this]()
    {
        if (images.empty()) return;

        auto dest_dir = QFileDialog::getExistingDirectory(nullptr, tr("Export all images"));
        if (dest_dir.isEmpty()) return;

        enableGui(false);

        futureExport = QtConcurrent::run(this, &MainWindow::exportAll, dest_dir);
        watcherExportAll.setFuture(futureExport);
    });
    connect(ui->pushButton_exportImage, QOverload<bool>::of(&QPushButton::clicked),[this]()
    {
        QList<QListWidgetItem*> list = ui->lw->selectedItems();
        if (list.isEmpty())
        {
            QMessageBox(QMessageBox::Information,
                        "", "Select any picture", QMessageBox::Ok, this).exec();
            return;
        }
        auto picture_idx = list.at(0)->data(Qt::UserRole).toInt();
        if(images.find(picture_idx) != images.end())
        {
            auto& picture = images[picture_idx];

            auto store_path = QFileDialog::getSaveFileName(this, tr("Export images"),
                                                           fs::path(picture.name).replace_extension(".bmp").string().c_str(),
                                                           tr("BMP image (*.bmp);;All Files (*)"));

            if (store_path.isEmpty()) return;

            picture.eif->saveBmp(store_path.toStdString());
        }
    });
    connect(ui->pushButton_replaceImage, QOverload<bool>::of(&QPushButton::clicked),[this]()
    {
        try {
            if(!vbf.IsOpen()) {
                throw runtime_error("VBF not open");
            }

            QList<QListWidgetItem*> list = ui->lw->selectedItems();
            if (list.isEmpty())
            {
                throw runtime_error("Select any picture");
            }
            auto picture_idx = list.at(0)->data(Qt::UserRole).toInt();

            if(images.find(picture_idx) == images.end()) {
                throw runtime_error("Wrong selected picture");
            }

            auto new_picture_path = QFileDialog::getOpenFileName(this,
                                                                 tr("Replace picture"), "", tr("Image (*.bmp)"));
            if(new_picture_path.isEmpty()) return;

            enableGui(false);

            ui->label_Status->setText("Replacing picture...");
            future = QtConcurrent::run(this, &MainWindow::ReplacePicture, picture_idx, new_picture_path);
            watcherUnpack.setFuture(future);

        } catch (const std::runtime_error& ex) {
            QMessageBox(QMessageBox::Information,
                        "", ex.what(), QMessageBox::Ok, this).exec();
            return;
        }
    });

	scrollArea = new QScrollArea();

	QSizePolicy sizePolicy = QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
	sizePolicy.setHorizontalStretch(6);
	scrollArea->setSizePolicy(sizePolicy);


	scrollArea->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
	ui->horizontalLayout->addWidget(scrollArea);

}

MainWindow::~MainWindow()
{
	delete ui;
}

void MainWindow::slotOpen()
{
    vbfPath = QFileDialog::getOpenFileName(this,
                                                tr("Open VBF"), "", tr("VBF File (*.vbf)"));

    if(vbfPath.isEmpty()) return;

    try {
        //unpack vbf and get section with image resources
        vbf.OpenFile(vbfPath.toStdString());
        if(!vbf.IsOpen()) {
            throw runtime_error("Can't parse VBF");
        }

        enableGui(false);
        future = QtConcurrent::run(this, &MainWindow::unpackVBF);
        watcherUnpack.setFuture(future);

    } catch (const runtime_error& ex) {
        QMessageBox(QMessageBox::Warning,
                    "", ex.what(), QMessageBox::Ok, this).exec();
    }
}

void MainWindow::slotClose() {

    if(vbf.IsOpen()) {

        images.clear();

        ui->lw->clear();
        label = new QLabel();
        scrollArea->setWidget(label);
        ui->label_Width->setText("");
        ui->label_Height->setText("");
        ui->label_Type->setText("");
        ui->label_Status->setText("No file selected");

        enableGui(false);
    }
}

void MainWindow::slotSave() {

    if (vbf.IsOpen()) {
        vbf.SaveToFile(vbfPath.toStdString());
    }
}

void MainWindow::slotSaveAs() {

    if(vbf.IsOpen()) {
        auto store_path = QFileDialog::getSaveFileName(this, tr("Save VBF"),
                                                       "",
                                                       tr("VBF file (*.vbf);;All Files (*)"));

        if (store_path.isEmpty()) return;

        vbf.SaveToFile(store_path.toStdString());
    }
}

void MainWindow::on_lw_itemSelectionChanged()
{
	QList<QListWidgetItem*> list = ui->lw->selectedItems();
	if (!list.isEmpty()) {
		auto picture_idx = list.at(0)->data(Qt::UserRole);
		if(images.find(picture_idx.toInt()) != images.end()) {
            auto& picture = images[picture_idx.toInt()];
            label = new QLabel();
            label->setPixmap(picture.image_pixmap);
            scrollArea->setWidget(label);
            ui->label_Width->setText("Width: " + QString::number(picture.width));
            ui->label_Height->setText("Height: " + QString::number(picture.height));
            ui->label_Type->setText(eitTypeToString(picture.type));
		}
	}
}

vector<uint8_t> unzipEIF(const std::vector<uint8_t>& zipped_data, std::string *p_eif_name = nullptr) {

    // unzip EIF
    mz_zip_archive zip_archive{};
    if (!mz_zip_reader_init_mem(&zip_archive,
                                (const void *) zipped_data.data(), zipped_data.size(), 0)) {
        throw runtime_error("Can't get image name form archive");
    }
    mz_zip_archive_file_stat file_stat;
    mz_zip_reader_file_stat(&zip_archive, 0, &file_stat);
    vector<uint8_t> eif(file_stat.m_uncomp_size);
    mz_zip_reader_extract_to_mem(&zip_archive, 0, (void *) eif.data(), eif.size(), 0);
    mz_zip_reader_end(&zip_archive);

    if (nullptr != p_eif_name) {
        *p_eif_name = file_stat.m_filename;
    }

    return eif;
}

QString MainWindow::unpackVBF() {

    images.clear();

    std::vector<uint8_t> img_sec_bin;
    if(vbf.GetSectionRaw(1, img_sec_bin)) {
        throw runtime_error("Can't get image section");
    }

    //parse image section
    ImageSection section;
    section.Parse(img_sec_bin);

    int zipped_items = section.GetItemsCount(ImageSection::RT_ZIP);

    for(int i = 0; i < zipped_items; i++) {

        progressChanged({i, zipped_items});
        auto& picture = images[i];

        // get zipped EIF from image section
        std::vector<uint8_t> img_zip_bin;
        section.GetItemData(ImageSection::RT_ZIP, i, img_zip_bin);

        std::string eif_name;
        auto eif_data = unzipEIF(img_zip_bin, &eif_name);

        picture.eif = EIF::EifConverter::makeEif(static_cast<EIF::EIF_TYPE>(eif_data[7]));
        picture.eif->openEif(eif_data);
        auto bitmap = picture.eif->getBitmapRBGA();

        picture.image_pixmap = QPixmap::fromImage(
                QImage(bitmap.data(), picture.eif->getWidth(), picture.eif->getHeight(),
                       QImage::Format_RGBA8888).convertToFormat(QImage::Format_ARGB32));

        if (picture.eif->getType() == EIF_TYPE_MULTICOLOR) {
            auto crc16 = CRC::Calculate((char *) eif_data.data() + 0x10, 768, CRC::CRC_16_CCITTFALSE());
            picture.palette_crc = crc16;
        }

        picture.index = i;
        picture.name = eif_name;
        picture.type = picture.eif->getType();
        picture.width = picture.eif->getWidth();
        picture.height = picture.eif->getHeight();

    }

    return "";
}

int MainWindow::compressVector(const std::vector<uint8_t> &data, const char *data_name,
                               std::vector<uint8_t> &compressed_data) {

    mz_bool status;
    mz_zip_archive zip_archive = {};
    unsigned flags = MZ_DEFAULT_LEVEL | MZ_ZIP_FLAG_ASCII_FILENAME;

    status = mz_zip_writer_init_heap(&zip_archive, 0, 0);
    if (!status) {
        cerr << "mz_zip_writer_init_heap failed!";
        return -1;
    }

    status = mz_zip_writer_add_mem_ex(&zip_archive, data_name, (void*)data.data(), data.size(), "", 0, flags, 0, 0);
    if (!status) {
        cerr << "mz_zip_writer_add_mem_ex failed!";
        return -1;
    }

    std::size_t size;
    void *pBuf;
    status = mz_zip_writer_finalize_heap_archive(&zip_archive, &pBuf, &size);
    if (!status) {
        cerr << "mz_zip_writer_finalize_heap_archive failed!";
        return -1;
    }

    //copy compressed data to vector
    compressed_data.resize(size);
    copy((uint8_t*)pBuf, (uint8_t*)pBuf + size, compressed_data.begin());

    status = mz_zip_writer_end(&zip_archive);
    if (!status) {
        cerr << "mz_zip_writer_end failed!";
        return -1;
    }

    return 0;
}

void MainWindow::CompressAndReplaceEIF(ImageSection& img_sec, int idx, const vector<uint8_t>& res_bin, const std::string& res_name) {

    //get header data
    auto eif_header_p = reinterpret_cast<const EIF::EifBaseHeader*>(res_bin.data());

    //compress
    vector<uint8_t> zip_bin;
    if(compressVector(res_bin, res_name.c_str(), zip_bin)) {
        throw runtime_error("Can't compress resource " + res_name);
    }

    //replace
    img_sec.ReplaceItem(ImageSection::RT_ZIP, idx, zip_bin,
                        eif_header_p->width, eif_header_p->height, eif_header_p->type);
    cout << "Replace eif " << res_name << endl;
}

QString MainWindow::eitTypeToString(uint8_t eif_t)
{
    switch (eif_t) {
        case EIF_TYPE_MONOCHROME: return "(8bit)";
        case EIF_TYPE_MULTICOLOR: return "(16bit)";
        case EIF_TYPE_SUPERCOLOR: return "(32bit)";
        default: return "";
    }
}

void MainWindow::enableGui(bool doEnable) {

    ui->pushButton_exportAll->setEnabled(doEnable);
    ui->pushButton_exportImage->setEnabled(doEnable);
    ui->pushButton_replaceImage->setEnabled(doEnable);
    ui->actionSave->setEnabled(doEnable);
    ui->actionSave_As->setEnabled(doEnable);
    ui->actionClose->setEnabled(doEnable);
}

void MainWindow::unpackFinished() {

    if (!future.result().isEmpty()) {
        QMessageBox(QMessageBox::Warning,
                    "", future.result(), QMessageBox::Ok, this).exec();
        ui->label_Status->setText(QString("Error"));
        enableGui(true);
        return;
    }

    ui->lw->clear();
    ui->label_Status->setText(QString("Done"));

    for (const auto& picture : images) {
        auto newItem = new QListWidgetItem;
        newItem->setData(Qt::UserRole, picture.second.index);
        newItem->setText(picture.second.name.c_str());
        ui->lw->addItem(newItem);
    }

    enableGui(true);
}

void MainWindow::exportFinished() {

    enableGui(true);

    ui->label_Status->setText(QString("Done"));
}

void MainWindow::onProgressChanged(QPoint progress) {
    ui->label_Status->setText(QString("Processing %1 of %2").arg(progress.x() + 1).arg(progress.y()));
}

int MainWindow::exportAll(const QString &dest_dir) {

    int i = 1;
    const int pics_count = images.size();
    for(const auto& picture :images) {
        progressChanged({i++, pics_count});
        fs::path store_path(dest_dir.toStdString() / fs::path(picture.second.name).replace_extension(".bmp"));
        picture.second.eif->saveBmp(store_path);
    }

    return 0;
}

QString MainWindow::ReplacePicture(int picture_idx, const QString &new_picture_path) {

    try {
        auto& orig_picture = images[picture_idx];

        std::vector<uint8_t> img_sec_bin;
        if(vbf.GetSectionRaw(1, img_sec_bin)) {
            throw runtime_error("Can't get image section");
        }

        ImageSection section;
        section.Parse(img_sec_bin);

        // Pack new BMP image to EIF
        EIF::EifImageBase* new_eif;
        if (orig_picture.type == EIF_TYPE_MONOCHROME) {
            new_eif = new EIF::EifImage8bit();
        } else if (orig_picture.type == EIF_TYPE_SUPERCOLOR) {
            new_eif = new EIF::EifImage32bit();
        } else if (orig_picture.type == EIF_TYPE_MULTICOLOR) {
            new_eif = new EIF::EifImage16bit();
        } else {
            throw runtime_error("Unknown resource type");
        }
        new_eif->openBmp(new_picture_path.toStdString());

        if (new_eif->getWidth() != orig_picture.width || new_eif->getHeight() != orig_picture.height) {
            throw runtime_error("Replaced picture size mismatch");
        }

        vector<EIF::EifImage16bit> eifs_set; // used only for 16bit EIFs

        struct sMod_pic_desc {
            int index{};
            string name;
            const EIF::EifImageBase *eif{};
        };
        vector<sMod_pic_desc> modified_pictures;

        if (0 == orig_picture.palette_crc) {
            // replaced pic is 8 or 32 bit eif
            // no additional actions required
            // just make eif from bmp and replace the
            // original pic

            sMod_pic_desc new_pic_desc;
            new_pic_desc.index = orig_picture.index;
            new_pic_desc.name  = orig_picture.name;
            new_pic_desc.eif   = new_eif;

            modified_pictures.emplace_back(std::move(new_pic_desc));

        } else {
            // replaced pic is definitely 16 bit eif.
            // 16bits eif may be bonded into a set and share one palette
            // so we need to recalculate a new palette for each eif included in that set


            // find all pictures with the same palette
            for (const auto &it :images) {
                const auto & picture = it.second;
                if(picture.palette_crc == orig_picture.palette_crc) {

                    sMod_pic_desc new_pic_desc;
                    new_pic_desc.index = picture.index;
                    new_pic_desc.name  = picture.name;
                    if (picture.index != orig_picture.index) {
                        // leave original eif data
                        eifs_set.push_back(*reinterpret_cast<const EIF::EifImage16bit *>(picture.eif.get()));
                    } else {
                        // use new eif data
                        eifs_set.push_back(*reinterpret_cast<const EIF::EifImage16bit *>(new_eif));  // 🤢
                    }
                    new_pic_desc.eif = &eifs_set.back(); //fixup pointer

                    modified_pictures.emplace_back(std::move(new_pic_desc));
                }
            }

            // calc new 'multipalette'
            EIF::EifConverter::mapMultiPalette(eifs_set);

            // fixup pointers
            for (int i=0; i < modified_pictures.size(); ++i) {
                modified_pictures[i].eif = &eifs_set[i]; // 🤢
            }
        }

        int i = 0, modded_count = modified_pictures.size();
        for (const auto &mod_pic : modified_pictures) {
            progressChanged({++i, modded_count});
            CompressAndReplaceEIF(section, mod_pic.index, mod_pic.eif->saveEifToVector(), mod_pic.name);
        }

        delete new_eif; //TODO:

        section.SaveToVector(img_sec_bin);

        //replace vbf image content
        vbf.ReplaceSectionRaw(1, img_sec_bin);

        unpackVBF();

    } catch (const runtime_error& e) {
        return e.what();
    }

    return "";
}