/*
-------------------------------------------------------------------------------
  About
-------------------------------------------------------------------------------

This header file is in the style of Sean Barret's stb nothings header libraries.
To implement the actual implementation you have to pound define GJK_IMPLEMENTATION _before_ including the header file.
i.e. 
#define GJK_IMPLEMENTATION 
#include "easy_gjk.h"

You can also pound define your own assert function with GJK_ASSERT
i.e.
#define GJK_ASSERT(statment) {if(statement) { exit(0); }}
 
This header file implementation provides two functions to use. These are:

bool gjk_objectsCollide(gjk_v2 *a, int aCount, gjk_v2 *b, int bCount);
Gjk_EPA_Info gjk_objectsCollide_withEPA(gjk_v2 *a, int aCount, gjk_v2 *b, int bCount);

1. Provide you with a yes/no answer if two polygons are colliding. Note being the polygon must be convex 
2. Provide you with the info from above, but also a penetration vector which you can offset the polygons accordingly. 
   By adding the pentration vector (normal*distance) to the first shapes position will resolve the collision. NOTE: The order of the shapes matters on what you add the pentration vector. 

-------------------------------------------------------------------------------
  License
-------------------------------------------------------------------------------
Easy_GJK is released under the following license:

MIT License

Copyright (c) 2018 Oliver Marsh

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:
The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

/*
TODO:
- Make support function for circles
- implement a stretch buffer for the simplex point count. Doens't have to be complex (or type safe? if we are the only ones using it.)
*/

#define gjk_arrayCount(array1) (sizeof(array1) / sizeof(array1[0]))

#ifndef GJK_ASSERT
#define GJK_ASSERT(statement) if(!(statement)) {printf("Something went wrong at line number: %d in %s\n", __LINE__, __FILE__); exit(0);}
#endif

typedef struct {
    float x, y;
} gjk_v2;

gjk_v2 gjk_V2(float x, float y) {
    gjk_v2 result = {};
    result.x = x;
    result.y = y;
    return result;
}

typedef struct {
    //Our simplex is no longer confined to size of three since we use it for the EPA routine as well. 
    gjk_v2 p[32]; //TODO(ollie): change this to using a stretchy buffer so we can have abritarily big shapes!!
    int count;
} Simplex;

typedef struct {
  bool collided;
  Simplex simplex;
} GjkInfo;

typedef struct {
  bool collided;
  gjk_v2 normal;
  float distance;

} Gjk_EPA_Info;

typedef struct {
  gjk_v2 normal;
  int index;
  float distance;
} EPA_Edge;

bool gjk_objectsCollide(gjk_v2 *a, int aCount, gjk_v2 *b, int bCount);
Gjk_EPA_Info gjk_objectsCollide_withEPA(gjk_v2 *a, int aCount, gjk_v2 *b, int bCount);

#ifdef GJK_IMPLEMENTATION 

float gjk_dot(gjk_v2 a, gjk_v2 b) {
    float result = a.x*b.x + a.y*b.y;
    return result;
}

gjk_v2 gjk_negate_v2(gjk_v2 a) {
    gjk_v2 result = gjk_V2(-a.x, -a.y);
    return result;
}

gjk_v2 gjk_normalize_2D(gjk_v2 a) {
  float length = sqrt(a.x*a.x + a.y*a.y);
  if(length == 0) {
    length = 1;
    printf("gjk length was equal to zero\n");
  }

  assert(length > 0);
  gjk_v2 result = gjk_V2(a.x/length, a.y/length);
  return result;

}

void gjk_addPointAt(Simplex *simplex, gjk_v2 p, int insertAt) {
    if(simplex->count >= gjk_arrayCount(simplex->p)) {
      //TODO: change this to using a stretchy buffer!
      GJK_ASSERT(!"we went over our limit");
    }
    //move all points foward by one to fit new point in
    for(int i = simplex->count - 1; i >= insertAt; --i) {
      simplex->p[i + 1] = simplex->p[i];
      GJK_ASSERT((i + 1) < gjk_arrayCount(simplex->p));
    }
    ///

    //////add the new point
    simplex->count++;
    simplex->p[insertAt] = p;
    ///
}

void gjk_addPoint(Simplex *simplex, gjk_v2 p) {
    if(simplex->count >= gjk_arrayCount(simplex->p)) {
      //TODO: change this to a stretchy buffer!
      GJK_ASSERT(!"we went over our limit");
    }
    simplex->p[simplex->count++] = p;
}

gjk_v2 gjk_perp2D(gjk_v2 a) {
    gjk_v2 result = {-a.y, a.x};
    return result;
}

gjk_v2 gjk_support(gjk_v2 d, gjk_v2 *a, int aCount, gjk_v2 *b, int bCount) {
    float maxDistA = 0; //set in loop
    gjk_v2 maxA = {};
    float maxDistB = 0; //set in loop
    gjk_v2 maxB = {};
    
    for(int i = 0; i < aCount; ++i) {
        gjk_v2 aPoint = a[i];
        float dist = gjk_dot(aPoint, d);
        if (dist > maxDistA || i == 0) { //or first element
            maxDistA = dist;
            maxA = aPoint;
        }
    }
    
    for(int j = 0; j < bCount; ++j) {
        gjk_v2 bPoint = b[j];
        float dist = gjk_dot(bPoint, gjk_V2(-d.x, -d.y));
        if (dist > maxDistB || j == 0) { //or first element
            maxDistB = dist;
            maxB = bPoint;
        }
    }

    gjk_v2 result = gjk_V2(maxA.x - maxB.x, maxA.y - maxB.y); //do our Minksowski difference (can decompose the function based on Caesy's video)
    return result;
}

bool doSimplex(Simplex *simplex, gjk_v2 *d) { //update both of these
  bool result = false;
  switch(simplex->count) {
      case 1: {
          GJK_ASSERT(!"Shouldn't ever be one point");
      } break;
      case 2: { //line
          gjk_v2 a = simplex->p[1];
          gjk_v2 b = simplex->p[0];
          gjk_v2 ao = gjk_V2(-a.x, -a.y);
          gjk_v2 ab = gjk_V2(b.x - a.x, b.y - a.y);
          if(gjk_dot(ab, ao)) {  //is inside the line boundary
              //don't have to update simplex
              gjk_v2 dir = gjk_perp2D(ab);
              if(gjk_dot(dir, ao) < 0) { //pointing the wrong direction 
                  dir = gjk_negate_v2(dir); //negate vector to point in right direction
                  GJK_ASSERT(gjk_dot(dir, ao) >= 0);
                  //make the winding clockwise
                  simplex->p[0] = a;
                  simplex->p[1] = b;
              } 
              *d = dir;
          } else {
              simplex->p[0] = a;
              simplex->count = 1;
              *d = ao;
          }
      } break;
      case 3: { //triangle
        gjk_v2 a = simplex->p[2];
        gjk_v2 b = simplex->p[1];
        gjk_v2 c = simplex->p[0];
        gjk_v2 ao = gjk_V2(-a.x, -a.y);
        gjk_v2 ab = gjk_V2(b.x - a.x, b.y - a.y);
        gjk_v2 ac = gjk_V2(c.x - a.x, c.y - a.y);
        //make sure there is consistent winding (clockwise)
        if(gjk_dot(gjk_perp2D(ac), ab) > 0) { 
            //the simplex should always be clockwise. Made sure of this in the 'line' case
            GJK_ASSERT(!"invalid code path");
            b = simplex->p[0];
            c = simplex->p[1];
            gjk_v2 temp = ab;
            ab = ac;
            ac = temp;
            GJK_ASSERT(gjk_dot(gjk_perp2D(ab), ac) > 0);
        }
        /////
        gjk_v2 perpAc = gjk_perp2D(ac);
        bool onLeft = (gjk_dot(perpAc, ao) > 0);
        
        gjk_v2 perpAb = gjk_negate_v2(gjk_perp2D(ab));
        bool onRight = (gjk_dot(perpAb, ao) > 0);
        
        bool simpleCase = false;
        if(!onLeft && !onRight) {
          result = true;
          break;
        } else if(onLeft) {
          GJK_ASSERT(!onRight);
          if(gjk_dot(ac, ao) > 0) {
            *d = perpAc;
            simplex->p[0] = c;
            simplex->p[1] = a;
            simplex->count = 2;
          } else {
            simpleCase = true;
          }
        } else if(onRight) {
          GJK_ASSERT(!onLeft);
          if(gjk_dot(ab, ao) > 0) {
            *d = perpAb;
            simplex->p[1] = b;
            simplex->p[0] = a;
            simplex->count = 2;
          } else {
            simpleCase = true;
          }
        } 
          if(simpleCase) {
            simplex->p[0] = a;
            simplex->count = 1;
            *d = ao;
          }			
        } break;
        default: {
          GJK_ASSERT(!"invalid code path");
        }
      }
  return result;
}

GjkInfo gjk_objectsCollide_(gjk_v2 *a, int aCount, gjk_v2 *b, int bCount) {
  bool result = false;
  GjkInfo info = {};
  if(aCount < 3 || bCount < 3) return info; //early out if not a full shape
  
  gjk_v2 s = gjk_support(gjk_V2(1, 0), a, aCount, b, bCount);
  Simplex simplex = {};
  simplex.p[0] = s;
  simplex.count = 1;
  gjk_v2 d = gjk_V2(-s.x, -s.y);
  while(true) {
    gjk_v2 p = gjk_support(d, a, aCount, b, bCount);
    if(gjk_dot(p, d) < 0) {
      result = false;
      break;
    }
    gjk_addPoint(&simplex, p);
    GJK_ASSERT(simplex.count >= 2 && simplex.count <= 3); //make sure we haven't gone over
    if(doSimplex(&simplex, &d)) {
      result = true;
      break;
    }
  }
  
  info.collided = result;
  info.simplex = simplex;

  return info;
}

EPA_Edge GJK_EPA_findClosestEdge(Simplex *simplex) {
  //We assume our simplex is wound clockwise 
  EPA_Edge result = {};
  assert(simplex->count >= 3);
  for(int i = 0; i < simplex->count; ++i) {
    int aIndex = i;
    int bIndex = (i == (simplex->count - 1)) ? 0 : i + 1; //wrap the index around 
    gjk_v2 pointA = simplex->p[aIndex];
    gjk_v2 pointB = simplex->p[bIndex];

    gjk_v2 ao = gjk_V2(-pointA.x, -pointA.y);
    //assume clockwise winding
    gjk_v2 ab = gjk_V2(pointB.x - pointA.x, pointB.y - pointA.y);  //goes from a -> b
    gjk_v2 edgeNormal = gjk_normalize_2D(gjk_perp2D(ab)); //This will be pointing away from the origin with clockwise winding
    float dist = gjk_dot(edgeNormal, ao); //find dist from the edge. NOTE: The closest distance is always the perpindicular distance from the edge. This is why it works. (I was getting confused about this.)
    GJK_ASSERT(dist <= 0); //this is to assert for colckwise winding, and therefore normal is towards the origin. 
    dist *= -1; //Make distance negative 
    GJK_ASSERT(dist >= 0);
    if(dist < result.distance || i == 0) { //or first edge to initialize struct 
      //shorter edge is found
      result.distance = dist;
      result.index = bIndex;
      result.normal = edgeNormal;
    }
  }

  return result;
}

#define GJK_TOLERANCE 0.00001
Gjk_EPA_Info gjk_objectsCollide_withEPA(gjk_v2 *a, int aCount, gjk_v2 *b, int bCount) {
    Gjk_EPA_Info result = {};
    GjkInfo info = gjk_objectsCollide_(a, aCount, b, bCount);
    result.collided = info.collided;
    if(result.collided) { //if there is a collision, find the penetration vector to resolve the collision
      while (true) {
        // obtain the feature (edge for 2D) closest to the 
        // origin on the Minkowski Difference
        EPA_Edge e = GJK_EPA_findClosestEdge(&info.simplex);
        // obtain a new support point in the direction of the edge normal (which is pointing away from the origin!)
        gjk_v2 p = gjk_support(e.normal, a, aCount, b, bCount); 
        // check the distance from the origin to the edge against the
        // distance p is along e.normal
        double d = gjk_dot(p, e.normal);
        if (d - e.distance < GJK_TOLERANCE) { //See if the points in the edges direction are the same as the ones we already have. If so we know it is an edge face of the minkowski convex hull 
          //found the solution
          result.normal = gjk_V2(-e.normal.x, -e.normal.y); //we negate it so it it pointing in the direction of the origin.  Since we want to move the minkwoski sum off the origin (because that's where there is a collision), we want it facing the direction we want to move!
          result.distance = d + 0.001; //Epsilon to make sure we aren't colliding anymore. Doens't seem like it is really neccessary thought. 
          break;
        } else {
          // we haven't reached the edge of the Minkowski Difference
          // so continue expanding by adding the new point to the simplex
          // in between the points that made the closest edge
          gjk_addPointAt(&info.simplex, p, e.index);
        }
      }
    }

    return result;
}

bool gjk_objectsCollide(gjk_v2 *a, int aCount, gjk_v2 *b, int bCount) {
  GjkInfo result = gjk_objectsCollide_(a, aCount, b, bCount);
  return result.collided;
}
#endif
