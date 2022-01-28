/*reference https://github.com/MorganTrench/ParticleSimulator
 */
#include "Particle2.hpp"

#ifndef INTERACTION_HEADER
#define INTERACTION_HEADER


	class Interaction{
		Particle *particles;
		int *n;
		int wP;

	public:
		Interaction(Particle *p, int *n);
		void interact();
		void doTask();

	private:
		void resetAccelerations();
		void constant(Particle *p, double acc[3]);
		void gravity(Particle *p1, Particle *p2);
		void interParticleGravity(int index);
		void drag(Particle *p);
	};

#endif
