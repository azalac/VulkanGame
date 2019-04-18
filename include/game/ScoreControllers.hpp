/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   ScoreControllers.hpp
 * Author: austin-z
 *
 * Created on April 11, 2019, 5:00 PM
 */

#ifndef SCORECONTROLLERS_HPP
#define SCORECONTROLLERS_HPP

#include "ControllerHelpers.hpp"

struct PlayerScoreControllerInfo {
    EventIn onplanetcollide, onbuttonpress;
    EventIn ondamage;
    EventOut ongameover;
    TextImage * planet_energy_display, *planet_science_display;
    ObjectHandle player;
};

/**
 * Handles player science and energy events
 */
class PlayerScoreController : public SceneDecorator, public EventHandler {
private:

    const PlayerScoreControllerInfo & info;

    int *planet_science_points = nullptr, *planet_energy_points = nullptr;

    const int MAX_ENERGY = 900;

public:

    static constexpr int GET_ENERGY_BUTTON_ID = 0,
            GET_SCIENCE_BUTTON_ID = 1,
            LEAVE_BUTTON_ID = 2;

    /**
     * @param menuopen Received when the menu opens
     * @param buttonpress Received when a menu button is pressed
     * @param ondamage Received when the player gets damaged
     * @param gameover Sent when a hit brings the player below 0 energy
     * @param planetinfo_energy The text sprite controller for energy info
     * @param planetinfo_science The text sprite controller for science info
     */
    PlayerScoreController(const PlayerScoreControllerInfo & info) : info(info) {

    }

    void RegisterHooks(EventManager* events) override {
        events->addHandler(info.onplanetcollide, this);
        events->addHandler(info.onbuttonpress, this);
        events->addHandler(info.ondamage, this);
    }

    void OnEvent(EventManager* manager, Event id, const std::shared_ptr<void> argument) override {
        Scene * scene = manager->getOwningScene();

        if (id == info.onplanetcollide) {
            ObjectHandle planet = reinterpret_cast<PlanetCollideEventArguments*> (argument.get())->planet.object;
            planet_energy_points = &scene->getObjectProperty<int>(planet, "energy");
            planet_science_points = &scene->getObjectProperty<int>(planet, "science");

            info.planet_energy_display->setText(std::string("ENERGY: ") + std::to_string(*planet_energy_points));
            info.planet_science_display->setText(std::string("SCIENCE: ") + std::to_string(*planet_science_points));
        }
        if (id == info.onbuttonpress) {
            int buttonid = *reinterpret_cast<int*> (argument.get());

            if (buttonid == GET_ENERGY_BUTTON_ID) {
                int & energy = scene->getObjectProperty<int>(info.player, "energy");
                energy += *planet_energy_points;
            }

            if (buttonid == GET_SCIENCE_BUTTON_ID) {
                int & science = scene->getObjectProperty<int>(info.player, "science");
                science += *planet_science_points;
            }

        }
        if (id == info.ondamage) {
            int & energy = scene->getObjectProperty<int>(info.player, "energy");
            energy -= *reinterpret_cast<int*> (argument.get());
            if (energy < 0) {
                scene->dispatchEvent(info.ongameover, nullptr);
            }
        }

        printf("Energy: %d, Science: %d\n",
                scene->getObjectProperty<int>(info.player, "energy"),
                scene->getObjectProperty<int>(info.player, "science"));

    }

    void Apply(Scene* scene, double deltat) override {
        // does nothing
    }

};

#endif /* SCORECONTROLLERS_HPP */

