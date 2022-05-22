#include "step_reader.h"

#include <mutex>
#include <condition_variable>
#include <list>

#include <TopoDS_Shape.hxx>
#include <STEPControl_Reader.hxx>
#include <TopoDS_Builder.hxx>

struct Data
{
    // 同步控制
    std::mutex mtxLoader, mtxTransfer;
    std::condition_variable cvLoader, cvTransfer;
    std::thread threadLoader;
    bool readyToLoad = false;
    bool readyToTransfer = false;

    // 待处理的文件名
    std::mutex mtxFilenames;
    std::list<QString> filenames;

    // 结果
    std::mutex mtxShapes;
    QHash<QString, TopoDS_Shape> shapes; // 已完成处理的模型

    // 结束
    bool finished = false;
};

StepReader& StepReader::Instance()
{
    static StepReader sr;
    return sr;
}

StepReader::StepReader()
{
    _d = std::make_shared<Data>();

    Loading();
}

StepReader::~StepReader()
{
    _d->finished = true;
    if (_d->threadLoader.joinable())
    {
        _d->threadLoader.join();
    }
}

void StepReader::Reset(const std::vector<QString>& filenames)
{
    {
        std::lock_guard<std::mutex> lock(_d->mtxShapes);
        _d->shapes.clear();
    }

    {
        std::lock_guard<std::mutex> lock(_d->mtxFilenames);
        _d->filenames.assign(filenames.begin(), filenames.end());

        _d->readyToLoad = true;
        _d->cvLoader.notify_one();
    }
}

// 1. 检查是否有结果
// 2. 如无结果，等待结果状态
// 3. 重复 1 步骤判断结果是否是所需的；
//TopoDS_Shape GetShape(const QString& filename)
//{
//    while (true)
//    {
//        {
//            std::lock_guard<std::mutex> lock(_d->mtxShapes);
//            if (_d->fileShapes.find(filename) != _d->fileShapes.end())
//            {
//                TopoDS_Shape shape = _d->fileShapes[filename];
//                return shape;
//            }
//        }
//        
//        { // 尚未处理完成，等待
//            std::unique_lock<std::mutex> lock(_d->mtxShapes);
//            _d->cvShapes.wait(lock, [&] {return _d->gotResult; });
//
//            // 有结果后，需要继续循环检查
//        }
//    }
//
//    assert(0);
//    return TopoDS_Shape();
//}

bool StepReader::GetShape(const QString& filename, bool block, TopoDS_Shape& shape) const
{
    do
    {
        {
            std::lock_guard<std::mutex> lock(_d->mtxShapes);
            auto it = _d->shapes.find(filename);
            if (it != _d->shapes.end())
            { // 已完成处理
                shape = *it;
                return true;
            }
        }

        // 未完成处理
        if (block)
        { // 是否在队列中
            std::lock_guard<std::mutex> lock(_d->mtxFilenames);
            auto it = _d->filenames.begin();
            for (; it != _d->filenames.end(); ++it)
            {
                if (filename == *it)
                {
                    break;
                }
            }
            if (it != _d->filenames.end()) // 存在
            {
                using namespace std::chrono;
                std::this_thread::sleep_for(100ms);
            }
            else
            {
                return false;
            }
        }
        else
        {
            return false;
        }
    } while (true);

    return false;
}

void StepReader::Loading()
{
    _d->threadLoader = std::thread([&]
        {
            while (!_d->finished)
            {
                {
                    std::unique_lock<std::mutex> lock(_d->mtxLoader);
                    _d->cvLoader.wait(lock, [&] {return _d->readyToLoad; });
                }
                
                QString filename;
                {
                    std::lock_guard<std::mutex> lock(_d->mtxFilenames);
                    if (!_d->filenames.empty())
                    {
                        filename = _d->filenames.front();
                        _d->filenames.pop_front();
                    }
                    else
                    {
                        _d->readyToLoad = false;
                        continue;
                    }
                }

                TopoDS_Shape shape = LoadFile(filename);
                {
                    std::lock_guard<std::mutex> lock(_d->mtxShapes);
                    _d->shapes[filename] = shape;
                }
            }
        });
}

TopoDS_Shape StepReader::LoadFile(const QString& filename) const
{
    STEPControl_Reader reader;
    TCollection_AsciiString  aFilePath = filename.toUtf8().data();
    IFSelect_ReturnStatus status = reader.ReadFile(aFilePath.ToCString());
    if (IFSelect_RetDone != status)
    {
        return TopoDS_Shape(); // TODO: 读取模型带上出错信息
    }

    //bool failsonly = false;
    //reader.PrintCheckLoad(failsonly, IFSelect_ItemsByEntity);

    int nbr = reader.NbRootsForTransfer();
    //bool failsonly = false;
    //reader.PrintCheckTransfer(failsonly, IFSelect_ItemsByEntity);
    for (Standard_Integer n = 1; n <= nbr; n++)
    {
        reader.TransferRoot(n);
    }

    // 显示模型
    int nbs = reader.NbShapes();
    TopoDS_Builder builder;
    TopoDS_Compound comp;
    builder.MakeCompound(comp);
    if (nbs > 0)
    {
        for (int i = 1; i <= nbs; i++)
        {
            TopoDS_Shape shape = reader.Shape(i);
            builder.Add(comp, shape);
        }
    }
    return comp;
}

//void StepReader::Transferring()
//{
//    while (true)
//    {
//        {
//            std::unique_lock<std::mutex> lock(_d->mtxTransfer);
//            _d->cvTransfer.wait(lock, [&] {return _d->readyToTransfer; });
//        }
//
//        for (size_t i = 0; i < _d->filenames.size(); i++)
//        {
//            {
//                std::lock_guard<std::mutex> guard(_d->mtxShapes);
//                _d->gotResult = false; // 标记为无结果
//            }
//
//            QString filename;
//            {
//                std::lock_guard<std::mutex> guard(_d->mtxFilenames);
//                filename = _d->filenames[i];
//            }
//            if (filename.isEmpty())
//            {
//                continue;
//            }
//
//            { // 写模型
//                std::lock_guard<std::mutex> guard(_d->mtxShapes);
//                if (_d->shapes.find(filename) != _d->shapes.end())
//                {
//                    // TODO: 绝对路径相同的文件内容被修改后无法处理
//                    continue;
//                }
//                _d->shapes[filename] = TopoDS_Shape();
//            }
//
//            // TODO: 加载文件内容，得到 reader
//            TopoDS_Shape shape;
//
//            { // 更新模型
//                std::lock_guard<std::mutex> guard(_d->mtxShapes);
//                _d->shapes[filename] = shape;
//                _d->gotResult = true;
//                _d->cvShapes.notify_one();
//            }
//        }
//
//        _d->readyToLoad = false;
//    }
//}
