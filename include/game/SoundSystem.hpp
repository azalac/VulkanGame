/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   SoundSystem.hpp
 * Author: austin-z
 *
 * Created on April 11, 2019, 5:04 PM
 */

#ifndef SOUNDSYSTEM_HPP
#define SOUNDSYSTEM_HPP

#include "ControllerHelpers.hpp"

#include <irrklang/irrKlang.h>

struct PlayingSound {
    SoundRequest request;
    irrklang::ISound * sound_reference;
    double time_left;
    
    PlayingSound() {
        
    }
};

/**
 * Manages the irrKlang sound engine and plays sounds when requested
 */
class SoundSystem : public SceneDecorator, public EventHandler {
private:

    irrklang::ISoundEngine * engine;

    EventIn onsoundrequest;

    const std::string & sound_dir;

    /**
     * Sounds that will be played in the future
     */
    std::list<SoundRequest> requests;

    /**
     * Sounds that are playing, but will stop in the future
     */
    std::list<PlayingSound> playing;

public:

    static constexpr double DELAY_IMMEDIATE = 0;

    static constexpr int LOOP_ALWAYS = -1;

    SoundSystem(EventIn onsoundrequest, const std::string & sound_directory) :
    onsoundrequest(onsoundrequest), sound_dir(sound_directory) {
        engine = irrklang::createIrrKlangDevice();
    }

    ~SoundSystem() {
        engine->drop();
    }

    void RegisterHooks(EventManager* events) override {
        events->addHandler(onsoundrequest, this);
    }

    void OnEvent(EventManager* manager, Event id, const std::shared_ptr<void> argument) override {
        if (id == onsoundrequest) {
            requests.push_back(*reinterpret_cast<SoundRequest*> (argument.get()));
        }
    }

    void Apply(Scene* scene, double deltat) override {
        auto iter = playing.begin();
        while (iter != playing.end()) {
            iter->time_left -= deltat;

            if (iter->time_left < 0) {
                iter->sound_reference->stop();
                iter->sound_reference->drop();

                // automatically increments iterator to position after current
                // and deletes the current item
                iter = playing.erase(iter);
            } else {
                iter++;
            }
        }

        auto req_iter = requests.begin();

        while (req_iter != requests.end()) {
            PlaySound(*req_iter);

            // always erases items from requests
            req_iter = requests.erase(req_iter);
        }

    }

private:

    void PlaySound(SoundRequest req) {
        std::string file = sound_dir + req.file;

        PlayingSound playing;
        playing.request = req;
        
        if (req.has_pos) {
            irrklang::vec3df pos(req.screen_pos.x, req.screen_pos.y, 1);
            playing.sound_reference = engine->play3D(file.c_str(), pos, req.loops > 1);
        } else {
            playing.sound_reference = engine->play2D(file.c_str(), req.loops > 1);
        }
        
        if (playing.sound_reference) {
            // getplaylength returns milliseconds
            playing.time_left = playing.sound_reference->getPlayLength() / 1000.0 * req.loops;
            this->playing.push_back(playing);
        }
    }

};

#endif /* SOUNDSYSTEM_HPP */

