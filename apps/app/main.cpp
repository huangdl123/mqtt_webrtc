#include "mainwindow.h"
#include "vcprtc/logger/logger.h"

#include <QApplication>

#include <shellapi.h>
//�޷�����״̬����windows��
int runAsAdmin() 
{
    SHELLEXECUTEINFO sei = { sizeof(sei) };
    sei.lpVerb = "runas";
    sei.lpFile = "mqtt_webrtc.exe";  // �滻Ϊ��ĳ��������
    sei.nShow = SW_NORMAL;
    if (!ShellExecuteEx(&sei)) {
        // ���� ShellExecuteEx ʧ�ܵ����
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
