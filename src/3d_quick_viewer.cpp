#include "3d_quick_viewer.h"
#include "file_system_viewer.h"
#include "3d_preview_widget.h"
#include "shape_tessellater.h"
#include "ui_3d_quick_viewer.h"

#include "reader/step_reader.h"

#include <QFileSystemModel>
#include <QFileInfo>
#include <QVector>
#include <QDebug>
#include <QScrollBar>
#include <QTime>
#include <QDateTime>
#include <QElapsedTimer>
#include <QTimer>
#include <QThread>

TdQuickViewer::TdQuickViewer(QWidget* parent) : QMainWindow(parent), _colCnt(3)
{
    _ui = new Ui::TdQuickViewerUi;
    _ui->setupUi(this);

    _layout = new QGridLayout(_ui->scrollArea_preview->widget());
    _layout->setSizeConstraint(QLayout::SetMinimumSize);
    _layout->setSpacing(6);

    connect(_ui->treeView_path, &FileSystemViewer::FolderPressed, this, &TdQuickViewer::OnFolderPressed);

    _timer = new QTimer(this);
    _timer->setInterval(200); // ��ѯƵ�ʣ�200ms ��ѯһ��ģ���Ƿ���ɼ���
    connect(_timer, &QTimer::timeout, this, &TdQuickViewer::ProcessTask);

#ifdef EVAL_PERFORMANCE
    _evalTimer = nullptr;
    _evalPath = QCoreApplication::applicationDirPath() + "/../0000_STEP_TEST/EVAL_TIME";
    EvalPerformanceStart();
#endif
}

TdQuickViewer::~TdQuickViewer()
{
}

void TdQuickViewer::OnFolderPressed(const QString& filepath)
{
    UpdateTasks(filepath);

    if (!_tasks.empty())
    {
        // ����3DԤ����ͼ
        IncreasePreviewWidget(int(_tasks.size()));

        std::vector<QString> load_files;
        for (auto it = _tasks.begin(); it != _tasks.end(); ++it)
        {
            load_files.emplace_back(it->filename);
        }
        StepReader::Instance().Reset(load_files);
        ShapeTessellater::Instance().Reset();

        _timer->start();

        //QApplication::processEvents();

        // ���̼߳����ļ�

        // ����ģ��

        //const int exist_count = _layout->count();
        //const int col_cnt = _colCnt; // ������
        //
        //for (int i = 0; i < filenames.size(); i++)
        //{
        //    const int row = i / 3;
        //    const int col = (i - row * col_cnt) % 3;
        //
        //    QLayoutItem* item = _layout->itemAtPosition(row, col);
        //    if (item && item->widget())
        //    {
        //        TdPreviewWidget* widget = static_cast<TdPreviewWidget*>(item->widget());
        //        widget->UpdateFilename(filenames[i], int(i + 1), (int)filenames.size());
        //    }
        //    qDebug() << "Updating: " << filenames[i];
        //}
        //
        //qDebug() << "File path update finished! Viewport size " 
        //    << _ui->scrollArea_preview->viewport()->size() << "\n";
    }
}

void TdQuickViewer::PreviewFinished()
{
#ifdef EVAL_PERFORMANCE
    if (0 == (--_evalCnt))
    {
        EvalPerformanceEnd();
    }
#endif
}

void TdQuickViewer::ProcessTask()
{
    for (auto it = _tasks.begin(); it != _tasks.end();)
    {
        TopoDS_Shape shape;
        if (TS_Ready == it->stage)
        {
            if (StepReader::Instance().GetShape(it->filename, false, shape))
            {
                it->stage = TS_LoadFileDone;

                Handle(AIS_InteractiveContext) ctx;
                ShapeTessellater::Instance().Do(ctx, shape);
            }
        }

        if (TS_LoadFileDone == it->stage)
        {
            if (StepReader::Instance().GetShape(it->filename, false, shape)) // �ظ�����
            {
                if (ShapeTessellater::Instance().Done(shape, false))
                {
                    it->stage = TS_TessellateShapeDone;

                    int id = it->id;
                    const int row = id / 3;
                    const int col = (id - row * _colCnt) % 3;

                    QLayoutItem* item = _layout->itemAtPosition(row, col);
                    if (item && item->widget())
                    {
                        TdPreviewWidget* widget = static_cast<TdPreviewWidget*>(item->widget());
                        widget->ResetShape(shape, it->filename);
                        widget->show();

                        //widget->UpdateFilename(filenames[i], int(i + 1), (int)filenames.size());
                    }
                    qDebug() << "Done: " << it->id << " " << it->filename;

                    it = _tasks.erase(it);
                }
            }
        }
        else
        {
            ++it;
        }
    }
}

void TdQuickViewer::IncreasePreviewWidget(int total_cnt)
{
    const int exist_count = _layout->count();
    const int col_cnt = _colCnt; // ������
    for (int i = 0; i < total_cnt; i++) 
    {
        const int row = i / 3;
        const int col = (i - row * col_cnt) % 3;

        if (i >= exist_count) // ���������
        {
            TdPreviewWidget* widget = new TdPreviewWidget(/*"", */this);
            connect(widget, &TdPreviewWidget::finished, this, &TdQuickViewer::PreviewFinished);
            _layout->addWidget(widget, row, col, 1, 1);
        }
        else
        { // ��ʾԭ�����ص�
            //QLayoutItem* item = _layout->itemAtPosition(row, col);
            //if (item && item->widget())
            //{
            //    item->widget()->show();
            //}
        }
    }
    for (int i = total_cnt; i < exist_count; i++) // ���ض���Ŀؼ�
    {
        const int row = i / 3;
        const int col = (i - row * col_cnt) % 3;
        QLayoutItem* item = _layout->itemAtPosition(row, col);
        if (item && item->widget())
        {
            item->widget()->hide();
        }
    }

#ifdef EVAL_PERFORMANCE
    _evalCnt = total_cnt;
#endif
}

void TdQuickViewer::EvalPerformanceStart()
{
#ifdef EVAL_PERFORMANCE
    _evalTimer = new QElapsedTimer;
    _evalTimer->start();

    OnFolderPressed(_evalPath);
#endif
}

void TdQuickViewer::EvalPerformanceEnd()
{
#ifdef EVAL_PERFORMANCE
    QString elapsed = QTime::fromMSecsSinceStartOfDay(_evalTimer->elapsed()).toString();

    // write log
    QFile file(_evalPath + "/records.txt");
    if (!file.open(QIODevice::Append | QIODevice::Text))
    {
        qDebug() << "records.txt open failed!";
    }

    qDebug() << "Eval finished elapsed time " << elapsed << "\n";

    QTextStream out(&file);
    out << "Test at, " << QDateTime::currentDateTime().toString()
        << ", elapsed time, " << elapsed << "\n";

    delete _evalTimer;
    _evalTimer = nullptr;
#endif
}

void TdQuickViewer::UpdateTasks(const QString& folder_path)
{
    _tasks.clear();
    int id = 0;

    QFileInfo check_path(folder_path);
    if (check_path.isDir())
    {
        QDir dir(folder_path);
        QList<QFileInfo> objects = dir.entryInfoList();

        for (const QFileInfo& info : objects)
        {
            if (info.isFile())
            {
                Task task;
                task.id = id++;
                task.filename = info.absoluteFilePath();
                task.stage = TS_Ready;
                _tasks.emplace_back(task);
                //filenames.emplace_back(info.absoluteFilePath());
            }
        }
    }
    else if (check_path.isFile())
    {
        Task task;
        task.id = id++;
        task.filename = check_path.absoluteFilePath();
        task.stage = TS_Ready;
        _tasks.emplace_back(task);
        //filenames.emplace_back(check_path.absoluteFilePath());
    }

}
