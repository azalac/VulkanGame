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

template<typename T>
static bool within(T test, T center, T deviation) {
    return test >= center - deviation && test <= center + deviation;
}

template<typename T>
static bool within2(T test, T lower, T upper) {
    return test >= lower && test <= upper;
}

template <typename T>
static int signum(T val) {
    return (T(0) < val) - (val < T(0));
}

class GameObjectRotator : public ObjectDecorator {
private:

    double circlespersec;

public:

    GameObjectRotator(double circlespersec) {
        this->circlespersec = circlespersec;
    }

    virtual void Apply(Scene * scene, ObjectHandle object, double deltat) override {
        (*scene)[object].deltaRotation((float) (deltat * circlespersec * M_PI * 2));
    }

};

class GameObjectRotatorBuilder : public DecoratorConstructor {
private:

    double circlespersec;

public:

    GameObjectRotatorBuilder(double circlespersec) {
        this->circlespersec = circlespersec;
    }

    virtual std::shared_ptr<ObjectDecorator> create() {
        return std::shared_ptr<ObjectDecorator>(new GameObjectRotator(circlespersec));
    }

};

enum class PlayerState {
    eIdle,
    eTurning,
    eMoving,
	ePaused
};

struct ShipStateChangeArguments {
    void * sender;
    const PlayerState state;

    ShipStateChangeArguments(void * sender, const PlayerState state) :
    sender(sender), state(state) {

    }
};

class ShipControllerDecorator : public ObjectDecorator {
private:

    glm::vec2 target = {0, 0};

    double shipspeed = DEFAULT_SHIP_SPEED;
    double rotationspeed = 1;

    bool target_reached = true;

    Event playerstatechange;
    bool statechanged = false;
    PlayerState state = PlayerState::eIdle;

    bool paused = false;

public:

    static constexpr double DEFAULT_SHIP_SPEED = 0.1;

    ShipControllerDecorator(Event playerstatechange = EventManager::NULL_EVENT) {
        this->playerstatechange = playerstatechange;
        setState(PlayerState::eIdle);
    }

    void setTarget(glm::vec2 target) {
        this->target = target;
        setState(PlayerState::eTurning);
    }

    virtual double getSpeed() {
        return shipspeed;
    }

    bool isMoving() {
        return state == PlayerState::eMoving && !paused;
    }

    virtual void Apply(Scene * scene, ObjectHandle object, double deltat) {
        if (state == PlayerState::eIdle || paused) {
            return;
        }

        GameObject & owner = (*scene)[object];

        // get the direction unit vector
        glm::vec2 direction = target - owner.getPosition();

        double length = glm::length(direction);

        // normalize the vector and scale it
        double k = getSpeed() * deltat / length;
        direction = {direction.x * k, direction.y * k};

        double target_rotation = atan2(direction.x, -direction.y) - M_PI_2;

        double current_rotation = (double) owner.getRotation();

        double delta = handleTurn(current_rotation, target_rotation);

        owner.deltaRotation((float)(delta * deltat));

        if (abs(delta) > M_PI_4 / 2) {
            setState(PlayerState::eTurning);
        }

        if (abs(delta) < M_PI_4 / 2) {
            setState(PlayerState::eMoving);
        }

        if (state == PlayerState::eMoving) {
            owner.deltaPosition(direction);
        }

        if (length <= 0.01) {
            setState(PlayerState::eIdle);
        }

        if (statechanged) {
            scene->dispatchEvent(playerstatechange, std::shared_ptr<ShipStateChangeArguments>(new ShipStateChangeArguments(this, paused ? PlayerState::ePaused : state)));
            statechanged = false;
        }

    }

protected:

    void setState(PlayerState newstate) {
        state = newstate;
        statechanged = true;
    }

    void setPaused(bool paused) {
        this->paused = paused;
        statechanged = true;
    }

private:

    double handleTurn(double current, double target) {
        double delta = fmod(target - current, 2 * M_PI);

        if (delta < -M_PI) {
            delta += 2 * M_PI;
        }

        if (delta >= M_PI) {
            delta -= 2 * M_PI;
        }

        double adelta = abs(delta);

        if (adelta > rotationspeed) {
            delta = signum(delta) * rotationspeed;
        }

        return delta;
    }

    static double normalizeAngle(double angle) {
        double tau = 2 * M_PI;

        return fmod(fmod(angle, tau) + tau, tau);
    }

};

class PlayerControlledShipDecorator : public ShipControllerDecorator, public GLFWMouseDelegate, public EventHandler {
private:

    CoordinateConverter * converter;

    Event onplanetcollide;
    Event onmenuclose;

public:

    PlayerControlledShipDecorator(
            CoordinateConverter * converter,
            Event playerstatechange = EventManager::NULL_EVENT,
            Event onplanetcollide = EventManager::NULL_EVENT,
            Event onmenuclose = EventManager::NULL_EVENT
            ) : ShipControllerDecorator(playerstatechange), converter(converter),
    onplanetcollide(onplanetcollide), onmenuclose(onmenuclose) {

    }

    virtual void RegisterHooks(EventManager * events) override {
        events->addHandler(onplanetcollide, this);
        events->addHandler(onmenuclose, this);
    }

    virtual void OnEvent(EventManager * manager, Event id, const std::shared_ptr<void> arguments) override {
        if (id == onplanetcollide) {
            setPaused(true);
        }
        if (id == onmenuclose) {
            setPaused(false);
        }
    }

    virtual void MouseButton(InputHandler * handler, int button, int action,
            int modifiers, double x, double y) override {

        if (action == GLFW_RELEASE) {
            return;
        }

        converter->updateView();
        glm::vec2 s = converter->getScale();

        glm::vec2 target = glm::vec2(x, y) / s;

        target.x = clamp(target.x, -s.x, s.x);
        target.y = clamp(target.y, -s.y, s.y);

        setTarget(target);
    }

    template<typename T>
    static T clamp(T value, T lower, T upper) {
        return value < lower ? lower : (value > upper ? upper : value);
    }

};

class AIControlledShipDecorator : public ShipControllerDecorator, public EventHandler {
private:

    ObjectHandle player;
    Event playerstatechange;

    bool playerismoving = false;

public:

    AIControlledShipDecorator(Event playerstatechange, ObjectHandle player) {
        this->playerstatechange = playerstatechange;
        this->player = player;
    }

    virtual void RegisterHooks(EventManager * events) override {
        events->addHandler(playerstatechange, this);
    }

    virtual double getSpeed() override {
        return playerismoving ? ShipControllerDecorator::getSpeed() / 2 : 0;
    }

    virtual void Apply(Scene * scene, ObjectHandle object, double deltat) {
        setTarget((*scene)[player].getPosition());

        ShipControllerDecorator::Apply(scene, object, deltat);
    }

    virtual void OnEvent(EventManager * manager, Event id, const std::shared_ptr<void> arguments) override {
        if (id == playerstatechange) {
            const ShipStateChangeArguments * state = reinterpret_cast<ShipStateChangeArguments*> (arguments.get());
            playerismoving = state->state == PlayerState::eMoving;

            if (!playerismoving) {
                setState(PlayerState::eIdle);
            }
        }
    }

};

struct PlanetCollideEventArguments {
    void * sender;
    ObjectHandle planet;
    ObjectHandle collider;
};

class PlanetCollidable : public ShipControllerDecorator, public EventHandler {
private:

    std::vector<ObjectHandle> * planets;
    std::set<ObjectHandle> collidedplanets;
    Event onplanetcollide, onplanetsreset;

public:

    PlanetCollidable(std::vector<ObjectHandle> * planets, Event onplanetcollide = EventManager::NULL_EVENT, Event onplanetsreset = EventManager::NULL_EVENT) {
        this->planets = planets;
        this->onplanetcollide = onplanetcollide;
        this->onplanetsreset = onplanetsreset;
    }

    virtual void Apply(Scene * scene, ObjectHandle object, double deltat) {
        for (auto handle : *planets) {
            if (collidedplanets.find(handle) != collidedplanets.end()) {
                continue;
            }

            // if the distance between this object and the planet is within the planet's boundaries, they're colliding
            if (glm::distance((*scene)[handle].getPosition(), (*scene)[object].getPosition()) < glm::length((*scene)[handle].getSize()) / 2) {
                scene->dispatchEvent(onplanetcollide, std::shared_ptr<void>(new PlanetCollideEventArguments{this, handle, object}));
                collidedplanets.insert(handle);
            }
        }
    }

    virtual void RegisterHooks(EventManager * events) override {
        events->addHandler(onplanetsreset, this);
    }

    virtual void OnEvent(EventManager * manager, Event id, const std::shared_ptr<void> arguments) override {
        if (id == onplanetsreset) {
            // when the planets get reset, clear the collided planet list
            collidedplanets.clear();
        }
    }

};

#endif /* OBJECT_CONTROLLER_HPP */
