/******************************************************************************************
  Date:    12.08.2016
  Author:  Nathan Greco (Nathan.Greco@gmail.com)

  Project:
      DAPrototype: Driver Assist Prototype
	  http://github.com/NateGreco/DAPrototype.git

  License:
	  This software is licensed under GNU GPL v3.0
	  
******************************************************************************************/

//Header guard
#ifndef LANEDETECTCONSTANTS_H
#define LANEDETECTCONSTANTS_H

//Project libraries
#include "lane_detect_processor.h"

/*****************************************************************************************/
namespace lanedetectconstants {
	//Default polygon
	const Polygon defaultpolygon{ cv::Point(0,0),
							cv::Point(0,0),
							cv::Point(0,0),
							cv::Point(0,0) };
							
	//ROI											//Relative to image size, must change
	const std::vector<std::vector<cv::Point>> k_roipoints{{	cv::Point(0,380),
							       	cv::Point(0,350),
								cv::Point(200,180),
								cv::Point(320,180),
								cv::Point(600,380) }};
													 
	//Image evaluation
	const cv::Scalar k_lowerwhitethreshold{ 0, 40, 80 };
	const cv::Scalar k_upperwhitethreshold{ 180, 255, 255 };
	const cv::Scalar k_loweryellowthreshold{ 0, 0, 80 };
	const cv::Scalar k_upperyellowthreshold{ 90, 255, 255 };
	const float k_contrastscalefactor{ 0.75f };
	
	//Line filtering
	const float k_maxvanishingpointangle{ 18.0f };
	const uint16_t k_vanishingpointx{ 266 };				//Relative to image size, must change
	const uint16_t k_vanishingpointy{ 200 };				//Relative to image size, must change
	const uint16_t k_verticallimit{ 280 };				//Relative to image size, must change
	const uint16_t k_rho{ 1 };
	const float k_theta{ CV_PI/22.5 };//0.13962634015f };				//Pi / 22.5
	const uint16_t k_minimumsize{ 35 };//25 };					//Relative to image size, must change
	const uint16_t k_maxlinegap{ 200 };//5 };						//Relative to image size, must change
	const uint16_t k_threshold{ 20 };//30 };						//Relative to image size, must change

	//Polygon filtering
	const uint16_t k_maxoffsetfromcenter{ 400 };			//Relative to image size, must change
    	const uint16_t k_minroadwidth{ 540 };					//Relative to image size, must change
    	const uint16_t k_maxroadwidth{ 800 };					//Relative to image size, must change
	
	//Scoring
	const float k_lowestscorelimit{ -400.0f };			//Relative to image size, must change
	const float k_weightedheightwidth{ 100.0f };			//Relative to image size, must change
	const float k_weightedangleoffset{ -5.0f };
	const float k_weightedcenteroffset{ -1.0f };			//Relative to image size, must change

}

#endif // LANEDETECTCONSTANTS_H
