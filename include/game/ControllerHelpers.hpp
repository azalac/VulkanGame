/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   ControllerHelpers.hpp
 * Author: austin-z
 *
 * Created on April 11, 2019, 5:00 PM
 */

#ifndef CONTROLLERHELPERS_HPP
#define CONTROLLERHELPERS_HPP

#include "scene.hpp"

/**
 * Checks if a number is within a range with a median.
 * @param test The number
 * @param center The range center
 * @param deviation The range deviation
 * @return test >= center - deviation && test <= center + deviation
 */
template<typename A, typename B, typename C>
static bool within_range(A test, B center, C deviation) {
    return test >= center - deviation && test <= center + deviation;
}

/**
 * Checks if a number is within two numbers.
 * @param test The number
 * @param lower The lower bound
 * @param upper The upper bound
 * @return test >= lower && test <= upper
 */
template<typename A, typename B, typename C>
static bool within_bounds(A test, B lower, C upper) {
    return test >= lower && test <= upper;
}

/**
 * Gets the sign of a number
 * @param val The number
 * @return -1, 0, or 1
 */
template <typename T>
static int signum(T val) {
    return (T(0) < val) - (val < T(0));
}

/**
 * Gets a random number from 0 to the parameter.
 * @param max_exclusive The maximum output (exclusive)
 * @return The random number
 */
template<typename T>
T rand(T max_exclusive) {
    return (T) (static_cast<double> (std::rand()) / RAND_MAX * max_exclusive);
}

/**
 * Gets a random boolean with a certain chance
 * @param chance The chance [0 .. 1]
 * @return The boolean
 */
bool rand_bool(double chance) {
    return static_cast<double> (std::rand()) / RAND_MAX < chance;
}

struct KeyEvent {
    InputHandler * input;
    int keycode;
    int action;
    int modifiers;
    
    KeyEvent(InputHandler* input, int keycode, int action, int modifiers) :
    input(input), keycode(keycode), action(action), modifiers(modifiers) {
    }

};

struct SoundRequest {
    std::string file;
    int loops = 1;
    glm::vec2 screen_pos = {0.0f, 0.0f};
    bool has_pos = false;

    SoundRequest() {
        
    }
    
    SoundRequest(const std::string filename, int loops = 1) :
    file(filename), loops(loops), has_pos(false) {
        
    }

    SoundRequest(const std::string filename, int loops, const glm::vec2 & pos) :
    file(filename), loops(loops), screen_pos(pos), has_pos(true) {

    }
};

#endif /* CONTROLLERHELPERS_HPP */

