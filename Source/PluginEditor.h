/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "AudioFFT.h"
#define fftSIZE 2048
#define halfSIZE 1024
//==============================================================================
/**
*/
class SampleChipperAudioProcessorEditor  : public juce::AudioProcessorEditor, public juce::FileDragAndDropTarget, juce::Timer
{
public:
    SampleChipperAudioProcessorEditor (SampleChipperAudioProcessor&);
    ~SampleChipperAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;
    bool isInterestedInFileDrag (const juce::StringArray& files) override;
    void filesDropped (const juce::StringArray &files, int x, int y) override;
    void fileDragEnter (const juce::StringArray& files, int x, int y) override{
        if(isInterestedInFileDrag(files)){
            hover.store(1);
            repaint();
        }
    }
    void fileDragMove (const juce::StringArray& files, int x, int y) override{
        if(isInterestedInFileDrag(files)){
            hover.store(1);
            repaint();
        }
    }
    void fileDragExit (const juce::StringArray& files) override{
        if(isInterestedInFileDrag(files)){
            hover.store(0);
            repaint();
        }
    }
    void timerCallback() override;
    // Fast FFT
    audiofft::AudioFFT FFT;
    // juce fft slower
    juce::dsp::FFT forwardFFT;
    
    //Loads the sample and meta data
    juce::AudioBuffer<float> waveform;
    unsigned int actualSamples = 0;
    unsigned int numSamples = 0;
    unsigned int power = 0;
    unsigned int padding = 64;
    //Ouptut image
    juce::Image spectrogram;
    
    // Files
    juce::File droppedFileLocation;
    juce::File outputImage;
private:
    // awesome background explaining how to drag and drop samples
    juce::Image sampleChip;
    juce::Image spectroImage;

    // interger for user drag display box
    std::atomic<int> hover{0};
    bool startCount = false;
    int hoverCounter = 0;
    juce::Rectangle<int> displayBox{0,0,720,405};
    
    
    // For reading wav files
    juce::AudioFormatManager mFormatManager;
    std::unique_ptr<juce::AudioFormatReader> mFormatReader { nullptr };
    
    
    // Process the image
    
    void createImage();
    
    
    // convert real and imaginery into magnitude
    
    void convertToMagnitude(float* magnitude, const float* real, const float* imag, int size) {
        for (int i = 0; i < size; ++i) {
            magnitude[i] = std::sqrt(real[i] * real[i] + imag[i] * imag[i]);
        }
    }
   
    
    SampleChipperAudioProcessor& audioProcessor;
    
    // ============ { NOT USED} ========= \\
    
    // This is so the FFT always lines up
    /*
     unsigned int actualNumber = static_cast<unsigned int>(std::pow(2, exponent));
     */
    
    unsigned int closestPowerOf2(unsigned int num) {
        if (num <= 1) {
            return 1;  // 2^0 is the smallest power of 2
        }

        unsigned int power = 1;
        while (power < num) {
            power <<= 1;  // Left shift to multiply by 2
        }

        // Check the difference between the found power and the next lower power
        unsigned int lowerPower = power >> 1;
        if (num - lowerPower < power - num) {
            return lowerPower;
        } else {
            return power;
        }
    }
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SampleChipperAudioProcessorEditor)
};



