#!/usr/bin/env python3
"""
GPS Signal Visualization Tool
Plots acquisition results, tracking loops, and navigation data
"""

import numpy as np
import matplotlib.pyplot as plt
from matplotlib.animation import FuncAnimation
import struct
import argparse
from datetime import datetime

class GPSPlotter:
    def __init__(self):
        self.fig = None
        self.axes = None
        
    def plot_acquisition_results(self, filename):
        """Plot 2D acquisition search space (code phase vs frequency)"""
        # Read binary data
        with open(filename, 'rb') as f:
            # Read header
            num_freqs = struct.unpack('I', f.read(4))[0]
            num_codes = struct.unpack('I', f.read(4))[0]
            
            # Read frequency bins
            freqs = np.array(struct.unpack(f'{num_freqs}f', f.read(4 * num_freqs)))
            
            # Read correlation matrix
            corr_matrix = np.zeros((num_freqs, num_codes))
            for i in range(num_freqs):
                row = struct.unpack(f'{num_codes}f', f.read(4 * num_codes))
                corr_matrix[i, :] = row
        
        # Create plot
        plt.figure(figsize=(12, 8))
        
        # 2D correlation plot
        plt.subplot(2, 2, 1)
        plt.imshow(corr_matrix, aspect='auto', origin='lower', 
                   extent=[0, num_codes, freqs[0], freqs[-1]])
        plt.colorbar(label='Correlation Power')
        plt.xlabel('Code Phase (chips)')
        plt.ylabel('Frequency (Hz)')
        plt.title('Acquisition Search Space')
        
        # Find peak
        peak_idx = np.unravel_index(np.argmax(corr_matrix), corr_matrix.shape)
        peak_freq = freqs[peak_idx[0]]
        peak_code = peak_idx[1]
        
        # Mark peak
        plt.plot(peak_code, peak_freq, 'r*', markersize=15)
        
        # Frequency slice
        plt.subplot(2, 2, 2)
        plt.plot(freqs, corr_matrix[:, peak_code])
        plt.axvline(peak_freq, color='r', linestyle='--', label=f'Peak: {peak_freq:.1f} Hz')
        plt.xlabel('Frequency (Hz)')
        plt.ylabel('Correlation Power')
        plt.title('Frequency Slice at Peak Code Phase')
        plt.legend()
        plt.grid(True)
        
        # Code phase slice
        plt.subplot(2, 2, 3)
        plt.plot(corr_matrix[peak_idx[0], :])
        plt.axvline(peak_code, color='r', linestyle='--', label=f'Peak: {peak_code} chips')
        plt.xlabel('Code Phase (chips)')
        plt.ylabel('Correlation Power')
        plt.title('Code Phase Slice at Peak Frequency')
        plt.legend()
        plt.grid(True)
        
        # 3D surface plot
        ax = plt.subplot(2, 2, 4, projection='3d')
        X, Y = np.meshgrid(range(num_codes), freqs)
        ax.plot_surface(X, Y, corr_matrix, cmap='viridis', alpha=0.8)
        ax.set_xlabel('Code Phase (chips)')
        ax.set_ylabel('Frequency (Hz)')
        ax.set_zlabel('Correlation Power')
        ax.set_title('3D Acquisition Surface')
        
        plt.tight_layout()
        plt.show()
    
    def plot_tracking_results(self, filename):
        """Plot tracking loop results over time"""
        # Read tracking data
        data = np.loadtxt(filename, delimiter=',', skiprows=1)
        time = data[:, 0]
        code_phase = data[:, 1]
        carrier_phase = data[:, 2]
        doppler = data[:, 3]
        cn0 = data[:, 4]
        i_prompt = data[:, 5]
        q_prompt = data[:, 6]
        
        fig, axes = plt.subplots(3, 2, figsize=(14, 10))
        
        # Code phase
        axes[0, 0].plot(time, code_phase)
        axes[0, 0].set_xlabel('Time (s)')
        axes[0, 0].set_ylabel('Code Phase (chips)')
        axes[0, 0].set_title('Code Phase Tracking')
        axes[0, 0].grid(True)
        
        # Carrier phase
        axes[0, 1].plot(time, carrier_phase)
        axes[0, 1].set_xlabel('Time (s)')
        axes[0, 1].set_ylabel('Carrier Phase (rad)')
        axes[0, 1].set_title('Carrier Phase Tracking')
        axes[0, 1].grid(True)
        
        # Doppler frequency
        axes[1, 0].plot(time, doppler)
        axes[1, 0].set_xlabel('Time (s)')
        axes[1, 0].set_ylabel('Doppler (Hz)')
        axes[1, 0].set_title('Doppler Shift')
        axes[1, 0].grid(True)
        
        # C/N0
        axes[1, 1].plot(time, cn0)
        axes[1, 1].set_xlabel('Time (s)')
        axes[1, 1].set_ylabel('C/N0 (dB-Hz)')
        axes[1, 1].set_title('Carrier-to-Noise Ratio')
        axes[1, 1].grid(True)
        
        # I-Q constellation
        axes[2, 0].scatter(i_prompt[::10], q_prompt[::10], alpha=0.5, s=1)
        axes[2, 0].set_xlabel('I')
        axes[2, 0].set_ylabel('Q')
        axes[2, 0].set_title('I-Q Constellation')
        axes[2, 0].grid(True)
        axes[2, 0].axis('equal')
        
        # Navigation bits
        axes[2, 1].plot(time, np.sign(i_prompt), linewidth=0.5)
        axes[2, 1].set_xlabel('Time (s)')
        axes[2, 1].set_ylabel('Navigation Bit')
        axes[2, 1].set_title('Demodulated Navigation Data')
        axes[2, 1].set_ylim([-1.5, 1.5])
        axes[2, 1].grid(True)
        
        plt.tight_layout()
        plt.show()
    
    def plot_skyplot(self, filename):
        """Plot satellite positions in sky view"""
        # Read satellite data
        data = np.loadtxt(filename, delimiter=',', skiprows=1)
        prn = data[:, 0].astype(int)
        azimuth = data[:, 1]  # degrees
        elevation = data[:, 2]  # degrees
        cn0 = data[:, 3]  # dB-Hz
        
        # Convert to radians
        azimuth_rad = np.radians(azimuth)
        
        # Create polar plot
        fig = plt.figure(figsize=(10, 10))
        ax = fig.add_subplot(111, projection='polar')
        
        # Plot satellites
        scatter = ax.scatter(azimuth_rad, 90 - elevation, 
                           c=cn0, s=200, cmap='viridis', 
                           vmin=20, vmax=50, edgecolors='black')
        
        # Add PRN labels
        for i in range(len(prn)):
            ax.annotate(str(prn[i]), 
                       (azimuth_rad[i], 90 - elevation[i]),
                       ha='center', va='center', fontsize=10)
        
        # Configure plot
        ax.set_theta_zero_location('N')
        ax.set_theta_direction(-1)  # Clockwise
        ax.set_ylim(0, 90)
        ax.set_yticks([0, 30, 60, 90])
        ax.set_yticklabels(['90°', '60°', '30°', '0°'])
        ax.grid(True)
        
        # Add colorbar
        cbar = plt.colorbar(scatter, ax=ax, pad=0.1)
        cbar.set_label('C/N0 (dB-Hz)', rotation=270, labelpad=20)
        
        plt.title('GPS Satellite Sky Plot', y=1.08)
        plt.show()
    
    def animate_tracking(self, filename, interval=100):
        """Animate real-time tracking results"""
        # Setup figure
        self.fig, self.axes = plt.subplots(2, 2, figsize=(12, 8))
        
        # Initialize data buffers
        self.time_buffer = []
        self.doppler_buffer = []
        self.cn0_buffer = []
        self.iq_buffer = []
        
        # Setup plots
        self.line_doppler, = self.axes[0, 0].plot([], [], 'b-')
        self.axes[0, 0].set_xlabel('Time (s)')
        self.axes[0, 0].set_ylabel('Doppler (Hz)')
        self.axes[0, 0].set_title('Doppler Shift')
        self.axes[0, 0].grid(True)
        
        self.line_cn0, = self.axes[0, 1].plot([], [], 'r-')
        self.axes[0, 1].set_xlabel('Time (s)')
        self.axes[0, 1].set_ylabel('C/N0 (dB-Hz)')
        self.axes[0, 1].set_title('Signal Strength')
        self.axes[0, 1].grid(True)
        
        self.scatter_iq = self.axes[1, 0].scatter([], [], s=1)
        self.axes[1, 0].set_xlabel('I')
        self.axes[1, 0].set_ylabel('Q')
        self.axes[1, 0].set_title('I-Q Constellation')
        self.axes[1, 0].grid(True)
        self.axes[1, 0].set_xlim(-1, 1)
        self.axes[1, 0].set_ylim(-1, 1)
        
        # Navigation data text
        self.nav_text = self.axes[1, 1].text(0.05, 0.95, '', 
                                             transform=self.axes[1, 1].transAxes,
                                             verticalalignment='top',
                                             fontfamily='monospace')
        self.axes[1, 1].set_title('Navigation Data')
        self.axes[1, 1].axis('off')
        
        # Animation
        self.data_file = open(filename, 'r')
        self.data_file.readline()  
        
        ani = FuncAnimation(self.fig, self.update_animation, 
                          interval=interval, blit=True)
        
        plt.tight_layout()
        plt.show()
        
        self.data_file.close()
    
    def update_animation(self, frame):
        """Update animation frame"""
        # Read new data
        line = self.data_file.readline()
        if not line:
            return self.line_doppler, self.line_cn0, self.scatter_iq, self.nav_text
        
        data = line.strip().split(',')
        time = float(data[0])
        doppler = float(data[3])
        cn0 = float(data[4])
        i_val = float(data[5])
        q_val = float(data[6])
        
        # Update buffers (keep last 1000 points)
        self.time_buffer.append(time)
        self.doppler_buffer.append(doppler)
        self.cn0_buffer.append(cn0)
        self.iq_buffer.append((i_val, q_val))
        
        if len(self.time_buffer) > 1000:
            self.time_buffer.pop(0)
            self.doppler_buffer.pop(0)
            self.cn0_buffer.pop(0)
            self.iq_buffer.pop(0)
        
        # Update plots
        self.line_doppler.set_data(self.time_buffer, self.doppler_buffer)
        self.axes[0, 0].relim()
        self.axes[0, 0].autoscale_view()
        
        self.line_cn0.set_data(self.time_buffer, self.cn0_buffer)
        self.axes[0, 1].relim()
        self.axes[0, 1].autoscale_view()
        
        if self.iq_buffer:
            iq_array = np.array(self.iq_buffer[-200:]) 
            self.scatter_iq.set_offsets(iq_array)
        
        # Update navigation data display
        nav_bits = ''.join(['1' if i > 0 else '0' for i, _ in self.iq_buffer[-50:]])
        self.nav_text.set_text(f'Navigation Bits (last 50):\n{nav_bits}')
        
        return self.line_doppler, self.line_cn0, self.scatter_iq, self.nav_text

def main():
    parser = argparse.ArgumentParser(description='GPS Signal Visualization Tool')
    parser.add_argument('command', choices=['acquisition', 'tracking', 'skyplot', 'animate'],
                       help='Visualization type')
    parser.add_argument('filename', help='Input data file')
    parser.add_argument('--interval', type=int, default=100,
                       help='Animation interval in ms (for animate command)')
    
    args = parser.parse_args()
    
    plotter = GPSPlotter()
    
    if args.command == 'acquisition':
        plotter.plot_acquisition_results(args.filename)
    elif args.command == 'tracking':
        plotter.plot_tracking_results(args.filename)
    elif args.command == 'skyplot':
        plotter.plot_skyplot(args.filename)
    elif args.command == 'animate':
        plotter.animate_tracking(args.filename, args.interval)

if __name__ == '__main__':
    main()