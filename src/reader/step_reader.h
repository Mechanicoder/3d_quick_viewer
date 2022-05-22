#pragma once

#include <vector>
#include <memory>

#include <QHash>

class QString;
class TopoDS_Shape;

struct Data;

// ��ʾԤ���ؼ�
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
    void Loading();

    // ת��ģ��
    //void Transferring();

    // �����ļ�
    TopoDS_Shape LoadFile(const QString& filename) const;

private:
    std::shared_ptr<Data> _d;
};

