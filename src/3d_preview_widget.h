#pragma once

#include <QWidget>

#include <Standard_Handle.hxx>
#include <TopoDS_Shape.hxx>

class QLabel;

class OpenGl_GraphicDriver;
class V3d_Viewer;
class AIS_InteractiveContext;
class V3d_View;
class TopTools_HSequenceOfShape;
class STEPControl_Reader;
class QThread;

// ��ʾԤ���ؼ�
class TdPreviewWidget : public QWidget
{
    Q_OBJECT
public:
    TdPreviewWidget(/*const QString& filename, */QWidget* parent = nullptr);
    ~TdPreviewWidget();

    // �������ļ���
    void UpdateFilename(const QString& filename, int index, int number);

    // �����ļ�������ȡ�ļ�
    void LoadFile();

    // ���ļ�����ת��Ϊģ��
    void TransferShape();

    // ��ʾת�����ģ��
    void DisplayShape();

signals:
    // ģ����ʾ���
    void finished();

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
    // ģ�ͼ�����ɺ�
    void OnGotShape();

    // ģ��Ԥ������ɢ��
    void OnTesselateDone();

private:
    QLabel* _label;

    Handle(OpenGl_GraphicDriver) _graphDriver;
    Handle(V3d_Viewer) _viewer;
    Handle(AIS_InteractiveContext) _context;
    Handle(V3d_View) _view;

private:
    // �ֲ���ִ��
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
    TopoDS_Shape _shape;
};
