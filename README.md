# OpenAIR Convolver
## About the Plugin
**OpenAIR Convolver** is a multichannel convolution reverb plugin configured to decode bformat ambisonic impulse responses (IR) into 5.1 surround sound. OpenAIR Convolver uses the JUCE C++ framework to handle 
non-uniform partitioned (NUP) convolution over multiple channels, using multithreading, making convolution of long IR's in realtime possible. This plugin can be deployed as a VST3 or Audio Unit, making 
it compatible with most current DAW's and audio software.

OpenAIR Convolver uses OpenAIR's library of impulse responses, curated through the University of Yorks AudioLab. The focus of the plugin is on the auralisation of sound and displaying the characteristics
of spaces, rather than a music making tool.

OpenAIR can be accessed here:
http://openair.hosted.york.ac.uk

## Requirements
The Plugin will only work at 48kHz Sample Rate.

JUCE v8.0.6 was used during development.

Opening project through Projucer is recommended.

AudioPluginHost can be used to easily test the Plugin from your IDE, alternatively build and open in Reaper.

![Screenshot 2025-05-01 at 14 27 47](https://github.com/user-attachments/assets/26333ada-e4fd-450a-8581-5ad8458392ec)

### For installation put the vst3 or AU (component) in the appropriate place shown in the table below.
![Screenshot 2025-05-02 at 00 36 25](https://github.com/user-attachments/assets/239fd635-2c5d-492d-8fb9-fcdeee999d07)

this table can be found here: https://juce.com/tutorials/tutorial_app_plugin_packaging/
