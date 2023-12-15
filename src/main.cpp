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
        fs::path mdlPath = argv[1];
        if (mdlPath.extension() == ".tex" || mdlPath.extension() == ".tbn2") {
            w.LoadTextureFromFile(mdlPath, true);
        }
    }

    return a.exec();
}
