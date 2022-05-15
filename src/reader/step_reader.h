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
    // �������ڴ�����ļ�
    void Load(const std::vector<QString>& filenames);

    TopoDS_Shape GetShape(const QString& filename) const;

private:
    // ��ȡ�ļ�
    void Loading();

    // ת��ģ��
    void Transferring();

private:

    // ���������Ѷ�ȡ����ģ�ͣ�ͬ���������޸ĵ��ļ���
    QHash<QString, TopoDS_Shape> _fileShapes;

    std::shared_ptr<Data> _d;

    std::shared_ptr<std::thread> _threadLoader;
    std::shared_ptr<std::thread> _threadTransfer;
};

