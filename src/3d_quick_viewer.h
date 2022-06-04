#pragma once

#include <QMainWindow>
#include <QTreeView>

#define EVAL_PERFORMANCE

namespace Ui
{
    class TdQuickViewerUi;
}

class QGridLayout;
class QElapsedTimer;
class QTimer;

class TdQuickViewer : public QMainWindow
{
    Q_OBJECT
public:
    TdQuickViewer(QWidget* parent = nullptr);
    ~TdQuickViewer();

private slots:
    void OnFolderPressed(const QString& filepath);

    void PreviewFinished();

    void ProcessTask();

private:
    void IncreasePreviewWidget(int total_cnt);

    void EvalPerformanceStart();
    void EvalPerformanceEnd();

    void UpdateTasks(const QString& folder_path);

signals:
    void EvalTimeFinished();

private:
    Ui::TdQuickViewerUi* _ui;

    QGridLayout* _layout;

    int _colCnt; // ÁÐÊýÁ¿
    enum TaskStage
    {
        TS_Ready = 0,
        TS_LoadFileDone,
        TS_TessellateShapeDone,
    };
    struct Task
    {
        int id = 0;

        QString filename;
        TaskStage stage = TS_Ready; // 1-load file done, 2-tessellate done
    };
    std::list<Task> _tasks;
    QTimer* _timer;
    //QHash<QString, std::pair<int, QWidget*>> _filenames;

#ifdef EVAL_PERFORMANCE
    QElapsedTimer* _evalTimer;
    int _evalCnt;
    QString _evalPath;
#endif
};