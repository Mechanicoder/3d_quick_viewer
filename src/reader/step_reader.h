#pragma once

#include <vector>
#include <memory>

#include <QHash>

class QString;
class TopoDS_Shape;
class STEPControl_Reader;

struct ReaderData;

// 异步读文件并加载模型
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
    void LoadingThread();

    // 加载文件
    std::shared_ptr<STEPControl_Reader> LoadFile(const QString& filename) const;

    // 转换模型
    void TransferringThread();

    TopoDS_Shape TransferShape(STEPControl_Reader& reader) const;

private:
    std::shared_ptr<ReaderData> _d;
};

