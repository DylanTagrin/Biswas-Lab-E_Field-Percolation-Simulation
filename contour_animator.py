# Note: This code simply will not work on any system but the original creators until further notice


from calendar import c
import matplotlib.animation as animation
import matplotlib.pyplot as plt
from matplotlib.colors import SymLogNorm
import numpy as np
from scipy.integrate import quad
import json
import os
from pylab import *

#define path to ffmpeg; change if needed based on location on your PC
plt.rcParams['animation.ffmpeg_path'] = 'C:\\Users\\marzm\\Documents\\School\\Research\\Simulation\\ffmpeg-master-latest-win64-gpl\\ffmpeg-master-latest-win64-gpl\\bin\\ffmpeg.exe'

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
    def LoadFile(self, filepath, limit_frames=0):
        with open(filepath) as json_file:
            print('Loading input file: "' + filepath + '"')
            j = json.load(json_file)
            self.data = np.asarray(j.get("data"))
            self.frames = len(self.data)
            # Reshape data for contour plot.
            self.data = np.reshape(self.data, (self.frames, j.get("height"), j.get("width")))
            x_ = np.arange(0, j.get("width"))
            y_ = np.arange(0, j.get("height"))
            if limit_frames != 0:
                self.frames = limit_frames
                self.data = self.data[:limit_frames]
            # Setup initial contour plot.
            self.contour = plt.contour(self.data[0], levels=self.levels, cmap=self.color, linewidths=self.linewidths)
            #self.contour = plt.contourf(x_, y_, self.data[0], levels=self.levels, norm=SymLogNorm(linthresh=10, vmin=0, 
            #                            vmax=np.max(self.data[0])), cmap=self.color, linewidths=self.linewidths)

    # input - Path for input JSON file (must end in .json).
    # output - Path to save MP4 animation to (must end in .mp4).
    # fps - Framerate of MP4 animation.
    # bitrate - Bitrate of the MP4 in kilobits per second (-1 for standard).
    def SaveToMP4(self, input, output, fps, bitrate=-1):
        assert self.fig is not None, "Please setup plot before calling SaveToMP4"
        self.LoadFile(input, limit_frames=0)
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
    def UpdateContour(self, n, x=800, y=600):
        # Clear previous contours.
        for c in self.contour.collections:
            c.remove()
        # Reset contour plot.
        self.contour = plt.contour(self.data[n], levels=self.levels, cmap=self.color, linewidths=self.linewidths)
        x_ = np.arange(0, x)
        y_ = np.arange(0, y)
        #self.contour = plt.contourf(x_, y_, self.data[n], levels=self.levels, norm=SymLogNorm(linthresh=1E-2, vmin=0, 
        #                            vmax=np.max(self.data[n])), cmap=self.color, linewidths=self.linewidths)
        # Update plot title.
        plt.title("Frame: " + str(n))
        # Print progress in percentages.
        print(str(int(round((n + 1) / self.frames * 100, 0))) + "% complete...")


animator = ContourAnimator()

animator.SetupPlot(levels=1000, \
                   linewidths=0.5, \
                   color=plt.cm.magma, \
                   x_title="X", \
                   y_title="Y")


# plots data and saves as image
def Graph(x, y, x_name, y_name, title, file_name, fit_dIdV = False):
        fig, ax = plt.subplots(1, 1)
        ax.plot(x, y)
        #fit IV curve
        if fit_dIdV:
            fig_fit, ax_fit = plt.subplots(1,1)
            ax_fit.plot(x,y)
            #fit to only the middle 5% of data points (where dI/dV vs V approximated well by parabola)
            center = int(np.ceil(np.size(x) / 2))
            half_width = int(0.025 * np.size(x))
            x_ = x[center-half_width : center+half_width]
            y_ = y[center-half_width : center+half_width]
            coeffs = np.polyfit(x_, y_, 2)
            a, b, c = coeffs[0], coeffs[1], coeffs[2]
            #plot parabola
            x_approx = np.arange(x[center-(5*half_width)], x[center+(5*half_width)], 0.01)
            y_approx = [(a*(i**2) + b*i + c) for i in x_approx]
            ax_fit.plot(x_approx, y_approx, 'r', linewidth=0.5)
            ax_fit.set_title('Fitted dI/dV vs V curve, zoomed in')
            ax_fit.set_ylabel('dI/dV (arb. unit)')
            ax_fit.set_xlabel('V (arb. unit)')
            #restrict graph axes
            ax_fit.set_xlim(x_approx[0], x_approx[-1])
            ax_fit.set_ylim(np.min(y), np.max(y_approx) / 5)
            fig_fit.savefig(f'{file_name}_fitted.png')
            plt.close(fig_fit)
            
        ax.set_title(title)
        ax.set_xlabel(x_name)
        ax.set_ylabel(y_name)
        fig.savefig(file_name+".png")
        plt.close(fig)
        
#change data file path if needed
main_file_dir = os.getcwd()
data_location = main_file_dir + '\\data_test_chains_10.json'
animator.SaveToMP4(input=data_location, output = "anim_test_chains_10.mp4", fps=1)
#animator.SaveToMP4(input="data2.json", output="demo2.mp4", fps=3)

with open(data_location) as json_file:
    #open file
    j = json.load(json_file)
    measurements = np.asarray(j.get("measurements"))
    n = len(measurements)
    measurements = np.reshape(measurements, (n, 2))
    frame = np.arange(n)
    separation = [m[0] for m in measurements]
    resistance = [m[1] for m in measurements]

static_frames = 5
frame = frame[:-static_frames]
separation = separation[:-static_frames]
resistance = resistance[:-static_frames]
Graph(frame, resistance, 'Frame', 'Resistance (arb. unit)', 'Resistance vs Time', 'temp')

'''
with open(data_location) as json_file:
    #open file
    j = json.load(json_file)
    measurements = np.asarray(j.get("measurements"))
    frames = len(measurements)
    #measurements = np.reshape(measurements, (frames, 4))
    #define integrand for current calculation
    def I(E, V, s, W):
        #E is indepentent variable, and V, s and W are parameters (V is voltage, s is minimum electrode separation, W is work function);
        e_plus = np.exp(E + (V / 2) )
        e_minus = np.exp(E - (V / 2) )
        val = np.exp(-s*np.sqrt(abs(W - E)) ) # * (e_plus - e_minus) / ( (1 + e_plus) * (1 + e_minus) )
        if (V < 0):
            val = - val
        return(val)
    # define variables which may be graphed
    time_frame = np.arange(frames)
    separation = measurements
    #current_sim = [measurements[i,1] for i in range(frames)]
    #voltage = [measurements[i,2] for i in range(frames)]
    #resistance = [measurements[i,3] for i in range(frames)]
    #vary voltage by small increments for higher resolution I vs V and dI/dV vs V curves
    n_points = 1000
    #v_start, v_end = voltage[0], voltage[-1]
    #dV = (v_end - v_start) / n_points
    #voltage_HR = np.arange(v_start, v_end + dV, dV)
    #create separation list of same size
    #separation_HR = np.full(np.size(voltage_HR), separation[-1])
    #integrate current
    # scaling factor is to prevent underflow error
    scale = 0.01
    work_func = 10
    current = np.array([])
    current_HR = np.array([])
    for s, V in zip(separation, voltage):
        if work_func < abs(V) * scale:
            print("Warning! Bounds of integration exceed work function.")
        integral = quad(I, -abs(V) * scale, abs(V) * scale, args=(V * scale, s * scale, work_func))
        current = np.append(current, integral[0])
    #for high resolution current
    for s, V in zip(separation_HR, voltage_HR):
        if work_func < abs(V) * scale:
            print("Warning! Bounds of integration exceed work function.")
        integral = quad(I, -abs(V) * scale, abs(V) * scale, args=(V * scale, s * scale, work_func))
        current_HR = np.append(current_HR, integral[0])
    dI_dV = np.gradient(current, voltage)
    dI_dV_HR = np.gradient(current_HR, voltage_HR)
    # graph x vs y
    '''
'''
    Graph(voltage, current, "Voltage (arb unit)", "Current (arb unit)", f'Current vs Voltage; W={work_func}',
          f'IV_curve_test_long_27_W{work_func}')
    Graph(time_frame[:100], resistance[:100], "Frame #", "Resistance (arb unit)", f'Resistance vs Time; W={work_func}', 
          f'Rt_curve_test_long_27_W{work_func}')      
    Graph(voltage, dI_dV, "Voltage (arb unit)", 'dI/dV (arb unit)', f'dI/dV vs Voltage; W={work_func}',
          f'dIdV_V_curve_test_long_27_W{work_func}')
    
    Graph(voltage_HR, current_HR, "Voltage (arb unit)", "Current (arb unit)", f'Current vs Voltage; W={work_func}',
          f'IV_curve_test_long_27_end_W{work_func}')
    Graph(voltage_HR, dI_dV_HR, "Voltage (arb unit)", 'dI/dV (arb unit)', f'dI/dV vs Voltage; W={work_func}',
          f'dIdV_V_curve_test_long_27_end_W{work_func}', fit_dIdV=True)
    '''