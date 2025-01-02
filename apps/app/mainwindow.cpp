#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include "vcprtc/app.h"

std::string convertToStdString(const QString& str)
{
    return str.toLocal8Bit().constData();
}

#include <windows.h>

// 移动鼠标到屏幕上的指定位置
void MoveMouse(int x, int y) {
    CURSORINFO cursorInfo = { 0 };
    cursorInfo.cbSize = sizeof(CURSORINFO);
    GetCursorInfo(&cursorInfo);

    // 如果鼠标当前是隐藏的，显示鼠标
    if (cursorInfo.flags == CURSOR_SHOWING) {
        SetCursor(cursorInfo.hCursor);
    }

    // 将鼠标移动到屏幕上的指定位置
    SetCursorPos(x, y);
}

// 模拟鼠标点击
void ClickMouse(int x, int y, bool leftButton = true) {
    MoveMouse(x, y); // 先移动鼠标到指定位置

    // 模拟点击
    INPUT input = { 0 };
    input.type = INPUT_MOUSE;
    if (leftButton) {
        input.mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
        SendInput(1, &input, sizeof(INPUT));

        input.mi.dwFlags = MOUSEEVENTF_LEFTUP;
        SendInput(1, &input, sizeof(INPUT));
    }
    else {
        // 右键点击类似，使用 MOUSEEVENTF_RIGHTDOWN 和 MOUSEEVENTF_RIGHTUP
    }
}

void PressMouse(int btn, int x, int y)
{
    //MoveMouse(x, y);
    INPUT input = { 0 };
    input.type = INPUT_MOUSE;
    switch (btn)
    {
    case Qt::MouseButton::LeftButton:
        input.mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
        break;
    case Qt::MouseButton::RightButton:
        input.mi.dwFlags = MOUSEEVENTF_RIGHTDOWN;
        break;
    case Qt::MouseButton::MiddleButton:
        input.mi.dwFlags = MOUSEEVENTF_MIDDLEDOWN;
        break;
    }
    
    SendInput(1, &input, sizeof(INPUT));
}

void ReleaseMouse(int btn, int x, int y)
{
    //MoveMouse(x, y);
    INPUT input = { 0 };
    input.type = INPUT_MOUSE;
    switch (btn)
    {
    case Qt::MouseButton::LeftButton:
        input.mi.dwFlags = MOUSEEVENTF_LEFTUP;
        break;
    case Qt::MouseButton::RightButton:
        input.mi.dwFlags = MOUSEEVENTF_RIGHTUP;
        break;
    case Qt::MouseButton::MiddleButton:
        input.mi.dwFlags = MOUSEEVENTF_MIDDLEUP;
        break;
    }
    SendInput(1, &input, sizeof(INPUT));
}

void DoWheel(int x, int y)
{
    INPUT input = { 0 };
    input.type = INPUT_MOUSE;
    //input.mi.dx = 0;
    //input.mi.dy = 0;
    // 正值表示向前滚动，负值表示向后滚动，滚动量通常为 WHEEL_DELTA 的倍数
    input.mi.mouseData = y;
    input.mi.dwFlags = MOUSEEVENTF_WHEEL;
    //input.mi.time = 0;
    //input.mi.dwExtraInfo = 0;
    SendInput(1, &input, sizeof(INPUT));
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    for (int i = 0; i < 4; i++)
    {
        MyGlView* render = new MyGlView(this);
        render->setProperty("autoFillBackground", false);
        ui->gridLayout->addWidget(render);
        //ui->gridLayout->addItem(new QWidgetItem(render), i/2, i%2);
        _renders.push_back(render);
        connect(render, &MyGlView::eventAction, this, &MainWindow::onEventAction);
    }
}

MainWindow::~MainWindow()
{
    for (int i = 0; i < _localVideos.size(); i++)
    {
        vcprtc::IRTCEngine::instance()->removeVideoTrack(_localVideos[i]);
    }
    
    delete ui;
}

MyGlView* MainWindow::getUnusedRender()
{
    for (int i = 0; i < _renders.size(); i++)
    {
        if (_renders[i]->getName() == "")
        {
            return _renders[i];
        }
    }
    return NULL;
}

void MainWindow::unuseRender(const std::string& name)
{
    for (int i = 0; i < _renders.size(); i++)
    {
        if (_renders[i]->getName() == name)
        {
            _renders[i]->setName("");
            break;
        }
    }
}

void MainWindow::onNewPeerClient(const std::string& sid)
{
    std::string video = _localVideos.size() > 0 ? _localVideos[0] : "";
    auto pc = _app->getPeer(sid);
    if (!pc) return;
    pc->setLocalVideo(video);
    MyGlView* render = getUnusedRender();
    if (render)
    {
        vcprtc::IRTCEngine::instance()->addVideoSink(sid, render);
        render->setName(sid, true);
    }
    _peers.push_back(sid);
    pc->onData.connect(this, &MainWindow::onPeerClientData);
}

void MainWindow::onDeletePeerClient(const std::string& sid)
{
    vcprtc::IRTCEngine::instance()->removeVideoSink(sid);
    _peers.removeOne(sid);
    unuseRender(sid);
}

void MainWindow::on_clientChangeBtn_clicked()
{
    QString txt = ui->clientEdit->text();
    std::string clientId = convertToStdString(txt);
    if (_app && _app->clientId() == clientId)
    {
        return;
    }
    _app = vcprtc::IApp::create(clientId);
    _app->onNewPeerClient.connect(this, &MainWindow::onNewPeerClient);
    _app->onDeletePeerClient.connect(this, &MainWindow::onDeletePeerClient);
}

void MainWindow::on_callBtn_clicked()
{
    QString txt = ui->callDstEdit->text();
    std::string sid = convertToStdString(txt);
    if (!_app) return;
    _app->call(sid);
}

void MainWindow::on_hangupBtn_clicked()
{
    QString txt = ui->callDstEdit->text();
    std::string sid = convertToStdString(txt);
    _app->hangup(sid);
}

void MainWindow::on_localShowBtn_clicked()
{
    QString txt = ui->callDstEdit->text();
    std::string sid = convertToStdString(txt);
    static bool enabled = false;
    auto pc = _app->getPeer(sid);
    if (!pc) return;
    pc->muteVideo(enabled);
    enabled = !enabled;
}

void MainWindow::on_remoteShowBtn_clicked()
{

}

void MainWindow::on_newLocalVideoBtn_clicked()
{
    QString txt = ui->videoNameEdit->text();
    std::string videoName = convertToStdString(txt);

    Json::Value config;
    config["name"] = videoName;
    vcprtc::IRTCEngine::instance()->CreateVideoTrack(config);
    //vcprtc::IRTCEngine::instance()->addVideoSink(videoName, ui->localGlView);
    _localVideos.push_back(videoName);
    
    MyGlView* render = getUnusedRender();
    if (render) 
    {
        vcprtc::IRTCEngine::instance()->addVideoSink(videoName, render);
        render->setName(videoName);
    }
}

void MainWindow::on_removeLocalVideoBtn_clicked()
{
    QString txt = ui->videoNameEdit->text();
    std::string videoName = convertToStdString(txt);
    vcprtc::IRTCEngine::instance()->removeVideoTrack(videoName);
    vcprtc::IRTCEngine::instance()->removeVideoSink(videoName);
    _localVideos.removeOne(videoName);

    unuseRender(videoName);
}

void MainWindow::on_msgSendBtn_clicked()
{
    MoveMouse(100, 100);
    return;
    QString txt = ui->callDstEdit->text();
    std::string sid = convertToStdString(txt);
    if (!_app) return;
    vcprtc::IPeerClient* pc = _app->getPeer(sid);
    if (!pc) return;

    static int clickCout = 0;
    clickCout++;
    pc->sendMessage(sid + " msg: "+ std::to_string(clickCout));
}

void MainWindow::onEventAction(const std::string& name, const MyEvent& event)
{
    if (!_app) return;
    vcprtc::IPeerClient* pc = _app->getPeer(name);
    if (!pc) return;
    pc->sendData((void*)&event, sizeof(event));
}

void MainWindow::onPeerClientData(const std::string& name, void* data, int size)
{
    MyEvent* me = (MyEvent*)data;
    //ILOG("onPeerClientData({}) {} {} {} ({},{})", name, me->etype, me->mouse.btn, me->mouse.btn2, me->mouse.x, me->mouse.y);
    
    int x = me->mouse.x;
    int y = me->mouse.y;
    int btn = me->mouse.btn;
    int btn2 = me->mouse.btn2;
    switch (me->etype)
    {
    case QEvent::MouseMove:
        MoveMouse(x, y);
        break;
    case QEvent::MouseButtonPress:
        PressMouse(btn, x, y);
        break;
    case QEvent::MouseButtonRelease:
        ReleaseMouse(btn, x, y);
        break;
    case QEvent::MouseButtonDblClick:
        PressMouse(btn, x, y);
        break;
    case QEvent::Wheel:
        DoWheel(btn, btn2);
        break;
    }
}
