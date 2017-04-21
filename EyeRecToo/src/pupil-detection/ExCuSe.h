#ifndef EXCUSE_H
#define EXCUSE_H

/*
  Version 1.0, 08.06.2015, Copyright University of Tübingen.

  The Code is created based on the method from the paper:
  "ExCuSe: Robust Pupil Detection in Real-World Scenarios", W. Fuhl, T. C. Kübler, K. Sippel, W. Rosenstiel, E. Kasneci
  CAIP 2015 : Computer Analysis of Images and Patterns

  The code and the algorithm are for non-comercial use only.
*/

#include <opencv2/imgproc/imgproc.hpp>

#include "PupilDetectionMethod.h"

class ExCuSe : public PupilDetectionMethod
{
public:
    ExCuSe() { mDesc = desc;}
    cv::RotatedRect run(const cv::Mat &frame);
    bool hasPupilOutline() { return true; }
    static std::string desc;
};

#endif // EXCUSE_H
