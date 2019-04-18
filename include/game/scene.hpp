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

/**
 * Converts from window space to game space (a centered square)
 */
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
        float w = (float) window->getWidth(), h = (float) window->getHeight();

        float x = w - h;

        float sx = x < 0 ? 1 : ((w - x) / w);

        float y = h - w;

        float sy = y < 0 ? 1 : ((h - y) / h);

        scale = {sx, sy};

    }

    /**
     * Translates a point from world to screen space.
     * @param p The point
     * @return The screen space point
     */
    glm::vec2 translatePoint(glm::vec2 p) {
        return p * scale;
    }

    /**
     * Gets the calculated scaling factor
     * @return The scaling factor
     */
    glm::vec2 getScale() {
        return scale;
    }

};

/**
 * Used for pushing object information to the shader code
 */
struct GameObjectPushConstant {
    glm::vec2 position;
    glm::vec2 size;
    float rotation;
    glm::vec2 aspect;
};

/**
 * Tuple which helps order material renderers
 */
struct RenderInfo {
    int depth;
    std::shared_ptr<MaterialRenderer> renderer;

    RenderInfo(MaterialRenderer * material, int depth = 0) : depth(depth), renderer(material) {
    }
};

/**
 * Internal struct for material renderer builders and context
 */
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

/**
 * Describes a game object
 */
struct GameObjectPrototype {
    glm::vec2 initPosition;
    glm::vec2 initSize;
    float initRotation;
    int pcid;
    std::vector<_riproto> renderers;

    GameObjectPrototype(glm::vec2 initPosition = {0, 0}, glm::vec2 initSize = {0, 0}, float initRotation = 0, int pcid = 0) :
    initPosition(initPosition), initSize(initSize), initRotation(initRotation), pcid(pcid) {
    }

    void addMesh(int depth, MaterialRendererBuilder * builder, Material * material, VulkanIndexBuffer * indexes) {
        renderers.push_back({depth, builder, material, indexes});
    }
};

/**
 * A game object which can be moved, scaled, and rotated. Can also host multiple
 * textures.
 */
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

    /**
     * The push constant prototype id
     * @param infoid The pcp id
     */
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

    /**
     * Controls whether this object is being rendered or not
     * @param visible {@code true} for visible
     */
    void setVisible(bool visible) {
        this->visible = visible;
    }

    float getRotation() {
        return rotation;
    }

    void setRotation(float theta) {
        rotation = theta;
    }

    /**
     * Changes the rotation by a certain amount
     * @param theta The amount
     */
    void deltaRotation(float theta) {
        rotation += theta;

        float tau = (float) (2 * M_PI);

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

    /**
     * Updates this object's push constants and descriptor sets.
     * @param frame The current frame
     * @param converter The coordinate converter
     */
    void update(size_t frame, CoordinateConverter & converter) {
        if (!visible) {
            return;
        }

        info.position = converter.getScale() * position;
        info.size = converter.getScale() * size;
        info.rotation = rotation;
        info.aspect = converter.getScale();

        for (auto & e : renderers) {
            e.renderer->checkForUpdates(frame);
        }
    }

    /**
     * Records a specific frame (thread safe, but multiple cannot be recorded in parallel)
     * @param frame The frame
     */
    void record(size_t frame) {
        if (!visible) {
            return;
        }

        for (auto & e : renderers) {
            memcpy(e.renderer->getPushConstant(pcid).get(), &info, sizeof (info));

            e.renderer->recordFrame(frame);
        }
    }

    /**
     * Gets the internal command buffers used in this object.
     * @param buffers A reference to a buffer vector
     * @param frame The current frame
     */
    void getbuffers(std::vector<vk::CommandBuffer> & buffers, size_t frame) {
        if (!visible) {
            return;
        }

        for (auto & e : renderers) {
            buffers.push_back(e.renderer->getBuffer(frame));
        }
    }

};

// Represents an object in a scene
using ObjectHandle = size_t;

// Represents a decorator in a scene
using DecoratorHandle = size_t;

// Represents an event 
using Event = size_t;

// Represents a receive-only event
using EventIn = Event;

// Represents a send-only event
using EventOut = Event;

// Represents a dynamic property for an object or the scene
using PropertyHandle = size_t;

// forward reference for event handler
class EventManager;

// forward reference for Decorators Apply and Event Manager
class Scene;

class EventHandler {
public:

    /**
     * Invoked when the event gets called (must be registered first!)
     * @param manager The calling event manager
     * @param id The event id
     * @param argument The event argument
     */
    virtual void OnEvent(EventManager * manager, Event id, const std::shared_ptr<void> argument) = 0;

};

/**
 * Controls the event queueing and handling system
 */
class EventManager {
private:

    struct QueuedEvent {
        Event id;
        std::shared_ptr<void> argument;
    };

    std::list<QueuedEvent> events;

    std::map<Event, std::vector<EventHandler*>> handlers;

    Event nextid = 0;

    Scene * owner;

public:

    static constexpr Event NULL_EVENT = -1;

    EventManager(Scene * owner) : owner(owner) {

    }

    /**
     * Queues an event to be processed later
     * @param id The event
     * @param argument The argument
     */
    void enqueueEvent(Event id, std::shared_ptr<void> argument) {
        if (id == NULL_EVENT) {
            return;
        }

        events.push_back({id, argument});
    }

    Scene * getOwningScene() {
        return owner;
    }

    /**
     * Handles all stored events
     */
    void handleEvents() {
        while (events.size() > 0) {
            QueuedEvent evt = events.front();
            events.pop_front();

            for (auto & handler : handlers[evt.id]) {
                handler->OnEvent(this, evt.id, evt.argument);
            }
        }
    }

    /**
     * Registers an event handler in this manager
     * @param id The event
     * @param handler The handler
     */
    void addHandler(Event id, EventHandler * handler) {
        handlers[id].push_back(handler);
    }

    /**
     * Creates a unique event
     * @return The event
     */
    Event getNewEventId() {
        return nextid++;
    }

};

/**
 * Adds features to an object.
 */
class ObjectDecorator {
public:

    /**
     * Registers any event hooks for this decorator
     * @param events The event manager
     */
    virtual void RegisterHooks(EventManager * events) {

    }

    /**
     * Called every frame
     * @param scene The owning scene
     * @param object The owning object
     * @param deltat The time since last frame in seconds
     */
    virtual void Apply(Scene * scene, ObjectHandle object, double deltat) = 0;

};

/**
 * Adds a feature to the scene.
 */
class SceneDecorator {
public:

    /**
     * Registers any event hooks for this decorator
     * @param events The event manager
     */
    virtual void RegisterHooks(EventManager * events) {

    }

    /**
     * Called every frame
     * @param scene The owning scene
     * @param deltat The time since last frame in seconds
     */
    virtual void Apply(Scene * scene, double deltat) = 0;

};

template<typename Handle, typename T>
class ordered_lookup {
public:
    std::list<std::shared_ptr<T>> objects;
    std::map<Handle, std::shared_ptr<T>> byid;

    virtual void insert(T * value) {
        std::shared_ptr<T> ptr(value);
        objects.push_back(ptr);
        byid[ptr->id] = ptr;
    }

    virtual void insert_sorted(T * value) {
        std::shared_ptr<T> ptr(value);

        auto iter = objects.begin();
        while (iter != objects.end() && *iter->get() < *value) {
            iter++;
        }

        objects.insert(iter, ptr);
        byid[value->id] = ptr;
    }

    bool getbyid(Handle id, T ** reference) {
        try {
            *reference = byid.at(id).get();
            return true;
        } catch (std::out_of_range) {
            return false;
        }
    }

    virtual bool remove(Handle id) {
        auto iter = objects.begin();
        while (iter != objects.end() && iter->get()->id != id) {
            iter++;
        }
        
        if (iter != objects.end()) {
            byid.erase(id);
            objects.erase(iter);
            return true;
        } else {
            return false;
        }
    }
    
};

template<typename Handle, typename T>
class named_ordered_lookup {
public:
    std::list<std::shared_ptr<T>> objects;
    std::map<Handle, std::shared_ptr<T>> byid;
    std::map<std::string, std::shared_ptr<T>> byname;

    void insert(T * value) {
        std::shared_ptr<T> ptr(value);
        objects.push_back(ptr);
        byid[ptr->id] = ptr;
        byname[ptr->name] = ptr;
    }

    bool getbyid(Handle id, T ** reference) {
        try {
            *reference = byid.at(id).get();
            return true;
        } catch (std::out_of_range) {
            return false;
        }
    }

    bool getbyname(const std::string & name, T ** reference) {
        try {
            *reference = byname.at(name).get();
            return true;
        } catch (std::out_of_range) {
            return false;
        }
    }
    
    virtual bool remove(Handle id) {
        auto iter = objects.begin();
        while (iter != objects.end() && iter->get()->id != id) {
            iter++;
        }
        
        if (iter != objects.end()) {
            byname.erase(iter->get()->name);
            byid.erase(id);
            objects.erase(iter);
            return true;
        } else {
            return false;
        }
    }
};

/**
 * Significantly simplifies game object handling, event handling, and decorator
 * handling.
 */
class Scene {
private:

    struct PropertyInfo {
        PropertyHandle id;

        const std::string & name;

        std::unique_ptr<unsigned char> buffer;

        size_t size, count;

        template<typename T>
        PropertyInfo(PropertyHandle id, const std::string & name, T * ptr, size_t el_count) :
        id(id), name(name), buffer(reinterpret_cast<unsigned char*> (ptr)), size(sizeof (T)),
        count(el_count) {

        }

        bool operator<(const PropertyInfo& right) const {
            return right.id > this->id;
        }


    };

    // Represents a scene or object decorator

    template<class T>
    struct DecoratorInfo {
        DecoratorHandle id;

        std::shared_ptr<T> decorator;

        bool operator<(const DecoratorInfo& right) const {
            return right.id > this->id;
        }

    };

    // Represents a game object

    struct ObjectInfo {
        const ObjectHandle id;
        int depth;
        std::shared_ptr<GameObject> object;

        ordered_lookup<DecoratorHandle, DecoratorInfo<ObjectDecorator>> decorators;

        named_ordered_lookup<PropertyHandle, PropertyInfo> properties;

        ObjectInfo(ObjectHandle id, int depth, std::shared_ptr<GameObject> object) :
        id(id), depth(depth), object(object) {

        }

        bool operator<(const ObjectInfo& right) const {
            return right.depth > this->depth;
        }

    };

    // All game objects in this scene
    ordered_lookup<ObjectHandle, ObjectInfo> objects;

    // All scene decorators in this scene
    ordered_lookup<DecoratorHandle, DecoratorInfo<SceneDecorator>> decorators;

    // All scene properties for this scene
    named_ordered_lookup<PropertyHandle, PropertyInfo> properties;

    // The event manager
    EventManager eventmanager;

    // The next object id allocation
    ObjectHandle nextid = 0;

    // The next decorator id allocation
    DecoratorHandle nextdecorator = 0;

    // The next property id allocation
    PropertyHandle nextprop = 0;

public:

    Scene() : eventmanager(this) {

    }

    /**
     * Adds an object to this scene
     * @param object The object
     * @param depth The object's layer
     * @return The object handle
     */
    ObjectHandle addObject(std::shared_ptr<GameObject> object, int depth = 0) {
        ObjectInfo * info = new ObjectInfo(nextid++, depth, object);

        objects.insert_sorted(info);

        return info->id;
    }

    /**
     * Adds an object to this scene
     * @param proto The object's prototype
     * @param depth The object's layer
     * @return The created object's handle
     */
    ObjectHandle addObject(GameObjectPrototype & proto, int depth = 0) {

        return addObject(std::shared_ptr<GameObject>(new GameObject(proto)), depth);
    }

    /**
     * Adds an object decorator to the specified object
     * @param owner The owning object
     * @param decorator The decorator
     * @return The decorator handle
     */
    DecoratorHandle addDecorator(ObjectHandle owner, std::shared_ptr<ObjectDecorator> decorator) {
        ObjectInfo * object;

        if (!objects.getbyid(owner, &object)) {
            throw std::runtime_error("Could not find object " + std::to_string(owner));
        }

        decorator->RegisterHooks(&eventmanager);

        DecoratorInfo<ObjectDecorator> * info = new DecoratorInfo<ObjectDecorator>{nextdecorator++, decorator};

        object->decorators.insert(info);

        return info->id;
    }

    /**
     * Adds an object decorator to the specified object
     * @param owner The owning object
     * @param decorator The decorator
     * @return The decorator handle
     */
    DecoratorHandle addDecorator(ObjectHandle owner, ObjectDecorator * decorator) {

        return addDecorator(owner, std::shared_ptr<ObjectDecorator>(decorator));
    }

    /**
     * Adds a scene decorator to this scene
     * @param decorator The decorator
     * @return The decorator handle
     */
    DecoratorHandle addDecorator(std::shared_ptr<SceneDecorator> decorator) {

        DecoratorInfo<SceneDecorator> * info = new DecoratorInfo<SceneDecorator>{nextdecorator++, decorator};

        decorator->RegisterHooks(&eventmanager);

        decorators.insert(info);

        return info->id;
    }

    /**
     * Adds a scene decorator to this scene
     * @param decorator The decorator
     * @return The decorator handle
     */
    DecoratorHandle addDecorator(SceneDecorator * decorator) {

        return addDecorator(std::shared_ptr<SceneDecorator>(decorator));
    }

    template<typename T>
    PropertyHandle addProperty(const std::string & name, size_t count = 1) {
        PropertyInfo * info = new PropertyInfo(nextprop++, name, new T[count], count);

        properties.insert(info);

        return info->id;
    }

    template<typename T>
    PropertyHandle addProperty(ObjectHandle owner, const std::string & name, size_t count = 1) {
        ObjectInfo * obj;

        if (!objects.getbyid(owner, &obj)) {
            throw std::runtime_error("Could not find object " + std::to_string(owner));
        }

        PropertyInfo * info = new PropertyInfo(nextprop++, name, new T[count], count);

        obj->properties.insert(info);

        return info->id;
    }

    /**
     * Removes an object from this scene
     * @param handle The handle
     * @return {@code true} if it was removed, {@code false} otherwise
     */
    bool removeObject(ObjectHandle handle) {
        return objects.remove(handle);
    }

    /**
     * Removes a decorator from the given object
     * @param handle The handle
     * @return {@code true} if it was removed, {@code false} otherwise
     */
    bool removeDecorator(ObjectHandle owner, DecoratorHandle decorator) {

        ObjectInfo * obj;

        if(!objects.getbyid(owner, &obj)) {
            return false;
        }
        
        return obj->decorators.remove(decorator);
        
    }

    bool removeProperty(PropertyHandle property) {
        return properties.remove(property);
    }

    bool removeProperty(ObjectHandle owner, PropertyHandle property) {

        ObjectInfo * obj;

        if(!objects.getbyid(owner, &obj)) {
            return false;
        }
        
        return obj->properties.remove(property);
        
    }

    /**
     * Used to access game objects
     * @param handle The object handle
     * @return The object reference
     */
    GameObject & operator[](ObjectHandle handle) {

        ObjectInfo * obj;

        if(!objects.getbyid(handle, &obj)) {
            throw std::runtime_error("Could not find object " + std::to_string(handle));
        }
        
        return *obj->object.get();
    }

    template<typename T>
    T & getProperty(PropertyHandle property, size_t index = 0, bool unsafe = false) {
        PropertyInfo * info;

        if(!properties.getbyid(property, &info)) {
            throw std::runtime_error("Could not find property " + std::to_string(index));
        }

        if (index < 0 || index >= info->count) {
            throw std::runtime_error("Invalid index " + std::to_string(index) + " for property " + std::to_string(property));
        }

        if (sizeof (T) != info->size && !unsafe) {
            throw std::runtime_error("Invalid access type for property " + info->name +
                    " - different sizes (" + std::to_string(sizeof (T)) + " & " + \
                    std::to_string(info->size) + ") and not using unsafe mode");
        }

        return info->buffer.get()[index];
    }

    template<typename T>
    T & getProperty(const std::string & name, size_t index = 0, bool unsafe = false) {
        PropertyInfo * info;

        if(!properties.getbyname(name, &info)) {
            throw std::runtime_error("Could not find property " + name);
        }

        if (index < 0 || index >= info->count) {
            throw std::runtime_error("Invalid index " + std::to_string(index) + " for property " + name);
        }

        if (sizeof (T) != info->size && !unsafe) {
            throw std::runtime_error("Invalid access type for property " + info->name +
                    " - different sizes (" + std::to_string(sizeof (T)) + " & " + \
                    std::to_string(info->size) + ") and not using unsafe mode");
        }

        return info->buffer.get()[index];
    }

    template<typename T>
    T & getObjectProperty(ObjectHandle owner, PropertyHandle property, size_t index = 0, bool unsafe = false) {
        
        ObjectInfo * obj;

        if(!objects.getbyid(owner, &obj)) {
            throw std::runtime_error("Could not find object " + std::to_string(owner));
        }

        PropertyInfo * info;

        if(!obj->properties.getbyid(property, &info)) {
            throw std::runtime_error("Could not find property " + std::to_string(property));
        }

        if (index < 0 || index >= info->count) {
            throw std::runtime_error("Invalid index " + std::to_string(index) + " for property " + std::to_string(property));
        }

        if (sizeof (T) != info->size && !unsafe) {
            throw std::runtime_error("Invalid access type for property " + info->name +
                    " - different sizes (" + std::to_string(sizeof (T)) + " & " + \
                    std::to_string(info->size) + ") and not using unsafe mode");
        }

        return reinterpret_cast<T*> (info->buffer.get())[index];
    }

    template<typename T>
    T & getObjectProperty(ObjectHandle owner, const std::string & name, size_t index = 0, bool unsafe = false) {
        
        ObjectInfo * obj;

        if(!objects.getbyid(owner, &obj)) {
            throw std::runtime_error("Could not find object " + std::to_string(owner));
        }

        PropertyInfo * info;

        if(!obj->properties.getbyname(name, &info)) {
            throw std::runtime_error("Could not find property " + name);
        }


        if (index < 0 || index >= info->count) {
            throw std::runtime_error("Invalid index " + std::to_string(index) + " for property " + name);
        }

        if (sizeof (T) != info->size && !unsafe) {
            throw std::runtime_error("Invalid access type for property " + info->name +
                    " - different sizes (" + std::to_string(sizeof (T)) + " & " + \
                    std::to_string(info->size) + ") and not using unsafe mode");
        }

        return reinterpret_cast<T*> (info->buffer.get())[index];
    }

    /**
     * Dispatches an event with a buffer argument
     * @param id The event
     * @param arguments The buffer argument
     */
    void dispatchEvent(Event id, std::shared_ptr<void> arguments) {

        eventmanager.enqueueEvent(id, arguments);
        printf("Dispatching event %d\n", (int) id);
    }

    /**
     * Dispatches an event with an object argument
     * @param id The event
     * @param argument The argument
     */
    template<typename T>
    void dispatchEvent(Event id, const T & argument) {

        eventmanager.enqueueEvent(id, std::shared_ptr<void>(new T(argument)));
        
        printf("Dispatching event %d\n", (int) id);
    }

    /**
     * Creates a unique event
     * @return The event
     */
    Event createEventId() {

        return eventmanager.getNewEventId();
    }

    /**
     * Updates all objects
     * @param frame The current frame
     * @param converter The world-to-screen converter
     * @param deltat The time since last frame in seconds
     */
    void updateObjects(size_t frame, CoordinateConverter & converter, double deltat) {
        for (auto & decorator : decorators.objects) {
            decorator->decorator->Apply(this, deltat);
        }

        eventmanager.handleEvents();
        converter.updateView();

        for (auto & object : objects.objects) {
            for (auto & decorator : object.get()->decorators.objects) {

                decorator->decorator->Apply(this, object->id, deltat);
            }

            object->object->update(frame, converter);
        }

    }

    /**
     * Records a given frame
     * @param frame The frame
     */
    void record(size_t frame) {
        for (auto & object : objects.objects) {

            object->object->record(frame);
        }
    }

    /**
     * Gets all used buffers in this scene
     * @param buffers The buffer vector
     * @param frame The frame
     */
    void getbuffers(std::vector<vk::CommandBuffer> & buffers, size_t frame) {
        for (auto & object : objects.objects) {
            object->object->getbuffers(buffers, frame);
        }
    }

};

#endif /* SCENE_HPP */

