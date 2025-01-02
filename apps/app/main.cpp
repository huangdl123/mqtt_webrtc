#include "mainwindow.h"
#include "vcprtc/logger/logger.h"

#include <QApplication>

#include <shellapi.h>
//无法操作状态栏和windows键
int runAsAdmin() 
{
    SHELLEXECUTEINFO sei = { sizeof(sei) };
    sei.lpVerb = "runas";
    sei.lpFile = "mqtt_webrtc.exe";  // 替换为你的程序的名称
    sei.nShow = SW_NORMAL;
    if (!ShellExecuteEx(&sei)) {
        // 处理 ShellExecuteEx 失败的情况
        return 1;
    }
    return 0;
}

int main(int argc, char *argv[])
{
    //runAsAdmin();
    QApplication a(argc, argv);
    vcprtc::Logger::init();
    MainWindow w;
    w.show();

    return a.exec();
}
