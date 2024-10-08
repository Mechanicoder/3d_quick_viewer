// Mechanicoder
// 2022/06/04

#pragma once

#include <vector>
#include <memory>

#include <QHash>

#include <Standard_Handle.hxx>

class QString;
class TopoDS_Shape;
class AIS_InteractiveContext;

struct TessellaterData;
struct TessellateShapeInfo;

// ��ʾԤ���ؼ�
class ShapeTessellater
{
public:
    static ShapeTessellater& Instance();

private:
    ShapeTessellater();
    ~ShapeTessellater();
    ShapeTessellater(const ShapeTessellater&) = delete;
    ShapeTessellater& operator=(const ShapeTessellater&) = delete;

public:
    void Reset();

    // ��ɢģ�ͣ���ɢģ���Դ����� shape ��
    // [in]     block               ������ģʽ��ɢģ��
    // [in]     deflection          �Ҹ����
    // [in]     angle_deflection    �Ҹ����
    void Do(TopoDS_Shape& shape);

    // shape �Ƿ������ɢ
    // [in]     block               �Ƿ������ȴ�
    bool Done(const TopoDS_Shape& shape, bool block);

    void Stop();

private:
    // Ԥ��ɢģ��
    void Tesselating();

    void TessellateShape(const TessellateShapeInfo& info) const;

private:
    std::shared_ptr<TessellaterData> _d;
};

