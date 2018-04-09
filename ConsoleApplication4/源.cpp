#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <iostream>
#include <string>
#include <thread>
#include <vector>
#include "SerialPort.h"

using namespace cv;
using namespace std;

struct Connected_Component {
	int area = 0;
	int index = 0;
};

struct MyPoint {
	double x = 0.0;
	double y = 0.0;
	bool isApple = true;
};

bool CompColumn(MyPoint &a, MyPoint&b) {
	return a.x < b.x;
}
bool CompRow(MyPoint &a, MyPoint&b)
{
	return a.y < b.y;
}

bool showImg(Mat &img)
{
	imshow("Display window", img);                // Show our image inside it.
	if (waitKey(50) >= 0)// Wait for a keystroke in the window
		return false;
	return true;
}


int main(int argc, char** argv)
{
	VideoCapture Cemara(0);
	CSerialPort mySerialPort;
	if (!mySerialPort.InitPort(7))
	{
		std::cout << "initPort fail !" << std::endl;
	}
	else
	{
		std::cout << "initPort success !" << std::endl;
	}
	int lastOrder = 0;
	while (true)
	{
		Mat image;
		Cemara >> image;
		if (!showImg(image))
			break;
	}
	while (true)
	{
		//String imageName("C:/source/God123.jpg"); // by default
		//if (argc > 1)
		//{
		//	imageName = argv[1];
		//}
		//Mat image;
		//image = imread(imageName, IMREAD_COLOR); // Read the file
		Mat src, image;
		Cemara.read(src);
		if (src.empty())                      // Check for invalid input
		{
			cout << "Could not open or find the image" << std::endl;
			return -1;
		}
		//////////////////////////////////////////////////////////////////////////////////////////////////////

		cvtColor(src, image, COLOR_BGR2HSV);			//Transfer the Color Space

		Mat bin(image.size(), CV_8UC1);							//a new picture bin to store the binary version of image.
		auto outit = bin.begin<unsigned char>();

		for (auto it = image.begin<Vec3b>(); it != image.end<Vec3b>(); it++, outit++)
		{
			if ((*it)[2] < 20 || (*it)[1] < 20)				//The white and black area in the image -> set black
			{
				*outit = 0;
				continue;
			}
			if ((*it)[0] < 10 || (*it)[0]>170)				//The red area ->set white
			{
				*outit = 255;
				continue;
			}
			*outit = 0;										//others ->set black
		}

		Mat label, stat, centroids;
		int count = connectedComponentsWithStats(bin, label, stat, centroids);
		if (count < 2)
		{
			cout << "The Number of red area less than 1 " << endl;
			showImg(src);
			continue;
		}


		int maxIndex = 0, maxSize = 0;
		for (int i = 1; i < count; i++)							//Find out the maxsize area and its index
		{
			int size = stat.at<int>(i, CC_STAT_AREA);
			if (size > maxSize)
			{
				maxSize = size;
				maxIndex = i;
			}
		}

		//Mat bin2(image.size(), CV_8UC1);
		//auto out2it = bin2.begin<unsigned char>();

		//for (auto it = label.begin<int>(); it != label.end<int>(); it++, out2it++)			//*it means the label_number of  connected_Component in bin.
		//{
		//	*out2it = *it == maxIndex ? 255 : 0;
		//}

		Point p(centroids.at<double>(maxIndex, 0), centroids.at<double>(maxIndex, 1));
		drawMarker(src, p, Scalar(0));

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

		//Banana
		Mat binBa(image.size(), CV_8UC1);
		auto itBa = binBa.begin<unsigned char>();

		//Bin image 
		for (auto it = image.begin<Vec3b>(); it != image.end<Vec3b>(); it++, itBa++) 
		{
			if ((*it)[2] < 20 || (*it)[1] < 20)
			{
				*itBa = 0;
				continue;
			}

			if ((*it)[0] >= 20 && (*it)[0] <=30)
			{
				*itBa = 255;
				continue;
			}
			*itBa = 0;
		}

		Mat labelBa, statBa, centroidsBa;
		int countBa = connectedComponentsWithStats(binBa, labelBa, statBa, centroidsBa);    //countBa stores the number of the connected component.
		if (countBa < 9) 
		{
			cout << "The Number of Yellow area less than 8 " << endl;
			showImg(src);
			continue;
		}

		// Find out the 8 max size connected components
		vector<Connected_Component>AllPiece;
		for (int i = 1; i < countBa; i++)
		{
			Connected_Component piece;
			piece.area= statBa.at<int>(i, CC_STAT_AREA);
			piece.index = i;
			AllPiece.push_back(piece);
		}
		sort(AllPiece.begin(), AllPiece.end(), [](Connected_Component& a, Connected_Component& b) { return a.area > b.area; });

		////Clear the picture ,leave 8 bananna
		//Mat binBaClear(image.size(), CV_8UC1);
		//auto drawit = binBaClear.begin<unsigned char>();
		//for (auto it = labelBa.begin<int>(); it != labelBa.end<int>(); it++, drawit++)
		//{
		//	if (*it == AllPiece[0].index || *it == AllPiece[1].index || *it == AllPiece[2].index
		//		|| *it == AllPiece[3].index || *it == AllPiece[4].index || *it == AllPiece[5].index
		//		|| *it == AllPiece[6].index || *it == AllPiece[7].index)
		//	{
		//		*drawit = 255;
		//	}
		//	else 
		//	{
		//		*drawit = 0;
		//	}
		//}


		for (size_t i = 0; i < 8; i++)
		{
			Point pBa(centroidsBa.at<double>(AllPiece[i].index, 0), centroidsBa.at<double>(AllPiece[i].index, 1));
			drawMarker(src, pBa, Scalar(0));
		}


		MyPoint ApplePoint;
		ApplePoint.x = centroids.at<double>(maxIndex, 0);
		ApplePoint.y = centroids.at<double>(maxIndex, 1);
		ApplePoint.isApple = true;

		//Compute the order of apple.
		vector<MyPoint>PointList;

		PointList.push_back(ApplePoint);
		for (int i = 0; i < 8; i++)
		{
			MyPoint BaPoint;
			BaPoint.x = centroidsBa.at<double>(AllPiece[i].index, 0);
			BaPoint.y = centroidsBa.at<double>(AllPiece[i].index, 1);
			BaPoint.isApple = false;
			PointList.push_back(BaPoint);
		}

		auto it = PointList.begin();
		sort(PointList.begin(), PointList.end(), CompRow);
		sort(it,it+3, CompColumn);
		sort(it+3, it+6, CompColumn);
		sort(it+6, it+9, CompColumn);

		int order;
		for (size_t i = 0; i < 9; i++)
		{
			if (PointList[i].isApple)
			{
				order = i + 1;
				break;
			}
		}
		if (order != lastOrder)
		{
			cout << "The apple's position is  " << order << endl;
			lastOrder = order;

			if (!mySerialPort.BuffSend(order))
			{
				std::cout << "SerialPort Data Send fail !" << std::endl;
			}
			else
			{
				std::cout << "SerialPort Data Send success !" << std::endl;
			}
		}

		//Banana End
		/////////////////////////////////////////////////////////////////////////////////////////////////////////
		if (!showImg(src))
			break;
	}

	return 0;
}