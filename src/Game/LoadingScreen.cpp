#include "LoadingScreen.hpp"
#include <thread>
WARN_GUARD_ON
#include "../Graphics/Image/ImageLoader.hpp"
#include "../Graphics/Geometry/ModelLoader.hpp"
WARN_GUARD_OFF
#include "../Util/FileLoad.hpp"

void LoadingScreen::onInit(ScreenContext& sc)
{
    // Store the engine reference
    mEngine = sc.GetEngine();

    // Fire loader thread
    mFileCacheIsReady = false;
    std::thread loaderThread(
        [this]()
        {
            LoadFileData();
            mFileCacheIsReady = true;
        }
    );
    loaderThread.detach();
}

void LoadingScreen::LoadFromMem()
{
    // Load the textures
    LoadTextures();

    // Load the models
    LoadModels();

    // Load font
    mEngine->GetRenderer().GetTextRenderer().GetFontStore().LoadFont("visitor", "ext/visitor.ttf");

    // Load the skybox
    auto skybox = std::make_unique<Skybox>();
    ImageLoader imLoader;
    skybox->Load(
        {
            { Skybox::Target::Right,  imLoader.Load(*mFileDataCache["ext/Skybox/right.jpg"], "jpg") },
            { Skybox::Target::Left,   imLoader.Load(*mFileDataCache["ext/Skybox/left.jpg"],  "jpg") },
            { Skybox::Target::Top,    imLoader.Load(*mFileDataCache["ext/Skybox/top.jpg"],   "jpg") },
            { Skybox::Target::Bottom, imLoader.Load(*mFileDataCache["ext/Skybox/bottom.jpg"],"jpg") },
            { Skybox::Target::Back,   imLoader.Load(*mFileDataCache["ext/Skybox/back.jpg"],  "jpg") },
            { Skybox::Target::Front,  imLoader.Load(*mFileDataCache["ext/Skybox/front.jpg"], "jpg") }
        }
    );
    mEngine->SetSkybox(std::move(skybox));

    // Call finish
    mFinishCb();
}

void LoadingScreen::LoadFileData()
{
    std::vector<std::string> fileList =
    {
      // Skybox
      "ext/Skybox/right.jpg"
    , "ext/Skybox/left.jpg"
    , "ext/Skybox/top.jpg"
    , "ext/Skybox/bottom.jpg"
    , "ext/Skybox/back.jpg"
    , "ext/Skybox/front.jpg"
      // Textures
    , "ext/mahogany_wood.jpg"
    , "ext/mahogany_wood_spec.jpg"
    , "ext/WoodenCabin/WoodCabinDif.jpg"
    , "ext/WoodenCabin/WoodCabinSM.jpg"
    , "ext/Dungeon/maps/Wall1_T.tga"
    , "ext/Dungeon/maps/Wall1_B.tga"
      // Models
    , "ext/Cube/cube.obj"
    , "ext/teapot.obj"
    , "ext/WoodenCabin/WoodenCabin.dae"
    , "ext/Dungeon/Well.obj"
    };

    for (const auto& file : fileList)
    {
        mFileDataCache.emplace(file, FileLoad<BufferType>(file));
        if(!mFileDataCache[file])
            throw std::runtime_error("Couldn't load file (" + file + ")");
    }
};

void LoadingScreen::LoadTextures()
{
    // Retrieve the textureStore from the renderer
    auto& textureStore = mEngine->GetRenderer().GetTextureStore();

    // The ImageLoader object
    ImageLoader imageLoader;

    // The texture map
    std::unordered_map<std::string, std::string> textures =
    {
        {"ext/mahogany_wood.jpg",            "mahogany_wood"},
        {"ext/mahogany_wood_spec.jpg",       "mahogany_wood_spec"},
        {"ext/WoodenCabin/WoodCabinDif.jpg", "house_diff"},
        {"ext/WoodenCabin/WoodCabinSM.jpg",  "house_spec"},
        {"ext/Dungeon/maps/Wall1_T.tga",     "well_diff"},
        {"ext/Dungeon/maps/Wall1_B.tga",     "well_spec"}
    };

    // Load the textures
    for (const auto& p : textures)
    {
        std::string ext = p.first.substr(p.first.find_last_of(".") + 1);
        RawImage<> pb = imageLoader.Load(*mFileDataCache[p.first], ext);
        textureStore.Load(p.second, pb);
    }
}

void LoadingScreen::LoadModels()
{
    // Retrieve the model and texture stores from the renderer
    auto& renderer = mEngine->GetRenderer();
    auto& modelStore = renderer.GetModelStore();
    auto& textureStore = renderer.GetTextureStore();

    // Model loader instance
    ModelLoader modelLoader;

    struct MdlData
    {
        std::string filepath;
        std::string extension;
        std::string name;
        Material& material;
    };

    // Create materials
    Material mahogany;
    mahogany.SetDiffuseTexture(textureStore["mahogany_wood"]->texId);
    mahogany.SetSpecularTexture(textureStore["mahogany_wood_spec"]->texId);

    Material white;
    white.SetDiffuseColor(glm::vec3(0xFF));

    Material house;
    house.SetDiffuseTexture(textureStore["house_diff"]->texId);
    house.SetSpecularTexture(textureStore["house_spec"]->texId);

    Material well;
    well.SetDiffuseTexture(textureStore["well_diff"]->texId);
    well.SetSpecularTexture(textureStore["well_spec"]->texId);

    std::vector<MdlData> models = {
        { "ext/Cube/cube.obj",               "obj", "cube",   mahogany } // Cube
    ,   { "ext/teapot.obj",                  "obj", "teapot", white    } // Teapot
    ,   { "ext/WoodenCabin/WoodenCabin.dae", "dae", "house",  house    } // House
    ,   { "ext/Dungeon/Well.obj",            "obj", "well",   well     } // Well
    };

    for(auto& m : models)
    {
        auto& file = mFileDataCache[m.filepath];
        ModelData model = modelLoader.Load(*file, m.extension.c_str());
        if(model.meshes.size() == 0)
            throw std::runtime_error("Couldn't load model (" + m.filepath + ")");

        modelStore.Load(m.name, std::move(model));
        modelStore[m.name]->material = m.material;
    }
}

void LoadingScreen::onUpdate(float dt)
{
    mEngine->Update(dt);

    if (mFileCacheIsReady)
    {
        LoadFromMem();
        mFileCacheIsReady = false;
    }
}

void LoadingScreen::onRender(float interpolation)
{
    (void) interpolation;
}

void LoadingScreen::onShutdown()
{
}

void LoadingScreen::SetFinishCb(FinishCb cb)
{
    mFinishCb = cb;
}
