#ifndef SMARTSCREEN_DIALOGWINDOW_H
#define SMARTSCREEN_DIALOGWINDOW_H

#include <QMainWindow>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

class DialogWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit DialogWindow(const QString& filePath, QWidget* parent = nullptr);
    ~DialogWindow() override = default;

    // 获取用户选择结果: true=允许, false=取消
    bool userAccepted() const { return accepted_; }

private slots:
    void onAccept();
    void onReject();

private:
    QString filePath_;
    bool accepted_ = false;

    QLabel* iconLabel_;
    QLabel* titleLabel_;
    QLabel* messageLabel_;
    QLabel* fileLabel_;
    QPushButton* acceptButton_;
    QPushButton* rejectButton_;
};

#endif // SMARTSCREEN_DIALOGWINDOW_H
