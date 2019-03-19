/* rtWindow.h
 */


#ifndef RTWINDOW_H
#define RTWINDOW_H

class RTwindow;


#include "main.h"
#include "arcballWindow.h"
#include "scene.h"


class RTwindow : public arcballWindow {

  Scene *scene;
  bool   viewpointChanged;

 public:

  RTwindow( int x, int y, int width, int height, const char *title, Scene *s )
    : arcballWindow( x, y, width, height, title ) {
    scene = s;
    viewpointChanged = true;
  }

  void userDraw( mat4 &WCS_to_VCS, mat4 &VCS_to_CCS ) {
    viewpointChanged = true;
    scene->renderGL( WCS_to_VCS, VCS_to_CCS );
  }

  void raytrace( mat4 &MVP ) {
    scene->renderRT( viewpointChanged );
    viewpointChanged = false;
  }

  bool userMouseClick( vec2 mouse, int button ) {

    if (button == GLFW_MOUSE_BUTTON_LEFT ) {

      scene->storedRays.clear();
      scene->storedRayColours.clear();
      scene->storingRays = true;
      scene->pixelColour( mouse.x, windowHeight-1-mouse.y );
      scene->storingRays = false;

      return true; // no further processing of this click

    } else if (button == GLFW_MOUSE_BUTTON_RIGHT ) {

      // A right-click on a pixel sets that as the "debugging pixel".
      // When that pixel is raytraced, the 'debug' flag is set.  The
      // 'debug' flag is cleared after that one pixel is raytraced.
      // So other parts of the code can have debugging code based on
      // the status of the 'debug' flag.

      scene->debugPixel = vec2( mouse.x, windowHeight - mouse.y - 1 );

      return true;
    }

    return false;
  }

  bool userMouseMotion( vec2 mouse, int button ) {

    scene->mouse = mouse;
    return false;
  }

  bool userMouseAction( vec2 mouse, int button, int action, int mods ) {

    if (button == GLFW_MOUSE_BUTTON_MIDDLE) {
      if (action == GLFW_PRESS) {
	scene->buttonDown = button;
	scene->mouse = mouse;
      } else
	scene->buttonDown = -1;
      return true;
    }

    return false;
  }

  bool userKeyAction( int key, int scancode, int action, int mods ) {

    switch (key) {
    case GLFW_KEY_DELETE:
      scene->storedRays.clear();
      scene->storedRayColours.clear();
      break;
    case GLFW_KEY_ESCAPE:
      exit(0);
    case 'E':
      scene->outputEye();
      break;
    case '+':
    case '=':
      scene->maxDepth++;
      viewpointChanged = true;
      redisplay = true;
      break;
    case '-':
    case '_':
      if (scene->maxDepth > 1) {
	scene->maxDepth--;
	viewpointChanged = true;
	redisplay = true;
      }
      break;
    case '<':
    case ',':
      if (scene->glossyIterations > 1) {
	scene->glossyIterations--;
	viewpointChanged = true;
	redisplay = true;
      }
      break;
    case '>':
    case '.':
      scene->glossyIterations++;
      viewpointChanged = true;
      redisplay = true;
      break;
    case 'H':
      scene->showBVH = !scene->showBVH;
      redisplay = true;
      break;
    case 'O':
      scene->showObjects = !scene->showObjects;
      redisplay = true;
      break;
    case 'P':
      if (mods & GLFW_MOD_SHIFT)
	scene->numPixelSamples++;
      else {
	scene->numPixelSamples--;
	if (scene->numPixelSamples < 1)
	  scene->numPixelSamples = 1;
      }
      redisplay = true;
      cout << "pixel sampling " <<  scene->numPixelSamples << " x " <<  scene->numPixelSamples << endl;
      break;
    case '[':
      if (mods & GLFW_MOD_SHIFT) {
	if (scene->bvhDisplayDepth > 0) {
	  scene->bvhDisplayDepth--;
	  redisplay = true;
	}
      }
      break;
    case ']':
      if (mods & GLFW_MOD_SHIFT) {
	scene->bvhDisplayDepth++;
	redisplay = true;
      } 
      break;
    case 'A':
      scene->showAxes = !scene->showAxes;
      break;
    case 'J':
      scene->jitter = !scene->jitter;
      redisplay = true;
      cout << "jittering = " << scene->jitter << endl;
      break;
    case '/':
      cout
	<< endl
	<< "h     show/hide BVH" << endl
	<< "}     increase shown BVH depth" << endl
	<< "{     decrease shown BVH depth" << endl
	<< "o     show/hide objects" << endl
	<< ">     increase glossy rays" << endl
	<< "<     decrease glossy rays" << endl
	<< "+     increase ray depth" << endl
	<< "-     decrease ray depth" << endl
	<< "a     show/hide axes" << endl
	<< "e     output eye position" << endl
	<< "P     increase pixel sampling" << endl
	<< "p     decrease pixel sampling" << endl
	<< "j     toggle pixel sample jittering" << endl
	<< "SPACE toggle rotation/translation mode" << endl
	<< "DEL   delete debugging rays" << endl
	<< "ESC   exit" << endl
	<< endl
	<< "left mouse click         - show rayracing through this pixel" << endl
	<< "left mouse drag          - rotate viewpoint" << endl
	<< "middle mouse click/drag  - zoom at this pixel" << endl
	<< "right mouse drag up/down - zoom viewpoint" << endl
	<< "right mouse click        - set 'debug' flag when tracing this pixel" << endl;
      break;
    default:
      return false;  // not handled
    }

    return true; // no further processing
  }
};


#endif
