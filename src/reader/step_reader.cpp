#include "step_reader.h"

#include <mutex>
#include <condition_variable>

#include <TopoDS_Shape.hxx>

struct Data
{
    std::mutex mtxLoader, mtxTransfer;
    std::condition_variable cvLoader, cvTransfer;

    // 不采用判断 filenames 是否为空的方式，避免出现奇异
    bool readyToLoad, readyToTransfer;

    std::mutex mtxFilenames;
    std::vector<QString> filenames;

    // 结果
    std::mutex mtxShapes;
    QHash<QString, TopoDS_Shape> fileShapes;
    std::condition_variable cvShapes;
    bool gotResult;
};

StepReader& StepReader::Instance()
{
    static StepReader sr;
    return sr;
}

StepReader::StepReader()
{
    _d = std::make_shared<Data>();
}

StepReader::~StepReader()
{
}

void StepReader::Load(const std::vector<QString>& filenames)
{

}

// 1. 检查是否有结果
// 2. 如无结果，等待结果状态
// 3. 重复 1 步骤判断结果是否是所需的；
TopoDS_Shape StepReader::GetShape(const QString& filename) const
{
    while (true)
    {
        {
            std::lock_guard<std::mutex> lock(_d->mtxShapes);
            if (_d->fileShapes.find(filename) != _d->fileShapes.end())
            {
                TopoDS_Shape shape = _d->fileShapes[filename];
                return shape;
            }
        }
        
        { // 尚未处理完成，等待
            std::unique_lock<std::mutex> lock(_d->mtxShapes);
            _d->cvShapes.wait(lock, [&] {return _d->gotResult; });

            // 有结果后，需要继续循环检查
        }
    }

    assert(0);
    return TopoDS_Shape();
}

void StepReader::Loading()
{
    while (true)
    {
        {
            std::unique_lock<std::mutex> lock(_d->mtxLoader);
            _d->cvLoader.wait(lock, [&] {return _d->readyToLoad; });
        }

        for (size_t i = 0; i < _d->filenames.size(); i++)
        {
            QString filename;
            {
                std::lock_guard<std::mutex> guard(_d->mtxFilenames);
                filename = _d->filenames[i];
            }
            if (filename.isEmpty())
            {
                continue;
            }

            // TODO: 加载文件内容，得到 reader


            // 文件读取完成

            _d->readyToTransfer = true;
            _d->cvTransfer.notify_one();
        }

        _d->readyToLoad = false;
    }
}

void StepReader::Transferring()
{
    while (true)
    {
        {
            std::unique_lock<std::mutex> lock(_d->mtxTransfer);
            _d->cvTransfer.wait(lock, [&] {return _d->readyToTransfer; });
        }

        for (size_t i = 0; i < _d->filenames.size(); i++)
        {
            {
                std::lock_guard<std::mutex> guard(_d->mtxShapes);
                _d->gotResult = false; // 标记为无结果
            }

            QString filename;
            {
                std::lock_guard<std::mutex> guard(_d->mtxFilenames);
                filename = _d->filenames[i];
            }
            if (filename.isEmpty())
            {
                continue;
            }

            { // 写模型
                std::lock_guard<std::mutex> guard(_d->mtxShapes);
                if (_d->fileShapes.find(filename) != _d->fileShapes.end())
                {
                    // TODO: 绝对路径相同的文件内容被修改后无法处理
                    continue;
                }
                _d->fileShapes[filename] = TopoDS_Shape();
            }

            // TODO: 加载文件内容，得到 reader
            TopoDS_Shape shape;

            { // 更新模型
                std::lock_guard<std::mutex> guard(_d->mtxShapes);
                _d->fileShapes[filename] = shape;
                _d->gotResult = true;
                _d->cvShapes.notify_one();
            }
        }

        _d->readyToLoad = false;
    }
}
