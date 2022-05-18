#pragma once

#include <QMainWindow>
#include <QTreeView>

namespace Ui
{
    class TdQuickViewerUi;
}

class QGridLayout;
class QElapsedTimer;

class TdQuickViewer : public QMainWindow
{
    Q_OBJECT
public:
    TdQuickViewer(QWidget* parent = nullptr);
    ~TdQuickViewer();


private slots:
    void OnFolderPressed(const QString& filepath);

private:
    void IncreasePreviewWidget(int total_cnt);

    void EvalPerformance();

signals:
    void EvalTimeFinished();

private:
    Ui::TdQuickViewerUi* _ui;

    QGridLayout* _layout;

    int _colCnt; // ������

    QElapsedTimer* _timer;
};