#include <node_api.h>
#include <stdio.h>
#include <string>
#include <deque>

#include "al/al_glm.h"
#include "al/al_math.h"

// TODO: does al/al_field3d.h also work?
#include "an_field3d.h"
#include "al/al_isosurface.h"

const glm::vec3 WORLD_DIM = glm::vec3(6, 3, 6);
const glm::vec3 WORLD_CENTER = WORLD_DIM * 0.5f;
const int NUM_PARTICLES = 20000;
const int NUM_GHOSTPOINTS = 320000;
const int NUM_SNAKES = 7;
const int SNAKE_MAX_SEGMENTS = 17;
const int NUM_SNAKE_SEGMENTS = 136;
const int NUM_BEETLES = 2048;

// TODO: rethink how this works!!
const glm::vec3 VOXEL_DIM = glm::vec3(48, 24, 48);
const glm::vec3 WORLD_TO_VOXEL = VOXEL_DIM/WORLD_DIM;
const glm::vec3 VOXEL_TO_WORLD = WORLD_DIM/VOXEL_DIM;
const int NUM_VOXELS = 48 * 24 * 48;
const uint32_t INVALID_VOXEL_HASH = 0xffffffff;

const float SPACE_ADJUST = WORLD_DIM.y / 32.;

struct Particle {
	glm::vec4 pos;
	//glm::vec4 color;
	float energy, age, agelimit, dead;
	glm::vec3 dpos;
	uint32_t cloud;
	struct Particle * next;
};

struct ParticleInstanceData {
	glm::vec3 a_location; float unused;
	glm::vec4 a_color;
};

struct SnakeSegment {
	glm::quat a_orientation;
	glm::vec4 a_color;
	glm::vec3 a_location; float a_phase;
	glm::vec3 a_scale; float unused;
};

struct Snake {
	glm::vec3 wtwist; float length;
	float pincr, energy;
	
	int32_t segments[SNAKE_MAX_SEGMENTS];
	int32_t victim;
	int32_t nearest_beetle;

	int32_t id;
};

struct BeetleInstanceData {
	glm::vec4 vInstanceOrientation;
	glm::vec4 vInstanceColor;
	glm::vec3 vInstancePosition;
	float vInstanceAge;
	glm::vec3 vInstanceScale;
	float pad;
};

struct Beetle {
	
	glm::quat orientation;
	glm::vec4 color;
	glm::vec3 pos;	float pospad;
	glm::vec3 scale; float scalepad;
	
	glm::vec3 vel; float velpad;
	glm::vec3 angvel; float flowsensor;
	
	//float noise, snoise;
	float accelerometer;
	float energy, wellbeing, wellbeingprev;
	float age;
	
	float acceleration, azimuth, elevation, bank;
	//float memory[16];
	
	int32_t id, at, alive, recycle;
	
	Particle * nearest_particle;
	
	Beetle * next; // for voxels
	
	// audio grain properties:
	float period;
	
	glm::vec4 grain_mix;
	int32_t grain_start, grain_remain, grain_active;
	float grain_modulate;
	
	float grain_oinc, grain_ophase, grain_einc, grain_ephase;
	
	float grain_filter, grain_smoothed;

};

struct Voxel {
	Particle * particles;
	Beetle * beetles;
};

struct Shared {
	// rendering data:
	SnakeSegment snakeSegments[NUM_SNAKE_SEGMENTS];
	BeetleInstanceData beetleInstances[NUM_BEETLES];
	ParticleInstanceData particleInstances[NUM_PARTICLES];

	al::Isosurface::VertexData isovertices[NUM_VOXELS * 5];
	uint32_t isoindices[NUM_VOXELS * 15];

	uint32_t live_beetles = 0;
	uint32_t ghostcount = 0;
	uint32_t isovcount = 0;
	uint32_t isoicount = 0;
	uint32_t updating = 1;

	// simulation data:
	Snake snakes[NUM_SNAKES];
	Beetle beetles[NUM_BEETLES];
	Particle particles[NUM_PARTICLES];
	Voxel voxels[NUM_VOXELS];
	glm::vec4 ghostpoints[NUM_GHOSTPOINTS];

	al::Isosurface isosurface;
	Fluid3D<float> fluid;
	Field3D<float> landscape;

	float now = 0;
	float dayphase = 0;
	float daylight = 0;
	float density_isolevel, density_ideal, density_diffuse, density_decay;

	// parameters:
	float particle_noise = 0.005;
	float particle_move_scale = 1.;
	float particle_agelimit = 10.;

	glm::vec3 snake_levity = glm::vec3(0, 0.01, 0);
	float snake_decay = 0.0002;
	float snake_hungry = 0.25;
	float snake_fluid_suck = SPACE_ADJUST * 4.;

	float beetle_decay = 0.92;
	float beetle_life_threshold = 0.01;
	float beetle_friction = 0.85;
	float beetle_friction_turn = 0.5;
	float beetle_push = 0.03;
	float beetle_reproduction_threshold = 1.5;
	float beetle_max_acceleration = 0.3;
	float beetle_max_turn = 4;
	float beetle_size = SPACE_ADJUST * 0.15;
	float beetle_speed = 0.6;
	
	// audio:
	float beetle_dur = 0.2; //0.1
	float beetle_frequency = 3000;
	float beetle_rpts = 3; // 3
	float beetle_filter = 0.4; //0.4
	float beetle_base_period = 1.;
	float beetle_modulation = 0.5f;
	float samplerate = 44100;
	size_t blocksize = 256;

	std::deque<int32_t> beetle_pool;

	void reset();

	void update_daynight(double dt);
	void update_snakes(double dt);
	void update_beetles(double dt);
	void beetle_birth(Beetle& self);
	void update_particles(double dt);

	void move_beetles(double dt);
	void move_particles(double dt);

	inline void beetle_pool_push(Beetle& b) {
		//printf("push beetle %d %d\n", b, app->beetle_pool.size());
		beetle_pool.push_back(b.id);
		b.recycle = 1;
		b.alive = 0;
	}
	
	void beetle_pool_clear() {
		while (!beetle_pool.empty()) beetle_pool_pop();
	}
	
	inline int32_t beetle_pool_pop() {
		int b = -1;
		if (!beetle_pool.empty()) {
			b = beetle_pool.front();
			beetle_pool.pop_front();
			beetles[b].recycle = 0;
		}
		return b;
	}

	inline uint32_t voxel_hash(glm::vec3 pos) { 
		return voxel_hash(pos.x, pos.y, pos.z); 
	}
	
	inline uint32_t voxel_hash(float px, float py, float pz) {
		static const int32_t DIMX = VOXEL_DIM.x;
		static const int32_t DIMY = VOXEL_DIM.y;
		static const int32_t DIMZ = VOXEL_DIM.z;
		
		int32_t x = px * WORLD_TO_VOXEL.x, 
				y = py * WORLD_TO_VOXEL.y, 
				z = pz * WORLD_TO_VOXEL.z;
		if (x < 0 || x >= DIMX || y < 0 || y >= DIMY || z < 0 || z >= DIMZ) {
			return INVALID_VOXEL_HASH;
		}
		return x + DIMX*(y + (DIMY*z));
	}
	
	inline void voxel_push(Voxel& v, Particle& p) {
		p.next = v.particles;
		v.particles = &p;
	}
	
	inline void voxel_push(Voxel& v, Beetle& p) {
		p.next = v.beetles;
		v.beetles = &p;
	}
	
	inline Particle * voxel_pop_particle(Voxel& v) {
		Particle * p = v.particles; // the top particle
		if (p) {
			v.particles = p->next;
			p->next = 0;
		}
		return p;
	}
	
	inline Beetle * voxel_pop_beetle(Voxel& v) {
		Beetle * p = v.beetles; // the top particle
		if (p) {
			v.beetles = p->next;
			p->next = 0;
		}
		return p;
	}
	
	inline void clear_voxels() {
		memset(voxels, 0, sizeof(voxels));
	}

	glm::vec3 fluid_velocity(const glm::vec3& pos) {
		glm::vec3 flow;
		fluid.velocities.front().read_interp<float>(pos.x, pos.y, pos.z, &flow.x);
		return flow;
	}
};

void Shared::reset() {

	int dimx = VOXEL_DIM.x;
	int dimy = VOXEL_DIM.y;
	int dimz = VOXEL_DIM.z;
	int dim3 = dimx * dimy * dimz;
	isosurface.vertices().resize(5 * dim3);
    isosurface.indices().resize(3 * isosurface.vertices().size());
	//printf("iso verts inds %d %d\n", isosurface.vertices().size(), isosurface.indices().size());
	// generate some demo data
	std::vector<float> volData;
	volData.resize(dim3);
	float rate = M_PI * 0.1;
	for(int z=0; z<dimz; ++z){ 
		double zz = z * rate;
		for(int y=0; y<dimy; ++y){ 
			double yy = y * rate;
			for(int x=0; x<dimx; ++x){ 
				double xx = x * rate;                 
				volData[((z*dimy + y)*dimx) + x] = cos(xx) + cos(yy) + cos(zz);
			}
		}
	}
	isosurface.level(0.);
	isosurface.generate(&volData[0], 
		dimx, dimy, dimz, 
		//1., 1., 1. // what tod_zkm had
		VOXEL_TO_WORLD.x, VOXEL_TO_WORLD.y, VOXEL_TO_WORLD.z
	);
	double volLengthX, volLengthY, volLengthZ;
	isosurface.volumeLengths(volLengthX, volLengthY, volLengthZ);

	isovcount = isosurface.vertices().size() * 6;
	isoicount = isosurface.indices().size();

	printf("iso vertices %d indices %d dims %dx%dx%d, %fx%fx%f first %f\n", 
	 	isovcount, isoicount,
		isosurface.fieldDim(0), isosurface.fieldDim(1), isosurface.fieldDim(2), 
	 	volLengthX, volLengthY, volLengthZ,
	 	isosurface.vertices().elems()->position.x);

	// TODO: can we avoid this copy?
	memcpy(isovertices, &isosurface.vertices().elems()->position.x, sizeof(float) * isovcount);
	memcpy(isoindices, &isosurface.indices()[0], sizeof(uint32_t) * isoicount);

	// init particles:
	for (int i=0; i<NUM_PARTICLES; i++) {
		Particle& o = particles[i];
		
		o.pos = glm::vec4(glm::linearRand(glm::vec3(0.f), WORLD_DIM), 1.);
		//o.color = glm::vec4(glm::ballRand(1.), 1.);
		o.age = rnd::uni() * particle_agelimit;
		o.dead = 0.;
		o.energy = rnd::uni();
		o.dpos = glm::vec3(0);
		o.agelimit = 0.01 + rnd::uni() * rnd::uni() * particle_agelimit * 0.25;
		o.next = 0;
	}

	// init beetles:
	for (int i=0; i<NUM_BEETLES; i++) {
		Beetle& o = beetles[i];
		o.orientation = glm::angleAxis(rnd::uni() * M_PI, glm::sphericalRand(1.));
		o.color = glm::vec4(0.2, 0.7, 0.8, 1);
		o.pos = glm::linearRand(glm::vec3(0.f), WORLD_CENTER);
		o.scale = SPACE_ADJUST * glm::vec3(0.3);
		o.vel = glm::vec3(0);
		o.angvel = glm::vec3(0);
		o.accelerometer = 0;
		o.energy = rnd::uni();
		o.wellbeing = 0;
		o.wellbeingprev = 0;
		o.age = rnd::uni();
		o.acceleration = 0;
		o.azimuth = 0;
		o.elevation = 0;
		o.bank = 0;
		
		o.id = i;
		o.at = rnd::uni() * 44100;
		o.alive = false;
		o.recycle = true;
		o.nearest_particle = NULL;
		o.next = NULL;
		
		o.period = beetle_base_period * 0.1*(rnd::uni()*9. + 1.);
		o.grain_mix = glm::vec4(0.1);
		o.grain_start = 0;
		o.grain_remain = 0;
		o.grain_active = 0;
		o.grain_filter = 0.5;
		o.grain_smoothed = 0.;
		o.grain_oinc = 1;
		o.grain_einc = 1;
		o.grain_ophase = 0;
		o.grain_ephase = 0;
		
		beetle_pool_push(o);
	}
			
	// init snakes	
	int k=0;
	for (int i=0; i<NUM_SNAKES; i++) {
		Snake& self = snakes[i];
		float factor = 0.3;
		self.wtwist = glm::vec3(rnd::bi() * factor, 0, rnd::bi() * factor);
		self.length = 1.;
		self.pincr = 0.01 * (1+rnd::uni());
		self.energy = rnd::uni();
		self.id = i;
		self.victim = -1;
		self.nearest_beetle = -1;

		SnakeSegment * prev = 0;
		for (int j=0; j<SNAKE_MAX_SEGMENTS; j++, k++) {
			self.segments[j] = k;
			SnakeSegment& s = snakeSegments[k];			
			if (j==0) {
				// head:
				s.a_orientation = glm::angleAxis(rnd::uni() * M_PI, glm::sphericalRand(1.));
				s.a_color = glm::vec4(1., 0.504, 0.648, 1);
				s.a_location = glm::linearRand(glm::vec3(0.f), WORLD_DIM);
				s.a_phase = M_PI * rnd::uni();
				s.a_scale = glm::vec3(0.4, 0.4, 0.1);
			} else {
				// body part
				float tailness = j/(float)SNAKE_MAX_SEGMENTS;
				s.a_orientation = prev->a_orientation;
				s.a_color = prev->a_color;
				s.a_location = prev->a_location + quat_uz(prev->a_orientation) * prev->a_scale.z;
				s.a_phase = prev->a_phase + 0.1;
				s.a_scale.x = glm::mix(0.4, 0.2, pow(1.f-tailness,3.f));
				s.a_scale.y = s.a_scale.x;
				s.a_scale.z = glm::mix(0.1, 0.25, tailness);
			}
			prev = &s;
		}
	}
}

void Shared::update_daynight(double dt) {
	
	dayphase = fmod(now / 300., 1.);	// 5 minute cycle
	//phase = fmod(now / 30., 1.);	// 5 minute cycle
	
	double phaserad = M_PI * 2.0 * dayphase;
	
	// daylight is 60% of the time
	daylight = sin(phaserad)+0.25;
	daylight *= (daylight > 0.) ? 0.8 : 1.25;
	
	// modify isosurf decay at dusk/twilight:
	double t1 = sin(phaserad-0.1);
	
	//double t1 = sin(phaserad+0.3);
	//app.gui.density_decay(min(0.994, 4*t1*t1));
	
	if (daylight > 0) {
		//printf("day\n");
		density_isolevel += 0.01 * (0.8 - density_isolevel);
		density_ideal += 0.01 * (0.04 - density_ideal);
		density_diffuse += 0.01 * (0.02 - density_diffuse);
		density_decay = glm::min(0.997, 0.7 + 2*t1*t1);
	} else {
		//printf("night\n");
		density_isolevel += 0.01 * (0.25 - density_isolevel);
		density_ideal += 0.01 * (0.4 - density_ideal);
		density_diffuse += 0.01 * (0.05 - density_diffuse);
		density_decay = glm::min(0.995, 0.7 + 2*t1*t1);
	}
}

void Shared::update_snakes(double dt) {
	for (int i=0; i<NUM_SNAKES; i++) {
		Snake& self = snakes[i];
		SnakeSegment * head = snakeSegments + self.segments[0];
		
		float speed = SPACE_ADJUST * .05 * (0.85 - sin((head->a_phase) * M_PI * 2.));
		
		//speed *= 4.;

		// nearer to 1 mkes the snake have greater tendency to straighten its tail
		// nearer to 0 makes the snake be more TRON-like
		float torque_tension = 0.15;
		
		// compute distance (squared) from center line
		float distz = fabs(head->a_location.z - WORLD_CENTER.z) * 0.5;
		float disty = head->a_location.y;
		float dist2 = distz*distz + disty*disty;
		bool wandering = dist2 < 1000;
		//printf("dist2 %f\n", dist2);
		
		glm::vec3 uf1 = quat_uf(head->a_orientation);

		glm::quat rot;
		if (wandering) {
			self.wtwist = glm::mix(self.wtwist, glm::vec3(rnd::bi()*M_PI, 0, rnd::bi()*M_PI), 0.01);
			rot = quat_fromEulerXYZ(self.wtwist);
			
		} else {
			
			//rot = head.orientation:uf():getRotationTo((center - head.pos):normalize());
			rot = glm::normalize(glm::rotation(uf1,
								 glm::normalize(WORLD_CENTER - head->a_location)));
		}

		// choose a beetle to eat:
		if (self.nearest_beetle >= 0) {
			// just eat this one.
			self.victim = self.nearest_beetle;
			torque_tension = torque_tension * 2;
			
		} else if (self.victim < 0) {
			// find a beetle:
			if (self.energy < snake_hungry) {
				//if random(200) == 1 then
				Beetle * v = beetles + (rand() % NUM_BEETLES);
				//print("checking out", victim)
				if (v->age > 0.1 && v->alive) {
					glm::vec3 rel = head->a_location - v->pos;
					// is it near?
					if (glm::dot(rel, rel) < 64) {
						self.victim = v->id;
						//printf("chasing %p\n", v);
						torque_tension = torque_tension * 2;
					}
				}
			}
		} else {
			Beetle * v = beetles + self.victim;
			if (v && v->alive) {
				glm::vec3 vpos = v->pos;

				// move toward victim:
				rot = glm::slerp(rot, glm::normalize(glm::rotation(uf1, glm::normalize(vpos - head->a_location))), 0.5f);

				// arrived?
				glm::vec3 diff = head->a_location - vpos;
				float d = glm::dot(diff, diff);	// distance squared
				if (d < 3) {
					//print("EAAAAAAT")
					self.energy = v->energy;
					v->energy = 0;
					// stop chasing:
					self.victim = -1;
				} else {
					speed = speed * 5;
				}
			} else {
				// no point chasing:
				self.victim = -1;
			}
		}

		float alpha = 0.f;
		
		head->a_phase = head->a_phase + self.pincr;
		head->a_color.r = glm::min(1.f, self.energy);
		head->a_orientation = glm::normalize( glm::slerp(head->a_orientation, rot * head->a_orientation, 0.1f) );
		
		uf1 = quat_uf(head->a_orientation);
		glm::vec3 target = head->a_location + (uf1*speed);
		
		// keep it above ground:
		if (target.y < 0) target.y = 0;
		if (head->a_location.y < WORLD_CENTER.y) target = target + snake_levity;
		// keep away from clipping planes:
		if (target.z < 1) target.z = 1;
		if (target.z > WORLD_DIM.z-1) target.z = WORLD_DIM.z-1;
		
		// limit to world:
		//target = glm::clamp(target, vec3(0), vec3(DIMWRAP));
		head->a_location = target;
		
		// now process tails:
		glm::quat qtarget = head->a_orientation;
		SnakeSegment * prev = head;
		for (int s = 1; s < SNAKE_MAX_SEGMENTS; s++) {
			SnakeSegment * segment = snakeSegments + self.segments[s];
			float phase = s / float(SNAKE_MAX_SEGMENTS+1);
			alpha = sin(M_PI * (1.-phase)*(1.-phase));
 
			// tails:
			segment->a_phase = prev->a_phase - 0.05; // + 0.1
			segment->a_color.r = segment->a_color.r + 0.5*(prev->a_color.r - segment->a_color.r);
			
			uf1 = quat_uf(segment->a_orientation);
			glm::quat rot = glm::normalize(glm::rotation(uf1, glm::normalize(target - segment->a_location)));
			//local rot = segment.orientation:uf():getRotationTo((target - segment.pos):normalize())
			segment->a_orientation = glm::normalize(rot * segment->a_orientation);

			// straighten out:
			segment->a_orientation = glm::slerp(segment->a_orientation, qtarget, torque_tension * alpha); //torque_tension)
			
			// derive position
			uf1 = quat_uf(segment->a_orientation);
			target = target - (uf1 * segment->a_scale.z);
			
			// clamp it:
			if (target.y < 0.) target.y = 0.;
			if (target.z < -WORLD_DIM.z) target.z = -WORLD_DIM.z;
			else if (target.z > WORLD_DIM.z*2) target.z = WORLD_DIM.z*2;
			if (target.x < -WORLD_DIM.x) target.x = -WORLD_DIM.x;
			else if (target.x > WORLD_DIM.x*2) target.x = WORLD_DIM.x*2;
			
			//target = glm::clamp(target, vec3(0), vec3(DIMWRAP));
			bool inworld = target.x >= 0.f && target.x < WORLD_DIM.x
						&& target.y >= 0.f && target.y < WORLD_DIM.y
						&& target.z >= 0.f && target.z < WORLD_DIM.z;
			if (inworld) {
				float goo = 0.05f*s;
				landscape.front().add(segment->a_location - uf1, &goo);				
				if (segment->a_location.y < WORLD_CENTER.y) target = target + snake_levity; // do this for all segments?

				// fluid & goo:
				//vec3 suck = uf1 * (-alpha * 0.5f);
				// C.an_fluid_add_velocity(pos, suck)
				glm::vec3 suck = uf1 * (-speed * alpha * snake_fluid_suck);
				fluid.velocities.front().add(segment->a_location, &suck.x);
			}
			
			// actually move it:
			segment->a_location = target;
			
			qtarget = segment->a_orientation;
			prev = segment;
		}
		self.energy = glm::max(0.f, self.energy - snake_decay);
	}
}

void Shared::update_beetles(double dt) {
	const glm::vec3 posmin(.1f);
	const glm::vec3 posmax = WORLD_DIM - (posmin * 2.f);
	bool shown = false;
	for (int i=0; i<NUM_BEETLES; i++) {
		Beetle& self = beetles[i];
		// skip beetles who are part of this world:
		if (!self.recycle) {
			float energy0 = self.energy;
			
			glm::vec3 flow = fluid_velocity(self.pos);
			glm::vec3 force = flow * beetle_push;

			// do this whether dead or alive:
			self.angvel *= beetle_friction_turn;
			self.vel *= beetle_friction;
			
			if (self.alive) {
				// sensing:
				glm::vec3 uf = quat_uf(self.orientation);
				self.flowsensor = -glm::dot(uf, flow);
				self.accelerometer = quat_uf(self.orientation).y; // tells us which way is up
				
				// update color:
				self.color.g = 0.4 + 0.1*self.energy;
				self.color.r = 0.5/(self.age+0.5);
				self.color.b = 0.5*(1 - self.color.r);
				
				// fade in:
				float s = beetle_size * glm::min(1.f, self.age);
				float t = beetle_size * glm::min(1.f, self.age*0.5f);
				self.scale = glm::vec3(s, s, t);
				
				// eating:
				Particle * p = self.nearest_particle;
				if (p) {	// check if it is dead?
					self.energy += (p->energy-0.1);
					p->dead = 1.;
					self.nearest_particle = 0;
				}
				
				// changing speed:
				float speed = (s + beetle_size) * beetle_speed;
				if (self.accelerometer > 0 && self.pos.y < (rnd::uni()*8.)) {
					self.acceleration = speed * 0.1 * rnd::uni();
					//} else if (self.pos.x < (dimf * rnd::uni())) {
					//self.acceleration = speed * 0.2 * (minimum(self.flowsensor, 0.2)); //0.1;
				} else {
					self.acceleration = speed * rnd::uni()*0.2;
				}
				
				// change direction:
				float turn = beetle_max_turn * beetle_size/(beetle_size+s*s);
				self.azimuth = rnd::bi() * 0.1 * turn;
				self.elevation = rnd::uni() * 0.02 * turn;
				self.bank = self.azimuth * 0.1 * turn;
				self.angvel += glm::vec3(self.elevation, self.azimuth, self.bank);
				
				// add motile forces:
				force += uf * (self.acceleration * beetle_max_acceleration);
				self.acceleration = 0;
				
				// reproducing:
				if (self.energy > beetle_reproduction_threshold) {
					int32_t child_id = beetle_pool_pop();
					if (child_id >= 0) {
						// spawn a child
						Beetle& child = beetles[child_id];
						beetle_birth(child);
						self.energy *= 0.66;
						child.energy = self.energy * 0.5;
						child.pos = self.pos;
						
						double mutability = 0.1;
						child.period = self.period + mutability * (child.period - self.period);
					}
				}
				
				// dying:
				if (self.energy < beetle_life_threshold) {
					self.alive = 0;
				} else {
					// smoothed differential:
					self.wellbeingprev = self.energy - energy0;
					self.wellbeing += 0.2 * (self.wellbeingprev - self.wellbeing);
					// aging
					self.age = self.age + dt;
					self.energy *= beetle_decay;
				}
				
			} else {
				// dead, but still visible:
				static const float grey = 0.4;
				self.color.r += 0.2*(self.color.g - self.color.r);
				self.color.g += 0.2*(grey - self.color.g);
				self.color.b += 0.2*(self.color.g - self.color.b);
				
				// fade out:
				self.scale *= 0.96;
				if (self.scale.x < 0.02) {
					beetle_pool_push(self);	// and recycle
											//printf("recyle of %d\n", self.id);
				}
			}
			
			// integrate forces:
			self.vel += force;
		}
	}
	
	// randomly re-populate:
	if (rnd::uni() < 0.1) {
		int32_t child_id = beetle_pool_pop();
		if (child_id >= 0) {
			Beetle& self = beetles[child_id];
			beetle_birth(self);
			self.energy = rnd::uni();
			self.pos = glm::linearRand(posmin, posmax);
		}
	}
}

void Shared::beetle_birth(Beetle& self) {
	//printf("birth of %d\n", self.id);
	self.age = 0;
	self.at = rnd::uni() * -blocksize;
	self.alive = 1;
	self.recycle = 0;
	self.wellbeing = self.wellbeingprev = 0;
	self.scale = glm::vec3(0.);
	self.vel = glm::vec3(0);
	self.angvel = glm::vec3(0);
	self.period = beetle_base_period * 0.1*(rnd::uni()*9. + 1.);
}

void Shared::move_beetles(double dt) {
	//const float dimf = float(DIM);
	//const float rdimf = 1.f/dimf;
	const glm::vec3 posmin(.1f);
	const glm::vec3 posmax = WORLD_DIM - (posmin * 2.f);
	live_beetles = 0;
	for (int i=0; i<NUM_BEETLES; i++) {
		Beetle& self = beetles[i];
		if (!self.recycle) {
			glm::quat angular = quat_fromEulerXYZ(self.angvel);	// ambiguous order of operation...
			self.orientation = glm::normalize(angular * self.orientation);
			self.pos = glm::clamp(self.pos + (self.vel * SPACE_ADJUST), posmin, posmax);
			uint32_t hash = voxel_hash(self.pos);
			if (hash != INVALID_VOXEL_HASH) {
				self.nearest_particle = voxel_pop_particle(voxels[hash]);
				if (self.alive) {
					voxel_push(voxels[hash], self);
				}
			} else {
				self.nearest_particle = NULL;
			}
			
			// copy instance data here:
			BeetleInstanceData& instance = beetleInstances[live_beetles];
			instance.vInstancePosition = self.pos;
			instance.vInstanceOrientation.x = self.orientation.x;
			instance.vInstanceOrientation.y = self.orientation.y;
			instance.vInstanceOrientation.z = self.orientation.z;
			instance.vInstanceOrientation.w = self.orientation.w;
			instance.vInstanceScale = self.scale;
			instance.vInstanceAge = self.age;
			instance.vInstanceColor = self.color;
			
			live_beetles++;
		}
	}
	//printf("live beetles %d\n", live_beetles);
}

void Shared::update_particles(double dt) {
	//const float dimf = float(DIM);
	//const float rdimf = 1.f/dimf;
	const float local_particle_move_scale = particle_move_scale; // *dt
	const float local_particle_noise = particle_noise;
	
	const glm::vec3 posmin(.1f);
	const glm::vec3 posmax = WORLD_DIM - (posmin * 2.f);
	for (int i=0; i<NUM_PARTICLES; i++) {
		Particle& o = particles[i];
		glm::vec4& pos = o.pos;
		// TODO: if owner, follow owner position?
		// TODO: recycle particle if it is inside the isosurface? (age = agelimit)
		
		bool recycle = o.age > o.agelimit
//					|| self.pos.y > maxheight
//					|| self.pos.z > maxheight
//					|| self.pos.z < (maxheight - world_dim.z)
//					|| self.pos.x < 0
		;
		
		if (recycle) {
			// TODO: pick a random ghostpoint to clone
			// chance should depend on how many ghostpoints we have
			// but there should always be a chance of random location
			
			// alternatively:
			uint32_t local_ghostcount = ghostcount;
			bool useghost = local_ghostcount > 100 && rnd::uni() < 0.8;
			//bool useghost = rand() % 40000 < ghostcount;
			if (useghost) {
				//
				int idx = rand() % local_ghostcount;
				o.pos = ghostpoints[idx] + glm::vec4(glm::ballRand(0.25f), 0.);
				// only these ones bring them back to life?
				o.dead = 0.;
				o.cloud = true;
				o.energy = 1;
				o.agelimit = 0.01 + rnd::uni() * rnd::uni() * particle_agelimit;
			} else {
				// else randomize:
				o.pos = glm::vec4(glm::linearRand(posmin, posmax), 1.f);
				if (rnd::uni() < 0.2) o.dead = 0.;
				o.cloud = false;
				o.agelimit = 0.01 + rnd::uni() * rnd::uni() * (particle_agelimit * 0.25);
			}
			// reset:
			o.age = 0;
		}
		
		const float local_div_agelimit = 1.f/float(o.agelimit);
		// energy level increases with age ... this means older particles move faster
		float energy = o.age * local_div_agelimit;
		o.energy += 0.1*(energy * energy - o.energy);
		
		// advect by fluid:
		glm::vec3 flow;
		fluid.velocities.front().read_interp<float>(pos.x, pos.y, pos.z, &flow.x);
		
		// add some deviation (brownian)
		glm::vec3 disturb = glm::ballRand(local_particle_noise);
		float fadein = glm::clamp(o.age * (o.cloud ? 4.f : 0.5f), 0.f, 1.f);

		// note that movement depends also on energy level:
		//o.dpos = (local_particle_move_scale * (1.5f - energy)) * (disturb + flow);
		//o.dpos = (local_particle_move_scale) * (disturb + flow);
		o.dpos = SPACE_ADJUST * (flow * fadein + disturb);
		
		// update color:/
		//float fadeout = glm::clamp(float(o.agelimit - o.age), 0.f, 1.f);
		//float fade = fadein * fadeout;
		//if (o.dead) {
		//	const float grey = 0.2f;
			//o.color = glm::vec4(grey, grey, grey, fade);
		//} else {
			//float fade = glm::clamp(32.f*fabsf(0.5f-fmodf(o.energy+0.5f, 1.f)), 0.f, 1.f);
			//o.color = glm::vec4(1, .1+.5*(1.-o.energy), 0.1, fade);
		//}
		
		// TODO: set particle voxels here, or in move?
		//if (!self.dead) particleVoxels.set(self.pos, &self);
		// also set particle density here:
		//double value = 1.;
		//particleDensity.input().write_interp(&value, p1);
		
		// update age:
		o.age += dt;
	}
}

void Shared::move_particles(double dt) {
	// move particles:
	const float local_particle_move_scale = particle_move_scale; // *dt
	const float local_particle_noise = particle_noise;
	for (int i=0; i<NUM_PARTICLES; i++) {
		Particle& o = particles[i];
		ParticleInstanceData& instance = particleInstances[i];
		glm::vec4& pos = o.pos;
		
		glm::vec3 newpos = glm::vec3(pos) + o.dpos;
		
		// wrap in world:
//		pos.x = euc_fmodf1(pos.x + dpos.x, dimf, rdimf);
//		pos.y = euc_fmodf1(pos.y + dpos.y, dimf, rdimf);
//		pos.z = euc_fmodf1(pos.z + dpos.z, dimf, rdimf);
		
		// clamp in world:
		//newpos = glm::clamp(newpos, posmin, posmax);
		
		// alternatively, let them wander outside the world -- they'll die anyway
		// except clamp at ground:
		if (newpos.y < 0.f) newpos.y = 0.f;
		
		uint32_t hash = voxel_hash(glm::vec3(o.pos));
		if (!o.dead && hash != INVALID_VOXEL_HASH) {
			voxel_push(voxels[hash], o);
		}
		
		pos.x = newpos.x;
		pos.y = newpos.y;
		pos.z = newpos.z;

		instance.a_location = glm::vec3(o.pos);
		instance.a_color = glm::vec4(o.energy, o.age, o.agelimit, o.dead);
		
		//if (i==0) printf("particle %f %f %f\n", pos.x, pos.y, pos.z);
	}
}

Shared shared;

///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////

napi_value setup(napi_env env, napi_callback_info info) {
	napi_status status = napi_ok;

	shared.reset();
	
	napi_value shared_value;
	//status = napi_create_arraybuffer(env, sizeof(Shared), (void**)&shared, &shared_value);
	status = napi_create_external_arraybuffer(env, &shared, sizeof(Shared), nullptr, nullptr, &shared_value);
	
	return shared_value;
}

napi_value update(napi_env env, napi_callback_info info) {
	napi_status status = napi_ok;

	napi_value args[2];
	size_t argc = 2;
	status = napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
	if(status != napi_ok) {
		napi_throw_type_error(env, nullptr, "Missing arguments");
	}

	double now = 0.;
	status = napi_get_value_double(env, args[0], &now);
	if (status != napi_ok) napi_throw_type_error(env, nullptr, "Expected number");
	double dt = 1/60.;
	status = napi_get_value_double(env, args[1], &dt);
	if (status != napi_ok) napi_throw_type_error(env, nullptr, "Expected number");

	shared.now = now;

#ifdef AN_USE_KINECT
	// would like to move this to another thread
	// but would need to doublebuffer ghostpoints for that to work
	// (or ringbuffer it)
	shared.update_cloud();
#endif

	shared.update_daynight(dt);
	shared.update_snakes(dt);
	shared.update_beetles(dt);
	shared.update_particles(dt);

	if (shared.updating) {
		shared.move_beetles(dt);
		shared.move_particles(dt);
	}
	// napi_typedarray_type type;
	// size_t length, byte_offset;
	// Shared * data;
	// status = napi_get_typedarray_info(env, args[0], &type, &length, (void **)&data, nullptr, &byte_offset);

	// printf("got array of %d at %d, value %f\n", length, byte_offset, data->snakeSegments[0].a_location.x);

	return nullptr;
}

napi_value test(napi_env env, napi_callback_info info) {
	napi_status status = napi_ok;

	// napi_value args[1];
	// size_t argc = 1;
	// status = napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
	// if(status != napi_ok) {
	// 	napi_throw_type_error(env, nullptr, "Missing arguments");
	// }

	// napi_typedarray_type type;
	// size_t length, byte_offset;
	// Shared * data;
	// status = napi_get_typedarray_info(env, args[0], &type, &length, (void **)&data, nullptr, &byte_offset);

	// printf("got array of %d at %d, value %f\n", length, byte_offset, data->snakeSegments[0].a_location.x);

	return nullptr;
}

napi_value init(napi_env env, napi_value exports) {
	napi_status status;
	napi_property_descriptor properties[] = {
		{ "setup", 0, setup, 0, 0, 0, napi_default, 0 },
		{ "test", 0, test, 0, 0, 0, napi_default, 0 },
		{ "update", 0, update, 0, 0, 0, napi_default, 0 },
	};
	status = napi_define_properties(env, exports, 3, properties);
	assert(status == napi_ok);
	return exports;
}

NAPI_MODULE(NODE_GYP_MODULE_NAME, init)