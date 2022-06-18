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
#include <QMenu>

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

class ViewHelper
{
public:
    static Handle(V3d_Viewer) InitViewer()
    {
        Handle(Aspect_DisplayConnection) aDisplayConnection;
#if !defined(_WIN32) && !defined(__WIN32__) && (!defined(__APPLE__) || defined(MACOSX_USE_GLX))
        aDisplayConnection = new Aspect_DisplayConnection(OSD_Environment("DISPLAY").Value());
#endif
        Handle(OpenGl_GraphicDriver) graph_driver = new OpenGl_GraphicDriver(aDisplayConnection);

        Handle(V3d_Viewer) viewer = new V3d_Viewer(graph_driver);

        const double default_view_size = 1000.0;
        const V3d_TypeOfOrientation default_orient = V3d_XposYnegZpos;

        viewer->SetDefaultViewSize(default_view_size);
        viewer->SetDefaultViewProj(default_orient);
        viewer->SetComputedMode(true);
        viewer->SetDefaultComputedMode(true);

        viewer->SetDefaultLights();
        viewer->SetLightOn();
        return viewer;
    }

    static Handle(AIS_InteractiveContext) InitContext(Handle(V3d_Viewer) viewer)
    {
        Handle(AIS_InteractiveContext) context = new AIS_InteractiveContext(viewer);

        Handle(Prs3d_Drawer) default_drawer = new Prs3d_Drawer();
        default_drawer->SetMaximalChordialDeviation(0.1);
        context->SetDefaultDrawer(default_drawer);
        //context->SetDeviationCoefficient(0.001);

        context->SetAutoActivateSelection(Standard_True);
        context->SetAutomaticHilight(Standard_True);

        //context->SetDeviationCoefficient(0.001 * 0.01);

        // ��ʾ����ϵ
        //Handle(Geom_Axis2Placement) placement = new Geom_Axis2Placement(gp::Origin(), gp::DZ(), gp::DX());
        //Handle(AIS_Trihedron) trihedron = new AIS_Trihedron(placement);
        //trihedron->SetXAxisColor(Quantity_NOC_RED);
        //trihedron->SetYAxisColor(Quantity_NOC_GREEN);
        //context->Display(trihedron, AIS_DisplayMode::AIS_WireFrame, -1, true);

        // ��ʾģʽ
        //context->SetDisplayMode(0, false); // WireFrame
        context->SetDisplayMode(1, false); // Shaded
        //context->SetDisplayMode(2, false); // box
        //context->SetDisplayMode(3, false); // Wireframe with hidden line
        //context->SetDisplayMode(5, false); // Wireframe with hidden line

        // ��ʾ����������
        //context->SetIsoNumber(5);

        //Handle(Prs3d_LineAspect) hidden_line_aspect = new Prs3d_LineAspect(Quantity_NOC_GRAY, Aspect_TOL_DASH, 1);
        //context->SetHiddenLineAspect(hidden_line_aspect);
        //context->DisableDrawHiddenLine();

        return context;
    }
};

TdPreviewWidget::TdPreviewWidget(QWidget* parent, QMenu* menu)
    : View(ViewHelper::InitContext(ViewHelper::InitViewer()), parent), _menu(menu)
{
    this->setMinimumSize(400, 300);
    this->hide(); // ��ʾ��ɺ�����ʾ

    if (_menu)
    {
        this->setContextMenuPolicy(Qt::CustomContextMenu);

        connect(this, &TdPreviewWidget::customContextMenuRequested,
            this, &TdPreviewWidget::RequestContexMenu);
    }
}

TdPreviewWidget::~TdPreviewWidget()
{
    qDebug() << "Destructor!";
}

void TdPreviewWidget::ResetShape(const TopoDS_Shape& shape, const QString& tooltip)
{
    DisplayOnlyShape(shape);

    this->setToolTip(tooltip);
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
    DisplayTopo(shape); // ������ģ����ʾ

    emit DisplayFinished();

    DisplayWithVTK(shape);
}

void TdPreviewWidget::DisplayOnlyShapes(const Handle(TopTools_HSequenceOfShape)& shapes)
{
    Handle(AIS_InteractiveContext) context = View::getContext();
    context->RemoveAll(false);
    for (int i = 1; i <= shapes->Length(); i++)
    {
        context->Display(new AIS_Shape(shapes->Value(i)), false);
    }

    Handle(V3d_View)& view = View::getView();

    view->SetComputedMode(false); // HLR OFF
    view->FitAll(0.1);
    view->SetComputedMode(true); // HLR ON

    context->UpdateCurrentViewer();
}

void TdPreviewWidget::DisplayTopo(const TopoDS_Shape& shape)
{
    View::getContext()->RemoveAll(false);
    Handle(AIS_Shape) ais = new AIS_Shape(shape);
    //Handle(AIS_SimpleShape) ais = new AIS_SimpleShape(shape);
    View::getContext()->Display(ais, false);

    View::getView()->SetComputedMode(false); // HLR OFF

    View::getView()->FitAll(0.1);

    //_view->SetComputedMode(true); // HLR ON
    View::getContext()->UpdateCurrentViewer();
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

void TdPreviewWidget::RequestContexMenu(const QPoint& pos)
{
    if (_menu)
    {
        _menu->popup(this->mapToGlobal(pos));
        QAction* act = _menu->exec();
        if (act)
        {
            emit ActionTriggered(act, this);
        }
    }
}
