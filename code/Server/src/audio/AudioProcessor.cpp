/*
 * AudioProcessor.cpp
 *
 *  Created on: Mar 18, 2018
 *      Author: Jochen Alt
 */

#include <audio/AudioFile.h>
#include <iostream>
#include <string.h>

#include <stdlib.h>
#include <signal.h>
#include <chrono>
#include <unistd.h>
#include <iomanip>
#include <thread>
#include <string>
#include <unistd.h>
#include <iomanip>
#include <algorithm>

#include <basics/stringhelper.h>
#include <basics/util.h>
#include <audio/AudioProcessor.h>
#include <audio/MicrophoneInput.h>
#include <audio/Playback.h>

#include <dance/RhythmDetector.h>
#include <dance/Dancer.h>

#include <beat/BTrack.h>

using namespace std;

AudioProcessor::AudioProcessor() {
	beatCallback = NULL;
}

AudioProcessor::~AudioProcessor() {
}

void AudioProcessor::setup(BeatCallbackFct newBeatCallback) {
    beatCallback = newBeatCallback;
	inputAudioDetected = false;

	// music detection requires 1s of music before flagging it as music
	beatScoreFilter.init(100);

	calibrateLatency();
}

void generateSinusoidTone(double buffer[], int bufferSize, float sampleRate, int numOfFrequencies, float tonefrequency[]) {
    for(int i=0; i<bufferSize; ++i) {
    	buffer[i] = 0;
    	for (int j = 0;j<numOfFrequencies;j++)
    		buffer[i] += 1.0/numOfFrequencies*sin( (2.f*M_PI*tonefrequency[j])/sampleRate * (float)i );
    }
}

void AudioProcessor::calibrateLatency() {
	// calibration is done by sending a sinusoid tone via the loudspeaker and
	// receiving it via microphone. The latency in between is measured.
	float measuredLatency = 0;

	playback.setup(MicrophoneSampleRate);
	microphone.setup(MicrophoneSampleRate);


	// measurement is repeated if not successful
	int tries = 0;

	const float length_s = 0.150; 					// [s], length of test tone
	int bufferSize = length_s*MicrophoneSampleRate; // length of test tone buffer
	double buffer[bufferSize];						// buffer containing the test tone
	const int numOfTestFrequencies = 4;				// test tone consists of several frequencies in case the microphone is really poor

	float testFrequency[numOfTestFrequencies];		// test frequencies are generated by thirds starting at 440Hz
	int fftBin[numOfTestFrequencies];				// index of the fft's bin per test frequency
	const int fftLength = 512;						// input buffer size of fft
	int inputBufferSize = fftLength;				// input buffer size of microphone
	double inputBuffer[inputBufferSize];			// buffer for microphone

	// generate the test frequencies
	cout << "calibrating microphone with test frequencies ";
	for (int i = 0;i< numOfTestFrequencies;i++) {
		if (i == 0)
			testFrequency[i] = 440;	// [Hz]
		else
			testFrequency[i] =2.0*sqrt(2)*testFrequency[i-1];
		assert (testFrequency[i]  <  MicrophoneSampleRate/2);

		fftBin[i] = testFrequency[i]/ (MicrophoneSampleRate / fftLength);

		// adapt test frequency to hit the middle of an fft bin
		testFrequency[i]= ((float)fftBin[i]) * (MicrophoneSampleRate / fftLength);
		cout << testFrequency[i] << "Hz" << string((i<numOfTestFrequencies-1)?string(", "):string(""));
	}
	cout << ". Be quiet." << endl;

	do {
		// sum up all test frequencies
		generateSinusoidTone(buffer, bufferSize , MicrophoneSampleRate, numOfTestFrequencies, testFrequency);

		// empty the alsa microphone buffer by reading
		microphone.readMicrophoneInput(inputBuffer, inputBufferSize);

		// play the test tone asynchronously
		playback.play(1.0, buffer, bufferSize);
		uint32_t start_ms = millis();
		uint32_t elapsedTime = 0;

		// do an fft to check for test frequencies
		double testThreshold = 0.0;

		// dont wait longer than two seconds for the test tone
		while ((millis() - start_ms < 1000) && (measuredLatency < floatPrecision)){
			kiss_fft_cpx fftIn [fftLength];
			kiss_fft_cpx fftOut[fftLength];
			kiss_fft_cfg cfgForwards = kiss_fft_alloc (fftLength, 0, 0, 0);
			kiss_fft_cfg cfgBackwards = kiss_fft_alloc (fftLength, 1, 0, 0);

			// get input from microphone
			bool ok = microphone.readMicrophoneInput(inputBuffer, inputBufferSize);
			if (ok) {
				elapsedTime += 1000.0*inputBufferSize/MicrophoneSampleRate;

				// copy microphone buffer into complex array and zero padding as input for fft
				for (int i = 0;i < inputBufferSize;i++)
				{
					if (i<inputBufferSize)
						fftIn[i].r = inputBuffer[i];
					else
						fftIn[i].r = 0;
					fftIn[i].i = 0.0;
				}

				// execute fft
				kiss_fft (cfgForwards, fftIn, fftOut);

				// multiply by complex conjugate
				for (int i = 0;i < fftLength;i++) {
					fftOut[i].r = fftOut[i].r * fftOut[i].r + fftOut[i].i * fftOut[i].i;
					fftOut[i].i = 0.0;
				}

				// perform the inverse fft
				kiss_fft (cfgBackwards, fftOut, fftIn);

				// compute the energy of the bands of the test-frequencies
				double testFrequencies = 0;
				for (int i = 0;i<numOfTestFrequencies; i++) {
					testFrequencies += fftOut[fftBin[0]].r;
				}

				// first measurement is used to detect the background energy
				if (testThreshold == 0)
					testThreshold = testFrequencies;

				// once the energy of the test frequencies is rising by a magnitude, we are obviously hearing the test tone
				if (testFrequencies > testThreshold*8.) {
					// cout << "t=" << millis() - start_ms << " " << " elapsed=" << elapsedTime << " ms";;
					// cout << std::fixed << std::setprecision(3) << fftBin[0] << ":" << fftOut[fftBin[0]].r << " " << fftBin[1] << ":" << fftOut[fftBin[1]].r << endl;
					testThreshold = testFrequencies;
					measuredLatency = (float)elapsedTime/1000.0;
				} else
					if (testFrequencies > testThreshold)
						testThreshold = testFrequencies;
				}
				free (cfgForwards);
				free (cfgBackwards);
			}

	} while ((measuredLatency == 0) && (tries++ < 3));
    if (measuredLatency > 0)
    	cout << "measured latency = " << measuredLatency << "s" << endl;
}

void AudioProcessor::setVolume(double newVolume) {
	volume = newVolume;
}
double AudioProcessor::getVolume() {
	return volume;
}

void AudioProcessor::setPlayback(bool ok) {
	playback.setPlayback(ok);
}

bool AudioProcessor::getPlayback() {
	return playback.getPlayback();
}

void AudioProcessor::setWavContent(std::vector<uint8_t>& newWavData) {
	// set new data and indicate to change the source
	nextWavContent = newWavData;

	// indicate that current processing is to be stopped
	stopCurrProcessing = true;

	nextInputType = WAV_INPUT;
}

void AudioProcessor::setAudioSource() {

	if (nextInputType == WAV_INPUT) {
		// likely a new rythm
		RhythmDetector::getInstance().setup();

		// current input is wav data
		currentInputType = WAV_INPUT;

		// reset position in wav content to start
		wavInputPosition = 0;

		beatScoreFilter.set(0);

		// re-initialize dancing when new input source is detected
		Dancer::getInstance().setup();

		// read in the wav data and set index pointer to first position
		currentWavContent.decodeWaveFile(nextWavContent);

		// playback is done with same sample rate like the input wav data
		playback.setup(currentWavContent.getSampleRate());

		// clear input, has been saved
		nextWavContent.clear();

		cout << "switching audio source to wav input" << endl;
	}
	if (nextInputType == MICROPHONE_INPUT) {
		currentInputType = MICROPHONE_INPUT;

		// playback is set to standard sample rate
		playback.setup(MicrophoneSampleRate);

		// initialize the
		microphone.setup(MicrophoneSampleRate);

		cout << "switching to microphone input" << endl;
	}

	// do not switch source again until explicitely set
	nextInputType = NO_CHANGE;

}
void AudioProcessor::setMicrophoneInput() {
	// flag that processing loop should stop
	stopCurrProcessing = true;

	// current input is microphone
	nextInputType = MICROPHONE_INPUT;
}

int AudioProcessor::readWavInput(double buffer[], unsigned BufferSize) {
	int numSamples = currentWavContent.getNumSamplesPerChannel();
	int numInputSamples = min((int)BufferSize, (int)numSamples-wavInputPosition);
	int numInputChannels = currentWavContent.getNumChannels();

	int bufferCount = 0;
	for (int i = 0; i < numInputSamples; i++)
	{
		double inputSampleValue = 0;
		inputSampleValue= currentWavContent.samples[0][wavInputPosition + i];
		assert(wavInputPosition+1 < numSamples);
		switch (numInputChannels) {
		case 1:
			inputSampleValue= currentWavContent.samples[0][wavInputPosition + i];
			break;
		case 2:
			inputSampleValue = (currentWavContent.samples[0][wavInputPosition + i]+currentWavContent.samples[1][wavInputPosition + i])/2;
			break;
		default:
			inputSampleValue = 0;
			for (int j = 0;j<numInputChannels;j++)
				inputSampleValue += currentWavContent.samples[j][wavInputPosition + i];
			inputSampleValue = inputSampleValue / numInputChannels;
		}
		buffer[bufferCount++] = inputSampleValue;
	}
	wavInputPosition += bufferCount;
	return bufferCount;
}

void AudioProcessor::processInput() {
	// reset flag that would stop the loop (is set from the outside)
	stopCurrProcessing = false;

	// hop size is the number of samples that will be fed into beat detection
	const int hopSize = 256; // approx. 3ms at 44100Hz

	// number of samples to be read
	const int numInputSamples = hopSize;

	// framesize is the number of samples that will be considered in this loop
	// cpu load goes up linear with the framesize
	int frameSize = hopSize*8;

	// initialize beat detector
	BTrack beatDetector(hopSize, frameSize);

	// start time used for delays and output
	uint32_t startTime_ms = millis();

	// buffer for audio coming from wav or microphone
	double inputBuffer[numInputSamples];
	int inputBufferSamples  = 0;

	int sampleRate = 0;
	while (!stopCurrProcessing) {
		if (currentInputType == MICROPHONE_INPUT) {
			inputBufferSamples = microphone.readMicrophoneInput(inputBuffer, numInputSamples);
			sampleRate = MicrophoneSampleRate;
		}
		if (currentInputType == WAV_INPUT) {
			inputBufferSamples = readWavInput(inputBuffer, numInputSamples);

			sampleRate = currentWavContent.getSampleRate();
			if (inputBufferSamples < numInputSamples) {
				cout << "end of song. Switching to microphone." << endl;
				beatDetector.initialise(hopSize, frameSize);

				// use microphone instead of wav input
				setMicrophoneInput();
				setAudioSource();
			}
		}

		// play the buffer of hopSize asynchronously
		playback.play(volume, inputBuffer,numInputSamples);

		// detect beat and bpm of that hop size
		beatDetector.processAudioFrame(inputBuffer);
		bool beat = beatDetector.beatDueInCurrentFrame();
		double bpm = beatDetector.getCurrentTempoEstimate();

		if (beat){
			cout << std::fixed << std::setprecision(2) << "Beat (" << beatDetector.getCurrentTempoEstimate() << ")"  << endl;
	    };

		// check if the signal is really music. low pass scoring to ensure that small pauses are not
		// misinterpreted as end of music
		double score = beatDetector.getLatestCumulativeScoreValue();
		beatScoreFilter.set(score);
		const double scoreThreshold = 10.;
		inputAudioDetected = (beatScoreFilter >= scoreThreshold);

		processedTime = millis()/1000.0;
		beatCallback(beat, bpm);

		if (currentInputType == WAV_INPUT) {
			// insert a delay to synchronize played audio and beat detection before entering the next cycle
			double elapsedTime = ((double)(millis() - startTime_ms)) / 1000.0f;  	// [s]
			double processedTime = (double)wavInputPosition / (double)sampleRate;	// [s]
			// wait such that elapsed time and processed time is synchronized
			double timeAhead_ms = (processedTime - elapsedTime)*1000.0;
			if (timeAhead_ms > 1.0) {
				delay_ms(timeAhead_ms);
			}
		}
	}
	// check if the source needs to be changed
	setAudioSource();
}

float AudioProcessor::getLatency() {
	if (currentInputType == MICROPHONE_INPUT)
		return microphone.getMicrophoneLatency();
	else
		return 0.5;
}

