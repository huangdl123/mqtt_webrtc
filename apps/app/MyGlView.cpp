#include "MyGlView.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <QMutexLocker>
#include <QSurface>
#include <QOffscreenSurface>
#include <QThread>
#include <QOpenGLShader>
#include <QOpenGLTexture>
#include <QPainter>

#include <api/video/i420_buffer.h>

#define PROGRAM_VERTEX_ATTRIBUTE 0
#define PROGRAM_TEXCOORD_ATTRIBUTE 1

#define ATTRIB_VERTEX 0
#define ATTRIB_TEXTURE 1

long long getCurrentTimeMillis2()
{
    auto currentTime = std::chrono::system_clock::now().time_since_epoch();
    return std::chrono::duration_cast<std::chrono::milliseconds>(currentTime).count();
}


struct MyGlView::OpenGLDisplayImpl
{
    OpenGLDisplayImpl()
        //: mBufYuv(nullptr)
        //, mFrameSize(0)
    {}

    //GLvoid* mBufYuv;
    //int                     mFrameSize;

    QOpenGLShader* mVShader;
    QOpenGLShader* mFShader;
    QOpenGLShaderProgram* mShaderProgram;

    QOpenGLTexture* mTextureY;
    QOpenGLTexture* mTextureU;
    QOpenGLTexture* mTextureV;

    GLuint                  id_y, id_u, id_v;
    int                     textureUniformY, textureUniformU, textureUniformV;
    GLsizei                 mVideoW, mVideoH;

};

MyGlView::MyGlView(QWidget *parent) :
    QOpenGLWidget(parent),
    impl(new OpenGLDisplayImpl)
{
    _eventHandlerEnabled = false;
    _orgWidth = -1;
    _orgHeight = -1;
    _yuv = new uint8_t[8192 * 4196 * 3 / 2];
    _lastRecTime = 0;
    _lastPicTime = 0;
    _frameCount = 0;
    _currentFps = 0.0;
    _timer = new QTimer(this);

    connect(_timer, &QTimer::timeout, this, &MyGlView::timeoutHandler);
    _timer->start(1000);
}

MyGlView::~MyGlView()
{
    _timer->stop();
    delete _timer;
    _timer = NULL;
    makeCurrent();
    delete[] _yuv;
    _yuv = NULL;

    //delete[] reinterpret_cast<unsigned char*>(impl->mBufYuv);
}

void MyGlView::initializeGL()
{
    //makeCurrent();
    initializeOpenGLFunctions();

    glEnable(GL_DEPTH_TEST);

    /* Modern opengl rendering pipeline relies on shaders to handle incoming data.
     *  Shader: is a small function written in OpenGL Shading Language (GLSL).
     * GLSL is the language that makes up all OpenGL shaders.
     * The syntax of the specific GLSL language requires the reader to find relevant information. */

     // Initialize the vertex shader object
    impl->mVShader = new QOpenGLShader(QOpenGLShader::Vertex, this);

    //Vertex shader source
    const char* vsrc = "attribute vec4 vertexIn; \
        attribute vec2 textureIn; \
        varying vec2 textureOut;  \
        void main(void)           \
        {                         \
            gl_Position = vertexIn; \
            textureOut = textureIn; \
        }";

    //Compile the vertex shader program
    bool bCompile = impl->mVShader->compileSourceCode(vsrc);
    if (!bCompile)
    {
        //throw OpenGlException();
    }

    // Initialize the fragment shader function yuv converted to rgb
    impl->mFShader = new QOpenGLShader(QOpenGLShader::Fragment, this);

    // Fragment shader source code

#ifdef QT_OPENGL_ES_2
    const char* fsrc = "precision mediump float; \
    varying vec2 textureOut; \
        uniform sampler2D tex_y; \
        uniform sampler2D tex_u; \
        uniform sampler2D tex_v; \
        void main(void) \
        { \
            vec3 yuv; \
            vec3 rgb; \
            yuv.x = texture2D(tex_y, textureOut).r; \
            yuv.y = texture2D(tex_u, textureOut).r - 0.5; \
            yuv.z = texture2D(tex_v, textureOut).r - 0.5; \
            rgb = mat3( 1,       1,         1, \
                        0,       -0.39465,  2.03211, \
                        1.13983, -0.58060,  0) * yuv; \
            gl_FragColor = vec4(rgb, 1); \
        }";
#else
    const char* fsrc = "varying vec2 textureOut; \
    uniform sampler2D tex_y; \
    uniform sampler2D tex_u; \
    uniform sampler2D tex_v; \
    void main(void) \
    { \
        vec3 yuv; \
        vec3 rgb; \
        yuv.x = texture2D(tex_y, textureOut).r; \
        yuv.y = texture2D(tex_u, textureOut).r - 0.5; \
        yuv.z = texture2D(tex_v, textureOut).r - 0.5; \
        rgb = mat3( 1,       1,         1, \
                    0,       -0.39465,  2.03211, \
                    1.13983, -0.58060,  0) * yuv; \
        gl_FragColor = vec4(rgb, 1); \
    }";
#endif

    bCompile = impl->mFShader->compileSourceCode(fsrc);
    if (!bCompile)
    {
        //throw OpenGlException();
    }

    // Create a shader program container
    impl->mShaderProgram = new QOpenGLShaderProgram(this);
    // Add the fragment shader to the program container
    impl->mShaderProgram->addShader(impl->mFShader);
    // Add a vertex shader to the program container
    impl->mShaderProgram->addShader(impl->mVShader);
    // Bind the property vertexIn to the specified location ATTRIB_VERTEX, this property
    // has a declaration in the vertex shader source
    impl->mShaderProgram->bindAttributeLocation("vertexIn", ATTRIB_VERTEX);
    // Bind the attribute textureIn to the specified location ATTRIB_TEXTURE, the attribute 
    // has a declaration in the vertex shader source
    impl->mShaderProgram->bindAttributeLocation("textureIn", ATTRIB_TEXTURE);
    //Link all the shader programs added to
    impl->mShaderProgram->link();
    //activate all links
    impl->mShaderProgram->bind();
    // Read the position of the data variables tex_y, tex_u, tex_v in the shader, the declaration
    // of these variables can be seen in 
    // fragment shader source 
    impl->textureUniformY = impl->mShaderProgram->uniformLocation("tex_y");
    impl->textureUniformU = impl->mShaderProgram->uniformLocation("tex_u");
    impl->textureUniformV = impl->mShaderProgram->uniformLocation("tex_v");

    //Vertex matrix
    static const GLfloat vertexVertices[] = {
        -1.0f, -1.0f,
         1.0f, -1.0f,
         -1.0f, 1.0f,
         1.0f, 1.0f,
    };

    //Texture matrix
    static const GLfloat textureVertices[] = {
        0.0f,  1.0f,
        1.0f,  1.0f,
        0.0f,  0.0f,
        1.0f,  0.0f,
    };

    // Set the value of the vertex matrix of the attribute ATTRIB_VERTEX and format
    glVertexAttribPointer(ATTRIB_VERTEX, 2, GL_FLOAT, 0, 0, vertexVertices);
    // Set the texture matrix value and format of the attribute ATTRIB_TEXTURE
    glVertexAttribPointer(ATTRIB_TEXTURE, 2, GL_FLOAT, 0, 0, textureVertices);
    // Enable the ATTRIB_VERTEX attribute data, the default is off
    glEnableVertexAttribArray(ATTRIB_VERTEX);
    // Enable the ATTRIB_TEXTURE attribute data, the default is off
    glEnableVertexAttribArray(ATTRIB_TEXTURE);

    // Create y, u, v texture objects respectively    
    impl->mTextureY = new QOpenGLTexture(QOpenGLTexture::Target2D);
    impl->mTextureU = new QOpenGLTexture(QOpenGLTexture::Target2D);
    impl->mTextureV = new QOpenGLTexture(QOpenGLTexture::Target2D);
    impl->mTextureY->create();
    impl->mTextureU->create();
    impl->mTextureV->create();

    // Get the texture index value of the return y component
    impl->id_y = impl->mTextureY->textureId();
    // Get the texture index value of the returned u component
    impl->id_u = impl->mTextureU->textureId();
    // Get the texture index value of the returned v component
    impl->id_v = impl->mTextureV->textureId();

    glClearColor(0.3, 0.3, 0.3, 0.0); // set the background color
}

void MyGlView::resizeGL(int width, int height)
{
    //makeCurrent();
    //glViewport(0, 0, (GLint)width, (GLint)height);
    //glMatrixMode(GL_PROJECTION);
    //glLoadIdentity();
    //glOrtho(0, width, -height, 0, 0, 1);
    //glMatrixMode(GL_MODELVIEW);
    QMutexLocker locker(&_drawMutex);
    calcRenderParam();
    //updateScene();
}

void MyGlView::updateAsync()
{
    if (isVisible())
    {
        //update();
        QMetaObject::invokeMethod(this, "update", Qt::QueuedConnection);
    }
}

void MyGlView::enableMouseTracking(bool enabled)
{
    setMouseTracking(enabled);
}

void MyGlView::paintGL()
{
    QMutexLocker locker(&_drawMutex);
    makeCurrent();
    
    glClear(GL_COLOR_BUFFER_BIT);

    if (_name == "") return;

    long long current = getCurrentTimeMillis2();
    //if(current - _lastPicTime > 3 * 1000)
    
    if(_orgWidth > 0 && _orgHeight > 0 && current - _lastPicTime < 3 * 1000)
    {
        uint8_t* yuv = _yuv;

        //Texture matrix
        static const GLfloat textureVertices[] = {
            0.0f,  1.0f,
            1.0f,  1.0f,
            0.0f,  0.0f,
            1.0f,  0.0f,
        };
        impl->mShaderProgram->bind();

        // Set the value of the vertex matrix of the attribute ATTRIB_VERTEX and format
        glVertexAttribPointer(ATTRIB_VERTEX, 2, GL_FLOAT, 0, 0, _vertexVertices);
        // Set the texture matrix value and format of the attribute ATTRIB_TEXTURE
        glVertexAttribPointer(ATTRIB_TEXTURE, 2, GL_FLOAT, 0, 0, textureVertices);
        // Enable the ATTRIB_VERTEX attribute data, the default is off
        glEnableVertexAttribArray(ATTRIB_VERTEX);
        // Enable the ATTRIB_TEXTURE attribute data, the default is off
        glEnableVertexAttribArray(ATTRIB_TEXTURE);

        glActiveTexture(GL_TEXTURE0);
        // Use the texture generated from y to generate texture
        glBindTexture(GL_TEXTURE_2D, impl->id_y);

        // Fixes abnormality with 174x100 yuv data
        //glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        //glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
        //glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0);
        //glPixelStorei(GL_UNPACK_SKIP_ROWS, 0);

        // Use the memory mBufYuv data to create a real y data texture
        //glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, impl->mVideoW, impl->mVideoH, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, impl->mBufYuv);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, _orgWidth, _orgHeight, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, yuv);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        // Load u data texture
        glActiveTexture(GL_TEXTURE1);//Activate texture unit GL_TEXTURE1
        glBindTexture(GL_TEXTURE_2D, impl->id_u);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, _orgWidth / 2, _orgHeight / 2 , 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, yuv + _orgWidth * _orgHeight);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        // Load v data texture
        glActiveTexture(GL_TEXTURE2);//Activate texture unit GL_TEXTURE2
        glBindTexture(GL_TEXTURE_2D, impl->id_v);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, _orgWidth / 2, _orgHeight / 2, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, yuv + _orgWidth * _orgHeight * 5 / 4);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        // Specify y texture to use the new value can only use 0, 1, 2, etc. to represent
        // the index of the texture unit, this is the place where opengl is not humanized
        //0 corresponds to the texture unit GL_TEXTURE0 1 corresponds to the
        // texture unit GL_TEXTURE1 2 corresponds to the texture unit GL_TEXTURE2
        glUniform1i(impl->textureUniformY, 0);
        // Specify the u texture to use the new value
        glUniform1i(impl->textureUniformU, 1);
        // Specify v texture to use the new value
        glUniform1i(impl->textureUniformV, 2);
        // Use the vertex array way to draw graphics
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    }

    if (_name != "")
    {
        bool noSignal = current - _lastPicTime >= 3 * 1000;
        QString fpsText = noSignal ? "No signal" : QString::number(_currentFps, 'f', 1) + "fps";
        QString text = QString::fromStdString(_name) + " " + fpsText;
        QPainter painter(this);
        painter.beginNativePainting();

        QColor noColor(0, 0, 0, 0);
        QColor brushColor = noSignal ? QColor(255, 255, 0) : QColor(255, 255, 0, 100);
        QColor penColor = noSignal ? QColor(255, 0, 0) : QColor(0, 0, 255);
        QFont font;
        font.setPointSize(12);
        //font.setBold(true);

        painter.setFont(font);
        painter.setPen(noColor);
        painter.setBrush(QBrush(brushColor));

        QFontMetrics fontMetrics(font);
        QRect rect = fontMetrics.boundingRect(text);
        int tx = 5;
        int ty = 5;
        int xp = 10;
        int yp = 5;

        painter.drawRect(tx, ty, rect.width() + 2 * xp, rect.height() + 2 * yp);

        painter.setPen(penColor);
        painter.drawText(tx + xp, yp + rect.height(), text);

        painter.endNativePainting();
    }
    
}

void MyGlView::calcRenderParam()
{
    float ratio = 1.0;
    ratio = (float)_orgWidth /(float)_orgHeight;

    _renderWidth = this->size().width();
    _renderHeight = floor(_renderWidth / ratio);

    if (_renderHeight > this->size().height())
    {
        _renderHeight = this->size().height();
        _renderWidth = floor(_renderHeight * ratio);
    }

    _renderScale = (double)_renderWidth / _orgWidth;
    _renderX = floor((this->size().width() - _renderWidth) / 2);
    _renderY = floor((this->size().height() - _renderHeight) / 2);

    float x1 = (float)_renderX / this->size().width();
    float y1 = (float)_renderY / this->size().height();
    float x2 = (float)(_renderX + _renderWidth) / this->size().width();
    float y2 = (float)(_renderY + _renderHeight) / this->size().height();
    x1 = 2 * x1 - 1.0;
    y1 = 2 * y1 - 1.0;
    x2 = 2 * x2 - 1.0;
    y2 = 2 * y2 - 1.0;
    _vertexVertices[0] = x1; _vertexVertices[1] = y1;
    _vertexVertices[2] = x2; _vertexVertices[3] = y1;
    _vertexVertices[4] = x1; _vertexVertices[5] = y2;
    _vertexVertices[6] = x2; _vertexVertices[7] = y2;
}

void MyGlView::mouseDoubleClickEvent(QMouseEvent* event)
{
    int x0 = event->x();
    int y0 = event->y();
    int px = qRound((x0 - _renderX) / _renderScale);
    int py = qRound((y0 + _renderY) / _renderScale);
    
    if (px >= 0 && py >= 0 && px < _orgWidth && py < _orgHeight)
    {
        Q_EMIT imageDoubleClicked(px, py);
    }
}

void MyGlView::OnFrame(const vcprtc::VideoFrame& frame)
{
    rtc::scoped_refptr<webrtc::I420BufferInterface> buffer(frame.video_frame_buffer()->ToI420());
    if (frame.rotation() != webrtc::kVideoRotation_0)
    {
        buffer = webrtc::I420Buffer::Rotate(*buffer, frame.rotation());
    }

    QMutexLocker locker(&_drawMutex);
    int width = buffer->width();
    int height = buffer->height();
    if (_orgWidth != width || _orgHeight != height)
    {
        _orgWidth = width;
        _orgHeight = height;
        calcRenderParam();
    }

    uint8_t* dstY = _yuv;
    uint8_t* dstU = _yuv + width * height;
    uint8_t* dstV = _yuv + width * height + width * height / 4;
    const uint8_t* srcY = buffer->DataY();
    const uint8_t* srcU = buffer->DataU();
    const uint8_t* srcV = buffer->DataV();

    int strideY = buffer->StrideY();
    int strideU = buffer->StrideU();
    int strideV = buffer->StrideV();
    for (int i = 0; i < height; i++)
    {
        memcpy(dstY, srcY, width);
        dstY += width;
        srcY += strideY;
        if (i % 2 == 0)
        {
            memcpy(dstU, srcU, width / 2);
            dstU += width / 2;
            srcU += strideU;

            memcpy(dstV, srcV, width / 2);
            dstV += width / 2;
            srcV += strideU;
        }
    }

    _frameCount++;

    long long current = getCurrentTimeMillis2();
    _lastPicTime = current;
    if (_lastRecTime < 0) _lastRecTime = current;
    if (current - _lastRecTime >= 1000)
    {
        _currentFps = 1000.0 * _frameCount / (current - _lastRecTime);
        _frameCount = 0;
        _lastRecTime = current;
    }
    
    updateAsync();
}

void MyGlView::setName(const std::string& name, bool eventHandlerEnabled)
{
    QMutexLocker locker(&_drawMutex);
    _name = name;
    updateAsync();
    if (!_name.empty() && eventHandlerEnabled)
    {
        QMetaObject::invokeMethod(this, "enableMouseTracking", Qt::QueuedConnection, Q_ARG(bool, true));
        _eventHandlerEnabled = true;
    }
    else
    {
        QMetaObject::invokeMethod(this, "enableMouseTracking", Qt::QueuedConnection, Q_ARG(bool, false));
        _eventHandlerEnabled = false;
    }
}

QPoint MyGlView::getRealPoint(const QPoint& pos)
{
    int x0 = pos.x();
    int y0 = pos.y();
    if (x0 < _renderX || x0 >= _renderX + _renderWidth || y0 < _renderY || y0 >= _renderY + _renderHeight) return QPoint(-1, -1);;
    int x1 = (x0 - _renderX) * _orgWidth / _renderWidth;
    int y1 = (y0 - _renderY) * _orgHeight / _renderHeight;
    //ILOG("{}: ({}, {})--->({}, {})", _name, x0, y0, x1, y1);
    return QPoint(x1, y1);
}

bool MyGlView::event(QEvent* event)
{
    if (!_eventHandlerEnabled || _name == "" || _orgWidth <= 0 || _orgHeight <= 0)
    {
        return QWidget::event(event);
    }
    MyEvent me;
    QEvent::Type etype = event->type();
    bool handled = false;
    switch (etype)
    {
    case QEvent::MouseButtonPress:
    case QEvent::MouseButtonRelease:
    case QEvent::MouseMove:
    case QEvent::MouseButtonDblClick:
    
    {
        QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);
        QPoint pt = getRealPoint(mouseEvent->pos());
        me.etype = etype;
        me.mouse.btn = mouseEvent->button();
        me.mouse.btn2 = 0;
        me.mouse.x = pt.x();
        me.mouse.y = pt.y();
        handled = pt.x() >= 0 && pt.y() >= 0;
        break;
    }
    case QEvent::Wheel:
    {
        QWheelEvent* wheelEvent = static_cast<QWheelEvent*>(event);
        QPoint pt = getRealPoint(wheelEvent->pos());
        me.etype = etype;
        me.wheel.dx = wheelEvent->angleDelta().x();
        me.wheel.dy = wheelEvent->angleDelta().y();
        me.wheel.x = pt.x();
        me.wheel.y = pt.y();
        handled = pt.x() >= 0 && pt.y() >= 0;
        break;
    }


    default:
        ;
    }


    if (handled)
    {
        ILOG("{}: {} {} {} ({}, {})", _name, me.etype, me.mouse.btn, me.mouse.btn2, me.mouse.x, me.mouse.y);
        Q_EMIT eventAction(_name, me);
    }
    return QWidget::event(event);
}

void MyGlView::timeoutHandler()
{
    QMutexLocker locker(&_drawMutex);
    long long current = getCurrentTimeMillis2();
    if (_orgWidth > 0 && _orgHeight > 0 && current - _lastPicTime > 3 * 1000)
    {
        _orgWidth = -1;
        _orgHeight = -1;
        _currentFps = 0;
        updateAsync();
        ILOG("picture lost, init view.");
    }
}
