// Mechanicoder
// 2022/06/04

#include "3d_quick_viewer.h"
#include "file_system_viewer.h"
#include "3d_preview_widget.h"
#include "shape_tessellater.h"
#include "ui_3d_quick_viewer.h"

#include "reader/step_reader.h"

#include <QFileSystemModel>
#include <QFileInfo>
#include <QVector>
#include <QDebug>
#include <QScrollBar>
#include <QTime>
#include <QDateTime>
#include <QElapsedTimer>
#include <QTimer>
#include <QThread>
#include <QProgressBar>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValueRef>
#include <QJsonArray>
#include <QProcess>
#include <QMessageBox>

#define INTERVAL 100 // 查询频率：200ms 查询一次模型是否完成加载

TdQuickViewer::TdQuickViewer(QWidget* parent)
    : QMainWindow(parent), _colCnt(3), _menu(nullptr)
{
    _ui = new Ui::TdQuickViewerUi;
    _ui->setupUi(this);
    _ui->menubar->hide();

    _layout = new QGridLayout(_ui->scrollArea_preview->widget());
    _layout->setSizeConstraint(QLayout::SetMinimumSize);
    _layout->setSpacing(9);

    connect(_ui->treeView_path, &FileSystemViewer::FolderPressed, this, &TdQuickViewer::OnFolderPressed);
    connect(_ui->pushButton_applyRoot, &QPushButton::clicked, this, &TdQuickViewer::OnApplyRootDir);

    InitProgressBar();

    OnInitMenu();

#ifdef EVAL_PERFORMANCE
    _evalTimer = nullptr;
    _evalPath = QCoreApplication::applicationDirPath() + "/../0000_STEP_TEST/EVAL_TIME";
    EvalPerformanceStart();
#endif
}

TdQuickViewer::~TdQuickViewer()
{
    StepReader::Instance().Stop();
    ShapeTessellater::Instance().Stop();
}

void TdQuickViewer::OnFolderPressed(const QString& filepath)
{
    UpdateTasks(filepath);

    if (!_tasks.empty())
    {
        // 增加3D预览视图
        IncreasePreviewWidget(int(_tasks.size()));

        std::vector<QString> load_files;
        for (auto it = _tasks.begin(); it != _tasks.end(); ++it)
        {
            load_files.emplace_back(it->filename);
        }
        StepReader::Instance().Reset(load_files);
        ShapeTessellater::Instance().Reset();
    }

    ProcessTask();
}

void TdQuickViewer::PreviewFinished()
{
#ifdef EVAL_PERFORMANCE
    if (0 == (--_evalCnt))
    {
        EvalPerformanceEnd();
    }
#endif
}

void TdQuickViewer::ProcessTask()
{
    if (_tasks.empty())
    {
        return;
    }

    for (auto it = _tasks.begin(); it != _tasks.end();)
    {
        TopoDS_Shape shape;
        if (TS_Ready == it->stage)
        {
            if (StepReader::Instance().GetShape(it->filename, false, shape))
            {
                it->stage = TS_LoadFileDone;

                Handle(AIS_InteractiveContext) ctx;
                ShapeTessellater::Instance().Do(shape);
            }
        }

        if (TS_LoadFileDone == it->stage)
        {
            if (StepReader::Instance().GetShape(it->filename, false, shape)) // 重复调用
            {
                if (ShapeTessellater::Instance().Done(shape, false))
                {
                    it->stage = TS_TessellateShapeDone;

                    int id = it->id;
                    const int row = id / 3;
                    const int col = (id - row * _colCnt) % 3;

                    QLayoutItem* item = _layout->itemAtPosition(row, col);
                    if (item && item->widget())
                    {
                        this->setCursor(Qt::WaitCursor);
                        this->blockSignals(true);

                        TdPreviewWidget* widget = static_cast<TdPreviewWidget*>(item->widget());
                        widget->ResetShape(shape, it->filename); // 该步骤会卡住界面
                        widget->show();

                        this->unsetCursor();
                        this->blockSignals(false);

                        //widget->UpdateFilename(filenames[i], int(i + 1), (int)filenames.size());
                    }
                    qDebug() << "Done: " << it->id << " " << it->filename;

                    it = _tasks.erase(it);

                    break; // 一次处理一个视图，避免卡住主界面
                }
            }
        }
        else
        {
            ++it;
        }
    }

    UpdateProgressBar();
    qApp->processEvents();

    // 处理完成后，开始计时
    QTimer::singleShot(INTERVAL, this, &TdQuickViewer::ProcessTask);
}

void TdQuickViewer::IncreasePreviewWidget(int total_cnt)
{
    const int exist_count = _layout->count();
    const int col_cnt = _colCnt; // 列数量
    for (int i = 0; i < total_cnt; i++) 
    {
        const int row = i / _colCnt;
        const int col = (i - row * col_cnt) % _colCnt;

        if (i >= exist_count) // 新增不足的
        {
            TdPreviewWidget* widget = new TdPreviewWidget(this, _menu);
            connect(widget, &TdPreviewWidget::DisplayFinished, this, &TdQuickViewer::PreviewFinished);
            connect(widget, &TdPreviewWidget::ActionTriggered, this, &TdQuickViewer::OnContextCmd);

            _layout->addWidget(widget, row, col, 1, 1);
        }
        else
        { // 显示原被隐藏的
            QLayoutItem* item = _layout->itemAtPosition(row, col);
            if (item && item->widget())
            {
                item->widget()->hide();
            }
        }
    }
    for (int i = total_cnt; i < exist_count; i++) // 隐藏多余的控件
    {
        const int row = i / _colCnt;
        const int col = (i - row * col_cnt) % _colCnt;
        QLayoutItem* item = _layout->itemAtPosition(row, col);
        if (item && item->widget())
        {
            item->widget()->hide();
        }
    }

#ifdef EVAL_PERFORMANCE
    _evalCnt = total_cnt;
#endif
}

void TdQuickViewer::EvalPerformanceStart()
{
#ifdef EVAL_PERFORMANCE
    _evalTimer = new QElapsedTimer;
    _evalTimer->start();

    OnFolderPressed(_evalPath);
#endif
}

void TdQuickViewer::EvalPerformanceEnd()
{
#ifdef EVAL_PERFORMANCE
    if (!_evalTimer)
    {
        return;
    }
    QString elapsed = QTime::fromMSecsSinceStartOfDay(_evalTimer->elapsed()).toString();

    // write log
    QFile file(_evalPath + "/records.txt");
    if (!file.open(QIODevice::Append | QIODevice::Text))
    {
        qDebug() << "records.txt open failed!";
    }

    qDebug() << "Eval finished elapsed time " << elapsed << "\n";

    QTextStream out(&file);
    out << "Test at, " << QDateTime::currentDateTime().toString()
        << ", elapsed time, " << elapsed << "\n";

    delete _evalTimer;
    _evalTimer = nullptr;
#endif
}

void TdQuickViewer::UpdateTasks(const QString& folder_path)
{
    _tasks.clear();
    int id = 0;

    QFileInfo check_path(folder_path);
    if (check_path.isDir())
    {
        QDir dir(folder_path);
        QList<QFileInfo> objects = dir.entryInfoList();

        for (const QFileInfo& info : objects)
        {
            if (IsSupportedFile(info))
            {
                Task task;
                task.id = id++;
                task.filename = info.absoluteFilePath();
                task.stage = TS_Ready;
                _tasks.emplace_back(task);
                //filenames.emplace_back(info.absoluteFilePath());
            }
        }
    }
    else if (check_path.isFile())
    {
        Task task;
        task.id = id++;
        task.filename = check_path.absoluteFilePath();
        task.stage = TS_Ready;
        _tasks.emplace_back(task);
        //filenames.emplace_back(check_path.absoluteFilePath());
    }

    if (!_tasks.empty()) // 没有新任务时，不需要忙等待
    {
        _progressBar->setRange(0, int(_tasks.size()));
    }
}

bool TdQuickViewer::IsSupportedFile(const QFileInfo& info) const
{
    if (info.isFile())
    {
        QString suffix = info.suffix().toLower();
        if ("step" == suffix || "stp" == suffix)
        {
            return true;
        }
    }

    return false;
}

void TdQuickViewer::UpdateProgressBar()
{
    _progressBar->setValue(int(_tasks.size()));
}

void TdQuickViewer::InitProgressBar()
{
    // 进度条
    _progressBar = new QProgressBar(_ui->statusbar);
    _progressBar->setFormat("Remaining %v/%m");
    _progressBar->setMinimumWidth(300);
    QSizePolicy size_policy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);
    size_policy.setHorizontalStretch(0);
    size_policy.setVerticalStretch(0);
    size_policy.setHeightForWidth(_progressBar->sizePolicy().hasHeightForWidth());
    _progressBar->setSizePolicy(size_policy);
    QLayout* status_layout = _ui->statusbar->layout();
    if (!status_layout)
    {
        status_layout = new QHBoxLayout(_ui->statusbar);
    }
    status_layout->addWidget(_progressBar);
}

void TdQuickViewer::InitDefaultMenu()
{
    if (!_menu)
    {
        _menu = new QMenu(this);
    }
    _menu->clear();

    QAction* action = new QAction("Update Menu", this);
    _menu->addAction(action);
    connect(action, &QAction::triggered, this, &TdQuickViewer::OnInitMenu);
}

/*
{
  "Send To": [
    {
      "Name": "Notepad",
      "Command": "notepad"
    },
    {
      "Name": "Calculator",
      "Command": "calc"
    }
  ],
  "Copy to": [
    {
      "Name": "Clipbord",
      "Command": "copy_to_clipbord.bat"
    }
  ]
}
*/
void TdQuickViewer::OnInitMenu()
{
    InitDefaultMenu(); // 始终初始化默认菜单

    QString curr_path = QCoreApplication::applicationDirPath();
    QFile file(curr_path + "/config/context_menu.json");
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        return;
    }
    QJsonDocument json = QJsonDocument::fromJson(file.readAll());
    if (json.isEmpty() || json.isNull())
    {
        return;
    }

    const QString menu_name("Name");
    const QString menu_command("Command");

    if (json.isObject())
    {
        int cmd_cnt = 0;
        QJsonObject obj = json.object();
        for (auto it = obj.begin(); it != obj.end(); ++it)
        {
            QMenu* sub_menu = nullptr;
            QString json_menu = it.key();
            QJsonValueRef json_cmds = it.value();
            if (!json_menu.isNull() && !json_cmds.isNull() && json_cmds.isArray())
            {
                QJsonArray json_array = json_cmds.toArray();
                for (int i = 0; i < json_array.size(); i++)
                {
                    QJsonValue name = json_array.at(i)[menu_name];
                    QJsonValue command = json_array.at(i)[menu_command];
                    if (!name.isNull() && !command.isNull())
                    {
                        if (!sub_menu)
                        {
                            sub_menu = new QMenu(json_menu, this);
                        }

                        QAction* action = new QAction(name.toString(), this);
                        action->setData(command.toString());
                        action->setStatusTip(command.toString());
                        sub_menu->addAction(action);

                        ++cmd_cnt;
                    }
                }
            }
            if (sub_menu)
            {
                _menu->insertMenu(_menu->actions().front(), sub_menu);
            }
        }

        if (cmd_cnt > 0)
        {
            QMessageBox::information(this, "Message",
                "Context menu [" + file.fileName() + "] loaded!");
        }
    }
}

// 将命令存储在 action data 中
void TdQuickViewer::OnContextCmd(const QAction* action, const QWidget* by_who)
{
    if (!by_who || !action || action->data().toString().isEmpty())
    {
        return;
    }

    QString filename = by_who->toolTip();
    QProcess::startDetached(action->data().toString(), QStringList() << filename);
}

void TdQuickViewer::OnApplyRootDir()
{
    QString root_dir = _ui->lineEdit_root->text();
    if (root_dir.isEmpty())
    {
        _ui->treeView_path->UpdateRootPath("");
    }
    else // 无效设置则保持最近一次设置不变
    {
        QFileInfo check_path(root_dir);
        if (check_path.isDir() || check_path.isFile())
        {
            _ui->treeView_path->UpdateRootPath(root_dir);
        }
        else
        {
            // nothing to do
        }
    }
}
