// Mechanicoder
// 2022/06/04

#pragma once

#include "View.h"

#include <Standard_Handle.hxx>
#include <TopoDS_Shape.hxx>

class OpenGl_GraphicDriver;
class V3d_Viewer;
class AIS_InteractiveContext;
class V3d_View;
class TopTools_HSequenceOfShape;
class STEPControl_Reader;

class QThread;
class QMenu;

// 显示预览控件
class TdPreviewWidget : public View
{
    Q_OBJECT
public:
    TdPreviewWidget(QWidget* parent = nullptr, QMenu* menu = nullptr);
    ~TdPreviewWidget();

    void ResetShape(const TopoDS_Shape& shape, const QString& tooltip = "");

signals:
    // 模型显示完成
    void finished();

    // 触发动作
    void ActionTriggered(const QAction* action, const QWidget* by_who);
private:

    void OpenFile(const QString& filename);

    Handle(TopTools_HSequenceOfShape) ImportSTEP(const QString& filename);

    void DisplayOnlyShape(const TopoDS_Shape& shape);
    void DisplayOnlyShapes(const Handle(TopTools_HSequenceOfShape)& shapes);

    void DisplayTopo(const TopoDS_Shape& shape);
    void DisplayWithVTK(const TopoDS_Shape& shape);

private slots:
    void RequestContexMenu(const QPoint& pos);

private:
    //QWidget* _viewWidget;
    QMenu* _menu;

    //Handle(OpenGl_GraphicDriver) _graphDriver;
    //Handle(V3d_Viewer) _viewer;
    //Handle(AIS_InteractiveContext) _context;
    //Handle(V3d_View) _view;
};
