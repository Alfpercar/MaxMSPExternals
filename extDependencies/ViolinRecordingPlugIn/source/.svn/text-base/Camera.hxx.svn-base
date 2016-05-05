#ifndef	INCLUDED_CAMERA_HXX
#define	INCLUDED_CAMERA_HXX

#include "SimpleMatrix.hxx"

// ---------------------------------------------------------------------------------------

// Class to	handle camera operations for OpenGL.
// Also	handles	mouse operation	to camera operation	translation.
//
// Adapted from	CGrCamera v1.01	(02/03/03) by Charles B. Owen.
//
// Camera position is defined by eye (x, y,	z).
// Center (x, y, z)	defines	some point on the (positive) line of sight vector, typically 
// the center of the scene that	is being looked	at.
// Up (x, y, z)	defines	the	vector (from eye) that determines the up direction of the 
// camera.
//
// dolly = rotate center around camera (?)
// pitch/yaw = rotate camera around center (?)
// pan/tilt = move center (tilt is disabled with gravity on?) (?)
// roll/move = move camera toward/from center (?)
class Camera
{
public:
	Camera();
	~Camera();

	// Initial set up of camera:
	void setupCamera(double p_eyex, double p_eyey, double p_eyez, double p_cenx, double p_ceny, double p_cenz, double p_upx, double	p_upy, double p_upz);
	void setGravityEnabled(bool p_gravity);

	// Query properties	of camera:
	double getCameraDistance() const;
	bool isGravityEnabled() const;

	const Matrix3x1 &getEye() const;
	const Matrix3x1 &getCenter() const;
	const Matrix3x1 &getUp() const;

	// Camera operations:
	void dollyCenter(double	x, double y, double	z);
	void dollyCamera(double	x, double y, double	z);
	void dollyCenterAndCamera(double x, double y, double z);

	void pitch(double degrees);
	void yaw(double	degrees);
	
	void roll(double degrees);
	void tilt(double degrees);
	void pan(double	degrees);

	// Mouse operations:
	enum MouseMode
	{
		PANTILT,
		ROLLMOVE,
		PITCHYAW,
		DOLLYXY
	};

	void setMouseMode(MouseMode mode);
	MouseMode getMouseMode() const;

	void mouseDown(int x, int y);
	void mouseMove(int x, int y);

	// OpenGL camera matching to internal state:
	void gluLookAt();

	// Additional methods:
	Matrix4x4 computeBillboardMatrix(const Matrix3x1 &billboardPoint) const;

private:
	Matrix3x1 up_;
	Matrix3x1 center_;
	Matrix3x1 eye_;

	bool isGravityEnabled_;

	int	mouseY_;
	int	mouseX_;
	MouseMode mouseMode_;

private:
	// Camera axes:
	Matrix3x1 cameraX_;
	Matrix3x1 cameraY_;
	Matrix3x1 cameraZ_;

	void computeCameraAxes();

	// Helper functions:
	Matrix4x4 dollyHelper(double x, double y, double z);
	Matrix4x4 getRotMatCamera();
	Matrix4x4 getUnRotMatCamera();
	Matrix4x4 rotCameraX(double angleRadians);
	Matrix4x4 rotCameraY(double angleRadians);
	Matrix4x4 rotCameraZ(double angleRadians);
};

#endif


