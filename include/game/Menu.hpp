/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   Menu.hpp
 * Author: austin-z
 *
 * Created on March 17, 2019, 11:41 PM
 */

#ifndef MENU_HPP
#define MENU_HPP

#include "scene.hpp"
#include "ObjectControllers.hpp"

// Represents a menu object

class MenuObject : public ObjectDecorator, public EventHandler {
private:
    Event showmenu;
    Event hidemenu;

    bool menuvisible = false;

public:

    /**
     * @param showmenu When received, the menu becomes visible
     * @param hidemenu when received, the menu becomes invisible
     */
    MenuObject(EventIn showmenu, EventIn hidemenu) {
        this->showmenu = showmenu;
        this->hidemenu = hidemenu;
    }

    virtual void RegisterHooks(EventManager * events) override {
        events->addHandler(showmenu, this);
        events->addHandler(hidemenu, this);
    }

    virtual void Apply(Scene * scene, ObjectHandle object, double deltat) override {
        (*scene)[object].setVisible(menuvisible);
    }

    virtual void OnEvent(EventManager * manager, Event id, const std::shared_ptr<void> argument) override {
        if (id == showmenu) {
            menuvisible = true;
        }
        if (id == hidemenu) {
            menuvisible = false;
        }
    }
};

// Represents a menu button

class MenuButton : public MenuObject {
private:

    EventIn mouseclick;
    EventOut buttonpress;
    std::list<glm::vec2> clicks;
    int buttonid;

public:

    /**
     * @param mouseclick The mouse click event
     * @param showmenu When received, the menu becomes visible
     * @param buttonpress when received, the menu becomes invisible.
     *      Also sent when the button is pressed with an int argument
     * @param buttonid The int sent when buttonpress is sent
     */
    MenuButton(EventIn mouseclick, EventIn showmenu, Event buttonpress, int buttonid) :
    mouseclick(mouseclick), MenuObject(showmenu, buttonpress) {
        this->buttonpress = buttonpress;
        this->buttonid = buttonid;
    }

    void RegisterHooks(EventManager* events) override {
        events->addHandler(mouseclick, this);
        MenuObject::RegisterHooks(events);
    }

    void OnEvent(EventManager* manager, Event id, const std::shared_ptr<void> argument) override {
        if (id == mouseclick) {
            clicks.push_back(*reinterpret_cast<glm::vec2*> (argument.get()));
        }

        MenuObject::OnEvent(manager, id, argument);
    }

    virtual void Apply(Scene * scene, ObjectHandle object, double deltat) override {

        glm::vec2 p = (*scene)[object].getPosition(), s = (*scene)[object].getSize();

        // while there are clicks, loop through each one and check if it's in the button
        // if the click is in the button, dispatch an event
        while (clicks.begin() != clicks.end()) {
            glm::vec2 click = clicks.front();
            clicks.pop_front();

            printf("Received click\n");
            
            if (within_range(click.x, p.x, s.x) && within_range(click.y, p.y, s.y)) {
                scene->dispatchEvent(buttonpress, buttonid);
                printf("Dispatching button press %d\n", buttonid);
            }
        }

        MenuObject::Apply(scene, object, deltat);
    }

};

class PlanetViewController : public MenuObject {
private:

    TextureSampler * sampler;
    EventIn showmenu;
    std::vector<Material*> planetmats;
    
public:

    PlanetViewController(EventIn showmenu, EventIn hidemenu, TextureSampler * sampler, std::vector<Material*> planetmats) :
    MenuObject(showmenu, hidemenu), sampler(sampler), showmenu(showmenu), planetmats(planetmats) {
        
    }

    void OnEvent(EventManager* manager, Event id, const std::shared_ptr<void> argument) override {
        
        if(id == showmenu) {
            PlanetCollideEventArguments * collide_args = reinterpret_cast<PlanetCollideEventArguments*>(argument.get());
            
            sampler->setTexture(planetmats[collide_args->planet.type]->getSampler(0)->getTexture());
        }
        
        MenuObject::OnEvent(manager, id, argument);
    }


};

#endif /* MENU_HPP */

