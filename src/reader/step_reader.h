#pragma once

#include <vector>
#include <memory>

#include <QHash>

class QString;
class TopoDS_Shape;

struct Data;

// 显示预览控件
class StepReader
{
public:
    static StepReader& Instance();

private:
    StepReader();
    ~StepReader();
    StepReader(const StepReader&) = delete;
    StepReader& operator=(const StepReader&) = delete;

public:
    // 跳过正在处理的文件
    void Load(const std::vector<QString>& filenames);

    TopoDS_Shape GetShape(const QString& filename) const;

private:
    // 读取文件
    void Loading();

    // 转换模型
    void Transferring();

private:

    // 保存所有已读取过的模型：同名但内容修改的文件？
    QHash<QString, TopoDS_Shape> _fileShapes;

    std::shared_ptr<Data> _d;

    std::shared_ptr<std::thread> _threadLoader;
    std::shared_ptr<std::thread> _threadTransfer;
};

