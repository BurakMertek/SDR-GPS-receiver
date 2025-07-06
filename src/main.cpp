#include <iostream>
#include <thread>
#include <chrono>
#include <signal.h>
#include <atomic>
#include <iomanip>
#include "acquisition/sdr_receiver.h"
#include "acquisition/signal_acquisition.h"
#include "tracking/gps_tracker.h"
#include "decoding/nav_decoder.h"

std::atomic<bool> g_running(true);

void signalHandler(int signum) {
    std::cout << "\nInterrupt signal (" << signum << ") received.\n";
    g_running = false;
}

void printHeader() {
    std::cout << "\n";
    std::cout << "╔══════════════════════════════════════════════════╗\n";
    std::cout << "║      SDR Real-Time GPS Receiver                  ║\n";
    std::cout << "║      Version 1.0.0                               ║\n";
    std::cout << "║      Built with Modern C++17                     ║\n";
    std::cout << "╚══════════════════════════════════════════════════╝\n";
    std::cout << "\n";
}

void printStatus(const std::vector<gps::SatelliteInfo>& satellites) {
    // Clear screen (works on Unix-like systems)
    std::cout << "\033[2J\033[1;1H";
    
    std::cout << "GPS Receiver Status - " 
              << std::chrono::system_clock::now().time_since_epoch().count() 
              << "\n\n";
    
    std::cout << std::setw(5) << "PRN" 
              << std::setw(15) << "Status"
              << std::setw(15) << "Doppler (Hz)"
              << std::setw(15) << "C/N0 (dB-Hz)"
              << std::setw(15) << "Ephemeris"
              << "\n";
    std::cout << std::string(65, '-') << "\n";
    
    for (const auto& sat : satellites) {
        std::cout << std::setw(5) << sat.prn
                  << std::setw(15) << (sat.is_tracked ? "TRACKING" : "SEARCHING")
                  << std::setw(15) << std::fixed << std::setprecision(1) << sat.doppler_shift
                  << std::setw(15) << std::fixed << std::setprecision(1) << sat.cn0
                  << std::setw(15) << (sat.has_ephemeris ? "YES" : "NO")
                  << "\n";
    }
    std::cout << "\nPress Ctrl+C to exit...\n";
}

int main(int argc, char* argv[]) {
    
    signal(SIGINT, signalHandler);
    
    printHeader();
    
    
    const double sample_rate = 2.048e6;  
    const double center_freq = 1575.42e6;  
    const int device_index = 0;
    
    
    std::vector<int> prn_list;
    for (int i = 1; i <= 32; ++i) {
        prn_list.push_back(i);
    }
    
    try {
        
        std::cout << "Initializing SDR receiver...\n";
        gps::SDRReceiver receiver;
        
        if (!receiver.initializeDevice(device_index, sample_rate, center_freq)) {
            std::cerr << "Failed to initialize SDR device!\n";
            return 1;
        }
        
        
        std::cout << "Initializing GPS tracker...\n";
        gps::GPSTracker tracker(sample_rate);
        tracker.initialize(prn_list);
        
        
        std::cout << "Initializing navigation decoder...\n";
        gps::NavigationDecoder decoder;
        
       
        std::cout << "Starting data capture...\n";
        if (!receiver.startCapture()) {
            std::cerr << "Failed to start data capture!\n";
            return 1;
        }
        
        
        tracker.startTracking();
        
        
        std::cout << "GPS receiver is running...\n\n";
        
        const size_t buffer_size = static_cast<size_t>(sample_rate * 0.001);  
        gps::IQBuffer sample_buffer;
        
        auto last_status_time = std::chrono::steady_clock::now();
        const auto status_interval = std::chrono::seconds(1);
        
        while (g_running) {
            
            if (receiver.getSamples(sample_buffer, buffer_size)) {
                
                tracker.processSamples(sample_buffer);
                
                
                auto satellites = tracker.getTrackedSatellites();
                for (const auto& sat : satellites) {
                    if (sat.is_tracked) {
                        auto nav_data = tracker.getNavigationData(sat.prn);
                        decoder.processNavigationData(sat.prn, nav_data);
                    }
                }
                
                
                auto now = std::chrono::steady_clock::now();
                if (now - last_status_time >= status_interval) {
                    printStatus(satellites);
                    last_status_time = now;
                }
            }
            
            
            std::this_thread::sleep_for(std::chrono::microseconds(100));
        }
        
        
        std::cout << "\nShutting down...\n";
        tracker.stopTracking();
        receiver.stopCapture();
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    std::cout << "GPS receiver stopped.\n";
    return 0;
}