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
#include <QThread>

TdQuickViewer::TdQuickViewer(QWidget* parent) : QMainWindow(parent), _colCnt(3)
{
    _ui = new Ui::TdQuickViewerUi;
    _ui->setupUi(this);

    _layout = new QGridLayout(_ui->scrollArea_preview->widget());
    _layout->setSizeConstraint(QLayout::SetMinimumSize);
    _layout->setSpacing(6);

    connect(_ui->treeView_path, &FileSystemViewer::FolderPressed, this, &TdQuickViewer::OnFolderPressed);

#ifdef EVAL_PERFORMANCE
    _timer = nullptr;
    _evalPath = QCoreApplication::applicationDirPath() + "/../0000_STEP_TEST/EVAL_TIME";
    EvalPerformanceStart();
#endif
}

TdQuickViewer::~TdQuickViewer()
{
}

void TdQuickViewer::OnFolderPressed(const QString& filepath)
{
    std::vector<QString> filenames;
    QFileInfo check_path(filepath);
    if (check_path.isDir())
    {
        QDir dir(filepath);
        QList<QFileInfo> objects = dir.entryInfoList();

        for (const QFileInfo& info : objects)
        {
            if (info.isFile())
            {
                filenames.emplace_back(info.absoluteFilePath());
            }
        }
    }
    else if (check_path.isFile())
    {
        filenames.emplace_back(check_path.absoluteFilePath());
    }

    // reader

    if (!filenames.empty())
    {
        // 增加3D预览视图
        IncreasePreviewWidget(int(filenames.size()));

        StepReader::Instance().Reset(filenames);
        ShapeTessellater::Instance().Reset();

        QApplication::processEvents();

        // 多线程加载文件

        // 设置模型

        const int exist_count = _layout->count();
        const int col_cnt = _colCnt; // 列数量

        for (int i = 0; i < filenames.size(); i++)
        {
            const int row = i / 3;
            const int col = (i - row * col_cnt) % 3;

            QLayoutItem* item = _layout->itemAtPosition(row, col);
            if (item && item->widget())
            {
                TdPreviewWidget* widget = static_cast<TdPreviewWidget*>(item->widget());
                widget->UpdateFilename(filenames[i], int(i + 1), (int)filenames.size());
            }
            qDebug() << "Done: " << filenames[i];
        }

        qDebug() << "File path update finished! Viewport size " 
            << _ui->scrollArea_preview->viewport()->size() << "\n";
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

void TdQuickViewer::IncreasePreviewWidget(int total_cnt)
{
    const int exist_count = _layout->count();
    const int col_cnt = _colCnt; // 列数量
    for (int i = 0; i < total_cnt; i++) 
    {
        const int row = i / 3;
        const int col = (i - row * col_cnt) % 3;

        if (i >= exist_count) // 新增不足的
        {
            TdPreviewWidget* widget = new TdPreviewWidget(/*"", */this);
            connect(widget, &TdPreviewWidget::finished, this, &TdQuickViewer::PreviewFinished);
            _layout->addWidget(widget, row, col, 1, 1);
        }
        else
        { // 显示原被隐藏的
            //QLayoutItem* item = _layout->itemAtPosition(row, col);
            //if (item && item->widget())
            //{
            //    item->widget()->show();
            //}
        }
    }
    for (int i = total_cnt; i < exist_count; i++) // 隐藏多余的控件
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
    _timer = new QElapsedTimer;
    _timer->start();

    OnFolderPressed(_evalPath);
#endif
}

void TdQuickViewer::EvalPerformanceEnd()
{
#ifdef EVAL_PERFORMANCE
    QString elapsed = QTime::fromMSecsSinceStartOfDay(_timer->elapsed()).toString();

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

    delete _timer;
    _timer = nullptr;
#endif
}
