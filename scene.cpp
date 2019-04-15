/* scene.C
 */


#include "headers.h"
#ifndef WIN32
  #include <unistd.h>
#endif

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "scene.h"
#include "rtWindow.h"
#include "sphere.h"
#include "triangle.h"
#include "volume.h"
#include "wavefrontobj.h"
#include "light.h"
#include "font.h"
#include "main.h"
#include "material.h"
#include "arrow.h"

#include <string>


#ifndef MAXFLOAT
  #define MAXFLOAT 9999999
#endif


#define UPDATE_INTERVAL 0.05  // update the screen with each 5% of RT progress

#define INDENT(n) { for (int i=0; i<(n); i++) cout << " "; }

vec3 backgroundColour(1,1,1);
vec3 blackColour(0,0,0);

#define NUM_SOFT_SHADOW_RAYS 50
#define MAX_NUM_LIGHTS 4


// Find the first object intersected

bool Scene::findFirstObjectInt( vec3 rayStart, vec3 rayDir, int thisObjIndex, int thisObjPartIndex,
                                vec3 &P, vec3 &N, vec3 &T, float &param, int &objIndex, int &objPartIndex, Material *&mat, int lightIndex )

{
  if (storingRays)
    storedRays.add( rayStart );

  bool hit = false;

  float maxParam = MAXFLOAT;

  for (int i=0; i<objects.size(); i++) {

    WavefrontObj* wfo = dynamic_cast<WavefrontObj*>( objects[i] );

     // don't check for int with the originating object for non-wavefront objects (since such objects are convex)
    
    if (wfo || i != thisObjIndex) {
      
      vec3 point, normal, texcoords;
      float t;
      Material *intMat;
      int intPartIndex;

      if (objects[i]->rayInt( rayStart, rayDir, ((i != thisObjIndex) ? -1 : thisObjPartIndex), maxParam, point, normal, texcoords, t, intMat, intPartIndex )) {

        P = point;
        N = normal;
        T = texcoords;
        param = t;
        objIndex = i;
        objPartIndex = intPartIndex;
        mat = intMat;

        maxParam = t; // In future, don't intersect any farther than this
        hit = true;
      }
    }
  }

  if (storingRays) {

    if (hit) {
      storedRays.add( P );
      if (lightIndex >= 0) {
        storedRayColours.add( vec3(.843,.710,.278) ); // GOLD: shadow ray toward a light that is (perhaps) blocked
      } else
        storedRayColours.add( vec3(.1,.7,.7) ); // CYAN: normal ray that hits something
    } else {
      if (lightIndex >= 0) {
        storedRays.add( lights[lightIndex]->position );
        storedRayColours.add( vec3(.843,.710,.278) ); // GOLD: shadow ray toward a light that is NOT blocked
      } else {
        storedRays.add( rayStart+sceneScale*2*rayDir );
        storedRayColours.add( vec3(.3,.3,.3) ); // GREY: normal ray that misses
      }
    }
  }

  return hit;
}

// Raytrace: This is the main raytracing routine which finds the first
// object intersected, performs the lighting calculation, and does
// recursive calls.
//
// This returns the colour received on the ray.

vec3 Scene::raytrace( vec3 &rayStart, vec3 &rayDir, int depth, int thisObjIndex, int thisObjPartIndex )

{
  // Terminate the ray?

  depth++;

  if (depth > maxDepth)
    return blackColour;

  // Find the closest object intersected

  vec3     P, N, texcoords;
  float    t;
  int      objIndex, objPartIndex;
  Material *mat;

  // Below, 'rayStart' is the ray staring point
  //        'rayDir' is the direction of the ray
  //        'thisObjIndex' is the index of the originating object
  //        'thisObjPartIndex' is the index of the part on the originating object (e.g. the triangle)
  //
  // If a hit is made then at the intersection point:
  //        'P' is the position
  //        'N' is the normal
  //        'texcoords' are the texture coordinates
  //        't' is the ray parameter at intersection
  //        'objIndex' is the index of the object that is hit
  //        'objPartIndex' is the index of the part of object that is hit
  //        'mat' is the material at the intersection point
  
  bool hit = findFirstObjectInt( rayStart, rayDir, thisObjIndex, thisObjPartIndex, P, N, texcoords, t, objIndex, objPartIndex, mat, -1 );

  // No intersection: Return background colour

  if (!hit)
    if (depth == 1)
      return backgroundColour;
    else
      return blackColour;

  // Find reflection direction & incoming light from that direction

  Object &obj = *objects[objIndex];

  vec3 E = (-1 * rayDir).normalize();
  vec3 R = (2 * (E * N)) * N - E;

  float alpha;
  vec3  colour = obj.textureColour( P, objPartIndex, alpha, texcoords );

  vec3 kd = vec3( colour.x*mat->kd.x, colour.y*mat->kd.y, colour.z*mat->kd.z );

  if (debug) { // 'debug' is set when tracing through a pixel that the user right-clicked
    INDENT(2*depth); cout << "texcoords " << texcoords << endl;
    INDENT(2*depth); cout << "   colour " << colour << endl;
    INDENT(2*depth); cout << "       id " << kd << endl;
    INDENT(2*depth); cout << "        P " << P << endl;
    INDENT(2*depth); cout << "        N " << N << endl;
    INDENT(2*depth); cout << "    alpha " << alpha << endl;
  }

  vec3 Iout = mat->Ie + vec3( mat->ka.x * Ia.x, mat->ka.y * Ia.y, mat->ka.z * Ia.z );

  // Compute glossy reflection

  if (mat->g < 0 || mat->g > 1) {
    cerr << "Material glossiness is outside the range [0,1]" << endl;
    exit(1);
  }

  float g = mat->g;

  if (g == 1 || glossyIterations == 1) {

    vec3 Iin = raytrace( P, R, depth, objIndex, objPartIndex );

    Iout = Iout + calcIout( N, R, E, E, kd, mat->ks, mat->n, Iin );
    
  } else if (g > 0) {

    // Glossy reflection
    //
    // Cast 'glossyIterations' random rays from P centred on direction
    // R using glossiness 'g' to control the spread of the rays.  For
    // glossiness 'g', the half angle of the cone of rays is
    // arccos(g).  Average the *reflected* colours of the rays and add
    // that average to Iout.

    // YOUR CODE HERE

  }
  
  // Add direct contributions from lights

  for (int i=0; i<lights.size(); i++) {
    Light &light = *lights[i];

    vec3 L = light.position - P; // point light

    if (N*L > 0) {

      float  Ldist = L.length();
      L = (1.0/Ldist) * L;

      vec3 intP, intN, intTexCoords;
      float intT;
      int intObjIndex, intObjPartIndex;
      Material *intMat;

      // Is there an object between P and the light?

      bool found = findFirstObjectInt( P, L, objIndex, objPartIndex, intP, intN, intTexCoords, intT, intObjIndex, intObjPartIndex, intMat, i );

      if (!found || intT > Ldist) { // no object: Add contribution from this light
        vec3 Lr = (2 * (L * N)) * N - L;
        Iout = Iout + calcIout( N, L, E, Lr, kd, mat->ks, mat->n, light.colour);
      }
    }
  }

  return Iout;
}


// Calculate the outgoing intensity due to light Iin entering from
// direction L and exiting to direction E, with normal N.  Reflection
// direction R is provided, along with the material properties Kd, 
// Ks, and n.
//
//       Iout = Iin * ( Kd (N.L) + Ks (R.V)^n )

vec3 Scene::calcIout( vec3 N, vec3 L, vec3 E, vec3 R,
                        vec3 Kd, vec3 Ks, float ns,
                        vec3 In )

{
  // Don't illuminate from the back of the surface

  if (N * L <= 0)
    return blackColour;

  // Both E and L are in front:

  vec3 Id = (L*N) * In;

  vec3 Is;

  if (R*E < 0)
    Is = blackColour;           // Is = 0 from behind the
  else                          // reflection direction
    Is = pow( R*E, ns ) * In;

  return vec3( Is.x*Ks.x+Id.x*Kd.x, Is.y*Ks.y+Id.y*Kd.y, Is.z*Ks.z+Id.z*Kd.z );
}


// Determine the colour of one pixel.  This is where
// the raytracing actually starts.

vec3 Scene::pixelColour( int x, int y )

{
  if (x == debugPixel.x && y == debugPixel.y) {
    debug = true;
    cout << "---------------- start debugging at pixel " << debugPixel << " ----------------" << endl;
  }

  vec3 result;

#if 1

  // DELETE THE FOLLOWING in your solution code. 

  vec3 dir = (llCorner + (x+0.5)*right + (y+0.5)*up).normalize();

  result = raytrace( eye->position, dir, 0, -1, -1 );

#else

  // Antialias through a pixel using ('numPixelSamples' x
  // 'numPixelSamples') rays.  Use a regular pattern if 'jitter' is
  // false; use a jittered patter if 'jitter' is true.

  // YOUR CODE HERE

#endif

  if (storingRays)
    storingRays = false;

  if (debug) {
    cout << "---------------- stop debugging ----------------" << endl;
    debug = false;
  }

  return result;
}


// Read the scene from an input stream

void Scene::read( const char *basename, istream &in )

{
  char command[1000];

  while (in) {

    skipComments( in );
    in >> command;
    if (!in || command[0] == '\0')
      break;
    
    skipComments( in );

    if (strcmp(command,"sphere") == 0) {

      Sphere *o = new Sphere();
      in >> *o;
      objects.add( o );
      
    } else if (strcmp(command,"triangle") == 0) {

      Triangle *o = new Triangle();
      in >> *o;
      objects.add( o );
      
    } else if (strcmp(command,"volume") == 0) {

      Volume *o = new Volume();
      in >> *o;
      objects.add( o );
      
    } else if (strcmp(command,"material") == 0) {

      Material *m = new Material();
      in >> *m;
      materials.add( m );
      
    } else if (strcmp(command,"wavefront") == 0) {

      // Rely on the wavefront.cpp code to read this

      std::string filename;
      in >> filename;

      char pathname[1000];
      sprintf( pathname, "%s/%s", basename, filename.c_str() );

      WavefrontObj *o = new WavefrontObj( pathname );
      objects.add( o );

      // Update scene's scale

      if (o->obj->radius/2 > sceneScale)
        sceneScale = o->obj->radius/2;
      
    } else if (strcmp(command,"light") == 0) {

      Light *o = new Light();
      in >> *o;
      lights.add( o );
      
    } else if (strcmp(command,"eye") == 0) {

      eye = new Eye();
      in >> *eye;

      win->eye = eye->position;
      win->lookAt = eye->lookAt;
      win->upDir = eye->upDir;
      win->fovy = eye->fovy;
      
    } else {
      
      cerr << "Command '" << command << "' not recognized" << endl;
      exit(-1);
    }
  }

  if (lights.size() == 0) {
    cerr << "No lights were provided in " << basename << " so the scene would be black." << endl;
    exit(1);
  }
}



// Output the whole scene (mainly for debugging the reader)


void Scene::write( ostream &out )

{
  out << *eye << endl;

  for (int i=0; i<lights.size(); i++)
    out << *lights[i] << endl;

  for (int i=0; i<materials.size(); i++)
    out << *materials[i] << endl;

  for (int i=0; i<objects.size(); i++)
    out << *objects[i] << endl;
}


char *Scene::statusMessage()

{
  static char buffer[1000];

  sprintf( buffer, "depth %d, glossy %d", maxDepth, glossyIterations );

  return buffer;
}



// Draw the scene.  This sets things up and simply
// calls pixelColour() for each pixel.


void Scene::renderRT( bool restart )

{
  static float nextDot;
  static int nextx, nexty;

  mat4 WCS_to_VCS = lookat( win->eye, win->lookAt, win->upDir );
  mat4 VCS_to_CCS = perspective( win->fovy, win->windowWidth / win->windowHeight, win->zNear, win->zFar );

  if (restart) {

    // Copy the window eye into the scene eye

    eye->position = win->eye;
    eye->lookAt = win->lookAt;
    eye->upDir = win->upDir;
    eye->fovy = win->fovy;

    // Compute the image plane coordinate system

    up = (2 * tan( eye->fovy / 2.0 )) * eye->upDir.normalize();

    right = (2 * tan( eye->fovy / 2.0 ) * (float) win->windowWidth / (float) win->windowHeight)
      * ((eye->lookAt - eye->position) ^ eye->upDir).normalize();
  
    llCorner = (eye->lookAt - eye->position).normalize()
      - 0.5 * up - 0.5 * right;

    up = (1.0 / (float) (win->windowHeight-1)) * up;
    right = (1.0 / (float) (win->windowWidth-1)) * right;

    if (nextDot != 0) {
      cout << "\r           \r";
      cout.flush();
    }

    nextx = 0;
    nexty = 0;
    nextDot = UPDATE_INTERVAL;

    stop = false;

    // Clear the RT image
    
    if (rtImage != NULL)
      delete [] rtImage;

    rtImage = NULL;
  }

  // Set up a new RT image

  if (rtImage == NULL) {
    rtImage = new vec4[ (int) (win->windowWidth * win->windowHeight) ];
    for (int i=0; i<win->windowWidth * win->windowHeight; i++)
      rtImage[i] = vec4(0,0,0,0); // transparent
  }

  if (stop || showBVH)
    return;

  // Draw the next pixel

  vec3 colour = pixelColour( nextx, nexty );

  rtImage[ nextx + nexty * (int) win->windowWidth ] = vec4( colour.x, colour.y, colour.z, 1 ); // opaque

  // Move (nextx,nexty) to the next pixel

  nexty++;
  if (nexty >= win->windowHeight) {
    nexty = 0;
    nextx++;
    if ((float)nextx/(float)win->windowWidth >= nextDot) {

      nextDot += UPDATE_INTERVAL;

      draw_RT_and_GL( gpuProg, WCS_to_VCS, VCS_to_CCS ); // gpuProg is from main.cpp
    }
    
    if (nextx >= win->windowWidth) { // finished

      draw_RT_and_GL( gpuProg, WCS_to_VCS, VCS_to_CCS );

      if (showBVH) {
        glDisable( GL_DEPTH_TEST );
        mat4 WCS_to_CCS = VCS_to_CCS * WCS_to_VCS;
        for (int i=0; i<objects.size(); i++) {
          WavefrontObj* wfo = dynamic_cast<WavefrontObj*>( objects[i] );
          if (wfo)
            wfo->bvh.renderGL( WCS_to_CCS );
        }
        glEnable( GL_DEPTH_TEST );
      }

      nextx = 0;
      stop = true;
      cout << "\r           \r";
      cout.flush();
    }
  }
}


// Render the scene with OpenGL


void Scene::renderGL( mat4 &WCS_to_VCS, mat4 &VCS_to_CCS )

{
  // create axes and glverts here so that they are not created before
  // the window is initialized

  if (axes == NULL)
    axes = new Axes();

  if (glverts == NULL)
    glverts = new GLVerts();
  
  // Set up the framebuffer
 
  glEnable( GL_DEPTH_TEST );
  glClearColor( backgroundColour.x, backgroundColour.y, backgroundColour.z, 0 );
  glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

  // Copy the window eye into the scene eye

  eye->position = win->eye;
  eye->lookAt = win->lookAt;
  eye->upDir = win->upDir;
  eye->fovy = win->fovy;

  // Draw the objects

  gpuProg->activate();

  // Light directions and colours for GPU

  int numLights = (lights.size() < MAX_NUM_LIGHTS ? lights.size() : MAX_NUM_LIGHTS);

  vec3 lightPos[MAX_NUM_LIGHTS];
  vec3 lightIin[MAX_NUM_LIGHTS];

  for (int i=0; i<numLights; i++) {
    lightPos[i] = (WCS_to_VCS * vec4( lights[i]->position, 1.0 )).toVec3();
    lightIin[i] = lights[i]->colour;
  }

  gpuProg->setVec3( "lightPos", lightPos, MAX_NUM_LIGHTS );
  gpuProg->setVec3( "lightIin", lightIin, MAX_NUM_LIGHTS );
  
  gpuProg->setInt( "numLights", numLights );

  // Ambient lighting

  gpuProg->setVec3( "Ia", vec3(1,1,1) );

  // Axes

  if (showAxes) {
    mat4 MVP = VCS_to_CCS * WCS_to_VCS;
    axes->draw( MVP );
  }

  // Draw any stored points (for debugging)

  // glPointSize( 4.0 );
  // glBegin( GL_POINTS );
  // for (int i=0; i<storedPoints.size(); i++)
  //   glVertex3fv( (GLfloat*) &storedPoints[i].x );
  // glEnd();

  // Show the lights

  for (int i=0; i<lights.size(); i++)
    lights[i]->draw( gpuProg, WCS_to_VCS, VCS_to_CCS );
      
  // Draw any stored rays (for debugging)

  drawStoredRays( gpuProg, WCS_to_VCS, VCS_to_CCS );

  // Show the BVH

  if (showBVH) {
    glDisable( GL_DEPTH_TEST );
    mat4 WCS_to_CCS = VCS_to_CCS * WCS_to_VCS;
    for (int i=0; i<objects.size(); i++) {
      WavefrontObj* wfo = dynamic_cast<WavefrontObj*>( objects[i] );
      if (wfo)
        wfo->bvh.renderGL( WCS_to_CCS );
    }
    glEnable( GL_DEPTH_TEST );
  }

  // Draw everything

  if (showObjects)
    for (int i=0; i<objects.size(); i++)
      objects[i]->renderGL( gpuProg, WCS_to_VCS, VCS_to_CCS );

  // Show status message
      
  render_text( statusMessage(), 10, 10, win->window );

  // Done

  gpuProg->deactivate();
}



// Draw any stored rays (for debugging)


#define BASE_ARROW_RADIUS 0.01


void Scene::drawStoredRays( GPUProgram *gpuProg, mat4 &WCS_to_VCS, mat4 &VCS_to_CCS )

{
  if (storedRays.size() > 0) {
        
    if (arrow == NULL)
      arrow = new Arrow();

    vec3 lightPos[2] = { vec3(1000,1000,1000), vec3(-1000,-1000,1000) }; // two lights on the rays
    gpuProg->setVec3( "lightPos", lightPos, 2 );
    gpuProg->setInt( "numLights", 2 );

    for (int i=0; i<storedRays.size(); i+=2) {

      vec3 dir = storedRays[i] - storedRays[i+1];
      mat4 OCS_to_WCS = translate( storedRays[i+1] ) * rotate( vec3(0,0,1), dir );

      arrow->mat->kd = storedRayColours[i/2];

      arrow->draw( gpuProg, OCS_to_WCS, WCS_to_VCS, VCS_to_CCS, dir.length(), sceneScale*BASE_ARROW_RADIUS );
    }
  }
}



// Draw the scene, then draw the RT image on top of it.  Transparent
// pixels in the RT image are those that have not been calculated yet.

void Scene::draw_RT_and_GL( GPUProgram *gpuProg, mat4 &WCS_to_VCS, mat4 &VCS_to_CCS )

{
  win->display(); // GL rendering
  drawRTImage();  // RT image on top

  // Redraw the stored rays and lights over top of the RT image

  gpuProg->activate();
  
  mat4 VP = translate(0,0,-0.1) * VCS_to_CCS; // move a little forward in the CCS so the it overdraws the previously-drawn arrow at this location

  for (int i=0; i<lights.size(); i++)
    lights[i]->draw( gpuProg, WCS_to_VCS, VP );
  
  drawStoredRays( gpuProg, WCS_to_VCS, VP );

  gpuProg->deactivate();

  // Perhaps show the BVH

  if (showBVH) {
    glDisable( GL_DEPTH_TEST );
    mat4 WCS_to_CCS = VCS_to_CCS * WCS_to_VCS;
    for (int i=0; i<objects.size(); i++) {
      WavefrontObj* wfo = dynamic_cast<WavefrontObj*>( objects[i] );
      if (wfo)
        wfo->bvh.renderGL( WCS_to_CCS );
    }
    glEnable( GL_DEPTH_TEST );
  }

  // Perhaps show the pixel zoom

  if (buttonDown == GLFW_MOUSE_BUTTON_MIDDLE)
    showPixelZoom( mouse );

  // Status message

  render_text( statusMessage(), 10, 10, win->window );

  // Done

  glfwSwapBuffers( win->window );
}



// Draw a quad at the mouse position.  Texture the quad with zoomed-pixels near the mouse position

#define ZOOM_FACTOR 13          // scale applied to one pixel
#define ZOOM_RADIUS 15          // radius of window, in original pixels

void Scene::showPixelZoom( vec2 mouse )

{
  if (rtImage == NULL)
    return;

  // Draw texture on a small quad

  vec2 texPos( mouse.x / win->windowWidth, (win->windowHeight - mouse.y) / win->windowHeight ); // mouse position in texture
  vec2 winPos = 2 * texPos - vec2(1,1); // mouse position in window
    
  vec2 texRadius = ZOOM_RADIUS * vec2( 1 / win->windowWidth, 1 / win->windowHeight ); // radius in texture coordinates
  vec2 winRadius = ZOOM_FACTOR * 2 * texRadius; // radius in window coordinates

  vec2 verts[8] = {
    winPos + vec2( -winRadius.x, -winRadius.y ), // positions
    winPos + vec2( -winRadius.x,  winRadius.y ),
    winPos + vec2(  winRadius.x, -winRadius.y ),
    winPos + vec2(  winRadius.x,  winRadius.y ),

    texPos + vec2( -texRadius.x, -texRadius.y ), // texture coordinates
    texPos + vec2( -texRadius.x,  texRadius.y ),
    texPos + vec2(  texRadius.x, -texRadius.y ),
    texPos + vec2(  texRadius.x,  texRadius.y )
  };
    
  GLuint VAO, VBO;

  glGenVertexArrays( 1, &VAO );
  glBindVertexArray( VAO );

  glGenBuffers( 1, &VBO );
  glBindBuffer( GL_ARRAY_BUFFER, VBO );

  glBufferData( GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW );

  glEnableVertexAttribArray( 0 );
  glVertexAttribPointer( 0, 2, GL_FLOAT, GL_FALSE, 0, 0 );

  glEnableVertexAttribArray( 1 );
  glVertexAttribPointer( 1, 2, GL_FLOAT, GL_FALSE, 0, (void*) (sizeof(vec2)*4) );

  // Set up GPU

  glActiveTexture( GL_TEXTURE1 );
  glBindTexture( GL_TEXTURE_2D, rtImageTexID );

  glDisable( GL_DEPTH_TEST );
  gpu->activate();
  gpu->setInt( "texUnitID", 1 );

  // Draw texture

  gpu->setInt( "texturing", 1 );
  glDrawArrays( GL_TRIANGLE_STRIP, 0, 4 );

  // Draw boundary

  vec2 lineverts[4] = {
    winPos + vec2( -winRadius.x, -winRadius.y ), // positions
    winPos + vec2(  winRadius.x, -winRadius.y ),
    winPos + vec2(  winRadius.x,  winRadius.y ),
    winPos + vec2( -winRadius.x,  winRadius.y )
  };
    
  glBufferData( GL_ARRAY_BUFFER, sizeof(lineverts), lineverts, GL_STATIC_DRAW );

  glEnableVertexAttribArray( 0 );
  glVertexAttribPointer( 0, 2, GL_FLOAT, GL_FALSE, 0, 0 );

  glDisableVertexAttribArray( 1 );

  gpu->setInt( "texturing", 0 );

  glLineWidth( 5 );
  glDrawArrays( GL_LINE_LOOP, 0, 4 );
  glLineWidth( 1 );

  // Done

  gpu->deactivate();
  glEnable( GL_DEPTH_TEST );

  glDeleteBuffers( 1, &VBO );
  glDeleteVertexArrays( 1, &VAO );

  glBindTexture( GL_TEXTURE_2D, 0 );
}



void Scene::drawRTImage()

{
  if (rtImage == NULL)
    return;

  if (gpu == NULL) {
    gpu = new GPUProgram();
    gpu->init( vertShader, fragShader );
  }

  // Send texture to GPU

  if (rtImageTexID == 0)
    glGenTextures( 1, &rtImageTexID );

  glActiveTexture( GL_TEXTURE1 );
  glBindTexture( GL_TEXTURE_2D, rtImageTexID );

  glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
  glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );

  glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA, win->windowWidth, win->windowHeight, 0, GL_RGBA, GL_FLOAT, rtImage );

  // Draw texture on a full-screen quad

  vec2 verts[8] = {
    vec2( -1, -1 ), vec2( -1, 1 ), vec2( 1, -1 ), vec2( 1, 1 ), // positions
    vec2(  0,  0 ), vec2(  0, 1 ), vec2( 1,  0 ), vec2( 1, 1 )  // texture coordinates
  };
    
  GLuint VAO, VBO;

  glGenVertexArrays( 1, &VAO );
  glBindVertexArray( VAO );

  glGenBuffers( 1, &VBO );
  glBindBuffer( GL_ARRAY_BUFFER, VBO );

  glBufferData( GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW );

  glEnableVertexAttribArray( 0 );
  glVertexAttribPointer( 0, 2, GL_FLOAT, GL_FALSE, 0, 0 );

  glEnableVertexAttribArray( 1 );
  glVertexAttribPointer( 1, 2, GL_FLOAT, GL_FALSE, 0, (void*) (sizeof(vec2)*4) );

  glDisable( GL_DEPTH_TEST );
  glEnable( GL_BLEND );
  glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

  gpu->activate();
  gpu->setInt( "texUnitID", 1 );
  gpu->setInt( "texturing", 1 );
  glDrawArrays( GL_TRIANGLE_STRIP, 0, 4 );
  gpu->deactivate();

  glDisable( GL_BLEND );
  glEnable( GL_DEPTH_TEST );

  glDeleteBuffers( 1, &VBO );
  glDeleteVertexArrays( 1, &VAO );

  glBindTexture( GL_TEXTURE_2D, 0 );
}



char *Scene::vertShader = "\n\
\n\
  #version 300 es\n\
\n\
  layout (location = 0) in mediump vec2 posIn;\n\
  layout (location = 1) in mediump vec2 texCoordsIn;\n\
\n\
  smooth out mediump vec2 pos;\n\
  smooth out mediump vec2 texCoords;\n\
\n\
  void main() {\n\
    gl_Position = vec4( posIn, -1, 1 );\n\
    pos = posIn;\n\
    texCoords = texCoordsIn;\n\
  }\n\
";


char *Scene::fragShader = "\n\
\n\
  #version 300 es\n\
\n\
  uniform sampler2D texUnitID;\n\
  uniform bool texturing;\n\
\n\
  smooth in mediump vec2 pos;\n\
  smooth in mediump vec2 texCoords;\n\
  out mediump vec4 outputColour;\n\
\n\
  void main() {\n\
\n\
    if (texturing) {\n\
      mediump vec4 c = texture( texUnitID, texCoords );\n \
      outputColour = c;\n\
    } else\n\
      outputColour = vec4(0.5,0.5,0.5,1);\n\
  }\n\
";



