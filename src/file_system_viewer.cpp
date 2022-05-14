#include "file_system_viewer.h"

#include <QDir>
#include <QFileSystemModel>
#include <QMouseEvent>

FileSystemViewer::FileSystemViewer(QWidget* parent) : QTreeView(parent)
{
    _model = new QFileSystemModel(this);
    _model->setRootPath(QDir::currentPath());
    this->setModel(_model);
}

FileSystemViewer::~FileSystemViewer()
{
}

void FileSystemViewer::mousePressEvent(QMouseEvent* event)
{
    QModelIndex index = this->indexAt(event->pos());
    if (index.row() >= 0)
    {
        if (!this->selectedIndexes().isEmpty())
        {
            QString filepath = _model->filePath(index);
            emit FolderPressed(filepath);
        }
    }
    QTreeView::mousePressEvent(event);
}
