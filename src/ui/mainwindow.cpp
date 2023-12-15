#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include "./aboutdlg.h"

#include "../sh2texture.h"
#include "../sh2map.h"
#include "../sh2model.h"
#include "../ddstexture.h"
#define BCDEC_IMPLEMENTATION
#include "../libs/bcdec/bcdec.h"

#include <QSettings>
#include <QListWidgetItem>
#include <QDragEnterEvent>
#include <QMimeData>
#include <QFileDialog>
#include <QMessageBox>

// makes path to have consistent separators '/'
static fs::path FixPath(const fs::path& pathToFix) {
    std::string str = pathToFix.u8string();
    std::string::size_type pos = 0;
    while (std::string::npos != (pos = str.find_first_of('\\'))) {
        str[pos] = '/';
    }
    return fs::path{ str };
}


// settings
static const QString kLastOpenPath("LastOpenPath");
static const QString kLastSavePath("LastSavePath");
static const QString kRecentTextureTemplate("RecentTexture_");

constexpr size_t kMaxRecentTextures = 10;

static QString SH2FormatToString(const SH2Texture::Format format) {
    switch (format) {
        case SH2Texture::Format::DXT1: return QString("DXT1");
        case SH2Texture::Format::DXT2: return QString("DXT2");
        case SH2Texture::Format::DXT3: return QString("DXT3");
        case SH2Texture::Format::DXT4: return QString("DXT4");
        case SH2Texture::Format::DXT5: return QString("DXT5");
        case SH2Texture::Format::Paletted: return QString("Paletted");
        case SH2Texture::Format::RGBX8: return QString("RGBX8");
        case SH2Texture::Format::RGBA8: return QString("RGBA8");
        default: return QString();
    }
}

static bool IsAcceptedExtension(const QString& path) {
    const QString& pathLower = path.toLower();
    return pathLower.endsWith(".tex")  ||
           pathLower.endsWith(".tbn2") ||
           pathLower.endsWith(".map")  ||
           pathLower.endsWith(".mdl");
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , mStatusLabel(new QLabel)
    , mTexturesContainer(nullptr)
{
    ui->setupUi(this);
    setAcceptDrops(true);

    mOriginalTitle = this->windowTitle();

    mStatusLabel->setText(QString());
    this->statusBar()->addWidget(mStatusLabel);

    ui->listProperties->headerItem()->setText(0, tr("Property"));
    ui->listProperties->headerItem()->setText(1, tr("Value"));

    this->UpdateRecentTexturesList();
}

MainWindow::~MainWindow() {
    delete ui;
}

void MainWindow::dragEnterEvent(QDragEnterEvent* event) {
    const QMimeData* md = event->mimeData();
    if (md->hasUrls()) {
        const QList<QUrl>& urls = md->urls();
        if (urls.size() > 0) {
            const QUrl& url = urls[0];
            if (url.isLocalFile()) {
                QString filePath = url.toLocalFile();
                if (IsAcceptedExtension(filePath)) {
                    event->acceptProposedAction();
                }
            }
        }
    }
}

void MainWindow::dropEvent(QDropEvent* event) {
    const QList<QUrl>& urls = event->mimeData()->urls();
    const QUrl& url = urls[0];
    if (url.isLocalFile()) {
        QString filePath = url.toLocalFile();
        if (IsAcceptedExtension(filePath)) {
            this->LoadTextureFromFile(filePath.toStdWString(), true);

            event->acceptProposedAction();
        }
    }
}

int MainWindow::GetSelectedTextureIdx() const {
    auto l = ui->listTextures->selectedItems();
    if (l.isEmpty()) {
        return -1;
    }

    return l[0]->data(Qt::UserRole).toInt();
}

void MainWindow::DecompressTexture(const SH2Texture* texture, BytesArray& output, const bool doNotSwizzle) {
    const SH2Texture::Format format = texture->GetFormat();
    const uint32_t width = texture->GetWidth();
    const uint32_t height = texture->GetHeight();
    const uint8_t* compressed = texture->GetData();

    if (texture->IsCompressed()) {
        const uint8_t* src = compressed;

        for (size_t i = 0; i < height; i += 4) {
            for (size_t j = 0; j < width; j += 4) {
                uint8_t* dst = output.data() + (i * width + j) * 4;

                if (format == SH2Texture::Format::DXT1) {
                    bcdec_bc1(src, dst, width * 4);
                    src += BCDEC_BC1_BLOCK_SIZE;
                } else if (format == SH2Texture::Format::DXT2 || format == SH2Texture::Format::DXT3) {
                    bcdec_bc2(src, dst, width * 4);
                    src += BCDEC_BC2_BLOCK_SIZE;
                } else if (format == SH2Texture::Format::DXT4 || format == SH2Texture::Format::DXT5) {
                    bcdec_bc3(src, dst, width * 4);
                    src += BCDEC_BC3_BLOCK_SIZE;
                }
            }
        }
    } else if (format == SH2Texture::Format::Paletted) {
        const uint8_t* indices = texture->GetData();
        const uint32_t* palette = rcast<const uint32_t*>(texture->GetPalette());
        uint32_t* dst = rcast<uint32_t*>(output.data());
        for (size_t i = 0, end = (width * height); i < end; ++i, ++indices, ++dst) {
            *dst = palette[*indices];
        }
    } else if (format == SH2Texture::Format::RGBX8) {
        std::memcpy(output.data(), compressed, output.size());
    } else /* RGBA8 */ {
        std::memcpy(output.data(), compressed, output.size());
    }

    if (!doNotSwizzle && !texture->IsCompressed()) {
        for (size_t i = 0, end = output.size(); i < end; i += 4) {
            std::swap(output[i + 0], output[i + 2]);
        }
    }
}

void MainWindow::LoadTextureFromFile(const fs::path& path, const bool addToRecent) {
    fs::path fixedPath = FixPath(path);

    bool loadSucceeded = false;

    if (fixedPath.extension() == ".map") {
        RefPtr<SH2Map> map = MakeRefPtr<SH2Map>();
        if (map->LoadFromFile(fixedPath)) {
            mMap = map;
            mTexturesContainer = mMap->GetTexturesContainer();
            mModel = nullptr;
            loadSucceeded = true;
        }
    } else if (fixedPath.extension() == ".mdl") {
        RefPtr<SH2Model> model = MakeRefPtr<SH2Model>();
        if (model->LoadFromFile(fixedPath)) {
            mModel = model;
            mTexturesContainer = mModel->GetTexturesContainer();
            mMap = nullptr;
            loadSucceeded = true;
        }
    } else {
        RefPtr<SH2TextureContainer> container = MakeRefPtr<SH2TextureContainer>();
        if (container->LoadFromFile(fixedPath)) {
#if 0
            auto& warnings = container->GetWarnings();
            if (!warnings.empty()) {
                QString warningsMesage;
                for (auto& e : warnings) {
                    warningsMesage += QString::fromStdString(e);
                    warningsMesage.push_back('\n');
                }
                QMessageBox::warning(this, tr("Texture loading warnings"), warningsMesage);
            }
#endif

            mTexturesContainer = container;
            mMap = nullptr;
            mModel = nullptr;
            loadSucceeded = true;
        } else if (!container->GetErrors().empty()) {
            auto& errors = container->GetErrors();
            QString errorsMesage;
            for (auto& e : errors) {
                errorsMesage += QString::fromStdString(e);
                errorsMesage.push_back('\n');
            }
            QMessageBox::critical(this, tr("Failed to load texture!"), errorsMesage);
        }
    }

    if (loadSucceeded) {
        this->OnTextureLoaded();

        mLastPath = fixedPath;
        this->setWindowTitle(mOriginalTitle + QString("   - ") + QString::fromStdWString(mLastPath.filename().wstring()));

        if (addToRecent) {
            this->AddToRecentTexturesList(QString::fromStdWString(fixedPath.wstring()));
        }
    }
}

void MainWindow::OnTextureLoaded(const int idx) {
    if (!mTexturesContainer) {
        return;
    }

    ui->listTextures->clear();

    QListWidgetItem* selected = nullptr;

    const size_t numTextures = mTexturesContainer->GetNumTextures();
    for (size_t i = 0; i < numTextures; ++i) {
        const SH2Texture* texture = mTexturesContainer->GetTexture(i);

        const uint32_t width = texture->GetWidth();
        const uint32_t height = texture->GetHeight();
        BytesArray decompressed(width * height * 4);
        this->DecompressTexture(texture, decompressed, false);

        const QImage::Format qfmt = texture->IsPremultiplied() ? QImage::Format_RGBA8888_Premultiplied : QImage::Format_RGBA8888;

        QImage icon = QImage(decompressed.data(), width, height, width * 4, qfmt).scaled(QSize(128, 128), Qt::IgnoreAspectRatio, Qt::SmoothTransformation);

        QString tooltip = QString("id: %1\n%2x%3\n%4").arg(texture->GetID()).arg(width).arg(height).arg(SH2FormatToString(texture->GetFormat()));

        QListWidgetItem* item = new QListWidgetItem();
        item->setText(QString("texture %1").arg(i));
        item->setToolTip(tooltip);
        item->setIcon(QIcon(QPixmap::fromImage(icon)));
        item->setData(Qt::UserRole, scast<int>(i));
        ui->listTextures->addItem(item);

        if (idx < 0 && !selected) {
            selected = item;
        } else if (idx == scast<int>(i)) {
            selected = item;
        }
    }

    ui->listTextures->setCurrentItem(selected);
}

QString MainWindow::GetLastPathFolder() const {
    if (mLastPath.empty()) {
        return QString();
    }

    return QString::fromStdWString(mLastPath.parent_path().wstring());
}

QString MainWindow::GetLastPathFileName() const {
    if (mLastPath.empty()) {
        return QString();
    }

    return QString::fromStdWString(mLastPath.filename().wstring());
}

QStringList MainWindow::GetRecentTexturesList() const {
    QList<QString> result;

    QSettings registry;
    for (size_t i = 0; i < kMaxRecentTextures; ++i) {
        QString keyName = QString("%1%2").arg(kRecentTextureTemplate).arg(i);
        QString keyValue = registry.value(keyName).toString();
        if (keyValue.isEmpty()) {
            break;
        }

        result.push_back(keyValue);
    }

    return result;
}

void MainWindow::AddToRecentTexturesList(const QString& entry) {
    QList<QString> currentList = this->GetRecentTexturesList();
    const qsizetype idx = currentList.indexOf(entry);
    if (idx != -1) {
        currentList.removeAt(idx);
    }

    QList<QString> newList;
    newList.push_back(entry);
    for (size_t i = 0; i < kMaxRecentTextures - 1; ++i) {
        if (i < scast<size_t>(currentList.size())) {
            newList.push_back(currentList[i]);
        } else {
            break;
        }
    }

    QSettings registry;
    for (size_t i = 0; i < scast<size_t>(newList.size()); ++i) {
        QString keyName = QString("%1%2").arg(kRecentTextureTemplate).arg(i);
        registry.setValue(keyName, newList[i]);
    }

    this->UpdateRecentTexturesList();
}

void MainWindow::UpdateRecentTexturesList() {
    ui->menuRecent_textures->clear();

    const QList<QString>& list = this->GetRecentTexturesList();

    if (list.isEmpty()) {
        ui->menuRecent_textures->setDisabled(true);
    } else {
        ui->menuRecent_textures->setDisabled(false);

        for (size_t i = 0; i < scast<size_t>(list.size()); ++i) {
            QAction* action = ui->menuRecent_textures->addAction(list[i]);
            connect(action, &QAction::triggered, this, [this, i]() { this->on_actionRecentTexture_triggered(i); });
        }
    }
}

void MainWindow::ExportTexture(const SH2Texture* texture, const fs::path& path) {
    DDSTexture dds;

    const uint32_t width = texture->GetWidth();
    const uint32_t height = texture->GetHeight();

    dds.SetWidth(width);
    dds.SetHeight(height);

    const SH2Texture::Format texFormat = texture->GetFormat();
    if (texFormat == SH2Texture::Format::RGBA8) {
        dds.SetFormat(32);
        const size_t dataSize = width * height * 4;
        dds.SetData(texture->GetData(), dataSize);
    } else if (texFormat == SH2Texture::Format::Paletted) {
        MyArray<uint8_t> unpaletted(width * height * 4);
        this->DecompressTexture(texture, unpaletted, true);
        dds.SetData(unpaletted.data(), unpaletted.size());
        dds.SetFormat(32);
    } else {
        switch (texFormat) {
            case SH2Texture::Format::DXT1:
                dds.SetFormat(DDS_FOURCC_DXT1);
            break;
            case SH2Texture::Format::DXT2:
                dds.SetFormat(DDS_FOURCC_DXT2);
            break;
            case SH2Texture::Format::DXT3:
                dds.SetFormat(DDS_FOURCC_DXT3);
            break;
            case SH2Texture::Format::DXT4:
                dds.SetFormat(DDS_FOURCC_DXT4);
            break;
            case SH2Texture::Format::DXT5:
                dds.SetFormat(DDS_FOURCC_DXT5);
            break;
        }

        const size_t dataSize = (texFormat == SH2Texture::Format::DXT1) ? BCDEC_BC1_COMPRESSED_SIZE(width, height) : BCDEC_BC3_COMPRESSED_SIZE(width, height);
        dds.SetData(texture->GetData(), dataSize);
    }
    dds.SaveToFile(path);
}

void MainWindow::ImportTexture(const fs::path& path, const int idx) {
    if (!mTexturesContainer || idx < 0 || idx >= mTexturesContainer->GetNumTextures()) {
        return;
    }

    DDSTexture dds;
    if (!dds.LoadFromFile(path)) {
        QMessageBox::critical(this, this->windowTitle(), tr("Failed to load DDS texture!"));
        return;
    }

    const uint32_t ddsFormat = dds.GetFormat();
    SH2Texture::Format sh2Format;
    if (ddsFormat == DDS_FOURCC_DXT1) {
        sh2Format = SH2Texture::Format::DXT1;
    } else if (ddsFormat == DDS_FOURCC_DXT2) {
        sh2Format = SH2Texture::Format::DXT2;
    } else if (ddsFormat == DDS_FOURCC_DXT3) {
        sh2Format = SH2Texture::Format::DXT3;
    } else if (ddsFormat == DDS_FOURCC_DXT4) {
        sh2Format = SH2Texture::Format::DXT4;
    } else if (ddsFormat == DDS_FOURCC_DXT5) {
        sh2Format = SH2Texture::Format::DXT5;
    } else {
        sh2Format = SH2Texture::Format::RGBA8;
    }

    SH2Texture* texture = mTexturesContainer->GetTexture(idx);
    texture->Replace(sh2Format, dds.GetWidth(), dds.GetHeight(), dds.GetData());

    this->OnTextureLoaded(idx);
}


void MainWindow::on_action_Open_triggered() {
    QString folder = this->GetLastPathFolder();
    QString fileName = QFileDialog::getOpenFileName(this,
                                                    tr("Select SH2 texture/map file"),
                                                    folder,
                                                    tr("SH2 known formats (*.tex *.tbn2 *.map *.mdl);;SH2 textures (*.tex *.tbn2);;SH2 map files (*.map);;SH2 model files (*.model);;All files (*.*)"));
    if (!fileName.isEmpty()) {
        this->LoadTextureFromFile(fileName.toStdWString(), true);
    }
}

void MainWindow::on_action_Save_triggered() {
    if (mTexturesContainer) {
        QString startPath = QString::fromStdWString(mLastPath.wstring());

        if (mMap) {
            QString fileName = QFileDialog::getSaveFileName(this, tr("Save SH2 map file"), startPath, tr("SH2 map (*.map);;All files (*.*)"));
            if (!fileName.isEmpty()) {
                mMap->SaveToFile(fileName.toStdWString());
            }
        } else if (mModel) {
            QString fileName = QFileDialog::getSaveFileName(this, tr("Save SH2 model file"), startPath, tr("SH2 model (*.mdl);;All files (*.*)"));
            if (!fileName.isEmpty()) {
                mModel->SaveToFile(fileName.toStdWString());
            }
        } else {
            QString fileName = QFileDialog::getSaveFileName(this, tr("Save SH2 texture file"), startPath, tr("SH2 textures (*.tex *.tbn2);;All files (*.*)"));
            if (!fileName.isEmpty()) {
                mTexturesContainer->SaveToFile(fileName.toStdWString());
            }
        }
    }
}

void MainWindow::on_actionRecentTexture_triggered(const size_t recentModelIdx) {
    const QList<QAction*>& actions = ui->menuRecent_textures->actions();
    if (recentModelIdx < scast<size_t>(actions.size())) {
        fs::path modelPath = actions[recentModelIdx]->text().toStdWString();
        this->LoadTextureFromFile(modelPath, false);
    }
}

void MainWindow::on_actionE_xit_triggered() {
    this->close();
}

void MainWindow::on_listTextures_itemSelectionChanged() {
    const int idx = this->GetSelectedTextureIdx();
    if (idx >= 0 && idx < mTexturesContainer->GetNumTextures()) {
        const SH2Texture* texture = mTexturesContainer->GetTexture(idx);

        const uint32_t width = texture->GetWidth();
        const uint32_t height = texture->GetHeight();
        BytesArray decompressed(width * height * 4);
        this->DecompressTexture(texture, decompressed, false);

        ui->imagePanel->SetImage(decompressed.data(), width, height);

        QString formatName = SH2FormatToString(texture->GetFormat());
        const uint32_t orgDataSize = texture->GetOriginalDataSize();
        const uint32_t dataSize = texture->CalculateDataSize();

        ui->listProperties->clear();
        ui->listProperties->addTopLevelItem(new QTreeWidgetItem({ tr("ID"), QString::number(texture->GetID()) }));
        ui->listProperties->addTopLevelItem(new QTreeWidgetItem({ tr("Width"), QString::number(width) }));
        ui->listProperties->addTopLevelItem(new QTreeWidgetItem({ tr("Height"), QString::number(height) }));
        ui->listProperties->addTopLevelItem(new QTreeWidgetItem({ tr("Format"), formatName }));

        QTreeWidgetItem* dataSizeItem = new QTreeWidgetItem({ tr("Data size"), QString::number(orgDataSize)});
        if (orgDataSize != dataSize) {
            dataSizeItem->setForeground(1, QBrush(Qt::red));
            dataSizeItem->setToolTip(0, QString("wrong size, should be %1").arg(dataSize));
            dataSizeItem->setToolTip(1, dataSizeItem->toolTip(0));
        }
        ui->listProperties->addTopLevelItem(dataSizeItem);

        if (texture->IsPS2File()) {
            QTreeWidgetItem* ps2FileItem = new QTreeWidgetItem({ tr("Is PS2 file"), tr("YES") });
            ps2FileItem->setForeground(0, QBrush(Qt::blue));
            ps2FileItem->setForeground(1, QBrush(Qt::blue));
            ui->listProperties->addTopLevelItem(ps2FileItem);
        }

        mStatusLabel->setText(QString("Selected texture - id: %1, size: %2x%3, format: %4").arg(texture->GetID()).arg(width).arg(height).arg(formatName));
    }
}

void MainWindow::on_listTextures_customContextMenuRequested(const QPoint &pos) {
    QMenu contextMenu(this);

    const int idx = this->GetSelectedTextureIdx();
    if (idx >= 0 && idx < mTexturesContainer->GetNumTextures()) {
        const SH2Texture* texture = mTexturesContainer->GetTexture(idx);

        QAction exportTexture(tr("Export texture..."));
        QAction replaceTexture(tr("Replace texture..."));

        if (texture->IsPS2File()) {
            replaceTexture.setEnabled(false);
        }

        contextMenu.addAction(&exportTexture);
        contextMenu.addSeparator();
        contextMenu.addAction(&replaceTexture);

        QAction* selectedAction = contextMenu.exec(ui->listTextures->mapToGlobal(pos));
        if (selectedAction == &exportTexture) {
            QString nameAppend;
            if (mTexturesContainer->GetNumTextures() > 1) {
                nameAppend = QString("_%1.dds").arg(idx);
            } else {
                nameAppend = ".dds";
            }

            QString folder = this->GetLastPathFolder();
            QString name = this->GetLastPathFileName();
            if (!name.isEmpty()) {
                name.replace(QString(".tex"), nameAppend);
                name.replace(QString(".tbn2"), nameAppend);
                name.replace(QString(".map"), nameAppend);
                name.replace(QString(".mdl"), nameAppend);
            }
            QString startPath = folder + '/' + name;

            QString fileName = QFileDialog::getSaveFileName(this, tr("Where to save DDS file"), startPath, tr("DDS texture (*.dds)"));
            if (!fileName.isEmpty()) {
                this->ExportTexture(texture, fileName.toStdWString());
            }
        } else if (selectedAction == &replaceTexture) {
            QString folder = this->GetLastPathFolder();
            QString fileName = QFileDialog::getOpenFileName(this, tr("Select DDS texture"), folder, tr("DDS texture (*.dds)"));
            if (!fileName.isEmpty()) {
                this->ImportTexture(fileName.toStdWString(), idx);
            }
        }
    } else {
        QAction addTexture(tr("Add texture..."));

#ifndef _DEBUG
        addTexture.setEnabled(false);
#endif

        contextMenu.addAction(&addTexture);

        QAction* selectedAction = contextMenu.exec(ui->listTextures->mapToGlobal(pos));
        if (selectedAction == &addTexture) {
            QString folder = this->GetLastPathFolder();
            QString fileName = QFileDialog::getOpenFileName(this, tr("Select DDS texture"), folder, tr("DDS texture (*.dds)"));
            if (!fileName.isEmpty()) {
                this->ImportTexture(fileName.toStdWString(), -1);
            }
        }
    }
}


void MainWindow::on_actionShow_transparency_triggered() {
    ui->imagePanel->ShowTransparency(ui->actionShow_transparency->isChecked());
}

void MainWindow::on_actionAbout_triggered() {
    AboutDlg dlg(this);
    dlg.exec();
}
