#pragma once
#include <QScrollArea>
#include <QLabel>

#include "../mycommon.h"

class MyLabel : public QLabel {
    Q_OBJECT

public:
    explicit MyLabel(QWidget* parent = nullptr, Qt::WindowFlags f = Qt::WindowFlags());

protected:
    void mouseMoveEvent(QMouseEvent* ev) override;
    void mousePressEvent(QMouseEvent* ev) override;
    void mouseReleaseEvent(QMouseEvent* ev) override;
};

class ImagePanel : public QScrollArea {
    Q_OBJECT

public:
    ImagePanel(QWidget* parent = nullptr);

    void        SetImage(const void* pixelsRGBA, const size_t width, const size_t height, const bool premultiplied = false);
    const void* GetImageData() const;
    size_t      GetImageWidth() const;
    size_t      GetImageHeight() const;
    void        ShowTransparency(const bool toShow);

    void        ZoomDown(int step = 0);
    void        ZoomUp(int step = 0);
    int         GetZoom() const;
    void        ResetZoom();

protected:
    void        mouseMoveEvent(QMouseEvent* ev) override;
    void        mousePressEvent(QMouseEvent* ev) override;
    void        mouseReleaseEvent(QMouseEvent* ev) override;

private:
    void        ResizeWithZoom();
    void        ResetScroll();

private:
    QLabel*     mImageLabel;
    bool        mTransparency;
    BytesArray  mImageData;
    int         mImageWidth;
    int         mImageHeight;
    bool        mImagePremultiplied;
    bool        mLMBDown;
    QPoint      mLastMPos;
    QSize       mOriginalSizeDPICorrected;
    int         mZoom;  // in %
};
