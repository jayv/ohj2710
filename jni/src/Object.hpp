/*
 * Object.h
 *
 *  Created on: Sep 24, 2012
 *      Author: juho
 */

#ifndef OBJECT_H_
#define OBJECT_H_

#include "SDL_log.h"
#include "Box2D/Box2D.h"

class Sprite;

class Object
{
public:
	Object(): body_(NULL) {};
	virtual ~Object() {};

	void CreateBody(b2BodyDef& def, b2FixtureDef* def2);
	inline b2Body* GetBody() const { return body_; }
	inline virtual float GetMass() const { return body_->GetMass(); }
private:
	b2Body* body_;
};

#endif /* OBJECT_H_ */
