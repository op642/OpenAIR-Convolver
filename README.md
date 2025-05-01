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

Ppening project through Projucer is recommended.

![Screenshot 2025-05-01 at 14 27 47](https://github.com/user-attachments/assets/26333ada-e4fd-450a-8581-5ad8458392ec)

