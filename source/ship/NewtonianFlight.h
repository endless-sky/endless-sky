/* NewtonianFlight.h
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

#pragma once

#include "../Point.h"



// The pure flight maths of the Newtonian flight model (plans/flight-simulation-A.md),
// kept free of ship state, outfits and resource costs so that it can be unit tested.
// Ship::DoNewtonianMovement plans each frame's burns with these functions, then pays
// for them, and the AI uses the same controllers to steer.
namespace NewtonianFlight {
	// At the soft cap (a multiple of the rated value) a push that adds speed is only
	// this effective (A1.2).
	constexpr double SOFT_CAP_RESIDUAL = .1;
	// Disabled or powerless ships keep this fraction of their velocity each frame
	// ("emergency station-keeping", A5.7): they come to rest in about 8 seconds.
	constexpr double STATION_KEEPING_DECAY = .99;

	// How effective a push is that would take a speed (or spin rate) further above
	// its rated value. The falloff is asymptotic: there is no hard ceiling.
	double SoftCapFactor(double speed, double rated, double softCap);


	// One frame of rotation (A2).
	struct RotationPlan {
		// The change in angular velocity the turning thrusters are asked for.
		double deltaOmega = 0.;
		// The soft cap applied to that change when it is delivered.
		double capFactor = 1.;
	};
	// With assist on, the turn input means "turn at this rate" and the computer
	// supplies the torque; with it off, the input is raw torque.
	RotationPlan PlanRotation(bool assist, double turnInput, double angularVelocity,
		double ratedTurn, double angularAccel, double softCap);


	// The delta-v each thruster group can deliver this frame.
	struct Thrusters {
		double forward = 0.;
		// Extra forward delta-v from the afterburner; zero unless it is firing.
		double afterburner = 0.;
		double retro = 0.;
		double lateral = 0.;
	};
	struct Input {
		// 1 forward, -1 back, 0 none.
		double thrust = 0.;
		// 1 right, -1 left, 0 none.
		double strafe = 0.;
		bool afterburner = false;
		bool stop = false;
	};
	// One frame of translation (A1, A3, A5.1, A5.5), before costs.
	struct TranslationPlan {
		// Delta-v along the facing and to its right, within the thrusters' limits.
		double dvForward = 0.;
		double dvLateral = 0.;
		// The soft cap applied when the delta-v is delivered (assist off only).
		double softCapFactor = 1.;
		// The flight computer is braking (STOP, or above the rated speed).
		bool braking = false;
		// The thrusters could not deliver all the delta-v that was wanted.
		bool saturated = false;
	};
	// The ship's velocity after delivering a translation plan of which the given
	// delta-v was affordable.
	Point Apply(const Point &velocity, const Point &facing, double dvForward, double dvLateral,
		double softCapFactor);
	TranslationPlan PlanTranslation(bool assist, const Point &velocity, const Point &facing,
		double ratedSpeed, const Thrusters &thrusters, const Input &input, double softCap);


	// The turn command that brings the ship's facing onto a target angle and stops
	// it there, counter-turning in time (A2). The delta is in degrees, positive to
	// the right. "Close" means the ship is already pointing closely enough.
	double TurnToward(bool assist, double delta, double angularVelocity, double ratedTurn,
		double angularAccel, bool close);

	// Whether turning to face retrograde and braking on the main engine stops the ship
	// sooner than braking on its retro thrusters while keeping its facing.
	bool ShouldFlipAndBurn(const Point &velocity, const Point &facing, double mainAccel,
		double retroAccel, double ratedTurn, double spinUpFrames);
}
