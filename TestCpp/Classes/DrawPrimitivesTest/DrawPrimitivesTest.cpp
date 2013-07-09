#include "pch.h"
#include "DrawPrimitivesTest.h"

DrawPrimitivesTest::DrawPrimitivesTest()
{
}

void DrawPrimitivesTest::draw()
{
	CCLayer::draw();

    CCSize s = CCDirector::sharedDirector()->getWinSize();
	
	// draw a simple line
	CCDrawingPrimitive::D3DColor4f(1.0, 0.0, 0.0, 1.0);
	ccDrawLine( CCPointMake(0, s.height), CCPointMake(s.width, 0) );

	
	// draw big point in the center

	CCDrawingPrimitive::D3DColor4f(0.0, 0.0, 1.0, 0.5);
	ccDrawPoint( CCPointMake(65, 65) );
	
	// draw 4 small points
	CCPoint points[] = { CCPointMake(60,60), CCPointMake(70,70), CCPointMake(60,70), CCPointMake(70,60) };
	CCDrawingPrimitive::D3DColor4f(0.0, 1.0, 1.0, 1.0);
	ccDrawPoints( points, 4);
	
	// draw a green circle with 10 segments
	CCDrawingPrimitive::D3DColor4f(0.0, 1.0, 0.0, 1.0);
	ccDrawCircle( CCPointMake(s.width/2,  s.height/2), 100, 0, 10, false);

	// draw a green circle with 50 segments with line to center
	CCDrawingPrimitive::D3DColor4f(0.0, 1.0, 1.0, 1.0);
	ccDrawCircle( CCPointMake(s.width/2, s.height/2), 50, CC_DEGREES_TO_RADIANS(90), 50, true);	
	
	// open yellow poly
	CCDrawingPrimitive::D3DColor4f(1.0, 1.0, 0.0, 1.0);
	CCPoint vertices[] = { CCPointMake(0,0), CCPointMake(50,50), CCPointMake(100,50), CCPointMake(100,100), CCPointMake(50,100) };
	ccDrawPoly( vertices, 5, false);
	
	// closed purble poly
	CCDrawingPrimitive::D3DColor4f(1.0, 0.0, 1.0, 1.0);
	CCPoint vertices2[] = { CCPointMake(30,130), CCPointMake(30,230), CCPointMake(50,200) };
	ccDrawPoly( vertices2, 3, true);
	
	// draw quad bezier path
	ccDrawQuadBezier(CCPointMake(0,s.height), CCPointMake(s.width/2,s.height/2), CCPointMake(s.width,s.height), 50);

	// draw cubic bezier path
	ccDrawCubicBezier(CCPointMake(s.width/2, s.height/2), CCPointMake(s.width/2+30,s.height/2+50), CCPointMake(s.width/2+60,s.height/2-50),CCPointMake(s.width, s.height/2),100);

	
	// restore original values
	CCDrawingPrimitive::D3DColor4f(1.0, 1.0, 1.0, 1.0);
}

void DrawPrimitivesTestScene::runThisTest()
{
    CCLayer* pLayer = new DrawPrimitivesTest();
    addChild(pLayer);
    pLayer->release();

    CCDirector::sharedDirector()->replaceScene(this);
}
