/* global.h
Copyright (c) 2019 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#ifndef GLOBAL_H_
#define GLOBAL_H_



const int FRAME_RATE = 60;

const double DEG_0 = 0;
const double DEG_90 = 90;
const double DEG_180 = DEG_90 * 2;
const double DEG_360 = DEG_90 * 4;

// Constants to replace M_PI (which is not available on all operating systems).
const double PI = 3.14159265358979323846;
const double TO_RAD = PI / DEG_180;
const double TO_DEG = DEG_180 / PI;

const double PERCENT = 100;



#endif /* GLOBAL_H_ */
