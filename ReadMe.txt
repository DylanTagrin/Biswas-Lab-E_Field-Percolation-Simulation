This code has been created in collaboration with members of Amlan Biswas's research group at the University of Florida
The primary contributors are Dylan Tagrin, Nicole Burg, and Mathew Shapiro.

This file documents how this code works and how it can be used to run a simulation.

This code utilizes multiple header (.h) files to contain the functions for the simulation.
The main simulation is run in main.cpp, while the main logic for the simulation is in Simulation.h

~~~~~~~~ How To Use This Program ~~~~~~~~
Compile the main.cpp file to get main.exe
Your debugger of choice will probably ask you do perform a run during compilation.
Once you have the .exe you can just run that for each new run.
Run main.exe and a terminal prompt will appear with multiple questions regarding what
type of simulation you would like to perform.
The terminal will display information regarding the simulation as it is running.
Once the simulation is complete it will end with "Done!"
Output data.json will be put into the output folder.

Once the simulation is done, the data can be analyzed with data_analysis.py 
Make sure whatever functions you want to run/data you want plotted are uncommented in the 
final function __main__
Data will plotted in the output directory, please move it out if you dont want it overwritten.

gif_converter.py can be used to convert mp4s to a gif for presentations.