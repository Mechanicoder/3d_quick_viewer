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
class QFileInfo;

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

    // 判断文件是否支持预览
    bool IsSupportedFile(const QFileInfo& info) const;

signals:
    void EvalTimeFinished();

private:
    Ui::TdQuickViewerUi* _ui;

    QGridLayout* _layout;

    int _colCnt; // 列数量
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

#ifdef EVAL_PERFORMANCE
    QElapsedTimer* _evalTimer;
    int _evalCnt;
    QString _evalPath;
#endif
};