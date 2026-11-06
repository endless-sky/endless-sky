/* NewtonianFlight.cpp
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

#include "NewtonianFlight.h"

#include "../pi.h"

#include <algorithm>
#include <cmath>

using namespace std;

namespace {
	// The direction to the right of a facing.
	Point RightOf(const Point &facing)
	{
		return Point(-facing.Y(), facing.X());
	}
}



double NewtonianFlight::SoftCapFactor(double speed, double rated, double softCap)
{
	if(speed <= rated || rated <= 0. || softCap <= 1.)
		return 1.;
	// Choose the width so that the factor equals SOFT_CAP_RESIDUAL at the soft cap.
	static const double SCALE = sqrt(1. / SOFT_CAP_RESIDUAL - 1.);
	double excess = (speed - rated) / ((softCap - 1.) * rated) * SCALE;
	return 1. / (1. + excess * excess);
}



NewtonianFlight::RotationPlan NewtonianFlight::PlanRotation(bool assist, double turnInput,
	double angularVelocity, double ratedTurn, double angularAccel, double softCap)
{
	RotationPlan plan;
	turnInput = clamp(turnInput, -1., 1.);
	if(assist)
		plan.deltaOmega = clamp(turnInput * ratedTurn - angularVelocity, -angularAccel, angularAccel);
	else if(turnInput)
	{
		// As with thrust, the full torque requested is paid for even when it is capped.
		plan.deltaOmega = turnInput * angularAccel;
		if(plan.deltaOmega * angularVelocity > 0.)
			plan.capFactor = SoftCapFactor(fabs(angularVelocity), ratedTurn, softCap);
	}
	return plan;
}



Point NewtonianFlight::Apply(const Point &velocity, const Point &facing, double dvForward, double dvLateral,
	double softCapFactor)
{
	return velocity + (facing * dvForward + RightOf(facing) * dvLateral) * softCapFactor;
}



NewtonianFlight::TranslationPlan NewtonianFlight::PlanTranslation(bool assist, const Point &velocity,
	const Point &facing, double ratedSpeed, const Thrusters &thrusters, const Input &input, double softCap)
{
	TranslationPlan plan;
	const Point right = RightOf(facing);
	const double speed = velocity.Length();
	const double forward = thrusters.forward + thrusters.afterburner;
	if(assist)
	{
		// A5.1 (revised 2026-11-06): assist flies like the old drag model, but
		// the "drag" is the ship's own weak manoeuvring thrusters. Thrust bends
		// the velocity gradually (the ship slides when it turns), speed is
		// capped at the rated speed, and with no thrust input the computer fires
		// the manoeuvring thrusters against the motion until the ship stops.
		Point target = velocity;
		if(input.stop)
		{
			// STOP brakes the whole velocity to zero, keeping the facing.
			target = Point();
			plan.braking = (speed > 0.);
		}
		else
		{
			bool hasInput = input.thrust || input.strafe || input.afterburner;
			if(input.thrust > 0.)
				target += facing * forward;
			else if(input.thrust < 0.)
				target -= facing * thrusters.retro;
			else if(input.afterburner)
				target += facing * thrusters.afterburner;
			target += right * (input.strafe * thrusters.lateral);
			if(!hasInput && speed > 0.)
			{
				// Coast to a stop on the weak manoeuvring thrusters.
				double brake = min(speed, min(thrusters.retro, thrusters.lateral));
				target -= velocity * (brake / speed);
			}
			double targetSpeed = target.Length();
			if(speed > ratedSpeed + .01)
			{
				// A5.5: above rated speed, brake back down to it.
				plan.braking = true;
				if(targetSpeed > ratedSpeed)
					target *= ratedSpeed / targetSpeed;
			}
			else if(targetSpeed > ratedSpeed)
				// Thrust at the rated speed bends the velocity without adding speed.
				target *= ratedSpeed / targetSpeed;
		}
		Point dv = target - velocity;
		plan.dvForward = dv.Dot(facing);
		plan.dvLateral = dv.Dot(right);
	}
	else
	{
		// Raw thrusters. STOP does nothing with assist off (A5.1).
		if(input.thrust > 0.)
			plan.dvForward = forward;
		else if(input.thrust < 0.)
			plan.dvForward = -thrusters.retro;
		else if(input.afterburner)
			plan.dvForward = thrusters.afterburner;
		plan.dvLateral = input.strafe * thrusters.lateral;
		// Soft cap (A1.2): pushes that add speed lose effect above rated speed. The
		// thrusters still burn (and are paid for) at the full throttle requested.
		Point push = facing * plan.dvForward + right * plan.dvLateral;
		if(push && (velocity + push).Length() > speed)
			plan.softCapFactor = SoftCapFactor(speed, ratedSpeed, softCap);
	}

	// Limit each thruster group to what it can deliver.
	double wantForward = plan.dvForward;
	double wantLateral = plan.dvLateral;
	plan.dvForward = clamp(plan.dvForward, -thrusters.retro, forward);
	plan.dvLateral = clamp(plan.dvLateral, -thrusters.lateral, thrusters.lateral);
	plan.saturated = fabs(wantForward - plan.dvForward) > 1e-9 || fabs(wantLateral - plan.dvLateral) > 1e-9;
	return plan;
}



double NewtonianFlight::TurnToward(bool assist, double delta, double angularVelocity, double ratedTurn,
	double angularAccel, bool close)
{
	if(ratedTurn <= 0. || angularAccel <= 0.)
		return 0.;
	if(close && fabs(delta) < ratedTurn && fabs(angularVelocity) <= angularAccel)
		return assist ? 0. : clamp(-angularVelocity / angularAccel, -1., 1.);

	// The fastest spin rate from which the ship can still stop on the target.
	double desired = copysign(min(ratedTurn, sqrt(2. * angularAccel * fabs(delta))), delta);
	if(fabs(desired) > fabs(delta))
		desired = delta;
	if(assist)
		return desired / ratedTurn;
	return clamp((desired - angularVelocity) / angularAccel, -1., 1.);
}



bool NewtonianFlight::ShouldFlipAndBurn(const Point &velocity, const Point &facing, double mainAccel,
	double retroAccel, double ratedTurn, double spinUpFrames)
{
	double speed = velocity.Length();
	if(mainAccel <= 0. || ratedTurn <= 0. || speed <= mainAccel)
		return false;
	double retroTime = retroAccel > 0. ? speed / retroAccel : 1e9;
	double flipAngle = acos(clamp(-velocity.Unit().Dot(facing), -1., 1.)) * TO_DEG;
	double flipTime = flipAngle / ratedTurn + spinUpFrames + speed / mainAccel;
	return flipTime < retroTime;
}
