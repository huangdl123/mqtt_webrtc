#pragma once

#include <QOpenGLWidget>
#include <QOpenGLFunctions_2_0>
//#include <QOpenGLFunctions_4_5_Core>
#include <QMutex>
#include <QMouseEvent>
#include <QTimer>
#include "vcprtc/app.h"

typedef struct 
{
    uint8_t type;
    uint8_t etype;
    
    union 
    {
        struct
        {
            int16_t btn;
            int16_t btn2;
            int16_t x;
            int16_t y;
        } mouse;
        struct
        {
            int16_t dx;
            int16_t dy;
            int16_t x;
            int16_t y;
        } wheel;
    };

} MyEvent;

class MyGlView : public QOpenGLWidget, protected QOpenGLFunctions_2_0, public vcprtc::IVideoRenderer
{
    Q_OBJECT
public:
    explicit MyGlView(QWidget *parent = 0);
    ~MyGlView();
    void setName(const std::string& name, bool eventHandlerEnabled=false);
    std::string getName()
    {
        return _name;
    }

    void OnFrame(const vcprtc::VideoFrame& frame) override;
public Q_SLOTS:
    void timeoutHandler();
    void enableMouseTracking(bool enabled);
Q_SIGNALS:
    void imageDoubleClicked(int x, int y);
    void eventAction(const std::string& name, const MyEvent& event);

protected:
    void initializeGL() override;                    // OpenGL initialization
    void paintGL() override;                         // OpenGL Rendering
    void resizeGL(int width, int height) override;   // Widget Resize Event

    void updateAsync();
    void mouseDoubleClickEvent(QMouseEvent* event);
    //void mouseMoveEvent(QMouseEvent* event) override;
    void calcRenderParam();
    bool event(QEvent* event) override;
    QPoint getRealPoint(const QPoint& pos);

    

private:
    std::string _name;
    uint8_t* _yuv;
    GLfloat _vertexVertices[8];

    int _orgWidth;
    int _orgHeight;
    int _renderWidth;
    int _renderHeight;
    int _renderX;
    int _renderY;
    float _renderScale;
	QMutex _drawMutex;
    struct OpenGLDisplayImpl;
    QScopedPointer<OpenGLDisplayImpl> impl;

    long long _lastRecTime;
    long long _lastPicTime;
    int _frameCount;
    double _currentFps;
    QTimer* _timer;
    bool _eventHandlerEnabled;
};

