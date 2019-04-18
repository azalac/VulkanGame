/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   SceneControllers.hpp
 * Author: austin-z
 *
 * Created on April 11, 2019, 4:58 PM
 */

#ifndef SCENECONTROLLERS_HPP
#define SCENECONTROLLERS_HPP

#include "ControllerHelpers.hpp"

/**
 Rotates an object based on the number given.
 */
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

/**
 * Couples the planet object and type together
 */
struct PlanetInfo {
    ObjectHandle object;
    int type;
};

/**
 * Sent from the planet collide event
 */
struct PlanetCollideEventArguments {
    void * sender;
    PlanetInfo planet;
    ObjectHandle collider;
};

/**
 * Handle object-planet collision. Should not be put on a planet.
 */
class PlanetCollidable : public ShipControllerDecorator, public EventHandler {
private:

    std::vector<PlanetInfo> * planets;
    std::set<ObjectHandle> collidedplanets;
    Event onplanetcollide, onplanetsreset;

    const double DISTANCE_THRESHOLD = 0.5;

public:

    /**
     * @param planets The planet list
     * @param onplanetcollide Sent when the owner collides with a planet
     * @param onplanetsreset Received when the world changes (on warp)
     */
    PlanetCollidable(std::vector<PlanetInfo> * planets, EventOut onplanetcollide = EventManager::NULL_EVENT,
            EventIn onplanetsreset = EventManager::NULL_EVENT) {
        this->planets = planets;
        this->onplanetcollide = onplanetcollide;
        this->onplanetsreset = onplanetsreset;
    }

    virtual void Apply(Scene * scene, ObjectHandle object, double deltat) {
        for (auto & planet : *planets) {
            if (collidedplanets.find(planet.object) != collidedplanets.end()) {
                continue;
            }

            double req_dist = glm::length((*scene)[planet.object].getSize()) * DISTANCE_THRESHOLD;
            double real_dist = glm::distance((*scene)[planet.object].getPosition(), (*scene)[object].getPosition());

            if (real_dist < req_dist) {
                scene->dispatchEvent(onplanetcollide, PlanetCollideEventArguments{this, planet, object});
                collidedplanets.insert(planet.object);
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

/**
 * Controls the planet placement and ship starting positions.
 */
class WorldController : public SceneDecorator, public EventHandler {
private:

    std::vector<PlanetInfo> * planets;
    std::vector<GameObjectPrototype*> * prototypes;

    ObjectHandle player, enemy;

    EventIn onresetworld;

    int planet_layer;

    bool worldneedsreset = false;

    const double STARTING_X_POS = 0.9, PLANET_CHANCE = 1 / 20.0, PLANET_MIN_SPEED = 0.025, PLANET_MAX_SPEED = 0.1;

    const int WORLD_SIZE = 10;

    const double WORLD_SIZE_REAL = 2;

    const int MAX_PLANET_POINTS = 300;

public:

    /**
     * @param planets The planet vector
     * @param prototypes The planet prototypes
     * @param player The player handle
     * @param enemy The enemy handle
     * @param onresetworld Received when the world should reset
     * @param planet_layer The layer planets should be placed on
     */
    WorldController(std::vector<PlanetInfo> * planets, std::vector<GameObjectPrototype*> * prototypes,
            ObjectHandle player, ObjectHandle enemy, EventIn onresetworld, int planet_layer) :
    planets(planets), prototypes(prototypes), player(player), enemy(enemy), onresetworld(onresetworld),
    planet_layer(planet_layer) {

        std::srand((unsigned int) std::time(0));

    }

    void RegisterHooks(EventManager* events) override {
        events->addHandler(onresetworld, this);
    }

    void OnEvent(EventManager* manager, Event id, const std::shared_ptr<void> argument) override {
        if (id == onresetworld) {
            worldneedsreset = true;
        }
    }

    void Apply(Scene* scene, double deltat) override {
        if (worldneedsreset) {
            (*scene)[player].setPosition(glm::vec2(-STARTING_X_POS, 0));
            (*scene)[enemy].setPosition(glm::vec2(STARTING_X_POS, 0));

            for (auto & planet : *planets) {
                scene->removeObject(planet.object);
            }

            planets->clear();

            generatePlanets(scene);

            worldneedsreset = false;
        }
    }

private:

    void generatePlanets(Scene * scene) {

        for (float x = 0; x < WORLD_SIZE; x++) {
            for (float y = 0; y < WORLD_SIZE; y++) {
                if (rand_bool(PLANET_CHANCE)) {

                    int type = rand<int>(prototypes->size());

                    ObjectHandle handle = scene->addObject(*prototypes->at(type), planet_layer);

                    planets->push_back({handle, type});

                    glm::vec2 world(x / WORLD_SIZE * WORLD_SIZE_REAL, y / WORLD_SIZE * WORLD_SIZE_REAL);

                    (*scene)[handle].setPosition(world - glm::vec2(1, 1));

                    scene->addDecorator(handle, new GameObjectRotator(rand<double>(
                            PLANET_MAX_SPEED - PLANET_MIN_SPEED) + PLANET_MIN_SPEED));

                    scene->addProperty<int>(handle, "energy");
                    scene->addProperty<int>(handle, "science");

                    scene->getObjectProperty<int>(handle, "energy") = rand<int>(MAX_PLANET_POINTS);
                    scene->getObjectProperty<int>(handle, "science") = rand<int>(MAX_PLANET_POINTS);

                }
            }
        }
    }

};

#endif /* SCENECONTROLLERS_HPP */

