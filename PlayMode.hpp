#include "Mode.hpp"

#include "Scene.hpp"
#include "Sound.hpp"

#include <glm/glm.hpp>

#include <vector>
#include <deque>

struct PlayMode : Mode {
	PlayMode();
	virtual ~PlayMode();

	//functions called by main loop:
	virtual bool handle_event(SDL_Event const &, glm::uvec2 const &window_size) override;
	virtual void update(float elapsed) override;
	virtual void draw(glm::uvec2 const &drawable_size) override;

	//----- game state -----

	//input tracking:
	struct Button {
		uint8_t downs = 0;
		uint8_t pressed = 0;
	} left, right, down, up;

	//local copy of the game scene (so code can change it during gameplay):
	Scene scene;

	//hexapod leg to wobble:
	Scene::Transform *ground = nullptr;
	Scene::Transform *upper_leg = nullptr;
	Scene::Transform *lower_leg = nullptr;
	Scene::Transform *playerBall=nullptr;

	std::vector<Scene::Transform*> coins;
	std::vector<Scene::Transform*> colliders;
	int currentCoinEaten=0;

	glm::quat hip_base_rotation;
	glm::quat upper_leg_base_rotation;
	glm::quat lower_leg_base_rotation;
	float wobble = 0.0f;

	glm::vec3 get_leg_tip_position();

	//music coming from the tip of the leg (as a demonstration):
	std::shared_ptr< Sound::PlayingSample > leg_tip_loop;
	std::shared_ptr< Sound::PlayingSample > upEffect;
	std::shared_ptr< Sound::PlayingSample > downEffect;

	//camera:
	Scene::Camera *camera = nullptr;
	float SnowBallWeight=1.0f;
	bool win=false;
	int currentIndex=0;
	glm::vec3 currentSpeed= glm::vec3(0,0,0);
	glm::vec3 currentForce= glm::vec3(0,0,0);

};
