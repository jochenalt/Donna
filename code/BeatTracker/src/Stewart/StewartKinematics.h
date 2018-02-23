/*
 * Kinematics.h
 *
 *  Created on: Feb 20, 2018
 *      Author: jochenalt
 */

#ifndef SRC_STEWART_STEWARTKINEMATICS_H_
#define SRC_STEWART_STEWARTKINEMATICS_H_

#include "basics/point.h"

struct StewartConfiguration {
	double servoCentreRadius_mm;       	// from birds perspective, distance from centre to this servo's turning centre
	double servoCentreAngle_rad;		// angle we need to turn the in order to come to the centre point

	double servoArmCentreRadius_mm;		// same like servoCentre, but the point where the servo arm is mounted. Used for rendering only, not for kinematics
	double servoArmCentreAngle_mm;

	double plateJointRadius_mm;		    // distance of a plate's ball joint to the centre
	double plateJointAngle_rad;			// Angle we need to turn from middle axis in order to come to the ball joint

	double rodLength_mm;				// length of the rod between base and plate
	double servoArmLength_mm; 		    // length of the servo lever
	double servoCentreHeight_mm;		// height of servo centre from the ground

	double plateBallJointHeight_mm;		// height of the plate relative to the ball joints
};

class StewartKinematics {
public:
	StewartKinematics();
	virtual ~StewartKinematics();

	// initializes cached computations. Required before calling computeServoAngles
	void setup(StewartConfiguration config);

	// compute the angle of all servos depending on the pose of the top plate
	// return ballpoint positions as well used for rendering
	void computeServoAngles(const Pose& plate, double servoAngle_rad[6], Point ballJoint_world[6],  Point servoBallJoint_world[6]);

	// get the centre where the servo arms are mounted (used for rendering)
	void getServoArmCentre(Point servoArmCentre_world[6]);
private:
	double computeServoAngle(int cornerNo, const Point& ballJoint);
	bool mirrorFrame(int cornerNo) { return (cornerNo % 2 == 1); };


	Pose servoCentre[6];
	Point servoArmCentre[6];
	Point plateBallJoint[6];
	HomogeneousMatrix servoCentreTransformationInv[6];
	StewartConfiguration config;
};

#endif /* SRC_STEWART_STEWARTKINEMATICS_H_ */