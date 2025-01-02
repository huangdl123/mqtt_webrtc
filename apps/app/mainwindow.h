#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <vcprtc/app.h>
#include "MyGlView.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE


class MainWindow : public QMainWindow, public sigslot::has_slots<>
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    MyGlView* getUnusedRender();
    void unuseRender(const std::string& name);

    void onNewPeerClient(const std::string& sid);
    void onDeletePeerClient(const std::string& sid);
    void onPeerClientData(const std::string& name, void* data, int size);

public Q_SLOTS:
    void on_clientChangeBtn_clicked();
    void on_callBtn_clicked();
    void on_hangupBtn_clicked();
    void on_localShowBtn_clicked();
    void on_remoteShowBtn_clicked();
    void on_newLocalVideoBtn_clicked();
    void on_removeLocalVideoBtn_clicked();
    void on_msgSendBtn_clicked();
    void onEventAction(const std::string& name, const MyEvent& event);

private:
    Ui::MainWindow *ui;
    std::shared_ptr<vcprtc::IApp> _app;
    QVector<std::string> _localVideos;
    QVector<std::string> _peers;
    //std::map<std::string, MyGlView*> _renderMap;
    QVector<MyGlView*> _renders;
};
#endif // MAINWINDOW_H
