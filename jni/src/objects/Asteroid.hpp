/*
 * Asteroid.h
 *
 *  Created on: Sep 25, 2012
 *      Author: juho
 */

#ifndef ASTEROID_H_
#define ASTEROID_H_

#include "Box2D/Box2D.h"
#include "Object.hpp"
#include "interfaces/Drawable.hpp"
#include "interfaces/Timed.hpp"
#include "Assets.hpp"

class Asteroid: public Object, public Drawable, public Timed
{
public:
	Asteroid(b2Body* planet);
	virtual ~Asteroid();

	// --- Drawable ---
	inline Sprite* GetSprite() const { return type_.sprite; }
	inline b2Vec2 GetDimensions() const { return type_.meters; }

	void Tick();

	inline static unsigned int Count() { return count_; }

private:
	Assets::Type type_;

	static unsigned int count_;

	Uint32 previousSlow_;
};

#endif /* ASTEROID_H_ */
