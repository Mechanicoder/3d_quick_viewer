#pragma once

#include <QWidget>

class QLabel;

// ÏÔÊ¾Ô¤ÀÀ¿Ø¼þ
class TdPreviewWidget : public QWidget
{
    Q_OBJECT
public:
    TdPreviewWidget(const QString& filename, QWidget* parent = nullptr);
    ~TdPreviewWidget();

    void UpdateFilename(const QString& filename);

private:
    QLabel* _label;
};
