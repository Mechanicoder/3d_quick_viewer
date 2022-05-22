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

// 显示预览控件
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

    // 离散模型，离散模型仍存在于 shape 中
    // [in]     block               以阻塞模式离散模型
    // [in]     deflection          弦高误差
    // [in]     angle_deflection    弦高误差
    void Do(const Handle(AIS_InteractiveContext)& context, TopoDS_Shape& shape);

    // shape 是否完成离散
    // [in]     block               是否阻塞等待
    bool Done(const TopoDS_Shape& shape, bool block);

private:
    // 预离散模型
    void Tesselating();

    void TessellateShape(const TessellateShapeInfo& info) const;

private:
    std::shared_ptr<TessellaterData> _d;
};

