/* volume.h
 */


#ifndef VOLUME_H
#define VOLUME_H


#include "object.h"
#include "bbox.h"


typedef unsigned short int Voxel;


class Volume : public Object {

  char *name;

  Voxel **volume;

  vec3 dim;			// dimensions of the dataset
  vec3 scale;			// scale factor
  vec3 centre;		// centre of dataset
  BBox   bbox;			// bounding box

  GLubyte *readVolume( char *filename, int width, int height, int depth );

 public:

  Volume() {}

  bool rayInt( vec3 rayStart, vec3 rayDir, int objPartIndex, float maxParam,
	       vec3 &intPoint, vec3 &intNorm, vec3 &intTexCoords, float &intParam, Material * &mat, int &intPartIndex );

  void input( istream &stream );
  void output( ostream &stream ) const;
  void renderGL( GPUProgram * gpuProg, mat4 &WCS_to_VCS, mat4 &VCS_to_CCS );

  float sample( float x, float y, float z ); /* in [0,1]x[0,1]x[0,1] */
  float sample( int x, int y, int z );       /* in [0,xdim-1]x[0,ydim-1]x[0,zdim-1] */
};


#endif
