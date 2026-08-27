#include "dialogwindow.h"
#include <QApplication>
#include <QFileInfo>
#include <QScreen>
#include <QPainter>
#include <QShortcut>
#include <QKeySequence>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QFrame>
#include <QDesktopServices>
#include <QUrl>

static const QColor BG_COLOR(0, 99, 177);       // 深蓝 #0063B1
static const QColor BTN_RUN_COLOR(240, 240, 240);
static const QColor BTN_NORUN_COLOR(44, 62, 80);

DialogWindow::DialogWindow(const QString& filePath, QWidget* parent)
    : QDialog(parent), filePath_(filePath) {
    setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
    setAttribute(Qt::WA_TranslucentBackground, false);
    setFixedSize(500, 380);

    // 居中
    if (QScreen* screen = QApplication::primaryScreen()) {
        QRect g = screen->availableGeometry();
        move((g.width() - width()) / 2, (g.height() - height()) / 2);
    }

    setupUi();

    // 快捷键
    auto* esc = new QShortcut(QKeySequence(Qt::Key_Escape), this);
    connect(esc, &QShortcut::activated, this, &DialogWindow::onReject);
}

void DialogWindow::setupUi() {
    QFileInfo fi(filePath_);
    QString fileName = fi.fileName();
    QString suffix = fi.suffix().isEmpty() ? "(无)" : "." + fi.suffix();

    // 主布局
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // 内容区域 (带内边距)
    QWidget* content = new QWidget(this);
    content->setStyleSheet("background-color: #0063B1;");
    QVBoxLayout* contentLayout = new QVBoxLayout(content);
    contentLayout->setContentsMargins(32, 32, 32, 0);
    contentLayout->setSpacing(0);

    // 标题
    titleLabel_ = new QLabel("永远无法访问 SmartScreen", content);
    titleLabel_->setStyleSheet("color: white; font-size: 22px; font-weight: bold; background: transparent;");
    contentLayout->addWidget(titleLabel_);
    contentLayout->addSpacing(16);

    // 描述
    descLabel_ = new QLabel(
        "检查你的 Internet 连接也没用。无法访问 Linux Defender SmartScreen，因此本来也无法帮助你确定是否可以运行此程序。",
        content);
    descLabel_->setStyleSheet("color: white; font-size: 13px; background: transparent;");
    descLabel_->setWordWrap(true);
    contentLayout->addWidget(descLabel_);
    contentLayout->addSpacing(20);

    // 分隔线
    QFrame* line = new QFrame(content);
    line->setFrameShape(QFrame::HLine);
    line->setStyleSheet("background-color: rgba(255,255,255,0.3);");
    line->setFixedHeight(1);
    contentLayout->addWidget(line);
    contentLayout->addSpacing(12);

    // 信息标签
    auto addInfoRow = [&](const QString& label, const QString& value, QLabel*& valueLabel) {
        QHBoxLayout* row = new QHBoxLayout();
        row->setSpacing(0);

        QLabel* lbl = new QLabel(label, content);
        lbl->setStyleSheet("color: rgba(255,255,255,0.8); font-size: 13px; background: transparent;");
        lbl->setFixedWidth(80);
        row->addWidget(lbl);

        valueLabel = new QLabel(value, content);
        valueLabel->setStyleSheet("color: white; font-size: 13px; font-weight: bold; background: transparent;");
        row->addWidget(valueLabel);
        row->addStretch();
        contentLayout->addLayout(row);
        contentLayout->addSpacing(6);
    };

    addInfoRow("发布者:", "未知", publisherValue_);
    addInfoRow("文件类型:", suffix, fileTypeValue_);
    addInfoRow("应用:", fileName, appValue_);

    contentLayout->addStretch();
    mainLayout->addWidget(content);

    // 底部按钮栏
    QWidget* bottomBar = new QWidget(this);
    bottomBar->setStyleSheet("background-color: #005A9E;");
    bottomBar->setFixedHeight(72);
    QHBoxLayout* btnLayout = new QHBoxLayout(bottomBar);
    btnLayout->setContentsMargins(0, 0, 32, 0);
    btnLayout->addStretch();

    // 不运行按钮
    noRunButton_ = new QPushButton("不运行", bottomBar);
    noRunButton_->setFixedSize(100, 36);
    noRunButton_->setStyleSheet(
        "QPushButton {"
        "  background-color: #2C3E50;"
        "  color: white;"
        "  border: 1px solid #1A252F;"
        "  border-radius: 2px;"
        "  font-size: 13px;"
        "}"
        "QPushButton:hover { background-color: #34495E; }"
        "QPushButton:pressed { background-color: #1A252F; }"
    );
    connect(noRunButton_, &QPushButton::clicked, this, &DialogWindow::onReject);
    btnLayout->addWidget(noRunButton_);

    btnLayout->addSpacing(8);

    // 运行按钮
    runButton_ = new QPushButton("运行", bottomBar);
    runButton_->setFixedSize(100, 36);
    runButton_->setStyleSheet(
        "QPushButton {"
        "  background-color: #F0F0F0;"
        "  color: #333333;"
        "  border: 1px solid #CCCCCC;"
        "  border-radius: 2px;"
        "  font-size: 13px;"
        "}"
        "QPushButton:hover { background-color: #E0E0E0; }"
        "QPushButton:pressed { background-color: #D0D0D0; }"
    );
    connect(runButton_, &QPushButton::clicked, this, &DialogWindow::onAccept);
    btnLayout->addWidget(runButton_);

    mainLayout->addWidget(bottomBar);
}

void DialogWindow::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    // 绘制圆角边框
    p.setPen(QPen(QColor(0, 50, 90), 2));
    p.setBrush(Qt::NoBrush);
    p.drawRoundedRect(rect().adjusted(1, 1, -1, -1), 0, 0);
}

void DialogWindow::onAccept() {
    accepted_ = true;
    accept();
}

void DialogWindow::onReject() {
    accepted_ = false;
    reject();
}
