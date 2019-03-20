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

class MenuButton : public MenuObject, public GLFWMouseDelegate {
private:

	Event onpress;
	CoordinateConverter * converter;

	std::list<glm::vec2> clicks;

public:
	MenuButton(Event showmenu, Event onpress, CoordinateConverter * converter) : MenuObject(showmenu, onpress) {
		this->onpress = onpress;
		this->converter = converter;
	}

	virtual void Apply(Scene * scene, ObjectHandle object, double deltat) override {
		
		// for every click, check if it's in the object
		while (clicks.begin() != clicks.end()) {
			glm::vec2 click = (clicks.front() - (*scene)[object].getPosition()) / (*scene)[object].getSize();
			clicks.pop_front();

			if (click.x > -1 && click.x < 1 && click.y > -1 && click.y < 1) {
				scene->dispatchEvent(onpress, nullptr);
			}
		}

		MenuObject::Apply(scene, object, deltat);
	}

	virtual void MouseButton(InputHandler * handler, int button, int action, int modifiers, double x, double y) {
		clicks.push_back(glm::vec2(x, y) / converter->getScale());
	}
};

#endif /* MENU_HPP */

