#include "3d_quick_viewer.h"
#include "file_system_viewer.h"
#include "3d_preview_widget.h"
#include "ui_3d_quick_viewer.h"

#include <QFileSystemModel>
#include <QFileInfo>
#include <QVector>
#include <QDebug>
#include <QScrollBar>

TdQuickViewer::TdQuickViewer(QWidget* parent) : QMainWindow(parent)
{
    _ui = new Ui::TdQuickViewerUi;
    _ui->setupUi(this);

    _layout = new QGridLayout(_ui->scrollArea_preview->widget());
    _layout->setSizeConstraint(QLayout::SetMinimumSize);
    _layout->setSpacing(6);

    connect(_ui->treeView_path, &FileSystemViewer::FolderPressed, this, &TdQuickViewer::OnFolderPressed);
}

TdQuickViewer::~TdQuickViewer()
{
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

    if (!filenames.isEmpty())
    {
        const int exist_count = _layout->count();
        const int col_cnt = 3; // 列数量

        for (int i = 0; i < filenames.size(); i++)
        {
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
            qDebug() << filenames[i];
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

        qDebug() << "layout: " << _layout->totalMinimumSize() << "  " << _layout->minimumSize();
        qDebug() << "viewport: " << _ui->scrollArea_preview->viewport()->size();
        qDebug() << "widget: " << _ui->scrollArea_preview->widget()->size();

        qDebug() << "\n";
    }
}