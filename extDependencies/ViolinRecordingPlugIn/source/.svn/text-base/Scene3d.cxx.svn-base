#include "Scene3d.hxx"

#include "concat/Utilities/Logging.hxx"
#include "concat/Utilities/FloatToInt.hxx"
#include "Exceptions.hxx"

// ---------------------------------------------------------------------------------------

Scene3d::Scene3d(String name)
{
	maxMotionTrailLength_ = 0;

	setName(name);

	// Starting camera position/orientation:
	resetCamera(false);
	camera_.setMouseMode(Camera::PITCHYAW);

	curFrameValid_ = false; // Scene3d might draw (from WM_PAINT) before timer is started
}

Scene3d::~Scene3d()
{
	stop();
}

// ---------------------------------------------------------------------------------------

void Scene3d::start(int updateIntervalMilliseconds, float productionRate, float toleranceFactor)
{
	const ScopedLock scopedFileLock(startStopLock_);

	if (isTimerRunning())
		return;

	curFrame_.rawSensorData.violinBodySensPos = Matrix3x1(0.0, 0.0, 0.0);
	curFrame_.rawSensorData.violinBodySensOrientation = Matrix3x1(0.0, 0.0, 0.0);
	curFrame_.rawSensorData.bowSensPos = Matrix3x1(0.0, 0.0, 0.0);
	curFrame_.rawSensorData.bowSensOrientation = Matrix3x1(0.0, 0.0, 0.0);
	curFrame_.rawSensorData.stylusSensPos = Matrix3x1(0.0, 0.0, 0.0);
	curFrame_.rawSensorData.stylusSensOrientation = Matrix3x1(0.0, 0.0, 0.0);
	curFrameValid_ = false;

	productionRate_ = productionRate;
	consumptionRate_ = 1000.0f/(float)(updateIntervalMilliseconds);
	const int bufferSize = concat::ceil_int(productionRate_/consumptionRate_*toleranceFactor);
	dataBuffer_.reserve(bufferSize);

	const double motionTrailDuration = 1.5; // 1.5 seconds
	maxMotionTrailLength_ = concat::round_int(motionTrailDuration*1000.0/(double)(updateIntervalMilliseconds));
	motionTrailBuffer_.reserve(maxMotionTrailLength_+8); // some head room to avoid get() with readidx == writeidx
	numMotionTrailFrames_ = 0;

	startTimer(updateIntervalMilliseconds);
}

void Scene3d::stop()
{
	const ScopedLock scopedFileLock(startStopLock_);

	if (!isTimerRunning())
		return;

	stopTimer();

	dataBuffer_.reserve(0);
	motionTrailBuffer_.reserve(0);

	// Repaint at default position:
	curFrame_.rawSensorData.violinBodySensPos = Matrix3x1(0.0, 0.0, 0.0);
	curFrame_.rawSensorData.violinBodySensOrientation = Matrix3x1(0.0, 0.0, 0.0);
	curFrame_.rawSensorData.bowSensPos = Matrix3x1(0.0, 0.0, 0.0);
	curFrame_.rawSensorData.bowSensOrientation = Matrix3x1(0.0, 0.0, 0.0);
	curFrame_.rawSensorData.stylusSensPos = Matrix3x1(0.0, 0.0, 0.0);
	curFrame_.rawSensorData.stylusSensOrientation = Matrix3x1(0.0, 0.0, 0.0);

	curFrameValid_ = false;
	maxMotionTrailLength_ = 0;
	numMotionTrailFrames_ = 0;

	repaint();
}

// ---------------------------------------------------------------------------------------

// when	the	component creates a	new	internal context, this is called, and
// we'll use the opportunity to	create the textures	needed.
void Scene3d::newOpenGLContextCreated()
{
	// Enable depth test:
	glDepthFunc(GL_LESS);
	glEnable(GL_DEPTH_TEST);

	// Make objects two-sided (disable culling backfacing polygons):
	glCullFace(GL_BACK);
	glDisable(GL_CULL_FACE);

//	glEnable(GL_TEXTURE_2D);
//	glEnable(GL_BLEND);
	glShadeModel(GL_SMOOTH);

	// Enable lighting:
	glEnable(GL_LIGHTING);

	GLfloat ambient[] = {0.5f, 0.5f, 0.5f, 1.0f};
	GLfloat brightWhite[] = {1.f, 1.f, 1.f, 1.0f};

	GLfloat posLight0[] = {70.0f, 50.0f, 80.0f, 0.f};//{1.f, 0.5f, 1.2f, 0.f};

	glEnable(GL_LIGHT0);
	glLightfv(GL_LIGHT0, GL_POSITION, posLight0);
	glLightfv(GL_LIGHT0, GL_DIFFUSE, brightWhite);
	glLightfv(GL_LIGHT0, GL_AMBIENT, ambient);

	// Quality hints:
	glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
	glHint(GL_POINT_SMOOTH_HINT, GL_NICEST);
}

// ---------------------------------------------------------------------------------------

// Utility function:
void glTranslatefv(const GLfloat *v)
{
	glTranslatef(v[0], v[1], v[2]);
}

// Utility class for using Matrix3x1 with OpenGL.
//
// OpenGL coordinate system (after glLoadIdentity()):
//      +y |  / -z
//         | /
//         |/
// -x -----/----- +x  (center = [0, 0, 0])
//        /|
//       / |
//   +z /  | -y
//
//
// Polhemus coordinate system:
//      -z |  / -y
//         | /
//         |/
// -x -----/----- +x  (center = [0, 0, 0])
//        /|
//       / |
//   +y /  | +z
//
// Thus converting from Polhemus to OpenGL coordinates 
// simply means inverting z and swapping z and y.
template<typename T>
class PolhemusPointToOpenGlVector
{
public:
	explicit PolhemusPointToOpenGlVector(const Matrix3x1 &polhemusPoint)
	{
		v_[0] = (T)polhemusPoint(0, 0); // x
		v_[1] = -(T)polhemusPoint(2, 0); // -z
		v_[2] = (T)polhemusPoint(1, 0); // y
	}

	const T *get() const
	{
		return v_;
	}

private:
	T v_[3];
};

typedef PolhemusPointToOpenGlVector<float> Pol2OGlf;

// Utility class for using Matrix4x4 (which is row-major) with OpenGL glMatrixMult() which is column-major.
// T should be float for glMatrixMultf() or double for glMatrixMultd().
template<typename T>
class Matrix4x4ToColumnMajorVector
{
public:
	Matrix4x4ToColumnMajorVector(const Matrix4x4 &m)
	{
		for (int r = 0; r < 4; ++r)
		{
			for (int c = 0; c < 4; ++c)
			{
				v_[r*4 + c] = m(c, r);
			}
		}
	}

	const T *get() const
	{
		return v_;
	}

private:
	T v_[16];
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

void setOrthographicProjection(int w, int h)
{
	// switch to projection mode
	glMatrixMode(GL_PROJECTION);
	// save previous matrix which contains the 
	// settings for the perspective projection
	glPushMatrix();
	// reset matrix
	glLoadIdentity();
	// set a 2D orthographic projection
	gluOrtho2D(0, w, 0, h);
	// invert the y axis, down is positive
	glScalef(1, -1, 1);
	// move the origin from the bottom left corner
	// to the upper left corner
	glTranslatef(0.f, (float)-h, 0.f);
	glMatrixMode(GL_MODELVIEW);
}

void resetPerspectiveProjection()
{
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
}

#include <GL/glut.h>

int getBitmapStringWidth(void *font, const char *string)
{
	int width = 0;
	const int len = (int)strlen(string);
	for (int i = 0; i < len; i++)
	{
		width += glutBitmapWidth(font, (unsigned char)string[i]);
	}

	return width;
}

int getBitmapStringHeight(void *font)
{
	if (font == GLUT_BITMAP_8_BY_13)
	{
		return 13;
	}
	else if (font == GLUT_BITMAP_9_BY_15)
	{
		return 15;
	}
	else if (font == GLUT_BITMAP_TIMES_ROMAN_10)
	{
		return 10;
	}
	else if (font == GLUT_BITMAP_TIMES_ROMAN_24)
	{
		return 24;
	}
	else if (font == GLUT_BITMAP_HELVETICA_10)
	{
		return 10;
	}
	else if (font == GLUT_BITMAP_HELVETICA_12)
	{
		return 12;
	}
	else if (font == GLUT_BITMAP_HELVETICA_18)
	{
		return 18;
	}
	else
	{
		return 0;
	}
}

// font must be one of the following values:
// GLUT_BITMAP_8_BY_13 
// GLUT_BITMAP_9_BY_15 
// GLUT_BITMAP_TIMES_ROMAN_10 
// GLUT_BITMAP_TIMES_ROMAN_24 
// GLUT_BITMAP_HELVETICA_10 
// GLUT_BITMAP_HELVETICA_12 
// GLUT_BITMAP_HELVETICA_18
void renderBitmapString(int left, int top, void *font, const char *string)
{
	if (string == NULL || string[0] == '\0')
		return;

	int height = getBitmapStringHeight(font);
	if (height == 0)
		return;

	glRasterPos2f((float)left, (float)top + height/2.0f);

	const int len = (int)strlen(string);
	for (int i = 0; i < len; i++)
	{
		glutBitmapCharacter(font, (unsigned char)string[i]);
		// Note: glutBitmapCharacter() renders center-left of character at current raster pos
		// Note: glutBitmapCharacter() also advances raster pos x by width of character
		// Note: need to cast to unsigned char to allow extended ASCII characters (e.g. ñ, á, é, ó, í, ú, etc.)
	}
}

// ---------------------------------------------------------------------------------------

void Scene3d::renderOpenGL()
{
	const ScopedLock scopedFileLock(startStopLock_);

	// Clear:
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// Projection mode:
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	// Set up the camera perspective:
	const int width = getWidth();
	const int height = getHeight();
	const GLdouble aspectRatio = GLdouble(width) / GLdouble(height);
	const GLdouble verticalFieldOfView = 25.0;
	const GLdouble nearClipping = 1.0; // 1 cm
	const GLdouble farClipping = 1000.0; // 10 m
	gluPerspective(verticalFieldOfView, aspectRatio, nearClipping, farClipping);

	// XXX: (standard parameters were here)

	// Model view mode:
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	// Set the camera location/orientation:
	camera_.gluLookAt();

	// XXX: (lighting was here)

	// Get last frame posted:
	int availData = dataBuffer_.getReadAvail();

	for (int i = 0; i < availData; ++i)
	{
		dataBuffer_.get(curFrame_);
		curFrameValid_ = true;
	}

	// Put frame in motion trail buffer:
	if (availData > 0)
	{
		motionTrailBuffer_.put(curFrame_, true);
		++numMotionTrailFrames_;
		if (numMotionTrailFrames_ > maxMotionTrailLength_)
			numMotionTrailFrames_ = maxMotionTrailLength_;
	}
	else
	{
		--numMotionTrailFrames_;
		if (numMotionTrailFrames_ < 0)
			numMotionTrailFrames_ = 0;
	}

	// ...:
	glEnable(GL_LINE_SMOOTH);
	glEnable(GL_POINT_SMOOTH);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glHint(GL_LINE_SMOOTH_HINT, GL_DONT_CARE);
	glLineWidth(1.5);

	// Colors:
	GLfloat white[] = {0.95f, 0.95f, 0.95f, 1.f};
	GLfloat gray[] = {0.7f, 0.7f, 0.7f, 1.f};
	GLfloat red[] = {0.9f, 0.1f, 0.1f, 1.f};
	GLfloat darkerRed[] = {0.7f, 0.05f, 0.05f, 1.f};
	GLfloat green[] = {0.1f, 0.9f, 0.1f, 1.f};
	GLfloat darkerGreen[] = {0.15f, 0.5f, 0.05f, 1.f};
	GLfloat yellow[] = {0.9f, 0.9f, 0.05f, 1.f};
	GLfloat lighterYellow[] = {0.985f, 0.985f, 0.5f, 1.f};

	// Draw floor:
	const GLfloat heightSource = 90.0f;
	glPushMatrix();
		glTranslatef(0.0f, -heightSource, 0.0f); // x, -z, y
		drawFloor(20.0f, 20);
	glPopMatrix();

	// Draw source:
	glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, gray);

	glPushMatrix();
		drawCube(5.0f); // 10x10x10 cm
		drawAxes(20.0f);
	glPopMatrix();

	if (curFrameValid_)
	{
		// Draw sensor 1 (body sensor):
		glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, darkerRed);
		drawSensor(5.0f, 4.0f, curFrame_.rawSensorData.violinBodySensPos, curFrame_.rawSensorData.violinBodySensOrientation);

		// Draw sensor 2 (bow sensor):
		glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, darkerGreen);
		drawSensor(5.0f, 4.0f, curFrame_.rawSensorData.bowSensPos, curFrame_.rawSensorData.bowSensOrientation);

		// Draw sensor 3 (stylus):
		glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, yellow);
		drawStylusSensor(4.0f, curFrame_.rawSensorData.stylusSensPos, curFrame_.rawSensorData.stylusSensOrientation);
	}

	if (curFrameValid_ && curFrame_.areDerived3dDataAndDescriptorsValid)
	{
		// Draw bow hair ribbon:
		Pol2OGlf frogLhs = Pol2OGlf(curFrame_.derived3dData.posBowFrogLhs);
		Pol2OGlf frogRhs = Pol2OGlf(curFrame_.derived3dData.posBowFrogRhs);
		Pol2OGlf tipLhs = Pol2OGlf(curFrame_.derived3dData.posBowTipLhs);
		Pol2OGlf tipRhs = Pol2OGlf(curFrame_.derived3dData.posBowTipRhs);

		glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, green);
		glPushMatrix();
			glBegin(GL_LINES);
				glVertex3fv(frogLhs.get());
				glVertex3fv(frogRhs.get());

				glVertex3fv(frogRhs.get());
				glVertex3fv(tipRhs.get());

				glVertex3fv(tipRhs.get());
				glVertex3fv(tipLhs.get());

				glVertex3fv(tipLhs.get());
				glVertex3fv(frogLhs.get());
			glEnd();
		glPopMatrix();

		// Draw bridge vectors:
		glPushMatrix();
			glBegin(GL_LINES);
				const double bridgeVecLength = 5.0;
				Pol2OGlf v_br_0 = Pol2OGlf(curFrame_.derived3dData.posBridgeMiddle);
				Pol2OGlf v_br_up = Pol2OGlf(curFrame_.derived3dData.posBridgeMiddle + bridgeVecLength*curFrame_.derived3dData.vecBridgeUp);
				Pol2OGlf v_br_left = Pol2OGlf(curFrame_.derived3dData.posBridgeMiddle + bridgeVecLength*curFrame_.derived3dData.vecBridgeLeft);
				Pol2OGlf v_br_fwd = Pol2OGlf(curFrame_.derived3dData.posBridgeMiddle + bridgeVecLength*curFrame_.derived3dData.vecBridgeFwd);

				glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, white);
				glVertex3fv(v_br_0.get());
				glVertex3fv(v_br_up.get());

				glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, yellow);
				glVertex3fv(v_br_0.get());
				glVertex3fv(v_br_left.get());

				glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, green);
				glVertex3fv(v_br_0.get());
				glVertex3fv(v_br_fwd.get());
			glEnd();
		glPopMatrix();

		// Draw frog vectors:
		glPushMatrix();
			glBegin(GL_LINES);
				const double frogVecLength = 5.0;
				Pol2OGlf v_fr_0 = Pol2OGlf(curFrame_.derived3dData.posBowFrogLhs);
				Pol2OGlf v_fr_up = Pol2OGlf(curFrame_.derived3dData.posBowFrogLhs + frogVecLength*curFrame_.derived3dData.vecFrogUp);
				Pol2OGlf v_fr_left = Pol2OGlf(curFrame_.derived3dData.posBowFrogLhs + frogVecLength*curFrame_.derived3dData.vecFrogLeft);
				Pol2OGlf v_fr_fwd = Pol2OGlf(curFrame_.derived3dData.posBowFrogLhs + frogVecLength*curFrame_.derived3dData.vecFrogFwd);

				glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, white);
				glVertex3fv(v_fr_0.get());
				glVertex3fv(v_fr_up.get());

				glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, yellow);
				glVertex3fv(v_fr_0.get());
				glVertex3fv(v_fr_left.get());

				glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, green);
				glVertex3fv(v_fr_0.get());
				glVertex3fv(v_fr_fwd.get());
			glEnd();
		glPopMatrix();

		if (!curFrame_.isCalibratingForce)
		{
			// Draw strings:
			Pol2OGlf posStr1Bridge = Pol2OGlf(curFrame_.derived3dData.posStr1Bridge);
			Pol2OGlf posStr2Bridge = Pol2OGlf(curFrame_.derived3dData.posStr2Bridge);
			Pol2OGlf posStr3Bridge = Pol2OGlf(curFrame_.derived3dData.posStr3Bridge);
			Pol2OGlf posStr4Bridge = Pol2OGlf(curFrame_.derived3dData.posStr4Bridge);
			Pol2OGlf posStr1Wood = Pol2OGlf(curFrame_.derived3dData.posStr1Wood);
			Pol2OGlf posStr2Wood = Pol2OGlf(curFrame_.derived3dData.posStr2Wood);
			Pol2OGlf posStr3Wood = Pol2OGlf(curFrame_.derived3dData.posStr3Wood);
			Pol2OGlf posStr4Wood = Pol2OGlf(curFrame_.derived3dData.posStr4Wood);
			Pol2OGlf posStr1Fb = Pol2OGlf(curFrame_.derived3dData.posStr1Fb);
			Pol2OGlf posStr2Fb = Pol2OGlf(curFrame_.derived3dData.posStr2Fb);
			Pol2OGlf posStr3Fb = Pol2OGlf(curFrame_.derived3dData.posStr3Fb);
			Pol2OGlf posStr4Fb = Pol2OGlf(curFrame_.derived3dData.posStr4Fb);

			glPushMatrix();
				glBegin(GL_LINES);
					// String 1:
					if (curFrame_.derived3dData.playedString == 1)
						glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, yellow);
					else
						glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, red);
					glVertex3fv(posStr1Bridge.get());
					glVertex3fv(posStr1Fb.get());

					// String 2:
					if (curFrame_.derived3dData.playedString == 2)
						glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, yellow);
					else
						glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, red);
					glVertex3fv(posStr2Bridge.get());
					glVertex3fv(posStr2Fb.get());

					// String 3:
					if (curFrame_.derived3dData.playedString == 3)
						glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, yellow);
					else
						glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, red);
					glVertex3fv(posStr3Bridge.get());
					glVertex3fv(posStr3Fb.get());
					
					// String 4:
					if (curFrame_.derived3dData.playedString == 4)
						glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, yellow);
					else
						glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, red);
					glVertex3fv(posStr4Bridge.get());
					glVertex3fv(posStr4Fb.get());
				glEnd();
			glPopMatrix();

			// Draw wood points:
			glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, yellow);
			glPointSize(3.0);
			glPushMatrix();
				glBegin(GL_POINTS);
					glVertex3fv(posStr1Wood.get());
					glVertex3fv(posStr2Wood.get());
					glVertex3fv(posStr3Wood.get());
					glVertex3fv(posStr4Wood.get());
				glEnd();
			glPopMatrix();
		}
		else
		{
			// Draw 'virtual' string:
			glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, red);
			glPushMatrix();
				glBegin(GL_LINES);
					glVertex3fv(Pol2OGlf(curFrame_.derived3dData.posStrRefBridge).get());
					glVertex3fv(Pol2OGlf(curFrame_.derived3dData.posStrRefFb).get());
				glEnd();
			glPopMatrix();

			// Draw acrylic glass cylinder on top of load cell:
			Matrix3x1 cylCenter1 = curFrame_.derived3dData.posStrRefBridge - Matrix3x1(0.0, 0.0, -1.1);
			Matrix3x1 cylCenter2 = curFrame_.derived3dData.posStrRefFb - Matrix3x1(0.0, 0.0, -1.1);
			// Note:
			// Use -1.1, slightly more than cylinder diameter so line is still visible and use 
			// negative value because Polhemus z-axis up means negative and down means positive.

			glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, green);
			drawCylinder(Pol2OGlf(cylCenter1).get(), Pol2OGlf(cylCenter2).get(), 1.0);

			// Draw cheese board:
			Matrix3x1 cheeseBoardCenter = Matrix3x1(10.0f/2.0f + 30.0f/2.0f,
													6.75f - (20.0f/2.0f - 10.0f/2.0f),
													3.0f);
			glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, lighterYellow);
			glPushMatrix();
				glTranslatefv(Pol2OGlf(cheeseBoardCenter).get());
				drawBox(30.0f, 1.4f, 20.0f);
			glPopMatrix();
		}

		// Draw bow force line:
		if (curFrame_.descriptors.bowForce > 0.0)
		{
			glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, yellow);

			glPushMatrix();
				glBegin(GL_LINES);
					if (curFrame_.descriptors.bowForceLhs >= curFrame_.descriptors.bowForceRhs)
					{
						glVertex3fv(Pol2OGlf(curFrame_.derived3dData.interRefLhs.p2).get());
						glVertex3fv(Pol2OGlf(curFrame_.derived3dData.interRefLhs.p1).get());
					}
					else
					{
						glVertex3fv(Pol2OGlf(curFrame_.derived3dData.interRefRhs.p2).get());
						glVertex3fv(Pol2OGlf(curFrame_.derived3dData.interRefRhs.p1).get());
					}
				glEnd();
			glPopMatrix();
		}

		// Draw violin body:
		drawViolinBody();

		// Draw bow stick (estimation):
		GLfloat yellowSeeThough[] = {0.9f, 0.9f, 0.05f, 0.5f};
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glLineWidth(2.0);
		glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, yellowSeeThough);
		glPushMatrix();
			glBegin(GL_LINES);
				glVertex3fv(Pol2OGlf(curFrame_.derived3dData.bowStickFrog).get());
				glVertex3fv(Pol2OGlf(curFrame_.derived3dData.bowStickTip).get());
			glEnd();
		glPopMatrix();

		//// Draw smallest line between stick and bridge:
		//glPushMatrix();
		//	glBegin(GL_LINES);
		//		glVertex3fv(Pol2OGlf(curFrame_.derived3dData.lineBetweenStickAndBridge.p1).get());
		//		glVertex3fv(Pol2OGlf(curFrame_.derived3dData.lineBetweenStickAndBridge.p2).get());
		//	glEnd();
		//glPopMatrix();
		//glDisable(GL_BLEND);

		// Display numerical descriptors:
		setOrthographicProjection(getWidth(), getHeight());

		glPushMatrix();
			glLoadIdentity();
			glDisable(GL_LIGHTING);
			glColor3f(1.f, 1.f, 0.f);

			renderBitmapString(10, 10, GLUT_BITMAP_HELVETICA_12, concat::formatStr("tilt: %.2f°", curFrame_.derived3dData.bowTiltAngleDegrees).c_str());
			renderBitmapString(260, 10, GLUT_BITMAP_HELVETICA_12, concat::formatStr("tilt z: %.2f°", curFrame_.derived3dData.bowTiltAngleZDegrees).c_str());

			renderBitmapString(10, 32, GLUT_BITMAP_HELVETICA_12, concat::formatStr("inclination: %.2f°", curFrame_.derived3dData.bowInclinationDegrees).c_str());
			renderBitmapString(260, 32, GLUT_BITMAP_HELVETICA_12, concat::formatStr("inclination z: %.2f°", curFrame_.derived3dData.bowInclinationZDegrees).c_str());

			renderBitmapString(10, 54, GLUT_BITMAP_HELVETICA_12, concat::formatStr("inclination (smooth): %.2f°", curFrame_.derived3dData.bowInclinationSmoothDegrees).c_str());
			renderBitmapString(260, 54, GLUT_BITMAP_HELVETICA_12, concat::formatStr("inclination z (smooth): %.2f°", curFrame_.derived3dData.bowInclinationZSmoothDegrees).c_str());

			renderBitmapString(10, 76, GLUT_BITMAP_HELVETICA_12, concat::formatStr("bow-bridge angle: %.2f°", curFrame_.derived3dData.bowBridgeAngleDegrees).c_str());

			glEnable(GL_LIGHTING);
		glPopMatrix();

		resetPerspectiveProjection();
	}

	// Motion trail sensors:
	const int motionTrailSize = numMotionTrailFrames_; // alias

	motionTrailBuffer_.clearBySettingReadIdxToWriteIdx(); // set read idx to write idx (last written frame)
	motionTrailBuffer_.decreaseReadIdx(motionTrailSize); // rewind to begin motion trail

	DescriptorFrame motionTrailFrame;
	DescriptorFrame motionTrailFramePrev;
	motionTrailBuffer_.get(motionTrailFramePrev);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glLineWidth(4.0);
	GLfloat greenFade[4];
	greenFade[0] = green[0];
	greenFade[1] = green[1];
	greenFade[2] = green[2];
	GLfloat redFade[4];
	redFade[0] = red[0];
	redFade[1] = red[1];
	redFade[2] = red[2];
	GLfloat alphaPrev = 0.0f;
	for (int i = 0; i < motionTrailSize-1; ++i) // draw all, except cur frame
	{
		motionTrailBuffer_.get(motionTrailFrame);

		GLfloat alpha = (GLfloat)(i+1)/(GLfloat)(motionTrailSize-1);

		glBegin(GL_LINES);
			redFade[3] = alphaPrev;
			glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, redFade);
			glVertex3fv(Pol2OGlf(motionTrailFramePrev.rawSensorData.violinBodySensPos).get());
			redFade[3] = alpha;
			glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, redFade);
			glVertex3fv(Pol2OGlf(motionTrailFrame.rawSensorData.violinBodySensPos).get());

			greenFade[3] = alphaPrev;
			glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, greenFade);
			glVertex3fv(Pol2OGlf(motionTrailFramePrev.rawSensorData.bowSensPos).get());
			greenFade[3] = alpha;
			glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, greenFade);
			glVertex3fv(Pol2OGlf(motionTrailFrame.rawSensorData.bowSensPos).get());
		glEnd();

		motionTrailFramePrev = motionTrailFrame;
		alphaPrev = alpha;
	}
	glDisable(GL_BLEND);

	// Flush:
	glFlush();
}

// ---------------------------------------------------------------------------------------

void Scene3d::resetCamera(bool redraw)
{
	camera_.setupCamera(-183.52, 63.88, -116.21, 6.03, 17.63, -1.36, 0.0, 1.0, 0.0);
	if (redraw && !isTimerRunning())
		repaint();
}

void Scene3d::mouseDown(const MouseEvent &e)
{
	camera_.mouseDown(e.getMouseDownX(), e.getMouseDownY());

	if (!isTimerRunning())
		repaint();
}

void Scene3d::mouseDrag(const MouseEvent &e)
{
	camera_.mouseMove(e.getMouseDownX() + e.getDistanceFromDragStartX(), e.getMouseDownY() + e.getDistanceFromDragStartY());
//	Matrix3x1 eye = camera_.getEye();
//	Matrix3x1 center = camera_.getCenter();
//	LOG_INFO_N("violin_recording_plugin", concat::formatStr("eye: %.2f %.2f %.2f; center: %.2f %.2f %.2f", eye(0, 0), eye(1, 0), eye(2, 0), center(0, 0), center(1, 0), center(2, 0)));

	if (!isTimerRunning())
		repaint();
}

void Scene3d::mouseDoubleClick(const MouseEvent &e)
{
	Camera::MouseMode curMode = camera_.getMouseMode();

	if (curMode == Camera::PANTILT)
	{
		LOG_INFO_N("violin_recording_plugin", "Mouse mode: Roll/move.");
		camera_.setMouseMode(Camera::ROLLMOVE);
	}
	else if (curMode == Camera::ROLLMOVE)
	{
		LOG_INFO_N("violin_recording_plugin", "Mouse mode: Pitch/yaw.");
		camera_.setMouseMode(Camera::PITCHYAW);
	}
	else if (curMode == Camera::PITCHYAW)
	{
		LOG_INFO_N("violin_recording_plugin", "Mouse mode: Dolly x/y.");
		camera_.setMouseMode(Camera::DOLLYXY);
	}
	else if (curMode == Camera::DOLLYXY)
	{
		LOG_INFO_N("violin_recording_plugin", "Mouse mode: Pan/tilt.");
		camera_.setMouseMode(Camera::PANTILT);
	}
}

// ---------------------------------------------------------------------------------------

void Scene3d::timerCallback()
{
BEGIN_IGNORE_EXCEPTIONS
	repaint();
END_IGNORE_EXCEPTIONS("Scene3d::timerCallback()")
}

// ---------------------------------------------------------------------------------------

void Scene3d::postData(const RawSensorData &rawSensorData)
{
	DescriptorFrame frame;
	frame.rawSensorData = rawSensorData;
	frame.areDerived3dDataAndDescriptorsValid = false;
	frame.isCalibratingForce = false;

	if (dataBuffer_.put(frame) != 1)
	{
//		LOG_DEBUG_N("violin_recording_plugin", "[r]Warning: Scene3d buffer overflow.");
	}
}

void Scene3d::postData(const RawSensorData &rawSensorData, const Derived3dData &derived3dData, const ViolinPerformanceDescriptors &descriptors, bool isCalibratingForce)
{
	DescriptorFrame frame;
	frame.rawSensorData = rawSensorData;
	frame.derived3dData = derived3dData;
	frame.descriptors = descriptors;
	frame.areDerived3dDataAndDescriptorsValid = true;
	frame.isCalibratingForce = isCalibratingForce;

	if (dataBuffer_.put(frame) != 1)
	{
//		LOG_DEBUG_N("violin_recording_plugin", "[r]Warning: Scene3d buffer overflow.");
	}
}

// ---------------------------------------------------------------------------------------

void Scene3d::drawCylinder(const float *p1, const float *p2, float radius)
{
	GLUquadricObj *cyl = gluNewQuadric();
	GLUquadricObj *disk = gluNewQuadric();

	const float pi = 3.1415926535897932384626433832795f;
	const float r2d = 180.0f/pi;	// radians to degrees conversion factor

	float rb = radius;		// radius of cylinder bottom
	float rt = radius;		// radius of cylinder top
	int dth = 10;			// no. of angular cylinder subdivsions 
	int dz = 10;			// no. of cylinder subdivsions in z direction

	// orientation vector 
	float vx = p2[0] - p1[0];	//	component in x-direction
	float vy = p2[1] - p1[1];	//	component in y-direction
	float vz = p2[2] - p1[2];	//	component in z-direction

	float v = sqrt(vx*vx + vy*vy + vz*vz);	// cylinder length

	// rotation vector, z x r  
	float rx = -vy*vz;
	float ry = +vx*vz;
	float ax = 0.0f;

	float zero = 1.0e-3f;
	if (fabs(vz) < zero)
	{
		ax = r2d*acos(vx/v);	// rotation angle in x-y plane
		if (vx <= 0.0f)
			ax = -ax;
	}
	else
	{
		ax = r2d*acos(vz/v);	// rotation angle
		if (vz <= 0.0f)
			ax = -ax;
	}

	glPushMatrix();
	{
		glTranslatef(p1[0], p1[1], p1[2]);		// translate to point 1

		if (fabs(vz) < zero)
		{
			glRotatef(90.0, 0, 1, 0.0);			// Rotate & align with x axis
			glRotatef(ax, -1.0, 0.0, 0.0);		// Rotate to point 2 in x-y plane
		}
		else
		{
			glRotatef(ax, rx, ry, 0.0);			// Rotate about rotation vector
		}

		gluCylinder(cyl, rb, rt, v, dth, dz);	// draw side faces of cylinder
		gluDisk(disk, 0.0, rb, dth, 1);			// draw bottom of cylinder
	}
	glPopMatrix();

	glPushMatrix();
	{
		glTranslatef(p2[0], p2[1], p2[2]);		// translate to point 2

		if (fabs(vz) < zero)
		{
			glRotatef(90.0, 0, 1, 0.0);			// Rotate & align with x axis
			glRotatef(ax, -1.0, 0.0, 0.0);		// Rotate to point 2 in the x-y plane
		}
		else
		{
			glRotatef(ax, rx, ry, 0.0);			// Rotate about rotation vector
		}

		gluDisk(disk, 0.0, rt, dth, 1);			// draw top of cylinder

	}
	glPopMatrix();

	gluDeleteQuadric(cyl);
	gluDeleteQuadric(disk);
}

void Scene3d::drawBox(float w, float h, float d)
{
	w /= 2.0f;
	h /= 2.0f;
	d /= 2.0f;

	glBegin(GL_QUADS);
		// Front face:
		glNormal3f( 0.0f,    0.0f,    1.0f); // normal pointing towards viewer
		glVertex3f(-1.0f*w, -1.0f*h,  1.0f*d);
		glVertex3f( 1.0f*w, -1.0f*h,  1.0f*d);
		glVertex3f( 1.0f*w,  1.0f*h,  1.0f*d);
		glVertex3f(-1.0f*w,  1.0f*h,  1.0f*d);

		// Back face:
		glNormal3f( 0.0f,    0.0f,   -1.0f); // normal pointing away from viewer
		glVertex3f(-1.0f*w, -1.0f*h, -1.0f*d);
		glVertex3f(-1.0f*w,  1.0f*h, -1.0f*d);
		glVertex3f( 1.0f*w,  1.0f*h, -1.0f*d);
		glVertex3f( 1.0f*w, -1.0f*h, -1.0f*d);

		// Top face:
		glNormal3f( 0.0f,    1.0f,    0.0f); // normal pointing up
		glVertex3f(-1.0f*w,  1.0f*h, -1.0f*d);
		glVertex3f(-1.0f*w,  1.0f*h,  1.0f*d);
		glVertex3f( 1.0f*w,  1.0f*h,  1.0f*d);
		glVertex3f( 1.0f*w,  1.0f*h, -1.0f*d);

		// Bottom face:
		glNormal3f( 0.0f,   -1.0f,    0.0f); // normal pointing down
		glVertex3f(-1.0f*w, -1.0f*h, -1.0f*d);
		glVertex3f( 1.0f*w, -1.0f*h, -1.0f*d);
		glVertex3f( 1.0f*w, -1.0f*h,  1.0f*d);
		glVertex3f(-1.0f*w, -1.0f*h,  1.0f*d);

		// Right face:
		glNormal3f( 1.0f,    0.0f,    0.0f); // normal pointing right
		glVertex3f( 1.0f*w, -1.0f*h, -1.0f*d);
		glVertex3f( 1.0f*w,  1.0f*h, -1.0f*d);
		glVertex3f( 1.0f*w,  1.0f*h,  1.0f*d);
		glVertex3f( 1.0f*w, -1.0f*h,  1.0f*d);

		// Left face:
		glNormal3f(-1.0f,    0.0f,    0.0f); // normal pointing left
		glVertex3f(-1.0f*w, -1.0f*h, -1.0f*d);
		glVertex3f(-1.0f*w, -1.0f*h,  1.0f*d);
		glVertex3f(-1.0f*w,  1.0f*h,  1.0f*d);
		glVertex3f(-1.0f*w,  1.0f*h, -1.0f*d);
	glEnd();
}

void Scene3d::drawCube(float radius)
{
	drawBox(2.0f*radius, 2.0f*radius, 2.0f*radius);
}

void Scene3d::drawAxes(float length)
{
	glBegin(GL_LINES);
		// positive x-axis:
		glVertex3f(0.0f,   0.0f,   0.0f);
		glVertex3f(length, 0.0f,   0.0f);

		// positive y-axis:
		glVertex3f(0.0f,   0.0f,   0.0f);
		glVertex3f(0.0f,   length, 0.0f);

		// positive z-axis:
		glVertex3f(0.0f,   0.0f,   0.0f);
		glVertex3f(0.0f,   0.0f,   length);
	glEnd();
}

void Scene3d::drawSensor(float pointSize, float axesSize, const Matrix3x1 &pos, const Matrix3x1 &att)
{
	Matrix3x3 rot = Matrix3x3::rotation_matrix_zyx(att(0, 0), att(1, 0), att(2, 0));

	Pol2OGlf origin(pos);

	double v = axesSize;
	Matrix3x1 xAxis = rot*Matrix3x1(v, 0.0, 0.0) + pos;
	Matrix3x1 yAxis = rot*Matrix3x1(0.0, v, 0.0) + pos;
	Matrix3x1 zAxis = rot*Matrix3x1(0.0, 0.0, v) + pos;

	glPointSize(pointSize);

	glBegin(GL_POINTS);
		glVertex3fv(origin.get());
	glEnd();

	glLineWidth(1.5f);

	glBegin(GL_LINES);
		glVertex3fv(origin.get());
		glVertex3fv(Pol2OGlf(xAxis).get());

		glVertex3fv(origin.get());
		glVertex3fv(Pol2OGlf(yAxis).get());

		glVertex3fv(origin.get());
		glVertex3fv(Pol2OGlf(zAxis).get());
	glEnd();
}

void Scene3d::drawStylusSensor(float pointSize, const Matrix3x1 &pos, const Matrix3x1 &att)
{
	Matrix3x3 rot = Matrix3x3::rotation_matrix_zyx(att(0, 0), att(1, 0), att(2, 0));

	Pol2OGlf origin(pos);

	Matrix3x1 backStylus = rot*Matrix3x1(-6.88*2.54, 0.0, 0.0) + pos;

	glPointSize(pointSize);

	glBegin(GL_POINTS);
		glVertex3fv(origin.get());
	glEnd();

	glLineWidth(1.5f);

	glBegin(GL_LINES);
		glVertex3fv(origin.get());
		glVertex3fv(Pol2OGlf(backStylus).get());
	glEnd();
}

void Scene3d::drawFloor(float tileDimensions, int numTiles)
{
	assert((numTiles % 2) == 0);
	GLfloat d = tileDimensions;
	GLfloat d0_5 = d/2.0f;

	GLfloat darkerGray[] = {0.3f, 0.3f, 0.3f, 1.f};
	GLfloat lighterGray[] = {0.6f, 0.6f, 0.6f, 1.f};

	GLfloat x;
	GLfloat z;
	int k;

	z = -numTiles/2*d + d0_5;

	for (int i = 0; i < numTiles; ++i)
	{
		x = -numTiles/2*d + d0_5;

		k = (i % 2);

		for (int j = 0; j < numTiles; ++j)
		{
			if ((k % 2) == 0)
				glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, darkerGray);
			else
				glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, lighterGray);

			glPushMatrix();
				glTranslatef(x, 0.0f, z);
				glBegin(GL_QUADS);
					glNormal3f( 0.0f,    1.0f,   0.0f); // normal pointing up
					glVertex3f(-1.0f*d0_5,  0.0f, -1.0f*d0_5);
					glVertex3f(-1.0f*d0_5,  0.0f,  1.0f*d0_5);
					glVertex3f( 1.0f*d0_5,  0.0f,  1.0f*d0_5);
					glVertex3f( 1.0f*d0_5,  0.0f, -1.0f*d0_5);
				glEnd();
			glPopMatrix();

			x += d;
			k += 1;
		}

		z += d;
	}
}


void Scene3d::drawViolinBody()
{
//	ScopedLock scopedLock(violinModel3dLock_);

	if (violinModel3d_.areAllStepsSet())
	{
		float wood[] = {198.f/255.f, 116.f/255.f, 26.f/255.f, 1.0f};
		glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, wood);

		// Top:
		glPushMatrix();
		{
			glBegin(GL_LINES);
				const int n = violinModel3d_.getNumVertexSteps(ViolinModel3d::BODY_OUTLINE_UPPER);
				for (int i = 0; i < n; ++i)
				{
					const int i1 = (i % n);
					const int i2 = ((i+1) % n);
					Matrix3x1 p1 = curFrame_.derived3dData.violinBodyRotMat*violinModel3d_.getVertex(ViolinModel3d::BODY_OUTLINE_UPPER, i1) + curFrame_.rawSensorData.violinBodySensPos;
					Matrix3x1 p2 = curFrame_.derived3dData.violinBodyRotMat*violinModel3d_.getVertex(ViolinModel3d::BODY_OUTLINE_UPPER, i2) + curFrame_.rawSensorData.violinBodySensPos;

					glVertex3fv(Pol2OGlf(p1).get());
					glVertex3fv(Pol2OGlf(p2).get());
				}
			glEnd();
		}
		glPopMatrix();

//		Matrix3x1 d = violinModel3d_.getVertex(ViolinModel3d::BODY_DEPTH, 1) - violinModel3d_.getVertex(ViolinModel3d::BODY_DEPTH, 0);
		Matrix3x1 d = 3.75f*curFrame_.derived3dData.vecBridgeUp; // approximate depth to 3.75 cm downward from top

		// Bottom:
		glPushMatrix();
		{
			glBegin(GL_LINES);
				const int n = violinModel3d_.getNumVertexSteps(ViolinModel3d::BODY_OUTLINE_UPPER);
				for (int i = 0; i < n; ++i)
				{
					const int i1 = (i % n);
					const int i2 = ((i+1) % n);
					Matrix3x1 p1 = curFrame_.derived3dData.violinBodyRotMat*violinModel3d_.getVertex(ViolinModel3d::BODY_OUTLINE_UPPER, i1) + curFrame_.rawSensorData.violinBodySensPos - d;
					Matrix3x1 p2 = curFrame_.derived3dData.violinBodyRotMat*violinModel3d_.getVertex(ViolinModel3d::BODY_OUTLINE_UPPER, i2) + curFrame_.rawSensorData.violinBodySensPos - d;

					glVertex3fv(Pol2OGlf(p1).get());
					glVertex3fv(Pol2OGlf(p2).get());
				}
			glEnd();
		}
		glPopMatrix();

		// Sides:
		glPushMatrix();
		{
			glBegin(GL_LINES);
				for (int i = 0; i < violinModel3d_.getNumVertexSteps(ViolinModel3d::BODY_OUTLINE_UPPER); ++i)
				{
					Matrix3x1 p1 = curFrame_.derived3dData.violinBodyRotMat*violinModel3d_.getVertex(ViolinModel3d::BODY_OUTLINE_UPPER, i) + curFrame_.rawSensorData.violinBodySensPos;
					Matrix3x1 p2 = curFrame_.derived3dData.violinBodyRotMat*violinModel3d_.getVertex(ViolinModel3d::BODY_OUTLINE_UPPER, i) + curFrame_.rawSensorData.violinBodySensPos - d;

					glVertex3fv(Pol2OGlf(p1).get());
					glVertex3fv(Pol2OGlf(p2).get());
				}
			glEnd();
		}
		glPopMatrix();

		float white[] = {1.f, 1.f, 1.f, 1.f};
		glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, white);

		// Finger board:
		glPushMatrix();
		{
			glBegin(GL_LINES);
				Matrix3x1 p1 = curFrame_.derived3dData.violinBodyRotMat*violinModel3d_.getVertex(ViolinModel3d::FINGER_BOARD, 0) + curFrame_.rawSensorData.violinBodySensPos;
				Matrix3x1 p2 = curFrame_.derived3dData.violinBodyRotMat*violinModel3d_.getVertex(ViolinModel3d::FINGER_BOARD, 1) + curFrame_.rawSensorData.violinBodySensPos;
				Matrix3x1 p3 = curFrame_.derived3dData.violinBodyRotMat*violinModel3d_.getVertex(ViolinModel3d::FINGER_BOARD, 2) + curFrame_.rawSensorData.violinBodySensPos;
				Matrix3x1 p4 = curFrame_.derived3dData.violinBodyRotMat*violinModel3d_.getVertex(ViolinModel3d::FINGER_BOARD, 3) + curFrame_.rawSensorData.violinBodySensPos;

				glVertex3fv(Pol2OGlf(p1).get());
				glVertex3fv(Pol2OGlf(p2).get());

				glVertex3fv(Pol2OGlf(p2).get());
				glVertex3fv(Pol2OGlf(p3).get());

				glVertex3fv(Pol2OGlf(p3).get());
				glVertex3fv(Pol2OGlf(p4).get());

				glVertex3fv(Pol2OGlf(p4).get());
				glVertex3fv(Pol2OGlf(p1).get());
			glEnd();
		}
		glPopMatrix();

		// Bridge:
		glPushMatrix();
		{
			glBegin(GL_LINES);
				Matrix3x1 p1 = curFrame_.derived3dData.violinBodyRotMat*violinModel3d_.getVertex(ViolinModel3d::BRIDGE, 0) + curFrame_.rawSensorData.violinBodySensPos;
				Matrix3x1 p2 = curFrame_.derived3dData.violinBodyRotMat*violinModel3d_.getVertex(ViolinModel3d::BRIDGE, 1) + curFrame_.rawSensorData.violinBodySensPos;
				Matrix3x1 p3 = curFrame_.derived3dData.violinBodyRotMat*violinModel3d_.getVertex(ViolinModel3d::BRIDGE, 2) + curFrame_.rawSensorData.violinBodySensPos;
				Matrix3x1 p4 = curFrame_.derived3dData.violinBodyRotMat*violinModel3d_.getVertex(ViolinModel3d::BRIDGE, 3) + curFrame_.rawSensorData.violinBodySensPos;
				Matrix3x1 p5 = curFrame_.derived3dData.violinBodyRotMat*violinModel3d_.getVertex(ViolinModel3d::BRIDGE, 4) + curFrame_.rawSensorData.violinBodySensPos;

				glVertex3fv(Pol2OGlf(p1).get()); // lower-right
				glVertex3fv(Pol2OGlf(p2).get()); // upper-right

				glVertex3fv(Pol2OGlf(p2).get()); // upper-right
				glVertex3fv(Pol2OGlf(curFrame_.derived3dData.posStr1Bridge).get()); // str1

				glVertex3fv(Pol2OGlf(curFrame_.derived3dData.posStr1Bridge).get()); // str1
				glVertex3fv(Pol2OGlf(curFrame_.derived3dData.posStr2Bridge).get()); // str2

				glVertex3fv(Pol2OGlf(curFrame_.derived3dData.posStr2Bridge).get()); // str2
				glVertex3fv(Pol2OGlf(p3).get()); // center

				glVertex3fv(Pol2OGlf(p3).get()); // center
				glVertex3fv(Pol2OGlf(curFrame_.derived3dData.posStr3Bridge).get()); // str3

				glVertex3fv(Pol2OGlf(curFrame_.derived3dData.posStr3Bridge).get()); // str3
				glVertex3fv(Pol2OGlf(curFrame_.derived3dData.posStr4Bridge).get()); // str4

				glVertex3fv(Pol2OGlf(curFrame_.derived3dData.posStr4Bridge).get()); // str4
				glVertex3fv(Pol2OGlf(p4).get()); // upper-right

				glVertex3fv(Pol2OGlf(p4).get()); // upper-right
				glVertex3fv(Pol2OGlf(p5).get()); // lower-right

				glVertex3fv(Pol2OGlf(p5).get()); // lower-right
				glVertex3fv(Pol2OGlf(p1).get()); // lower-left
			glEnd();
		}
		glPopMatrix();

		// Tail piece:
		glPushMatrix();
		{
			Matrix3x1 p1 = curFrame_.derived3dData.violinBodyRotMat*violinModel3d_.getVertex(ViolinModel3d::TAIL_PIECE, 0) + curFrame_.rawSensorData.violinBodySensPos;
			Matrix3x1 p2 = curFrame_.derived3dData.violinBodyRotMat*violinModel3d_.getVertex(ViolinModel3d::TAIL_PIECE, 1) + curFrame_.rawSensorData.violinBodySensPos;
			Matrix3x1 p3 = curFrame_.derived3dData.violinBodyRotMat*violinModel3d_.getVertex(ViolinModel3d::TAIL_PIECE, 2) + curFrame_.rawSensorData.violinBodySensPos;

			glBegin(GL_LINES);
				glVertex3fv(Pol2OGlf(p1).get());
				glVertex3fv(Pol2OGlf(p2).get());

				glVertex3fv(Pol2OGlf(p2).get());
				glVertex3fv(Pol2OGlf(p3).get());

				glVertex3fv(Pol2OGlf(p3).get());
				glVertex3fv(Pol2OGlf(p1).get());
			glEnd();
		}
		glPopMatrix();

		// Strings to tail piece:
		GLfloat red[] = {0.7f, 0.1f, 0.1f, 1.f};
		glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, red);

		glPushMatrix();
		{
			Matrix3x1 p1 = curFrame_.derived3dData.violinBodyRotMat*violinModel3d_.getVertex(ViolinModel3d::STRINGS_ON_TAIL, 0) + curFrame_.rawSensorData.violinBodySensPos;
			Matrix3x1 p2 = curFrame_.derived3dData.violinBodyRotMat*violinModel3d_.getVertex(ViolinModel3d::STRINGS_ON_TAIL, 1) + curFrame_.rawSensorData.violinBodySensPos;
			Matrix3x1 p3 = curFrame_.derived3dData.violinBodyRotMat*violinModel3d_.getVertex(ViolinModel3d::STRINGS_ON_TAIL, 2) + curFrame_.rawSensorData.violinBodySensPos;
			Matrix3x1 p4 = curFrame_.derived3dData.violinBodyRotMat*violinModel3d_.getVertex(ViolinModel3d::STRINGS_ON_TAIL, 3) + curFrame_.rawSensorData.violinBodySensPos;

			glBegin(GL_LINES);
				glVertex3fv(Pol2OGlf(p1).get());
				glVertex3fv(Pol2OGlf(curFrame_.derived3dData.posStr1Bridge).get());
				
				glVertex3fv(Pol2OGlf(p2).get());
				glVertex3fv(Pol2OGlf(curFrame_.derived3dData.posStr2Bridge).get());

				glVertex3fv(Pol2OGlf(p3).get());
				glVertex3fv(Pol2OGlf(curFrame_.derived3dData.posStr3Bridge).get());

				glVertex3fv(Pol2OGlf(p4).get());
				glVertex3fv(Pol2OGlf(curFrame_.derived3dData.posStr4Bridge).get());
			glEnd();
		}
		glPopMatrix();
	}
}



