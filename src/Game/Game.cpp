#include "Game.hpp"
#include <sstream>
#include <iterator>
#include <GL/gl.h>
#include <GL/glu.h>
#include "../Graphics/Image/PixelTraits.hpp"
#include "../Graphics/Image/PixelBufferTraits.hpp"
WARN_GUARD_ON
#include "../Graphics/Image/Png/Png.hpp"
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <png++/png.hpp>
WARN_GUARD_OFF
#include "Cube.hpp"

const GLchar* vertexShader = R"rsd(

#version 330

layout(location=0) in vec3 position;
layout(location=1) in vec3 color;
layout(location=2) in vec2 texUV;

out vec3 vertexColor;
out vec2 texCoords;

uniform mat4 MVP;

void main(void)
{
    texCoords = texUV;
    vertexColor = color;
    gl_Position = MVP * vec4(position, 1.0f);
}

)rsd";

const GLchar* fragmentShader = R"rsd(

#version 330

in vec3 vertexColor;
in vec2 texCoords;

out vec4 color;

uniform sampler2D tex;

void main(void)
{
    color = texture(tex, texCoords) * vec4(vertexColor, 1);
}

)rsd";

///==============================================================
///= FileSystem Helpers
///==============================================================
template <typename Buffer>
std::unique_ptr<Buffer> FileLoad(const std::string& file)
{
    // Open file
    std::ifstream ifs(file, std::ios::binary);
    if (!ifs.good())
        return std::unique_ptr<Buffer>(nullptr);

    // Stop the istream_iterator from eating newlines
    ifs.unsetf(std::ios::skipws);

    // Calculate total filesize
    std::streampos sz;
    ifs.seekg(0, std::ios::end);
    sz = ifs.tellg();

    // Rewind file pointer
    ifs.seekg(0, std::ios::beg);

    // Create buffer with preallocated size
    Buffer buf(static_cast<std::size_t>(sz));

    // Read the data into the buffer
    buf.assign(std::istream_iterator<typename Buffer::value_type>(ifs),
               std::istream_iterator<typename Buffer::value_type>());

    return std::make_unique<Buffer>(std::move(buf));
}

///==============================================================
///= GL Helpers
///==============================================================
static void CompileShader(GLuint shaderId)
{
    glCompileShader(shaderId);

    GLint compileStatus;
    glGetShaderiv(shaderId, GL_COMPILE_STATUS, &compileStatus);
    if (compileStatus == GL_FALSE)
    {
        GLint logLength;
        glGetShaderiv(shaderId, GL_INFO_LOG_LENGTH, &logLength);
        if (logLength != 0)
        {
            std::vector<GLchar> buf(logLength, 0);
            glGetShaderInfoLog(shaderId, logLength, 0, buf.data());
            //Error("Shader Compilation Error!", buf.data());
        }
    }
}

static void LinkProgram(GLuint programId)
{
    glLinkProgram(programId);

    GLint linkStatus;
    glGetProgramiv(programId, GL_LINK_STATUS, &linkStatus);
    if (linkStatus == GL_FALSE)
    {
        GLint logLength;
        glGetShaderiv(programId, GL_INFO_LOG_LENGTH, &logLength);
        if (logLength != 0)
        {
            std::vector<GLchar> buf(logLength, 0);
            glGetShaderInfoLog(programId, logLength, 0, buf.data());
            //Error("GL Program Link Error!", buf.data());
        }
    }
}

template <typename PixelBuffer>
void SetTextureData(PixelBuffer pb)
{
    GLint format = PixelTraits<typename PixelBufferTraits<PixelBuffer>::Pixel>::GetChannels() == 4 ? GL_RGBA : GL_RGB;
    uint32_t width = PixelBufferTraits<PixelBuffer>::Width(pb);
    uint32_t height = PixelBufferTraits<PixelBuffer>::Height(pb);
    const GLvoid* data = PixelBufferTraits<PixelBuffer>::GetData(pb);

    glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
}

static void CheckGLError()
{
    GLenum errVal = glGetError();
    if (errVal != GL_NO_ERROR)
    {
        std::stringstream ss;
        ss << "OpenGL Error! Code: " << errVal;
        const char* desc = reinterpret_cast<const char*>(gluErrorString(errVal));
        (void) desc;
        //Error(ss.str().c_str(), desc);
    }
}

///==============================================================
///= Model
///==============================================================
Game::Model::Model()
{
    glGenVertexArrays(1, &mVao);
    glGenBuffers(1, &mVbo);
    glGenBuffers(1, &mColBuf);
    glGenBuffers(1, &mTexBuf);
    glGenBuffers(1, &mEbo);
}

Game::Model::~Model()
{
    glDeleteBuffers(1, &mEbo);
    glDeleteBuffers(1, &mTexBuf);
    glDeleteBuffers(1, &mColBuf);
    glDeleteBuffers(1, &mVbo);
    glDeleteVertexArrays(1, &mVao);
}

void Game::Model::Load(const Game::ModelData& data)
{
    glBindVertexArray(mVao);
    {
        glBindBuffer(GL_ARRAY_BUFFER, mVbo);
        {
            glBufferData(GL_ARRAY_BUFFER,
                data.vertices.size() * sizeof(GLfloat),
                data.vertices.data(),
                GL_STATIC_DRAW
            );
            GLuint index = 0;
            glVertexAttribPointer(index, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
            glEnableVertexAttribArray(index);
        }
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        glBindBuffer(GL_ARRAY_BUFFER, mColBuf);
        {
            glBufferData(GL_ARRAY_BUFFER,
                data.colors.size() * sizeof(GLfloat),
                data.colors.data(),
                GL_STATIC_DRAW
            );
            GLuint index = 1;
            glVertexAttribPointer(index, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
            glEnableVertexAttribArray(index);
        }
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        glBindBuffer(GL_ARRAY_BUFFER, mTexBuf);
        {
            glBufferData(GL_ARRAY_BUFFER,
                data.texCoords.size() * sizeof(GLfloat),
                data.texCoords.data(),
                GL_STATIC_DRAW
            );
            GLuint index = 2;
            glVertexAttribPointer(index, 2, GL_FLOAT, GL_FALSE, 0, nullptr);
            glEnableVertexAttribArray(index);
        }
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mEbo);
        {
            glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                data.indices.size() * sizeof(GLuint),
                data.indices.data(),
                GL_STATIC_DRAW
            );
        }
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    }
    glBindVertexArray(0);
}

GLuint Game::Model::VaoId() const
{
    return mVao;
}

GLuint Game::Model::EboId() const
{
    return mEbo;
}

///==============================================================
///= Game
///==============================================================
Game::Game()
{
}

void Game::Init()
{
    mWindow.Create(800, 600, "TheRoom", Window::Mode::Windowed);
    mWindow.SetShowFPS(true);
    mWindow.SetCloseHandler(mExitHandler);
    mWindow.SetMouseButtonPressHandler(
        [this](MouseButton mb, ButtonAction ba)
        {
            if (mb == MouseButton::Left && ba == ButtonAction::Press)
                mWindow.SetMouseGrabEnabled(true);
        }
    );
    mWindow.SetKeyPressedHandler(
        [this](Key k, KeyAction ka)
        {
            // Exit
            if(k == Key::Escape && ka == KeyAction::Release)
                mExitHandler();
            // Ungrab mouse
            if(k == Key::RightControl && ka == KeyAction::Release)
                mWindow.SetMouseGrabEnabled(false);
            // Toggle polygon mode
            if(k == Key::P && ka == KeyAction::Release)
            {
                if (mGLData.drawMode == GL_FILL)
                    mGLData.drawMode = GL_POINT;
                else if (mGLData.drawMode == GL_POINT)
                    mGLData.drawMode = GL_LINE;
                else if (mGLData.drawMode == GL_LINE)
                    mGLData.drawMode = GL_FILL;
            }
        }
    );

    mRenderData.degrees = 0.1f;
    mRenderData.degreesInc = 0.05f;

    mCamera.SetPos(glm::vec3(0, 0, 8));

    // Load the Cube
    ModelData cubeData;
    cubeData.vertices.assign(std::begin(cubeVertexes), std::end(cubeVertexes));
    cubeData.indices.assign(std::begin(cubeIndices), std::end(cubeIndices));
    cubeData.colors.assign(std::begin(cubeColorData), std::end(cubeColorData));
    cubeData.texCoords.assign(std::begin(cubeTextureUVMappings), std::end(cubeTextureUVMappings));

    std::unique_ptr<Model> cube = std::make_unique<Model>();
    Model* cubePtr = cube.get();
    cube->Load(cubeData);
    mModelStore["cube"] = std::move(cube);

    // Create various Cube instances in the world
    std::vector<glm::vec3> cubePositions = {
        glm::vec3(0.0f, 0.0f, 0.0f),
        glm::vec3(4.0f, 10.0f, -20.0f),
        glm::vec3(-3.0f, -4.4f, -5.0f),
        glm::vec3(-7.6f, -4.0f, -14.0f),
        glm::vec3(4.4f, -3.5f, -4.0f),
        glm::vec3(-3.4f, 6.0f, -15.0f),
        glm::vec3(2.6f, -4.0f, -17.0f),
        glm::vec3(4.0f, 3.0f, -5.0f),
        glm::vec3(3.0f, 0.4f, -12.0f),
        glm::vec3(-3.5f, 2.0f, -3.0f)
    };
    for (const auto& pos : cubePositions)
        mWorld.push_back({pos, cubePtr});

    CheckGLError();
    GLInit();
    mGLData.drawMode = GL_FILL;
}

void Game::GLInit()
{
    // Generate the various resources
    glGenTextures(1, &mGLData.tex);
    mGLData.programId = glCreateProgram();

    // Set the clear color
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

    // Enable the depth buffer
    glEnable(GL_DEPTH_TEST);
    //glEnable(GL_TEXTURE_2D);
    glDepthFunc(GL_LESS);

    // Compile and Link shaders
    GLuint vShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vShader, 1, &vertexShader, 0);
    CompileShader(vShader);

    GLuint fShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fShader, 1, &fragmentShader, 0);
    CompileShader(fShader);

    glAttachShader(mGLData.programId, vShader);
    glAttachShader(mGLData.programId, fShader);
    LinkProgram(mGLData.programId);
    glDeleteShader(vShader);
    glDeleteShader(fShader);

    glUseProgram(mGLData.programId);

    // Load the sample texture
    glActiveTexture(GL_TEXTURE0);
    png::image<png::rgba_pixel, png::solid_pixel_buffer<png::rgba_pixel>> img("ext/tree.png");
    auto pb = img.get_pixbuf();
    glBindTexture(GL_TEXTURE_2D, mGLData.tex);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    SetTextureData(pb);
    glGenerateMipmap(GL_TEXTURE_2D);

    // Pass it to OpenGL
    GLuint samplerId = glGetUniformLocation(mGLData.programId, "tex");
    glUniform1i(samplerId, 0);

    mGLData.matrixId = glGetUniformLocation(mGLData.programId, "MVP");
    CheckGLError();
}

std::vector<Camera::MoveDirection> Game::CameraMoveDirections()
{
    std::vector<Camera::MoveDirection> mds;
    if(mWindow.IsKeyPressed(Key::W))
        mds.push_back(Camera::MoveDirection::Forward);
    if(mWindow.IsKeyPressed(Key::A))
        mds.push_back(Camera::MoveDirection::Left);
    if(mWindow.IsKeyPressed(Key::S))
        mds.push_back(Camera::MoveDirection::BackWard);
    if(mWindow.IsKeyPressed(Key::D))
        mds.push_back(Camera::MoveDirection::Right);
    return mds;
}

std::tuple<float, float> Game::CameraLookOffset()
{
    std::tuple<double, double> curDiff = mWindow.GetCursorDiff();
    return std::make_tuple(
        static_cast<float>(std::get<0>(curDiff)),
        static_cast<float>(std::get<1>(curDiff))
    );
}

void Game::Update(float dt)
{
    (void) dt;

    // Poll window events
    mWindow.Update();

    // Update camera euler angles
    if (mWindow.MouseGrabEnabled())
        mCamera.Look(CameraLookOffset());

    mCamera.Move(CameraMoveDirections());

    // Update state
    mRenderData.degrees += mRenderData.degreesInc;
}

void Game::Render(float interpolation)
{
    // Clear color
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Set the polygon mode
    glPolygonMode(GL_FRONT_AND_BACK, mGLData.drawMode);

    // Create the projection matrix
    glm::mat4 projection = glm::perspective(45.0f, 4.0f / 3.0f, 0.1f, 100.0f);

    // View calculation with camera
    auto lookOffset = mWindow.MouseGrabEnabled() ? CameraLookOffset() : std::make_tuple(0.0f, 0.0f);
    auto iCamState = mCamera.Interpolate(CameraMoveDirections(), lookOffset, interpolation);
    glm::vec3& cameraPos = iCamState.position;
    glm::vec3& cameraFront = iCamState.front;
    glm::vec3& cameraUp = iCamState.up;

    // Create the view matrix
    glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);

    for (std::size_t i = 0; i < mWorld.size(); ++i)
    {
        const auto& gObj = mWorld[i];

        // Calculate the model matrix
        glm::mat4 model;
        model = glm::translate(model, gObj.position);
        model = glm::rotate(model, mRenderData.degrees + mRenderData.degreesInc * interpolation, glm::vec3(0, 1, 0));
        model = glm::rotate(model, 20.0f * i, glm::vec3(1.0f, 0.3f, 0.5f));

        // Combine the projection, view and model matrices
        glm::mat4 MVP = projection * view * model;
        // Upload the combined matrix as a uniform
        glUniformMatrix4fv(mGLData.matrixId, 1, GL_FALSE, glm::value_ptr(MVP));

        // Draw the object
        glBindVertexArray(gObj.model->VaoId());
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gObj.model->EboId());
        glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
    }

    // Check for errors
    CheckGLError();

    // Show it
    mWindow.SwapBuffers();
}

void Game::Shutdown()
{
    // OpenGL
    glUseProgram(0);

    glDeleteTextures(1, &mGLData.tex);
    glDeleteProgram(mGLData.programId);

    // Window
    mWindow.Destroy();
}

void Game::SetExitHandler(std::function<void()> f)
{
    mExitHandler = f;
}

