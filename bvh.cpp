// bvh.cpp


#include "bvh.h"
#include "triangle.h"


#define MIN(a,b) ((a) < (b) ? (a) : (b))
#define MAX(a,b) ((a) > (b) ? (a) : (b))



// Build the BVH
//
// Each level has <= k children clustered with k-means.
//
// Upon call, there is guaranteed to be at least one triangle.  


#define K                          8 // number of means in k-means
#define NUM_RANDOM_CANDIDATES     20 // number of candidates for next random seed of K seeds
#define NUM_CLUSTERING_ITERATIONS  4 // number of times to shift cluster means
#define LEAF_COUNT_THRESHOLD       2 // max number of triangles in a leaf



BVH_node * BVH::makeLeafNode( seq<int> &triangleIndices )

{
  BVH_node *n = new BVH_node();
  
  n->isLeaf    = true;
  n->triangles = new seq<int>( triangleIndices ); // copy constructor
  n->bbox      = trianglesBBox( triangleIndices );
    
  return n;
}


BVH_node * BVH::buildSubtree( seq<int> &triangleIndices, int depth )

{
  // Return a leaf node if there are sufficiently few triangles

  if (triangleIndices.size() <= LEAF_COUNT_THRESHOLD)
    return makeLeafNode( triangleIndices );
  
  // Find K seed boxes

  int numSeeds = MIN( K, triangleIndices.size() );

  BBox *seedBoxes = new BBox[numSeeds];
  int  *seedIndices =  new int[numSeeds];

  // Get first seed box

  int randIndex = rand() % triangleIndices.size();
  seedBoxes[0] = triangleBBox( triangleIndices[randIndex] );
  seedIndices[0] = randIndex;

  // Get K-1 more seed boxes
  
  for (int i=1; i<numSeeds; i++) {

    // For the next seed box, try candidate boxes and select the one
    // that is maximally distant from all other boxes so far.

    float maxDist = -1;
    
    for (int j=0; j<NUM_RANDOM_CANDIDATES; j++) { // candidates

      // Get a random triangle that isn't already used as a seed

      int randIndex;
      bool alreadyExists;
      do {
        randIndex = rand() % triangleIndices.size();
        alreadyExists = false;
        for (int k=0; k<i; k++)
          if (randIndex == seedIndices[k]) {
            alreadyExists = true;
            break;
          }
      } while (alreadyExists);

      // Build the box for this triangle

      BBox candidateBox = triangleBBox( triangleIndices[randIndex] );

      // Find distance to closest other box

      float minDist = MAXFLOAT;
      for (int k=0; k<i; k++) {
        float thisDist = boxBoxDistance( candidateBox, seedBoxes[k] );
        if (thisDist < minDist)
          minDist = thisDist;
      }

      // Use this candidate if it's farther than the previous candidate.

      if (minDist > maxDist) {
        maxDist = minDist;
        seedBoxes[i] = candidateBox;
        seedIndices[i] = randIndex;
      }
    }
  }

  // cout << "Found " << numSeeds << " seed boxes" << endl;

  // Iteratively cluster around each seed

  
  seq<int> *clusterTriangles;

#if 0

  // Demonstration code, which assigns each triangle to a RANDOM cluster
  //
  // DELETE THIS CODE in your solution.

  clusterTriangles = new seq<int>[numSeeds];

  for (int i=0; i<triangleIndices.size(); i++) { // all triangles
    int j = rand() % numSeeds;
    clusterTriangles[j].add( triangleIndices[i] );
  }

#else

  // Do NUM_CLUSTERING_ITERATIONS of the following: 1. Put triangles
  // into their closest cluster.  2. adjust the cluster means using
  // equation 2 from Meister and Bittner's paper.  (There's already a
  // function to compute boxBoxDistance() using the metric of that
  // paper.)
  //
  // At the end of this clusterTriangles[i] contains the triangle
  // indices of the triangles in cluster i.  See the demonstration
  // code above for an example. 
  //
  // You can see your results when running this by pressing 'h' to
  // see/hide the hierarchy, 'o' to hide/see the object, and '{' or
  // '}' to decrease/increase the level of the hierarchy that is
  // shown.
  //
  // The bounding boxes of your k-means clusters should be reasonably
  // well separated, unlike those of the demonstration code, above.

  // YOUR CODE HERE

  clusterTriangles = new seq<int>[numSeeds];

  for (int n = 0; n < NUM_CLUSTERING_ITERATIONS; n++) {
	  clusterTriangles->clear();
	  clusterTriangles = new seq<int>[numSeeds];
	  for (int i = 0; i < triangleIndices.size(); i++) {
		  BBox  t = triangleBBox(triangleIndices[i]);
		  float d = boxBoxDistance(t, seedBoxes[0]);
		  int minindex = 0;
		  for (int j = 0; j < numSeeds; j++) {
			  float d_i = boxBoxDistance(t, seedBoxes[j]);
			  if (d_i < d) {
				  minindex = j;
				  d = d_i;
			  }
		  }
		  clusterTriangles[minindex].add(triangleIndices[i]);
	  }

	  for (int i = 0; i < numSeeds; i++) {
		  vec3 minsum = vec3(0, 0, 0);
		  vec3 maxsum = vec3(0, 0, 0);
		  for (int j = 0; j < clusterTriangles[i].size(); j++) {
			  BBox t = triangleBBox(clusterTriangles[i][j]);
			  minsum = minsum + t.min;
			  maxsum = maxsum + t.max;
		  }
		  if (clusterTriangles[i].size() > 0) {
			  minsum = (1.0 / float(clusterTriangles[i].size())) * minsum;
			  maxsum = (1.0 / float(clusterTriangles[i].size())) * maxsum;

			  seedBoxes[i] = BBox(minsum, maxsum);
		  }
	  }

  }
#endif

  delete[] seedBoxes;
  delete[] seedIndices;

  // Now build the node

  BVH_node *n = new BVH_node();
  
  n->isLeaf = false;

  // (recursively build the subtrees)

  n->children  = new seq<BVH_node*>();

  for (int i=0; i<numSeeds; i++)
    if (clusterTriangles[i].size() > 0)
      n->children->add( buildSubtree( clusterTriangles[i], depth+1 ) );

  // (find the bbox around all the subtrees)

  if (n->children->size() > 0) {

    n->bbox = (*n->children)[0]->bbox;

    for (int i=1; i<n->children->size(); i++) {

      BBox bbox = (*n->children)[i]->bbox;

      n->bbox.min.x = MIN( n->bbox.min.x, bbox.min.x );
      n->bbox.min.y = MIN( n->bbox.min.y, bbox.min.y );
      n->bbox.min.z = MIN( n->bbox.min.z, bbox.min.z );

      n->bbox.max.x = MAX( n->bbox.max.x, bbox.max.x );
      n->bbox.max.y = MAX( n->bbox.max.y, bbox.max.y );
      n->bbox.max.z = MAX( n->bbox.max.z, bbox.max.z );
    }
  }

  // Done
    
  return n;
}



// Distance between two bounding boxes (stored in nodes) from Meister
// and Bittner "Parallel BVH Construction ..." paper.

float BVH::boxBoxDistance( BBox &b1, BBox &b2 )

{
  return (b1.min - b2.min).squaredLength() + (b1.max - b2.max).squaredLength();
}



// Return a bounding box around a triangle

BBox BVH::triangleBBox( int triIndex )

{
  vec3 &v0 = (*vertices)[triangles[triIndex].v0];
  vec3 &v1 = (*vertices)[triangles[triIndex].v1];
  vec3 &v2 = (*vertices)[triangles[triIndex].v2];

  vec3 min( MIN(v0.x,MIN(v1.x,v2.x)),
            MIN(v0.y,MIN(v1.y,v2.y)),
            MIN(v0.z,MIN(v1.z,v2.z)) );

  vec3 max( MAX(v0.x,MAX(v1.x,v2.x)),
            MAX(v0.y,MAX(v1.y,v2.y)),
            MAX(v0.z,MAX(v1.z,v2.z)) );

  return BBox(min,max);
}



// Find the bounding box of a SET of triangles

BBox BVH::trianglesBBox( seq<int> &triangleIndices )

{
  BBox bbox = triangleBBox( triangleIndices[0] );

  for (int i=1; i<triangleIndices.size(); i++) {

    BBox triBox = triangleBBox( triangleIndices[i] );

    bbox.min.x = MIN( bbox.min.x, triBox.min.x );
    bbox.min.y = MIN( bbox.min.y, triBox.min.y );
    bbox.min.z = MIN( bbox.min.z, triBox.min.z );

    bbox.max.x = MAX( bbox.max.x, triBox.max.x );
    bbox.max.y = MAX( bbox.max.y, triBox.max.y );
    bbox.max.z = MAX( bbox.max.z, triBox.max.z );
  }

  return bbox;
}


// Intersect a ray with an axis-aligned bounding box (only the box).
//
// Box is [vmin,vmax].  Ray parameters are restricted to [tmin,tmax].
// Return true iff ray intersects box (even if starting from the inside).

bool BVH::rayBoxInt( vec3 &rayStart, vec3 &rayDir, float tmin, float tmax, BBox &bbox )

{
	// YOUR CODE HERE

	float tmin_x, tmax_x, tmin_y, tmax_y, tmin_z, tmax_z, tmin_current, tmax_current = 0;
	
	if (rayDir.x >= 0) {
		tmin_x = (bbox.min.x - rayStart.x) / rayDir.x;
		tmax_x = (bbox.max.x - rayStart.x) / rayDir.x;
	}
	else {
		tmax_x = (bbox.min.x - rayStart.x) / rayDir.x;
		tmin_x = (bbox.max.x - rayStart.x) / rayDir.x;
	}

	if (rayDir.y >= 0) {
		tmin_y = (bbox.min.y - rayStart.y) / rayDir.y;
		tmax_y = (bbox.max.y - rayStart.y) / rayDir.y;
	}
	else {
		tmax_y = (bbox.min.y - rayStart.y) / rayDir.y;
		tmin_y = (bbox.max.y - rayStart.y) / rayDir.y;
	}

	if (rayDir.z >= 0) {
		tmin_z = (bbox.min.z - rayStart.z) / rayDir.z;
		tmax_z = (bbox.max.z - rayStart.z) / rayDir.z;
	}
	else {
		tmax_z = (bbox.min.z - rayStart.z) / rayDir.z;
		tmin_z = (bbox.max.z - rayStart.z) / rayDir.z;
	}

	if (tmax_x < tmin_y || tmin_x > tmax_y) {
		return false;
	}

	tmin_current = MAX(tmin_x, tmin_y);
	tmax_current = MIN(tmax_x, tmax_y);

	if (tmin_current > tmax || tmax_current < tmin) {
		return false;
	}

	if (tmin_current > tmax_z  || tmax_current < tmin_z) {
		return false;
	}

	tmin_current = MAX(tmin_current, tmin_z);
	tmax_current = MIN(tmax_current, tmax_z);

	if (tmin_current < tmax && tmax_current > tmin) {
		return true;
	}
	else {
		return false;
	}

}



// Draw a certain number of levels of the BVH.


void BVH::renderSubtreeGL( BVH_node *n, mat4 &WCS_to_CCS, int levelsRemaining )

{
  if (levelsRemaining < 0)
    return;

  if (!n->isLeaf)
    for (int i=0; i<n->children->size(); i++)
      renderSubtreeGL( (*n->children)[i], WCS_to_CCS, levelsRemaining-1 );

  if (levelsRemaining == 0)
    n->bbox.renderGL( WCS_to_CCS );
}



// Compute plane/ray intersection.
//
// 'sourceTriangleIndex' is passed in as the triangleIndex of the
// originating triangle.  Do not check for intersection with this
// triangle.


bool BVH::rayIntBVH( BVH_node*n, vec3 rayStart, vec3 rayDir, int sourceTriangleIndex, float maxParam, vec3 & intPoint, vec3 & intNormal, vec3 & intTexCoords, float & intParam, Material * &intMaterial, int &intTriangleIndex )

{
  bool hit = false;

  if (n->isLeaf) { // A leaf, so check all the triangles

    for (int i=0; i<n->triangles->size(); i++) {
      int triangleIndex = (*n->triangles)[i];
      if (triangleIndex != sourceTriangleIndex) { // this isn't the triangle from which the ray started

        float param, alpha, beta, gamma;
        vec3 point, normal, texcoords;

        if (triangleInt( rayStart, rayDir, triangleIndex, maxParam, param, point, normal, texcoords, alpha, beta, gamma )) { // returns param, point, alpha, beta, gamma

          // found a new closest point

          intParam  = param;
          intPoint  = point;
          intNormal = normal;
          intTexCoords = texcoords;
          intTriangleIndex = triangleIndex;
          intMaterial = materials[ triangles[ (*n->triangles)[i] ].materialID ]; 

          maxParam = param;
          hit = true;
        }
      }
    }

    // Note that bump mapping is not implemented yet, but should be
    // done here to return the bump-mapped normal.

  } else { // Not a leaf, so recurse into children

    for (int i=0; i<n->children->size(); i++) {
      BVH_node *thisNode = (*n->children)[i];
      if (rayBoxInt( rayStart, rayDir, 0, maxParam, thisNode->bbox )) {
        if (rayIntBVH( thisNode, rayStart, rayDir, sourceTriangleIndex, maxParam, intPoint, intNormal, intTexCoords, intParam, intMaterial, intTriangleIndex )) {
          maxParam = intParam;
          hit = true;
        }
      }
    }
  }

  return hit;
}
  


// Adapted from triangle.cpp for use by BVH

bool BVH::triangleInt( vec3 &rayStart, vec3 &rayDir, int triangleIndex, float maxParam, float &param, vec3 &point, vec3 &normal, vec3 &texCoord, float &alpha, float &beta, float &gamma )

{
  BVH_triangle &tri = triangles[triangleIndex];

  vec3 &v0 = (*vertices)[ tri.v0 ];
  vec3 &v1 = (*vertices)[ tri.v1 ];
  vec3 &v2 = (*vertices)[ tri.v2 ];

  vec3 faceNormal = (*facetnorms)[ tri.faceID ];

  // Compute ray/plane intersection

  float dn = rayDir * faceNormal;

  if (fabs(dn) < 0.0001) // 'fabs' allows intersection from behind the plane.
    return false; // ray is parallel to plane.

  float t = (faceNormal*(v0-rayStart)) / dn;
  if (t < 0)
    return false; // plane is behind starting point

  if (t >= maxParam)
    return false; // a closer intersection (at 'maxParam') has already been detected in other code
  
  vec3 thisPoint = rayStart + t * rayDir;

  // Compute barycentric coords

  float denom = ((v1-v0) ^ (v2-v0)) * faceNormal; // should cache this cross product.

  float thisBeta  = (((v1-v0) ^ (thisPoint - v0)) * faceNormal) / denom; // for v2
  float thisAlpha = (((thisPoint-v0) ^ (v2 - v0)) * faceNormal) / denom; // for v1
  float thisGamma = 1 - thisAlpha - thisBeta; // for v0

  if (thisAlpha < 0 || thisBeta < 0 || thisGamma < 0)
    return false; // outside of triangle

  // Return intersection info

  param  = t;
  point  = thisPoint;
  alpha  = thisAlpha;
  beta   = thisBeta;
  gamma  = thisGamma;

  if (!obj->hasVertexNormals)
    
    normal = faceNormal; // use face normal

  else {

    vec3 &n0 = (*normals)[ tri.n0 ]; // interpolate vertex normals
    vec3 &n1 = (*normals)[ tri.n1 ];
    vec3 &n2 = (*normals)[ tri.n2 ];

    normal = (gamma*n0 + alpha*n1 + beta*n2).normalize();
  }

  if (obj->hasVertexTexCoords) {
    
    vec3 &t0 = (*texcoords)[ tri.t0 ]; // interpolate vertex texcoords
    vec3 &t1 = (*texcoords)[ tri.t1 ];
    vec3 &t2 = (*texcoords)[ tri.t2 ];

    texCoord = gamma*t0 + alpha*t1 + beta*t2;
  }

  return true;
}
