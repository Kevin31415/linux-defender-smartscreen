#ifndef SMARTSCREEN_DIALOGWINDOW_H
#define SMARTSCREEN_DIALOGWINDOW_H

#include <QDialog>
#include <QLabel>
#include <QPushButton>

class DialogWindow : public QDialog {
    Q_OBJECT

public:
    explicit DialogWindow(const QString& filePath, QWidget* parent = nullptr);
    ~DialogWindow() override = default;

    bool userAccepted() const { return accepted_; }

protected:
    void paintEvent(QPaintEvent* event) override;

private slots:
    void onAccept();
    void onReject();

private:
    void setupUi();
    void setInfoText(const QString& publisher, const QString& fileType, const QString& appName);

    QString filePath_;
    bool accepted_ = false;

    QLabel* titleLabel_;
    QLabel* descLabel_;
    QLabel* publisherValue_;
    QLabel* fileTypeValue_;
    QLabel* appValue_;
    QPushButton* runButton_;
    QPushButton* noRunButton_;
};

#endif // SMARTSCREEN_DIALOGWINDOW_H
