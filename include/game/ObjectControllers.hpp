/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/*
 * File:   ObjectController.hpp
 * Author: austin-z
 *
 * Created on March 6, 2019, 3:46 PM
 */

#ifndef OBJECT_CONTROLLER_HPP
#define OBJECT_CONTROLLER_HPP

#include "scene.hpp"

#include <set>
#include <ctime>

#include "ShipControllers.hpp"
#include "SceneControllers.hpp"
#include "ScoreControllers.hpp"
#include "SoundSystem.hpp"
#include "WeaponControllers.hpp"

/**
 * Listens for mouse clicks and propagates them as events. Does not do anything
 * if the click is outside the game square.
 */
class ClickEventDispatcher : public GLFWMouseDelegate, public SceneDecorator {
private:

    std::list<glm::vec2> clicks;

    EventOut onclick;

    CoordinateConverter * converter;

public:

    /**
     * @param onclick Sent when a click is received. argument is a glm::vec2
     * @param converter The world-to-screen converter
     */
    ClickEventDispatcher(EventOut onclick, CoordinateConverter * converter) :
    onclick(onclick), converter(converter) {

    }

    virtual void Apply(Scene * scene, double deltat) override {
        for (auto & click : clicks) {
            scene->dispatchEvent(onclick, click);
            printf("Dispatching mouse click\n");
        }
        clicks.clear();
    }

    virtual void MouseButton(InputHandler * handler, int button, int action, int modifiers, double x, double y) override {
        if (action == GLFW_PRESS) {
            return;
        }

        glm::vec2 p = glm::vec2(x, y) / converter->getScale();

        if (within_bounds(p.x, -1, 1) && within_bounds(p.y, -1, 1)) {
            clicks.push_back(p);
        }
    }

};

class KeyEventDispatcher : public GLFWKeyDelegate, public SceneDecorator {
private:

    std::list<KeyEvent> keys;

    EventOut onkey;

public:

    KeyEventDispatcher(EventOut onkey) :
    onkey(onkey) {
    }

    void Apply(Scene* scene, double deltat) override {
        for (auto & key : keys) {
            scene->dispatchEvent(onkey, key);
        }
        keys.clear();
    }

    void KeyDown(InputHandler* handler, int key, int scancode, int action, int modifiers) override {
        keys.push_back(KeyEvent(handler, key, action, modifiers));
    }

    void KeyUp(InputHandler* handler, int key, int scancode, int action, int modifiers) override {
        keys.push_back(KeyEvent(handler, key, action, modifiers));
    }

};

#endif /* OBJECT_CONTROLLER_HPP */
