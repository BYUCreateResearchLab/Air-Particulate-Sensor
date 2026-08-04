Quantifying Powder Dispersion 
During Binder Jetting
Additive Manufacturing

By Sam Erickson

This work develops a novel method of quantifying the powder dispersion caused by binder jetting processes.
This folder includes work relevant to one of the methods of measurement: airborne particulate sensors.

The airborne particulate concentrations are measured using a custom system that I designed. The system is designed using Autodesk Fusion 360 and programmed using Arduino IDE on an ESP32 dev board. The custom system records the particle concentration per 0.1 L of air at a sampling frequency of 0.8 Hz and logs the data to a ".txt" file on a MicroSD card. The data can then be processed using the included python scripts to evaluate average particulate concentrations separated by particle size ranges (0.3-0.5um, 0.5-1.0um, 1.0-2.5um, 2.5-5.0um, 5.0-10.0um, >10.0um).