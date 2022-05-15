#pragma once

#include <QWidget>

#include <Standard_Handle.hxx>

class QLabel;

class OpenGl_GraphicDriver;
class V3d_Viewer;
class AIS_InteractiveContext;
class V3d_View;
class TopTools_HSequenceOfShape;

// ÏÔÊ¾Ô¤ÀÀ¿Ø¼þ
class TdPreviewWidget : public QWidget
{
    Q_OBJECT
public:
    TdPreviewWidget(const QString& filename, QWidget* parent = nullptr);
    ~TdPreviewWidget();

    void UpdateFilename(const QString& filename);

protected:
    virtual void paintEvent(QPaintEvent*) override;
    virtual void resizeEvent(QResizeEvent*) override;

private:
    void InitViewer();

    void InitContext();

    void InitView();

    void OpenFile(const QString& filename);

    Handle(TopTools_HSequenceOfShape) ImportSTEP(const QString& filename);

    void DisplayOnlyShapes(const Handle(TopTools_HSequenceOfShape)& shapes);

private:
    QLabel* _label;

    Handle(OpenGl_GraphicDriver) _graphDriver;
    Handle(V3d_Viewer) _viewer;
    Handle(AIS_InteractiveContext) _context;
    Handle(V3d_View) _view;
};
