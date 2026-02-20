import matplotlib.animation as animation
import matplotlib.pyplot as plt
import numpy as np
import json
import os
from pylab import *

#define path to ffmpeg; change if needed based on location on your PC
plt.rcParams['animation.ffmpeg_path'] = 'C:\\Users\\Matthew\\Documents\\School\\Research\\Simulation\\ffmpeg-master-latest-win64-gpl\\ffmpeg-master-latest-win64-gpl\\bin\\ffmpeg.exe'

class ContourAnimator():
    def __init__(self):
        # Enables assertion to check for initialized figure.
        self.fig = None

    # Function which sets up figure, axes, 
    # and any values used in all animations.
    def SetupPlot(self, levels, linewidths, color, x_title, y_title):
        self.fig = plt.figure()
        self.ax = self.fig.add_subplot(111)
        self.ax.set_xlabel(x_title)
        self.ax.set_ylabel(y_title)
        self.levels = levels
        self.color = color
        self.linewidths = linewidths
        print("Plot successfully setup.")

    # Internal function which interprets the JSON data
    # and sets up class variables required for animation.
    def LoadFile(self, filepath):
        with open(filepath) as json_file:
            print('Loading input file: "' + filepath + '"')
            j = json.load(json_file)
            self.data = np.asarray(j.get("data"))
            self.frames = len(self.data)
            # Reshape data for contour plot.
            self.data = np.reshape(self.data, (self.frames, j.get("height"), j.get("width")))
            # Setup initial contour plot.
            self.contour = plt.contour(self.data[0], levels=self.levels, cmap=self.color, linewidths=self.linewidths)

    # input - Path for input JSON file (must end in .json).
    # output - Path to save MP4 animation to (must end in .mp4).
    # fps - Framerate of MP4 animation.
    # bitrate - Bitrate of the MP4 in kilobits per second (-1 for standard).
    def SaveToMP4(self, input, output, fps, bitrate=-1):
        assert self.fig is not None, "Please setup plot before calling SaveToMP4"
        self.LoadFile(input)
        # Setup animation object.
        self.anim = animation.FuncAnimation(self.fig, self.UpdateContour, self.frames, init_func=self.Init, interval=1, repeat=False)
        print("Computing animation...")
        # Save animation to MP4.
        self.anim.save(output, writer=animation.FFMpegWriter(fps=fps, bitrate=bitrate))
        # Clear contour plots before exiting.
        for c in self.contour.collections:
            c.remove()
        print('Saved animation to: "' + output + '"')

    # Empty function for first call from animation object.
    def Init(self):
        pass

    # Called once per animation frame. 
    # Updates the existing contour with next set of data from JSON. 
    def UpdateContour(self, n):
        # Clear previous contours.
        for c in self.contour.collections:
            c.remove()
        # Reset contour plot.
        self.contour = plt.contour(self.data[n], levels=self.levels, cmap=self.color, linewidths=self.linewidths)
        # Update plot title.
        plt.title("Frame: " + str(n))
        # Print progress in percentages.
        print(str(int(round((n + 1) / self.frames * 100, 0))) + "% complete...")

animator = ContourAnimator()

animator.SetupPlot(levels=1000, \
                   linewidths=0.5, \
                   color=plt.cm.magma, \
                   x_title="X (micrometers)", \
                   y_title="Y (micrometers)")
       
#change data file path if needed
data_location = os.getcwd() + '\data_run2.json'
animator.SaveToMP4(input=data_location, output="video18.mp4", fps=1)
#animator.SaveToMP4(input="data2.json", output="demo2.mp4", fps=3)