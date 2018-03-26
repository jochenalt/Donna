/*
 * AudioProcessor.h
 *
 *  Created on: Mar 18, 2018
 *      Author: jochenalt
 */

#ifndef SRC_AUDIOPROCESSOR_H_
#define SRC_AUDIOPROCESSOR_H_

#include <audio/AudioFile.h>
#include <audio/Playback.h>
#include <pulse/simple.h>
#include <pulse/error.h>
#include "audio/MicrophoneInput.h"

typedef void (*BeatCallbackFct)(bool beat, double Bpm);


class AudioProcessor {
public:
	AudioProcessor();
	virtual ~AudioProcessor();

	static AudioProcessor& getInstance() {
		static AudioProcessor instance;
		return instance;
	}

	// call setup before anything else
	void setup(BeatCallbackFct beatCallback);

	// set wav content to be processed with next loop
	void setWavContent(std::vector<uint8_t>& wavData);
	void setMicrophoneInput();

	bool isWavContentPending() { return currentInputType == WAV_INPUT; };
	bool isMicrophoneInputPending() { return currentInputType == MICROPHONE_INPUT; };

	// process content of passed wav content or content coming from microphone.
	// returns whenever the current content is empty (valid of wav content only)
	// needs to be called repeatedly.
	void processInput();

	// get/set volume [0..1]
	void setVolume(double newVolume);
	double getVolume();

	// switch playback on or off
	void setPlayback(bool ok);
	bool getPlayback();

	// get current latency of input source
	float getLatency();

	// get processed time relative to input source (wav or microphone)
	double getProcessedTime() { return processedTime; };
private:
	enum InputType { WAV_INPUT, MICROPHONE_INPUT };
	int readMicrophoneInput(float buffer[], unsigned BufferSize);
	int readWavInput(float buffer[], unsigned BufferSize);

	volatile bool stopCurrProcessing = false;
	volatile bool currProcessingStopped = true;

	double volume = 1.0;
	BeatCallbackFct beatCallback;
	Playback playback;					// used to send the input source to the loudspeaker
	MicrophoneInput microphone;			// used to get input from microphone
	AudioFile<double> wavContent;		// used to get input from wav (actually no file, but an array of samples)
	int wavInputPosition = -1;			// current position within wav source
	double processedTime = 0; 			// [s] processing time of input source. Is determined by position within wav file or realtime in case of micropone input
	TimeSamplerStatic callbackTimer; 	// timer for callback as passed via setup()
	InputType currentInputType = MICROPHONE_INPUT;

};

#endif /* SRC_AUDIOPROCESSOR_H_ */
