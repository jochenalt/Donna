
#include <stdlib.h>
#include <chrono>
#include <unistd.h>
#include <iomanip>

#include "basics/util.h"
#include <cstdarg>
#include <iostream>
#include <sstream>
#include <thread>
#include <chrono>

#include "basics/stringhelper.h"

void ExclusiveMutex::lock() {
	isInBlock = true;
};

void ExclusiveMutex::unlock() {
	isInBlock = false;
};

bool ExclusiveMutex::isLocked() {
	return !isInBlock;
};
void ExclusiveMutex::waitAndLock() {
	while (isInBlock) delay_ms(1);
	lock();
};

ExclusiveMutex::ExclusiveMutex() {
	isInBlock = false;
}
ExclusiveMutex::~ExclusiveMutex() {
}

CriticalBlock::CriticalBlock (ExclusiveMutex & newMutex) {
	mutex = &newMutex;
	mutex->waitAndLock();
};

CriticalBlock::~CriticalBlock() {
	mutex->unlock();
};

void CriticalBlock::waitAndLock() {
	mutex->waitAndLock();
}


float roundValue(float x) {
	float roundedValue = sgn(x)*((int)(abs(x)*10.0f+.5f))/10.0f;
	return roundedValue;
}

realnum arctanApprox(realnum x) {
	return 3.0*x/(3.0 + x*x);
}

bool hasPrefix(string str, string prefix) {
	int idx = upcase(str).find(upcase(prefix));
	return (idx>=0);
}


long mapLong(long x, long in_min, long in_max, long out_min, long out_max)
{
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

int randomInt(int min,int max) {
	int r = rand() % (max-min) + min;
	return r;
}

realnum randomFloat (realnum a, realnum b) {
	return randomInt(a*1000, b*1000)/1000.;
}

bool randomBool() {
	return randomInt(0,100)>50;
}

int randomPosNeg() {
	return (randomInt(0,100)>50)?+1:-1;
}

milliseconds millis() {
	/*
    static uint32_t clockPerMs = (CLOCKS_PER_SEC)/1000;
    uint32_t c = clock();
    return c/clockPerMs;
    */
    static auto epoch = std::chrono::high_resolution_clock::from_time_t(0);
    auto now   = std::chrono::high_resolution_clock::now();
    auto mseconds = std::chrono::duration_cast<std::chrono::milliseconds>(now - epoch).count();
    return mseconds;
}


seconds secondsSinceEpoch() {
    static auto epoch = std::chrono::high_resolution_clock::from_time_t(0);
    auto now   = std::chrono::high_resolution_clock::now();
    auto mseconds = std::chrono::duration_cast<std::chrono::milliseconds>(now - epoch).count();
    return mseconds/1000.0;
}

realnum lowpass (realnum oldvalue, realnum newvalue, realnum tau, realnum dT) {
	realnum alpha = tau/(tau+dT);
	realnum result = newvalue*(1.0-alpha) + oldvalue * alpha;
	return result;
}

// use polynom 3rd grade with (f(0) = 0, f(1) = 01 f'(0) = grade, f'(1) = 0)
// input is number between 0..1, output is number between 0..1 but moving to 1 quicker in the beginning
// grade has to be <=3
realnum moderate(realnum input, realnum grade) {
	return (grade-2.0)*input*input*input + (-2.0*grade+3.0)*input*input + grade*input;
}

// function with f(0) = 0, f(1) = 0, f'(0) = 0, in between the function goes down and accelerates
realnum speedUpAndDown(realnum input) {
	return 3*input*input*input - 2*input*input;
}

realnum ellipseCircumference(realnum a, realnum b) {
	realnum lambda = (a-b)/(a+b);
	return (a+b)*M_PI*(1.0 + 3.0*lambda*lambda/(10.0 + sqrt(4.0-3.0*lambda*lambda)));
}



void delay_ms(long ms) {
	if (ms > 0)
		std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

void delay_us(long us) {
	usleep(us);
}


realnum radians(realnum degrees) {
	const realnum fac = (M_PI / 180.0);
	return degrees * fac;
}

realnum  degrees(realnum radians) {
	const realnum fac = (180.0 / M_PI);
	return radians * fac;
}

// cosine sentence
realnum triangleAlpha(realnum a, realnum b, realnum c) {
	realnum x = acos((a*a-b*b-c*c)/(-2.0*b*c));
    return x;
}

// cosine sentence
realnum triangleGamma(realnum a, realnum b, realnum c) {
	return triangleAlpha(c,b,a);
}

// abc formula, root of 0 = a*x*x + b*x + c;
bool polynomRoot2ndOrder(realnum a, realnum b, realnum c, realnum& root0, realnum& root1)
{
	realnum disc = b*b-4.0*a*c;
	if (disc>=0) {
		root0 = (-b + sqrt(disc)) / (2.0*a);
		root1 = (-b - sqrt(disc)) / (2.0*a);
		return true;
	}
	return false;
}


bool almostEqual(realnum a, realnum b, realnum precision) {
	if (a==b)
		return true;
	if (a == 0)
		return (abs(b)<precision);
	if (b == 0)
		return (abs(a)<precision);

	if (b<a)
		return (abs((b/a)-1.0) < precision);
	else
		return (abs((a/b)-1.0) < precision);

}

#include <unistd.h>
#include <termios.h>
#include <stdio.h>
#include <termios.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/time.h>

char getch() {
	        char buf = 0;
	        struct termios old = {0};
	        if (tcgetattr(0, &old) < 0)
	                perror("tcsetattr()");
	        old.c_lflag &= ~ICANON;
	        old.c_lflag &= ~ECHO;
	        old.c_cc[VMIN] = 1;
	        old.c_cc[VTIME] = 0;
	        if (tcsetattr(0, TCSANOW, &old) < 0)
	                perror("tcsetattr ICANON");
	        if (read(0, &buf, 1) < 0)
	                perror ("read()");
	        old.c_lflag |= ICANON;
	        old.c_lflag |= ECHO;
	        if (tcsetattr(0, TCSADRAIN, &old) < 0)
	                perror ("tcsetattr ~ICANON");
	        return (buf);
}



void changemode(int dir)
{
	static int existingDir = 0;
  static struct termios oldt, newt;

  if (dir == existingDir )
	  return;

  if ( dir == 1 )
  {
    tcgetattr( STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~( ICANON | ECHO );
    tcsetattr( STDIN_FILENO, TCSANOW, &newt);
  }
  else
    tcsetattr( STDIN_FILENO, TCSANOW, &oldt);
}

int kbhit (void)
{
  struct timeval tv;
  fd_set rdfs;

  tv.tv_sec = 0;
  tv.tv_usec = 0;

  FD_ZERO(&rdfs);
  FD_SET (STDIN_FILENO, &rdfs);

  select(STDIN_FILENO+1, &rdfs, NULL, NULL, &tv);
  return FD_ISSET(STDIN_FILENO, &rdfs);

}


ErrorCodeType glbError = ErrorCodeType::ABSOLUTELY_NO_ERROR;

void resetError() {
	glbError = ErrorCodeType::ABSOLUTELY_NO_ERROR;
}

ErrorCodeType getLastError() {
	return glbError;
}

void setError(ErrorCodeType err) {
	// set error only if error has been reset upfront
	if (glbError == ErrorCodeType::ABSOLUTELY_NO_ERROR)
		glbError = err;
}

bool isError() {
	return glbError != ErrorCodeType::ABSOLUTELY_NO_ERROR;
}

std::string getErrorMessage(ErrorCodeType err) {
	std::ostringstream msg;
	switch (err) {
	case ABSOLUTELY_NO_ERROR: msg << "no error";break;

	// hostCommunication
	case FILE_NOT_FOUND: 			msg << "file not found";break;
	case UNKNOWN_ERROR: 			msg << "mysterious error";break;

	default:
		msg << "unknown error message";
	}
	msg << " (" << (int)err << ")";

	return msg.str();
}
