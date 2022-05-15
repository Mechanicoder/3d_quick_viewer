#ifndef VIEW_H
#define VIEW_H

#include <Standard_WarningsDisable.hxx>
#include <QWidget>
#include <QAction>
#include <QList>
#include <Standard_WarningsRestore.hxx>

#include <AIS_InteractiveContext.hxx>
#include <V3d_View.hxx>

#include <unordered_map>

class TopoDS_Shape;
class QRubberBand;

enum SelectionMode
{
    SM_SHAPE = 0,
    SM_VERTEX = 1,
    SM_EDGE,
    SM_WIRE,
    SM_FACE,
    SM_SHELL,
    SM_SOLID,
    SM_COMPSOLID,
    SM_COMPOUND,
    SM_SIZE,
};

//class COMMONSAMPLE_EXPORT View: public QWidget
class View: public QWidget
{
    Q_OBJECT
protected:
    enum CurrentAction3d { CurAction3d_Nothing, CurAction3d_DynamicZooming,
                           CurAction3d_WindowZooming, CurAction3d_DynamicPanning,
                           CurAction3d_GlobalPanning, CurAction3d_DynamicRotation };

public:
    enum ViewAction { ViewAxoId, ViewFitAllId, ViewTopId, ViewFitAreaId, ViewZoomId, ViewPanId, ViewGlobalPanId,
                      ViewFrontId, ViewBackId, ViewBottomId, ViewLeftId, ViewRightId,
                      ViewRotationId, ViewResetId, ViewHlrOffId, ViewHlrOnId };
    enum RaytraceAction { ToolRaytracingId, ToolShadowsId, ToolReflectionsId, ToolAntialiasingId };

    View( Handle(AIS_InteractiveContext) theContext, QWidget* parent );

    ~View();

    virtual void                  init();
    bool                          dump( Standard_CString theFile );
    QList<QAction*>*              getViewActions();
    QList<QAction*>*              getRaytraceActions();
    void                          noActiveActions();
    bool                          isShadingMode();

    void                          EnableRaytracing();
    void                          DisableRaytracing();

    void                          SetRaytracedShadows (bool theState);
    void                          SetRaytracedReflections (bool theState);
    void                          SetRaytracedAntialiasing (bool theState);

    bool                          IsRaytracingMode() const { return myIsRaytracing; }
    bool                          IsShadowsEnabled() const { return myIsShadowsEnabled; }
    bool                          IsReflectionsEnabled() const { return myIsReflectionsEnabled; }
    bool                          IsAntialiasingEnabled() const { return myIsAntialiasingEnabled; }

    static QString                GetMessages( int type,TopAbs_ShapeEnum aSubShapeType,
                                               TopAbs_ShapeEnum aShapeType );
    static QString                GetShapeType( TopAbs_ShapeEnum aShapeType );

    Standard_EXPORT static void   OnButtonuseraction( int ExerciceSTEP,
                                                      Handle(AIS_InteractiveContext)& );
    Standard_EXPORT static void   DoSelection( int Id,
                                               Handle(AIS_InteractiveContext)& );
    Standard_EXPORT static void   OnSetSelectionMode( Handle(AIS_InteractiveContext)&,
                                                      Standard_Integer&,
                                                      TopAbs_ShapeEnum& SelectionMode,
                                                      Standard_Boolean& );
    virtual QPaintEngine*         paintEngine() const;

    void DrawShape(const TopoDS_Shape &shape) const;

    Handle(V3d_View)& GetView();
    Handle(AIS_InteractiveContext)& GetContext();

signals:
    void                          LeftButtonClicked(const QPointF &pos);

public slots:
// zhangzw
	void						bndBox();
// zhangzw-end
    void                          fitAll();
    void                          fitArea();
    void                          zoom();
    void                          pan();
    void                          globalPan();
    void                          front();
    void                          back();
    void                          top();
    void                          bottom();
    void                          left();
    void                          right();
    void                          axo();
    void                          rotation();
    void                          reset();
    void                          hlrOn();
    void                          hlrOff();
    void                          updateToggled( bool );
    void                          onBackground();
    void                          onEnvironmentMap();
    void                          onRaytraceAction();

private slots:
    void OnSelectionModeChanged();

protected:
    virtual void                  paintEvent( QPaintEvent* );
    virtual void                  resizeEvent( QResizeEvent* );
    virtual void                  mousePressEvent( QMouseEvent* );
    virtual void                  mouseReleaseEvent(QMouseEvent* );
    virtual void                  mouseMoveEvent( QMouseEvent* );
    virtual void wheelEvent(QWheelEvent *event) override;

    virtual void                  addItemInPopup( QMenu* );

    void                                  activateCursor( const CurrentAction3d );
    void                                  Popup( const int x, const int y );
    CurrentAction3d                       getCurrentMode();

    virtual void                          onLButtonDown( const int nFlags, const QPoint point );
    virtual void                          onMButtonDown( const int nFlags, const QPoint point );
    virtual void                          onRButtonDown( const int nFlags, const QPoint point );
    virtual void                          onLButtonUp( Qt::MouseButtons nFlags, const QPoint point );
    virtual void                          onMButtonUp( Qt::MouseButtons nFlags, const QPoint point );
    virtual void                          onRButtonUp( Qt::MouseButtons nFlags, const QPoint point );
    virtual void                          onMouseMove( Qt::MouseButtons nFlags, const QPoint point );

private:
    void                          initCursors();
    void                          initViewActions();
    void                          initRaytraceActions();
    void                          DragEvent( const int x, const int y, const int TheState );
    void                          InputEvent( const int x, const int y );
    void                          MoveEvent( const int x, const int y );
    void                          MultiMoveEvent( const int x, const int y );
    void                          MultiDragEvent( const int x, const int y, const int TheState );
    void                          MultiInputEvent( const int x, const int y );
    void                          DrawRectangle( const int MinX, const int MinY,
                                                 const int MaxX, const int MaxY, const bool Draw );
    void InitSelectionsModesActions();

    void SetSelectionMode(std::vector<SelectionMode> modes);

private:
    bool                            myIsRaytracing;
    bool                            myIsShadowsEnabled;
    bool                            myIsReflectionsEnabled;
    bool                            myIsAntialiasingEnabled;

    bool                            myDrawRect;           // set when a rect is used for selection or magnify 
    Handle(V3d_View)                myView;
    Handle(AIS_InteractiveContext)  myContext;
    CurrentAction3d                 myCurrentMode;
    Standard_Integer                myXmin;
    Standard_Integer                myYmin;
    Standard_Integer                myXmax;
    Standard_Integer                myYmax;
    Standard_Real                   myCurZoom;
    Standard_Boolean                myHlrModeIsOn;
    QList<QAction*>*                myViewActions;
    QList<QAction*>*                myRaytraceActions;
    QMenu *myBackMenu;
    QMenu *myModeMunu;
    QRubberBand*                    myRectBand; //!< selection rectangle rubber band
    
    std::unordered_map<QAction*, SelectionMode> _selectionModeActions;
};

#endif


