#ifndef	INCLUDED_SCENE3D_HXX
#define	INCLUDED_SCENE3D_HXX

#ifdef _WIN32
 #include <windows.h>
#elif !	defined	(LINUX)
 #include <Carbon.h>
 #include <Movies.h>
#endif

#include "juce.h"

#ifndef JUCE_OPENGL
#error Juce OpenGL is not enabled.
#endif

#ifdef _WIN32
 #include <gl/gl.h>
 #include <gl/glu.h>
 #ifdef	_MSC_VER
  #pragma comment(lib, "OpenGL32.Lib")
  #pragma comment(lib, "GlU32.Lib")
 #endif
#elif defined (LINUX)
 #include <GL/gl.h>
 #include <GL/glut.h>
 #undef	KeyPress
#else
 #include <AGL/agl.h>
 #include <GLUT/glut.h>
#endif

#include "Camera.hxx"
#include "LockFreeFifo.hxx"
#include "ComputeDescriptors.hxx"
#include "ViolinModel3d.hxx"

#include "ViolinRecordingPlugInConfig.hxx"
#if (DISABLE_TIMERS != 0)
#include "DummyTimer.hxx"
#define Timer DummyTimer
#endif

// ---------------------------------------------------------------------------------------

class Scene3d : public OpenGLComponent, public Timer
{
public:
	Scene3d(String name);
	~Scene3d();

	void start(int updateIntervalMilliseconds, float productionRate, float toleranceFactor);
	void stop();

	void newOpenGLContextCreated();
	void renderOpenGL();

	void resetCamera(bool redraw = true);
	void mouseDown(const MouseEvent &e);
	void mouseDrag(const MouseEvent &e);
	void mouseDoubleClick(const MouseEvent &e);

	void timerCallback();

	void setViolinModel3d(const ViolinModel3d &model)
	{
//		ScopedLock scopedLock(violinModel3dLock_);
		violinModel3d_ = model;
	}

	void postData(const RawSensorData &rawSensorData);
	void postData(const RawSensorData &rawSensorData, const Derived3dData &derived3dData, const ViolinPerformanceDescriptors &descriptors, bool isCalibratingForce);

	Camera &getCamera() { return camera_; }
	const Camera &getCamera() const { return camera_; }
	void setCamera(const Camera &camera) { camera_ = camera; } // not thread safe

private:
	Camera camera_;

	struct DescriptorFrame
	{
		RawSensorData rawSensorData;
		Derived3dData derived3dData;
		ViolinPerformanceDescriptors descriptors;
		bool areDerived3dDataAndDescriptorsValid;
		bool isCalibratingForce;
	};

	CriticalSection startStopLock_;

	LockFreeFifo<DescriptorFrame> dataBuffer_;
	DescriptorFrame curFrame_;
	bool curFrameValid_;
	float productionRate_;
	float consumptionRate_;

	ViolinModel3d violinModel3d_;

	LockFreeFifo<DescriptorFrame> motionTrailBuffer_;
	int maxMotionTrailLength_;
	int numMotionTrailFrames_;

	void drawCylinder(const float *p1, const float *p2, float radius);
	void drawBox(float w, float h, float d);
	void drawCube(float radius);
	void drawAxes(float length);
	void drawSensor(float pointSize, float axesSize, const Matrix3x1 &pos, const Matrix3x1 &att);
	void drawStylusSensor(float pointSize, const Matrix3x1 &pos, const Matrix3x1 &att);
	void drawFloor(float tileDimensions, int numTiles);
	void drawViolinBody();
};

#endif