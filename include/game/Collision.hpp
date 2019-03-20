
#ifndef OBJECT_COLLISION_H
#define OBJECT_COLLISION_H

#include <glm/glm.hpp>
#include <vector>

#define _USE_MATH_DEFINES
#include <math.h>

// these two types are the same, but they're different mathimatically
using Point = glm::vec2;
using Direction = glm::vec2;

template<typename T>
struct Optional {
	T value;
	bool has_value;

	Optional(T value) {
		this->value = value;
		has_value = true;
	}

	Optional(nullptr_t) {
		value = default(T);
		has_value = false;
	}

	Optional() {
		value = default(T);
		has_value = false;
	}

	operator bool() {
		return has_value;
	}

	operator T&() {
		return value;
	}

	operator T() {
		return value;
	}
};

struct Ray {
	Point origin;
	Direction direction;
};

struct Line {
	Point p;
	Direction d;

	Point at(double t) const{
		return { t * d.x + p.x, t * d.y + p.y };
	}

	Optional<double> at_inv(Point p) const {
		p = (p - this->p) / d;
		if (p.x == p.y) {
			return p.x;
		}
		return nullptr;
	}
};

struct Circle {
	Point center;
	double radius;
};

template<typename T>
struct HitInfo {
	T value;
	double length;

	HitInfo() : value(default(T)), length(0) {}

	HitInfo(T & value, double length) : value(value), length(length) {}

	HitInfo(T && value, double length) : value(value), length(length) {}

	operator T() {
		return value;
	}
};

namespace intersectors {
	
	// Multiplies a vector by a scalar and adds another vector
	glm::vec2 FMA(double k, const glm::vec2 & m, const glm::vec2 & a) {
		return { k * m.x + a.x, k * m.y + a.y };
	}

	// Projects a point onto a line
	static Point project(const Line & line, const  Point & point) {
		double a = glm::dot(point - line.p, line.d) / 2;
		return line.at(a);
	}

	// Finds the intersection of two lines. The lines' directions must be normalized
	static Optional<HitInfo<Point>> lxl_normalized(const Line & one, const Line & two) {
		if (one.d == two.d) {
			// directions are the same, no intersection possible
			return nullptr;
		}

		glm::vec2 delta = one.p - two.p;

		// calculate the determinants of the coefficients
		double denom = glm::determinant(glm::mat2x2{ one.d, two.d });
		double x_det = glm::determinant(glm::mat2x2{ delta, two.d });
		//double y_num = glm::determinant(glm::mat2x2{ one.d, delta });

		// calculate the position
		double t = x_det / denom;
		return HitInfo<Point>{ one.at(t), t };
	}

	// Finds the intersection of two lines. The lines' directions do not have to be normalized
	static Optional<HitInfo<Point>> lxl(Line one, Line two) {
		one.d = glm::normalize(one.d);
		two.d = glm::normalize(two.d);
		return lxl_normalized(one, two);
	}

	// Finds the two intersections between a line and circle
	// Results are guaranteed sorted by distance, ascending order
	static std::vector<Point> cxl(const Circle & circle, const Line & line) {

		// find the intersection's midpoint and its distance from the line
		Point a = project(line, circle.center) + line.p;

		Direction y = circle.center - a;
		double len2 = glm::dot(y, y);
		
		double r2 = circle.radius * circle.radius;

		// the intersection midpoint is outside of the circle, therefore no intersections
		if (len2 > r2) {
			return {};
		}

		// the radius and the length of Y are the same, therefore one intersection
		if (len2 == r2) {
			return std::vector<Point>{ a };
		}

		// find the delta t for the intersections
		double x = sqrt(r2 - len2);

		auto t = line.at_inv(a);
		// if a is behind the requested point, x must be flipped
		// this has no functional change, but it satisfies the 'lower must come first' requirement
		if (t && t.value < 0) {
			x *= -1;
		}

		// return a + x * d and a - x * d (the two intersections)
		return std::vector<Point>{FMA(-x, line.d, a), FMA(x, line.d, a) };
	}

	// Collides a circle moving along a line with a static circle
	// Results are guaranteed sorted by distance, ascending order
	static std::vector<Circle> cxc(const Circle & circle, const Line & line, double radius) {

		// find the midpoint and its distance from the line
		Point a = project(line, circle.center) + line.p;

		Direction y = circle.center - a;
		double len2 = glm::dot(y, y);

		double r_tot = (circle.radius + radius) * (circle.radius + radius);

		// the intersection midpoint is outside of the circle, therefore no intersections
		if (len2 > r_tot) {
			return {};
		}

		// the radius and the length of Y are the same, therefore one intersection
		if (len2 == r_tot) {
			return std::vector<Circle>{ Circle{a,radius} };
		}

		// find the delta t for the intersections
		double x = sqrt(r_tot - len2);

		auto t = line.at_inv(a);
		// if a is behind the requested point, x must be flipped
		// this has no functional change, but it satisfies the 'lower must come first' requirement
		if (t && t.value < 0) {
			x *= -1;
		}

		return { Circle{FMA(-x, line.d, a), radius}, Circle{FMA(x, line.d, a), radius} };
	}

};

#endif /* OBJECT_COLLISION_H */
