#pragma once

#include <vector>
#include <memory>

#include <QHash>

class QString;
class TopoDS_Shape;
class STEPControl_Reader;

struct ReaderData;

// �첽���ļ�������ģ��
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
    // �������ô���ȡ�ļ�
    void Reset(const std::vector<QString>& filenames);

    // ��ȡģ��
    // return:  ģ���Ƿ��Ѽ������
    // [in]     block   ������ģ�ͻ�ȡģ�ͣ����ȴ�ģ�ʹ�����ɲ����ؽ��
    // [out]    shape   �Ѽ�����ɵ�ģ��
    bool GetShape(const QString& filename, bool block, TopoDS_Shape& shape) const;

private:
    // ��ȡ�ļ�
    void LoadingThread();

    // �����ļ�
    std::shared_ptr<STEPControl_Reader> LoadFile(const QString& filename) const;

    // ת��ģ��
    void TransferringThread();

    TopoDS_Shape TransferShape(STEPControl_Reader& reader) const;

private:
    std::shared_ptr<ReaderData> _d;
};

