#include "ui/mainwindow.h"

#include <QApplication>

int main(int argc, char *argv[]) {
    QApplication a(argc, argv);

    // this is for QSettings
    a.setOrganizationName("iOrange");
    a.setApplicationName("sh2tex");

    MainWindow w;
    w.show();

    if (argc > 1) {
        fs::path filePath = argv[1];
        if (filePath.extension() == ".tex" || filePath.extension() == ".tbn2" || filePath.extension() == "*.map" || filePath.extension() == "*.mdl") {
            w.LoadTextureFromFile(filePath, true);
        }
    }

    return a.exec();
}
