// Mechanicoder
// 2022/06/04

#pragma once

#include <QTreeView>

// 文件树视图
class QDir;
class QFileSystemModel;

class FileSystemViewer : public QTreeView
{
    Q_OBJECT
public:
    FileSystemViewer(QWidget* parent = nullptr);
    ~FileSystemViewer();

    // 更新浏览根目录：<filepath> 可以文件路径或文件夹路径
    void UpdateRootPath(const QString& filepath);

protected:
    virtual void mousePressEvent(QMouseEvent* event) override;

signals:
    void FolderPressed(const QString& filepath);

private:
    QFileSystemModel* _model;
    QModelIndex* _prevIndex;
};
