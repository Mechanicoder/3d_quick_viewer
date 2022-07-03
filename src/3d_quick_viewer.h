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

    // �ж��ļ��Ƿ�֧��Ԥ��
    bool IsSupportedFile(const QFileInfo& info) const;
    
    void UpdateProgressBar();

    // ��ʼ��������
    void InitProgressBar();

    // ��ʼ��Ĭ�ϲ˵�<Update>
    void InitDefaultMenu();

private slots:
    // ��ʼ������²˵�
    void OnInitMenu();

    // ִ������
    void OnContextCmd(const QAction* action, const QWidget* by_who);

    // ����Ĭ�ϸ�Ŀ¼
    void OnApplyRootDir();

signals:
    void EvalTimeFinished();

private:
    Ui::TdQuickViewerUi* _ui;
    QProgressBar* _progressBar;
    QMenu* _menu;

    int _colCnt; // ������
    QGridLayout* _layout; // ����3D��ʾ�ؼ�������

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