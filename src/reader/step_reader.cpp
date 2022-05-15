#include "step_reader.h"

#include <mutex>
#include <condition_variable>

#include <TopoDS_Shape.hxx>

struct Data
{
    std::mutex mtxLoader, mtxTransfer;
    std::condition_variable cvLoader, cvTransfer;

    // �������ж� filenames �Ƿ�Ϊ�յķ�ʽ�������������
    bool readyToLoad, readyToTransfer;

    std::mutex mtxFilenames;
    std::vector<QString> filenames;

    // ���
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

// 1. ����Ƿ��н��
// 2. ���޽�����ȴ����״̬
// 3. �ظ� 1 �����жϽ���Ƿ�������ģ�
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
        
        { // ��δ������ɣ��ȴ�
            std::unique_lock<std::mutex> lock(_d->mtxShapes);
            _d->cvShapes.wait(lock, [&] {return _d->gotResult; });

            // �н������Ҫ����ѭ�����
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

            // TODO: �����ļ����ݣ��õ� reader


            // �ļ���ȡ���

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
                _d->gotResult = false; // ���Ϊ�޽��
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

            { // дģ��
                std::lock_guard<std::mutex> guard(_d->mtxShapes);
                if (_d->fileShapes.find(filename) != _d->fileShapes.end())
                {
                    // TODO: ����·����ͬ���ļ����ݱ��޸ĺ��޷�����
                    continue;
                }
                _d->fileShapes[filename] = TopoDS_Shape();
            }

            // TODO: �����ļ����ݣ��õ� reader
            TopoDS_Shape shape;

            { // ����ģ��
                std::lock_guard<std::mutex> guard(_d->mtxShapes);
                _d->fileShapes[filename] = shape;
                _d->gotResult = true;
                _d->cvShapes.notify_one();
            }
        }

        _d->readyToLoad = false;
    }
}
