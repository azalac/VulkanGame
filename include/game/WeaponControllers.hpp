/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   WeaponControllers.hpp
 * Author: austin-z
 *
 * Created on April 11, 2019, 9:17 PM
 */

#ifndef WEAPONCONTROLLERS_HPP
#define WEAPONCONTROLLERS_HPP

#include "ControllerHelpers.hpp"

/**
 * Collects 'dead' objects, specifically old lasers and missiles
 */
class DeadObjectCollector : public SceneDecorator, public EventHandler {
private:

    EventIn onobjectdied;

public:

    DeadObjectCollector(EventIn onobjectdied) :
    onobjectdied(onobjectdied) {
    }

    void RegisterHooks(EventManager* events) override {
        events->addHandler(onobjectdied, this);
    }

    void OnEvent(EventManager* manager, Event id, const std::shared_ptr<void> argument) override {
        if (id == onobjectdied) {
            manager->getOwningScene()->removeObject(*reinterpret_cast<ObjectHandle*> (argument.get()));
        }
    }


};

/**
 * Makes the owning object act like a laser beam with a duration. Leaves the
 * object's height as its default, but modifies the width so it fits between the
 * beam's source and target
 */
class PlasmaBallController : public ObjectDecorator {
private:

    glm::vec2 velocity;

    EventOut oncleanuprequest;

    ObjectHandle enemy;
    
    bool cleanup_requested = false, enemy_killed = false;

    double length_remaining = TRAVEL_LENGTH;
    
    static constexpr double TRAVEL_LENGTH = 0.6, RADIUS = 0.1, TRAVEL_SPEED = 0.3;

public:

    PlasmaBallController(glm::vec2 velocity, EventOut oncleanuprequest, ObjectHandle enemy) :
    velocity(velocity), oncleanuprequest(oncleanuprequest), enemy(enemy) {

    }

    void Apply(Scene* scene, ObjectHandle object, double deltat) override {

        // do nothing if this object is invalid
        if(cleanup_requested) {
            return;
        }
        
        GameObject & owner = (*scene)[object];

        if (length_remaining < 0) {
            owner.setVisible(false);
            if(!cleanup_requested) {
                scene->dispatchEvent(oncleanuprequest, object);
                cleanup_requested = true;
            }
            return;
        }

        owner.setVisible(true);
        owner.setSize(RADIUS * 2, RADIUS * 2);

        glm::vec2 delta = velocity;
        delta *= deltat * TRAVEL_SPEED;

        owner.deltaPosition(delta);

        length_remaining -= glm::length(delta);

    }

};

class EnemyWeaponController : public ObjectDecorator {
private:

    ObjectHandle player;

    ObjectHandle enemy;
    
    EventOut onplayerdamaged;

    EventOut onrequestsound;
    
    double size = MAX_SIZE;

    static constexpr double MAX_SIZE = 0.4, GROW_SPEED = 0.2;

    static constexpr int DMG_LOWER = 50, DMG_UPPER = 100;

public:

    EnemyWeaponController(ObjectHandle player, ObjectHandle enemy, EventOut onplayerdamaged,
            EventOut onrequestsound) :
    player(player), enemy(enemy), onplayerdamaged(onplayerdamaged),
    onrequestsound(onrequestsound) {
        
    }

    void Apply(Scene* scene, ObjectHandle object, double deltat) override {

        GameObject & owner = (*scene)[object];
        GameObject & enemyobj = (*scene)[enemy];
        GameObject & playerobj = (*scene)[player];
        
        if(size < MAX_SIZE) {
            size += GROW_SPEED * deltat;
        } else if(glm::distance(enemyobj.getPosition(), playerobj.getPosition()) < MAX_SIZE) {
            // if the weapon is ready to fire again, and the player is within
            // range, fire
            size = 0;
            
            scene->dispatchEvent(onplayerdamaged, rand<int>(DMG_UPPER - DMG_LOWER) + DMG_LOWER);
            scene->dispatchEvent(onrequestsound, SoundRequest("missle_launch.wav"));
        }
        
        owner.setVisible(size < MAX_SIZE);
        owner.setSize(size, size);
        owner.setPosition(enemyobj.getPosition());
        
    }



};

#endif /* WEAPONCONTROLLERS_HPP */

