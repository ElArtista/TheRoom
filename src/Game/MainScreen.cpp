#include "MainScreen.hpp"
#include "../Util/WarnGuard.hpp"
WARN_GUARD_ON
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "../Asset/Scene/SceneLoader.hpp"
WARN_GUARD_OFF
#include "../Util/FileLoad.hpp"
#include "../Asset/Image/ImageLoader.hpp"
#include "../Graphics/Scene/SceneFactory.hpp"

// BufferType for the files loaded
using BufferType = std::vector<std::uint8_t>;

void MainScreen::onInit(ScreenContext& sc)
{
    // Store engine ref
    mEngine = sc.GetEngine();

    // Store file data cache ref
    mFileDataCache = sc.GetFileDataCache();

    // Cube rotation state
    mRotationData.degreesInc = 0.05f;
    mRotationData.rotating = false;

    // Camera initial position
    mCamera.SetPos(glm::vec3(0, 0, 8));

    // Add sample UV Sphere
    ModelData sphereModel = GenUVSphere(1, 32, 32);
    mEngine->GetModelStore().Load("4", std::move(sphereModel));

    // Create world objects
    SetupWorld();

    // Create world lights
    SetupLights();

    // Cam shouldn't follow character initially
    mFollowingCharacter = false;

    // Initial choosen moving light
    mMovingLightIndex = 0;

    // Init character
    mCharacter.Init(&mEngine->GetWindow(), mScene.get());

    // Load the skybox
    mSkybox = std::make_unique<Skybox>();
    ImageLoader imLoader;
    mSkybox->Load(
        {
            { Cubemap::Target::Right,  imLoader.Load(*(*mFileDataCache)["ext/Assets/Textures/Skybox/Bluesky/right.jpg"], "jpg") },
            { Cubemap::Target::Left,   imLoader.Load(*(*mFileDataCache)["ext/Assets/Textures/Skybox/Bluesky/left.jpg"],  "jpg") },
            { Cubemap::Target::Top,    imLoader.Load(*(*mFileDataCache)["ext/Assets/Textures/Skybox/Bluesky/top.jpg"],   "jpg") },
            { Cubemap::Target::Bottom, imLoader.Load(*(*mFileDataCache)["ext/Assets/Textures/Skybox/Bluesky/bottom.jpg"],"jpg") },
            { Cubemap::Target::Back,   imLoader.Load(*(*mFileDataCache)["ext/Assets/Textures/Skybox/Bluesky/back.jpg"],  "jpg") },
            { Cubemap::Target::Front,  imLoader.Load(*(*mFileDataCache)["ext/Assets/Textures/Skybox/Bluesky/front.jpg"], "jpg") }
        }
    );

    // Load the irr map
    mIrrMap = std::make_unique<Skybox>();
    mIrrMap->Load(
        {
            { Cubemap::Target::Right,  imLoader.Load(*(*mFileDataCache)["ext/Assets/Textures/Skybox/Bluesky/Irradiance/right.jpg"], "jpg") },
            { Cubemap::Target::Left,   imLoader.Load(*(*mFileDataCache)["ext/Assets/Textures/Skybox/Bluesky/Irradiance/left.jpg"],  "jpg") },
            { Cubemap::Target::Top,    imLoader.Load(*(*mFileDataCache)["ext/Assets/Textures/Skybox/Bluesky/Irradiance/top.jpg"],   "jpg") },
            { Cubemap::Target::Bottom, imLoader.Load(*(*mFileDataCache)["ext/Assets/Textures/Skybox/Bluesky/Irradiance/bottom.jpg"],"jpg") },
            { Cubemap::Target::Back,   imLoader.Load(*(*mFileDataCache)["ext/Assets/Textures/Skybox/Bluesky/Irradiance/back.jpg"],  "jpg") },
            { Cubemap::Target::Front,  imLoader.Load(*(*mFileDataCache)["ext/Assets/Textures/Skybox/Bluesky/Irradiance/front.jpg"], "jpg") }
        }
    );

    // Load the rad map
    mRadMap = std::make_unique<Skybox>();
    for (unsigned int i = 0; i < 7; ++i) {
        std::string pathPrefix = "ext/Assets/Textures/Skybox/Bluesky/Radiance/" + std::to_string(i);
        mRadMap->Load(
            {
                { Cubemap::Target::Right,  imLoader.Load(*(*mFileDataCache)[pathPrefix + "/right.jpg"], "jpg") },
                { Cubemap::Target::Left,   imLoader.Load(*(*mFileDataCache)[pathPrefix + "/left.jpg"],  "jpg") },
                { Cubemap::Target::Top,    imLoader.Load(*(*mFileDataCache)[pathPrefix + "/top.jpg"],   "jpg") },
                { Cubemap::Target::Bottom, imLoader.Load(*(*mFileDataCache)[pathPrefix + "/bottom.jpg"],"jpg") },
                { Cubemap::Target::Back,   imLoader.Load(*(*mFileDataCache)[pathPrefix + "/back.jpg"],  "jpg") },
                { Cubemap::Target::Front,  imLoader.Load(*(*mFileDataCache)[pathPrefix + "/front.jpg"], "jpg") }
            }
            , i
        );
    }

    // Do not show AABBs by default
    mShowAABBs = false;

    // Do not show Debug info by default
    mShowDbgInfo = false;

    // Init renderform creator
    mRenderformCreator = std::make_unique<RenderformCreator>(&(mEngine->GetModelStore()), &(mEngine->GetMaterialStore()));
}

void MainScreen::SetupWorld()
{
    // Load sample scene file
    std::string sceneFile = "res/Scenes/main_scene.json";
    auto sceneFileData = FileLoad<BufferType>(sceneFile);
    if(!sceneFileData)
        throw std::runtime_error("Couldn't load file (" + sceneFile + ")");
    SceneLoader sceneLoader;
    SceneFile sf = sceneLoader.Load(*sceneFileData);

    SceneFactory factory(
        &mEngine->GetTextureStore(),
        &mEngine->GetModelStore(),
        &mEngine->GetMaterialStore(),
        mFileDataCache);
    mScene = std::move(factory.CreateFromSceneFile(sf));

    // Set positions for cubes
    SceneNode* node;
    std::vector<glm::vec3> cubePositions = {
        glm::vec3( 0.0f,  0.0f,    0.0f),
        glm::vec3( 4.0f, 10.0f,  -20.0f),
        glm::vec3(-3.0f, -4.4f,  - 5.0f),
        glm::vec3(-7.6f, -4.0f,  -14.0f),
        glm::vec3( 4.4f, -3.5f,  - 4.0f),
        glm::vec3(-3.4f,  6.0f,  -15.0f),
        glm::vec3( 2.6f, -4.0f,  -17.0f),
        glm::vec3( 4.0f,  3.0f,  - 5.0f),
        glm::vec3( 3.0f,  0.4f,  -12.0f),
        glm::vec3(-3.5f,  2.0f,  - 3.0f)
    };
    for(std::size_t i = 0; i < cubePositions.size(); ++i)
    {
        const auto& pos = cubePositions[i];

        const std::string name = "cube" + std::to_string(i);
        node = mScene->FindNodeByUuid(name);
        mScene->Move(node, pos);
        mScene->Scale(node, glm::vec3(2.0f));
        mScene->Rotate(node, RotationAxis::X, 20.0f * i);
        mScene->Rotate(node, RotationAxis::Y, 7.0f * i);
        mScene->Rotate(node, RotationAxis::Z, 10.0f * i);
    }
}

void UpdateLight(Renderer& renderer, Scene* const scene, int index, const glm::vec3& move)
{
    auto& lights = scene->GetLights();

    // Get light
    SceneNode* curLight = lights[index];

    // Move light in scene
    scene->Move(curLight, move);

    // Move light in renderer
    auto trans = curLight->GetTransformation().GetInterpolated(1.0f);
    renderer.GetLights().pointLights[index].position = glm::vec3(trans[3].x, trans[3].y, trans[3].z);
};

void UpdateDirectionalLight(Renderer& renderer, const glm::vec3& move) {
    renderer.GetLights().dirLights[0].direction += move;
}

void MainScreen::SetupLights()
{
    // Setup scene lights
    Lights& lights = mEngine->GetRenderer().GetLights();

    // Add directional light
    DirLight dirLight;
    dirLight.direction = glm::vec3(-0.3f, -0.5f, -0.5f);
    dirLight.color = glm::vec3(0.9f);
    lights.dirLights.push_back(dirLight);

    // Add point lights
    for (int i = 0; i < 2; ++i)
    {
        PointLight pointLight;
        pointLight.color = glm::vec3(0.8f, 0.8f, 0.8f);
        pointLight.attProps.constant   = 1.0f;
        pointLight.attProps.linear     = 0.09f;
        pointLight.attProps.quadratic  = 0.032f;
        lights.pointLights.push_back(pointLight);
    }

    // Update lights once to take their initial position
    for (int i = 0; i < 2; i++)
        UpdateLight(mEngine->GetRenderer(), mScene.get(), i, glm::vec3(0));
}

std::vector<Camera::MoveDirection> MainScreen::CameraMoveDirections()
{
    auto& window = mEngine->GetWindow();
    std::vector<Camera::MoveDirection> mds;
    if(window.IsKeyPressed(Key::W))
        mds.push_back(Camera::MoveDirection::Forward);
    if(window.IsKeyPressed(Key::A))
        mds.push_back(Camera::MoveDirection::Left);
    if(window.IsKeyPressed(Key::S))
        mds.push_back(Camera::MoveDirection::BackWard);
    if(window.IsKeyPressed(Key::D))
        mds.push_back(Camera::MoveDirection::Right);
    return mds;
}

std::tuple<float, float> MainScreen::CameraLookOffset()
{
    auto& window = mEngine->GetWindow();
    std::tuple<double, double> curDiff = window.GetCursorDiff();
    return std::make_tuple(
        static_cast<float>(std::get<0>(curDiff)),
        static_cast<float>(std::get<1>(curDiff))
    );
}

void MainScreen::onKey(Key k, KeyAction ka)
{
    if(k == Key::B && ka == KeyAction::Release)
        mShowAABBs = !mShowAABBs;
    if(k == Key::F12 && ka == KeyAction::Release)
        mShowDbgInfo = !mShowDbgInfo;
    if(k == Key::R && ka == KeyAction::Release)
        mRotationData.rotating = !mRotationData.rotating;
    if(k == Key::F && ka == KeyAction::Release)
        mFollowingCharacter = !mFollowingCharacter;
    if(k == Key::L && ka == KeyAction::Release)
    {
        if (mMovingLightIndex == 0)
            mMovingLightIndex = 1;
        else if (mMovingLightIndex == 1)
            mMovingLightIndex = 0;
    }
    if(k == Key::F2 && ka == KeyAction::Release)
        mOnNextScreenCb();
}

void MainScreen::onUpdate(float dt)
{
    (void) dt;
    // Update the core
    mEngine->Update(dt);

    auto& scene = mScene;
    auto& window = mEngine->GetWindow();
    auto& renderer = mEngine->GetRenderer();

    // Update interpolation variables
    for (auto& obj : mScene->GetNodes())
    {
        Transform& trans = obj.second->GetTransformation();
        trans.Update();

        AABB& aabb = obj.second->GetAABB();
        aabb.Update(trans.GetPosition(), trans.GetScale(), trans.GetRotation());
    }

    // Update camera euler angles
    if (window.MouseGrabEnabled())
        mCamera.Look(CameraLookOffset());

    if (mFollowingCharacter)
    {
        // Update character
        mCharacter.Update();

        // Move Camera following character
        auto trans = mCharacter.GetCharacterNode()->GetTransformation().GetInterpolated(1.0f);
        mCamera.SetPos(glm::vec3(trans[3].x, trans[3].y + 4, trans[3].z + 4));
    }
    else
    {
        // Update camera position
        mCamera.Move(CameraMoveDirections());
    }

    // Update the camera matrix
    mCamera.Update();

    // Update light position
    //auto updLight = std::bind(updateLight, renderer, mScene.get(), std::placeholders::_1);
    auto updLight = [this, &renderer, &scene](const glm::vec3& move)
    {
        UpdateLight(renderer, scene.get(), mMovingLightIndex, move);
    };

    float increase = 0.3f;
    if(window.IsKeyPressed(Key::Kp8))
        updLight(glm::vec3(0.0f, increase, 0.0f));
    if(window.IsKeyPressed(Key::Kp4))
        updLight(glm::vec3(-increase, 0.0f, 0.0f));
    if(window.IsKeyPressed(Key::Kp2))
        updLight(glm::vec3(0.0f, -increase, 0.0f));
    if(window.IsKeyPressed(Key::Kp6))
        updLight(glm::vec3(increase, 0.0f, 0.0f));
    if(window.IsKeyPressed(Key::Kp5))
        updLight(glm::vec3(0.0f, 0.0f, -increase));
    if(window.IsKeyPressed(Key::Kp0))
        updLight(glm::vec3(0.0f, 0.0f, increase));

    // Update cubes' rotations
    if (mRotationData.rotating)
    {
        for(auto& p : scene->GetNodes())
        {
            if (p.first.substr(0, 4) == "cube")
                scene->Rotate(p.first, RotationAxis::Y, mRotationData.degreesInc);
        }
    }

    // Update dir light
    increase = 0.3f;
    if (window.IsKeyPressed(Key::Right))
    {
        if (window.IsKeyPressed(Key::LeftShift))
            UpdateDirectionalLight(renderer, glm::vec3(0.0f, 0.0f, increase));
        else
            UpdateDirectionalLight(renderer, glm::vec3(increase, 0.0f, 0.0f));
    }
    if (window.IsKeyPressed(Key::Left))
    {
        if (window.IsKeyPressed(Key::LeftShift))
            UpdateDirectionalLight(renderer, glm::vec3(0.0f, 0.0f, -increase));
        else
            UpdateDirectionalLight(renderer, glm::vec3(-increase, 0.0f, 0.0f));
    }
    if (window.IsKeyPressed(Key::Up))
        UpdateDirectionalLight(renderer, glm::vec3(0.0f, increase, 0.0f));
    if (window.IsKeyPressed(Key::Down))
        UpdateDirectionalLight(renderer, glm::vec3(0.0f, -increase, 0.0f));

    // Update physics
    UpdatePhysics(dt);
}

void MainScreen::UpdatePhysics(float dt)
{
    (void) dt;

    auto& scene = mScene;
    SceneNode* teapot = mScene->FindNodeByUuid("teapot");
    for(auto& p : scene->GetNodes())
    {
        SceneNode* cur = p.second.get();
        if(cur->GetUUID() == teapot->GetUUID())
            continue;

        if(Intersects(teapot->GetAABB(), cur->GetAABB()))
            scene->Move(teapot, CalcCollisionResponce(teapot->GetAABB(), cur->GetAABB()));
    }
}

void MainScreen::onRender(float interpolation)
{
    // Get the view matrix and pass it to the renderer
    glm::mat4 view = mCamera.InterpolatedView(interpolation);

    // Update render form
    mRenderformCreator->Update(mScene->PullUpdates());

    // Convert render form to int form
    auto intForm = bakeIntForm(*mRenderformCreator);

    // Add skybox and irrMap id to intform
    intForm.skyboxId = mSkybox->GetCubemap()->Id();
    intForm.irrMapId = mIrrMap->GetCubemap()->Id();
    intForm.radMapId = mRadMap->GetCubemap()->Id();

    // Render
    mEngine->GetRenderer().SetView(view);
    mEngine->GetRenderer().Render(interpolation, intForm);

    // Render the AABBs if enabled
    if (mShowAABBs)
    {
        AABBRenderer& aabbRenderer = mEngine->GetAABBRenderer();
        aabbRenderer.SetView(view);
        aabbRenderer.SetScene(mScene.get());
        aabbRenderer.Render(interpolation);
    }

    // Render the skybox
    if (mSkybox)
        mSkybox->Render(mEngine->GetRenderer().GetProjection(), view);

    // Render the debug info if enabled
    if (mShowDbgInfo)
    {
        DebugRenderer& dbgRenderer = mEngine->GetDebugRenderer();
        dbgRenderer.Render(interpolation);
    }

    // Render sample text
    mEngine->GetTextRenderer().RenderText("ScaryBox Studios", 10, 10, glm::vec3(1.0f, 0.5f, 0.3f), "visitor");
}

void MainScreen::onShutdown()
{
    // Remove lights
    mEngine->GetRenderer().GetLights().pointLights.clear();
    mEngine->GetRenderer().GetLights().dirLights.clear();

    // Clear previous stores
    mEngine->GetModelStore().Clear();
    mEngine->GetMaterialStore().Clear();
    mEngine->GetTextureStore().Clear();
}

void MainScreen::SetOnNextScreenCb(OnNextScreenCb cb)
{
    mOnNextScreenCb = cb;
}
