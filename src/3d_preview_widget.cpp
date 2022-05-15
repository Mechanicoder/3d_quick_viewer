#include "3d_preview_widget.h"
#include "OcctWindow.h"

#include <QLabel>
#include <QHBoxLayout>
#include <QFileInfo>
#include <QDebug>

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

TdPreviewWidget::TdPreviewWidget(const QString& filename, QWidget* parent) : QWidget(parent)
{
    this->setStyleSheet("background-color:white");

    //_label = new QLabel(this);
    //_label->setText(filename);

    this->setMinimumSize(400, 300);
    this->setMaximumSize(400, 300);

    setAttribute(Qt::WA_PaintOnScreen);
    setAttribute(Qt::WA_NoSystemBackground);
    setMouseTracking(true);
    setFocusPolicy(Qt::StrongFocus);

    InitViewer();
    InitContext();
    InitView();

    OpenFile(filename);
}

TdPreviewWidget::~TdPreviewWidget()
{
    qDebug() << "Destructor!";
}

void TdPreviewWidget::UpdateFilename(const QString& filename)
{
    OpenFile(filename);

    //_label->setText(filename);
}

void TdPreviewWidget::paintEvent(QPaintEvent* e)
{
    if (_view && !_view.IsNull())
    {
        _view->Redraw();
    }
    //QWidget::paintEvent(e);
}

void TdPreviewWidget::resizeEvent(QResizeEvent*)
{
    if (_view && !_view.IsNull())
    {
        _view->MustBeResized();
    }
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
    _context->SetDisplayMode(AIS_Shaded, false);

    _context->SetDeviationCoefficient(0.001 * 0.01);

    // ÏÔÊ¾×ø±êÏµ
    Handle(Geom_Axis2Placement) placement = new Geom_Axis2Placement(gp::Origin(), gp::DZ(), gp::DX());
    Handle(AIS_Trihedron) trihedron = new AIS_Trihedron(placement);
    trihedron->SetXAxisColor(Quantity_NOC_RED);
    trihedron->SetYAxisColor(Quantity_NOC_GREEN);
    _context->Display(trihedron, AIS_DisplayMode::AIS_WireFrame, -1, true);
}

void TdPreviewWidget::InitView()
{
    if (_view.IsNull())
        _view = _context->CurrentViewer()->CreateView();

    Handle(OcctWindow) hWnd = new OcctWindow(this);

    //QHBoxLayout* layout = new QHBoxLayout(this);
    //layout->addWidget(_label);

    _view->SetWindow(hWnd);
    if (!hWnd->IsMapped())
    {
        hWnd->Map();
    }
    _view->SetBackgroundColor(Quantity_NOC_BLACK);
    _view->MustBeResized();

   // _view->ChangeRenderingParams().Method = Graphic3d_RM_RAYTRACING;
}

void TdPreviewWidget::OpenFile(const QString& filename)
{
    QFileInfo info(filename);
    if (!info.exists() || !info.isFile())
    {
        qDebug() << "Not a file!";
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

    DisplayShapes(shapes);
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

void TdPreviewWidget::DisplayShapes(const Handle(TopTools_HSequenceOfShape)& shapes)
{
    for (int i = 1; i <= shapes->Length(); i++)
    {
        _context->Display(new AIS_Shape(shapes->Value(i)), false);
    }
    _view->FitAll();
    _context->UpdateCurrentViewer();
}
