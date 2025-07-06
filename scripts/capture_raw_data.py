#!/usr/bin/env python3
"""
RTL-SDR Raw Data Capture Tool
Captures IQ samples from RTL-SDR for GPS processing
"""

import numpy as np
import argparse
import subprocess
import struct
import time
from datetime import datetime

class RTLSDRCapture:
    def __init__(self, device_index=0):
        self.device_index = device_index
        self.sample_rate = 2048000  # 2.048 MHz
        self.center_freq = 1575420000  # GPS L1 frequency
        self.gain = 40  # dB
        
    def capture_samples(self, duration_seconds, output_file):
        num_samples = int(self.sample_rate * duration_seconds)
        
        cmd = [
            'rtl_sdr',
            '-d', str(self.device_index),
            '-f', str(self.center_freq),
            '-s', str(self.sample_rate),
            '-g', str(self.gain),
            '-n', str(num_samples * 2),  # 2 bytes per IQ sample
            output_file
        ]
        
        print(f"Capturing {duration_seconds} seconds of GPS data...")
        print(f"Sample rate: {self.sample_rate/1e6} MHz")
        print(f"Center frequency: {self.center_freq/1e6} MHz")
        print(f"Gain: {self.gain} dB")
        
        try:
            subprocess.run(cmd, check=True)
            print(f"Data saved to {output_file}")
            return True
        except subprocess.CalledProcessError as e:
            print(f"Error capturing data: {e}")
            return False
    
    def convert_to_complex(self, input_file, output_file):
        with open(input_file, 'rb') as f_in:
            raw_data = f_in.read()
        
        num_samples = len(raw_data) // 2
        iq_samples = np.zeros(num_samples, dtype=np.complex64)
        
        for i in range(num_samples):
            i_val = (raw_data[2*i] - 127.5) / 127.5
            q_val = (raw_data[2*i + 1] - 127.5) / 127.5
            iq_samples[i] = i_val + 1j * q_val
        
        with open(output_file, 'wb') as f_out:
            f_out.write(struct.pack('I', num_samples))
            f_out.write(struct.pack('d', self.sample_rate))
            f_out.write(struct.pack('d', self.center_freq))
            iq_samples.tofile(f_out)
        
        print(f"Converted {num_samples} samples to {output_file}")
        return iq_samples
    
    def analyze_signal(self, iq_samples):
        power = np.abs(iq_samples) ** 2
        avg_power = np.mean(power)
        peak_power = np.max(power)
        
        print("\nSignal Analysis:")
        print(f"Average power: {10 * np.log10(avg_power):.1f} dB")
        print(f"Peak power: {10 * np.log10(peak_power):.1f} dB")
        print(f"Dynamic range: {10 * np.log10(peak_power/avg_power):.1f} dB")
        
        spectrum = np.abs(np.fft.fftshift(np.fft.fft(iq_samples[:8192])))
        spectrum_db = 20 * np.log10(spectrum + 1e-10)
        
        return spectrum_db

def main():
    parser = argparse.ArgumentParser(description='RTL-SDR GPS Data Capture')
    parser.add_argument('--duration', type=float, default=1.0,
                       help='Capture duration in seconds')
    parser.add_argument('--output', type=str, default=None,
                       help='Output filename')
    parser.add_argument('--device', type=int, default=0,
                       help='RTL-SDR device index')
    parser.add_argument('--gain', type=int, default=40,
                       help='Gain in dB (0 for auto)')
    parser.add_argument('--analyze', action='store_true',
                       help='Analyze captured signal')
    
    args = parser.parse_args()
    
    if args.output is None:
        timestamp = datetime.now().strftime('%Y%m%d_%H%M%S')
        args.output = f'gps_capture_{timestamp}.bin'
    
    capture = RTLSDRCapture(args.device)
    capture.gain = args.gain
    
    raw_file = args.output + '.raw'
    if capture.capture_samples(args.duration, raw_file):
        iq_samples = capture.convert_to_complex(raw_file, args.output)
        
        if args.analyze:
            spectrum = capture.analyze_signal(iq_samples)
            
            try:
                import matplotlib.pyplot as plt
                
                fig, (ax1, ax2) = plt.subplots(2, 1, figsize=(10, 8))
                
                ax1.plot(np.real(iq_samples[:1000]), 'b-', label='I', alpha=0.7)
                ax1.plot(np.imag(iq_samples[:1000]), 'r-', label='Q', alpha=0.7)
                ax1.set_xlabel('Sample')
                ax1.set_ylabel('Amplitude')
                ax1.set_title('IQ Time Domain (first 1000 samples)')
                ax1.legend()
                ax1.grid(True)
                
                freqs = np.linspace(-capture.sample_rate/2, capture.sample_rate/2, len(spectrum))
                ax2.plot(freqs/1e6, spectrum)
                ax2.set_xlabel('Frequency (MHz)')
                ax2.set_ylabel('Power (dB)')
                ax2.set_title('Frequency Spectrum')
                ax2.grid(True)
                
                plt.tight_layout()
                plt.savefig(args.output + '_analysis.png')
                print(f"Analysis plot saved to {args.output}_analysis.png")
                plt.show()
                
            except ImportError:
                print("Matplotlib not available for plotting")

if __name__ == '__main__':
    main()