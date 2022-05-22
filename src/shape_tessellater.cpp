#include "shape_tessellater.h"

#include <mutex>
#include <condition_variable>
#include <list>
#include <unordered_map>

#include <TopoDS_Shape.hxx>
#include <BRepMesh_DiscretFactory.hxx>
#include <AIS_InteractiveContext.hxx>
#include <AIS_Shape.hxx>

int HashCode(const TopoDS_Shape& shape)
{
    const int upper_bnd = std::numeric_limits<int>::max();
    return shape.HashCode(upper_bnd);
}

struct TessellateShapeInfo
{
    TopoDS_Shape shape;
    Handle(AIS_InteractiveContext) context;
    //double deflection = 0.1;
    //double angleDeflection = 0.1;
};

struct TessellaterData
{
    std::thread threadTessellater;

    // 待处理模型: 按序处理
    std::mutex mtxProcShapes;
    std::list<TessellateShapeInfo> procShapes;

    // 处理后模型
    std::mutex mtxShapes;
    std::unordered_map<int, TopoDS_Shape> shapes;

    // 结束
    bool finished = false;
};

ShapeTessellater& ShapeTessellater::Instance()
{
    static ShapeTessellater sr;
    return sr;
}

ShapeTessellater::ShapeTessellater()
{
    _d = std::make_shared<TessellaterData>();

    Tesselating();
}

ShapeTessellater::~ShapeTessellater()
{
    _d->finished = true;
    if (_d->threadTessellater.joinable())
    {
        _d->threadTessellater.join();
    }
}

void ShapeTessellater::Reset()
{
    {
        std::lock_guard<std::mutex> lock(_d->mtxProcShapes);
        _d->procShapes.clear();
    }
    {
        std::lock_guard<std::mutex> lock(_d->mtxShapes);
        _d->shapes.clear();
    }
}

void ShapeTessellater::Do(const Handle(AIS_InteractiveContext)& context, TopoDS_Shape& shape)
{
    std::lock_guard<std::mutex> lock(_d->mtxProcShapes);
    TessellateShapeInfo info;
    info.shape = shape;
    info.context = context;
    //info.deflection = deflection;
    //info.angleDeflection = angle_deflection;
    _d->procShapes.emplace_back(info);
}

bool ShapeTessellater::Done(const TopoDS_Shape& shape, bool block)
{
    do
    {
        {
            std::lock_guard<std::mutex> lock(_d->mtxShapes);
            if (_d->shapes.find(HashCode(shape)) != _d->shapes.end())
            {
                return true;
            }
        }

        if (block)
        {
            std::lock_guard<std::mutex> lock(_d->mtxProcShapes);
            const int hc = HashCode(shape);
            auto it = _d->procShapes.begin();
            for (; it != _d->procShapes.end(); ++it)
            {
                if (hc == HashCode(it->shape))
                {
                    break;
                }
            }
            if (it != _d->procShapes.end())
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
        {
            return false;
        }
    } while (!_d->finished);

    return false;
}

// 轮询是否存在待处理模型
void ShapeTessellater::Tesselating()
{
    _d->threadTessellater = std::thread([&]
        {
            while (!_d->finished)
            {
                TessellateShapeInfo info;
                {
                    std::lock_guard<std::mutex> lock(_d->mtxProcShapes);
                    if (!_d->procShapes.empty())
                    {
                        info = _d->procShapes.front();
                    }
                    else
                    {
                        using namespace std::chrono;
                        std::this_thread::sleep_for(100ms); // 等待任务
                        continue;
                    }
                }

                TessellateShape(info);
                {
                    std::lock_guard<std::mutex> lock(_d->mtxShapes);
                    _d->shapes[HashCode(info.shape)] = info.shape;
                }
                { // 仅当处理完成后，才从队列中移除
                    std::lock_guard<std::mutex> lock(_d->mtxProcShapes);
                    _d->procShapes.pop_front();
                }
            }
        });
}

void ShapeTessellater::TessellateShape(const TessellateShapeInfo& info) const
{
    //Handle(AIS_Shape) ais = new AIS_Shape(info.shape);
    //info.context->Display(ais, false);

    info.context->RemoveAll(false);
    Handle(AIS_Shape) ais = new AIS_Shape(info.shape);
    info.context->Display(ais, false);

    // retrieve meshing tool from Factory
    //Handle(BRepMesh_DiscretRoot) mesher = BRepMesh_DiscretFactory::Get().Discret(shape,
    //    deflection, angle_deflection);
    //
    //if (!mesher.IsNull())
    //{
    //    mesher->Perform();
    //}
}
