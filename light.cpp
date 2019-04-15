/* light.C
 */


#include "headers.h"

#include "light.h"
#include "main.h"


// Draw a light

void Light::draw( GPUProgram *prog, mat4 &WCS_to_VCS, mat4 &VCS_to_CCS )

{
  if (sphere == NULL) {
    sphere = new Sphere( position, 0.1 );
  }

  // Need two over-the-shoulder lights to illuminate the rendered light sphere

  vec3 lightPos[2] = { vec3(1000,1000,1000), vec3(-1000,-1000,1000) };
  gpuProg->setVec3( "lightPos", lightPos, 2 );
  gpuProg->setInt( "numLights", 1 );

  sphere->renderGL( prog, WCS_to_VCS, VCS_to_CCS, scene->sceneScale );
}



// Output a light

ostream& operator << ( ostream& stream, Light const& obj )

{
  stream << "light" << endl
         << "  " << obj.position << endl
         << "  " << obj.colour << endl;
  return stream;
}


// Input a light

istream& operator >> ( istream& stream, Light & obj )

{
  skipComments( stream );  stream >> obj.position;
  skipComments( stream );  stream >> obj.colour;
  return stream;
}

