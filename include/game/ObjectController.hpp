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

template<typename A, typename B, typename C>
static bool within(A test, B center, C deviation) {
    return test >= center - deviation && test <= center + deviation;
}

template<typename A, typename B, typename C>
static bool within2(A test, B lower, C upper) {
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
	bool paused;

    ShipStateChangeArguments(void * sender, const PlayerState state, bool paused) :
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
            scene->dispatchEvent(playerstatechange, ShipStateChangeArguments(this, state, paused));
            statechanged = false;
        }

    }

	bool isPaused() {
		return paused;
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

class PlayerControlledShipDecorator : public ShipControllerDecorator, public EventHandler {
private:

    CoordinateConverter * converter;

    EventIn onpause, onunpause, onclick;

public:

    PlayerControlledShipDecorator(
            CoordinateConverter * converter,
            EventOut playerstatechange = EventManager::NULL_EVENT,
            EventIn onpause = EventManager::NULL_EVENT,
            EventIn onunpause = EventManager::NULL_EVENT,
			EventIn onclick = EventManager::NULL_EVENT
            ) : ShipControllerDecorator(playerstatechange), converter(converter),
		onpause(onpause), onunpause(onunpause), onclick(onclick) {

    }

    virtual void RegisterHooks(EventManager * events) override {
        events->addHandler(onpause, this);
        events->addHandler(onunpause, this);
    }

    virtual void OnEvent(EventManager * manager, Event id, const std::shared_ptr<void> arguments) override {
        if (id == onpause) {
            setPaused(true);
        }
        if (id == onunpause) {
            setPaused(false);
        }
		if (id == onclick && !isPaused()) {
			setTarget(*reinterpret_cast<glm::vec2*>(arguments.get()));
		}
    }

};

class AIControlledShipDecorator : public ShipControllerDecorator, public EventHandler {
private:

    ObjectHandle player;
    Event playerstatechange;

    bool playerismoving = false;

public:

    AIControlledShipDecorator(EventIn playerstatechange, ObjectHandle player) {
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

			setPaused(state->paused);

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
				scene->dispatchEvent(onplanetcollide, PlanetCollideEventArguments{ this, handle, object });
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

class ClickEventDispatcher : public GLFWMouseDelegate, public SceneDecorator {
private:

	CoordinateConverter * converter;

	std::list<glm::vec2> clicks;

	EventOut onclick;

public:

	ClickEventDispatcher(EventOut onclick, CoordinateConverter * converter):
	onclick(onclick), converter(converter) {

	}

	virtual void Apply(Scene * scene, double deltat) override {
		for (auto & click : clicks) {
			scene->dispatchEvent(onclick, click);
		}
		clicks.clear();
	}

	virtual void MouseButton(InputHandler * handler, int button, int action, int modifiers, double x, double y) override {
		if (action == GLFW_PRESS) {
			return;
		}

		glm::vec2 p = glm::vec2(x, y) / converter->getScale();

		if (within2(p.x, -1, 1) && within2(p.y, -1, 1)) {
			clicks.push_back(p);
		}
	}

};

#endif /* OBJECT_CONTROLLER_HPP */
