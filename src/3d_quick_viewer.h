// Mechanicoder
// 2022/06/04

#pragma once

#include <QMainWindow>
#include <QTreeView>

class QProgressBar;
class QMenu;

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
    
    void UpdateProgressBar();

    // 初始化进度条
    void InitProgressBar();

    // 初始化默认菜单<Update>
    void InitDefaultMenu();

private slots:
    // 初始化或更新菜单
    void OnInitMenu();

    // 执行命令
    void OnContextCmd(const QAction* action, const QWidget* by_who);

    // 重设默认根目录
    void OnApplyRootDir();

signals:
    void EvalTimeFinished();

private:
    Ui::TdQuickViewerUi* _ui;
    QProgressBar* _progressBar;
    QMenu* _menu;

    int _colCnt; // 列数量
    QGridLayout* _layout; // 所有3D显示控件的容器

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