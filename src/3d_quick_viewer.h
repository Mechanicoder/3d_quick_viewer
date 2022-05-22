#pragma once

#include <QMainWindow>
#include <QTreeView>

#define EVAL_PERFORMANCEx

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

    void PreviewFinished();

private:
    void IncreasePreviewWidget(int total_cnt);

    void EvalPerformanceStart();
    void EvalPerformanceEnd();

signals:
    void EvalTimeFinished();

private:
    Ui::TdQuickViewerUi* _ui;

    QGridLayout* _layout;

    int _colCnt; // ÁÐÊýÁ¿

#ifdef EVAL_PERFORMANCE
    QElapsedTimer* _timer;
    int _evalCnt;
    QString _evalPath;
#endif
};