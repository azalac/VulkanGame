/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   scene.hpp
 * Author: austin-z
 *
 * Created on February 13, 2019, 11:25 AM
 */

#ifndef SCENE_HPP
#define SCENE_HPP

#include "VulkanRenderer.hpp"

#include <glm/glm.hpp>

#include <list>
#include <algorithm>

#define _USE_MATH_DEFINES
#include <math.h>

// converts from window space to a square in the center

class CoordinateConverter {
private:

    glm::vec2 scale;
    Window * window;

public:

    CoordinateConverter(Window & window) {
        this->window = &window;
        updateView();
    }

    void updateView(void) {
        float w = (float)window->getWidth(), h = (float)window->getHeight();

        float x = w - h;

        float sx = x < 0 ? 1 : ((w - x) / w);

        float y = h - w;

        float sy = y < 0 ? 1 : ((h - y) / h);

        scale = {sx, sy};

    }

    /*
    Parameters:
            p - the [0, 1] game-space point
    Returns:
            The [0, 1] screen-space point
     */
    glm::vec2 translatePoint(glm::vec2 p) {
        return p * scale;
    }

    /*
    Parameters:
            p - the [0, 1] game-space size
    Returns:
            The [0, 1] screen-space size
     */
    glm::vec2 translateSize(glm::vec2 s) {
        return s * scale;
    }

    glm::vec2 getScale() {
        return scale;
    }

};

struct GameObjectPushConstant {
    glm::vec2 position;
    glm::vec2 size;
    float rotation;
    glm::vec2 aspect;
};

struct RenderInfo {
    int depth;
    std::shared_ptr<MaterialRenderer> renderer;

    RenderInfo(MaterialRenderer * material, int depth = 0) : depth(depth), renderer(material) {
    }
};

struct _riproto {
    int depth;
    MaterialRendererBuilder * builder;
    Material * material;
    VulkanIndexBuffer * indexes;

    RenderInfo create() {
        RenderInfo info(builder->createRenderer(material, indexes), depth);
        return info;
    }
};

struct GameObjectPrototype {
    glm::vec2 initPosition;
    glm::vec2 initSize;
    float initRotation;
    int pcid;
    std::vector<_riproto> renderers;

    GameObjectPrototype(glm::vec2 initPosition = {0, 0}, glm::vec2 initSize = {0, 0}, float initRotation = 0, int pcid = 0) :
    initPosition(initPosition), initSize(initSize), initRotation(initRotation), pcid(pcid) {
    }

    void addRenderer(int depth, MaterialRendererBuilder * builder, Material * material, VulkanIndexBuffer * indexes) {
        renderers.push_back({depth, builder, material, indexes});
    }
};

class GameObject {
private:
    struct GameObjectPushConstant info;
    std::list<struct RenderInfo> renderers;
    int pcid;
    bool visible = true;

    glm::vec2 position, size;
    float rotation;

public:

    GameObject(float x = 0, float y = 0, float width = 1, float height = 1, float rotation = 0) {
        this->position = {x, y};
        this->rotation = rotation;
        this->size = {width, height};
    }

    GameObject(glm::vec2 position, glm::vec2 size, float rotation) {
        this->position = position;
        this->rotation = rotation;
        this->size = size;
    }

    GameObject(struct GameObjectPrototype & prototype) :
    GameObject(prototype.initPosition, prototype.initSize, prototype.initRotation) {

        pcid = prototype.pcid;

        for (auto & mproto : prototype.renderers) {
            addMaterial(mproto.create());
        }
    }

    void addMaterial(RenderInfo info) {
        auto iter = renderers.begin();
        while (iter != renderers.end() && iter->depth < info.depth) {
            iter++;
        }

        renderers.insert(iter, info);
    }

    void addMaterial(MaterialRenderer * material, int depth = 0) {
        auto iter = renderers.begin();
        while (iter != renderers.end() && iter->depth < depth) {
            iter++;
        }

        renderers.insert(iter, RenderInfo(material, depth));
    }

    void setPushConstantID(int infoid) {
        pcid = infoid;
    }

    void setSize(double width, double height) {
        size = glm::vec2(width, height);
    }

    void setPosition(glm::vec2 pos) {
        position = pos;
    }

    void deltaPosition(glm::vec2 delta) {
        position += delta;
    }

    void setVisible(bool visible) {
        this->visible = visible;
    }

    float getRotation() {
        return rotation;
    }

    void setRotation(float theta) {
        rotation = theta;
    }

    void deltaRotation(float theta) {
        rotation += theta;

        float tau = (float)(2 * M_PI);

        while (rotation < 0) {
            rotation += tau;
        }

        while (rotation > tau) {
            rotation -= tau;
        }

    }

    float getX() {
        return position.x;
    }

    float getY() {
        return position.y;
    }

    glm::vec2 getPosition() {
        return position;
    }

    glm::vec2 getSize() {
        return size;
    }

    void update(size_t frame, CoordinateConverter & converter) {
        if (!visible) {
            return;
        }

        info.position = converter.translatePoint(position);
        info.size = converter.translateSize(size);
        info.rotation = rotation;
        info.aspect = converter.getScale();

        for (auto & e : renderers) {
            e.renderer->checkForUpdates(frame);
        }
    }

    void record(size_t frame) {
        if (!visible) {
            return;
        }

        for (auto & e : renderers) {
            memcpy(e.renderer->getPushConstant(pcid).get(), &info, sizeof (info));

            e.renderer->recordFrame(frame);
        }
    }

    void getbuffers(std::vector<vk::CommandBuffer> & buffers, size_t frame) {
        if (!visible) {
            return;
        }

        for (auto & e : renderers) {
            buffers.push_back(e.renderer->getBuffer(frame));
        }
    }

};

using ObjectHandle = size_t;
using DecoratorHandle = size_t;
using Event = size_t;

class EventManager;

class EventHandler {
public:

    virtual void OnEvent(EventManager * manager, Event id, const std::shared_ptr<void> argument) = 0;

};

class EventManager {
private:

    struct QueuedEvent {
        Event id;
        std::shared_ptr<void> argument;
    };

    std::list<QueuedEvent> events;

    std::map<Event, std::vector<EventHandler*>> handlers;

    Event nextid = 0;

public:

    static constexpr Event NULL_EVENT = -1;

    void enqueueEvent(Event id, std::shared_ptr<void> argument) {
        if (id == NULL_EVENT) {
            return;
        }

        events.push_back({id, argument});
    }

    void handleEvents() {
        while (events.size() > 0) {
            QueuedEvent evt = events.front();
            events.pop_front();

            for (auto & handler : handlers[evt.id]) {
                handler->OnEvent(this, evt.id, evt.argument);
            }
        }
    }

    void addHandler(Event id, EventHandler * handler) {
        handlers[id].push_back(handler);
    }

    Event getNewEventId() {
        return nextid++;
    }

};

// forward reference for ObjectDecorator.Apply
class Scene;

class ObjectDecorator {
public:

    virtual void RegisterHooks(EventManager * events) {}

	virtual void Apply(Scene * scene, ObjectHandle object, double deltat) = 0;

};

class DecoratorConstructor {
public:

    virtual std::shared_ptr<ObjectDecorator> create() = 0;

};

template<typename T>
class SimpleDecoratorConstructor : public DecoratorConstructor {
public:

    virtual std::shared_ptr<ObjectDecorator> create() override {
        return std::shared_ptr<ObjectDecorator>(new T());
    }

};

class Scene {
private:

    struct DecoratorInfo {
        DecoratorHandle id;

        std::shared_ptr<ObjectDecorator> decorator;
    };

    struct ObjectInfo {
        int depth;
        ObjectHandle id;
        std::shared_ptr<GameObject> object;

        std::list<DecoratorInfo> decorators;
        std::map<DecoratorHandle, DecoratorInfo*> decoratorsbyid;
    };

    std::list<ObjectInfo> objects;
    std::map<ObjectHandle, ObjectInfo*> objectsbyid;

    EventManager eventmanager;

    ObjectHandle nextid = 0;
    DecoratorHandle nextdecorator = 0;

public:

    ObjectHandle addObject(std::shared_ptr<GameObject> object, int depth = 0) {
        auto iter = objects.begin();
        while (iter != objects.end() && iter->depth < depth) {
            iter++;
        }

        auto entry = objects.insert(iter,{depth, nextid++, object});
        objectsbyid[entry->id] = &(*entry);

        return entry->id;
    }

    ObjectHandle addObject(GameObjectPrototype & proto, int depth = 0) {
        return addObject(std::shared_ptr<GameObject>(new GameObject(proto)), depth);
    }

    DecoratorHandle addDecorator(ObjectHandle owner, std::shared_ptr<ObjectDecorator> decorator) {
        auto iter = objects.begin();
        while (iter != objects.end() && iter->id != owner) {
            iter++;
        }

        if (iter == objects.end()) {
            throw std::runtime_error("Invalid object handle");
        }

        DecoratorInfo info = {nextdecorator++, decorator};

        iter->decorators.push_back(info);
        iter->decoratorsbyid[info.id] = &iter->decorators.back();

        decorator->RegisterHooks(&eventmanager);

        return info.id;
    }

    DecoratorHandle addDecorator(ObjectHandle owner, DecoratorConstructor * builder) {
        return addDecorator(owner, std::shared_ptr<ObjectDecorator>(builder->create()));
    }

    bool removeObject(ObjectHandle handle) {
        auto iter = objects.begin();
        while (iter != objects.end() && iter->id != handle) {
            iter++;
        }

        if (iter != objects.end()) {
            objects.erase(iter);
            return true;
        } else {
            return false;
        }
    }

    bool removeDecorator(ObjectHandle owner, DecoratorHandle decorator) {
        auto iter = objects.begin();
        while (iter != objects.end() && iter->id != owner) {
            iter++;
        }

        if (iter == objects.end()) {
            return false;
        }

        auto iter2 = iter->decorators.begin();
        while (iter2 != iter->decorators.end() && iter2->id != decorator) {
            iter2++;
        }

        if (iter2 != iter->decorators.end()) {
            iter->decorators.erase(iter2);
            return false;
        } else {
            return false;
        }
    }

    GameObject & operator[](ObjectHandle handle) {
        return *objectsbyid.at(handle)->object;
    }

    ObjectDecorator * decorator(ObjectHandle owner, DecoratorHandle handle) {
        ObjectInfo * object;

        try {
            object = objectsbyid.at(owner);
        } catch (std::out_of_range) {
            throw std::runtime_error("Invalid object handle");
        }

        try {
            return object->decoratorsbyid.at(handle)->decorator.get();
        } catch (std::out_of_range) {
            throw std::runtime_error("Invalid decorator handle");
        }
    }

    void dispatchEvent(Event id, std::shared_ptr<void> arguments) {
        eventmanager.enqueueEvent(id, arguments);
    }

    Event createEventId() {
        return eventmanager.getNewEventId();
    }

    void updateObjects(size_t frame, CoordinateConverter & converter, double deltat) {
        converter.updateView();

        for (auto & object : objects) {
            for (auto & decorator : object.decorators) {
                decorator.decorator->Apply(this, object.id, deltat);
            }

            object.object->update(frame, converter);
        }

        eventmanager.handleEvents();
    }

    void record(size_t frame) {
        for (auto & object : objects) {
            object.object->record(frame);
        }
    }

    void getbuffers(std::vector<vk::CommandBuffer> & buffers, size_t frame) {
        for (auto & object : objects) {
            object.object->getbuffers(buffers, frame);
        }
    }

};

#endif /* SCENE_HPP */

