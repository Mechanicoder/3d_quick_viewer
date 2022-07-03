// Mechanicoder
// 2022/06/04

#include "file_system_viewer.h"

#include <QDir>
#include <QFileSystemModel>
#include <QMouseEvent>

FileSystemViewer::FileSystemViewer(QWidget* parent) : QTreeView(parent)
{
    _model = new QFileSystemModel(this);
    _model->setRootPath(QDir::currentPath());
    this->setModel(_model);

    for (int i = 1; i < _model->columnCount(); i++)
    {
        this->setColumnHidden(i, true);
    }

    _prevIndex = new QModelIndex();
}

FileSystemViewer::~FileSystemViewer()
{
}

void FileSystemViewer::UpdateRootPath(const QString& filepath)
{
    if (filepath.isEmpty())
    {
        this->setRootIndex(QModelIndex());
    }
    else
    {
        QFileInfo check_path(filepath);
        if (check_path.isDir())
        {
            this->setRootIndex(_model->index(filepath));
        }
        else if (check_path.isFile())
        {
            this->setRootIndex(_model->index(check_path.absolutePath()));
        }
    }
}

void FileSystemViewer::mousePressEvent(QMouseEvent* event)
{
    QModelIndex index = this->indexAt(event->pos());
    if (index.row() >= 0)
    {
        if (!this->selectedIndexes().isEmpty() && (*_prevIndex != index))
        {
            QString filepath = _model->filePath(index);
            emit FolderPressed(filepath);

            *_prevIndex = index;
        }
    }
    QTreeView::mousePressEvent(event);
}
