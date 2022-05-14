#pragma once

#include <QMainWindow>
#include <QTreeView>

namespace Ui
{
    class TdQuickViewerUi;
}

class QGridLayout;

class TdQuickViewer : public QMainWindow
{
    Q_OBJECT
public:
    TdQuickViewer(QWidget* parent = nullptr);
    ~TdQuickViewer();

private slots:
    void OnFolderPressed(const QString& filepath);

private:
    Ui::TdQuickViewerUi* _ui;

    QGridLayout* _layout;
};