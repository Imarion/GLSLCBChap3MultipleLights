#include "MultipleLights.h"

#include <QtGlobal>

#include <QDebug>
#include <QFile>
#include <QImage>
#include <QTime>

#include <QVector2D>
#include <QVector3D>
#include <QMatrix4x4>

#include <cmath>
#include <cstring>
#include <sstream>

MyWindow::~MyWindow()
{
    if (mProgram2SidesMultipleLights != 0) delete mProgram2SidesMultipleLights;
    delete mPlane;
    delete mPig;
}

MyWindow::MyWindow()
    : mProgram2SidesMultipleLights(0), currentTimeMs(0), currentTimeS(0)
{
    setSurfaceType(QWindow::OpenGLSurface);
    setFlags(Qt::Window | Qt::WindowSystemMenuHint | Qt::WindowTitleHint | Qt::WindowMinMaxButtonsHint | Qt::WindowCloseButtonHint);

    QSurfaceFormat format;
    format.setDepthBufferSize(24);
    format.setMajorVersion(4);
    format.setMinorVersion(3);
    format.setSamples(4);
    format.setProfile(QSurfaceFormat::CoreProfile);
    setFormat(format);
    create();

    resize(800, 600);

    mContext = new QOpenGLContext(this);
    mContext->setFormat(format);
    mContext->create();

    mContext->makeCurrent( this );

    mFuncs = mContext->versionFunctions<QOpenGLFunctions_4_3_Core>();
    if ( !mFuncs )
    {
        qWarning( "Could not obtain OpenGL versions object" );
        exit( 1 );
    }
    if (mFuncs->initializeOpenGLFunctions() == GL_FALSE)
    {
        qWarning( "Could not initialize core open GL functions" );
        exit( 1 );
    }

    initializeOpenGLFunctions();

    QTimer *repaintTimer = new QTimer(this);
    connect(repaintTimer, &QTimer::timeout, this, &MyWindow::render);
    repaintTimer->start(1000/60);

    QTimer *elapsedTimer = new QTimer(this);
    connect(elapsedTimer, &QTimer::timeout, this, &MyWindow::modCurTime);
    elapsedTimer->start(1);       
}

void MyWindow::modCurTime()
{
    currentTimeMs++;
    currentTimeS=currentTimeMs/1000.0f;
}

void MyWindow::initialize()
{
    CreateVertexBuffer();
    initShaders();
    initMatrices();

    mRotationMatrixLocation = mProgram2SidesMultipleLights->uniformLocation("RotationMatrix");

    glFrontFace(GL_CCW);
    glEnable(GL_DEPTH_TEST);
}

void MyWindow::CreateVertexBuffer()
{
    // Pig
    mFuncs->glGenVertexArrays(1, &mVAOPig);
    mFuncs->glBindVertexArray(mVAOPig);
    mPig   = new VBOMesh("pig_triangulated.obj", true);
    // Create and populate the buffer objects
    unsigned int PigHandles[3];
    glGenBuffers(3, PigHandles);

    glBindBuffer(GL_ARRAY_BUFFER, PigHandles[0]);
    glBufferData(GL_ARRAY_BUFFER, (3 * mPig->getnVerts()) * sizeof(float), mPig->getv(), GL_STATIC_DRAW);

    glBindBuffer(GL_ARRAY_BUFFER, PigHandles[1]);
    glBufferData(GL_ARRAY_BUFFER, (3 * mPig->getnVerts()) * sizeof(float), mPig->getn(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, PigHandles[2]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, 3 * mPig->getnFaces() * sizeof(unsigned int), mPig->getelems(), GL_STATIC_DRAW);

    // Setup the VAO
    // Vertex positions
    mFuncs->glBindVertexBuffer(0, PigHandles[0], 0, sizeof(GLfloat) * 3);
    mFuncs->glVertexAttribFormat(0, 3, GL_FLOAT, GL_FALSE, 0);
    mFuncs->glVertexAttribBinding(0, 0);

    // Vertex normals
    mFuncs->glBindVertexBuffer(1, PigHandles[1], 0, sizeof(GLfloat) * 3);
    mFuncs->glVertexAttribFormat(1, 3, GL_FLOAT, GL_FALSE, 0);
    mFuncs->glVertexAttribBinding(1, 1);

    // Indices
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, PigHandles[2]);

    mFuncs->glBindVertexArray(0);

    // Plane
    mFuncs->glGenVertexArrays(1, &mVAOPlane);
    mFuncs->glBindVertexArray(mVAOPlane);
    mPlane = new VBOPlane(10.0f, 10.0f, 100, 100);
    // Create and populate the buffer objects
    unsigned int PlaneHandles[3];
    glGenBuffers(3, PlaneHandles);

    glBindBuffer(GL_ARRAY_BUFFER, PlaneHandles[0]);
    glBufferData(GL_ARRAY_BUFFER, (3 * mPlane->getnVerts()) * sizeof(float), mPlane->getv(), GL_STATIC_DRAW);

    glBindBuffer(GL_ARRAY_BUFFER, PlaneHandles[1]);
    glBufferData(GL_ARRAY_BUFFER, (3 * mPlane->getnVerts()) * sizeof(float), mPlane->getn(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, PlaneHandles[2]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, 6 * mPlane->getnFaces() * sizeof(unsigned int), mPlane->getelems(), GL_STATIC_DRAW);

    // Setup the VAO
    // Vertex positions
    mFuncs->glBindVertexBuffer(0, PlaneHandles[0], 0, sizeof(GLfloat) * 3);
    mFuncs->glVertexAttribFormat(0, 3, GL_FLOAT, GL_FALSE, 0);
    mFuncs->glVertexAttribBinding(0, 0);

    // Vertex normals
    mFuncs->glBindVertexBuffer(1, PlaneHandles[1], 0, sizeof(GLfloat) * 3);
    mFuncs->glVertexAttribFormat(1, 3, GL_FLOAT, GL_FALSE, 0);
    mFuncs->glVertexAttribBinding(1, 1);

    // Indices
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, PlaneHandles[2]);

    mFuncs->glBindVertexArray(0);
}

void MyWindow::initMatrices()
{
    ModelMatrixPig.rotate( 90.0f, QVector3D(0.0f, 1.0f, 0.0f));

    ModelMatrixPlane.translate(0.0f, -0.45f, 0.0f);

    ViewMatrix.setToIdentity();
    ViewMatrix.lookAt(QVector3D(0.5f, 0.75f, 0.75f), QVector3D(0.0f, 0.0f, 0.0f), QVector3D(0.0f,1.0f,0.0f));
    //ViewMatrix.lookAt(QVector3D(0.0f, 0.0f, -2.0f), QVector3D(0.0f, 0.0f, 0.0f), QVector3D(0.0f,1.0f,0.0f));
}

void MyWindow::resizeEvent(QResizeEvent *)
{
    mUpdateSize = true;

    ProjectionMatrix.setToIdentity();
    ProjectionMatrix.perspective(70.0f, (float)this->width()/(float)this->height(), 0.3f, 100.0f);
}

void MyWindow::setLights()
{
    float x, z;

    for( int i = 0; i < 5; i++ ) {
        std::stringstream name;
        name << "Lights[" << i << "].Position";
        x = 2.0f * cosf((float)(TwoPI / 5.0f) * (float)i);
        z = 2.0f * sinf((float)(TwoPI / 5.0f) * (float)i);
        mProgram2SidesMultipleLights->setUniformValue(name.str().c_str(), ViewMatrix * QVector4D(x, 1.2f, z + 1.0f, 1.0f) );
    }

    mProgram2SidesMultipleLights->setUniformValue("Lights[0].Intensity", 0.0f, 0.8f, 0.8f);
    mProgram2SidesMultipleLights->setUniformValue("Lights[1].Intensity", 0.0f, 0.0f, 0.8f);
    mProgram2SidesMultipleLights->setUniformValue("Lights[2].Intensity", 0.8f, 0.0f, 0.0f);
    mProgram2SidesMultipleLights->setUniformValue("Lights[3].Intensity", 0.0f, 0.8f, 0.0f);
    mProgram2SidesMultipleLights->setUniformValue("Lights[4].Intensity", 0.8f, 0.8f, 0.8f);
}

void MyWindow::render()
{
    if(!isVisible() || !isExposed())
        return;

    if (!mContext->makeCurrent(this))
        return;

    static bool initialized = false;
    if (!initialized) {
        initialize();
        initialized = true;
    }

    if (mUpdateSize) {
        glViewport(0, 0, size().width(), size().height());
        mUpdateSize = false;
    }

    static float EvolvingVal = 0;
    EvolvingVal += 0.1f;

    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Pig
    //QMatrix4x4 RotationMatrix;
    //RotationMatrix.rotate(EvolvingVal, QVector3D(0.1f, 0.0f, 0.1f));
    //ModelMatrix.rotate(0.3f, QVector3D(0.1f, 0.0f, 0.1f));
    mFuncs->glBindVertexArray(mVAOPig);

    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);

    mProgram2SidesMultipleLights->bind();
    {
        setLights();        

        mProgram2SidesMultipleLights->setUniformValue("Material.Kd", 0.4f, 0.4f, 0.4f);
        mProgram2SidesMultipleLights->setUniformValue("Material.Ks", 0.9f, 0.9f, 0.9f);
        mProgram2SidesMultipleLights->setUniformValue("Material.Ka", 0.1f, 0.1f, 0.1f);
        mProgram2SidesMultipleLights->setUniformValue("Material.Shininess", 180.0f);

        QMatrix4x4 mv1 = ViewMatrix * ModelMatrixPig;
        mProgram2SidesMultipleLights->setUniformValue("ModelViewMatrix", mv1);
        mProgram2SidesMultipleLights->setUniformValue("NormalMatrix", mv1.normalMatrix());
        mProgram2SidesMultipleLights->setUniformValue("MVP", ProjectionMatrix * mv1);

        glDrawElements(GL_TRIANGLES, 3 * mPig->getnFaces(), GL_UNSIGNED_INT, ((GLubyte *)NULL + (0)));

        glDisableVertexAttribArray(0);
        glDisableVertexAttribArray(1);
    }
    mProgram2SidesMultipleLights->release();

    // Plane
    mFuncs->glBindVertexArray(mVAOPlane);

    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);

    mProgram2SidesMultipleLights->bind();
    {
        setLights();

        mProgram2SidesMultipleLights->setUniformValue("Material.Kd", 0.1f, 0.1f, 0.1f);
        mProgram2SidesMultipleLights->setUniformValue("Material.Ks", 0.9f, 0.9f, 0.9f);
        mProgram2SidesMultipleLights->setUniformValue("Material.Ka", 0.1f, 0.1f, 0.1f);
        mProgram2SidesMultipleLights->setUniformValue("Material.Shininess", 180.0f);

        QMatrix4x4 mv1 = ViewMatrix * ModelMatrixPlane;
        mProgram2SidesMultipleLights->setUniformValue("ModelViewMatrix", mv1);
        mProgram2SidesMultipleLights->setUniformValue("NormalMatrix", mv1.normalMatrix());
        mProgram2SidesMultipleLights->setUniformValue("MVP", ProjectionMatrix * mv1);

        glDrawElements(GL_TRIANGLES, 6 * mPlane->getnFaces(), GL_UNSIGNED_INT, ((GLubyte *)NULL + (0)));

        glDisableVertexAttribArray(0);
        glDisableVertexAttribArray(1);
    }
    mProgram2SidesMultipleLights->release();

    mContext->swapBuffers(this);
}

void MyWindow::initShaders()
{
    QOpenGLShader vShader(QOpenGLShader::Vertex);
    QOpenGLShader fShader(QOpenGLShader::Fragment);    
    QFile         shaderFile;
    QByteArray    shaderSource;

    //Simple ADS
    shaderFile.setFileName(":/vshader_2sides_multiplelights.txt");
    shaderFile.open(QIODevice::ReadOnly);
    shaderSource = shaderFile.readAll();
    shaderFile.close();
    qDebug() << "vertex 2sides_multiplelights compile: " << vShader.compileSourceCode(shaderSource);

    shaderFile.setFileName(":/fshader_2sides_multiplelights.txt");
    shaderFile.open(QIODevice::ReadOnly);
    shaderSource = shaderFile.readAll();
    shaderFile.close();
    qDebug() << "frag   2sides_multiplelights compile: " << fShader.compileSourceCode(shaderSource);

    mProgram2SidesMultipleLights = new (QOpenGLShaderProgram);
    mProgram2SidesMultipleLights->addShader(&vShader);
    mProgram2SidesMultipleLights->addShader(&fShader);
    qDebug() << "shader link 2sides_multiplelights: " << mProgram2SidesMultipleLights->link();
}

void MyWindow::PrepareTexture(GLenum TextureTarget, const QString& FileName, GLuint& TexObject, bool flip)
{
    QImage TexImg;

    if (!TexImg.load(FileName)) qDebug() << "Erreur chargement texture";
    if (flip==true) TexImg=TexImg.mirrored();

    glGenTextures(1, &TexObject);
    glBindTexture(TextureTarget, TexObject);
    glTexImage2D(TextureTarget, 0, GL_RGB, TexImg.width(), TexImg.height(), 0, GL_BGRA, GL_UNSIGNED_BYTE, TexImg.bits());
    glTexParameterf(TextureTarget, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameterf(TextureTarget, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
}

void MyWindow::keyPressEvent(QKeyEvent *keyEvent)
{
    switch(keyEvent->key())
    {
        case Qt::Key_P:
            break;
        case Qt::Key_Up:
            break;
        case Qt::Key_Down:
            break;
        case Qt::Key_Left:
            break;
        case Qt::Key_Right:
            break;
        case Qt::Key_Delete:
            break;
        case Qt::Key_PageDown:
            break;
        case Qt::Key_Home:
            break;
        case Qt::Key_Z:
            break;
        case Qt::Key_Q:
            break;
        case Qt::Key_S:
            break;
        case Qt::Key_D:
            break;
        case Qt::Key_A:
            break;
        case Qt::Key_E:
            break;
        default:
            break;
    }
}

void MyWindow::printMatrix(const QMatrix4x4& mat)
{
    const float *locMat = mat.transposed().constData();

    for (int i=0; i<4; i++)
    {
        qDebug() << locMat[i*4] << " " << locMat[i*4+1] << " " << locMat[i*4+2] << " " << locMat[i*4+3];
    }
}
