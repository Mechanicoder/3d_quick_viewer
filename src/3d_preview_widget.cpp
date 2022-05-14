#include "3d_preview_widget.h"

#include <QLabel>
#include <QHBoxLayout>

TdPreviewWidget::TdPreviewWidget(const QString& filename, QWidget* parent) : QWidget(parent)
{
    this->setStyleSheet("background-color:white");

    _label = new QLabel(this);
    _label->setText(filename);

    QHBoxLayout* layout = new QHBoxLayout(this);
    layout->addWidget(_label);

    this->setMinimumSize(400, 300);
    this->setMaximumSize(400, 300);
}

TdPreviewWidget::~TdPreviewWidget()
{
}

void TdPreviewWidget::UpdateFilename(const QString& filename)
{
    _label->setText(filename);
}
