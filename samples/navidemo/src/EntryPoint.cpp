#include "NaviDemo.h"
#include <windows.h>

int main()
{
	try
	{
		NaviDemo demo;

		while(!demo.shouldQuit)
			demo.Update();
	}
	catch(Ogre::Exception& e)
	{
		ShowCursor(true);
		MessageBox(NULL, e.getFullDescription().c_str(), "An exception has occured!", MB_OK | MB_ICONERROR | MB_TASKMODAL);
	}

	return 0;
}