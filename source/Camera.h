#include "Angle.h"
#include "Point.h"

#include <vector>

#ifndef CAMERA_H

#define CAMERA_H

class Camera {
public:
	static Point Offset();
	static Point VelocityOffset();

	static Point Position();
	static Point Velocity();

	static Point CenterPos();
	static Point CenterVel();

	static void SetPosition(Point newPosition);
	static void SetVelocity(Point newVelocity);
	static void SetOffset(Point newOffset);
	static void SetVelocityOffset(Point newVelocity);

	static void Update(Point flagshipCenter, Point flagshipVelocity);
	static void SetCenter(Point newCenter, Point newVelocity);
};

#endif
