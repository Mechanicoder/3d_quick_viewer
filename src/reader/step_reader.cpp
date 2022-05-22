#include "step_reader.h"

#include <mutex>
#include <condition_variable>
#include <list>

#include <TopoDS_Shape.hxx>
#include <STEPControl_Reader.hxx>
#include <TopoDS_Builder.hxx>

struct ReaderData
{
    // ��������ļ���
    std::mutex mtxFilenames;
    std::list<QString> filenames;
    bool readyToLoad = false;

    // ͬ�����ƣ�״̬����
    std::mutex mtxLoader, mtxTransfer;
    std::condition_variable cvLoader, cvTransfer;
    std::thread threadLoader;
    //bool readyToTransfer = false;
    
    // �м�ģ��
    std::mutex mtxProcShapes;
    std::list<std::pair<QString, TopoDS_Shape>> procShapes;

    // ͬ�����ƣ���ѯ
    std::thread threadTesselater;

    // ���
    std::mutex mtxShapes;
    QHash<QString, TopoDS_Shape> shapes; // ����ɴ����ģ��

    // ����
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
        std::lock_guard<std::mutex> lock(_d->mtxProcShapes);
        _d->procShapes.clear();
    }

    {
        std::lock_guard<std::mutex> lock(_d->mtxFilenames);
        _d->filenames.assign(filenames.begin(), filenames.end());

        _d->readyToLoad = true;
        _d->cvLoader.notify_one();
    }
}

// 1. ����Ƿ��н��
// 2. ���޽�����ȴ����״̬
// 3. �ظ� 1 �����жϽ���Ƿ�������ģ�
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
//        { // ��δ������ɣ��ȴ�
//            std::unique_lock<std::mutex> lock(_d->mtxShapes);
//            _d->cvShapes.wait(lock, [&] {return _d->gotResult; });
//
//            // �н������Ҫ����ѭ�����
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
            { // ����ɴ���
                shape = *it;
                return true;
            }
        }

        // δ��ɴ���
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

            // ���ڶ����У��ȴ��������; �� Loading �У����ҽ�����ɴ���󣬲ŻὫ�ļ����Ӵ�����������Ƴ�
            if (it != _d->filenames.end())
            {
                using namespace std::chrono;
                std::this_thread::sleep_for(100ms); // ������ѯ��100ms
            }
            else
            {  // �޽������Ҳ���ڴ����������
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

// ��������ļ����У���������洢���м�ģ�Ͷ�����
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
                
                { // ����������ɺ󣬲ŴӶ������Ƴ�
                    std::lock_guard<std::mutex> lock(_d->mtxFilenames);
                    _d->filenames.pop_front();
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
        return TopoDS_Shape(); // TODO: ��ȡģ�ʹ��ϳ�����Ϣ
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

    // ��ʾģ��
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
//                _d->gotResult = false; // ���Ϊ�޽��
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
//            { // дģ��
//                std::lock_guard<std::mutex> guard(_d->mtxShapes);
//                if (_d->shapes.find(filename) != _d->shapes.end())
//                {
//                    // TODO: ����·����ͬ���ļ����ݱ��޸ĺ��޷�����
//                    continue;
//                }
//                _d->shapes[filename] = TopoDS_Shape();
//            }
//
//            // TODO: �����ļ����ݣ��õ� reader
//            TopoDS_Shape shape;
//
//            { // ����ģ��
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
