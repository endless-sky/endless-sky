/* Gesture.h
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
#include "Gesture.h"

#include <SDL2/SDL.h>

#include <array>
#include <cmath>
#include <cassert>
#include <map>

namespace
{
   // how many units wide our gesture should be, before we bother checking
   // it. (so that we don't analyze every screen tap as a gesture)
   const float MIN_SIZE = 50;
   // The protractor variant should only need 16 points to work optimally
   const size_t VSIZE = 16;
   typedef std::array<Gesture::Point, VSIZE> GVector;
   

   static const float MATCH_THRESHOLD = .90; // about 20 degree variance.

   float Distance(Gesture::Point a, Gesture::Point b)
   {
      float dx = a.x - b.x;
      float dy = a.y - b.y;
      return sqrt(dx * dx + dy * dy);
   }

   // Resample the vector into VSIZE equidistant points.
   GVector Resample(bool orientation_sensitive, const std::vector<Gesture::Point>& v)
   {
      assert(!v.empty());

      float path_length = 0;
      for(size_t i = 1; i < v.size(); ++i)
      {
         path_length += Distance(v[i-1], v[i]);
      }
      const float I = path_length / (VSIZE-1);


      GVector ret;
      size_t ret_idx = 0;
      Gesture::Point prev = ret[ret_idx++] = v.front();
      float distance_from_ret = 0;
      for(size_t i = 1; i < v.size(); ++i)
      {
         float distance_from_prev = Distance(prev, v[i]);
         while(distance_from_prev + distance_from_ret >= I)
         {
            float f = (I - distance_from_ret) / distance_from_prev;
            Gesture::Point next{
               (1 - f) * prev.x + f * v[i].x,
               (1 - f) * prev.y + f * v[i].y
            };
            assert(ret_idx < VSIZE);
            ret[ret_idx++] = next;
            prev = next;
            distance_from_ret = 0;
            distance_from_prev = Distance(prev, v[i]);
         }
         distance_from_ret += distance_from_prev;
         prev = v[i];
      }

      // explicitly set the last entry, in case of rounding errors
      ret[VSIZE-1] = v.back();

      // Reposition around its centroid
      Gesture::Point center{};
      for(auto& p: ret)
      {
         center.x += p.x;
         center.y += p.y;
      }
      center.x /= VSIZE;
      center.y /= VSIZE;
      for(auto& p: ret)
      {
         p.x -= center.x;
         p.y -= center.y;
      }

      // indicative angle is the angle from the X axis to the first point
      float indicative_angle = atan2(ret[0].y, ret[0].x);
      float delta = 0;
      if(orientation_sensitive)
      {
         // Since we care about the angle, snap to the nearest PI/4
         float base_orientation = (M_PI/4) * floor((indicative_angle + M_PI/8) / (M_PI / 4));
         delta = base_orientation - indicative_angle;
      }
      else
      {
         // Rotate so that the indicative angle will be 0 degrees
         delta = -indicative_angle;
      }

      float cd = cos(delta);
      float sd = sin(delta);
      
      for(auto& p: ret)
      {
         p = Gesture::Point{
            p.x * cd - p.y * sd,
            p.y * cd + p.x * sd
         };
      }


      // We are going to treat the VSIZE point array as a 32 dimensional vector,
      // and then run our search through the vector space to determine
      // similarity. We need to normalize this vector to simplify the angle
      // computation, and also to scale the compared gestures to similar sizes.
      float magnitude = 0;
      for(auto& p: ret)
         magnitude += p.x * p.x + p.y * p.y;
      magnitude = sqrt(magnitude);
      for(auto& p: ret)
      {
         p.x /= magnitude;
         p.y /= magnitude;
      }

      return ret;
   }

   float GestureDistanceOrientationSensitive(const GVector& v, const GVector& t)
   {
      // Slow variant of GestureDistance that tries to keep the gesture
      // orientation correct by limiting the rotation. This just brute force
      // rotates and computes the cosine distance, and keeps the best match.
      float best_cosine = -1;
      for (float delta = -M_PI/16; delta <= M_PI/16; delta += M_PI/8/20)
      {
         float cd = cos(delta);
         float sd = sin(delta);
         GVector vp;
         for(size_t i = 0; i < v.size(); ++i)
         {
            vp[i].x = v[i].x * cd - v[i].y * sd;
            vp[i].y = v[i].y * cd + v[i].x * sd;
         }

         // dot product of these pre-normalized vectors. remember that here, we
         // have conceptually stopped viewing v as a set of coordinates, and
         // instead are viewing both x and y as a vector component in a VSIZE*2
         // dimensional space.
         float ca = 0;
         for(size_t i = 0; i < v.size(); ++i)
         {
            ca += v[i].x * t[i].x + v[i].y * t[i].y;
         }
         // TODO: if we assume no local minima, then just stop looking if the
         //       solutions are getting worse.
         if(ca > best_cosine)
            best_cosine = ca;
      }

      return best_cosine;
   }
#if 0
   // Code based on original algorithm, less reliable for oriented gestures.
   float GestureDistance(const GVector& v1, const GVector& v2)
   {
      float a = 0; // dot product
      float b = 0; // equation 7, determinant of every coordinate pair
      for (size_t i = 0; i + 1 < v1.size(); ++i)
      {
         a += v1[i].x * v2[i].x + v1[i].y * v2[i].y;
         b += v1[i].x * v2[i].y - v1[i].y * v2[i].x;
      }
      // Compute the angle we need to purturb the vector by to get the optimal
      // solution... assuming that our alignment may be slightly off. The
      // computation of a is just a dot product to retrieve the cosine, but b
      // b plus the atan is the result of solving a differential equation to
      // find local minima in the matching of the original trace by varying
      // the rotation angle. I won't pretend to understand how they solved
      // that, but this step comes with a problem. For some shapes (such as
      // a simple line), this is capable of arbitrarily rotating the shape a
      // full 360 degrees to match, which defeats the purpose of having
      // invariant angles locked to a specific orientation. You can see this by
      // putting in an invariant top-down line, and it still matches it no
      // matter how you draw it.
      float angle = atan2(b, a);

      // rotate the computed vector by the perturbation angle, which should
      // yield the closest match.
      return a * cos(angle) + b * sin(angle);
   }

   void Dump(const GVector& v)
   {
      SDL_Log("Dump: \n"
              "(%f,%f) (%f,%f) (%f,%f) (%f,%f) (%f,%f) (%f,%f) (%f,%f) (%f,%f)\n"
              "(%f,%f) (%f,%f) (%f,%f) (%f,%f) (%f,%f) (%f,%f) (%f,%f) (%f,%f)\n",
              v[0].x,v[0].y,v[1].x,v[1].y,v[2].x,v[2].y,v[3].x,v[3].y,
              v[4].x,v[4].y,v[5].x,v[5].y,v[6].x,v[6].y,v[7].x,v[7].y,
              v[8].x,v[8].y,v[9].x,v[9].y,v[10].x,v[10].y,v[11].x,v[11].y,
              v[12].x,v[12].y,v[13].x,v[13].y,v[14].x,v[14].y,v[15].x,v[15].y);
   }
#endif

   // Here are where the strokes are defined
   struct {
      Gesture::GestureEnum name;
      bool orientation_sensitive;
      GVector points;
   } UNISTROKES[] = {
      // X is rotationally invariant, do a left handed and a right handed version
      {Gesture::X, false, Resample(false, {{0, 0}, {1, 1}, {0, 1}, {1, 0}})},
      {Gesture::X, false, Resample(false, {{1, 0}, {0, 1}, {1, 1}, {0, 0}})},

      // carets are directional. provide a left and right handed version plus a
      // sharp/shallow angle for each one
      {Gesture::CARET_RIGHT, true, Resample(true, {{0, 1}, {1, 0}, {0, -1}})},
      {Gesture::CARET_RIGHT, true, Resample(true, {{0, -1}, {1, 0}, {0, 1}})},
      {Gesture::CARET_RIGHT, true, Resample(true, {{-1, 1}, {1, 0}, {-1, -1}})},
      {Gesture::CARET_RIGHT, true, Resample(true, {{-1, -1}, {1, 0}, {-1, 1}})},
      {Gesture::CARET_LEFT, true, Resample(true, {{0, 1}, {-1, 0}, {0, -1}})},
      {Gesture::CARET_LEFT, true, Resample(true, {{0, -1}, {-1, 0}, {0, 1}})},
      {Gesture::CARET_LEFT, true, Resample(true, {{-1, 1}, {-1, 0}, {-1, -1}})},
      {Gesture::CARET_LEFT, true, Resample(true, {{-1, -1}, {-1, 0}, {-1, 1}})},
      {Gesture::CARET_DOWN, true, Resample(true, {{-1, 0}, {0, 1}, {1, 0}})}, // down is positive
      {Gesture::CARET_DOWN, true, Resample(true, {{1, 0}, {0, 1}, {-1, 0}})},
      {Gesture::CARET_DOWN, true, Resample(true, {{-1, -1}, {0, 1}, {1, -1}})},
      {Gesture::CARET_DOWN, true, Resample(true, {{1, -1}, {0, 1}, {-1, -1}})},
      {Gesture::CARET_UP, true, Resample(true, {{-1, 0}, {0, -1}, {1, 0}})},  // up is negative
      {Gesture::CARET_UP, true, Resample(true, {{1, 0}, {0, -1}, {-1, 0}})},
      {Gesture::CARET_UP, true, Resample(true, {{-1, -1}, {0, -1}, {1, -1}})},
      {Gesture::CARET_UP, true, Resample(true, {{1, -1}, {0, -1}, {-1, -1}})},

      // Circle is rotationally invariant. Do a left and right handed version plus
      // some slightly overlapping versions
      {Gesture::CIRCLE, false, Resample(false, {
         {1.000000, 0.000000}, {0.923880, 0.382683}, {0.707107, 0.707107}, {0.382683, 0.923880},
         {0.000000, 1.000000}, {-0.382683, 0.923880}, {-0.707107, 0.707107}, {-0.923880, 0.382683},
         {-1.000000, 0.000000}, {-0.923880, -0.382683}, {-0.707107, -0.707107}, {-0.382683, -0.923880},
         {-0.000000, -1.000000}, {0.382683, -0.923880}, {0.707107, -0.707107}, {0.923880, -0.382683}})},
      {Gesture::CIRCLE, false, Resample(false, {
         {1.000000, 0.000000}, {0.923880, 0.382683}, {0.707107, 0.707107}, {0.382683, 0.923880},
         {0.000000, 1.000000}, {-0.382683, 0.923880}, {-0.707107, 0.707107}, {-0.923880, 0.382683},
         {-1.000000, 0.000000}, {-0.923880, -0.382683}, {-0.707107, -0.707107}, {-0.382683, -0.923880},
         {-0.000000, -1.000000}, {0.382683, -0.923880}, {0.707107, -0.707107}, {0.923880, -0.382683},
         {1.000000, 0.000000}})},
      {Gesture::CIRCLE, false, Resample(false, {
         {1.000000, 0.000000}, {0.923880, 0.382683}, {0.707107, 0.707107}, {0.382683, 0.923880},
         {0.000000, 1.000000}, {-0.382683, 0.923880}, {-0.707107, 0.707107}, {-0.923880, 0.382683},
         {-1.000000, 0.000000}, {-0.923880, -0.382683}, {-0.707107, -0.707107}, {-0.382683, -0.923880},
         {-0.000000, -1.000000}, {0.382683, -0.923880}, {0.707107, -0.707107}, {0.923880, -0.382683},
         {1.000000, 0.000000}, {0.923880, 0.382683}})},
      {Gesture::CIRCLE, false, Resample(false, {
         {1.000000, 0.000000}, {0.923880, 0.382683}, {0.707107, 0.707107}, {0.382683, 0.923880},
         {0.000000, 1.000000}, {-0.382683, 0.923880}, {-0.707107, 0.707107}, {-0.923880, 0.382683},
         {-1.000000, 0.000000}, {-0.923880, -0.382683}, {-0.707107, -0.707107}, {-0.382683, -0.923880},
         {-0.000000, -1.000000}, {0.382683, -0.923880}, {0.707107, -0.707107}, {0.923880, -0.382683},
         {1.000000, 0.000000}, {0.923880, 0.382683}, {0.707107, 0.707107}})},
      {Gesture::CIRCLE, false, Resample(false, {
         {1.000000, 0.000000}, {0.923880, 0.382683}, {0.707107, 0.707107}, {0.382683, 0.923880},
         {0.000000, 1.000000}, {-0.382683, 0.923880}, {-0.707107, 0.707107}, {-0.923880, 0.382683},
         {-1.000000, 0.000000}, {-0.923880, -0.382683}, {-0.707107, -0.707107}, {-0.382683, -0.923880},
         {-0.000000, -1.000000}, {0.382683, -0.923880}, {0.707107, -0.707107}, {0.923880, -0.382683},
         {1.000000, 0.000000}, {0.923880, 0.382683}, {0.707107, 0.707107}, {0.382683, 0.923880}})},
      {Gesture::CIRCLE, false, Resample(false, {
         {1.000000, 0.000000},{0.923880, -0.382683},{0.707107, -0.707107},{0.382683, -0.923880},
         {0.000000, -1.000000},{-0.382683, -0.923880},{-0.707107, -0.707107},{-0.923880, -0.382683},
         {-1.000000, -0.000000},{-0.923880, 0.382683},{-0.707107, 0.707107},{-0.382683, 0.923880},
         {-0.000000, 1.000000},{0.382683, 0.923880},{0.707107, 0.707107},{0.923880, 0.382683}})},
      {Gesture::CIRCLE, false, Resample(false, {
         {1.000000, 0.000000},{0.923880, -0.382683},{0.707107, -0.707107},{0.382683, -0.923880},
         {0.000000, -1.000000},{-0.382683, -0.923880},{-0.707107, -0.707107},{-0.923880, -0.382683},
         {-1.000000, -0.000000},{-0.923880, 0.382683},{-0.707107, 0.707107},{-0.382683, 0.923880},
         {-0.000000, 1.000000},{0.382683, 0.923880},{0.707107, 0.707107},{0.923880, 0.382683},
         {1.000000, 0.000000}})},
      {Gesture::CIRCLE, false, Resample(false, {
         {1.000000, 0.000000},{0.923880, -0.382683},{0.707107, -0.707107},{0.382683, -0.923880},
         {0.000000, -1.000000},{-0.382683, -0.923880},{-0.707107, -0.707107},{-0.923880, -0.382683},
         {-1.000000, -0.000000},{-0.923880, 0.382683},{-0.707107, 0.707107},{-0.382683, 0.923880},
         {-0.000000, 1.000000},{0.382683, 0.923880},{0.707107, 0.707107},{0.923880, 0.382683},
         {1.000000, 0.000000},{0.923880, -0.382683}})},
      {Gesture::CIRCLE, false, Resample(false, {
         {1.000000, 0.000000},{0.923880, -0.382683},{0.707107, -0.707107},{0.382683, -0.923880},
         {0.000000, -1.000000},{-0.382683, -0.923880},{-0.707107, -0.707107},{-0.923880, -0.382683},
         {-1.000000, -0.000000},{-0.923880, 0.382683},{-0.707107, 0.707107},{-0.382683, 0.923880},
         {-0.000000, 1.000000},{0.382683, 0.923880},{0.707107, 0.707107},{0.923880, 0.382683},
         {1.000000, 0.000000},{0.923880, -0.382683},{0.707107, -0.707107}})},
      {Gesture::CIRCLE, false, Resample(false, {
         {1.000000, 0.000000},{0.923880, -0.382683},{0.707107, -0.707107},{0.382683, -0.923880},
         {0.000000, -1.000000},{-0.382683, -0.923880},{-0.707107, -0.707107},{-0.923880, -0.382683},
         {-1.000000, -0.000000},{-0.923880, 0.382683},{-0.707107, 0.707107},{-0.382683, 0.923880},
         {-0.000000, 1.000000},{0.382683, 0.923880},{0.707107, 0.707107},{0.923880, 0.382683},
         {1.000000, 0.000000},{0.923880, -0.382683},{0.707107, -0.707107},{0.382683, -0.923880}})},
      
   };

   const std::map<Gesture::GestureEnum, std::string> DESCRIPTIONS = {
      {Gesture::X, "Draw an X"}, 
      {Gesture::CIRCLE, "Draw a circle"},
      {Gesture::CARET_UP, "Draw a ^"},
      {Gesture::CARET_LEFT, "Draw a <"},
      {Gesture::CARET_RIGHT, "Draw a >"},
      {Gesture::CARET_DOWN, "Draw a V"}
   };
   static const size_t NUM_UNISTROKES = sizeof(UNISTROKES)/sizeof(*UNISTROKES);
}



void Gesture::Start(float x, float y, int finger_id)
{
   if(finger_id == 0)
   {
      m_tick_start = SDL_GetTicks();
      m_path.clear();
      m_path.push_back(Point{x, y});
      m_ymin = m_ymax = y;
      m_xmin = m_xmax = x;
      m_valid = true;
   }
   else if(finger_id == 1)
   {
      if(!m_path.empty())
      {
         m_finger1.first = m_finger1.second = m_path.back();
         m_path.clear(); // don't track the path, two finger events are zooms
         m_finger2.first = m_finger2.second = Point{x, y};
         m_valid = true;
      }
   }
   else
   {
      // we don't support more than one or two finger gestures.
      m_valid = false;
   }
}



Gesture::GestureEnum Gesture::Add(float x, float y, int finger_id)
{
   if(!m_valid)
      return NONE;
   
   Point prev_finger1 = m_finger1.second;
   Point prev_finger2 = m_finger2.second;
   if(finger_id == 0 && !m_path.empty())
   {
      m_finger1.second = Point{x, y};
      if(!m_path.empty())
      {
         if(SDL_GetTicks() - m_tick_start > 2000)
            m_valid = false;
         m_path.push_back({x, y});
         if(x < m_xmin) m_xmin = x;
         else if(x > m_xmax) m_xmax = x;
         if(y < m_ymin) m_ymin = y;
         else if(y > m_ymax) m_ymax = y;
      }
   }
   else if(finger_id == 1)
      m_finger2.second = Point{x, y};

   if(m_path.empty())
   {
      SDL_Event event{};
      event.type = EventID();
      event.user.code = ZOOM;
      // The data1 and data2 pointer fields are almost unusable. They are
      // different sizes on 32 vs 64 bit architectures, and I can't put a
      // reference to static data there due to race conditions, and I can't
      // allocate data on the fly because I'm not guaranteed that the target
      // of the event actually cares about it. For now, memcpy the 32 bit
      // floats in there that I want.
      float total_zoom = ZoomAmount();
      float d1 = Distance(prev_finger1, prev_finger2);
      float d2 = Distance(m_finger1.second, m_finger2.second);
      float incremental_zoom = d2/d1;
      memcpy(&event.user.data1, &total_zoom, sizeof(total_zoom));
      memcpy(&event.user.data2, &incremental_zoom, sizeof(incremental_zoom));

      SDL_PushEvent(&event);
      return ZOOM;
   }
   return NONE;
}



Gesture::GestureEnum Gesture::End()
{
   if(!m_valid)
      return NONE;
   m_valid = false;

   // Don't analyze really small gestures, as they are probably just plain taps
   if(MIN_SIZE > m_ymax - m_ymin && MIN_SIZE > m_xmax - m_xmin)
      return NONE;

   // using the protractor variation of the dollar algorithm, as described
   // here: http://depts.washington.edu/acelab/proj/dollar/index.html
   // Step 1. Resample path to VSIZE equidistant points.
   // Step 2. Treat the points as a (math) vector, and normalize.
   // Step 3. Compute the angular distance from each template.
   

   size_t best_idx = NUM_UNISTROKES;
   // distance is the cosine of the angle between two path vectors in
   // VSIZE-dimensional space. It ranges from -1 (worst match) to 1 (perfect
   // match), with average matches around .97
   float best_distance = -1; // cos(-PI), worst match possible
   GVector v_oriented = Resample(true, m_path);
   GVector v_rotated = Resample(false, m_path);
   for(size_t i = 0; i < NUM_UNISTROKES; ++i)
   {
      float cos_distance = GestureDistanceOrientationSensitive(UNISTROKES[i].points,
         UNISTROKES[i].orientation_sensitive ? v_oriented : v_rotated);
      if(cos_distance > best_distance)
      {
         best_distance = cos_distance;
         best_idx = i;
      }
   }


   if(best_idx >= NUM_UNISTROKES || best_distance < MATCH_THRESHOLD)
      return NONE;

   SDL_Event event{};
	event.type = EventID();
   event.user.code = UNISTROKES[best_idx].name;
	SDL_PushEvent(&event);
   return UNISTROKES[best_idx].name;
}



float Gesture::ZoomAmount() const
{
   float d1 = Distance(m_finger1.first, m_finger2.first);
   float d2 = Distance(m_finger1.second, m_finger2.second);
   return d2/d1;
}



uint32_t Gesture::EventID()
{
   static uint32_t event_id = SDL_RegisterEvents(1);
   return event_id;
}



const std::string& Gesture::Description(GestureEnum gesture)
{
   static std::string EMPTY;
   auto it = DESCRIPTIONS.find(gesture);
   return it != DESCRIPTIONS.end() ? it->second : EMPTY;
}