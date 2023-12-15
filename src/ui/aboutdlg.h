#ifndef ABOUTDLG_H
#define ABOUTDLG_H

#include <QDialog>

namespace Ui {
class AboutDlg;
}

class AboutDlg : public QDialog {
    Q_OBJECT

public:
    explicit AboutDlg(QWidget *parent = nullptr);
    ~AboutDlg();

private slots:
    void on_buttonBox_accepted();

private:
    Ui::AboutDlg *ui;
};

#endif // ABOUTDLG_H
