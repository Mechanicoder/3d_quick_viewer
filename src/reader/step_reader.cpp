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
    // ��������ļ���
    BlockQueue<QString> filenames;

    // �����ļ�
    std::mutex mtxLoader;
    std::condition_variable cvLoader;
    std::thread threadLoader;

    // �м����
    typedef std::shared_ptr<STEPControl_Reader> ReaderPtr;
    BlockQueue<std::pair<QString, ReaderPtr>> processingShapes;

    // ģ��ת��
    std::vector<std::thread> threadTrsfer;

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

// 1. ����Ƿ��н��
// ��2. ���޽��������Ƿ����ڴ��������
// 3. �ظ� 1 �����жϽ���Ƿ�������ģ�
bool StepReader::GetShape(const QString& filename, bool block, TopoDS_Shape& shape) const
{
    do
    {
        {
            //  #1
            std::lock_guard<std::mutex> lock(_d->mtxShapes);
            auto it = _d->shapes.find(filename);
            if (it != _d->shapes.end())
            { // ����ɴ���
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
#endif
        {
            return false;
        }
    } while (true);

    return false;
}

// ��������ļ����У���������洢���м�ģ�Ͷ�����
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
        return nullptr; // TODO: ��ȡģ�ʹ��ϳ�����Ϣ
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
                // ʹ�������߳�ʱ��Ϊ�α����ڴˣ�
                shape = TransferShape(*info.second);
            }

            {
                std::lock_guard<std::mutex> lock(_d->mtxShapes);
                _d->shapes[info.first] = shape; // ����Ϊ��
            }
        }
    };

    // ���ܷ������֣�ģ��ת����ʱ��Լ�Ǽ����ļ��� 2 �������������� 1 -> 2 �ķ�ʽ��
    //      1 ���̼߳����ļ���2 ���߳�ת��ģ��
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
