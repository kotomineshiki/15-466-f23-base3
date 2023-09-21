#include "PlayMode.hpp"

#include "LitColorTextureProgram.hpp"

#include "DrawLines.hpp"
#include "Mesh.hpp"
#include "Load.hpp"
#include "gl_errors.hpp"
#include "data_path.hpp"

#include <glm/gtc/type_ptr.hpp>

#include <random>

GLuint hexapod_meshes_for_lit_color_texture_program = 0;
Load< MeshBuffer > hexapod_meshes(LoadTagDefault, []() -> MeshBuffer const * {
	MeshBuffer const *ret = new MeshBuffer(data_path("MyTestScene.pnct"));
	hexapod_meshes_for_lit_color_texture_program = ret->make_vao_for_program(lit_color_texture_program->program);
	return ret;
});

Load< Scene > hexapod_scene(LoadTagDefault, []() -> Scene const * {
	return new Scene(data_path("MyTestScene.scene"), [&](Scene &scene, Scene::Transform *transform, std::string const &mesh_name){
		Mesh const &mesh = hexapod_meshes->lookup(mesh_name);

		if(transform->name.substr(0,4)=="Coll"){

		}else{
			scene.drawables.emplace_back(transform);
		}
		Scene::Drawable &drawable = scene.drawables.back();

		drawable.pipeline = lit_color_texture_program_pipeline;

		drawable.pipeline.vao = hexapod_meshes_for_lit_color_texture_program;
		drawable.pipeline.type = mesh.type;
		drawable.pipeline.start = mesh.start;
		drawable.pipeline.count = mesh.count;

	});
});

Load< Sound::Sample > dusty_floor_sample(LoadTagDefault, []() -> Sound::Sample const * {
	return new Sound::Sample(data_path("BeepBox-Song.wav"));
});
Load< Sound::Sample > UpEffect(LoadTagDefault, []() -> Sound::Sample const * {
	return new Sound::Sample(data_path("BBSEUP.wav"));
});
Load< Sound::Sample > DownEffect(LoadTagDefault, []() -> Sound::Sample const * {
	return new Sound::Sample(data_path("BBSEDown.wav"));
});

PlayMode::PlayMode() : scene(*hexapod_scene) {
	//get pointers to leg for convenience:
	for (auto &transform : scene.transforms) {
		if (transform.name == "Ground") ground = &transform;
		if (transform.name == "UpperLeg.FL") upper_leg = &transform;
		if (transform.name == "LowerLeg.FL") lower_leg = &transform;
		if(transform.name=="Sphere")playerBall=&transform;
		if(transform.name.substr(0,4)=="Coin")coins.push_back(&transform);
		if(transform.name.substr(0,4)=="Coll")colliders.push_back(&transform);
	}

	if (ground == nullptr) throw std::runtime_error("ground not found.");
	if (playerBall == nullptr) throw std::runtime_error("Sphere not found.");
	if (lower_leg == nullptr) throw std::runtime_error("Lower leg not found.");
	std::cout<<coins.size()<<std::endl;

	playerBall->position.z=3.0f;
	for(int i=0;i<coins.size();++i){
		coins[i]->position=playerBall->position+glm::vec3(0,(1.2f+1.6f*currentIndex)*5.0f,0.0f);
		currentIndex++;
		if(i%2==0){
			coins[i]->position.z=5.0f;
		}else{
			coins[i]->position.z=1.0f;
		}

	}

	hip_base_rotation = ground->rotation;
	upper_leg_base_rotation = coins[0]->rotation;
	lower_leg_base_rotation = lower_leg->rotation;

	//get pointer to camera for convenience:
	if (scene.cameras.size() != 1) throw std::runtime_error("Expecting scene to have exactly one camera, but it has " + std::to_string(scene.cameras.size()));
	camera = &scene.cameras.front();

	//start music loop playing:
	// (note: position will be over-ridden in update())
	leg_tip_loop = Sound::loop_3D(*dusty_floor_sample, 1.0f, get_leg_tip_position(), 10.0f);
	
}

PlayMode::~PlayMode() {
}

bool PlayMode::handle_event(SDL_Event const &evt, glm::uvec2 const &window_size) {

	if (evt.type == SDL_KEYDOWN) {
		if (evt.key.keysym.sym == SDLK_ESCAPE) {
			SDL_SetRelativeMouseMode(SDL_FALSE);
			return true;
		} else if (evt.key.keysym.sym == SDLK_a) {
			left.downs += 1;
			left.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_d) {
			right.downs += 1;
			right.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_w) {
			up.downs += 1;
			up.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_s) {
			down.downs += 1;
			down.pressed = true;
			return true;
		}
	} else if (evt.type == SDL_KEYUP) {
		if (evt.key.keysym.sym == SDLK_a) {
			left.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_d) {
			right.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_w) {
			up.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_s) {
			down.pressed = false;
			return true;
		}
	} else if (evt.type == SDL_MOUSEBUTTONDOWN) {
		if (SDL_GetRelativeMouseMode() == SDL_FALSE) {
			SDL_SetRelativeMouseMode(SDL_TRUE);
			return true;
		}
	} else if (evt.type == SDL_MOUSEMOTION) {
		if (SDL_GetRelativeMouseMode() == SDL_TRUE) {
			glm::vec2 motion = glm::vec2(
				evt.motion.xrel / float(window_size.y),
				-evt.motion.yrel / float(window_size.y)
			);
			camera->transform->rotation = glm::normalize(
				camera->transform->rotation
				* glm::angleAxis(-motion.x * camera->fovy, glm::vec3(0.0f, 1.0f, 0.0f))
				* glm::angleAxis(motion.y * camera->fovy, glm::vec3(1.0f, 0.0f, 0.0f))
			);
			return true;
		}
	}

	return false;
}

void PlayMode::update(float elapsed) {

	//slowly rotates through [0,1):
	wobble += elapsed / 10.0f;
	wobble -= std::floor(wobble);

	/*hip->rotation = hip_base_rotation * glm::angleAxis(
		glm::radians(5.0f * std::sin(wobble * 2.0f * float(M_PI))),
		glm::vec3(0.0f, 1.0f, 0.0f)
	);*/
	upper_leg->rotation = upper_leg_base_rotation * glm::angleAxis(
		glm::radians(7.0f * std::sin(wobble * 2.0f * 2.0f * float(M_PI))),
		glm::vec3(0.0f, 0.0f, 1.0f)
	);
	lower_leg->rotation = lower_leg_base_rotation * glm::angleAxis(
		glm::radians(10.0f * std::sin(wobble * 3.0f * 2.0f * float(M_PI))),
		glm::vec3(0.0f, 0.0f, 1.0f)
	);

	//move sound to follow leg tip position:
	leg_tip_loop->set_position(get_leg_tip_position(), 1.0f / 60.0f);

	//move camera:
	{

		//combine inputs into a move:
		constexpr float PlayerSpeed = 30.0f;
		glm::vec2 move = glm::vec2(0.0f);
	//	if (left.pressed && !right.pressed) move.x =-1.0f;
	//	if (!left.pressed && right.pressed) move.x = 1.0f;
		if (down.pressed && !up.pressed){
			playerBall->position.z=1.0f;
		}
		if (!down.pressed && up.pressed){
			playerBall->position.z=5.0f;
		} 
		if(!down.pressed && !up.pressed){
			playerBall->position.z=3.0f;
		}

		//make it so that moving diagonally doesn't go faster:
		if (move != glm::vec2(0.0f)) move = glm::normalize(move) * PlayerSpeed;// * elapsed;

		glm::mat4x3 worldPosition=ground->make_local_to_world();//get the world vector of the ground
		glm::vec3 world_right=glm::normalize(worldPosition[0]);
		glm::vec3 world_forward=glm::normalize(worldPosition[1]);
		glm::vec3 world_up=-glm::normalize(worldPosition[2]);

		glm::mat4x3 frame = camera->transform->make_local_to_parent();
		glm::vec3 frame_right = frame[0];
		//glm::vec3 up = frame[1];
		glm::vec3 frame_forward = -frame[2];

		//currentForce//(move.x * world_right + move.y * world_forward)*50.0f-currentSpeed*1.0f;
		//currentSpeed+=currentForce*elapsed;
		//currentSpeed=move.y*world_forward*elapsed*10.0f;
		currentSpeed=glm::vec3(0,5,0);
		//std::cout<<currentSpeed.length()*elapsed*40<<std::endl;
		playerBall->scale=glm::vec3(SnowBallWeight,SnowBallWeight,SnowBallWeight);

		//camera->transform->position += move.x * frame_right + move.y * frame_forward;
	}

	{ //update listener to camera position:
		glm::mat4x3 frame = camera->transform->make_local_to_parent();
		glm::vec3 frame_right = frame[0];
		glm::vec3 frame_at = frame[3];
		Sound::listener.set_position_right(frame_at, frame_right, 1.0f / 60.0f);
	}
	//Rotate Coins
	for(int i=0;i<coins.size();++i){
		//glm::quat=glm::quat(,);
		coins[i]->rotation = coins[i]->rotation * glm::angleAxis(
		glm::radians(700.0f * elapsed),
		glm::vec3(1.0f, 0.0f, 0.0f)
	);
	}
	//Detect Collision between Coins and Player
	for(int i=0;i<coins.size();++i){
		glm::vec3 distance=coins[i]->position-playerBall->position;
	//	std::cout<<glm::length(distance)<<std::endl;
		if(glm::length(distance)<SnowBallWeight){
			std::cout<<"Coin Touched"<<std::endl;
			currentCoinEaten++;

			if(coins[i]->position.z>4.0f){
				Sound::play(*UpEffect, 1.0f, 1.0f);
			}else{
				Sound::play(*DownEffect, 1.0f, 1.0f);
			}

			coins[i]->position=glm::vec3(playerBall->position.x,(1.2f+1.6f*currentIndex)*5.0f,0.0f);
			if(currentIndex%2==0){
				coins[i]->position.z=5.0f;
			}else{
				coins[i]->position.z=1.0f;
			}
			currentIndex++;
			//coins[i]->position=glm::vec3(1000.0f,0,0);
		//	SnowBallWeight=SnowBallWeight+0.15f;

		}
		if(coins[i]->position.y<playerBall->position.y-10){
			coins[i]->position=glm::vec3(playerBall->position.x,(1.2f+1.6f*currentIndex)*5.0f,0.0f);
			if(currentIndex%2==0){
				coins[i]->position.z=5.0f;
			}else{
				coins[i]->position.z=1.0f;
			}
			currentIndex++;
		}
	}
	//Detect Collision between Player and Colliders
	for(int i=0;i<colliders.size();++i){
		glm::vec3 distance=colliders[i]->position-playerBall->position;

		if(glm::length(distance)<0.7*glm::length( SnowBallWeight+colliders[i]->scale*0.5f)){
			if(colliders[i]->name=="Collider.001"){
				if(currentCoinEaten>=10){
					win=true;
				}
			std::cout<<"FInal"<<std::endl;
		}
			//now, if the current vector is toward it ,stop it
			glm::vec3 nextframe=playerBall->position+currentSpeed*elapsed;
			glm::vec3 newDistance=nextframe-colliders[i]->position;
			if(glm::length(distance)>glm::length(newDistance)){
				currentSpeed=glm::vec3(0,0,0);
			}
		}
	}

	playerBall->position+=currentSpeed*elapsed;
	glm::vec3 temp=currentSpeed*elapsed;
	glm::vec3 tempRotate(-temp.y,temp.x,0);
	playerBall->rotation*=glm::quat(tempRotate);
	camera->transform->position =playerBall->position+glm::vec3(20.0f,0.0f,0.0f);
	camera->transform->position.z=3.0f;
	//reset button press counters:
	left.downs = 0;
	right.downs = 0;
	up.downs = 0;
	down.downs = 0;
}

void PlayMode::draw(glm::uvec2 const &drawable_size) {
	//update camera aspect ratio for drawable:
	camera->aspect = float(drawable_size.x) / float(drawable_size.y);

	//set up light type and position for lit_color_texture_program:
	// TODO: consider using the Light(s) in the scene to do this
	glUseProgram(lit_color_texture_program->program);
	glUniform1i(lit_color_texture_program->LIGHT_TYPE_int, 1);
	glUniform3fv(lit_color_texture_program->LIGHT_DIRECTION_vec3, 1, glm::value_ptr(glm::vec3(0.0f, 0.0f,-1.0f)));
	glUniform3fv(lit_color_texture_program->LIGHT_ENERGY_vec3, 1, glm::value_ptr(glm::vec3(1.0f, 1.0f, 0.95f)));
	glUseProgram(0);

	glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
	glClearDepth(1.0f); //1.0 is actually the default value to clear the depth buffer to, but FYI you can change it.
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS); //this is the default depth comparison function, but FYI you can change it.

	scene.draw(*camera);

	{ //use DrawLines to overlay some text:
		glDisable(GL_DEPTH_TEST);
		float aspect = float(drawable_size.x) / float(drawable_size.y);
		DrawLines lines(glm::mat4(
			1.0f / aspect, 0.0f, 0.0f, 0.0f,
			0.0f, 1.0f, 0.0f, 0.0f,
			0.0f, 0.0f, 1.0f, 0.0f,
			0.0f, 0.0f, 0.0f, 1.0f
		));

		constexpr float H = 0.09f;
		lines.draw_text("WASD moves the ball; escape ungrabs mouse",
			glm::vec3(-aspect + 0.1f * H, -1.0 + 0.1f * H, 0.0),
			glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
			glm::u8vec4(0x00, 0x00, 0x00, 0x00));
		lines.draw_text("Eat coins to grow up",
			glm::vec3(-aspect + 1.2f * H, -1.0 + 1.2f * H, 0.0),
			glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
			glm::u8vec4(0x00, 0x00, 0x00, 0x00));

		if(win){
			lines.draw_text("The House was Destroyed by the big snowBall!",
			glm::vec3(-aspect + 8.0f * H, -1.0 + 8.0f * H, 0.0),
			glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
			glm::u8vec4(0x00, 0x00, 0x00, 0x00));
		}else if(currentCoinEaten<10){
			lines.draw_text("You need to grow bigger to destroy the house",
			glm::vec3(-aspect + 8.0f * H, -1.0 + 8.0f * H, 0.0),
			glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
			glm::u8vec4(0x00, 0x00, 0x00, 0x00));
		}
		GL_ERRORS();
	}
}

glm::vec3 PlayMode::get_leg_tip_position() {
	//the vertex position here was read from the model in blender:
	return lower_leg->make_local_to_world() * glm::vec4(-1.26137f, -11.861f, 0.0f, 1.0f);
}
