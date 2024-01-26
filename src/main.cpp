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
        if (WStrEqualsCaseInsensitive(filePath.extension(), L".tex")  ||
            WStrEqualsCaseInsensitive(filePath.extension(), L".tbn2") ||
            WStrEqualsCaseInsensitive(filePath.extension(), L".map")  ||
            WStrEqualsCaseInsensitive(filePath.extension(), L".mdl")) {
            w.LoadTextureFromFile(filePath, true, false);
        }
    }

    return a.exec();
}
