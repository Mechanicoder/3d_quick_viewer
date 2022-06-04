#include "step_reader.h"
#include "../block_queue.h"

#include <mutex>
#include <condition_variable>
#include <list>

#include <TopoDS_Shape.hxx>
#include <STEPControl_Reader.hxx>
#include <TopoDS_Builder.hxx>

struct ReaderData
{
    // 待处理的文件名
    BlockQueue<QString> filenames;

    // 加载文件
    std::mutex mtxLoader;
    std::condition_variable cvLoader;
    std::thread threadLoader;

    // 中间过程
    typedef std::shared_ptr<STEPControl_Reader> ReaderPtr;
    BlockQueue<std::pair<QString, ReaderPtr>> processingShapes;

    // 模型转换
    std::vector<std::thread> threadTrsfer;

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
    _d = std::make_shared<ReaderData>();

    LoadingThread();
    TransferringThread();
}

StepReader::~StepReader()
{
    _d->finished = true;
    if (_d->threadLoader.joinable())
    {
        _d->threadLoader.join();
    }

    for (size_t i = 0; i < _d->threadTrsfer.size(); i++)
    {
        if (_d->threadTrsfer[i].joinable())
        {
            _d->threadTrsfer[i].join();
        }
    }
}

void StepReader::Reset(const std::vector<QString>& filenames)
{
    {
        std::lock_guard<std::mutex> lock(_d->mtxShapes);
        _d->shapes.clear();
    }

    _d->processingShapes.Clear();

    _d->filenames.Push(filenames.cbegin(), filenames.end());

    _d->cvLoader.notify_one();
}

// 1. 检查是否有结果
// ×2. 如无结果，检查是否正在处理过程中
// 3. 重复 1 步骤判断结果是否是所需的；
bool StepReader::GetShape(const QString& filename, bool block, TopoDS_Shape& shape) const
{
    do
    {
        {
            //  #1
            std::lock_guard<std::mutex> lock(_d->mtxShapes);
            auto it = _d->shapes.find(filename);
            if (it != _d->shapes.end())
            { // 已完成处理
                shape = *it;
                return true;
            }
        }

        //  #2
#if 0
        if (block)
        {
            std::lock_guard<std::mutex> lock(_d->mtxFilenames);
            auto it = _d->filenames.begin();
            for (; it != _d->filenames.end(); ++it)
            {
                if (filename == *it)
                {
                    break;
                }
            }

            // 尚在队列中，等待处理完成; 在 Loading 中，当且仅当完成处理后，才会将文件名从待处理队列中移除
            if (it != _d->filenames.end())
            {
                using namespace std::chrono;
                std::this_thread::sleep_for(100ms); // 阻塞轮询：100ms
            }
            else
            {  // 无结果，且也不在待处理队列中
                return false;
            }
        }
        else
#endif
        {
            return false;
        }
    } while (true);

    return false;
}

// 逐个处理文件队列，并将结果存储在中间模型队列中
void StepReader::LoadingThread()
{
    _d->threadLoader = std::thread([&]
        {
            while (!_d->finished)
            {
                {
                    std::unique_lock<std::mutex> lock(_d->mtxLoader);
                    _d->cvLoader.wait(lock, [&] {return !_d->filenames.Empty(); });
                }

                QString filename;
                if (!_d->filenames.NotEmptyThenPop(filename))
                {
                    continue;
                }

                auto reader = LoadFile(filename);
                _d->processingShapes.Push(std::make_pair(filename, reader));
            }
        });
}

std::shared_ptr<STEPControl_Reader> StepReader::LoadFile(const QString& filename) const
{
    std::shared_ptr<STEPControl_Reader> reader = std::make_shared<STEPControl_Reader>();
    TCollection_AsciiString  aFilePath = filename.toUtf8().data();
    IFSelect_ReturnStatus status = reader->ReadFile(aFilePath.ToCString());
    if (IFSelect_RetDone != status)
    {
        return nullptr; // TODO: 读取模型带上出错信息
    }
    return reader;
}

void StepReader::TransferringThread()
{
    auto L_E_Fun = [&]
    {
        while (!_d->finished)
        {
            std::pair<QString, ReaderData::ReaderPtr> info;
            while (!_d->processingShapes.NotEmptyThenPop(info))
            {
                using namespace std::chrono;
                std::this_thread::sleep_for(100ms);
                continue;
            }

            TopoDS_Shape shape;
            if (info.second)
            {
                // 使用两个线程时，为何崩溃在此？
                shape = TransferShape(*info.second);
            }

            {
                std::lock_guard<std::mutex> lock(_d->mtxShapes);
                _d->shapes[info.first] = shape; // 可能为空
            }
        }
    };

    // 性能分析发现：模型转换耗时大约是加载文件的 2 倍，因此这里采用 1 -> 2 的方式：
    //      1 个线程加载文件，2 个线程转换模型
    const size_t n = 1;
    for (size_t i = 0; i < n; i++)
    {
        _d->threadTrsfer.emplace_back(std::thread(L_E_Fun));
    }
}

TopoDS_Shape StepReader::TransferShape(STEPControl_Reader& reader) const
{
    int nbr = reader.NbRootsForTransfer();
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
