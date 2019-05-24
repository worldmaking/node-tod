#include <node_api.h>
#include <stdio.h>
#include <string>
#include <deque>
#include <iostream>       // std::cout
#include <thread>         // std::thread

#include "al/al_kinect2.h"
//#include "al/al_hmd.h"
#include "al/al_glm.h"
#include "al/al_math.h"
#include "al/al_field3d.h"
#include "al/al_isosurface.h"
#include "al/al_mmap.h"
#include "al/al_hashspace.h"

#ifdef AN_USE_AUDIO
#include "RtAudio.h"
#endif

//#define AN_USE_HASHSPACE 1

struct KinectData {
	CloudFrame cloudFrames[KINECT_FRAME_BUFFERS];
	int lastCloudFrame = 0;
	int deviceId = 0;

	// the most recently completed frame:
	const CloudFrame& cloudFrame() const {
		return cloudFrames[lastCloudFrame];
	}
};

Mmap<KinectData> kinectMap[2];
KinectData * kinectData[2];

static const glm::vec3 WORLD_DIM = glm::vec3(6, 3, 6);
static const glm::vec3 WORLD_CENTER = WORLD_DIM * 0.5f;
static const int NUM_PARTICLES = 20000;
static const int NUM_GHOSTPOINTS = 320000;
static const int NUM_SNAKES = 7;
static const int SNAKE_MAX_SEGMENTS = 17;
static const int NUM_SNAKE_SEGMENTS = 136;
static const int NUM_BEETLES = 2048;

// TODO: rethink how this works!!
static const int VOXEL_DIM_X = 32;
static const int VOXEL_DIM_Y = 16;
static const int VOXEL_DIM_Z = VOXEL_DIM_X;
static const int VOXEL_BITS_X = 5; //log(VOXEL_DIM_X) / log(2);
static const int VOXEL_BITS_Y = 4; //log(VOXEL_DIM_Y) / log(2);
static const int VOXEL_BITS_Z = 5; //log(VOXEL_DIM_Z) / log(2);
const glm::vec3 VOXEL_DIM = glm::vec3(VOXEL_DIM_X, VOXEL_DIM_Y, VOXEL_DIM_Z);
const glm::vec3 VOXEL_BITS = glm::vec3(VOXEL_BITS_X, VOXEL_BITS_Y, VOXEL_BITS_Z); 
const glm::vec3 WORLD_TO_VOXEL = VOXEL_DIM/WORLD_DIM;
const glm::vec3 VOXEL_TO_WORLD = WORLD_DIM/VOXEL_DIM;
const int NUM_VOXELS = VOXEL_DIM_X * VOXEL_DIM_Y * VOXEL_DIM_Z;
const uint32_t INVALID_VOXEL_HASH = 0xffffffff;

const float SPACE_ADJUST = 3. / 32.;


const int OSC_TABLE_DIM = 2048;
const int OSC_TABLE_WRAP = (OSC_TABLE_DIM-1);

bool threadsRunning = false;

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
	int32_t victim = INVALID_VOXEL_HASH;
	int32_t nearest_beetle = INVALID_VOXEL_HASH;

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

struct BeetleAudioData {
	float age; 		// == 0 for not alive, also maps to rpts?
	float energy;   // maps to filter

	float period; 	// genetic
	float freq;		// genetic

	float modulation; // from what? speed?
	float scale; 	// x-> amp

	glm::vec2 pos_xz; // for panning
};

struct AudioData {
	BeetleAudioData beetles[NUM_BEETLES];
};

Mmap<AudioData> audioData;


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

	// genetics:
	glm::vec3 genes;
	
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
	glm::vec4 ghostpoints[NUM_GHOSTPOINTS]; 

	al::Isosurface::VertexData isovertices[NUM_VOXELS * 5];
	uint32_t isoindices[NUM_VOXELS * 15];

	uint32_t live_beetles = 0;
	uint32_t ghostcount = 0;
	uint32_t isovcount = 0;
	uint32_t isoicount = 0;
	uint32_t updating = 0;


	// simulation data:
	Snake snakes[NUM_SNAKES];
	Beetle beetles[NUM_BEETLES];
	Particle particles[NUM_PARTICLES];
	Voxel voxels[NUM_VOXELS];
	// TODO are these in world or voxel space?
	float human_present[NUM_VOXELS];

	al::Isosurface isosurface;
	Fluid3D<float> fluid;
	Field3D<float> landscape;
	Array<float> noisefield;
	// for the ghost clouds, as voxel fields
	// (double-buffered)
	Field3D<float> density;
	Field3D<glm::vec3> density_gradient;
	Array<glm::vec3> density_change;

#ifdef AN_USE_HASHSPACE
	Hashspace3D<NUM_PARTICLES, VOXEL_BITS_X> hashspaceParticles;
	Hashspace3D<NUM_BEETLES, VOXEL_BITS_X> hashspaceBeetles;
#endif

	float now = 0;
	float dayphase = 0;
	float daylight = 0;
	float density_isolevel=0, density_ideal=0, density_diffuse=0, density_decay=0;

	float fluid_passes = 14;
	float fluid_viscocity = 0.0001;
	float fluid_advection = 0.;
	float fluid_decay = 0.99;
	float fluid_boundary_friction = 0.25;
	float human_flow = -125;//2.;
	float human_cv_flow = 0.5;
	float human_fluid_smoothing = 0;//0.01;
	float human_smoothing = 0.01;
	float goo_rate = 3;

	// parameters:
	float particle_noise = 0.004;
	float particle_move_scale = 1.;
	float particle_agelimit = 4.;
	float ghost_chance = 0.5;

	glm::vec3 snake_levity = glm::vec3(0, 0.01, 0);
	float snake_decay = 0.0002;
	float snake_hungry = 0.25;
	float snake_fluid_suck = 4;//10.; 
	float snake_goo_add = 0.05f;

	float beetle_decay = 0.89;
	float beetle_life_threshold = 0.01;
	float beetle_friction = 0.85;
	float beetle_friction_turn = 0.5;
	float beetle_push = 0.1;
	float beetle_fluid_suck = 1;
	float beetle_reproduction_threshold = 1.5;
	float beetle_max_acceleration = 0.3;
	float beetle_max_turn = 2;
	float beetle_size = 0.03;
	float beetle_speed = 2.;
	float beetle_dead_fade = 0.98; //0.96
	
	// audio:
	float beetle_dur = 0.2; //0.1
	float beetle_frequency = 3000;
	float beetle_rpts = 3; // 3
	float beetle_filter = 0.4; //0.4
	float beetle_base_period = 1.;
	float beetle_modulation = 0.5f;
	float samplerate = 44100;
	uint32_t blocksize = 128;
	uint32_t audio_channels = 4;
	float audio_gain = 0.008f;

	float mEnvTable[OSC_TABLE_DIM];
	float mOscTable[OSC_TABLE_DIM];

#ifdef AN_USE_AUDIO
	RtAudio audio;
	RtAudio::DeviceInfo info;
#endif
	// figure out build error for this or do a mmapfile thing?
	//CloudDeviceManager cloudDeviceManager;

	std::deque<int32_t> beetle_pool;

	std::thread mSimulationThread, mFluidThread, mHumanThread;
	float mSimulationSeconds=0, mFluidSeconds=0, mHumanSeconds=0;

	void reset();
	void exit();

	void audio_callback(float * out0, float * out1, float * out2, float * out3, long blocksize);
	void update_daynight(double dt);
	void update_isosurface(double dt);

	void update_cloud();

	void serviceFluid();
	void serviceSimulation();

	void update_fluid(double dt);
	void apply_fluid_boundary(glm::vec3 * velocities, const float * landscape, const size_t dim0, const size_t dim1, const size_t dim2);
	void update_density(double dt);
	void update_goo(double dt);

	void update_particles(double dt);
	void update_beetles(double dt);
	void beetle_birth(Beetle& self);
	void update_snakes(double dt);

	void move_particles(double dt);
	void move_beetles(double dt);
	void move_snakes(double dt);

	void dsp_initialize(double sr, long blocksize);
	void perform_audio(float * FL, float * FR, float * BL, float * BR, long frames);

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
		if (x < 0 || x >= DIMX || 
			y < 0 || y >= DIMY || 
			z < 0 || z >= DIMZ) {
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

	void fluid_velocity_add(const glm::vec3& pos, const glm::vec3& vel) {
		fluid.velocities.front().add(WORLD_TO_VOXEL * pos, &vel.x);
	}
	glm::vec3 fluid_velocity(const glm::vec3& pos) {
		glm::vec3 flow;
		fluid.velocities.front().read_interp<float>(WORLD_TO_VOXEL.x * pos.x, WORLD_TO_VOXEL.y * pos.y, WORLD_TO_VOXEL.z * pos.z, &flow.x);
		return flow;
	}
};

#ifdef AN_USE_AUDIO
static void rtErrorCallback(RtAudioError::Type type, const std::string &errorText)
{
	// This exampEle error handling function does exactly the same thing
	// as the embedded RtAudio::error() function.
	std::cout << "in errorCallback" << std::endl;
	if (type == RtAudioError::WARNING)
		std::cerr << '\n' << errorText << "\n\n";
	else if (type != RtAudioError::WARNING) {
		//throw(RtAudioError(errorText, type));
		printf("RTAudio error %s\n", errorText.data());
	}
}

static int rtAudioCallback(void *outputBuffer, void * inputBuffer, unsigned int nBufferFrames,
						   double /*streamTime*/, RtAudioStreamStatus status, void *data)
{
	if (status) {
		std::cout << "Stream underflow detected!" << std::endl;
	}

	//printf(".\n");

	Shared& shared = *(Shared *)data;
	
	shared.blocksize = nBufferFrames;
	shared.samplerate = 44100;
	
	float * out0 = (float *)outputBuffer;
	float * out1 = out0 + nBufferFrames;
	float * out2 = out0;
	float * out3 = out1;
	
	if (shared.audio_channels >= 4) {
		out2 = out1 + nBufferFrames;
		out3 = out2 + nBufferFrames;
	}
	
	for (int i = 0; i < nBufferFrames; i++) {
		out0[i] = out1[i] = out2[i] = out3[i] = 0.f;
	}

	shared.audio_callback(out0, out1, out2, out3, nBufferFrames);
	
	return 0;
}
#endif

void Shared::reset() {

	int dimx = VOXEL_DIM.x;
	int dimy = VOXEL_DIM.y;
	int dimz = VOXEL_DIM.z;
	int dim3 = dimx * dimy * dimz;

	fluid.initialize(dimx, dimy, dimz);
	density.initialize(dimx, dimy, dimz);
	noisefield.initialize(dimx, dimy, dimz, 4);
	landscape.initialize(dimx, dimy, dimz, 1);
	density_change.initialize(dimx, dimy, dimz);
	density_gradient.initialize(dimx, dimy, dimz);
	int i=0;
	for (unsigned z=0; z<dimx; z++) {
		for (unsigned y=0; y<dimy; y++) {
			for (unsigned x=0; x<dimz; x++) {
				density_gradient.front().ptr()[i] = glm::normalize(glm::vec3( -0.5+x/(float)dimx, -0.5+y/(float)dimy, -0.5+z/(float)dimz ));
				i++;
			}
		}
	}

	for (int i=0; i<NUM_VOXELS; i++) {
		voxels[i].particles = 0;
		voxels[i].beetles = 0;
		
		glm::vec4 * n = (glm::vec4 *)noisefield[i];
		*n = glm::linearRand(glm::vec4(0.), glm::vec4(1.));
		//boundary[i] = 1;
		
		float * l = (float *)landscape.front()[i];
		*l = glm::linearRand(0.f, 0.2f);
	}

#ifdef AN_USE_HASHSPACE
	// hashspace is cuboid; use X dim as the measure
	// (yes, this means half the hashspace is unused...)
	hashspaceParticles.reset(glm::vec3(0), glm::vec3(WORLD_DIM.x));
	hashspaceBeetles.reset(glm::vec3(0), glm::vec3(WORLD_DIM.x));
#endif

	isosurface.vertices().resize(5 * dim3);
    isosurface.indices().resize(3 * isosurface.vertices().size());
	//printf("iso verts inds %d %d\n", isosurface.vertices().size(), isosurface.indices().size());
	// generate some demo data
	std::vector<float> volData;
	volData.resize(dim3);
	float rate = M_PI * (0.1);
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
		self.length = 0.6;
		self.pincr = 0.01 * (1+rnd::uni());
		self.energy = rnd::uni();
		self.id = i;
		self.victim = INVALID_VOXEL_HASH;
		self.nearest_beetle = INVALID_VOXEL_HASH;

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
				s.a_scale = glm::vec3(0.22, 0.22, 0.06);
			} else {
				// body part
				float tailness = j/(float)SNAKE_MAX_SEGMENTS;
				s.a_orientation = prev->a_orientation;
				s.a_color = prev->a_color;
				s.a_location = prev->a_location + quat_uz(prev->a_orientation) * prev->a_scale.z;
				s.a_phase = prev->a_phase + 0.1;
				s.a_scale.x = glm::mix(0.22, 0.11, pow(1.f-tailness,3.f));
				s.a_scale.y = s.a_scale.x;
				s.a_scale.z = glm::mix(0.06, 0.2, tailness);
			}
			prev = &s;
		}
	}

	
	
	// setup audio:
	
	// initialize envelopes:
	for (int i=0; i<OSC_TABLE_DIM; i++) {
		float phase = i / (float)OSC_TABLE_DIM;
		mOscTable[i] = sin(M_PI * 2.f * phase);
		
		// skew the phase for the env:
		phase = sin(M_PI * 0.5 * phase);
		mEnvTable[i] = 0.5 - cos(M_PI * 2.f * phase) * 0.5;
		mEnvTable[i] = 0.42-0.5*(cos(2*M_PI*phase))+0.08*cos(4*M_PI*phase);
		
		// nutall
		mEnvTable[i] = 0.35875-0.48829*cos(2*M_PI*phase)+0.14128*cos(4*M_PI*phase)-0.01168*cos(6*M_PI*phase);
		//mEnvTable[i] = 0.22*(1. -1.93*cos(2*M_PI*phase)  +1.29*cos(4*M_PI*phase)  -0.388*cos(6*M_PI*phase)  +0.032*cos(8*M_PI*phase));
	}
	#ifdef AN_USE_AUDIO
	if (1) {
		unsigned int devices = audio.getDeviceCount();
		std::cout << "\nFound " << devices << " device(s) ...\n";
		
		/*
		for (unsigned int i = 0; i<devices; i++) {
			info = audio.getDeviceInfo(i);
			
			std::cout << "\nDevice Name = " << info.name << '\n';
			if (info.probed == false)
				std::cout << "Probe Status = UNsuccessful\n";
			else {
				std::cout << "Probe Status = Successful\n";
				std::cout << "Output Channels = " << info.outputChannels << '\n';
				std::cout << "Input Channels = " << info.inputChannels << '\n';
				std::cout << "Duplex Channels = " << info.duplexChannels << '\n';
				if (info.isDefaultOutput) std::cout << "This is the default output device.\n";
				else std::cout << "This is NOT the default output device.\n";
				if (info.isDefaultInput) std::cout << "This is the default input device.\n";
				else std::cout << "This is NOT the default input device.\n";
				if (info.nativeFormats == 0)
					std::cout << "No natively supported data formats(?)!";
				else {
					std::cout << "Natively supported data formats:\n";
					if (info.nativeFormats & RTAUDIO_SINT8)
						std::cout << "  8-bit int\n";
					if (info.nativeFormats & RTAUDIO_SINT16)
						std::cout << "  16-bit int\n";
					if (info.nativeFormats & RTAUDIO_SINT24)
						std::cout << "  24-bit int\n";
					if (info.nativeFormats & RTAUDIO_SINT32)
						std::cout << "  32-bit int\n";
					if (info.nativeFormats & RTAUDIO_FLOAT32)
						std::cout << "  32-bit float\n";
					if (info.nativeFormats & RTAUDIO_FLOAT64)
						std::cout << "  64-bit float\n";
				}
				if (info.sampleRates.size() < 1)
					std::cout << "No supported sample rates found!";
				else {
					std::cout << "Supported sample rates = ";
					for (unsigned int j = 0; j<info.sampleRates.size(); j++)
						std::cout << info.sampleRates[j] << " ";
				}
				std::cout << std::endl;
			}
		}
		std::cout << std::endl;
		*/
		RtAudio::StreamParameters oParams;
		oParams.deviceId = audio.getDefaultOutputDevice();
		printf("output device %d s\n", oParams.deviceId);
	#ifdef _MSC_VER
		audio_channels = 4;
	#else
		audio_channels = 2;
	#endif
		oParams.nChannels = audio_channels;
		oParams.firstChannel = 0;
		RtAudio::StreamOptions options;
		options.flags = RTAUDIO_NONINTERLEAVED;
		//options.flags = RTAUDIO_HOG_DEVICE;
		options.flags |= RTAUDIO_SCHEDULE_REALTIME;
		
		unsigned int bufferFrames = blocksize;
		
		try {
			audio.openStream(&oParams, NULL, RTAUDIO_FLOAT32, 44100, &bufferFrames, &rtAudioCallback, (void *)this, &options, &rtErrorCallback);
			audio.startStream();
			printf("audio started with buffersize %d\n", bufferFrames);
		}
		catch (RtAudioError& e) {
			e.printMessage();
			printf("audio error %s\n", e.what());
			return;
		}

	}
	#endif

	// cloudDeviceManager.reset();
	// cloudDeviceManager.devices[0].use_colour = 0;
	// cloudDeviceManager.devices[1].use_colour = 0;
	// cloudDeviceManager.open_all();
	kinectData[0] = kinectMap[0].create("../alicenode/kinect0.bin");
	kinectData[1] = kinectMap[1].create("../alicenode/kinect1.bin");
	printf("kinect state %p should be size %d\n", kinectData[0], sizeof(KinectData));
	printf("kinect state %p should be size %d\n", kinectData[1], sizeof(KinectData));

	// TODO start threads
	if (!threadsRunning) {
 		threadsRunning = true;
		
		// setup threads:
		// mFluidSeconds = 1./30.;
		// mSimulationSeconds = mFluidSeconds;
		// mHumanSeconds = mFluidSeconds;
		mFluidThread = std::thread(std::bind(&Shared::serviceFluid, this));
		mSimulationThread = std::thread(std::bind(&Shared::serviceSimulation, this));
		//mHumanThread = thread(bind(&MainApp::serviceHuman, this));
	}

	updating = 1;
	printf("initialized\n");
}


/*
void Shared::startThreads() {
	
}
*/

void Shared::exit() {

	printf("closing threads\n");
#ifdef AN_USE_AUDIO
	audio.stopStream();
	audio.closeStream();
#endif
	if (threadsRunning) {
 	 	threadsRunning = false;
	 	mFluidThread.join();
	 	mSimulationThread.join();
	// 	mHumanThread.join();
	}

	
	kinectMap[0].destroy();
	kinectMap[1].destroy();

	printf("closed threads\n");
}


void Shared::audio_callback(float * out0, float * out1, float * out2, float * out3, long blocksize) {
	
	const float samplerate = this->samplerate;
	const float r_samplerate = 1.f/samplerate;
	const glm::vec3 scale = 1.f/WORLD_DIM;
	const float osc_dimf = OSC_TABLE_DIM;
	const float r_beetle_scale = audio_gain/beetle_size;
	
	const float local_beetle_filter = beetle_filter;
	const float local_beetle_rpts = beetle_rpts;
	const float local_beetle_frequency = beetle_frequency;
	const float local_beetle_dur = beetle_dur;
	const float local_beetle_modulation = beetle_modulation;
	
	const float * env = mEnvTable;
	const float * osc = mOscTable;
	
	bool shown = false;
	
	unsigned count = 0;	// count active grains
	for (int i = 0; i < NUM_BEETLES; i++) {
		Beetle& beetle = beetles[i];
		if (beetle.grain_active) {
			count++;

			const float amp0 = beetle.grain_mix.x;
			const float amp1 = beetle.grain_mix.y;
			const float amp2 = beetle.grain_mix.z;
			const float amp3 = beetle.grain_mix.w;

			const float einc = beetle.grain_einc;
			const float oinc = beetle.grain_oinc;
			const float filter = beetle.grain_filter;
			float ephase = beetle.grain_ephase;
			float ophase = beetle.grain_ophase;
			float smoothed = beetle.grain_smoothed;
			float omod = beetle.grain_modulate;

			// figure out frame indices
			const int32_t start = beetle.grain_start;
			int32_t remain = beetle.grain_remain;
			int32_t end = start + remain;
			if (end > blocksize) end = blocksize;
			const int32_t frames = end - start;

			//printf("%d to %d\n", start, end);

			for (int32_t i = start; i < end; i++) {

				float eidx = ephase * osc_dimf;
				ephase += einc;
				uint32_t ep0(eidx);
				float ea = eidx - float(ep0);
				float e = glm::mix(env[(ep0)& OSC_TABLE_WRAP],
					env[(ep0 + 1) & OSC_TABLE_WRAP],
					ea);
				//
				//					// try filtering that instead:
				//					//e = e + 0.9998*(smoothed-e);
				//					//smoothed = e;

				//					e = 1.; //sin(M_PI * ephase);

				float oidx = ophase * osc_dimf;
				ophase += oinc * (1.f + -0.001f*omod*ophase);// + 0.001f*ophase);e*omod

				uint32_t op0(oidx);
				float oa = oidx - float(op0);
				float o = glm::mix(osc[(op0)& OSC_TABLE_WRAP],
					osc[(op0 + 1) & OSC_TABLE_WRAP],
					oa);
				//					double o = sin(M_PI * 2. * ophase);

				float oe = o * e*e;
				float s = oe + filter*(smoothed - oe);
				smoothed = s;

				out0[i] += s * amp0;
				out1[i] += s * amp1;
				out2[i] += s * amp2;
				out3[i] += s * amp3;
			}

			// update timing
			remain -= frames;

			// recycle
			if (remain <= 0) {
				//printf("ending grain with ephase %f\n", ephase);
				beetle.grain_active = 0;
			}

			beetle.grain_start = 0;
			beetle.grain_remain = remain;
			beetle.grain_smoothed = smoothed;
			beetle.grain_ephase = ephase;
			beetle.grain_ophase = ophase;
		}
	//}

	// keep picking beetles until we fill up the capacity
	//int c = NUM_BEETLES;
	//while (count < MAX_GRAINS && --c) {
	//	Beetle& beetle = beetles[rand() % NUM_BEETLES];
	//	if (beetle.alive && !beetle.grain_active) {

		else if (beetle.alive && count < NUM_BEETLES) {
			
			if (beetle.at >= 0) {
				
				// prevent overloading of DSP
				
				// schedule new grain using current beetle properties
				const float period = beetle.period;
				const float amp = beetle.scale.x * r_beetle_scale; // * (rand() % 2 ? 1. : -1.);
				const float dur = period * local_beetle_dur * beetle.scale.z; //
				//const float dur = period * beetle.scale.x * (local_beetle_dur*(2.+sin(beetle.age*2.*M_PI)));
				const float freq = local_beetle_frequency/period;
				const float filter = glm::clamp(beetle.energy * local_beetle_filter, 0.f, 1.f);
				
				beetle.grain_start = beetle.at % blocksize;
				beetle.grain_remain = dur * samplerate;
				beetle.grain_oinc = freq * r_samplerate;
				beetle.grain_ophase = 0.f; //urandom();
				beetle.grain_einc = ((local_beetle_rpts + (rand() % 4)) / dur) * r_samplerate;
				beetle.grain_ephase = 0.f;
				beetle.grain_filter = 1.-(filter*filter);
				beetle.grain_active = 1;
				beetle.grain_smoothed = 0.f;
				beetle.grain_modulate = 0.1f * local_beetle_modulation; //beetle.energy * local_beetle_modulation;
				
				// TODO: derive channel mixes from posnorm
				glm::vec3 posnorm = (beetle.pos * scale);
				//posnorm = glm::mix(posnorm, glm::linearRand(glm::vec3(0.), glm::vec3(1.)), 0.5);

				// TMAC:
				// ch0 is +x +z
				// ch1 is -x -z
				// ch2,3 are sub

				float x0 = posnorm.x;
				float x1 = 1.f-posnorm.x;
				float z0 = posnorm.z;
				float z1 = 1.f-posnorm.z;
				beetle.grain_mix = glm::vec4(
											 amp * (x0+z0)*0.5, // +x +z
											 amp * (x1+z1*0.5),  // -x -z
											 amp *  (x0+z1*0.5),  // -x +z
											 amp *  (x1+z0*0.5) 	// +x -z
											 );
				
				
				//printf("play for %d %f\n", beetle.grain_remain, samplerate);
			
				
				// schedule next grain to follow:
				beetle.at = (int32_t)(-(beetle.period * samplerate + beetle.grain_start));
				
				// this counts as playing:
				count++;
				
			} else {
				
				// keep ticking, we're in the quiet period
				beetle.at += blocksize;
			}
		}
	}
	//if (mPerfLog) 
	//if (count) std::cout << "grains: " << count << std::endl;

}

void Shared::update_daynight(double dt) {
	
	dayphase = fmod(now / 300., 1.);	// 5 minute cycle
	//phase = fmod(now / 30., 1.);	// 5 minute cycle
	
	double phaserad = M_PI * 2.0 * dayphase;
	
	// daylight is 60% of the time
	daylight = sin(phaserad)+0.25;
	daylight *= (daylight > 0.) ? 0.8 : 1.25;
	
	// modify isosurf decay at dusk/twilight:
	float t1 = sin(phaserad-0.1);
	
	//double t1 = sin(phaserad+0.3);
	//app.gui.density_decay(min(0.994, 4*t1*t1));
	
	if (daylight > 0) {
		//printf("day\n");
		density_isolevel += 0.01 * (0.8 - density_isolevel);
		density_ideal += 0.01 * (0.04 - density_ideal);
		density_diffuse += 0.01 * (0.02 - density_diffuse);
		density_decay = glm::min(0.997f, 0.7f + 2.f*t1*t1);
	} else {
		//printf("night\n");
		density_isolevel += 0.01 * (0.25 - density_isolevel);
		density_ideal += 0.01 * (0.4 - density_ideal);
		density_diffuse += 0.01 * (0.05 - density_diffuse);
		density_decay = glm::min(0.995f, 0.7f + 2.f*t1*t1);
	}
}

void Shared::update_isosurface(double dt) {
	int dimx = VOXEL_DIM.x;
	int dimy = VOXEL_DIM.y;
	int dimz = VOXEL_DIM.z;
	int dim3 = dimx*dimy*dimz;
	//isosurface.level(0.5);
	isosurface.level(density_isolevel);
	isosurface.generate(landscape.front().ptr(), dimx, dimy, dimz, 
		//1., 1., 1. // what tod_zkm had
		VOXEL_TO_WORLD.x, VOXEL_TO_WORLD.y, VOXEL_TO_WORLD.z);

	// std::vector<float> volData;
	// volData.resize(dim3);
	// float rate = M_PI * (0.1);
	// for(int z=0; z<dimz; ++z){ 
	// 	double zz = z * rate;
	// 	for(int y=0; y<dimy; ++y){ 
	// 		double yy = y * rate;
	// 		for(int x=0; x<dimx; ++x){ 
	// 			double xx = x * rate;                 
	// 			volData[((z*dimy + y)*dimx) + x] = cos(xx + now) + cos(yy + now) + cos(zz + now);
	// 		}
	// 	}
	// }
	// isosurface.generate(&volData[0], 
	// 	dimx, dimy, dimz, 
	// 	//1., 1., 1. // what tod_zkm had
	// 	VOXEL_TO_WORLD.x, VOXEL_TO_WORLD.y, VOXEL_TO_WORLD.z
	// );

	isovcount = isosurface.vertices().size() * 6;
	isoicount = isosurface.indices().size();

	// double volLengthX, volLengthY, volLengthZ;
	// isosurface.volumeLengths(volLengthX, volLengthY, volLengthZ);
	// printf("iso vertices %d indices %d dims %dx%dx%d, %fx%fx%f first %f\n", 
	// 	isovcount, isoicount,
	// 	isosurface.fieldDim(0), isosurface.fieldDim(1), isosurface.fieldDim(2), 
	// 	volLengthX, volLengthY, volLengthZ,
	// 	isosurface.vertices().elems()->position.x);

	// TODO: can we avoid this copy?
	memcpy(isovertices, &isosurface.vertices().elems()->position.x, sizeof(float) * isovcount);
	memcpy(isoindices, &isosurface.indices()[0], sizeof(uint32_t) * isoicount);
}

void Shared::serviceSimulation() {
	printf("starting sim thread\n");
	
	double dt = 1/30.;
	while(threadsRunning) {
		Timer t;
			
		//TODO: mGooUpdated = true;
		
		update_snakes(dt);
		update_beetles(dt);
		update_particles(dt);

		double elapsed = t.measure();
		if (elapsed < dt) {
			al_sleep(dt - elapsed);
			double slept = t.measure();
			//printf("goo fps %f => %f\n", 1./elapsed, 1./(elapsed + slept));
		}
	}
	printf("ending sim thread\n");
}

void Shared::serviceFluid() {
	printf("starting fluid thread\n");
	
	double dt = 1/30.;
	while(threadsRunning) {
		Timer t;
		update_fluid(dt);
		update_goo(dt);
		double elapsed = t.measure();
		if (elapsed < dt) {
			al_sleep(dt - elapsed);
			double slept = t.measure();
			//printf("fluid fps %f => %f\n", 1./elapsed, 1./(elapsed + slept));
		}
	}
	printf("ending fluid thread\n");
}

void Shared::update_fluid(double dt) {
	// update fluid
	Field3D<>& velocities = fluid.velocities;
	
	const size_t stride0 = velocities.stride(0);
	const size_t stride1 = velocities.stride(1);
	const size_t stride2 = velocities.stride(2);
	const size_t dim0 = velocities.dimx();
	const size_t dim1 = velocities.dimy();
	const size_t dim2 = velocities.dimz();
	const size_t dimwrap0 = dim0-1;
	const size_t dimwrap1 = dim1-1;
	const size_t dimwrap2 = dim2-1;

	const size_t DIM3 = dim0*dim1*dim2;
	glm::vec3 * data = (glm::vec3 *)velocities.front().ptr();

	update_density(dt);
	
	//add to fluid:
	{
		// add human flow field effects:
		for (int i = 0; i < NUM_VOXELS; i++) {
			glm::vec3 * f = (glm::vec3 *)fluid.velocities.front()[i];
			glm::vec3 * src = density_change[i];

			*f += human_fluid_smoothing * ((*src) * human_flow - (*f));
		}
	}
	
	// add some turbulence:
	if (0) {
		int fluid_noise_count = 1;
		float fluid_noise = 10.;
	
		for (int i=0; i<fluid_noise_count; i++) {
			// pick a cell at random:
			glm::vec3 * cell = data + (rand() % DIM3);
			// add a random vector:
			*cell = glm::sphericalRand(glm::linearRand(0.f, fluid_noise));
		}
	}
	if (1) {
		
		apply_fluid_boundary(data, (float *)landscape.front().ptr(), dim0, dim1, dim2);
		
		velocities.diffuse(fluid_viscocity, fluid_passes);
		
		// apply boundaries:
		apply_fluid_boundary(data, (float *)landscape.front().ptr(), dim0, dim1, dim2);
		// stabilize:
		fluid.project(fluid_passes/2);
		// advect:
		velocities.advect(velocities.back(), fluid_advection);
		
		velocities.front().scale(fluid_decay);
		
		// apply boundaries:
		apply_fluid_boundary(data, (float *)landscape.front().ptr(), dim0, dim1, dim2);
		
		// clear gradients:
		fluid.gradient.front().zero();
		fluid.gradient.back().zero();
	} else {
		fluid.velocities.diffuse(fluid_viscocity, fluid_passes);
		// apply boundaries:
		apply_fluid_boundary(data, (float *)landscape.front().ptr(), dim0, dim1, dim2);
		
		// stabilize:
		fluid.project(fluid_passes);
		// advect:
		velocities.advect(velocities.back(), fluid_advection);
		// apply boundaries:
		apply_fluid_boundary(data, (float *)landscape.front().ptr(), dim0, dim1, dim2);
		
		// stabilize:
		fluid.project(fluid_passes);
		// decay:
		velocities.front().scale(fluid_decay);
		// apply boundaries:
		apply_fluid_boundary(data, (float *)landscape.front().ptr(), dim0, dim1, dim2);
		
		// clear gradients:
		fluid.gradient.front().zero();
		fluid.gradient.back().zero();
	
	}

}

void Shared::apply_fluid_boundary(glm::vec3 * velocities, const float * landscape, const size_t dim0, const size_t dim1, const size_t dim2) {
	
	const float friction = 1.f - fluid_boundary_friction;
	
	//const float influence_offset = -world.fluid_boundary_damping;
	//const float influence_scale = 1.f/world.fluid_boundary_damping;
	
	// probably don't need the triple loop here -- could do it cell by cell.
	int i=0;
	for (size_t z=0; z<dim2; z++) {
		for (size_t y=0; y<dim1; y++) {
			for (size_t x=0; x<dim0; x++, i++) {
				
				glm::vec3 normal(0.);
				float influence = 0.f;
				if (x == 0) {
					normal += glm::vec3(1., 0, 0);
					influence = 1.f;
				} else if (x == (dim0-1)) {
					normal += glm::vec3(-1., 0, 0);
					influence = 1.f;
				} 
				if (y == 0) {
					normal += glm::vec3(0, 1, 0);
					influence = 1.f;
				} else if (y == (dim1-1)) {
					normal += glm::vec3(0, -1, 0);
					influence = 1.f;
				} 
				if (z == 0) {
					normal += glm::vec3(0, 0, 1);
					influence = 1.f;
				} else if (z == (dim2-1)) {
					normal += glm::vec3(0, 0, -1);
					influence = 1.f;
				} 
				{
					// look at field:
					
					//const float distance = fabsf(land.w);
					//const float inside = sign(land.w);	// do we care?
					//influence = clamp((distance + influence_offset) * influence_scale, 0., 1.);
					//normal = glm::vec3(land);	// already normalized?
					
					// TODO: limit fluid by isosurface
					
				}
				
				if (influence > 0.f) {
					
					normal = glm::normalize(normal);	// necssary for corners at least
					
					
					glm::vec3& vel = velocities[i];
					glm::vec3 veln = safe_normalize(vel);
					float speed = glm::length(vel);
					
					// get the projection of vel onto normal axis
					// i.e. the component of vel that points in either normal direction:
					glm::vec3 normal_component = normal * (glm::dot(vel, normal));
					
					// remove this component from the original velocity:
					glm::vec3 without_normal_component = vel - normal_component;
					
					// and re-scale to original magnitude:
					// with some loss for friction:
					glm::vec3 rescaled = safe_normalize(without_normal_component) * speed * friction;
					
					// update:
					vel = glm::mix(rescaled, vel, influence);
	
				}
			}
		}
	}
}
void Shared::update_density(double dt){
	
	// update which voxels are occupied by humans:
	memset(human_present, 0, sizeof(human_present));
	for (unsigned i = 0; i<ghostcount; i++) {
		glm::vec4& v = ghostpoints[i];
		int idx = voxel_hash(v.x, v.y, v.z);
		if (idx != INVALID_VOXEL_HASH) {
			human_present[idx] = 1;
		}
	}
	
	Array<float>& arr = density.front();
	float * front = density.front().ptr();
	float * back = density.back().ptr();

	size_t dimx = VOXEL_DIM.x;
	size_t dimy = VOXEL_DIM.y;
	size_t dimz = VOXEL_DIM.z;

	// update density field
	for (unsigned z=0; z<dimx; z++) {
		for (unsigned y=0; y<dimy; y++) {
			for (unsigned x=0; x<dimz; x++) {
				int idx = voxel_hash(x, y, z);
				float human = human_present[idx];
				float f1 = human;
				int i = arr.index(x, y, z);
				float f0 = front[i];
				
				// zero the field here if a human is present:
				if (human > 0.) landscape.front().cell(x, y, z)[0] = 0;

				// with smoothing to remove sensor noise:
				f1 = f0 + human_smoothing * (f1-f0);
				back[i] = f1;
				//occupied[idx] = 0; // this is done by voxel clearing anyway

			}
		}
	}
	density.swap();
	density.diffuse(density_diffuse);
	
	density_gradient.swap();	// move previous gradient into density_gradient.back()
	density.calculateGradient(density_gradient.front());

	// update density_change:
	bool shown = 0;
	int i = 0;
	for (unsigned z = 0; z<dimx; z++) {
		for (unsigned y = 0; y<dimy; y++) {
			for (unsigned x = 0; x<dimz; x++) {
				glm::vec3 prev = density_gradient.back().ptr()[i];
				glm::vec3 cur = density_gradient.front().ptr()[i];

				// mag should depend on the change in *density*
				float mag = fabsf(density.front().ptr()[i] - density.back().ptr()[i]);

				glm::vec3 dir = safe_normalize(cur);		// or avg cur&prev, or cur - prev, etc.?

				glm::vec3 change = density_change.ptr()[i];
				// smooth it:
				density_change.ptr()[i] = glm::mix(change, dir * mag, 1.);

				i++;
			}
		}
	}

	// TODO:
	
	// compute flow:
	/*
	 a		b		e		f		f-e		a+b		(f-e)*(a+b)
	 l1-l0	r1-r0	r1-l0	l1-r0
	 .. -> .. = 0	0		0		0		0		0		0		0
	 .. -> ++ = 0	+		+		+		+		0		++		0
	 ++ -> .. = 0	-		-		-		-		0		--		0
	 ++ -> ++ = 0	0		0		0		0		0		0		0
	 +. -> .+ = 0	-		+		0		0		0		0		0
	 .+ -> +. = 0	+		-		0		0		0		0		0
	 
	 +. -> +. = 0	0		0		-		+		++		0		0
	 .+ -> .+ = 0	0		0		+		-		--		0		0
	 
	 
	 .. -> +. = +	+		0		0		+		+		+		+
	 +. -> ++ = +	0		+		0		+		+		+		+
	 ++ -> .+ = +	-		0		0		-		-		-		+
	 .+ -> .. = +	0		-		0		-		-		-		+
	 
	 .. -> .+ = -	0		+		+		0		-		+		-
	 .+ -> ++ = -	+		0		+		0		-		+		-
	 ++ -> +. = -	0		-		-		0		+		-		-
	 +. -> .. = -	-		0		-		0		+		-		-
	 
	 velocity = (f-e) * (b+a) = (l0+l1 -r0-r1) * (l1+r1 -l0-r0)
	 */
	
	// note boundaries:
	
	/*

	front = density.front().ptr();
	back = density.back().ptr();

	unsigned z0 = 0;
	for (unsigned z=1; z<=DIMWRAP; z++) {
		unsigned y0 = 0;
		for (unsigned y=1; y<=DIMWRAP; y++) {
			unsigned x0 = 0;
			
			int idx = arr.index(0, y, z);
			
			float npp1 = front[idx];
			float npp0 = back[idx];
			for (unsigned x=1; x<=DIMWRAP; x++) {
				idx = arr.index(x, y, z);
				//printf("idx %d\n", idx);
				
				// compute flow from density:
				// positive direction
				float ppp1 = front[idx];
				float ppp0 = back[idx];
				// negative directions
				// (x-axis can just use the previous loop value)
				float pnp1 = front[arr.index(x, y0, z)];
				float pnp0 = back[arr.index(x, y0, z)];
				float ppn1 = front[arr.index(x, y, z0)];
				float ppn0 = back[arr.index(x, y, z0)];
				
				float delta = ppp1-ppp0;
				float total = ppp1+ppp0;
				
				// this seems to capture direction properly; not sure about magnitude...
				// in English: flow = (negativegain + positivegain) * (negativeshift - positiveshift)
				// re-arranged & simplified a bit:
				vec3 flow(
						   (npp1-npp0 + delta) * (npp1+npp0 - total) * human_flow,
						   (pnp1-pnp0 + delta) * (pnp1+pnp0 - total) * human_flow,
						   (ppn1-ppn0 + delta) * (ppn1+ppn0 - total) * human_flow
						   );
				// cheat:
				//flow *= vec3(2., 0.5, 1.);
				
				// add to fluid:
				fluid.velocities.front().add(vec3(x, y, z), &flow.x);
				//fluid.velocities.front().add(x, y, z, &flow.x);
				
				// save a couple of lookups:
				npp0 = ppp0;
				npp1 = ppp1;
				x0 = x;
			}
			y0 = y;
		}
		z0 = z;
	}*/
	
#ifdef AN_USE_CV_FLOW
	// or, CV-based turbulence:
	for (int k=0; k<2; k++) {
		KinectData& kd = kdata[k];
		int i = 0;
		for (unsigned y=0; y<240; y++) {
			for (unsigned x=0; x<320; x++, i++) {
				
				float fx = kd.hsvx[i];
				float fy = kd.hsvy[i];
				vec3 flow(fx * human_cv_flow, -fy * human_cv_flow, 0.);
				vec3 where = kd.world[x*2 + y*2*640];
				
				//printf("%f %f %f @ %f %f %f\n", flow[0], flow[1], flow[2], x1, y1, z1);
				//fluid.velocities.front().add(where, &flow.x);
				fluid.velocities.front().add((unsigned)(where.x * WORLD_TO_VOXEL.x),
											 (unsigned)(where.y * WORLD_TO_VOXEL.y),
											 (unsigned)(where.z * WORLD_TO_VOXEL.z),
											 &flow.x);
			}
		}
	}
#endif
}



void Shared::update_goo(double dt) {
	// TODO
	landscape.diffuse(density_diffuse);
	landscape.advect(fluid.velocities.front(), goo_rate);
	landscape.front().scale(density_decay);
	
	// flickery
	//landscape.clump(density_diffuse, density_ideal, 10);
	
//	for (int i=0; i<NUM_VOXELS; i++) {
//		//glm::vec4 * l = (glm::vec4 *)landscape.front()[i];
//		//*l = glm::linearRand(glm::vec4(0.), glm::vec4(1.));
//		
//	}
}

void Shared::update_cloud() {

	glm::mat4 vive2world = glm::mat4();
	vive2world = glm::translate(vive2world, glm::vec3(3.5f, 0.f, 2.f));
	vive2world = glm::rotate(vive2world, float(M_PI/2.), glm::vec3(0.f, 1.f, 0.f));

	ghostcount = 0;
	int g=0;
	for (int i=0; i<2; i++) {
		if (kinectData[i] == nullptr) continue;

		const CloudFrame& frame = kinectData[i]->cloudFrame();

		for (int k=0; k<cDepthWidth*cDepthHeight && g < NUM_GHOSTPOINTS; k++) {
			const uint16_t& mm = frame.depth[k];
			if (mm) {
				glm::vec3 world = transform(vive2world, frame.xyz[k]);
				if (world.x > 0.5f && world.x < WORLD_DIM.x - 0.5f &&
				    world.y > 0.4f && world.y < 2.f &&
					world.z > 0.5f && world.z < WORLD_DIM.z - 0.5f ) {
					ghostpoints[g++] = glm::vec4(world, 1.f);
				}
			}
		}
	}
	ghostcount = g;
	//printf("kinect %d %d coutn %d, frist %s\n", kinectData[0]->lastCloudFrame, kinectData[1]->lastCloudFrame, ghostcount, glm::to_string(ghostpoints[0]).data());

	
}

void Shared::update_snakes(double dt) {
	for (int i=0; i<NUM_SNAKES; i++) {
		//if (i) return; // for debugging
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
		if (self.nearest_beetle != INVALID_VOXEL_HASH) {
			// just eat this one.
			self.victim = self.nearest_beetle;
			torque_tension = torque_tension * 2;
			
		} else if (self.victim == INVALID_VOXEL_HASH) {
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
					self.victim = INVALID_VOXEL_HASH;
				} else {
					speed = speed * 5;
				}
			} else {
				// no point chasing:
				self.victim = INVALID_VOXEL_HASH;
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
				float goo = snake_goo_add*s;
				landscape.front().add((segment->a_location - uf1)*WORLD_TO_VOXEL, &goo);				
				if (segment->a_location.y < WORLD_CENTER.y) target = target + snake_levity; // do this for all segments?

				// fluid & goo:
				//vec3 suck = uf1 * (-alpha * 0.5f);
				// C.an_fluid_add_velocity(pos, suck)
				glm::vec3 suck = uf1 * (-speed * alpha * snake_fluid_suck);
				//printf("suck %f %f %f\n", suck.x, suck.y, suck.z);
				fluid_velocity_add(segment->a_location, suck);
			}
			
			// actually move it:
			segment->a_location = target;
			
			qtarget = segment->a_orientation;
			prev = segment;
		}
		self.energy = glm::max(0.f, self.energy - snake_decay);
	}
}

void Shared::move_snakes(double dt) {
	
	// crashing in here
	for (int i=0; i<NUM_SNAKES; i++) {
		Snake& self = snakes[i];
		SnakeSegment& head = snakeSegments[self.segments[0]];
		self.nearest_beetle = INVALID_VOXEL_HASH;
		uint32_t hash = voxel_hash(head.a_location);
		if (hash != INVALID_VOXEL_HASH) {
			#ifdef AN_USE_HASHSPACE
			// TODO:
			// find near agents:
			float range_of_view = 1./32.;
			int32_t id = hashspaceBeetles.first(head.a_location, hashspaceBeetles.invalidObject(), range_of_view, 0.f, false);
			if (id != hashspaceBeetles.invalidObject()) {
				hashspaceBeetles.remove(id);
				self.nearest_beetle = id;
			} else {
				self.nearest_beetle = INVALID_VOXEL_HASH;
			}
			#else
			Voxel& voxel = voxels[hash];
			if (voxel.beetles) {
				//printf("voxel %p %p\n", voxel.beetles, voxel.particles);
				Beetle * b = voxel_pop_beetle(voxel);
				//printf("voxel %p %p beetle %p \n", voxel.beetles, voxel.particles, b);
				uint32_t id = b->id;
				
				self.nearest_beetle = id;
			}
			#endif
				
		} 
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
			float grad;
			fluid.gradient.front().read_interp<float>(WORLD_TO_VOXEL.x * self.pos.x, WORLD_TO_VOXEL.y * self.pos.y, WORLD_TO_VOXEL.z * self.pos.z, &grad);
			//printf("beetle %i grad %f flow %s\n", i, grad, glm::to_string(flow).data());
			
			
			if (self.alive) {
				
				glm::vec3 force = flow * beetle_push;

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
				float turn = beetle_max_turn;// * beetle_size/(beetle_size+s*s);
				self.azimuth = rnd::bi() * 0.1 * turn;
				self.elevation = rnd::uni() * 0.02 * turn;
				self.bank = rnd::bi() * 0.1 * 0.1 * turn;
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

						// chance of inheriting
						if (rnd::uni() > mutability) { 
							// inaccuracy of inheritance:
							child.genes = glm::mix(self.genes, child.genes, mutability);
						}
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

				// do this whether dead or alive:
				self.vel *= beetle_friction;
				self.angvel *= beetle_friction_turn;
				
				// integrate forces:
				self.vel += force * SPACE_ADJUST;

				
				// fluid & goo:
				//vec3 suck = uf1 * (-alpha * 0.5f);
				// C.an_fluid_add_velocity(pos, suck)
				glm::vec3 suck = -force * beetle_fluid_suck;
				//printf("suck %f %f %f\n", suck.x, suck.y, suck.z);
				fluid_velocity_add(self.pos, suck);
				
			} else {
				// dead, but still visible:
				static const float grey = 0.4;
				self.color.r += 0.2*(self.color.g - self.color.r);
				self.color.g += 0.2*(grey - self.color.g);
				self.color.b += 0.2*(self.color.g - self.color.b);
				
				// fade out:
				self.scale *= beetle_dead_fade;
				if (self.scale.x < 0.005) {
					beetle_pool_push(self);	// and recycle
											//printf("recyle of %d\n", self.id);
				}
				
				
				self.angvel *= beetle_friction_turn;
				self.vel = flow * beetle_push;
			}
			
			
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
	self.scale = glm::vec3(0.01);
	self.vel = glm::vec3(0);
	self.angvel = glm::vec3(0);
	self.period = beetle_base_period * 0.1*(rnd::uni()*9. + 1.);
	self.genes = glm::linearRand(glm::vec3(0), glm::vec3(1));
}

void Shared::move_beetles(double dt) {
	//const float dimf = float(DIM);
	//const float rdimf = 1.f/dimf;
	const glm::vec3 posmin(.1f);
	const glm::vec3 posmax = WORLD_DIM - (posmin * 2.f);
	live_beetles = 0;
	for (int i=0; i<NUM_BEETLES; i++) {
		Beetle& self = beetles[i];
		BeetleAudioData& bad = audioData.shared->beetles[i];
		if (!self.recycle) {

			glm::quat angular = quat_fromEulerXYZ(self.angvel);	// ambiguous order of operation...
			self.orientation = safe_normalize(angular * self.orientation);
			/// TODO:
			//self.pos = glm::clamp(self.pos + (self.vel), posmin, posmax);
			self.pos = glm::clamp(self.pos + (self.vel), posmin, posmax);
			uint32_t hash = voxel_hash(self.pos);
			if (hash != INVALID_VOXEL_HASH) {
				#ifdef AN_USE_HASHSPACE
					// TODO:
					// find near agents:
					float range_of_view = 0.001;
					int32_t id = hashspaceParticles.first(self.pos, -1, range_of_view, 0.f, false);
					if (id != hashspaceParticles.invalidObject()) {
						hashspaceParticles.remove(id);
						self.nearest_particle = &particles[id];
						printf("yes %d\n", id);
					} else {
						//printf("no\n");
						self.nearest_particle = nullptr;
					}

					hashspaceBeetles.move(i, self.pos);
				#else 
					voxel_push(voxels[hash], self);
					if (self.alive) {
						self.nearest_particle = voxel_pop_particle(voxels[hash]);
					} else {
						self.nearest_particle = nullptr;
					}
				#endif
			} else {
				#ifdef AN_USE_HASHSPACE
					hashspaceBeetles.remove(i);
				#endif
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
		// copy audio data:
		if (self.alive) {
			bad.age = self.age;
			bad.energy = self.energy;
			bad.period = self.genes.x;
			bad.freq = self.genes.y;
			bad.modulation = self.genes.z;
			bad.scale = self.scale.x / beetle_size;
			bad.pos_xz.x = self.pos.x / WORLD_DIM.x;
			bad.pos_xz.y = self.pos.z / WORLD_DIM.z;
		} else {
			bad.age = 0;
		}
	}
	//printf("live beetles %d\n", live_beetles);

	audioData.sync();
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
			bool useghost = local_ghostcount > 100 && rnd::uni() < ghost_chance;
			//bool useghost = rand() % 40000 < ghostcount;
			if (useghost) {
				//
				int idx = rand() % local_ghostcount;
				o.pos = ghostpoints[idx] + glm::vec4(glm::ballRand(0.01f), 0.);
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
		glm::vec3 pos3 = glm::vec3(pos);
		glm::vec3 flow = fluid_velocity(pos3);
		//fluid.velocities.front().read_interp<float>(pos.x, pos.y, pos.z, &flow.x);
		
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
		
		#ifdef AN_USE_HASHSPACE
		hashspaceParticles.move(i, newpos);
		#else 
		uint32_t hash = voxel_hash(newpos);
		if (!o.dead && hash != INVALID_VOXEL_HASH) {
			voxel_push(voxels[hash], o);
		}
		#endif

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


	// would like to move this to another thread
	// but would need to doublebuffer ghostpoints for that to work
	// (or ringbuffer it)
	shared.update_cloud();

	shared.update_daynight(dt);
	shared.update_isosurface(dt);

	
	if (shared.updating) {
		// clear all the voxels:
		shared.clear_voxels();
		// update all the things that use voxels:
		shared.move_particles(dt);
		shared.move_beetles(dt);
		shared.move_snakes(dt);
	}
	return nullptr;
}



napi_value close(napi_env env, napi_callback_info info) {
	napi_status status = napi_ok;

	shared.exit();

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
		{ "close", 0, close, 0, 0, 0, napi_default, 0 },
		{ "update", 0, update, 0, 0, 0, napi_default, 0 },
	};
	status = napi_define_properties(env, exports, 3, properties);

	audioData.create("beetles.bin", true);

	assert(status == napi_ok);
	return exports;
}

NAPI_MODULE(NODE_GYP_MODULE_NAME, init)