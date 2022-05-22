#include "3d_preview_widget.h"

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

TdPreviewWidget::TdPreviewWidget(/*const QString& filename, */QWidget* parent)
    : QWidget(parent), _reader(nullptr)
{
    this->setMinimumSize(400, 300);
    //this->setMaximumSize(400, 300);

    InitViewer();
    InitContext();
    InitView();
    
    _timer = new QTimer(this);
    _timer->setInterval(100);
    connect(_timer, &QTimer::timeout, this, &TdPreviewWidget::TryDisplay);

    //UpdateFilename(filename);
}

TdPreviewWidget::~TdPreviewWidget()
{
    qDebug() << "Destructor!";

    if (_reader)
    {
        delete _reader;
    }
}

// �ļ����º󣬶�ʱ����Ƿ���ɼ���
void TdPreviewWidget::UpdateFilename(const QString& filename)
{
    _filename = filename;

    _timer->start(); // ׼������ģ��

    //_ps = PS_ChangeFileDone;
    
    //_thread = QThread::create([&]
    //    {
    //        LoadFile();
    //
    //        TransferShape();
    //    });
    //_thread->start();
    //
    //connect(_thread, &QThread::finished, this, &TdPreviewWidget::DisplayShape);

    //OpenFile(filename);
    //_label->setText(filename);
}

void TdPreviewWidget::LoadFile()
{
    // TODO: �������ļ�
    if (_reader)
    {
        delete _reader;
    }
    _reader = new STEPControl_Reader();

    TCollection_AsciiString  aFilePath = _filename.toUtf8().data();
    IFSelect_ReturnStatus status = _reader->ReadFile(aFilePath.ToCString());
    if (IFSelect_RetDone != status)
    {
        _ps = PS_Error;
        return;
    }

    bool failsonly = false;
    _reader->PrintCheckLoad(failsonly, IFSelect_ItemsByEntity);

    _ps = PS_LoadFileDone;
}

void TdPreviewWidget::TransferShape()
{
    if (PS_ChangeFileDone == _ps)
    {
        LoadFile();
    }

    if (PS_Error == _ps)
    {
        return;
    }
    
    // TODO: ��ת��ģ��
    int nbr = _reader->NbRootsForTransfer();
    bool failsonly = false;
    _reader->PrintCheckTransfer(failsonly, IFSelect_ItemsByEntity);
    for (Standard_Integer n = 1; n <= nbr; n++)
    {
        _reader->TransferRoot(n);
    }

    _ps = PS_TransferShapeDone;
}

void TdPreviewWidget::DisplayShape()
{
    if (PS_Error == _ps)
    {
        return;
    }

    if (_ps < PS_TransferShapeDone)
    {
        TransferShape();
    }

    if (PS_Error == _ps)
    {
        return;
    }

    qDebug() << "Display Shape!";
    
    //QApplication::processEvents();

    // ��ʾģ��
    int nbs = _reader->NbShapes();
    TopoDS_Builder builder;
    TopoDS_Compound comp;
    builder.MakeCompound(comp);
    if (nbs > 0)
    {
        for (int i = 1; i <= nbs; i++)
        {
            TopoDS_Shape shape = _reader->Shape(i);
            builder.Add(comp, shape);
        }
    }

    //QApplication::processEvents();

    if (!comp.IsNull())
    {
        DisplayOnlyShape(comp);
    }

    _ps = PS_DisplayShapeDone;
}

void TdPreviewWidget::paintEvent(QPaintEvent* e)
{
    _view->Redraw();
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

    _context->SetAutoActivateSelection(Standard_True);
    _context->SetAutomaticHilight(Standard_True);

    _context->SetDeviationCoefficient(0.001 * 0.01);

    // ��ʾ����ϵ
    //Handle(Geom_Axis2Placement) placement = new Geom_Axis2Placement(gp::Origin(), gp::DZ(), gp::DX());
    //Handle(AIS_Trihedron) trihedron = new AIS_Trihedron(placement);
    //trihedron->SetXAxisColor(Quantity_NOC_RED);
    //trihedron->SetYAxisColor(Quantity_NOC_GREEN);
    //_context->Display(trihedron, AIS_DisplayMode::AIS_WireFrame, -1, true);

    // ��ʾģʽ
    _context->SetDisplayMode(0, false); // WireFrame
    //_context->SetDisplayMode(1, false); // WireFrame
    //_context->SetDisplayMode(2, false); // box
    //_context->SetDisplayMode(3, false); // Wireframe with hidden line
    //_context->SetDisplayMode(5, false); // Wireframe with hidden line

    // ��ʾ����������
    _context->SetIsoNumber(5);

    //Handle(Prs3d_LineAspect) hidden_line_aspect = new Prs3d_LineAspect(Quantity_NOC_GRAY, Aspect_TOL_DASH, 1);
    //_context->SetHiddenLineAspect(hidden_line_aspect);
    //_context->DisableDrawHiddenLine();
}

void TdPreviewWidget::InitView()
{
#if 1
    View* view = new View(_context, this);
    QHBoxLayout* layout = new QHBoxLayout(this);
    layout->setMargin(3);
    layout->addWidget(view);
    _view = view->GetView();

#else
    // ��֪Ϊ�Σ���ʾ��˸
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

    // ����ϵ
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
    _context->RemoveAll(false);
    _context->Display(new AIS_Shape(shape), false);

    _view->SetComputedMode(false); // HLR OFF
    
    _view->FitAll(0.1);

    _view->SetComputedMode(true); // HLR ON
    _context->UpdateCurrentViewer();
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

void TdPreviewWidget::TryDisplay()
{
    qDebug() << "Try display shape " << _filename;

    TopoDS_Shape shape;
    if (StepReader::Instance().GetShape(_filename, false, shape))
    {
        DisplayOnlyShape(shape);

        _timer->stop();
    }
    else
    {
        _timer->start();
    }
}
