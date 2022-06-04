#include "shape_tessellater.h"
#include "block_queue.h"

#include <mutex>
#include <condition_variable>
#include <list>
#include <unordered_map>

#include <TopoDS_Shape.hxx>
#include <BRepMesh_DiscretFactory.hxx>
#include <AIS_Shape.hxx>
#include <StdPrs_ToolTriangulatedShape.hxx>

#include <QDebug>

int HashCode(const TopoDS_Shape& shape)
{
    const int upper_bnd = std::numeric_limits<int>::max();
    return shape.HashCode(upper_bnd);
}

struct TessellateShapeInfo
{
    TopoDS_Shape shape;
};

struct TessellaterData
{
    std::mutex mtxTessellater;
    std::condition_variable cvTessellater;
    std::thread threadTessellater;

    // 待处理模型: 按序处理
    BlockQueue<TessellateShapeInfo> procShapes;

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
    _d->procShapes.Clear();

    {
        std::lock_guard<std::mutex> lock(_d->mtxShapes);
        _d->shapes.clear();
    }
}

void ShapeTessellater::Do(TopoDS_Shape& shape)
{
    TessellateShapeInfo info;
    info.shape = shape;
    _d->procShapes.Push(info);

    _d->cvTessellater.notify_one();
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

#if 0
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
#endif
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
                {
                    std::unique_lock<std::mutex> lock(_d->mtxTessellater);
                    _d->cvTessellater.wait(lock, [&] {return !_d->procShapes.Empty(); });
                }

                TessellateShapeInfo info;
                if (!_d->procShapes.NotEmptyThenPop(info))
                {
                    continue;
                }
                
                TessellateShape(info);

                {
                    std::lock_guard<std::mutex> lock(_d->mtxShapes);
                    _d->shapes[HashCode(info.shape)] = info.shape;
                }
            }
        });
}

// 按网格显示时
void ShapeTessellater::TessellateShape(const TessellateShapeInfo& info) const
{
    Handle(Prs3d_Drawer) drawer = new Prs3d_Drawer();
    drawer->SetMaximalChordialDeviation(0.1);
    StdPrs_ToolTriangulatedShape::Tessellate(info.shape, drawer);
}
