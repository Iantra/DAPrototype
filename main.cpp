/******************************************************************************************
  Date:    12.08.2016
  Author:  Nathan Greco (Nathan.Greco@gmail.com)

  Project:
      DAPrototype: Driver Assist Prototype
	  http://github.com/NateGreco/DAPrototype.git

  Description:
      This project is an attempt to create a standalone, windshield mounted driver assist
      unit with the following functionality:
		  - LDW (Lane Departure Warning)
		  - FCW (Forward Collision Warning)
		  - Tailgate warning
		  - Driver pull-ahead warning
		  - Dashcam functionality (with GPS & timestamp overlay)

  Target Hardware:
      - Raspberry Pi (v3)
	  - RPi (v2.1) 8MP camera
	  - LidarLite (v3) LIDAR rangefinder
      - Adafruit Ultimate GPS
      - HDMI screen (800x480)
	  - Custom built HAT w/ power loss GPIO and capacitors to delay power off

  Target Software platform:
      Debian disto (DietPi) running LDXE, with below libraries installed

  3rd Party Libraries:
      - OpenCV 3.1.0		-> http://www.opencv.org *Compiled with OpenGL support
      - Raspicam 0.1.3		-> http://www.uco.es/investiga/grupos/ava/node/40
	  - WiringPi 2.29		-> http://wiringpi.com/
	  
  License:
	  This software is licensed under GNU GPL v3.0

  History:
      Date         Author      Description
-------------------------------------------------------------------------------------------
      12.08.2016   N. Greco    Initial creation
      07.09.2016   N. Greco    GIT repository created
	  04.10.2016   N. Greco    Compiled and tested on raspberry pi hardware
******************************************************************************************/


//Standard libraries
#include <iostream>
#include <algorithm>
#include <fstream>
#include <string>
#include <thread>
#include <cstdlib>
#include <mutex>
#include <atomic>

//3rd party libraries
#include "opencv2/opencv.hpp"
//#include <libgpsmm.h>

//DAPrototype source files
#include "display_handler.h"
#include "gpio_handler.h"
#include "gps_polling.h"
#include "image_capturer.h"
#include "image_editor.h"
#include "image_processor.h"
#include "lane_detect_processor.h"
#include "lidar_polling.h"
#include "pace_setter_class.h"
#include "process_values_class.h"
#include "video_writer.h"
#include "xml_reader.h"
#include "fcw_tracker_class.h"

void PrintHeader ()
{
	time_t t = time(0);
    struct tm * now = localtime( & t );
    std::cout << '\n';
    std::cout << "Program launched at: ";
    std::cout << (now->tm_year + 1900) << '-' 
			  << (now->tm_mon + 1) << '-'
			  <<  now->tm_mday
			  << '\n' << '\n';
}

int main()
{
	std::cout << "Program launched, starting log file..." << '\n';
	//Quick and dirty log file
	std::ofstream out("/log.txt", std::ios_base::app | std::ios_base::out);
	std::streambuf *coutbuf = std::cout.rdbuf();
	std::cout.rdbuf(out.rdbuf());
	
	//Create log header
	PrintHeader();
	
    //Check XML Properties
	if (settings::kreadsuccess < 0) {
        std::cout << "XML reading failed, using defaults." << '\n';
    } else {
        std::cout << "XML reading successful!" << '\n';
    }

	//Create shared resources
	std::atomic<bool> exitsignal{ false };
    cv::Mat captureimage;
	std::mutex capturemutex;
	cv::Mat displayimage;
	std::mutex displaymutex;
	ProcessValues processvalues;
	
	//Start threads

	//Start image capture thread
    std::thread t_imagecapture( CaptureImageThread,
	                            &captureimage,
								&capturemutex,
								&exitsignal );
	//Start image processor thread
    std::thread t_imageprocessor( ProcessImageThread,
								  &captureimage,
								  &capturemutex,
								  &processvalues,
								  &exitsignal );
	//Start display thread
	std::thread t_displayupdate( DisplayUpdateThread,
								 &displayimage,
								 &displaymutex,
								 &exitsignal );
	//Start image editor thread
    std::thread t_imeageeditor( ImageEditorThread,
								&captureimage,
								&capturemutex,
								&displayimage,
								&displaymutex,
								&processvalues,
								&exitsignal );
	//Start video writer thread
    std::thread t_videowriter( VideoWriterThread,
							   &captureimage,
							   &capturemutex,
							   &displayimage,
							   &displaymutex,
							   &exitsignal );

    //Set pace setter class!
	int pollrate{ std::max(std::max(settings::comm::kpollrategps,
									settings::comm::kpollratelidar),
						   settings::comm::kpollrategpio) };
	int bufferflushrate { pollrate * 10};	//Every 10 seconds
	int gpspollinterval{ pollrate / settings::comm::kpollrategps };
	int gpiopollinterval{ pollrate / settings::comm::kpollrategpio };
	int fcwpollinterval{ pollrate / settings::comm::kpollratelidar };
	PaceSetter mypacesetter( pollrate, "Main" );
	
	//Setup polling

	//GPIO
	bool gpiopoll{ false };
	if ( settings::gpio::kenabled ) {
		try {
			//gpiopoll = GpioHandlerSetup();
		} catch ( const std::exception& ex ) {
			std::cout << "GPIO handler setup threw exception: "<< ex.what() << '\n';
			gpiopoll = false;
		} catch ( const std::string& str ) {
			std::cout << "GPIO handler setup threw exception: "<< str << '\n';
			gpiopoll = false;
		} catch (...) {
			std::cout << "GPIO handler setup threw exception of unknown type!" << '\n';
			gpiopoll = false;
		}
	}
	//GPS
	gpsmm* gpsrecv{ NULL };
	bool gpspoll{ false };
	bool timeset{ false };
	if ( settings::gps::kenabled ) {
		try {
			gpsrecv = new gpsmm("localhost", DEFAULT_GPSD_PORT);
			gpspoll = GpsPollingSetup( gpsrecv );
		} catch ( const std::exception& ex ) {
			std::cout << "GPS polling setup threw exception: "<< ex.what() << '\n';
			gpspoll = false;
		} catch ( const std::string& str ) {
			std::cout << "GPS polling setup threw exception: "<< str << '\n';
			gpspoll = false;
		} catch (...) {
			std::cout << "GPS polling setup threw exception of unknown type!" << '\n';
			gpspoll = false;
		}
	} else {
		gpsrecv = NULL;
	}
	//FCW
	FcwTracker* fcwtracker{ NULL };
	bool fcwpoll{ false };
	int dacmodule{ -1 };
	if ( settings::fcw::kenabled ) {
		try {
			fcwtracker = new FcwTracker( settings::fcw::ksamplestoaverage );
			dacmodule = LidarPollingSetup();
			if ( dacmodule >= 0 ) fcwpoll = true;
		} catch ( const std::exception& ex ) {
			std::cout << "FCW setup threw exception: "<< ex.what() << '\n';
			fcwpoll = false;
		} catch ( const std::string& str ) {
			std::cout << "FCW setup threw exception: "<< str << '\n';
			fcwpoll = false;
		} catch (...) {
			std::cout << "FCW setup threw exception of unknown type!" << '\n';
			fcwpoll = false;
		}
	} else {
		fcwtracker = NULL;
	}
    
	//Loop
	int i{ 0 };
	do {
		//Increment
		i++;
		//Flush cout buffer every second
		if ( i % bufferflushrate == 0 ) std::cout << std::flush;
		//if ( (gpspoll) && (i % gpspollinterval == 0) ) 
		//	GpsPolling( processvalues,  gpsrecv,  timeset );
		//if ( (gpiopoll) &&
		//	 (i % gpiopollinterval == 0) ) GpioHandler( processvalues,	exitsignal );
		//if ( (fcwpoll) && (i % fcwpollinterval == 0) ) 
		//	LidarPolling( processvalues, dacmodule, fcwtracker );

		//Set Pace
		mypacesetter.SetPace();
	} while( !exitsignal );
	
	//Cleanup variables
	//delete gpsrecv;
	//gpsrecv = NULL;
	//delete fcwtracker;
	//fcwtracker = NULL;

    //Handle all the threads
	t_videowriter.join();
	t_imageprocessor.join();
	t_imeageeditor.join();
	t_imagecapture.join();
	t_displayupdate.join();

	//Flush buffer and close file
	std::cout << std::flush;
	std::cout.rdbuf(coutbuf);
	std::cout << "Program exited gracefully!"  << '\n';

    return 0;
}
