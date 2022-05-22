#pragma once

#include <QWidget>

#include <Standard_Handle.hxx>

class QLabel;

class OpenGl_GraphicDriver;
class V3d_Viewer;
class AIS_InteractiveContext;
class V3d_View;
class TopTools_HSequenceOfShape;
class TopoDS_Shape;
class STEPControl_Reader;
class QThread;

// 显示预览控件
class TdPreviewWidget : public QWidget
{
    Q_OBJECT
public:
    TdPreviewWidget(/*const QString& filename, */QWidget* parent = nullptr);
    ~TdPreviewWidget();

    // 仅更新文件名
    void UpdateFilename(const QString& filename);

    // 根据文件名仅读取文件
    void LoadFile();

    // 将文件内容转化为模型
    void TransferShape();

    // 显示转换后的模型
    void DisplayShape();

protected:
    virtual void paintEvent(QPaintEvent*) override;
    virtual void resizeEvent(QResizeEvent*) override;

private:
    void InitViewer();

    void InitContext();

    void InitView();

    void OpenFile(const QString& filename);

    Handle(TopTools_HSequenceOfShape) ImportSTEP(const QString& filename);


    void DisplayOnlyShape(const TopoDS_Shape& shape);
    void DisplayOnlyShapes(const Handle(TopTools_HSequenceOfShape)& shapes);

private slots:
    void TryDisplay();

private:
    QLabel* _label;

    Handle(OpenGl_GraphicDriver) _graphDriver;
    Handle(V3d_Viewer) _viewer;
    Handle(AIS_InteractiveContext) _context;
    Handle(V3d_View) _view;

private:
    // 分步骤执行
    enum ProcessingStage
    {
        PS_ChangeFileDone,
        PS_LoadFileDone,
        PS_TransferShapeDone,
        PS_DisplayShapeDone,
        PS_Error,
    };

    QThread* _thread;

    QString _filename;
    ProcessingStage _ps;
    STEPControl_Reader* _reader;

    QTimer* _timer;
};
