#pragma once

#include <QWidget>

class QLabel;

// ��ʾԤ���ؼ�
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
