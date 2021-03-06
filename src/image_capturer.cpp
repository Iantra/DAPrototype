/******************************************************************************************
  Date:    12.08.2016
  Author:  Nathan Greco (Nathan.Greco@gmail.com)

  Project:
      DAPrototype: Driver Assist Prototype
	  http://github.com/NateGreco/DAPrototype.git

  License:
	  This software is licensed under GNU GPL v3.0
	  
******************************************************************************************/

//Standard libraries
#include <iostream>
#include <algorithm>
#include <mutex>
#include <atomic>
#include <exception>
#include <string>

//3rd party libraries
#include "opencv2/opencv.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "raspicam/raspicam_cv.h"

//Project libraries
#include "pace_setter_class.h"
#include "xml_reader.h"

/*****************************************************************************************/
void CaptureImageThread( cv::Mat *capture,
                         std::mutex *capturemutex,
                         std::atomic<bool> *exitsignal )
{
	std::cout << "Image capturer thread starting!" << '\n';

    	//Create camera
	raspicam::RaspiCam_Cv Camera;
	
	//Create reference
	const char* src = "../data/HighwayDashcam.avi";
	cv::VideoCapture cap(src);
	
	//Set properties
	Camera.set( cv::CAP_PROP_FRAME_WIDTH, settings::cam::kpixwidth );
	Camera.set( cv::CAP_PROP_FRAME_HEIGHT, settings::cam::kpixheight );
	Camera.set( cv::CAP_PROP_FORMAT, CV_8UC3 );
	int frameNum = -1;          // Frame counter

    	//Open
	if ( !Camera.open() ) {
		std::cerr << "Error opening the camera" << '\n';
		exit(-1);
	}
	std::cout << "Camera opened succesfully!" << '\n';
	
	if ( !cap.isOpened())
 	 {
  		std::cout  << "Error opening the reference" << src << '\n';
 		 exit(-1);
  	}
	std::cout << "Reference opened succesfully!" << '\n';
	//create pace setter
	PaceSetter camerapacer( std::max(std::max(settings::disp::kupdatefps,
											  settings::cam::krecfps),
									 settings::ldw::kupdatefps),
							"Image capturer");

	//Loop indefinitely
	while( !(*exitsignal) ) {
		try {
			cv::Mat newimage;	//Frame data storage
			
			if(settings::gen::kusecam){	//Using camera
				Camera.grab();
				Camera.retrieve( newimage );
				cv::flip( newimage, newimage, -1 );
				//resize image
				if ( newimage.rows != settings::cam::kpixheight ) {
					cv::resize( 	newimage,
							newimage,
							cv::Size(settings::cam::kpixwidth,
							settings::cam::kpixheight) );
				}
			}else{	//Using reference
				if(frameNum < 1000){
					++frameNum;
				}else{
					frameNum = 0;
					cap.set(1, 0 );
				}


				cap >> newimage;
			}
			
			//Send back frame data
			capturemutex->lock();
			*capture = newimage;
			capturemutex->unlock(); 

			//Set pace
			camerapacer.SetPace();
		} catch ( const std::exception& ex ) {
			std::cout << "Image Capturer thread threw exception: "<< ex.what() << '\n';
		} catch ( const std::string& str ) {
			std::cout << "Image Capturer thread threw exception: "<< str << '\n';
		} catch (...) {
			std::cout << "Image Capturer thread threw exception of unknown type!" << '\n';
		}
	}

	std::cout << "Exiting image capturer thread!" << '\n';
	return;
}
