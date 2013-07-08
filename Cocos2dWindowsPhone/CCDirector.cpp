/*
* cocos2d-x   http://www.cocos2d-x.org
*
* Copyright (c) 2010-2011 - cocos2d-x community
* Copyright (c) 2010-2011 cocos2d-x.org
* Copyright (c) 2008-2010 Ricardo Quesada
* Copyright (c) 2011      Zynga Inc.
* 
* Portions Copyright (c) Microsoft Open Technologies, Inc.
* All Rights Reserved
* 
* Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. 
* You may obtain a copy of the License at 
* 
* http://www.apache.org/licenses/LICENSE-2.0 
* 
* Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an 
* "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. 
* See the License for the specific language governing permissions and limitations under the License.
*/

#include "pch.h"

#include "cocoa/CCNS.h"
#include "CCDirector.h"
#include "CCScene.h"
#include "CCArray.h"
#include "CCScheduler.h"
#include "ccMacros.h"
#include "CCTouchDispatcher.h"
#include "CCPointExtension.h"
#include "CCTransition.h"
#include "CCTextureCache.h"
//#include "CCTransition.h"
#include "CCSpriteFrameCache.h"
#include "CCAutoreleasePool.h"
#include "platform.h"
#include "CCApplication.h"
#include "CCLabelBMFont.h"
#include "CCActionManager.h"
#include "CCLabelTTF.h"
#include "CCConfiguration.h"
#include "CCKeypadDispatcher.h"
#include "CCGL.h"
#include "CCAnimationCache.h"
#include "CCTouch.h"
#if (CC_TARGET_PLATFORM != CC_PLATFORM_MARMALADE)
#include "CCUserDefault.h"
#endif
#include "CCNotificationCenter.h"

#if CC_ENABLE_PROFILERS
#include "support/CCProfiling.h"
#endif // CC_ENABLE_PROFILERS

#include <string>

#ifndef CC_DIRECTOR_STATS_POSITION
#define CC_DIRECTOR_STATS_POSITION CCDirector::sharedDirector()->getVisibleOrigin()
#endif
using namespace std;
using namespace cocos2d;
namespace  cocos2d 
{

// singleton stuff
static CCDisplayLinkDirector *s_SharedDirector= NULL;
static bool s_bFirstRun = true;

#define kDefaultFPS		60  // 60 frames per second
extern const char* cocos2dVersion(void);

CCDirector* CCDirector::sharedDirector(void)
{
    if (!s_SharedDirector)
    {
        s_SharedDirector = new CCDisplayLinkDirector();
        s_SharedDirector->init();
    }

    return s_SharedDirector;
}

bool CCDirector::init(void)
{
	CCLOG("cocos2d: %s", cocos2dVersion());

	// scenes
	m_pRunningScene = NULL;
	m_pNextScene = NULL;

	m_pNotificationNode = NULL;

	m_dOldAnimationInterval = m_dAnimationInterval = 1.0 / kDefaultFPS;	
	m_pobScenesStack = new CCArray();


	// Set default projection (3D)
	m_eProjection = kCCDirectorProjectionDefault;

	// projection delegate if "Custom" projection is used
	m_pProjectionDelegate = NULL;

	// FPS
	m_bDisplayStats = false;
	m_uTotalFrames = m_uFrames = 0;
	m_pszFPS = new char[10];
	m_pLastUpdate = new struct cc_timeval();

	// paused ?
	m_bPaused = false;
	
	// purge ?
	m_bPurgeDirecotorInNextLoop = false;

	m_obWinSizeInPixels = m_obWinSizeInPoints = CCSizeZero;

	// portrait mode default
	m_eDeviceOrientation = CCDeviceOrientationPortrait;		

	m_pobOpenGLView = NULL;
    m_bRetinaDisplay = false;
    m_fContentScaleFactor = 1;	
	m_bIsContentScaleSupported = false;

	// scheduler
	m_pScheduler = new CCScheduler();
	// action manager
	m_pActionManager = new CCActionManager();
	m_pScheduler->scheduleUpdateForTarget(m_pActionManager, kCCPrioritySystem, false);

	// touchDispatcher
    m_pTouchDispatcher = new CCTouchDispatcher();
    m_pTouchDispatcher->init();

	// KeypadDispatcher
    m_pKeypadDispatcher = new CCKeypadDispatcher();

	// Accelerometer
    m_pAccelerometer = new CCAccelerometer();

	// create autorelease pool
	CCPoolManager::sharedPoolManager()->push();

	return true;
}
	
CCDirector::~CCDirector(void)
{
	CCLOGINFO("cocos2d: deallocing %p", this);

#if CC_DIRECTOR_FAST_FPS
	CC_SAFE_RELEASE(m_pFPSLabel);
#endif 
    
	CC_SAFE_RELEASE(m_pRunningScene);
	CC_SAFE_RELEASE(m_pNotificationNode);
	CC_SAFE_RELEASE(m_pobScenesStack);
	CC_SAFE_RELEASE(m_pScheduler);
	CC_SAFE_RELEASE(m_pActionManager);
	CC_SAFE_RELEASE(m_pKeypadDispatcher);
	CC_SAFE_DELETE(m_pAccelerometer);

	// pop the autorelease pool
	CCPoolManager::sharedPoolManager()->pop();

	// delete m_pLastUpdate
	CC_SAFE_DELETE(m_pLastUpdate);

    //CCKeypadDispatcher::purgeSharedDispatcher();

	// delete fps string
	delete []m_pszFPS;
}

void CCDirector::setGLDefaultValues(void)
{
	// This method SHOULD be called only after openGLView_ was initialized
	CCAssert(m_pobOpenGLView, "opengl view should not be null");

	setAlphaBlending(true);
	setDepthTest(true);
	setProjection(m_eProjection);

	// set other opengl default values
	m_pobOpenGLView->D3DClearColor(0.0f, 0.0f, 0.0f, 1.0f);

#if CC_DIRECTOR_FAST_FPS
	if (false)//(! m_pFPSLabel)
	{
        m_pFPSLabel = CCLabelTTF::create("00.0", "Arial", 24);
		m_pFPSLabel->retain();
	}
#endif
}


// Draw the SCene
void CCDirector::drawScene(void)
{
	// calculate "global" dt
	calculateDeltaTime();

	//tick before glClear: issue #533
	if (! m_bPaused)
	{
		m_pScheduler->update(m_fDeltaTime);
	}
	
	m_pobOpenGLView->clearRender(NULL);

    /* to avoid flickr, nextScene MUST be here: after tick and before draw.
	 XXX: Which bug is this one. It seems that it can't be reproduced with v0.9 */
	if (m_pNextScene)
	{
		setNextScene();
	}
	m_pobOpenGLView->D3DPushMatrix();

	applyOrientation();
	
	// By default enable VertexArray, ColorArray, TextureCoordArray and Texture2D

	// draw the scene
    if (m_pRunningScene)
    {
        m_pRunningScene->visit();
    }

	// draw the notifications node
	if (m_pNotificationNode)
	{
		m_pNotificationNode->visit();
	}

	if (m_bDisplayStats)
	{
		showFPS();
	}

#if CC_ENABLE_PROFILERS
	showProfilers();
#endif

	//=CC_DISABLE_DEFAULT_GL_STATES();
	m_pobOpenGLView->D3DPopMatrix();

	m_uTotalFrames++;


	// render and swap buffers
	if (m_pobOpenGLView)
    {
		//m_pobOpenGLView->render();
        m_pobOpenGLView->swapBuffers();
    }
}

void CCDirector::calculateDeltaTime(void)
{
    struct cc_timeval now;

	if (CCTime::gettimeofdayCocos2d(&now, NULL) != 0)
	{
		CCLOG("error in gettimeofday");
        m_fDeltaTime = 0;
		return;
	}

	// new delta time
	if (m_bNextDeltaTimeZero)
	{
		m_fDeltaTime = 0;
		m_bNextDeltaTimeZero = false;
	}
	else
	{
		m_fDeltaTime = (now.tv_sec - m_pLastUpdate->tv_sec) + (now.tv_usec - m_pLastUpdate->tv_usec) / 1000000.0f;
		m_fDeltaTime = MAX(0, m_fDeltaTime);
	}

#ifdef DEBUG
	// If we are debugging our code, prevent big delta time
	if(m_fDeltaTime > 0.2f)
	{
		m_fDeltaTime = 1 / 60.0f;
	}
#endif

	*m_pLastUpdate = now;
}
void CCDirector::setTouchDispatcher(CCTouchDispatcher* pTouchDispatcher)
{
    if (m_pTouchDispatcher != pTouchDispatcher)
    {
        CC_SAFE_RETAIN(pTouchDispatcher);
        CC_SAFE_RELEASE(m_pTouchDispatcher);
        m_pTouchDispatcher = pTouchDispatcher;
    }    
}

CCTouchDispatcher* CCDirector::getTouchDispatcher()
{
    return m_pTouchDispatcher;
}


// m_pobOpenGLView

void CCDirector::setOpenGLView(CC_GLVIEW *pobOpenGLView)
{
	CCAssert(pobOpenGLView, "opengl view should not be null");

	if (m_pobOpenGLView != pobOpenGLView)
	{
		// because EAGLView is not kind of CCObject
		delete m_pobOpenGLView; // [openGLView_ release]
		m_pobOpenGLView = pobOpenGLView;

		// set size
		m_obWinSizeInPoints = m_pobOpenGLView->getSize();
		m_obWinSizeInPixels = CCSizeMake(m_obWinSizeInPoints.width * m_fContentScaleFactor, m_obWinSizeInPoints.height * m_fContentScaleFactor);
        setGLDefaultValues();

		if (m_fContentScaleFactor != 1)
		{
			updateContentScaleFactor();
		}

 		m_pobOpenGLView->setTouchDelegate(m_pTouchDispatcher);
        m_pTouchDispatcher->setDispatchEvents(true);
	}
}

void CCDirector::setNextDeltaTimeZero(bool bNextDeltaTimeZero)
{
	m_bNextDeltaTimeZero = bNextDeltaTimeZero;
}

void CCDirector::setProjection(ccDirectorProjection kProjection)
{
	CCSize size = m_obWinSizeInPixels;
	float zeye = this->getZEye();
	switch (kProjection)
	{
	case kCCDirectorProjection2D:
		if (m_pobOpenGLView) 
		{
			m_pobOpenGLView->setViewPortInPoints(0, 0, size.width, size.height);
		}
		m_pobOpenGLView->D3DMatrixMode(CC_PROJECTION);
		m_pobOpenGLView->D3DLoadIdentity();
		m_pobOpenGLView->D3DOrtho(0, size.width, 0, size.height, -1024 * CC_CONTENT_SCALE_FACTOR(), 
			1024 * CC_CONTENT_SCALE_FACTOR());
		m_pobOpenGLView->D3DMatrixMode(CC_MODELVIEW);
		m_pobOpenGLView->D3DLoadIdentity();
		break;

	case kCCDirectorProjection3D:
		if (m_pobOpenGLView) 
		{
			m_pobOpenGLView->setViewPortInPoints(0, 0, size.width, size.height);
		}
		m_pobOpenGLView->D3DMatrixMode(CC_PROJECTION);
		m_pobOpenGLView->D3DLoadIdentity();
		m_pobOpenGLView->D3DPerspective(60, (CCfloat)size.width/size.height, 0.5f, 1500.0f);

		m_pobOpenGLView->D3DMatrixMode(CC_MODELVIEW);	
		m_pobOpenGLView->D3DLoadIdentity();
		m_pobOpenGLView->D3DLookAt( size.width/2, size.height/2, zeye,
			size.width/2, size.height/2, 0,
			0.0f, 1.0f, 0.0f);	
		break;
			
	case kCCDirectorProjectionCustom:
		if (m_pProjectionDelegate)
		{
			m_pProjectionDelegate->updateProjection();
		}
		break;
			
	default:
		CCLOG("cocos2d: Director: unrecognized projecgtion");
		break;
	}

	m_eProjection = kProjection;
}

void CCDirector::purgeCachedData(void)
{
    CCLabelBMFont::purgeCachedData();
	CCTextureCache::sharedTextureCache()->removeUnusedTextures();
}

float CCDirector::getZEye(void)
{
    return (m_obWinSizeInPixels.height / 1.1566f);	
}

void CCDirector::setAlphaBlending(bool bOn)
{
	if (bOn)
	{
		m_pobOpenGLView->D3DBlendFunc(CC_BLEND_SRC, CC_BLEND_DST);
	}
	else
	{
		m_pobOpenGLView->D3DBlendFunc(-1, -1);
	}
}

void CCDirector::setDepthTest(bool bOn)
{
	if (bOn)
	{
		//=ccglClearDepth(1.0f);
		//=glEnable(GL_DEPTH_TEST);
		//=glDepthFunc(GL_LEQUAL);

		m_pobOpenGLView->D3DDepthFunc(CC_LEQUAL);
	}
	else
	{
		//=glDisable(GL_DEPTH_TEST);
		m_pobOpenGLView->D3DDepthFunc(-1);
	}
}

CCPoint CCDirector::convertToGL(const CCPoint& obPoint)
{
	CCSize s = m_obWinSizeInPoints;
	float newY = s.height - obPoint.y;
	float newX = s.width - obPoint.x;
	CCPoint ret = CCPointZero;
	switch (m_eDeviceOrientation)
	{
	case CCDeviceOrientationPortrait:
		ret = ccp(obPoint.x, newY);
		break;
	case CCDeviceOrientationPortraitUpsideDown:
		ret = ccp(newX, obPoint.y);
		break;
	case CCDeviceOrientationLandscapeLeft:
		ret.x = obPoint.y;
		ret.y = obPoint.x;
		break;
	case CCDeviceOrientationLandscapeRight:
		ret.x = newY;
		ret.y = newX;
		break;
	}
	
	return ret;
}

CCPoint CCDirector::convertToUI(const CCPoint& obPoint)
{
	CCSize winSize = m_obWinSizeInPoints;
	float oppositeX = winSize.width - obPoint.x;
	float oppositeY = winSize.height - obPoint.y;
	CCPoint uiPoint = CCPointZero;

	switch (m_eDeviceOrientation)
	{
	case CCDeviceOrientationPortrait:
		uiPoint = ccp(obPoint.x, oppositeY);
		break;
	case CCDeviceOrientationPortraitUpsideDown:
		uiPoint = ccp(oppositeX, obPoint.y);
		break;
	case CCDeviceOrientationLandscapeLeft:
		uiPoint = ccp(obPoint.y, obPoint.x);
		break;
	case CCDeviceOrientationLandscapeRight:
		// Can't use oppositeX/Y because x/y are flipped
		uiPoint = ccp(winSize.width - obPoint.y, winSize.height - obPoint.x);
		break;
	}

	return uiPoint;
}

CCSize CCDirector::getWinSize(void)
{
	CCSize s = m_obWinSizeInPoints;

	if (m_eDeviceOrientation == CCDeviceOrientationLandscapeLeft
		|| m_eDeviceOrientation == CCDeviceOrientationLandscapeRight)
	{
		// swap x,y in landspace mode
		CCSize tmp = s;
		s.width = tmp.height;
		s.height = tmp.width;
	}

	return s;
}

CCSize CCDirector::getWinSizeInPixels()
{
	CCSize s = getWinSize();

	s.width *= CC_CONTENT_SCALE_FACTOR();
	s.height *= CC_CONTENT_SCALE_FACTOR();

	return s;
}

// return the current frame size
CCSize CCDirector::getDisplaySizeInPixels(void)
{
	return m_obWinSizeInPixels;
}

void CCDirector::reshapeProjection(const CCSize& newWindowSize)
{
    CC_UNUSED_PARAM(newWindowSize);
    m_obWinSizeInPoints = m_pobOpenGLView->getSize();
	m_obWinSizeInPixels = CCSizeMake(m_obWinSizeInPoints.width * m_fContentScaleFactor,
		                             m_obWinSizeInPoints.height * m_fContentScaleFactor);

	setProjection(m_eProjection);
}

CCSize CCDirector::getVisibleSize()
{
    if (m_pobOpenGLView)
    {
        return m_pobOpenGLView->getSizeInPixel();
    }
    else 
    {
        return CCSizeZero;
    }
}

CCPoint CCDirector::getVisibleOrigin()
{
    //if (m_pobOpenGLView)
    //{
    //    return m_pobOpenGLView->getSizeInPixel();
    //}
    //else 
    //{
        return CCPointZero;
    //}
}

// scene management

void CCDirector::runWithScene(CCScene *pScene)
{
	CCAssert(pScene != NULL, "running scene should not be null");
	CCAssert(m_pRunningScene == NULL, "m_pRunningScene should be null");

	pushScene(pScene);
	startAnimation();
}

void CCDirector::replaceScene(CCScene *pScene)
{
	CCAssert(pScene != NULL, "the scene should not be null");

	unsigned int index = m_pobScenesStack->count();

	m_bSendCleanupToScene = true;
	m_pobScenesStack->replaceObjectAtIndex(index - 1, pScene);

	m_pNextScene = pScene;
}

void CCDirector::pushScene(CCScene *pScene)
{
	CCAssert(pScene, "the scene should not null");

	m_bSendCleanupToScene = false;

	m_pobScenesStack->addObject(pScene);
	m_pNextScene = pScene;
}

void CCDirector::popScene(void)
{
	CCAssert(m_pRunningScene != NULL, "running scene should not null");

	m_pobScenesStack->removeLastObject();
	unsigned int c = m_pobScenesStack->count();

	if (c == 0)
	{
		end();
	}
	else
	{
		m_bSendCleanupToScene = true;
		m_pNextScene = (CCScene*)m_pobScenesStack->objectAtIndex(c - 1);
	}
}
void CCDirector::popToRootScene(void)
{
    CCAssert(m_pRunningScene != NULL, "A running Scene is needed");
    unsigned int c = m_pobScenesStack->count();

    if (c == 1) 
    {
        m_pobScenesStack->removeLastObject();
        this->end();
    } 
    else 
    {
        while (c > 1) 
        {
            CCScene *current = (CCScene*)m_pobScenesStack->lastObject();
            if( current->isRunning() )
            {
                current->onExitTransitionDidStart();
                current->onExit();
            }
            current->cleanup();

            m_pobScenesStack->removeLastObject();
            c--;
        }
        m_pNextScene = (CCScene*)m_pobScenesStack->lastObject();
        m_bSendCleanupToScene = false;
    }
}

void CCDirector::end()
{
	m_bPurgeDirecotorInNextLoop = true;
}


void CCDirector::resetDirector()
{
	// don't release the event handlers
	// They are needed in case the director is run again
	m_pTouchDispatcher->removeAllDelegates();

    if (m_pRunningScene)
    {
    	m_pRunningScene->onExit();
    	m_pRunningScene->cleanup();
    	m_pRunningScene->release();
    }
    
	m_pRunningScene = NULL;
	m_pNextScene = NULL;

	// remove all objects, but don't release it.
	// runWithScene might be executed after 'end'.
	m_pobScenesStack->removeAllObjects();

	stopAnimation();

    CCObject* pProjectionDelegate = (CCObject*)m_pProjectionDelegate;
	CC_SAFE_RELEASE_NULL(pProjectionDelegate);

	// purge bitmap cache
	CCLabelBMFont::purgeCachedData();

	// purge all managers
	CCAnimationCache::purgeSharedAnimationCache();
 	CCSpriteFrameCache::purgeSharedSpriteFrameCache();
	//CCActionManager::sharedManager()->purgeSharedManager();
	//CCScheduler::purgeSharedScheduler();
	CCTextureCache::purgeSharedTextureCache();
}


void CCDirector::purgeDirector()
{
	// don't release the event handlers
	// They are needed in case the director is run again
	m_pTouchDispatcher->removeAllDelegates();

    if (m_pRunningScene)
    {
    	m_pRunningScene->onExit();
    	m_pRunningScene->cleanup();
    	m_pRunningScene->release();
    }
    
	m_pRunningScene = NULL;
	m_pNextScene = NULL;

	// remove all objects, but don't release it.
	// runWithScene might be executed after 'end'.
	m_pobScenesStack->removeAllObjects();

	stopAnimation();

#if CC_DIRECTOR_FAST_FPS
	CC_SAFE_RELEASE_NULL(m_pFPSLabel);
#endif

    CCObject* pProjectionDelegate = (CCObject*)m_pProjectionDelegate;
    CC_SAFE_RELEASE_NULL(pProjectionDelegate);

	// purge bitmap cache
	CCLabelBMFont::purgeCachedData();

	// purge all managers
	CCAnimationCache::purgeSharedAnimationCache();
 	CCSpriteFrameCache::purgeSharedSpriteFrameCache();
	//CCActionManager::sharedManager()->purgeSharedManager();
	//CCScheduler::purgeSharedScheduler();
	CCTextureCache::purgeSharedTextureCache();
	
#if (CC_TARGET_PLATFORM != CC_PLATFORM_MARMALADE)	
	CCUserDefault::purgeSharedUserDefault();
#endif
	CCNotificationCenter::purgeNotifCenter();
	// OpenGL view
	m_pobOpenGLView->release();
	m_pobOpenGLView = NULL;
}

void CCDirector::setNextScene(void)
{
	bool runningIsTransition = dynamic_cast<CCTransitionScene*>(m_pRunningScene) != NULL;
	bool newIsTransition = dynamic_cast<CCTransitionScene*>(m_pNextScene) != NULL;

	// If it is not a transition, call onExit/cleanup
 	if (! newIsTransition)
 	{
         if (m_pRunningScene)
         {
             m_pRunningScene->onExit();
         }
 
 		// issue #709. the root node (scene) should receive the cleanup message too
 		// otherwise it might be leaked.
 		if (m_bSendCleanupToScene && m_pRunningScene)
 		{
 			m_pRunningScene->cleanup();
 		}
 	}

    if (m_pRunningScene)
    {
        m_pRunningScene->release();
    }
    m_pRunningScene = m_pNextScene;
	m_pNextScene->retain();
	m_pNextScene = NULL;

	if ((! runningIsTransition) && m_pRunningScene)
	{
		m_pRunningScene->onEnter();
		m_pRunningScene->onEnterTransitionDidFinish();
	}
}

void CCDirector::pause(void)
{
	if (m_bPaused)
	{
		return;
	}

	m_dOldAnimationInterval = m_dAnimationInterval;

	// when paused, don't consume CPU
	setAnimationInterval(1 / 4.0);
	m_bPaused = true;
}

void CCDirector::resume(void)
{
	if (! m_bPaused)
	{
		return;
	}

	setAnimationInterval(m_dOldAnimationInterval);

	if (CCTime::gettimeofdayCocos2d(m_pLastUpdate, NULL) != 0)
	{
		CCLOG("cocos2d: Director: Error in gettimeofday");
	}

	m_bPaused = false;
	m_fDeltaTime = 0;
}

#if CC_DIRECTOR_FAST_FPS
// display the FPS using a LabelAtlas
// updates the FPS every frame
void CCDirector::showFPS(void)
{
	m_uFrames++;
	m_fAccumDt += m_fDeltaTime;

	if (m_fAccumDt > CC_DIRECTOR_FPS_INTERVAL)
	{
		m_fFrameRate = m_uFrames / m_fAccumDt;
		m_uFrames = 0;
		m_fAccumDt = 0;

		sprintf(m_pszFPS, "%.1f", m_fFrameRate);
		m_pFPSLabel->setString(m_pszFPS);
	}

    m_pFPSLabel->draw();
}
#endif // CC_DIRECTOR_FAST_FPS


void CCDirector::showProfilers()
{
#if CC_ENABLE_PROFILERS
	m_fAccumDtForProfiler += m_fDeltaTime;
	if (m_fAccumDtForProfiler > 1.0f)
	{
		m_fAccumDtForProfiler = 0;
		CCProfiler::sharedProfiler()->displayTimers();
	}
#endif
}

/***************************************************
* mobile platforms specific functions
**************************************************/

void CCDirector::updateContentScaleFactor()
{
	// [openGLView responseToSelector:@selector(setContentScaleFactor)]
	if (m_pobOpenGLView->canSetContentScaleFactor())
	{
		m_pobOpenGLView->setContentScaleFactor(m_fContentScaleFactor);
		m_bIsContentScaleSupported = true;
	}
	else
	{
		CCLOG("cocos2d: setContentScaleFactor:'is not supported on this device");
	}
}


bool CCDirector::setDirectorType(ccDirectorType obDirectorType)
{
    CC_UNUSED_PARAM(obDirectorType);
	// we only support CCDisplayLinkDirector
	CCDirector::sharedDirector();

	return true;
}

bool CCDirector::enableRetinaDisplay(bool enabled)
{
	// Already enabled?
	if (enabled && m_fContentScaleFactor == 2)
	{
		return true;
	}

	// Already diabled?
	if (!enabled && m_fContentScaleFactor == 1)
	{
		return false;
	}

	// setContentScaleFactor is not supported
	if (! m_pobOpenGLView->canSetContentScaleFactor())
	{
		return false;
	}

	float newScale = (float)(enabled ? 2 : 1);
	setContentScaleFactor(newScale);

    // release cached texture
    CCTextureCache::purgeSharedTextureCache();

#if CC_DIRECTOR_FAST_FPS
    if (m_pFPSLabel)
    {
        CC_SAFE_RELEASE_NULL(m_pFPSLabel);
        m_pFPSLabel = CCLabelTTF::create("00.0", "Arial", 24);
        m_pFPSLabel->retain();
    }
#endif

    if (m_fContentScaleFactor == 2)
    {
        m_bRetinaDisplay = true;
    }
    else
    {
        m_bRetinaDisplay = false;
    }

	return true;
}

void CCDirector::setScheduler(CCScheduler* pScheduler)
{
    if (m_pScheduler != pScheduler)
    {
        CC_SAFE_RETAIN(pScheduler);
        CC_SAFE_RELEASE(m_pScheduler);
        m_pScheduler = pScheduler;
    }
}


CCScheduler* CCDirector::getScheduler()
{
    return m_pScheduler;
}

void CCDirector::setActionManager(CCActionManager* pActionManager)
{
    if (m_pActionManager != pActionManager)
    {
        CC_SAFE_RETAIN(pActionManager);
        CC_SAFE_RELEASE(m_pActionManager);
        m_pActionManager = pActionManager;
    }    
}

CCActionManager* CCDirector::getActionManager()
{
    return m_pActionManager;
}

void CCDirector::setKeypadDispatcher(CCKeypadDispatcher* pKeypadDispatcher)
{
    CC_SAFE_RETAIN(pKeypadDispatcher);
    CC_SAFE_RELEASE(m_pKeypadDispatcher);
    m_pKeypadDispatcher = pKeypadDispatcher;
}

CCKeypadDispatcher* CCDirector::getKeypadDispatcher()
{
    return m_pKeypadDispatcher;
}

void CCDirector::setAccelerometer(CCAccelerometer* pAccelerometer)
{
    if (m_pAccelerometer != pAccelerometer)
    {
        CC_SAFE_DELETE(m_pAccelerometer);
        m_pAccelerometer = pAccelerometer;
    }
}

CCAccelerometer* CCDirector::getAccelerometer()
{
    return m_pAccelerometer;
}

void CCDirector::createStatsLabel()
{
    if( m_pFPSLabel && m_pSPFLabel ) 
    {
        CC_SAFE_RELEASE_NULL(m_pFPSLabel);
        CC_SAFE_RELEASE_NULL(m_pSPFLabel);
        CC_SAFE_RELEASE_NULL(m_pDrawsLabel);

        CCFileUtils::sharedFileUtils()->purgeCachedEntries();
    }

    int fontSize = 0;
    if (m_obWinSizeInPoints.width > m_obWinSizeInPoints.height)
    {
        fontSize = (int)(m_obWinSizeInPoints.height / 320.0f * 24);
    }
    else
    {
        fontSize = (int)(m_obWinSizeInPoints.width / 320.0f * 24);
    }
    
    m_pFPSLabel = CCLabelTTF::create("00.0", "Arial", fontSize);
    m_pFPSLabel->retain();
    m_pSPFLabel = CCLabelTTF::create("0.000", "Arial", fontSize);
    m_pSPFLabel->retain();
    m_pDrawsLabel = CCLabelTTF::create("000", "Arial", fontSize);
    m_pDrawsLabel->retain();

    CCSize contentSize = m_pDrawsLabel->getContentSize();
    m_pDrawsLabel->setPosition(ccpAdd(ccp(contentSize.width/2, contentSize.height*5/2), CC_DIRECTOR_STATS_POSITION));
    contentSize = m_pSPFLabel->getContentSize();
    m_pSPFLabel->setPosition(ccpAdd(ccp(contentSize.width/2, contentSize.height*3/2), CC_DIRECTOR_STATS_POSITION));
    contentSize = m_pFPSLabel->getContentSize();
    m_pFPSLabel->setPosition(ccpAdd(ccp(contentSize.width/2, contentSize.height/2), CC_DIRECTOR_STATS_POSITION));
}



CGFloat CCDirector::getContentScaleFactor(void)
{
	return m_fContentScaleFactor;
}

void CCDirector::setContentScaleFactor(CGFloat scaleFactor)
{
	if (scaleFactor != m_fContentScaleFactor)
	{
		m_fContentScaleFactor = scaleFactor;
		m_obWinSizeInPixels = CCSizeMake(m_obWinSizeInPoints.width * scaleFactor, m_obWinSizeInPoints.height * scaleFactor);

		if (m_pobOpenGLView)
		{
			updateContentScaleFactor();
		}

		// update projection
		setProjection(m_eProjection);
	}
}

CCNode* CCDirector::getNotificationNode() 
{ 
	return m_pNotificationNode; 
}

void CCDirector::setNotificationNode(CCNode *node)
{
	CC_SAFE_RELEASE(m_pNotificationNode);
	m_pNotificationNode = node;
	CC_SAFE_RETAIN(m_pNotificationNode);
}

void CCDirector::applyOrientation(void)
{
	CCSize s = m_obWinSizeInPixels;
	float w = s.width / 2;
	float h = s.height / 2;

	// XXX it's using hardcoded values.
	// What if the the screen size changes in the future?
	switch (m_eDeviceOrientation)
	{
	case CCDeviceOrientationPortrait:
		// nothing
		break;
	case CCDeviceOrientationPortraitUpsideDown:
		m_pobOpenGLView->D3DTranslate(w,h,0);
		m_pobOpenGLView->D3DRotate(CC_DEGREES_TO_RADIANS(180),0,0,1);
		m_pobOpenGLView->D3DTranslate(-w,-h,0);
		break;
	case CCDeviceOrientationLandscapeRight:
		m_pobOpenGLView->D3DTranslate(w,h,0);
		m_pobOpenGLView->D3DRotate(CC_DEGREES_TO_RADIANS(90),0,0,1);
		m_pobOpenGLView->D3DTranslate(-h,-w,0);
		break;
	case CCDeviceOrientationLandscapeLeft:
		m_pobOpenGLView->D3DTranslate(w,h,0);
		m_pobOpenGLView->D3DRotate(CC_DEGREES_TO_RADIANS(-90),0,0,1);
		m_pobOpenGLView->D3DTranslate(-h,-w,0);
		break;
	}
}

ccDeviceOrientation CCDirector::getDeviceOrientation(void)
{
	return m_eDeviceOrientation;
}

void CCDirector::setDeviceOrientation(ccDeviceOrientation kDeviceOrientation)
{
	ccDeviceOrientation eNewOrientation;

	eNewOrientation = (ccDeviceOrientation)CCApplication::sharedApplication()->setOrientation(
        (CCApplication::Orientation)kDeviceOrientation);

	if (m_eDeviceOrientation != eNewOrientation)
	{
		m_eDeviceOrientation = eNewOrientation;
	}
    else
    {
        // this logic is only run on win32 now
        // On win32,the return value of CCApplication::setDeviceOrientation is always kCCDeviceOrientationPortrait
        // So,we should calculate the Projection and window size again.
        m_obWinSizeInPoints = m_pobOpenGLView->getSize();
        m_obWinSizeInPixels = CCSizeMake(m_obWinSizeInPoints.width * m_fContentScaleFactor, m_obWinSizeInPoints.height * m_fContentScaleFactor);
        setProjection(m_eProjection);
    }
}


/***************************************************
* implementation of DisplayLinkDirector
**************************************************/

// should we afford 4 types of director ??
// I think DisplayLinkDirector is enough
// so we now only support DisplayLinkDirector
void CCDisplayLinkDirector::startAnimation(void)
{
	if (CCTime::gettimeofdayCocos2d(m_pLastUpdate, NULL) != 0)
	{
		CCLOG("cocos2d: DisplayLinkDirector: Error on gettimeofday");
	}

	m_bInvalid = false;
	CCApplication::sharedApplication()->setAnimationInterval(m_dAnimationInterval);
}

void CCDisplayLinkDirector::mainLoop(void)
{
	if (m_bPurgeDirecotorInNextLoop)
	{
		purgeDirector();
        m_bPurgeDirecotorInNextLoop = false;
	}
	else if (! m_bInvalid)
 	{
		m_pobOpenGLView->SetBackBufferRenderTarget();
 		drawScene();
	 
 		// release the objects
 		CCPoolManager::sharedPoolManager()->pop();		
 	}
}

void CCDisplayLinkDirector::stopAnimation(void)
{
	m_bInvalid = true;
}

void CCDisplayLinkDirector::setAnimationInterval(double dValue)
{
	m_dAnimationInterval = dValue;
	if (! m_bInvalid)
	{
		stopAnimation();
		startAnimation();
	}	
}

} //namespace   cocos2d 
