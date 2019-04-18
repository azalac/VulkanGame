
#include <iostream>
#include <exception>

#include "VulkanController.hpp"

#include "game/scene.hpp"
#include "game/ObjectControllers.hpp"
#include "game/Menu.hpp"

#include <chrono>

const int WIDTH = 1024;
const int HEIGHT = WIDTH * 9 / 16;

const int MENU_BUTTON_LAYER = 5;
const int MENU_BACKGROUND_LAYER = 4;
const int SHIP_LAYER = 3;
const int SHIP_EFFECT_LAYER = 2;
const int PLANET_LAYER = 1;
const int BACKGROUND_LAYER = 0;

const glm::vec2 CELL_SIZE(0.2f, 0.2f), BUTTON_SIZE(0.3f, 0.1f);

const std::string SOUNDS_DIRECTORY = "./sounds/";

struct Color {
    float r, g, b, a;
};

struct ChromaKey {
    struct Color search, replace;
    float epsilon1, epsilon2;
};

struct Events {
    Event mouseclick;
    Event playerstatechange;
    Event playercollide;
    Event playerdamaged, gameover;
    Event menubutton;
    Event deltaenergy;
    Event deltascience;
    Event worldreset;
    Event soundrequest;
    Event keypress;
    Event requestcleanup;

    Events(Scene * scene) :
    mouseclick(scene->createEventId()), playerstatechange(scene->createEventId()),
    playercollide(scene->createEventId()), playerdamaged(scene->createEventId()),
    gameover(scene->createEventId()), menubutton(scene->createEventId()),
    deltaenergy(scene->createEventId()), deltascience(scene->createEventId()),
    worldreset(scene->createEventId()), soundrequest(scene->createEventId()),
    keypress(scene->createEventId()), requestcleanup(scene->createEventId()) {

    }
};

struct MaterialInfo : public MaterialRendererBuilder {
    TextureSamplerPrototype texture;
    MaterialPrototype prototype;
    Material * material = nullptr;

    UBOMap * globals;

    VulkanController * controller;

    MaterialInfo(VulkanController * controller, UBOMap * globals = nullptr) :
    texture(0, nullptr, 1, vk::ShaderStageFlagBits::eFragment) {
        this->globals = globals;
        this->controller = controller;
    }

    void finalize(VulkanController * controller, bool use_texture = true) {
        if (use_texture) {
            prototype.samplers.push_back(texture);
        }
        material = controller->createMaterial(prototype, globals);
    }

    virtual MaterialRenderer * createRenderer(Material * material, VulkanIndexBuffer * indexbuffer = nullptr) override {
        return controller->createRenderer(material, indexbuffer);
    }

};

int run() {
    Window window(WIDTH, HEIGHT, "Vulkan Test");

    InputHandler input(window);

    VulkanController * controller = new VulkanController(window);

    controller->getDevice()->printExtensions();

    glfwSetWindowUserPointer(window.getWindow(), controller);
    glfwSetFramebufferSizeCallback(window.getWindow(), [](GLFWwindow * wnd, int width, int height) {
        VulkanController * controller = (VulkanController*) glfwGetWindowUserPointer(wnd);

        controller->recreateSwapchain();
    });

    // <editor-fold defaultstate="collapsed" desc="Material Setup">

    // green chroma key
    UniformBufferPrototype chromaKeyDescriptor(0, 0, sizeof (struct ChromaKey), 1, vk::ShaderStageFlagBits::eFragment);

    UBOMap globals = Material::createubos(*controller->getDevice(), std::vector<UniformBufferPrototype> {
        chromaKeyDescriptor });

    VulkanBufferView<struct ChromaKey> chromaKeyView(*globals[chromaKeyDescriptor.id]);

    chromaKeyView->search = {0, 1.0f, 0, 1.0f};
    chromaKeyView->replace = {0, 0, 0, 0};
    chromaKeyView->epsilon1 = 0.25f;
    chromaKeyView->epsilon2 = 0.75f;

    // blue chroma key
    UniformBufferPrototype chromaKeyBlueDescriptor(0, 0, sizeof (struct ChromaKey), 1, vk::ShaderStageFlagBits::eFragment);

    UBOMap globals2 = Material::createubos(*controller->getDevice(), std::vector<UniformBufferPrototype> {
        chromaKeyBlueDescriptor });

    VulkanBufferView<struct ChromaKey> chromaKey2View(*globals2[chromaKeyBlueDescriptor.id]);

    chromaKey2View->search = {0, 0, 1.0f, 1.0f};
    chromaKey2View->replace = {0, 0, 0, 0};
    chromaKey2View->epsilon1 = 0.25f;
    chromaKey2View->epsilon2 = 0.75f;

    // black chroma key
    UniformBufferPrototype chromaKeyBlackDescriptor(0, 0, sizeof (struct ChromaKey), 1, vk::ShaderStageFlagBits::eFragment);

    UBOMap globals3 = Material::createubos(*controller->getDevice(), std::vector<UniformBufferPrototype> {
        chromaKeyBlackDescriptor });

    VulkanBufferView<struct ChromaKey> chromaKey3View(*globals3[chromaKeyBlackDescriptor.id]);

    chromaKey3View->search = {0, 0, 0.0f, 1.0f};
    chromaKey3View->replace = {0, 0, 0, 0};
    chromaKey3View->epsilon1 = 0.25f;
    chromaKey3View->epsilon2 = 0.75f;



    VulkanIndexBuffer indices(controller->getDevice(), 6);

    indices[0] = 0;
    indices[1] = 1;
    indices[2] = 2;
    indices[3] = 3;
    indices[4] = 2;
    indices[5] = 0;


    MaterialInfo shipbase(controller, &globals),
            shipdetail(controller, &globals),
            background(controller, &globals),
            planet1(controller, &globals),
            planet2(controller, &globals),
            planet3(controller, &globals),
            enemyship(controller, &globals2),
            menubackground(controller),
            menu_planetinfo_energy(controller),
            menu_planetinfo_science(controller),
            menu_getenergy(controller),
            menu_getscience(controller),
            menu_leave(controller),
            menuplanet(controller, &globals),
            enemyweapon(controller, &globals),
            playerweapon(controller, &globals3);

    shipbase.texture.texture = controller->getImageManager()->getImage("sprites/ShipBase.bmp");
    shipdetail.texture.texture = controller->getImageManager()->getImage("sprites/ShipDetail.bmp");
    background.texture.texture = controller->getImageManager()->getImage("sprites/SectorBackground.bmp");
    planet1.texture.texture = controller->getImageManager()->getImage("sprites/Planet1.bmp");
    planet2.texture.texture = controller->getImageManager()->getImage("sprites/Planet2.bmp");
    planet3.texture.texture = controller->getImageManager()->getImage("sprites/Planet3.bmp");
    enemyship.texture.texture = controller->getImageManager()->getImage("sprites/EnemyShip.bmp");
    menubackground.texture.texture = controller->getImageManager()->getImage("sprites/MenuButton.bmp");
    menu_planetinfo_energy.texture.texture = controller->getImageManager()->getImage("sprites/MenuButton.bmp");
    menu_planetinfo_science.texture.texture = controller->getImageManager()->getImage("sprites/MenuButton.bmp");
    menu_getenergy.texture.texture = controller->getImageManager()->getImage("sprites/MenuButton.bmp");
    menu_getscience.texture.texture = controller->getImageManager()->getImage("sprites/MenuButton.bmp");
    menu_leave.texture.texture = controller->getImageManager()->getImage("sprites/MenuButton.bmp");
    menuplanet.texture.texture = controller->getImageManager()->getImage("sprites/MenuButton.bmp");
    enemyweapon.texture.texture = controller->getImageManager()->getImage("sprites/enemy_attack.png");
    playerweapon.texture.texture = controller->getImageManager()->getImage("sprites/player_attack.png");


    ShaderPrototype chromakey = {"shader/chromakey.frag.spv", vk::ShaderStageFlagBits::eFragment};
    ShaderPrototype sphere = {"shader/sphere.frag.spv", vk::ShaderStageFlagBits::eFragment};
    ShaderPrototype frag = {"shader/shader.frag.spv", vk::ShaderStageFlagBits::eFragment};
    ShaderPrototype vert = {"shader/shader.vert.spv", vk::ShaderStageFlagBits::eVertex};


    PushConstantPrototype pcproto = {0, 0, sizeof (GameObjectPushConstant), vk::ShaderStageFlagBits::eVertex};



    std::vector<MaterialPrototype*> prototypes{ &shipbase.prototype, &shipdetail.prototype,
        &background.prototype, &planet1.prototype, &planet2.prototype, &planet3.prototype,
        &enemyship.prototype, &enemyweapon.prototype, &playerweapon.prototype};

    // menu buttons have a different fragment shader than sprites
    std::vector<MaterialPrototype*> menuprotos{&menubackground.prototype,
        &menu_planetinfo_energy.prototype, &menu_getenergy.prototype, &menu_getscience.prototype,
        &menu_leave.prototype, &menu_planetinfo_science.prototype};

    MaterialPrototypeHelpers::AddShaderToMany(menuprotos, frag);
    MaterialPrototypeHelpers::AddShaderToMany(prototypes, chromakey);


    for (auto & proto : menuprotos) {
        prototypes.push_back(proto);
    }


    // menu planet view has a special 3d effect
    menuplanet.prototype.shaders.push_back(sphere);
    prototypes.push_back(&menuplanet.prototype);

    MaterialPrototypeHelpers::AddShaderToMany(prototypes, vert);

    MaterialPrototypeHelpers::AddPushConstantToMany(prototypes, pcproto);



    VulkanVertexBufferDefault vertexBuffer(controller->getDevice(), 0, 4);

    vertexBuffer[0].position = {-0.5, -0.5};
    vertexBuffer[1].position = {0.5, -0.5};
    vertexBuffer[2].position = {0.5, 0.5};
    vertexBuffer[3].position = {-0.5, 0.5};

    vertexBuffer[0].uv = {1.0, 0.0};
    vertexBuffer[1].uv = {0.0, 0.0};
    vertexBuffer[2].uv = {0.0, 1.0};
    vertexBuffer[3].uv = {1.0, 1.0};


    MaterialPrototypeHelpers::AddVertexDescriptorToMany(prototypes,{&vertexBuffer, &vertexBuffer});




    shipbase.finalize(controller);
    shipdetail.finalize(controller);
    background.finalize(controller);
    planet1.finalize(controller);
    planet2.finalize(controller);
    planet3.finalize(controller);
    enemyship.finalize(controller);
    menubackground.finalize(controller);
    menu_planetinfo_energy.finalize(controller);
    menu_planetinfo_science.finalize(controller);
    menu_getenergy.finalize(controller);
    menu_getscience.finalize(controller);
    menu_leave.finalize(controller);
    menuplanet.finalize(controller);
    enemyweapon.finalize(controller);
    playerweapon.finalize(controller);
    
    // </editor-fold>

    // <editor-fold defaultstate="collapsed" desc="GameObject Prototype Setup">

    GameObjectPrototype backgroundProto(glm::vec2(0, 0), glm::vec2(2, 2), 0, pcproto.id);
    backgroundProto.addMesh(0, &background, background.material, &indices);

    GameObjectPrototype shipProto(glm::vec2(0, 0), CELL_SIZE, 0, pcproto.id);
    shipProto.addMesh(0, &shipbase, shipbase.material, &indices);
    shipProto.addMesh(1, &shipdetail, shipdetail.material, &indices);

    GameObjectPrototype enemyproto(glm::vec2(0.9f, 0), CELL_SIZE, 0, pcproto.id);
    enemyproto.addMesh(0, &enemyship, enemyship.material, &indices);

    GameObjectPrototype enemyweaponproto(glm::vec2(0, 0), glm::vec2(0,0), 0, pcproto.id);
    enemyweaponproto.addMesh(0, &enemyweapon, enemyweapon.material, &indices);

    GameObjectPrototype playerweaponproto(glm::vec2(0, 0), glm::vec2(0,0), 0, pcproto.id);
    playerweaponproto.addMesh(0, &playerweapon, playerweapon.material, &indices);



    GameObjectPrototype planet1Proto(glm::vec2(0, 0), CELL_SIZE, 0, pcproto.id);
    planet1Proto.addMesh(0, &planet1, planet1.material, &indices);

    GameObjectPrototype planet2Proto(glm::vec2(0, 0), CELL_SIZE, 0, pcproto.id);
    planet2Proto.addMesh(0, &planet2, planet2.material, &indices);

    GameObjectPrototype planet3Proto(glm::vec2(0, 0), CELL_SIZE, 0, pcproto.id);
    planet3Proto.addMesh(0, &planet3, planet3.material, &indices);

    std::vector<GameObjectPrototype*> planetprotos{ &planet1Proto, &planet2Proto, &planet3Proto};




    GameObjectPrototype menubackgroundproto(glm::vec2(0, 0), glm::vec2(1.5f, 1.5f), 0, pcproto.id);
    menubackgroundproto.addMesh(0, &menubackground, menubackground.material, &indices);

    GameObjectPrototype menu_planetinfo_energy_proto(glm::vec2(0.5f, -0.6f), BUTTON_SIZE, 0, pcproto.id);
    menu_planetinfo_energy_proto.addMesh(0, &menu_planetinfo_energy, menu_planetinfo_energy.material, &indices);

    GameObjectPrototype menu_planetinfo_science_proto(glm::vec2(0.5f, -0.4f), BUTTON_SIZE, 0, pcproto.id);
    menu_planetinfo_science_proto.addMesh(1, &menu_planetinfo_science, menu_planetinfo_science.material, &indices);

    GameObjectPrototype menu_getenergy_proto(glm::vec2(0, 0.1f), BUTTON_SIZE, 0, pcproto.id);
    menu_getenergy_proto.addMesh(0, &menu_getenergy, menu_getenergy.material, &indices);

    GameObjectPrototype menu_getscience_proto(glm::vec2(0, 0.3f), BUTTON_SIZE, 0, pcproto.id);
    menu_getscience_proto.addMesh(0, &menu_getscience, menu_getscience.material, &indices);

    GameObjectPrototype menu_leave_proto(glm::vec2(0, 0.5f), BUTTON_SIZE, 0, pcproto.id);
    menu_leave_proto.addMesh(0, &menu_leave, menu_leave.material, &indices);

    GameObjectPrototype menu_planet_proto(glm::vec2(-0.4f, -0.4f), CELL_SIZE + CELL_SIZE + CELL_SIZE, 0, pcproto.id);
    menu_planet_proto.addMesh(0, &menuplanet, menuplanet.material, &indices);

    // </editor-fold>

    // <editor-fold defaultstate="collapsed" desc="Object Setup">

    std::vector<PlanetInfo> planets;

    Scene scene;

    Font font("sprites/pixel_font.png");

    TextImage text_planetinfo_energy(*controller->getDevice(), menu_planetinfo_energy.material->getSampler(0), &font,
            *controller->getBufferPool(), *controller->getQueue(), "Hello World");

    TextImage text_planetinfo_science(*controller->getDevice(), menu_planetinfo_science.material->getSampler(0), &font,
            *controller->getBufferPool(), *controller->getQueue(), "Hello World");

    TextImage text_getenergy(*controller->getDevice(), menu_getenergy.material->getSampler(0), &font,
            *controller->getBufferPool(), *controller->getQueue(), "GET ENERGY");

    TextImage text_getscience(*controller->getDevice(), menu_getscience.material->getSampler(0), &font,
            *controller->getBufferPool(), *controller->getQueue(), "GET SCIENCE");

    TextImage text_leave(*controller->getDevice(), menu_leave.material->getSampler(0), &font,
            *controller->getBufferPool(), *controller->getQueue(), "LEAVE");


    ObjectHandle shiphandle = scene.addObject(shipProto, SHIP_LAYER),
            enemyhandle = scene.addObject(enemyproto, SHIP_LAYER),
            enemyweaponhandle = scene.addObject(enemyweaponproto, SHIP_EFFECT_LAYER),
            menu_background_handle = scene.addObject(menubackgroundproto, MENU_BACKGROUND_LAYER),
            menu_planetinfo_energy_handle = scene.addObject(menu_planetinfo_energy_proto, MENU_BUTTON_LAYER),
            menu_planetinfo_science_handle = scene.addObject(menu_planetinfo_science_proto, MENU_BUTTON_LAYER),
            menu_getscience_handle = scene.addObject(menu_getscience_proto, MENU_BUTTON_LAYER),
            menu_getenergy_handle = scene.addObject(menu_getenergy_proto, MENU_BUTTON_LAYER),
            menu_leave_handle = scene.addObject(menu_leave_proto, MENU_BUTTON_LAYER),
            menu_planet_handle = scene.addObject(menu_planet_proto, MENU_BUTTON_LAYER);

    scene.addObject(backgroundProto, BACKGROUND_LAYER);
    // </editor-fold>

    // <editor-fold defaultstate="collapsed" desc="Decorators">


    CoordinateConverter spaceconverter(window);

    Events events(&scene);

    PlayerShipControllerInfo playerShipInfo;

    playerShipInfo.enemy = enemyhandle;
    playerShipInfo.oncleanuprequest = events.requestcleanup;
    playerShipInfo.onclick = events.mouseclick;
    playerShipInfo.onkey = events.keypress;
    playerShipInfo.onpause = events.playercollide;
    playerShipInfo.onunpause = events.menubutton;
    playerShipInfo.onplayaudio = events.soundrequest;
    playerShipInfo.onshipexit = events.worldreset;
    playerShipInfo.playerstatechange = events.playerstatechange;
    playerShipInfo.playerweaponproto = &playerweaponproto;
    playerShipInfo.shipeffect_depth = SHIP_EFFECT_LAYER;
    
    scene.addDecorator(shiphandle, new PlayerShipController(playerShipInfo));

    scene.addDecorator(shiphandle, new PlanetCollidable(&planets, events.playercollide, events.worldreset));

    scene.addDecorator(enemyhandle, new AIShipController(events.playerstatechange, shiphandle));

    scene.addDecorator(menu_background_handle, new MenuObject(events.playercollide, events.menubutton));
    scene.addDecorator(menu_planetinfo_energy_handle, new MenuObject(events.playercollide, events.menubutton));
    scene.addDecorator(menu_planetinfo_science_handle, new MenuObject(events.playercollide, events.menubutton));

    scene.addDecorator(menu_getscience_handle, new MenuButton(events.mouseclick, events.playercollide, events.menubutton,
            PlayerScoreController::GET_SCIENCE_BUTTON_ID));
    scene.addDecorator(menu_getenergy_handle, new MenuButton(events.mouseclick, events.playercollide, events.menubutton,
            PlayerScoreController::GET_ENERGY_BUTTON_ID));
    scene.addDecorator(menu_leave_handle, new MenuButton(events.mouseclick, events.playercollide, events.menubutton,
            PlayerScoreController::LEAVE_BUTTON_ID));

    scene.addDecorator(new WorldController(&planets, &planetprotos, shiphandle, enemyhandle, events.worldreset, PLANET_LAYER));

    PlayerScoreControllerInfo pscInfo = {0};
    pscInfo.onplanetcollide = events.playercollide;
    pscInfo.onbuttonpress = events.menubutton;
    pscInfo.ondamage = events.playerdamaged;
    pscInfo.ongameover = events.gameover;

    pscInfo.planet_energy_display = &text_planetinfo_energy;
    pscInfo.planet_science_display = &text_planetinfo_science;
    pscInfo.player = shiphandle;

    scene.addDecorator(new PlayerScoreController(pscInfo));

    scene.addProperty<int>(shiphandle, "energy");
    scene.addProperty<int>(shiphandle, "science");
    
    scene.getObjectProperty<int>(shiphandle, "energy") = 300;
    scene.getObjectProperty<int>(shiphandle, "science") = 0;

    PropertyHandle max_turn_speed = scene.addProperty<double>(shiphandle, "max_turn_speed");
    PropertyHandle max_speed = scene.addProperty<double>(shiphandle, "max_speed");
    PropertyHandle turn_acceleration = scene.addProperty<double>(shiphandle, "turn_acceleration");
    PropertyHandle acceleration = scene.addProperty<double>(shiphandle, "acceleration");
    PropertyHandle shoot_period = scene.addProperty<double>(shiphandle, "shoot_period");

    scene.getObjectProperty<double>(shiphandle, max_turn_speed) = 0.5;
    scene.getObjectProperty<double>(shiphandle, max_speed) = 0.2;
    scene.getObjectProperty<double>(shiphandle, turn_acceleration) = 0.5;
    scene.getObjectProperty<double>(shiphandle, acceleration) = 0.1;
    scene.getObjectProperty<double>(shiphandle, shoot_period) = 1;

    scene.addDecorator(new SoundSystem(events.soundrequest, SOUNDS_DIRECTORY));

    scene.addDecorator(enemyweaponhandle, new EnemyWeaponController(shiphandle, enemyhandle,
            events.playerdamaged, events.soundrequest));

    scene.addDecorator(menu_planet_handle, new PlanetViewController(events.playercollide,
            events.menubutton, menuplanet.material->getSampler(0),
            std::vector<Material*> {
                planet1.material, planet2.material, planet3.material
            }));

    std::shared_ptr<ClickEventDispatcher> mouseclick(new ClickEventDispatcher(events.mouseclick, &spaceconverter));

    scene.addDecorator(mouseclick);
    input.addMouseHandler(GLFW_MOUSE_BUTTON_1, mouseclick.get());

    std::shared_ptr<KeyEventDispatcher> keypress(new KeyEventDispatcher(events.keypress));

    scene.addDecorator(keypress);
    input.addKeyHandler(keypress.get());

    // </editor-fold>

    scene.dispatchEvent(events.worldreset, nullptr);

    auto start = std::chrono::high_resolution_clock::now();

    while (!window.shouldClose()) {
        window.pollEvents();

        auto now = std::chrono::high_resolution_clock::now();
        double delta = std::chrono::duration_cast<std::chrono::nanoseconds>(now - start).count() / 1e9;
        start = now;

        controller->startRender();

        size_t frame = controller->getFrameIndex();

        scene.updateObjects(frame, spaceconverter, delta);

        scene.record(frame);

        std::vector<vk::CommandBuffer> buffers;

        scene.getbuffers(buffers, frame);

        controller->submitSecondaries(buffers);
    }

    delete controller;

    return EXIT_SUCCESS;
}

int main() {
    try {
        return run();
    } catch (std::exception & ex) {
        std::cerr << ex.what() << std::endl;
        throw ex;
    }
}
