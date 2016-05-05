#include "Camera.hxx"

// ---------------------------------------------------------------------------------------

#ifdef _WIN32
 #include <windows.h>
#elif !	defined	(LINUX)
 #include <Carbon.h>
 #include <Movies.h>
#endif

#include "juce.h"

#ifndef	JUCE_OPENGL
#error Juce	OpenGL is not enabled.
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

// ---------------------------------------------------------------------------------------

const double pi = 3.1415926535897932384626433832795;
const double degreesToRadians = pi/180.0;

// ---------------------------------------------------------------------------------------

Camera::Camera()
{
	// Default parameters:
	mouseMode_ = PITCHYAW;
	isGravityEnabled_ = true; // (can't use setGravityEnabled() because it checks isGravityEnabled_)
	setupCamera(0.0, 0.0, 100.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0); // calls computeCameraAxes()
}

Camera::~Camera()
{
}

// ---------------------------------------------------------------------------------------

void Camera::setupCamera(double	p_eyex,	double p_eyey, double p_eyez, double p_centerx,	double p_centery, double p_centerz,	double p_upx, double p_upy,	double p_upz)
{
	eye_ = Matrix3x1(p_eyex, p_eyey, p_eyez);
	center_ = Matrix3x1(p_centerx, p_centery, p_centerz);
	up_ = normalize(Matrix3x1(p_upx, p_upy, p_upz));

	computeCameraAxes();
}

// Forces up direction of camera to always stay pointed up.
void Camera::setGravityEnabled(bool enabled)
{
	if (isGravityEnabled_ == enabled)
		return;

	isGravityEnabled_ = enabled;

	if (isGravityEnabled_)
	{
		up_ = Matrix3x1(0.0, 1.0, 0.0); // superfluous
		computeCameraAxes();
	}
}

// ---------------------------------------------------------------------------------------

double Camera::getCameraDistance() const
{
	Matrix3x1 view = eye_ - center_; // view vector
	return euclidean_length(view);
}

bool Camera::isGravityEnabled() const
{
	return isGravityEnabled_;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

const Matrix3x1 &Camera::getEye() const
{
	return eye_;
}

const Matrix3x1 &Camera::getCenter() const
{
	return center_;
}

const Matrix3x1 &Camera::getUp() const
{
	return up_;
}

// ---------------------------------------------------------------------------------------

void Camera::dollyCenter(double	x, double y, double	z)
{
	Matrix4x4 t = dollyHelper(x, y, z);
	center_ = t*center_;
	computeCameraAxes();
}

void Camera::dollyCamera(double	x, double y, double	z)
{
	Matrix4x4 t = dollyHelper(x, y, z);
	eye_ = t*eye_;
	computeCameraAxes();
}

void Camera::dollyCenterAndCamera(double x, double	y, double z)
{
	Matrix4x4 t = dollyHelper(x, y, z);
	center_ = t*center_;
	eye_ = t*eye_;
	// (frame does not cange)
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

//
// Center rotation operations.	These function rotate the camera around	
// the center location.	 Note that camera roll and center roll would
// be the same thing.  So, we only need	Yaw	and	Pitch.
//

void Camera::pitch(double degrees)
{
	Matrix4x4 ucen = Matrix4x4::translation_matrix(center_(0, 0), center_(1, 0), center_(2, 0));
	Matrix4x4 rot = rotCameraX(degrees*degreesToRadians);
	Matrix4x4 cen = Matrix4x4::translation_matrix(-center_(0, 0), -center_(1, 0), -center_(2, 0));
	Matrix4x4 t = (ucen*rot)*cen;

	eye_ = t*eye_;
	up_ = t*up_;

	computeCameraAxes();
}

void Camera::yaw(double	degrees)
{
	Matrix4x4 ucen = Matrix4x4::translation_matrix(center_(0, 0), center_(1, 0), center_(2, 0));
	Matrix4x4 rot = rotCameraY(degrees*degreesToRadians);
	Matrix4x4 cen = Matrix4x4::translation_matrix(-center_(0, 0), -center_(1, 0), -center_(2, 0));
	Matrix4x4 t = (ucen*rot)*cen;

	eye_ = t*eye_;
	up_ = t*up_;

	computeCameraAxes();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

//
// Camera rotation operations.	These function rotate the camera
// around the eye position.
//

void Camera::roll(double degrees)
{
	Matrix4x4 ucen = Matrix4x4::translation_matrix(eye_(0, 0), eye_(1, 0), eye_(2, 0));
	Matrix4x4 rot = rotCameraZ(degrees*degreesToRadians);
	Matrix4x4 cen = Matrix4x4::translation_matrix(-eye_(0, 0), -eye_(1, 0), -eye_(2, 0));
	Matrix4x4 t = (ucen*rot)*cen;

	center_ = t*center_;
	up_ = t*up_;

	computeCameraAxes();
}

void Camera::tilt(double degrees)
{
	Matrix4x4 ucen = Matrix4x4::translation_matrix(eye_(0, 0), eye_(1, 0), eye_(2, 0));
	Matrix4x4 rot = rotCameraX(degrees*degreesToRadians);
	Matrix4x4 cen = Matrix4x4::translation_matrix(-eye_(0, 0), -eye_(1, 0), -eye_(2, 0));
	Matrix4x4 t = (ucen*rot)*cen;

	center_ = t*center_;
	up_ = t*up_;

	computeCameraAxes();
}

void Camera::pan(double	degrees)
{
	Matrix4x4 ucen = Matrix4x4::translation_matrix(eye_(0, 0), eye_(1, 0), eye_(2, 0));
	Matrix4x4 rot = rotCameraY(degrees*degreesToRadians);
	Matrix4x4 cen = Matrix4x4::translation_matrix(-eye_(0, 0), -eye_(1, 0), -eye_(2, 0));
	Matrix4x4 t = (ucen*rot)*cen;

	center_ = t*center_;
	up_ = t*up_;

	computeCameraAxes();
}

// ---------------------------------------------------------------------------------------

void Camera::setMouseMode(MouseMode mode)
{
	mouseMode_ = mode;
}

Camera::MouseMode Camera::getMouseMode() const
{
	return mouseMode_;
}

void Camera::mouseDown(int x, int y)
{
	mouseX_ = x;
	mouseY_ = y;
}

void Camera::mouseMove(int x, int y)
{
	switch (mouseMode_)
	{
	case PANTILT:
		pan((x - mouseX_)*-0.1);
		tilt((y - mouseY_)*-0.1);
		break;

	case ROLLMOVE:
		roll((x - mouseX_)*0.3);
		dollyCamera(0.0, 0.0, y - mouseY_);
		break;

	case DOLLYXY:
		dollyCamera(x - mouseX_, y - mouseY_, 0.0);
		break;

	case PITCHYAW:
		yaw((x - mouseX_)*-0.1);
		pitch((y - mouseY_)*-0.1);
		break;
	}

	mouseX_ = x;
	mouseY_ = y;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

void Camera::gluLookAt()
{
	::gluLookAt(eye_(0, 0), eye_(1, 0), eye_(2, 0),
				center_(0, 0), center_(1, 0), center_(2, 0), 
				up_(0, 0), up_(1, 0), up_(2, 0));
}

// ---------------------------------------------------------------------------------------

// Computes transformation matrix so object faces camera and is located at the 
// billboard point.
Matrix4x4 Camera::computeBillboardMatrix(const Matrix3x1 &billboardPoint) const
{
	Matrix3x1 look = normalize(eye_ - billboardPoint);
	Matrix3x1 right = cross(up_, look);
	Matrix3x1 up = cross(look, right);
	return Matrix4x4(	right(0, 0),	up(0, 0),	look(0, 0),	billboardPoint(0, 0),
						right(1, 0),	up(1, 0),	look(1, 0),	billboardPoint(1, 0),
						right(2, 0),	up(2, 0),	look(2, 0),	billboardPoint(2, 0),
						0.0,			0.0,		0.0,		1.0);
}


// ---------------------------------------------------------------------------------------

//
// Name	:		  Camera::ComputeFrame()
// Description :  We maintain variables	that describe the X,Y,Z	axis of	
//				  the camera frame.	 This function computes	those values.
//
void Camera::computeCameraAxes()
{
	if (isGravityEnabled_)
	{
		up_ = Matrix3x1(0.0, 1.0, 0.0);
	}

	cameraZ_ = eye_ - center_;
	cameraZ_ = normalize(cameraZ_);
	cameraX_ = cross(cameraZ_, up_);
	cameraX_ = normalize(cameraX_);
	cameraY_ = cross(cameraZ_, cameraX_);
}

// ---------------------------------------------------------------------------------------

Matrix4x4 Camera::dollyHelper(double x, double y, double z)
{
	Matrix4x4 uncam = getUnRotMatCamera();
	Matrix4x4 tran = Matrix4x4::translation_matrix(x, y, z);
	Matrix4x4 tocam = getRotMatCamera();
	Matrix4x4 result = (uncam*tran)*tocam;
	return result;
}

Matrix4x4 Camera::getRotMatCamera()
{
	Matrix4x4 result = Matrix4x4::identity_matrix();
	result(0, 0) = cameraX_(0, 0);	result(0, 1) = cameraX_(1, 0);	result(0, 2) = cameraX_(2, 0);
	result(1, 0) = cameraY_(0, 0);	result(1, 1) = cameraY_(1, 0);	result(1, 2) = cameraY_(2, 0);
	result(2, 0) = cameraZ_(0, 0);	result(2, 1) = cameraZ_(1, 0);	result(2, 2) = cameraZ_(2, 0);
	return result;
}

Matrix4x4 Camera::getUnRotMatCamera()
{
	Matrix4x4 result = Matrix4x4::identity_matrix();
	result(0, 0) = cameraX_(0, 0);	result(0, 1) = cameraY_(0, 0);	result(0, 2) = cameraZ_(0, 0);
	result(1, 0) = cameraX_(1, 0);	result(1, 1) = cameraY_(1, 0);	result(1, 2) = cameraZ_(1, 0);
	result(2, 0) = cameraX_(2, 0);	result(2, 1) = cameraY_(2, 0);	result(2, 2) = cameraZ_(2, 0);
	return result;
}

Matrix4x4 Camera::rotCameraX(double angleRadians)
{
	Matrix4x4 uncam = getUnRotMatCamera();
	Matrix4x4 rot = Matrix4x4::rotation_matrix_x(angleRadians);
	Matrix4x4 tocam = getRotMatCamera();
	Matrix4x4 result = (uncam*rot)*tocam;
	return result;
}

Matrix4x4 Camera::rotCameraY(double angleRadians)
{
	Matrix4x4 uncam = getUnRotMatCamera();
	Matrix4x4 rot = Matrix4x4::rotation_matrix_y(angleRadians);
	Matrix4x4 tocam = getRotMatCamera();
	Matrix4x4 result = (uncam*rot)*tocam;
	return result;
}

Matrix4x4 Camera::rotCameraZ(double angleRadians)
{
	Matrix4x4 uncam = getUnRotMatCamera();
	Matrix4x4 rot = Matrix4x4::rotation_matrix_z(angleRadians);
	Matrix4x4 tocam = getRotMatCamera();
	Matrix4x4 result = (uncam*rot)*tocam;
	return result;
}





