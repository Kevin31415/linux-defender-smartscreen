#include "dialogwindow.h"
#include <QApplication>
#include <QStyle>
#include <QFileInfo>

DialogWindow::DialogWindow(const QString& filePath, QWidget* parent)
    : QMainWindow(parent), filePath_(filePath) {
    setWindowTitle("SmartScreen 安全警告");
    setFixedSize(420, 260);
    setWindowFlags(windowFlags() | Qt::WindowStaysOnTopHint);

    QWidget* central = new QWidget(this);
    setCentralWidget(central);

    QVBoxLayout* mainLayout = new QVBoxLayout(central);
    mainLayout->setSpacing(12);
    mainLayout->setContentsMargins(24, 24, 24, 24);

    // 警告图标
    iconLabel_ = new QLabel(this);
    iconLabel_->setPixmap(QApplication::style()->standardIcon(QStyle::SP_MessageBoxWarning).pixmap(48, 48));
    iconLabel_->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(iconLabel_);

    // 标题
    titleLabel_ = new QLabel("文件来自网络下载", this);
    titleLabel_->setStyleSheet("font-weight: bold; font-size: 16px;");
    titleLabel_->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(titleLabel_);

    // 文件路径
    QFileInfo fi(filePath_);
    fileLabel_ = new QLabel(QString("文件: %1").arg(fi.fileName()), this);
    fileLabel_->setStyleSheet("font-size: 13px; color: #666;");
    fileLabel_->setAlignment(Qt::AlignCenter);
    fileLabel_->setWordWrap(true);
    mainLayout->addWidget(fileLabel_);

    // 提示信息
    messageLabel_ = new QLabel("是否确定要运行此文件？", this);
    messageLabel_->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(messageLabel_);

    mainLayout->addStretch();

    // 按钮布局
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->setSpacing(12);

    acceptButton_ = new QPushButton("仍然运行", this);
    acceptButton_->setMinimumHeight(36);
    acceptButton_->setStyleSheet(
        "QPushButton { background-color: #2196F3; color: white; border: none; "
        "border-radius: 4px; padding: 8px 24px; font-size: 14px; }"
        "QPushButton:hover { background-color: #1976D2; }"
    );
    connect(acceptButton_, &QPushButton::clicked, this, &DialogWindow::onAccept);
    buttonLayout->addWidget(acceptButton_);

    rejectButton_ = new QPushButton("取消", this);
    rejectButton_->setMinimumHeight(36);
    rejectButton_->setStyleSheet(
        "QPushButton { background-color: #f5f5f5; color: #333; border: 1px solid #ddd; "
        "border-radius: 4px; padding: 8px 24px; font-size: 14px; }"
        "QPushButton:hover { background-color: #e0e0e0; }"
    );
    connect(rejectButton_, &QPushButton::clicked, this, &DialogWindow::onReject);
    buttonLayout->addWidget(rejectButton_);

    mainLayout->addLayout(buttonLayout);
}

void DialogWindow::onAccept() {
    accepted_ = true;
    close();
}

void DialogWindow::onReject() {
    accepted_ = false;
    close();
}
