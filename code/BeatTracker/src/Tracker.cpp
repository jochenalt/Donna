//============================================================================
// Name        : Tracker.cpp
// Author      : 
// Version     :
// Copyright   : Your copyright notice
// Description : Hello World in C++, Ansi-style
//============================================================================

#include <iostream>

#include <stdlib.h>
#include <chrono>
#include <unistd.h>
#include <iomanip>
#include <thread>

#include "BTrack.h"
#include "AudioFile.h"

using namespace std;

void delay_ms(long ms) {
	if (ms > 0)
		std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}


uint32_t millis() {
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

int main() {


	AudioFile<double> audioFile;
	audioFile.load ("/home/jochenalt/dancedetector/UpbeatFunk.wav");

	int sampleRate = audioFile.getSampleRate();
	int bitDepth = audioFile.getBitDepth();

	int numSamples = audioFile.getNumSamplesPerChannel();
	double lengthInSeconds = audioFile.getLengthInSeconds();

	int numChannels = audioFile.getNumChannels();
	bool isMono = audioFile.isMono();
	bool isStereo = audioFile.isStereo();

	// or, just use this quick shortcut to print a summary to the console
	audioFile.printSummary();

	int hopSize = 512;
	int frameSize = hopSize*4;
	BTrack b(hopSize, frameSize);



	int position = 0;
	double elapsedTime = 0;
	uint32_t startTime_ms = millis();

	while (position < numSamples) {
		double frame[hopSize];
		int sampleLen = min(hopSize, numSamples);
		for (int i = 0; i < sampleLen; i++)
		{
			if (isStereo)
				frame[i] = (audioFile.samples[0][position + i] + audioFile.samples[1][position + i]) / 2.0;
			else
				frame[i] = (audioFile.samples[0][position + i]);
		}
		position += sampleLen;

		b.processAudioFrame(frame);
		elapsedTime = ((double)(millis() - startTime_ms)) / 1000.0f;
		double elapsedFrameTime = (double)position / (double)sampleRate;

		delay_ms((elapsedFrameTime - elapsedTime)*1000.0);

		if (b.beatDueInCurrentFrame())
		{
			cout << std::fixed << std::setprecision(1) << "Beat (" << b.getCurrentTempoEstimate() << ")" << std::setprecision(1) << (elapsedFrameTime) << "s" << endl;
		};
	}
}
