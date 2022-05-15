#pragma once

#include <QTreeView>

// �ļ�����ͼ
class QDir;
class QFileSystemModel;

class FileSystemViewer : public QTreeView
{
    Q_OBJECT
public:
    FileSystemViewer(QWidget* parent = nullptr);
    ~FileSystemViewer();

protected:
    virtual void mousePressEvent(QMouseEvent* event) override;

signals:
    void FolderPressed(const QString& filepath);

private:
    QFileSystemModel* _model;
    QModelIndex* _prevIndex;
};
