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
    // 重新设置待读取文件
    void Reset(const std::vector<QString>& filenames);

    // 获取模型
    // return:  模型是否已加载完成
    // [in]     block   以阻塞模型获取模型，即等待模型处理完成并返回结果
    // [out]    shape   已加载完成的模型
    bool GetShape(const QString& filename, bool block, TopoDS_Shape& shape) const;

private:
    // 读取文件
    void Loading();

    // 转换模型
    //void Transferring();

    // 加载文件
    TopoDS_Shape LoadFile(const QString& filename) const;

private:
    std::shared_ptr<Data> _d;
};

