/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   ShipController.hpp
 * Author: austin-z
 *
 * Created on April 11, 2019, 4:58 PM
 */

#ifndef SHIPCONTROLLER_HPP
#define SHIPCONTROLLER_HPP

#include "ControllerHelpers.hpp"

#include <complex>
#include "WeaponControllers.hpp"

enum class PlayerState {
    // The ship is idle
    eIdle,
    // The ship is turning
    eTurning,
    // The ship is moving
    eMoving
};

/**
 Sent from the ship state change event.
 */
struct ShipStateChangeArguments {
    void * sender;
    const PlayerState state;
    bool paused;

    ShipStateChangeArguments(void * sender, const PlayerState state, bool paused) :
    sender(sender), state(state), paused(paused) {

    }
};

/**
 * Controls an actor via setting a target. The actor must first rotate to face the
 * target, then it can start moving. player state change sent when the internal
 * state chances (see above enum class).
 */
class ShipControllerDecorator : public ObjectDecorator {
private:

    glm::vec2 target = {0, 0};

    double shipspeed = DEFAULT_SHIP_SPEED;
    double rotationspeed = 1;

    bool target_reached = true;

    EventOut playerstatechange;
    bool statechanged = false;
    PlayerState state = PlayerState::eIdle;

    bool paused = false;

public:

    static constexpr double DEFAULT_SHIP_SPEED = 0.1;

    ShipControllerDecorator(EventOut playerstatechange = EventManager::NULL_EVENT) {
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

    bool isPaused() {
        return paused;
    }

    virtual void Apply(Scene * scene, ObjectHandle object, double deltat) {
        checkStateEvents(scene);

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

        owner.deltaRotation((float) (delta * deltat));

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

    }

protected:

    void checkStateEvents(Scene * scene) {
        if (statechanged) {
            scene->dispatchEvent(playerstatechange, ShipStateChangeArguments(this, state, paused));
            statechanged = false;
        }
    }

    void setState(PlayerState newstate) {
        statechanged = state != newstate;
        state = newstate;
    }

    void setPaused(bool paused) {
        statechanged = this->paused != paused;
        this->paused = paused;
    }

private:

    /**
     * Gets the amount to turn by without any time scaling.
     * @param current The current rotation
     * @param target The target rotation
     * @return The delta rotation
     */
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

};

struct PlayerShipControllerInfo {
    EventIn onpause;
    EventIn onunpause;
    EventIn onclick;
    EventIn onkey;
    EventOut playerstatechange;
    EventOut onshipexit;
    EventOut onplayaudio;

    GameObjectPrototype * playerweaponproto;
    int shipeffect_depth;
    EventIn oncleanuprequest;
    ObjectHandle enemy;

};

/**
 * A ship controller which responsed to mouse click events
 */
class PlayerShipController : public ShipControllerDecorator, public EventHandler {
private:

    PlayerShipControllerInfo info;
    
    int turn_direction = 0, move_direction = 0;

    glm::vec2 shoot_direction;

    double angular_momentum = 0;
    glm::vec2 velocity = {0.0f, 0.0f};

    bool braking = false;
    bool shooting = false;

    double shoot_delay = 0;

    static constexpr int TURN_LEFT = -1,
            TURN_NONE = 0,
            TURN_RIGHT = 1,
            MOVE_BACKWARD = -1,
            MOVE_NONE = 0,
            MOVE_FORWARD = 1;

    static constexpr int LEFT_KEY = GLFW_KEY_A,
            RIGHT_KEY = GLFW_KEY_D,
            BACK_KEY = GLFW_KEY_S,
            FORWARD_KEY = GLFW_KEY_W,
            BRAKE_KEY = GLFW_KEY_TAB,

            SHOOT_KEY = GLFW_KEY_SPACE;

public:

    /**
     * @param playerstatechange Sent when the state changes.
     * @param onpause Received when the ship should paused (on menu open)
     * @param onunpause Received when the ship should continue (on menu close)
     * @param onclick Received when the ship should change its target
     */
    PlayerShipController(PlayerShipControllerInfo & info) :
    ShipControllerDecorator(info.playerstatechange), info(info) {

    }

    virtual void RegisterHooks(EventManager * events) override {
        events->addHandler(info.onpause, this);
        events->addHandler(info.onunpause, this);
        events->addHandler(info.onclick, this);
        events->addHandler(info.onkey, this);
    }

    virtual void OnEvent(EventManager * manager, Event id, const std::shared_ptr<void> argument) override {
        if (id == info.onpause) {
            setPaused(true);
        }
        if (id == info.onunpause) {
            setPaused(false);
        }
        if (id == info.onclick && !isPaused()) {
            setTarget(*reinterpret_cast<glm::vec2*> (argument.get()));
        }
        if (id == info.onkey) {
            checkKeys(reinterpret_cast<KeyEvent*> (argument.get()));
        }
    }

    void Apply(Scene* scene, ObjectHandle object, double deltat) override {

        checkStateEvents(scene);

        // don't move when paused
        if (isPaused()) {
            return;
        }

        GameObject & owner = (*scene)[object];

        double max_ang_momentum = scene->getObjectProperty<double>(object, "max_turn_speed");
        double max_speed = scene->getObjectProperty<double>(object, "max_speed");
        double ang_accel = scene->getObjectProperty<double>(object, "turn_acceleration");
        double accel = scene->getObjectProperty<double>(object, "acceleration");

        owner.deltaRotation(angular_momentum * deltat);

        if (!braking) {
            angular_momentum += turn_direction * ang_accel * deltat;
        } else {
            if (abs(angular_momentum) > ang_accel * deltat) {
                angular_momentum -= signum(angular_momentum) * ang_accel * deltat;
            } else {
                angular_momentum = 0;
            }
        }

        angular_momentum = clamp(angular_momentum, -max_ang_momentum, max_ang_momentum);

        owner.deltaPosition({velocity.x * deltat, velocity.y * deltat});

        // delta v can never be side-to-side, so it just accounts for the y axis
        glm::vec2 facing(cos(owner.getRotation()), sin(owner.getRotation()));

        double k = accel * move_direction * deltat;

        if (!braking) {
            velocity += glm::vec2(facing.x * k, facing.y * k);
        } else {
            k = accel * deltat;

            glm::vec2 norm = glm::normalize(velocity);

            if (glm::length(velocity) > k) {
                velocity -= glm::vec2(norm.x * k, norm.y * k);
            } else {
                velocity = glm::vec2(0.0f, 0.0f);
            }
        }

        // I could've used len2 here, but it doesn't help much
        double len = glm::length(velocity);

        if (len > max_speed) {
            velocity *= (max_speed / len);
        }

        setState(len > 0.01 ? PlayerState::eMoving : PlayerState::eIdle);

        if (shoot_delay < 0 && shooting) {
            // reset shoot time to default value
            shoot_delay = scene->getObjectProperty<double>(object, "shoot_period");

            ObjectHandle weapon = scene->addObject(*info.playerweaponproto, info.shipeffect_depth);

            scene->addDecorator(weapon, new PlasmaBallController(facing, info.oncleanuprequest, info.enemy));
            
            (*scene)[weapon].setPosition(owner.getPosition());
            
            scene->dispatchEvent(info.onplayaudio, SoundRequest("laser_shoot.wav"));
        }

        shoot_delay -= deltat;

        // if the owner is outside the bounds, dispatch a world reset event and
        // reset the velocities
        if (!within_bounds(owner.getX(), -1, 1) || !within_bounds(owner.getY(), -1, 1)) {
            scene->dispatchEvent(info.onplayaudio, SoundRequest("warpdrive.wav"));
            scene->dispatchEvent(info.onshipexit, nullptr);
            velocity = {0.0f, 0.0f};
            angular_momentum = 0;
        }
    }


private:

    void checkKeys(KeyEvent * evt) {
        InputHandler * input = evt->input;

        turn_direction = 0;
        move_direction = 0;
        shoot_direction = {0.0f, 0.0f};

        if (input->isKeyDown(LEFT_KEY)) {
            turn_direction += TURN_LEFT;
        }

        if (input->isKeyDown(RIGHT_KEY)) {
            turn_direction += TURN_RIGHT;
        }

        if (input->isKeyDown(BACK_KEY)) {
            move_direction += MOVE_BACKWARD;
        }

        if (input->isKeyDown(FORWARD_KEY)) {
            move_direction += MOVE_FORWARD;
        }

        braking = input->isKeyDown(BRAKE_KEY);
        shooting = input->isKeyDown(SHOOT_KEY);
    }

};

/**
 * A ship controller which follows the AI rules from the assignment specification.
 */
class AIShipController : public ShipControllerDecorator, public EventHandler {
private:

    ObjectHandle player;
    Event playerstatechange;

    bool playerismoving = false;

    double playerdistance;

    const double SPEED_BOOST_DISTANCE = 0.4;

public:

    /**
     * @param playerstatechange Received when the player changes state
     * @param player The player object handle
     */
    AIShipController(EventIn playerstatechange, ObjectHandle player) {
        this->playerstatechange = playerstatechange;
        this->player = player;
    }

    virtual void RegisterHooks(EventManager * events) override {
        events->addHandler(playerstatechange, this);
    }

    virtual double getSpeed() override {
        double scalar = playerdistance < SPEED_BOOST_DISTANCE ? 1 : 0.5;
        return playerismoving ? ShipControllerDecorator::getSpeed() * scalar : 0;
    }

    virtual void Apply(Scene * scene, ObjectHandle object, double deltat) override {
        glm::vec2 p = (*scene)[player].getPosition();
        playerdistance = glm::distance(p, (*scene)[object].getPosition());
        setTarget(p);

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


#endif /* SHIPCONTROLLER_HPP */

