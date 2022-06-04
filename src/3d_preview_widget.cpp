// Mechanicoder
// 2022/06/04

#include "3d_preview_widget.h"
#include "shape_tessellater.h"

#include "occ/OcctWindow.h"
#include "occ/View.h"

#include "reader/step_reader.h"

#include <QLabel>
#include <QHBoxLayout>
#include <QFileInfo>
#include <QDebug>
#include <QThread>
#include <QApplication>
#include <QTimer>

#include <Aspect_DisplayConnection.hxx>
#include <OpenGl_GraphicDriver.hxx>
#include <V3d_Viewer.hxx>
#include <AIS_InteractiveContext.hxx>
#include <V3d_View.hxx>
#include <Geom_Axis2Placement.hxx>
#include <AIS_Trihedron.hxx>
#include <TopTools_HSequenceOfShape.hxx>
#include <STEPControl_Reader.hxx>
#include <AIS_Shape.hxx>
#include <TopoDS_Compound.hxx>
#include <TopoDS_Builder.hxx>
#include <TopExp_Explorer.hxx>

#define TRY_VTKx

#ifdef TRY_VTK
#include <IVtkTools_ShapeDataSource.hxx>
#include <vtkRenderWindow.h>
#include <vtkRender.h>
#include <vtkInteractorStyleTrackabllCamera.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkPolyDataMapper.h>
#endif

TdPreviewWidget::TdPreviewWidget(/*const QString& filename, */QWidget* parent)
    : QWidget(parent), _reader(nullptr)
{
    this->setMinimumSize(400, 300);
    
    InitViewer();
    InitContext();
    InitView();
    
    //_timer = new QTimer(this);
    //_timer->setInterval(100); // 查询频率：200ms 查询一次模型是否完成加载

    this->hide(); // 显示完成后再显示
}

TdPreviewWidget::~TdPreviewWidget()
{
    qDebug() << "Destructor!";

    if (_reader)
    {
        delete _reader;
    }
}

void TdPreviewWidget::ResetShape(const TopoDS_Shape& shape, const QString& tooltip)
{
    DisplayOnlyShape(shape);

    this->setToolTip(tooltip);
}

// 文件更新后，定时检查是否完成加载
// TODO: 加载文件处理，也可由 TdPreviewWidget 移动到此处管理
//void TdPreviewWidget::UpdateFilename(const QString& filename, int index, int number)
//{
//    _filename = filename;
//
//    // 准备加载模型: 定时查询模型是否完成加载，如完成，则加载模型
//    connect(_timer, &QTimer::timeout, this, &TdPreviewWidget::OnGotShape);
//
//    _timer->start();
//
//    //_ps = PS_ChangeFileDone;
//    
//    //_thread = QThread::create([&]
//    //    {
//    //        LoadFile();
//    //
//    //        TransferShape();
//    //    });
//    //_thread->start();
//    //
//    //connect(_thread, &QThread::finished, this, &TdPreviewWidget::DisplayShape);
//
//    //OpenFile(filename);
//    //_label->setText(QString("%1 / %2").arg(index).arg(number));
//    this->setToolTip(QString("%1 / %2").arg(index).arg(number));
//
//    this->setDisabled(true); // 不响应任何事件
//}

//void TdPreviewWidget::LoadFile()
//{
//    // TODO: 仅加载文件
//    if (_reader)
//    {
//        delete _reader;
//    }
//    _reader = new STEPControl_Reader();
//
//    TCollection_AsciiString  aFilePath = _filename.toUtf8().data();
//    IFSelect_ReturnStatus status = _reader->ReadFile(aFilePath.ToCString());
//    if (IFSelect_RetDone != status)
//    {
//        _ps = PS_Error;
//        return;
//    }
//
//    bool failsonly = false;
//    _reader->PrintCheckLoad(failsonly, IFSelect_ItemsByEntity);
//
//    _ps = PS_LoadFileDone;
//}

//void TdPreviewWidget::TransferShape()
//{
//    if (PS_ChangeFileDone == _ps)
//    {
//        LoadFile();
//    }
//
//    if (PS_Error == _ps)
//    {
//        return;
//    }
//    
//    // TODO: 仅转换模型
//    int nbr = _reader->NbRootsForTransfer();
//    bool failsonly = false;
//    _reader->PrintCheckTransfer(failsonly, IFSelect_ItemsByEntity);
//    for (Standard_Integer n = 1; n <= nbr; n++)
//    {
//        _reader->TransferRoot(n);
//    }
//
//    _ps = PS_TransferShapeDone;
//}

//void TdPreviewWidget::DisplayShape()
//{
//    if (PS_Error == _ps)
//    {
//        return;
//    }
//
//    if (_ps < PS_TransferShapeDone)
//    {
//        TransferShape();
//    }
//
//    if (PS_Error == _ps)
//    {
//        return;
//    }
//
//    qDebug() << "Display Shape!";
//    
//    //QApplication::processEvents();
//
//    // 显示模型
//    int nbs = _reader->NbShapes();
//    TopoDS_Builder builder;
//    TopoDS_Compound comp;
//    builder.MakeCompound(comp);
//    if (nbs > 0)
//    {
//        for (int i = 1; i <= nbs; i++)
//        {
//            TopoDS_Shape shape = _reader->Shape(i);
//            builder.Add(comp, shape);
//        }
//    }
//
//    //QApplication::processEvents();
//
//    if (!comp.IsNull())
//    {
//        DisplayOnlyShape(comp);
//    }
//
//    _ps = PS_DisplayShapeDone;
//}

void TdPreviewWidget::paintEvent(QPaintEvent* e)
{
    _view->Redraw();

    //_label->update();
}

void TdPreviewWidget::resizeEvent(QResizeEvent*)
{
    _view->MustBeResized();
}

void TdPreviewWidget::InitViewer()
{
    if (_graphDriver.IsNull())
    {
        Handle(Aspect_DisplayConnection) aDisplayConnection;
#if !defined(_WIN32) && !defined(__WIN32__) && (!defined(__APPLE__) || defined(MACOSX_USE_GLX))
        aDisplayConnection = new Aspect_DisplayConnection(OSD_Environment("DISPLAY").Value());
#endif
        _graphDriver = new OpenGl_GraphicDriver(aDisplayConnection);
    }

    _viewer = new V3d_Viewer(_graphDriver);

    const double default_view_size = 1000.0;
    const V3d_TypeOfOrientation default_orient = V3d_XposYnegZpos;

    _viewer->SetDefaultViewSize(default_view_size);
    _viewer->SetDefaultViewProj(default_orient);
    _viewer->SetComputedMode(true);
    _viewer->SetDefaultComputedMode(true);

    _viewer->SetDefaultLights();
    _viewer->SetLightOn();
}

void TdPreviewWidget::InitContext()
{
    _context = new AIS_InteractiveContext(_viewer);

    Handle(Prs3d_Drawer) default_drawer = new Prs3d_Drawer();
    default_drawer->SetMaximalChordialDeviation(0.1);
    _context->SetDefaultDrawer(default_drawer);
    //_context->SetDeviationCoefficient(0.001);

    _context->SetAutoActivateSelection(Standard_True);
    _context->SetAutomaticHilight(Standard_True);

    //_context->SetDeviationCoefficient(0.001 * 0.01);

    // 显示坐标系
    //Handle(Geom_Axis2Placement) placement = new Geom_Axis2Placement(gp::Origin(), gp::DZ(), gp::DX());
    //Handle(AIS_Trihedron) trihedron = new AIS_Trihedron(placement);
    //trihedron->SetXAxisColor(Quantity_NOC_RED);
    //trihedron->SetYAxisColor(Quantity_NOC_GREEN);
    //_context->Display(trihedron, AIS_DisplayMode::AIS_WireFrame, -1, true);

    // 显示模式
    //_context->SetDisplayMode(0, false); // WireFrame
    _context->SetDisplayMode(1, false); // Shaded
    //_context->SetDisplayMode(2, false); // box
    //_context->SetDisplayMode(3, false); // Wireframe with hidden line
    //_context->SetDisplayMode(5, false); // Wireframe with hidden line

    // 显示参数线数量
    //_context->SetIsoNumber(5);

    //Handle(Prs3d_LineAspect) hidden_line_aspect = new Prs3d_LineAspect(Quantity_NOC_GRAY, Aspect_TOL_DASH, 1);
    //_context->SetHiddenLineAspect(hidden_line_aspect);
    //_context->DisableDrawHiddenLine();
}

void TdPreviewWidget::InitView()
{
#if 1
    // 使用 OCC 代码中的 view
    View* view = new View(_context, this);
    QHBoxLayout* layout = new QHBoxLayout(this);
    layout->setMargin(3);
    layout->addWidget(view);
    _view = view->GetView();

#else
    // 不知为何，显示闪烁
    if (_view.IsNull())
        _view = _context->CurrentViewer()->CreateView();

    Handle(OcctWindow) hWnd = new OcctWindow(this);

    _view->SetWindow(hWnd);
    if (!hWnd->IsMapped())
    {
        hWnd->Map();
    }
    _view->SetBackgroundColor(Quantity_NOC_BLACK);
    _view->MustBeResized();

    // 坐标系
    _view->TriedronDisplay(Aspect_TOTP_LEFT_LOWER, Quantity_NOC_RED, 0.1, V3d_ZBUFFER);
    //_view->SetBgGradientColors(Quantity_NOC_WHITE, Quantity_NOC_DARKSLATEGRAY, Aspect_GFM_VER);
   // _view->ChangeRenderingParams().Method = Graphic3d_RM_RAYTRACING;
#endif
}

void TdPreviewWidget::OpenFile(const QString& filename)
{
    QFileInfo info(filename);
    if (!info.exists() || !info.isFile())
    {
        qDebug() << "Not a file of [" << filename << "]";
        return;
    }

    QString suffix = info.suffix().toLower();
    if ("step" != suffix && "stp" != suffix)
    {
        qDebug() << "Not a STEP file!";
        return;
    }

    Handle(TopTools_HSequenceOfShape) shapes = ImportSTEP(filename);
    if (!shapes || shapes.IsNull() || shapes->IsEmpty())
    {
        qDebug() << "Empty file!";
        return;
    }

    DisplayOnlyShapes(shapes);
}

Handle(TopTools_HSequenceOfShape) TdPreviewWidget::ImportSTEP(const QString& filename)
{
    Handle(TopTools_HSequenceOfShape) aSequence = new TopTools_HSequenceOfShape;
    TCollection_AsciiString  aFilePath = filename.toUtf8().data();
    STEPControl_Reader aReader;
    IFSelect_ReturnStatus status = aReader.ReadFile(aFilePath.ToCString());
    if (status != IFSelect_RetDone)
    {
        return aSequence;
    }

    //Interface_TraceFile::SetDefault();
    bool failsonly = false;
    aReader.PrintCheckLoad(failsonly, IFSelect_ItemsByEntity);

    int nbr = aReader.NbRootsForTransfer();
    aReader.PrintCheckTransfer(failsonly, IFSelect_ItemsByEntity);
    for (Standard_Integer n = 1; n <= nbr; n++)
    {
        aReader.TransferRoot(n);
    }

    int nbs = aReader.NbShapes();
    if (nbs > 0)
    {
        for (int i = 1; i <= nbs; i++)
        {
            TopoDS_Shape shape = aReader.Shape(i);
            aSequence->Append(shape);
        }
    }

    return aSequence;
}

void TdPreviewWidget::DisplayOnlyShape(const TopoDS_Shape& shape)
{
    DisplayTopo(shape); // 以拓扑模型显示

    //DisplayTriangulation(shape); // 显示模型的网格

    emit finished();

    DisplayWithVTK(shape);
}

void TdPreviewWidget::DisplayOnlyShapes(const Handle(TopTools_HSequenceOfShape)& shapes)
{
    _context->RemoveAll(false);
    for (int i = 1; i <= shapes->Length(); i++)
    {
        _context->Display(new AIS_Shape(shapes->Value(i)), false);
    }

    _view->SetComputedMode(false); // HLR OFF
    _view->FitAll(0.1);
    _view->SetComputedMode(true); // HLR ON

    _context->UpdateCurrentViewer();
}

void TdPreviewWidget::DisplayTopo(const TopoDS_Shape& shape)
{
    _context->RemoveAll(false);
    Handle(AIS_Shape) ais = new AIS_Shape(shape);
    //Handle(AIS_SimpleShape) ais = new AIS_SimpleShape(shape);
    _context->Display(ais, false);

    _view->SetComputedMode(false); // HLR OFF

    _view->FitAll(0.1);

    //_view->SetComputedMode(true); // HLR ON
    _context->UpdateCurrentViewer();
}

void TdPreviewWidget::DisplayWithVTK(const TopoDS_Shape& shape)
{
#ifdef TRY_VTK
    vtkNew<vtkRenderWindow> rewin;
    vtkNew<vtkRender> ren;
    rewin->addRenderer(ren);

    vtkNew<vtkInteractorStyleTrackabllCamera> istyle;
    vtkNew<vtkRenderWindowInteractor> iren;

    iren->SetRenderWindow(renwin);
    iren->SetInteractorStyle(istyle);

    vtkNew<IVtkTools_ShapeDataSource> occSource;
    occSource->SetShape(new IVtkOCC_Shape(shape));

    vtkNew<vtkPolyDataMapper> mapper;
    mapper->SetInputConnection(occSource->GetOutputPort()); 

    rewin->Render();
    iren->Start();
#endif
}

//void TdPreviewWidget::OnGotShape()
//{
//    qDebug() << "File-->Shape done  " << _filename;
//
//    if (StepReader::Instance().GetShape(_filename, false, _shape))
//    {
//        
//        double deflection = _context->DefaultDrawer()->MaximalChordialDeviation();
//        double angle_deflection = _context->DefaultDrawer()->DeviationAngle();
//
//        ShapeTessellater::Instance().Do(_context, _shape);
//
//        disconnect(_timer, &QTimer::timeout, this, &TdPreviewWidget::OnGotShape);
//        connect(_timer, &QTimer::timeout, this, &TdPreviewWidget::OnTesselateDone);
//    }
//
//    //_timer->start();
//}
//
//void TdPreviewWidget::OnTesselateDone()
//{
//    if (ShapeTessellater::Instance().Done(_shape, false))
//    {
//        DisplayOnlyShape(_shape);
//
//        _timer->stop();
//    }
//    else
//    {
//        //_timer->start();
//    }
//}
