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

class MenuObject : public ObjectDecorator, public EventHandler {
private:
    Event showmenu;
    Event hidemenu;

    bool menuvisible = false;

public:

    MenuObject(Event showmenu, Event hidemenu) {
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

class MenuButton : public MenuObject {
private:

    Event buttonpress, mouseclick;
    std::list<glm::vec2> clicks;
    int buttonid;

public:

    MenuButton(Event mouseclick, Event showmenu, Event buttonpress, int buttonid) :
    MenuObject(showmenu, buttonpress) {
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

        // for every click, if it's within the button, send a button press event
        while (clicks.begin() != clicks.end()) {
            glm::vec2 click = (clicks.front() - (*scene)[object].getPosition()) / (*scene)[object].getSize();
            clicks.pop_front();
            
            if (click.x > -1 && click.x < 1 && click.y > -1 && click.y < 1) {
                scene->dispatchEvent(buttonpress, std::shared_ptr<int>(new int(buttonid)));
            }
        }

        MenuObject::Apply(scene, object, deltat);
    }

};

#endif /* MENU_HPP */

