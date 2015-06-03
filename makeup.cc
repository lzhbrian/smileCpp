#include <string>
#include <fstream>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <iostream>
#include "./ThinPlateSpline/CThinPlateSpline.cpp"
#include "./layers.cc"

using namespace std;
using namespace cv;

vector<Point> GetPoints(string filename);
Mat TransferMakeup(String input_image, String input_landmarks, 
                   String example_image, String example_landmarks);
void Process();

Mat TransferMakeup(String input_image, String input_landmarks, String example_image, String example_landmarks){

  // reads the landmarks from the file
  vector<Point> landmarks = GetPoints(example_landmarks);
  vector<Point> destinations = GetPoints(input_landmarks);

  CThinPlateSpline tps(landmarks, destinations);

  Mat exampleRaw = imread(example_image);
  Mat input = imread(input_image);
  Mat example;

  // warps the image
  tps.warpImage(exampleRaw, example, INTER_CUBIC, BACK_WARP);
  Rect crop(0,0, min(example.cols, input.cols), min(example.rows, input.rows));

  // crops the images so they are the same size
  example = example(crop);
  input = input(crop);
  
  // converts the example to lab colorspace
  cv::cvtColor(example, example, cv::COLOR_BGR2Lab);
  
  // separates the lab color channels of the example image
  Mat *labChannels = new Mat[example.channels()];
  cv::split(example, labChannels);

  Mat inputLab;
  cv::cvtColor(input, inputLab, cv::COLOR_BGR2Lab);
  
  Mat *InputLabChannels = new Mat[inputLab.channels()];
  
  cv::split(inputLab, InputLabChannels);
  layers lay = layers(example, destinations);

  Mat anose = lay.getShapes();
  Mat bnose = anose*(-1) + 255;

  Mat finalL = InputLabChannels[0].clone();
  Mat finalA;
  cv::addWeighted(InputLabChannels[1].mul(bnose)/255, 1, labChannels[1].mul(anose)/255, 1, 0, finalA);
  Mat finalB;
  cv::addWeighted(InputLabChannels[2].mul(bnose)/255, 1, labChannels[2].mul(anose)/255, 1, 0, finalB);

  Mat finalimage[] = {finalL.clone(), finalA.clone(), finalB.clone()};

  Mat finalMat;

  cv::merge(finalimage, 3, finalMat);
  cvtColor(finalMat, finalMat, cv::COLOR_Lab2BGR);

  return finalMat;
}

void Process() {
  string str;
  
  // reads the landmarks from the file
  vector<Point> landmarks = GetPoints("example.txt");
  vector<Point> destinations = GetPoints("obama.txt");

  CThinPlateSpline tps(landmarks, destinations);

  Mat exampleRaw = imread("./images/example.jpg");
  Mat input = imread("./images/obama.jpg");
  Mat example;

  // warps the image
  tps.warpImage(exampleRaw, example, INTER_CUBIC, BACK_WARP);
  Rect crop(0,0, min(example.cols, input.cols), min(example.rows, input.rows));

  example = example(crop);
  input = input(crop);

  imshow("example", example);
  imshow("INPUT", input);
  
  Mat lab;
  cv::cvtColor(example, lab, cv::COLOR_BGR2Lab);
  
  Mat *labChannels = new Mat[lab.channels()];
  cv::split(lab, labChannels);

  imshow("greyscale", labChannels[0]);

  Mat a(lab.rows, lab.cols, CV_8U);
  
  Mat empty = Mat::zeros(lab.rows, lab.cols, CV_8U);
  Mat ab[] = {empty, a, a};
  int from_to[] = {1,1,2,2};
  cv::mixChannels(labChannels, 3, ab, 3, from_to, 2);

  Mat abimage;
  cv::merge(ab, 3, abimage);

  cvtColor(abimage, abimage, cv::COLOR_Lab2BGR);

  imshow("ab layer", abimage);


  Mat inputLab;
  cv::cvtColor(input, inputLab, cv::COLOR_BGR2Lab);
  
  Mat *InputLabChannels = new Mat[inputLab.channels()];
  
  cv::split(inputLab, InputLabChannels);
  layers lay = layers(abimage, destinations);

  Mat anose = lay.getShapes();
  Mat bnose = anose*(-1) + 255;

  imshow("anose",anose);
  imshow("bnose", bnose);

  Mat finalL = InputLabChannels[0].clone();
  Mat finalA;
  cv::addWeighted(InputLabChannels[1].mul(bnose)/255, 1, labChannels[1].mul(anose)/255, 1, 0, finalA);
  Mat finalB;
  cv::addWeighted(InputLabChannels[2].mul(bnose)/255, 1, labChannels[2].mul(anose)/255, 1, 0, finalB);

  Mat finalimage[] = {finalL.clone(), finalA.clone(), finalB.clone()};

  Mat finalMat;
  cv::merge(finalimage, 3, finalMat);

  cvtColor(finalMat, finalMat, cv::COLOR_Lab2BGR);

  imshow("pre-result", finalMat);

  waitKey(0); 
}

/**
 * function that reads the landmark points from a file generated for the purpose
 *
 * filename - the filename of the landmarks file, with extension (.txt in the examples)
 */
vector<Point> GetPoints(string filename) {
  vector<vector<double> > pts(2, vector<double>(83));
  string str;
  string path = "images/" + filename;
  ifstream ifs(path.c_str());
  if (ifs.is_open()) {
    for (int i=0; i<83; i++){
      getline(ifs, str);
      pts[0][i] = atof(str.c_str());
    }
    for (int i=0; i<83; i++) {
      getline(ifs, str);
      pts[1][i] = atof(str.c_str());
    }
  }
  ifs.close();
  vector<Point> points(83);
  for (int i=0; i<83; i++) {
    points[i] = Point(pts[0][i], pts[1][i]);
  }
  return points;
}
