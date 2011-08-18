## How Do I Use This?

This library makes it easy to interact with web-pages in Ogre3D. You can either create an on-screen overlay (NaviManager::createNavi) or create a pure material that you can wrap to any mesh (NaviManager::createNaviMaterial).

A full sample application (NaviDemo) is included that demonstrates usage of both to create a full 3D browser. The entire UI is rendered using HTML.

## Dependencies

This version depends on Awesomium 1.6.2, Ogre 1.7.2, and Visual Studio 2008.

1. You will need to install the Ogre3D SDK (make sure it registers the OGRE_HOME environment path).
2. You will also need to download and unzip the Awesomium SDK to the following path:

    dependencies/win/awesomium_v1.6.2_sdk_win
	
You can download Awesomium from http://www.awesomium.com/download

After you got the dependencies installed, just open up Navi.sln and build the solution.

## Licensing

This wrapper is LGPL. Its main dependency, Awesomium, is free for evaluation, non-commercial use, and independent use (by companies who made less than $100K in revenue last year).

If you're a larger company and would like to use Awesomium for commercial use, you can purchase a Pro License at http://awesomium.com/buy/
	
