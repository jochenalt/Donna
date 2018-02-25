
#include <stdio.h>

#include <assert.h>
#include "basics/util.h"
#include "MoveMaker.h"
#include "DanceMove.h"

#include "WindowController.h"


#include "BotView.h"
#include "uiconfig.h"
#include "setup.h"

using namespace std;

// Initial main window size
int WindowWidth = 600;
int WindowHeight = 800;

// GLUI Window handlers
int wMain;			// main window
int wMainBotView; 	// sub window with dancing bot

const int DanceMoveRows = 2;
GLUI_RadioGroup* currentDancingModeWidget[DanceMoveRows] = { NULL, NULL };
int dancingModeLiveVar[DanceMoveRows] = { 0,0 };

GLUI_RadioGroup* currentSequenceModeWidget = NULL;
int currentSequenceModeLiveVar = 0;


/* Handler for window-repaint event. Call back when the window first appears and
 whenever the window needs to be re-painted. */
void displayMainView() {
}

void nocallback(int value) {
}

void reshape(int w, int h) {
	int savedWindow = glutGetWindow();
	WindowWidth = w;
	WindowHeight = h;
	glViewport(0, 0, w, h);

	int MainSubWindowWidth = w - 2 * WindowGap;
	int MainSubWindowHeight = h - InteractiveWindowHeight - 3 * WindowGap;

	WindowController::getInstance().mainBotView.reshape(WindowGap, WindowGap,MainSubWindowWidth, MainSubWindowHeight);

	glutSetWindow(savedWindow);
}

void GluiReshapeCallback( int x, int y )
{
	reshape(x,y);
	int tx, ty, tw, th;
	int saveWindow = glutGetWindow();
	glutSetWindow(wMain);
	GLUI_Master.get_viewport_area( &tx, &ty, &tw, &th );
	glViewport( tx, ty, tw, th );
	glutSetWindow(saveWindow);
	// WindowController::getInstance().postRedisplay();
}


void WindowController::setBodyPose(const Pose& bodyPose, const Pose& headPose) {

	Point eyeLookAt = mainBotView.getEyePosition();
	// Point eyeLookAt (500,0,100);

	// compute the position of the look-at point from the heads perspective:
	//      compute homogenous transformation matrix of head
	// 		compute inverse homogenous matrix for reversing the coord system
	// 	    get look at point from heads perspective by multiplying inverse matrix above with look-at-position
	/*
	HomogeneousMatrix bodyTransformation = HomogeneousMatrix(4,4,
					{ 1, 	0,  	0,  	bodyPose.position.x,
					  0, 	1, 		0,	 	bodyPose.position.y,
					  0,	0,		1,		bodyPose.position.z,
					  0,	0,		0,		1});
	HomogeneousMatrix rotateBody;
	createRotationMatrix(bodyPose.orientation, rotateBody);
	bodyTransformation *= rotateBody;
	HomogeneousMatrix inverseBodyTransformation;
	*/
	HomogeneousMatrix bodyTransformation;
	HomogeneousMatrix inverseBodyTransformation;

	createTransformationMatrix(bodyPose, bodyTransformation);
	computeInverseTransformationMatrix(bodyTransformation, inverseBodyTransformation);

	HomogeneousVector lookAtPosition = {
						eyeLookAt.x,
						eyeLookAt.y,
						eyeLookAt.z,
						1.0 };

	Point lookAtCoordFromBodysPerspective= inverseBodyTransformation * lookAtPosition;
	mainBotView.setBodyPose(bodyPose, headPose, lookAtCoordFromBodysPerspective);
}

void setDancingMoveWidget() {
	int movesPerRow = MoveMaker::getInstance().getNumMoves()/DanceMoveRows ;
	Move::MoveType move = MoveMaker::getInstance().getCurrentMove();
	int moveNumber = (int)move;
	int row = moveNumber / movesPerRow;
	int line = moveNumber - movesPerRow*row;
	if (moveNumber == Move::MoveType::NO_MOVE) {
		row = 0;
		line = 0;
	} else {
		if (row == 0)
			line++;
	}

	currentDancingModeWidget[row]->set_int_val(line);

	for (int i = 0;i<DanceMoveRows;i++) {
		if (i != row)
			currentDancingModeWidget[i]->set_int_val(-1);
	}
}


void currentDancingMoveCallback(int widgetNo) {
	int movesPerRow = MoveMaker::getInstance().getNumMoves()/DanceMoveRows ;
	int row = widgetNo;
	assert(row < DanceMoveRows);
	if (widgetNo == 0) {
		if (dancingModeLiveVar[row] == 0)
			MoveMaker::getInstance().setCurrentMove(Move::MoveType::NO_MOVE);
		else
			MoveMaker::getInstance().setCurrentMove((Move::MoveType)(dancingModeLiveVar[row]-1));
	}
	else
		MoveMaker::getInstance().setCurrentMove((Move::MoveType)(dancingModeLiveVar[row] + row*movesPerRow));
}

void setSequenceModeWidget() {
	currentSequenceModeWidget->set_int_val((int)MoveMaker::getInstance().getSequenceMode());
}

void currentSequenceModeCallback(int widgetNo) {
	MoveMaker::getInstance().setSequenceMode((MoveMaker::SequenceModeType)currentSequenceModeLiveVar);
}

GLUI* WindowController::createInteractiveWindow(int mainWindow) {

	string emptyLine = "                                               ";

	GLUI *windowHandle= GLUI_Master.create_glui_subwindow( wMain,  GLUI_SUBWINDOW_BOTTOM);
	windowHandle->set_main_gfx_window( wMain );

	GLUI_Panel* interactivePanel = new GLUI_Panel(windowHandle,"interactive panel", GLUI_PANEL_NONE);

	GLUI_StaticText* text = NULL;
	// GLUI_StaticText* text = new GLUI_StaticText(interactivePanel,"Current Dance Move                                                   ");
	// text->set_alignment(GLUI_ALIGN_LEFT);

	GLUI_Panel* dancingModePanel[DanceMoveRows];
	int moveCounter = 0;
	for (int row = 0;row < DanceMoveRows; row++) {
		dancingModePanel[row] = new GLUI_Panel(interactivePanel,"Dancing Mode Panel", GLUI_PANEL_RAISED);

		currentDancingModeWidget[row] =  new GLUI_RadioGroup(dancingModePanel[row], dancingModeLiveVar + row, row, currentDancingMoveCallback);

		if (row == 0)
			new GLUI_RadioButton(currentDancingModeWidget[0], Move::getMove(Move::NO_MOVE).getName().c_str());

		while (moveCounter < MoveMaker::getInstance().getNumMoves()/DanceMoveRows*(row+1)) {
			Move& move = Move::getMove((Move::MoveType)moveCounter);
			new GLUI_RadioButton(currentDancingModeWidget[row], move.getName().c_str());

			moveCounter++;
		}

		// fill up with empty lines to have the containers of the same height
		if (row == DanceMoveRows-1) {
			for (int lines = moveCounter;lines < MoveMaker::getInstance().getNumMoves()+1;lines++)
				new GLUI_StaticText(dancingModePanel[row],"");
		}

		// add a column for next row
		if (row < DanceMoveRows)
			windowHandle->add_column_to_panel(interactivePanel, false);

	}

	GLUI_Panel* sequenceModePanel= new GLUI_Panel(interactivePanel,"Sequence Mode", GLUI_PANEL_RAISED);
	text = new GLUI_StaticText(sequenceModePanel,"Sequence Move");
	text->set_alignment(GLUI_ALIGN_LEFT);

	currentSequenceModeWidget =  new GLUI_RadioGroup(sequenceModePanel, &currentSequenceModeLiveVar, 0, currentSequenceModeCallback);
	new GLUI_RadioButton(currentSequenceModeWidget, "Automated Sequence");
	new GLUI_RadioButton(currentSequenceModeWidget, "Selected Dance Move");

	return windowHandle;
}


bool WindowController::setup(int argc, char** argv) {
	glutInit(&argc, argv);

	// start the initialization in a thread so that this function returns
	// (the thread runs the endless GLUT main loop)
	// main thread can do something else while the UI is running
	eventLoopThread = new std::thread(&WindowController::UIeventLoop, this);

	// wait until UI is ready
	unsigned long startTime  = millis();
	do { delay_ms(10); }
	while ((millis() - startTime < 20000) && (!uiReady));

	return uiReady;
}

void WindowController::tearDown() {
	glutExit();
}

// Idle callback is called by GLUI when nothing is to do.
void idleCallback( void )
{
	const milliseconds emergencyRefreshRate = 1000; 		// refresh everything once a second at least due to refresh issues


	milliseconds now = millis();
	static milliseconds lastDisplayRefreshCall = millis();

	// update all screens once a second in case of refresh issues (happens)
	if ((now - lastDisplayRefreshCall > emergencyRefreshRate)) {
		WindowController::getInstance().mainBotView.postRedisplay();

		setDancingMoveWidget();
	}
}


void WindowController::UIeventLoop() {

	glutInitWindowSize(WindowWidth, WindowHeight);
    wMain = glutCreateWindow("Private Dancer"); // Create a window with the given title
	glutInitWindowPosition(20, 20); // Position the window's initial top-left corner
	glutDisplayFunc(displayMainView);
	glutReshapeFunc(reshape);

	GLUI_Master.set_glutReshapeFunc( GluiReshapeCallback );
	GLUI_Master.set_glutIdleFunc( idleCallback);

	wMainBotView= mainBotView.create(wMain,"");

	// Main Bot view has comprehensive mouse motion
	glutSetWindow(wMainBotView);

	// double buffering
	glutInitDisplayMode(GLUT_DOUBLE);

	// initialize all widgets
	createInteractiveWindow(wMain);

	uiReady = true; 							// flag to tell calling thread to stop waiting for ui initialization
	glutMainLoop();
}

