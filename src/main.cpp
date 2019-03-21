
#include <iostream>
#include <exception>

#include "VulkanController.hpp"

#include "game/scene.hpp"
#include "game/ObjectController.hpp"
#include "game/Menu.hpp"

#include <chrono>
#include <thread>

const int WIDTH = 1024;
const int HEIGHT = WIDTH * 9 / 16;

const int SHIP_LAYER = 2;
const int PLANET_LAYER = 1;
const int BACKGROUND_LAYER = 0;

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
	Event playerhit, gameover;
	Event menubutton;
	Event deltaenergy;
	Event deltascience;

	Events(Scene * scene):
	mouseclick(scene->createEventId()), playerstatechange(scene->createEventId()),
		playercollide(scene->createEventId()), playerhit(scene->createEventId()),
		gameover(scene->createEventId()), menubutton(scene->createEventId()),
		deltaenergy(scene->createEventId()), deltascience(scene->createEventId()) {
		
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

    void finalize(VulkanController * controller) {
        prototype.samplers.push_back(texture);
        material = controller->createMaterial(prototype, globals);
    }

    virtual MaterialRenderer * createRenderer(Material * material, VulkanIndexBuffer * indexbuffer = nullptr) override {
        return controller->createRenderer(material, indexbuffer);
    }

};

void generatePlanets(Scene & scene, std::vector<ObjectHandle> & planets, std::vector<GameObjectPrototype*> & protos) {

    for (auto & handle : planets) {
        scene.removeObject(handle);
    }

    planets.clear();

    srand((unsigned int) time(nullptr));

    GameObjectRotatorBuilder rotator(0.1);

    for (float x = 0; x < 10; x++) {
        for (float y = 0; y < 10; y++) {
            if (rand() % 20 == 1) {
                ObjectHandle handle = scene.addObject(*protos[rand() % protos.size()], PLANET_LAYER);

                planets.push_back(handle);

                scene[handle].setPosition({x / 5.0f - 1.0f + 0.1, y / 5.0f - 1.0f + 0.1});

                scene.addDecorator(handle, &rotator);
            }
        }
    }
}

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


    UniformBufferPrototype chromaKeyDescriptor(0, 0, sizeof (struct ChromaKey), 1, vk::ShaderStageFlagBits::eFragment);

    UBOMap globals = Material::createubos(*controller->getDevice(), std::vector<UniformBufferPrototype> {
        chromaKeyDescriptor });

    VulkanBufferView<struct ChromaKey> chromaKeyView(*globals[chromaKeyDescriptor.id]);

    chromaKeyView->search = {0, 1.0f, 0, 1.0f};
    chromaKeyView->replace = {0, 0, 0, 0};
    chromaKeyView->epsilon1 = 0.25f;
    chromaKeyView->epsilon2 = 0.75f;

    UniformBufferPrototype chromaKeyBlueDescriptor(0, 0, sizeof (struct ChromaKey), 1, vk::ShaderStageFlagBits::eFragment);

    UBOMap globals2 = Material::createubos(*controller->getDevice(), std::vector<UniformBufferPrototype> {
        chromaKeyBlueDescriptor });

    VulkanBufferView<struct ChromaKey> chromaKey2View(*globals2[chromaKeyBlueDescriptor.id]);

    chromaKey2View->search = {0, 0, 1.0f, 1.0f};
    chromaKey2View->replace = {0, 0, 0, 0};
    chromaKey2View->epsilon1 = 0.25f;
    chromaKey2View->epsilon2 = 0.75f;



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
            menubutton(controller);

    shipbase.texture.texture = controller->getImageManager()->getImage("sprites/ShipBase.bmp");
    shipdetail.texture.texture = controller->getImageManager()->getImage("sprites/ShipDetail.bmp");
    background.texture.texture = controller->getImageManager()->getImage("sprites/SectorBackground.bmp");
    planet1.texture.texture = controller->getImageManager()->getImage("sprites/Planet1.bmp");
    planet2.texture.texture = controller->getImageManager()->getImage("sprites/Planet2.bmp");
    planet3.texture.texture = controller->getImageManager()->getImage("sprites/Planet3.bmp");
    enemyship.texture.texture = controller->getImageManager()->getImage("sprites/EnemyShip.bmp");
    menubutton.texture.texture = controller->getImageManager()->getImage("sprites/MenuButton.bmp");


    ShaderPrototype frag = {"shader/shader.frag.spv", vk::ShaderStageFlagBits::eFragment};
    ShaderPrototype vert = {"shader/shader.vert.spv", vk::ShaderStageFlagBits::eVertex};


    PushConstantPrototype pcproto = {0, 0, sizeof (GameObjectPushConstant), vk::ShaderStageFlagBits::eVertex};



    std::vector<MaterialPrototype*> prototypes{ &shipbase.prototype, &shipdetail.prototype,
            &background.prototype, &planet1.prototype, &planet2.prototype, &planet3.prototype,
            &enemyship.prototype, &menubutton.prototype};

    MaterialPrototypeHelpers::AddShaderToMany(prototypes, frag);
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
    menubutton.finalize(controller);

    // </editor-fold>

    // <editor-fold defaultstate="collapsed" desc="GameObject Prototype Setup">

    GameObjectPrototype backgroundProto(glm::vec2(0, 0), glm::vec2(2, 2), 0, pcproto.id);
    backgroundProto.addRenderer(0, &background, background.material, &indices);

    GameObjectPrototype shipProto(glm::vec2(-0.9f, 0), glm::vec2(0.2f, 0.2f), 0, pcproto.id);
    shipProto.addRenderer(0, &shipbase, shipbase.material, &indices);
    shipProto.addRenderer(1, &shipdetail, shipdetail.material, &indices);


    GameObjectPrototype planet1Proto(glm::vec2(0, 0), glm::vec2(0.2f, 0.2f), 0, pcproto.id);
    planet1Proto.addRenderer(0, &planet1, planet1.material, &indices);

    GameObjectPrototype planet2Proto(glm::vec2(0, 0), glm::vec2(0.2f, 0.2f), 0, pcproto.id);
    planet2Proto.addRenderer(0, &planet2, planet2.material, &indices);

    GameObjectPrototype planet3Proto(glm::vec2(0, 0), glm::vec2(0.2f, 0.2f), 0, pcproto.id);
    planet3Proto.addRenderer(0, &planet3, planet3.material, &indices);

    std::vector<GameObjectPrototype*> planetprotos{ &planet1Proto, &planet2Proto, &planet3Proto};


    GameObjectPrototype enemyproto(glm::vec2(0.9f, 0), glm::vec2(0.2f, 0.2f), 0, pcproto.id);
    enemyproto.addRenderer(0, &enemyship, enemyship.material, &indices);

    GameObjectPrototype menubuttonproto(glm::vec2(0, 0), glm::vec2(0.3f, 0.1f), 0, pcproto.id);
    menubuttonproto.addRenderer(0, &menubutton, menubutton.material, &indices);
    
    // </editor-fold>

    std::vector<ObjectHandle> planets;

    Scene scene;

    generatePlanets(scene, planets, planetprotos);


    ObjectHandle shiphandle = scene.addObject(shipProto, SHIP_LAYER),
            enemyhandle = scene.addObject(enemyproto, SHIP_LAYER),
            menubuttonhandle = scene.addObject(menubuttonproto, 4);
            
    scene.addObject(backgroundProto, BACKGROUND_LAYER);


    CoordinateConverter spaceconverter(window);

	Events events(&scene);

    auto shipcontroller = new PlayerControlledShipDecorator(&spaceconverter, events.playerstatechange, events.playercollide, events.menubutton, events.mouseclick);
	auto mouseclick = new ClickEventDispatcher(events.mouseclick, &spaceconverter);

    scene.addDecorator(shiphandle, shipcontroller);

    scene.addDecorator(shiphandle, new PlanetCollidable(&planets, events.playercollide));
    
    scene.addDecorator(enemyhandle, new AIControlledShipDecorator(events.playerstatechange, shiphandle));

    scene.addDecorator(menubuttonhandle, new MenuButton(events.mouseclick, events.playercollide, events.menubutton, 0));

    scene.addDecorator(mouseclick);
    input.addMouseHandler(GLFW_MOUSE_BUTTON_1, mouseclick);

	//Font font("cmunrm.ttf", 12);

	//TextImage text(*controller->getDevice(), shipdetail.material->getSampler(0), &font, "Hello World");

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
    }
}
