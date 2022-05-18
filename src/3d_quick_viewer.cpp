#include "3d_quick_viewer.h"
#include "file_system_viewer.h"
#include "3d_preview_widget.h"
#include "ui_3d_quick_viewer.h"

#include <QFileSystemModel>
#include <QFileInfo>
#include <QVector>
#include <QDebug>
#include <QScrollBar>
#include <QTime>
#include <QDateTime>
#include <QElapsedTimer>

#define TEST_PATH ("../0000_STEP_TEST/EVAL_TIME")

TdQuickViewer::TdQuickViewer(QWidget* parent) : QMainWindow(parent), _colCnt(3), _timer(nullptr)
{
    _ui = new Ui::TdQuickViewerUi;
    _ui->setupUi(this);

    _layout = new QGridLayout(_ui->scrollArea_preview->widget());
    _layout->setSizeConstraint(QLayout::SetMinimumSize);
    _layout->setSpacing(6);

    connect(_ui->treeView_path, &FileSystemViewer::FolderPressed, this, &TdQuickViewer::OnFolderPressed);

    EvalPerformance();
}

TdQuickViewer::~TdQuickViewer()
{
    if (_timer)
    {
        delete _timer;
    }
}

void TdQuickViewer::OnFolderPressed(const QString& filepath)
{
    QVector<QString> filenames;
    QFileInfo check_path(filepath);
    if (check_path.isDir())
    {
        QDir dir(filepath);
        QList<QFileInfo> objects = dir.entryInfoList();

        for (const QFileInfo& info : objects)
        {
            if (info.isFile())
            {
                filenames.append(info.absoluteFilePath());
            }
        }
    }
    else if (check_path.isFile())
    {
        filenames.append(check_path.absoluteFilePath());
    }

    // reader

    if (!filenames.isEmpty())
    {
        // 增加3D预览视图
        IncreasePreviewWidget(filenames.size());

        QApplication::processEvents();

        // 多线程加载文件

        // 设置模型

        const int exist_count = _layout->count();
        const int col_cnt = _colCnt; // 列数量

        for (int i = 0; i < filenames.size(); i++)
        {
            QApplication::processEvents();

            const int row = i / 3;
            const int col = (i - row * col_cnt) % 3;
            if (i >= exist_count)
            {
                TdPreviewWidget* widget = new TdPreviewWidget(filenames[i], this);
                _layout->addWidget(widget, row, col, 1, 1);
                widget->show();
            }
            else
            {
                QLayoutItem* item = _layout->itemAtPosition(row, col);
                if (item && item->widget())
                {
                    static_cast<TdPreviewWidget*>(item->widget())->UpdateFilename(filenames[i]);
                    item->widget()->show();
                }
            }
            qDebug() << "Done: " << filenames[i];
        }

        // 隐藏其余的
        for (int i = filenames.size(); i < exist_count; i++)
        {
            const int row = i / 3;
            const int col = (i - row * col_cnt) % 3;
            QLayoutItem* item = _layout->itemAtPosition(row, col);
            if (item && item->widget())
            {
                item->widget()->hide();
            }
        }

        qDebug() << "File path update finished! Viewport size " 
            << _ui->scrollArea_preview->viewport()->size() << "\n";
    }
}

void TdQuickViewer::IncreasePreviewWidget(int total_cnt)
{
    const int exist_count = _layout->count();
    const int col_cnt = _colCnt; // 列数量
    for (int i = exist_count; i < total_cnt; i++)
    {
        const int row = i / 3;
        const int col = (i - row * col_cnt) % 3;

        TdPreviewWidget* widget = new TdPreviewWidget("", this);
        _layout->addWidget(widget, row, col, 1, 1);
    }
}

void TdQuickViewer::EvalPerformance()
{
    connect(this, &TdQuickViewer::EvalTimeFinished, this,
        [&]()
        {
            QString elapsed = QTime::fromMSecsSinceStartOfDay(_timer->elapsed()).toString();

            // write log
            QFile file(QString(TEST_PATH) + "/records.txt");
            if (!file.open(QIODevice::Append | QIODevice::Text))
            {
                qDebug() << "records.txt open failed!";
            }

            qDebug() << "Eval finished elapsed time " << elapsed << "\n";

            QTextStream out(&file);
            out << "Test at, " << QDateTime::currentDateTime().toString()
                << ", elapsed time, " << elapsed << "\n";
        });

    _timer = new QElapsedTimer;
    _timer->start();

    OnFolderPressed(QString(TEST_PATH));
    emit EvalTimeFinished();
}
