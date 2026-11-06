/* test_newtonianFlight.cpp
Copyright (c) 2026 by Jake Phillips

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#include "es-test.hpp"

// Include only the tested class's header.
#include "../../../source/ship/NewtonianFlight.h"

// ... and any system includes needed for the test file.
#include <cmath>

namespace { // test namespace

// #region mock data

using namespace NewtonianFlight;
using Catch::Matchers::WithinAbs;

constexpr double PI = 3.14159265358979323846;
constexpr double TO_RAD = PI / 180.;
constexpr double TO_DEG = 180. / PI;

// A mid-sized ship with the default Newtonian tuning: the built-in retro and
// lateral thrust are 19% of the main engine, and spin-up takes 10 frames.
constexpr double RATED_SPEED = 6.;
constexpr double FORWARD = .3;
constexpr double BASELINE = FORWARD * .19;
constexpr double RATED_TURN = 3.;
constexpr double ANGULAR_ACCEL = RATED_TURN / 10.;
constexpr double SOFT_CAP = 2.5;

Thrusters ShipThrusters(double afterburner = 0.)
{
	Thrusters thrusters;
	thrusters.forward = FORWARD;
	thrusters.afterburner = afterburner;
	thrusters.retro = BASELINE;
	thrusters.lateral = BASELINE;
	return thrusters;
}

Input Thrust(double thrust = 1.)
{
	Input input;
	input.thrust = thrust;
	return input;
}

Input Stop()
{
	Input input;
	input.stop = true;
	return input;
}

// Facing in degrees, 0 pointing "up" (0, -1) and positive turning right, as Angle does.
Point Unit(double degrees)
{
	return Point(sin(degrees * TO_RAD), -cos(degrees * TO_RAD));
}

// Degrees to turn right to face the given direction, in [-180, 180].
double DeltaTo(double degrees, const Point &direction)
{
	return remainder(atan2(direction.X(), -direction.Y()) * TO_DEG - degrees, 360.);
}

// One ship flown frame by frame the way Ship::DoNewtonianMovement does, with
// unlimited energy so that every planned burn is delivered.
struct Sim {
	bool assist = true;
	double angle = 0.;
	double angularVelocity = 0.;
	Point velocity;
	double rated = RATED_SPEED;
	Thrusters thrusters = ShipThrusters();

	TranslationPlan Step(const Input &input, double turnInput = 0.)
	{
		// Translation uses the facing from the start of the frame.
		const Point facing = Unit(angle);
		RotationPlan rotation = PlanRotation(assist, turnInput, angularVelocity, RATED_TURN, ANGULAR_ACCEL, SOFT_CAP);
		angularVelocity += rotation.deltaOmega * rotation.capFactor;
		if(std::fabs(angularVelocity) < 1e-6)
			angularVelocity = 0.;
		angle += angularVelocity;

		TranslationPlan plan = PlanTranslation(assist, velocity, facing, rated, thrusters, input, SOFT_CAP);
		velocity = Apply(velocity, facing, plan.dvForward, plan.dvLateral, plan.softCapFactor);
		if(assist && input.stop && velocity.Length() < 1e-3)
			velocity = Point();
		return plan;
	}
};

// #endregion mock data



// #region unit tests
SCENARIO( "The soft cap above the rated speed", "[NewtonianFlight][softCap]" ) {
	GIVEN( "a rated speed and the default soft cap" ) {
		THEN( "pushes at or below the rated speed are fully effective" ) {
			CHECK( SoftCapFactor(0., RATED_SPEED, SOFT_CAP) == 1. );
			CHECK( SoftCapFactor(RATED_SPEED, RATED_SPEED, SOFT_CAP) == 1. );
		}
		THEN( "a push at the soft cap is 10% effective" ) {
			CHECK_THAT( SoftCapFactor(RATED_SPEED * SOFT_CAP, RATED_SPEED, SOFT_CAP),
				WithinAbs(SOFT_CAP_RESIDUAL, 1e-12) );
		}
		THEN( "the effectiveness falls steadily but never reaches zero" ) {
			double last = 1.;
			for(double speed = RATED_SPEED + .1; speed < RATED_SPEED * 10.; speed += .1)
			{
				double factor = SoftCapFactor(speed, RATED_SPEED, SOFT_CAP);
				CHECK( factor < last );
				CHECK( factor > 0. );
				last = factor;
			}
		}
	}
	GIVEN( "no rated speed or no soft cap" ) {
		THEN( "there is no cap" ) {
			CHECK( SoftCapFactor(10., 0., SOFT_CAP) == 1. );
			CHECK( SoftCapFactor(10., RATED_SPEED, 1.) == 1. );
		}
	}
}

SCENARIO( "Coasting down with no thrust input", "[NewtonianFlight][coast]" ) {
	Sim sim;
	// Sliding sideways and forward at once, so both manoeuvring groups are involved.
	const Point start = Unit(30.) * RATED_SPEED;
	sim.velocity = start;
	GIVEN( "flight assist is on" ) {
		THEN( "the computer brakes on the manoeuvring thrusters, at their pace" ) {
			int frames = 0;
			while(sim.velocity.Length() > 1e-9 && frames < 1000)
			{
				double before = sim.velocity.Length();
				TranslationPlan plan = sim.Step(Input());
				++frames;
				CHECK_FALSE( plan.braking );
				// Never faster than the weaker group, and never reversing.
				CHECK( before - sim.velocity.Length() <= BASELINE + 1e-9 );
				CHECK( sim.velocity.Dot(start) >= 0. );
			}
			CHECK( frames == static_cast<int>(std::ceil(RATED_SPEED / BASELINE)) );
		}
		THEN( "once stopped it stays stopped" ) {
			for(int i = 0; i < 200; ++i)
				sim.Step(Input());
			CHECK( sim.velocity.Length() < 1e-9 );
		}
	}
	GIVEN( "flight assist is off" ) {
		sim.assist = false;
		THEN( "there is no drag: the ship keeps its velocity" ) {
			for(int i = 0; i < 600; ++i)
				sim.Step(Input());
			CHECK_THAT( sim.velocity.X(), WithinAbs(start.X(), 1e-12) );
			CHECK_THAT( sim.velocity.Y(), WithinAbs(start.Y(), 1e-12) );
		}
	}
}

SCENARIO( "Flight assist caps the speed at the rated speed", "[NewtonianFlight][rated]" ) {
	Sim sim;
	GIVEN( "full thrust from rest" ) {
		THEN( "the speed rises to the rated speed and never above it" ) {
			int framesToRated = 0;
			for(int i = 0; i < 600; ++i)
			{
				sim.Step(Thrust());
				CHECK( sim.velocity.Length() <= RATED_SPEED + 1e-9 );
				if(!framesToRated && sim.velocity.Length() >= RATED_SPEED - 1e-9)
					framesToRated = i + 1;
			}
			CHECK( framesToRated == static_cast<int>(std::ceil(RATED_SPEED / FORWARD)) );
			CHECK_THAT( sim.velocity.Length(), WithinAbs(RATED_SPEED, 1e-9) );
		}
	}
	GIVEN( "the ship is at rated speed and turns 90 degrees" ) {
		sim.velocity = Unit(90.) * RATED_SPEED;
		WHEN( "it thrusts along its new facing" ) {
			sim.Step(Thrust());
			THEN( "it slides: the velocity bends gradually without gaining speed" ) {
				CHECK( sim.velocity.Length() <= RATED_SPEED + 1e-9 );
				CHECK( std::fabs(DeltaTo(90., sim.velocity)) < 5. );
			}
			AND_WHEN( "it keeps thrusting" ) {
				for(int i = 0; i < 300; ++i)
					sim.Step(Thrust());
				THEN( "it ends up flying along its facing at rated speed" ) {
					CHECK( sim.velocity.Unit().Dot(Unit(0.)) > .9999 );
					CHECK_THAT( sim.velocity.Length(), WithinAbs(RATED_SPEED, 1e-9) );
				}
			}
		}
	}
}

SCENARIO( "The afterburner raises the rated speed", "[NewtonianFlight][afterburner]" ) {
	Sim sim;
	const double boostedRated = RATED_SPEED * 1.5;
	GIVEN( "the afterburner is held with thrust" ) {
		sim.rated = boostedRated;
		sim.thrusters = ShipThrusters(.2);
		for(int i = 0; i < 600; ++i)
			sim.Step(Thrust());
		THEN( "the ship reaches the boosted rated speed" ) {
			CHECK_THAT( sim.velocity.Length(), WithinAbs(boostedRated, 1e-9) );
		}
		WHEN( "the afterburner is released but thrust is held" ) {
			sim.rated = RATED_SPEED;
			sim.thrusters = ShipThrusters();
			THEN( "assist brakes back down to the rated speed and holds it there" ) {
				double last = sim.velocity.Length();
				bool sawBraking = false;
				for(int i = 0; i < 600; ++i)
				{
					TranslationPlan plan = sim.Step(Thrust());
					sawBraking |= plan.braking;
					CHECK( sim.velocity.Length() <= last + 1e-9 );
					last = sim.velocity.Length();
				}
				CHECK( sawBraking );
				CHECK_THAT( sim.velocity.Length(), WithinAbs(RATED_SPEED, 1e-9) );
				CHECK_FALSE( sim.Step(Thrust()).braking );
			}
		}
	}
}

SCENARIO( "Flight assist off has a soft cap, not a speed limit", "[NewtonianFlight][softCap]" ) {
	Sim sim;
	sim.assist = false;
	GIVEN( "full thrust from rest" ) {
		THEN( "the speed passes the rated speed and keeps rising, ever more slowly" ) {
			double last = 0.;
			double lastGain = FORWARD;
			double gainAtSoftCap = 0.;
			for(int i = 0; i < 20000 && !gainAtSoftCap; ++i)
			{
				sim.Step(Thrust());
				double speed = sim.velocity.Length();
				double gain = speed - last;
				REQUIRE( gain > 0. );
				CHECK( gain <= lastGain + 1e-12 );
				if(last < RATED_SPEED * SOFT_CAP && speed >= RATED_SPEED * SOFT_CAP)
					gainAtSoftCap = gain;
				last = speed;
				lastGain = gain;
			}
			REQUIRE( gainAtSoftCap > 0. );
			// The factor is taken at the start-of-frame speed, so it's just over 10%.
			CHECK_THAT( gainAtSoftCap / FORWARD, WithinAbs(SOFT_CAP_RESIDUAL, .01) );
		}
	}
	GIVEN( "the ship is far above rated speed" ) {
		sim.velocity = Unit(0.) * RATED_SPEED * 3.;
		WHEN( "it fires its retro thrusters" ) {
			sim.Step(Thrust(-1.));
			THEN( "braking is not soft-capped" ) {
				CHECK_THAT( sim.velocity.Length(), WithinAbs(RATED_SPEED * 3. - BASELINE, 1e-12) );
			}
		}
		WHEN( "STOP is pressed" ) {
			sim.Step(Stop());
			THEN( "nothing happens: STOP needs the flight computer" ) {
				CHECK_THAT( sim.velocity.Length(), WithinAbs(RATED_SPEED * 3., 1e-12) );
			}
		}
	}
}

SCENARIO( "Rotation has angular momentum", "[NewtonianFlight][rotation]" ) {
	Sim sim;
	GIVEN( "flight assist is on" ) {
		THEN( "a full turn input spins up to the rated turn rate and no faster" ) {
			for(int i = 1; i <= 30; ++i)
			{
				sim.Step(Input(), 1.);
				CHECK_THAT( sim.angularVelocity, WithinAbs(std::min(RATED_TURN, i * ANGULAR_ACCEL), 1e-9) );
			}
		}
		WHEN( "the turn input is released" ) {
			for(int i = 0; i < 30; ++i)
				sim.Step(Input(), 1.);
			THEN( "the computer stops the spin within the spin-up time, without reversing" ) {
				for(int i = 0; i < 10; ++i)
				{
					sim.Step(Input());
					CHECK( sim.angularVelocity >= 0. );
				}
				CHECK( sim.angularVelocity == 0. );
			}
		}
	}
	GIVEN( "flight assist is off" ) {
		sim.assist = false;
		for(int i = 0; i < 10; ++i)
			sim.Step(Input(), 1.);
		WHEN( "the turn input is released" ) {
			double omega = sim.angularVelocity;
			for(int i = 0; i < 100; ++i)
				sim.Step(Input());
			THEN( "the ship keeps spinning" ) {
				CHECK( sim.angularVelocity == omega );
			}
		}
		WHEN( "the turn input is held" ) {
			for(int i = 0; i < 100; ++i)
				sim.Step(Input(), 1.);
			THEN( "the spin passes the rated turn rate, but the soft cap slows it" ) {
				CHECK( sim.angularVelocity > RATED_TURN );
				CHECK( sim.angularVelocity < RATED_TURN + 100 * ANGULAR_ACCEL * .5 );
			}
		}
	}
}

SCENARIO( "Turning toward a target angle stops on it", "[NewtonianFlight][turnToward]" ) {
	for(bool assist : {true, false})
		for(double target : {30., 90., 180., -135.})
		{
			DYNAMIC_SECTION( "assist " << (assist ? "on" : "off") << ", target " << target << " degrees" ) {
				Sim sim;
				sim.assist = assist;
				double overshoot = 0.;
				for(int i = 0; i < 600; ++i)
				{
					double delta = remainder(target - sim.angle, 360.);
					double turn = TurnToward(assist, delta, sim.angularVelocity, RATED_TURN, ANGULAR_ACCEL,
						std::fabs(delta) < 1.);
					sim.Step(Input(), turn);
					double after = remainder(target - sim.angle, 360.);
					// Passing the target means the remaining delta changed sign.
					if(after * delta < 0.)
						overshoot = std::max(overshoot, std::fabs(after));
				}
				CHECK( overshoot < 1. );
				CHECK( std::fabs(remainder(target - sim.angle, 360.)) < 1. );
				CHECK( std::fabs(sim.angularVelocity) <= ANGULAR_ACCEL );
			}
		}
}

SCENARIO( "Flip and burn", "[NewtonianFlight][flip]" ) {
	GIVEN( "a fast ship with weak retro thrust" ) {
		const Point velocity = Unit(0.) * 20.;
		THEN( "flipping is faster than braking on the retro thrusters" ) {
			CHECK( ShouldFlipAndBurn(velocity, Unit(0.), FORWARD, BASELINE, RATED_TURN, 10.) );
			CHECK( ShouldFlipAndBurn(velocity, Unit(90.), FORWARD, BASELINE, RATED_TURN, 10.) );
		}
		THEN( "it isn't when the retro thrusters are as strong as the main engine" ) {
			CHECK_FALSE( ShouldFlipAndBurn(velocity, Unit(0.), FORWARD, FORWARD, RATED_TURN, 10.) );
		}
		THEN( "it isn't when one frame of main thrust would stop the ship anyway" ) {
			CHECK_FALSE( ShouldFlipAndBurn(Unit(0.) * FORWARD, Unit(0.), FORWARD, BASELINE, RATED_TURN, 10.) );
		}
		THEN( "it isn't when the ship can't turn or has no main engine" ) {
			CHECK_FALSE( ShouldFlipAndBurn(velocity, Unit(0.), FORWARD, BASELINE, 0., 10.) );
			CHECK_FALSE( ShouldFlipAndBurn(velocity, Unit(0.), 0., BASELINE, RATED_TURN, 10.) );
		}
	}
	GIVEN( "a fast ship pressing STOP with assist on" ) {
		auto framesToStop = [](bool flip) -> int
		{
			Sim sim;
			sim.velocity = Unit(0.) * 20.;
			for(int i = 1; i <= 2000; ++i)
			{
				double turn = 0.;
				if(flip && ShouldFlipAndBurn(sim.velocity, Unit(sim.angle), FORWARD, BASELINE, RATED_TURN, 10.))
				{
					double delta = DeltaTo(sim.angle, -sim.velocity);
					turn = TurnToward(true, delta, sim.angularVelocity, RATED_TURN, ANGULAR_ACCEL, std::fabs(delta) < 1.);
				}
				sim.Step(Stop(), turn);
				if(!sim.velocity)
					return i;
			}
			return 0;
		};
		int flipFrames = framesToStop(true);
		int retroFrames = framesToStop(false);
		THEN( "both come to an exact stop" ) {
			CHECK( flipFrames > 0 );
			CHECK( retroFrames > 0 );
		}
		THEN( "flipping and burning stops it much sooner" ) {
			CHECK( flipFrames < retroFrames / 2 );
		}
	}
}

SCENARIO( "Emergency station-keeping", "[NewtonianFlight][stationKeeping]" ) {
	GIVEN( "a disabled ship" ) {
		THEN( "it comes nearly to rest in about 8 seconds" ) {
			CHECK( std::pow(STATION_KEEPING_DECAY, 8. * 60.) < .01 );
			CHECK( std::pow(STATION_KEEPING_DECAY, 4. * 60.) > .05 );
		}
	}
}
// #endregion unit tests



} // test namespace
