#include <cstdint>
#include "SDL.h"
#include "SDL_log.h"
#include "SDL_ttf.h"

#include "Screen.hpp"
#include "Game.hpp"
#include "interfaces/Drawable.hpp"
#include "interfaces/Touchable.hpp"
#include "objects/Sprite.hpp"
#include "objects/Planet.hpp"
#include "DebugDraw.hpp"
#include "UI.hpp"

#if SDL_BYTEORDER == SDL_BIG_ENDIAN
Uint32 Screen::RMASK = 0xff000000;
Uint32 Screen::GMASK = 0x00ff0000;
Uint32 Screen::BMASK = 0x0000ff00;
Uint32 Screen::AMASK = 0x000000ff;
#else
Uint32 Screen::RMASK = 0x000000ff;
Uint32 Screen::GMASK = 0x0000ff00;
Uint32 Screen::BMASK = 0x00ff0000;
Uint32 Screen::AMASK = 0xff000000;
#endif

Screen::Screen():
	window_(NULL),
	renderer_(NULL),
	surface_(NULL),
	font_(NULL),
	w_(0),
	h_(0),
	pixelsPerMeter_(0),
	debug_(false),
	debugger_(NULL),
	touchPoint_(0, 0),
	touchObject_(NULL),
	ui_(NULL)
{
}

Screen::~Screen()
{
}

void Screen::init()
{
	// Initialize SDL.
	if (SDL_Init(SDL_INIT_VIDEO) < 0) // | SDL_INIT_AUDIO
	{
		return;
	}

	if (TTF_Init() == -1)
	{
		printf("TTF_Init: %s\n", TTF_GetError());
		return;
	}

	// Debug SDL, on Android messages go to log, on Linux to stderr.
	SDL_LogSetAllPriority(SDL_LOG_PRIORITY_INFO);

	/*
	 * On android, this resolution is ignored and devices screen resolution
	 * is used instead.
	 * Windows are never resizable on Android.
	 */
	window_ = SDL_CreateWindow("PELI",
	                           SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
	                           1280, 900, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);

	// Get the first available Hardware-accelerated renderer for this window.
	renderer_ = SDL_CreateRenderer(window_, -1, SDL_RENDERER_ACCELERATED);

	unsigned int size = 24;
	font_ = TTF_OpenFont("visitor1.ttf", size);
	if (!font_)
	{
		printf("TTF_OpenFont: %s\n", TTF_GetError());
		return;
	}

	resized();

	debugger_ = new DebugDraw();
	debugger_->SetFlags(b2Draw::e_shapeBit
	                    | b2Draw::e_aabbBit
	                    | b2Draw::e_jointBit
	                    | b2Draw::e_centerOfMassBit);
	Game::instance().World()->SetDebugDraw(debugger_);

	ui_ = new UI();
}

void Screen::destroy()
{
	SDL_DestroyRenderer(renderer_);
	SDL_DestroyWindow(window_);
	SDL_Quit();
}

void Screen::draw()
{
	// Black base color.
	SDL_SetRenderDrawColor(renderer_, 0, 0, 0, 255);
	// Clear the entire screen to the Renderer's base colour.
	SDL_RenderClear(renderer_);

	b2Body* body = Game::instance().World()->GetBodyList();
	while (body != NULL) {
		Drawable* obj = dynamic_cast<Drawable*>((Object*)body->GetUserData());
		if (obj != NULL) {
			obj->Draw(body);
		}

		body = body->GetNext();
	}

	ui_->Draw();

	if (debug_)
	{
		Game::instance().World()->DrawDebugData();
	}

	// Flip the shown and hidden buffers to refresh the screen.
	SDL_RenderPresent(renderer_);
}

// bool Screen::ReportFixture(b2Fixture *fixture)
// {
// 	// SDL_Log("Fixture touched");
// 	b2Body* body = fixture->GetBody();
// 	Touchable* touchable = dynamic_cast<Touchable*>((Object*)body->GetUserData());
// 	if (touchable != NULL && fixture->TestPoint(touchPoint_)) {
// 		touchObject_ = touchable;
// 	}
// 	return true;
// }

std::pair<unsigned int, unsigned int> Screen::TouchPosition(const SDL_Event &event)
{
	std::pair<unsigned int, unsigned int> p;
	if (event.type == SDL_MOUSEBUTTONDOWN
	 || event.type == SDL_MOUSEMOTION
	 || event.type == SDL_MOUSEBUTTONUP)
	{
		p.first = event.button.x;
		p.second = event.button.y;
	}
	else if (event.type == SDL_FINGERDOWN
	      || event.type == SDL_FINGERMOTION
	      || event.type == SDL_FINGERUP)
	{
		// Quess: Touch points are represented with values from
		// 0 to 32768
		p.first = (event.tfinger.x * w_) / 32768;
		p.second = (event.tfinger.y * h_) / 32768;
	}

	return p;
}

b2Vec2 Screen::TouchPositionMeters(const SDL_Event &event)
{
	auto p2 = TouchPosition(event);

	b2Vec2 p(p2.first, p2.second);
	p.x -= w_ / 2;
	p.y -= h_ / 2;

	return toMeters(p);
}

void Screen::processInput()
{
	static SDL_Event event;
	while (SDL_PollEvent(&event))
	{
		if (event.type == SDL_QUIT)
		{
			Game::instance().Stop();
		}
		else if (event.type == SDL_WINDOWEVENT)
		{
			if (event.window.event == SDL_WINDOWEVENT_RESIZED)
			{
				resized();
			}
		}
		// On android duplicate events are sent because of Mouse emulation which
		// can't be turned of... So for we'll ignore finger events.
		else if ((event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT)
			  /*|| (event.type == SDL_FINGERDOWN)*/)
		{
			touchPoint_ = TouchPositionMeters(event);

			// b2AABB aabb;
			// aabb.lowerBound = touchPoint_;
			// aabb.upperBound = touchPoint_;

			// // This will call ReportFixture.
			// Game::instance().World()->QueryAABB(this, aabb);

			if (!ui_->TouchStart(TouchPosition(event))) {
				// touchObject_ = (Touchable*)Game::instance().GetPlanet();
				if (Game::instance().GetPlanet() != NULL)
				{
					Game::instance().GetPlanet()->TouchStart();
					Game::instance().GetPlanet()->TouchMovement(touchPoint_);
				}
			}
		}
		else if (event.type == SDL_MOUSEMOTION
			  /*|| event.type == SDL_FINGERMOTION*/)
		{
			if (!ui_->Touch(TouchPosition(event)))
			{
				if (Game::instance().GetPlanet() != NULL)
				{
					Game::instance().GetPlanet()->TouchMovement(TouchPositionMeters(event));
				}
			}
		}
		else if ((event.type == SDL_MOUSEBUTTONUP && event.button.button == SDL_BUTTON_LEFT)
			  /*|| (event.type == SDL_FINGERUP)*/)
		{
			SDL_Log("Joku event");
			if (!ui_->TouchEnd())
			{
				if (Game::instance().GetPlanet() != NULL)
				{
					Game::instance().GetPlanet()->TouchMovement(TouchPositionMeters(event));
					Game::instance().GetPlanet()->TouchEnd();
					// Game::instance().GetPlanet() = NULL;
				}
			}
		}
		else if (event.type == SDL_KEYUP)
		{
			if (event.key.keysym.sym == SDLK_1)
			{
				Game::instance().SelectWeapon(Bomb::BombType::NORMAL);
			}
			else if (event.key.keysym.sym == SDLK_2)
			{
				Game::instance().SelectWeapon(Bomb::BombType::SPLASH);
			}
			else if (event.key.keysym.sym == SDLK_3)
			{
				Game::instance().SelectWeapon(Bomb::BombType::CHAIN);
			}
			else if (event.key.keysym.sym == SDLK_4)
			{
				Game::instance().SelectWeapon(Bomb::BombType::LASER);
			}
		}
		else if (event.type == SDL_USEREVENT)
		{
			Game::instance().HandleEvent(event);
		}
	}
}

void Screen::resized()
{
	SDL_GetWindowSize(window_, &w_, &h_);

	SDL_Rect rect;
	rect.x = 0;
	rect.y = 0;
	rect.w = w_;
	rect.h = h_;

	// Compute new pixels per meter
	// Planet is 4 meters and should be 128px on 800px wide screen.

	pixelsPerMeter_ = 480.0 * std::min(static_cast<float>(w_) / DEF_SCREEN_WIDTH,
                                       static_cast<float>(h_) / DEF_SCREEN_HEIGHT)
                      / 16.0;

	SDL_Log("Screen resolution %ix%i, pixels per meter %i", w_, h_, pixelsPerMeter_);

	// Render viewport needs to be fixed after resize or it will be
	// messed up after window shrinking.
	SDL_RenderSetViewport(renderer_, &rect);
}

b2Vec2 Screen::toPixels(const b2Vec2 &coord, bool center) const
{
	b2Vec2 copy(coord);
	copy *= pixelsPerMeter_;
	if (center)
	{
		copy.x += w_ / 2;
		copy.y += h_ / 2;
	}
	return copy;
}

b2Vec2 Screen::toMeters(const b2Vec2 &coord) const
{
	b2Vec2 copy(coord.x / pixelsPerMeter_, coord.y / pixelsPerMeter_);
	return copy;
}

SDL_Surface* Screen::CreateSurface(int width, int height, int depth)
{
	return SDL_CreateRGBSurface(0, 1, 1, 32, RMASK, GMASK, BMASK, AMASK);
}

SDL_Surface* Screen::CreateSurfaceFrom(void *data, int width, int height, int depth, int pitch)
{
	return SDL_CreateRGBSurfaceFrom(data, width, height, depth, pitch, RMASK, GMASK, BMASK, AMASK);
}
