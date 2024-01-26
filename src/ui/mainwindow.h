#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "../mycommon.h"

class SH2TextureContainer;
class SH2Texture;
class SH2Map;
class SH2Model;

class QLabel;

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    void        dragEnterEvent(QDragEnterEvent* event) override;
    void        dropEvent(QDropEvent* event) override;
    bool        eventFilter(QObject* obj, QEvent* event) override;

public:
    int         GetSelectedTextureIdx() const;
    void        DecompressTexture(const SH2Texture* texture, BytesArray& output, const bool doNotSwizzle);
    void        LoadTextureFromFile(const fs::path& path, const bool addToRecent, const bool fromIterator);
    void        OnTextureLoaded(const int idx = -1);
    QString     GetLastPathFolder() const;
    QString     GetLastPathFileName() const;
    QStringList GetRecentTexturesList() const;
    void        AddToRecentTexturesList(const QString& entry);
    void        UpdateRecentTexturesList();
    void        UpdateStatusBar();

    void        ExportTexture(const SH2Texture* texture, const fs::path& path);
    void        ExportAllTextures(const fs::path& dstFolder);
    void        ImportTexture(const fs::path& path, const int idx);

    void        SetDarkTheme(const bool isDark);

private slots:
    void        on_action_Open_triggered();
    void        on_action_Save_triggered();
    void        on_actionRecentTexture_triggered(const size_t recentTextureIdx);
    void        on_actionE_xit_triggered();
    void        on_listTextures_itemSelectionChanged();
    void        on_listTextures_customContextMenuRequested(const QPoint &pos);
    void        on_actionShow_transparency_triggered();
    void        on_actionDark_theme_triggered();
    void        on_actionAbout_triggered();

private:
    Ui::MainWindow*             ui;
    QString                     mOriginalTitle;
    QLabel*                     mStatusLabel;
    RefPtr<SH2TextureContainer> mTexturesContainer;
    RefPtr<SH2Map>              mMap;
    RefPtr<SH2Model>            mModel;
    fs::path                    mLastPath;

    // 
    QPalette                    mOriginalPalette;
    QString                     mOriginalStyleSheet;
    QString                     mOriginalStyleName;
};
#endif // MAINWINDOW_H
