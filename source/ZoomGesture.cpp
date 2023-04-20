/* ZoomGesture.cpp
Copyright (c) 2023 by Rian Shelley

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/
#include "ZoomGesture.h"

#include <cmath>

static constexpr float FINGER_EPSILON = 20;

/**
 * Track fingerdown events to check if they are zoom gestures
 * @param pos The finger position in pixel coordinates
 * @param fid The finger id
 * @return false for the first finger, true for the second.
 */
bool ZoomGesture::FingerDown(Point pos, int fid)
{
	if(m_finger1_id == fid)
	{
		// shouldn't happen. reset state
		m_finger_start = m_finger1 = pos;
		m_first_canceled = false;
		m_finger1_was_first = m_finger2_id == -1;
		return m_finger2_id != -1;
	}
	else if(m_finger2_id == fid)
	{
		// shouldn't happen. reset state
		m_finger_start = m_finger2 = pos;
		m_first_canceled = false;
		return m_finger1_id != -1;
	}
	else if(m_finger1_id == -1)
	{
		m_finger1_id = fid;
		m_finger_start = m_finger1 = pos;
		m_first_canceled = false;
		m_finger1_was_first = m_finger2_id == -1;
		return m_finger2_id != -1;
	}
	else if(m_finger2_id == -1)
	{
		m_finger2_id = fid;
		m_finger_start = m_finger2 = pos;
		m_first_canceled = false;
		m_finger1_was_first = m_finger1_id != -1;
		return m_finger1_id != -1;
	}
	// else both fingers are already set. This is not part of a pinch gesture.

	return false;
}



/**
 * Track fingermove events to check if they are zoom gestures
 * @param pos The finger position in pixel coordinates
 * @param fid The finger id
 * @return true if we are positive this is a zoom gesture
 */
bool ZoomGesture::FingerMove(Point pos, int fid)
{
	if(fid != m_finger1_id && fid != m_finger2_id)
		return false;
	
	// If we are only tracking one finger, then make sure it doesn't move too
	// far
	if(m_finger1_id == -1 || m_finger2_id == -1)
	{
		if(m_finger_start.DistanceSquared(pos) > FINGER_EPSILON * FINGER_EPSILON)
		{
			// The finger moved too far. reset it.
			(m_finger1_id == -1 ? m_finger2_id : m_finger1_id) = -1;
		}
		else
		{
			(m_finger1_id == fid ? m_finger1 : m_finger2) = pos;
		}
		return false;
	}
	else
	{
		// we are tracking both fingers. This is a zoom event.
		// compare the previous distance to the new distance. Defer the square
		// root operation until after the divide, so that we only have to do it
		// once.
		// previous distance
		float d1s = m_finger1.DistanceSquared(m_finger2);
		// current distance from pos to the "other" finger
		const Point& other = m_finger1_id == fid ? m_finger2 : m_finger1;
		float d2s = pos.DistanceSquared(other);
		m_zoom = sqrt(d2s/d1s);

		// how far did we drag as well as zoom?
		Point oldCenter = (m_finger1 + m_finger2) / 2;
		Point newCenter = (pos + other) / 2;
		m_delta = newCenter - oldCenter;

		(m_finger1_id == fid ? m_finger1 : m_finger2) = pos;
		return true;
	}
	return false;
}



/**
 * Track fingerup events to check if they are zoom gestures
 * @param pos The finger position in pixel coordinates
 * @param fid The finger id
 * @return true if we are positive this is a zoom gesture
 */
bool ZoomGesture::FingerUp(Point pos, int fid)
{
	// The fingerdown event returns false for the first finger, but true for the
	// second. Duplicate that logic here.
	if(m_finger2_id == fid)
	{
		m_finger2_id = -1;
		return m_first_canceled || m_finger1_was_first;
	}
	else if(m_finger1_id == fid)
	{
		m_finger1_id = -1;
		return m_first_canceled || !m_finger1_was_first;
	}
	return false;
}