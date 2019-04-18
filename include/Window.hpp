
#ifndef _WINDOW_H
#define _WINDOW_H

#include <GLFW/glfw3.h>
#include <typeinfo>
#include <map>
#include <set>
#include <list>

// Manages the glfwInit and glfwTerminate calls

class GLFWManager {
private:
    static int instances;

public:
    // real constructor, calls glfwInit when first instance

    GLFWManager() {
        if (instances == 0) {
            glfwInit();
        }
        instances++;
    }

    // destructor, calls glfwTerminate when last instance

    ~GLFWManager() {
        instances--;

        if (instances == 0) {
            glfwTerminate();
        }
    }

    // copy constructor, does nothing but increment instances

    GLFWManager(const GLFWManager & orig) {
        instances++;
    }

};

// Wraps a GLFW window

class Window {
private:
    GLFWwindow * wnd;

    // a dummy object which controls glfwInit
    GLFWManager manager;

    const char * title;

public:

    Window(int width, int height, const char * title) {
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        wnd = glfwCreateWindow(width, height, title, nullptr, nullptr);

        this->title = title;
    }

    ~Window(void) {
        glfwDestroyWindow(wnd);
    }

    Window(const Window & other) = delete;

    bool shouldClose(void) const {
        return glfwWindowShouldClose(wnd);
    }

    void pollEvents(void) {
        glfwPollEvents();
    }

    const char * getTitle(void) {
        return title;
    }

    GLFWwindow * getWindow(void) const {
        return wnd;
    }

    uint32_t getWidth(void) {
        int w, h;

        glfwGetWindowSize(wnd, &w, &h);
        return w;
    }

    uint32_t getHeight(void) {
        int w, h;

        glfwGetWindowSize(wnd, &w, &h);
        return h;
    }

};

//forward reference so that the delegate interfaces can use the input handler as a parameter
class InputHandler;

// A delegate which is invoked when a key is pressed or released

class GLFWKeyDelegate {
public:

    virtual void KeyDown(InputHandler * handler, int key, int scancode, int action, int modifiers) {
    }

    virtual void KeyUp(InputHandler * handler, int key, int scancode, int action, int modifiers) {
    }
};

// A delegate which is invoked when the mouse is moved or a button is pressed

class GLFWMouseDelegate {
public:

    virtual void MouseButton(InputHandler * handler, int button, int action, int modifiers, double x, double y) {
    }

    virtual void MouseMove(InputHandler * handler, double x, double y) {
    }
};

// Controls the window's input. Do not use the glfwSet...Callback functions when
// using this

class InputHandler {
private:

    std::list<GLFWKeyDelegate*> allkeys_delegates;
    std::multimap<int, GLFWKeyDelegate*> keydelegates;
    std::multimap<int, GLFWMouseDelegate*> mousedelegates;
    std::set<GLFWMouseDelegate*> mousemovedelegates;

    std::set<int> pressedkeys;

    double x = 0, y = 0;

    Window & window;

    static std::map<GLFWwindow*, InputHandler*> handlers;

public:

    InputHandler(Window & window) : window(window) {
        handlers[window.getWindow()] = this;

        glfwSetKeyCallback(window.getWindow(), KeyCallback);
        glfwSetMouseButtonCallback(window.getWindow(), MouseButtonCallback);
        glfwSetCursorPosCallback(window.getWindow(), MouseMoveCallback);
    }

    void addKeyHandler(GLFWKeyDelegate * handler) {
        allkeys_delegates.push_back(handler);
    }

    void addKeyHandler(int key, GLFWKeyDelegate * handler) {
        keydelegates.insert(std::make_pair(key, handler));
    }

    void addMouseHandler(int button, GLFWMouseDelegate * handler) {
        mousedelegates.insert(std::make_pair(button, handler));
    }

    void addMouseMoveHandler(GLFWMouseDelegate * handler) {
        mousemovedelegates.insert(handler);
    }

    double getMouseX() {
        return x;
    }

    double getMouseY() {
        return y;
    }

    bool isKeyDown(int key) {
        return pressedkeys.find(key) != pressedkeys.end();
    }

private:

    // INTERAL HACKY METHODS

    static void KeyCallback(GLFWwindow * window, int key, int scancode, int action, int modifiers) {
        try {
            handlers.at(window)->OnKeyCallback(key, scancode, action, modifiers);
        } catch (std::out_of_range) {
            return;
        }
    }

    static void MouseButtonCallback(GLFWwindow* window, int button, int action, int modifiers) {
        try {
            handlers.at(window)->OnMouseButtonCallback(button, action, modifiers);
        } catch (std::out_of_range) {
            return;
        }
    }

    static void MouseMoveCallback(GLFWwindow* window, double xpos, double ypos) {
        try {
            handlers.at(window)->OnMouseMoveCallback(xpos, ypos);
        } catch (std::out_of_range) {
            return;
        }
    }

    void OnKeyCallback(int key, int scancode, int action, int modifiers) {
        if (action == GLFW_PRESS) {
            pressedkeys.insert(key);
            for (auto & handler : keydelegates) {
                if (handler.first == key) {
                    handler.second->KeyDown(this, key, scancode, action, modifiers);
                }
            }
            for (auto keyhandler : allkeys_delegates) {
                keyhandler->KeyDown(this, key, scancode, action, modifiers);
            }
        } else if (action == GLFW_RELEASE) {
            pressedkeys.erase(key);
            for (auto & handler : keydelegates) {
                if (handler.first == key) {
                    handler.second->KeyUp(this, key, scancode, action, modifiers);
                }
            }
            for (auto keyhandler : allkeys_delegates) {
                keyhandler->KeyUp(this, key, scancode, action, modifiers);
            }
        }

    }

    void OnMouseButtonCallback(int button, int action, int modifiers) {
        for (auto & handler : mousedelegates) {
            if (handler.first == button) {
                handler.second->MouseButton(this, button, action, modifiers, x, y);
            }
        }
    }

    void OnMouseMoveCallback(double x, double y) {
        this->x = x / window.getWidth() * 2 - 1;
        this->y = y / window.getHeight() * 2 - 1;

        for (auto & movedelegate : mousemovedelegates) {
            movedelegate->MouseMove(this, this->x, this->y);
        }
    }

};

#endif /* _WINDOW_H */
