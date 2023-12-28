/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
SampleChipperAudioProcessorEditor::SampleChipperAudioProcessorEditor (SampleChipperAudioProcessor& p)
: AudioProcessorEditor (&p),forwardFFT(log2 (2048)), audioProcessor (p)
{
    mFormatManager.registerBasicFormats();
    FFT.init(fftSIZE);
    // Make sure that before the constructor has finished, you've set the
    // editor's size to whatever you need it to be.
    sampleChip = juce::ImageCache::getFromMemory(BinaryData::samplechipper_png, BinaryData::samplechipper_pngSize);
    setSize (720, 405);
    startTimer(20);
}

SampleChipperAudioProcessorEditor::~SampleChipperAudioProcessorEditor()
{
    mFormatReader = nullptr;
}

//==============================================================================
void SampleChipperAudioProcessorEditor::paint (juce::Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));

    g.drawImage(sampleChip, 0, 0, 720, 405, 0, 0, sampleChip.getWidth(), sampleChip.getHeight());
    
    switch(hover.load()){
        case 0:
            break;
        case 1: // hover
            g.setColour(juce::Colours::blue);
            g.drawRect(displayBox,4.0f);
            break;
        case 2: // dropped
            g.setColour(juce::Colours::red);
            g.drawRect(displayBox,4.0f);
            break;
        case 3: // processed
            g.setColour(juce::Colours::green);
            g.drawRect(displayBox,4.0f);
            break;
        default:
            break;
    }
}

void SampleChipperAudioProcessorEditor::resized()
{
    // This is generally where you'll want to lay out the positions of any
    // subcomponents in your editor..
}

bool SampleChipperAudioProcessorEditor::isInterestedInFileDrag(const juce::StringArray &files)
{
    for (auto file : files) {
        if (file.contains(".wav") || file.contains(".mp3") || file.contains(".aif")) {
            return true;
        }
    }
    return false;
}

void SampleChipperAudioProcessorEditor::filesDropped(const juce::StringArray &files, int x, int y)
{
    for (auto file : files) {
        if (isInterestedInFileDrag (file)){
            
            auto tmpFile = std::make_unique<juce::File>(file);
            juce::String filename = tmpFile->getFileNameWithoutExtension();
            
            // files recieved
            startCount = false;
            hoverCounter = 0;
            hover.store(2);
            repaint();
            //================ process load wavetable ==============
            mFormatReader.reset(mFormatManager.createReaderFor (file));
            outputImage = tmpFile->getParentDirectory().getChildFile(filename + ".png");
            numSamples = mFormatReader->lengthInSamples;
            // only processes mono or left channel as of now.
            waveform.setSize(1, numSamples);
            mFormatReader->read(&waveform, 0, numSamples, 0, true, false);
            
            //================ create image ==============
            createImage();
            // files processed
            hover.store(3);
            repaint();
            startCount = true;
            hoverCounter = 0;
        }
    }

}

void SampleChipperAudioProcessorEditor::timerCallback()
{
    if(startCount){
        if(hoverCounter == 100){
            startCount = false;
           
            hover.store(0);
            repaint();
        }
        hoverCounter++;
    }

}


void SampleChipperAudioProcessorEditor::createImage()
{
    // Image
    int imageWidth = (numSamples - fftSIZE) / (padding*2);
    if(imageWidth > 256){
        imageWidth = 256;
    }
    int imageHeight = fftSIZE * 0.5f;
    
   // std::array<float, fftSIZE * 2> fftData;
    // FFT arrays
    float data[fftSIZE];
    float real[fftSIZE];
    float imaginery[fftSIZE];
    float magnitude[fftSIZE];
    
    auto input = waveform.getReadPointer(0);
    
    
   
    // clear FFT arrays
    for(int i = 0; i < fftSIZE; ++i){
        data[i] = 0.0f;
        real[i] = 0.0f;
        imaginery[i] = 0.0f;
    }
    

    //forwardFFT.performFrequencyOnlyForwardTransform (fftData.data(),true);
    

    
    // draw to image
    spectroImage = juce::Image(juce::Image::PixelFormat::ARGB, imageWidth, imageHeight, true);
    
    juce::Graphics g (spectroImage);
    {
        g.fillAll (juce::Colours::black);

    }
    int startSample = 0;
    for(int x = 0; x < imageWidth; ++x){
        
    
        juce::FloatVectorOperations::copy(&data[padding], &input[startSample], (int)(fftSIZE - (padding * 2)));
        
        FFT.fft(data, real, imaginery);
        
       convertToMagnitude(magnitude, real, imaginery, fftSIZE);
        
        
        auto maxMagnitude = juce::FloatVectorOperations::findMinAndMax (magnitude, fftSIZE / 2);
        
        
        for (int y = 0; y < halfSIZE; ++y)
        {
            float mag = magnitude[y];
            float normalizedMagnitude = mag / maxMagnitude.getEnd();
            juce::Colour color = juce::Colours::white.withAlpha(normalizedMagnitude);
            spectroImage.setPixelAt(x, y, color);
        }
        
        startSample += 64;
    }
   

    // Output image file to the location of the .wav
    juce::PNGImageFormat pngFormat;
        juce::FileOutputStream stream(outputImage);
        pngFormat.writeImageToStream(spectroImage, stream);
}
