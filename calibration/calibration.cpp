#include "opencv2/core/core.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/calib3d/calib3d.hpp"
#include "opencv2/highgui/highgui.hpp"
#include <iostream>
#include <fstream>

using namespace cv;
using namespace std;

enum { DETECTION = 0, CAPTURING = 1, CALIBRATED = 2 };
int main()
{
	/************************************************************************
	��������ж�ȡ���ͼ��,������ȡ���ǵ㣬Ȼ��Խǵ���������ؾ�ȷ��
	*************************************************************************/
	int image_count = 10;                    /****    ͼ������     ****/
	Mat frame;
	Size image_size;                         /****     ͼ��ĳߴ�      ****/
	Size board_size = Size(8, 6);            /****    �������ÿ�С��еĽǵ���       ****/
	vector<Point2f> corners;                  /****    ����ÿ��ͼ���ϼ�⵽�Ľǵ�       ****/
	vector<vector<Point2f>>  corners_Seq;    /****  �����⵽�����нǵ�       ****/
	ofstream fout("calibration_result.txt");  /**    ���涨�������ļ�     **/
	int mode = DETECTION;

	VideoCapture cap(0);
	cap.set(CV_CAP_PROP_FRAME_WIDTH, 640);
	cap.set(CV_CAP_PROP_FRAME_HEIGHT, 480);
	if (!cap.isOpened()) {
		std::cout << "������ͷʧ�ܣ��˳�";
		exit(-1);
	}
	namedWindow("Calibration");
	std::cout << "Press 'g' to start capturing images!" << endl;

	int count = 0, n = 0;
	stringstream tempname;
	string filename;
	int key;
	string msg;
	int baseLine;
	Size textSize;
	while (n < image_count)
	{
		frame.setTo(0);
		cap >> frame;
		if (mode == DETECTION)
		{
			key = 0xff & waitKey(30);
			if ((key & 255) == 27)
				break;

			if (cap.isOpened() && key == 'g')
			{
				mode = CAPTURING;
			}
		}

		if (mode == CAPTURING)
		{
			key = 0xff & waitKey(30);
			if ((key & 255) == 32)
			{
				image_size = frame.size();
				/* ��ȡ�ǵ� */
				Mat imageGray;
				cvtColor(frame, imageGray, CV_RGB2GRAY);
				bool patternfound = findChessboardCorners(frame, Size(8,6), corners, CALIB_CB_ADAPTIVE_THRESH + CALIB_CB_NORMALIZE_IMAGE + CALIB_CB_FAST_CHECK);
				if (patternfound)
				{
					n++;
					tempname << n;
					tempname >> filename;
					filename += ".jpg";
					/* �����ؾ�ȷ�� */
					cornerSubPix(imageGray, corners, Size(11, 11), Size(-1, -1), TermCriteria(CV_TERMCRIT_EPS + CV_TERMCRIT_ITER, 30, 0.1));
					count += corners.size();
					corners_Seq.push_back(corners);
					imwrite(filename, frame);
					tempname.clear();
					filename.clear();
				}
				else
				{
					std::cout << "Detect Failed.\n";
				}
			}
		}
		msg = mode == CAPTURING ? "100/100/s" : mode == CALIBRATED ? "Calibrated" : "Press 'g' to start";
		baseLine = 0;
		textSize = getTextSize(msg, 1, 1, 1, &baseLine);
		Point textOrigin(frame.cols - 2 * textSize.width - 10, frame.rows - 2 * baseLine - 10);

		if (mode == CAPTURING)
		{
			msg = format("%d/%d", n, image_count);
		}

		putText(frame, msg, textOrigin, 1, 1, mode != CALIBRATED ? Scalar(0, 0, 255) : Scalar(0, 255, 0));

		imshow("Calibration", frame);
		key = 0xff & waitKey(1);
		if ((key & 255) == 27)
			break;
	}

	std::cout << "�ǵ���ȡ��ɣ�\n";

	/************************************************************************
	���������
	*************************************************************************/
	std::cout << "��ʼ���ꡭ����������" << endl;
	Size square_size = Size(28, 28);                                      /**** ʵ�ʲ����õ��Ķ������ÿ�����̸�Ĵ�С   ****/
	vector<vector<Point3f>>  object_Points;                                      /****  ���涨����Ͻǵ����ά����   ****/

	Mat image_points = Mat(1, count, CV_32FC2, Scalar::all(0));          /*****   ������ȡ�����нǵ�   *****/
	vector<int>  point_counts;                                          /*****    ÿ��ͼ���нǵ������    ****/
	Mat intrinsic_matrix = Mat(3, 3, CV_32FC1, Scalar::all(0));                /*****    ������ڲ�������    ****/
	Mat distortion_coeffs = Mat(1, 5, CV_32FC1, Scalar::all(0));            /* �������5������ϵ����k1,k2,p1,p2,k3 */
	vector<Mat> rotation_vectors;                                      /* ÿ��ͼ�����ת���� */
	vector<Mat> translation_vectors;                                  /* ÿ��ͼ���ƽ������ */

																	  /* ��ʼ��������Ͻǵ����ά���� */
	for (int t = 0; t<image_count; t++)
	{
		vector<Point3f> tempPointSet;
		for (int i = 0; i<board_size.height; i++)
		{
			for (int j = 0; j<board_size.width; j++)
			{
				/* ���趨��������������ϵ��z=0��ƽ���� */
				Point3f tempPoint;
				tempPoint.x = i*square_size.width;
				tempPoint.y = j*square_size.height;
				tempPoint.z = 0;
				tempPointSet.push_back(tempPoint);
			}
		}
		object_Points.push_back(tempPointSet);
	}

	/* ��ʼ��ÿ��ͼ���еĽǵ������������Ǽ���ÿ��ͼ���ж����Կ��������Ķ���� */
	for (int i = 0; i< image_count; i++)
	{
		point_counts.push_back(board_size.width*board_size.height);
	}

	/* ��ʼ���� */
	calibrateCamera(object_Points, corners_Seq, image_size, intrinsic_matrix, distortion_coeffs, rotation_vectors, translation_vectors);
	std::cout << "������ɣ�\n";

	/************************************************************************
	�Զ�������������
	*************************************************************************/
	std::cout << "��ʼ���۶�����������������" << endl;
	double total_err = 0.0;                   /* ����ͼ���ƽ�������ܺ� */
	double err = 0.0;                        /* ÿ��ͼ���ƽ����� */
	vector<Point2f>  image_points2;             /****   �������¼���õ���ͶӰ��    ****/

	std::cout << "ÿ��ͼ��Ķ�����" << endl;
	fout << "ÿ��ͼ��Ķ�����" << endl << endl;
	for (int i = 0; i<image_count; i++)
	{
		vector<Point3f> tempPointSet = object_Points[i];
		/****    ͨ���õ������������������Կռ����ά���������ͶӰ���㣬�õ��µ�ͶӰ��     ****/
		projectPoints(tempPointSet, rotation_vectors[i], translation_vectors[i], intrinsic_matrix, distortion_coeffs, image_points2);
		/* �����µ�ͶӰ��;ɵ�ͶӰ��֮������*/
		vector<Point2f> tempImagePoint = corners_Seq[i];
		Mat tempImagePointMat = Mat(1, tempImagePoint.size(), CV_32FC2);
		Mat image_points2Mat = Mat(1, image_points2.size(), CV_32FC2);
		for (int j = 0; j < tempImagePoint.size(); j++)
		{
			image_points2Mat.at<Vec2f>(0, j) = Vec2f(image_points2[j].x, image_points2[j].y);
			tempImagePointMat.at<Vec2f>(0, j) = Vec2f(tempImagePoint[j].x, tempImagePoint[j].y);
		}
		err = norm(image_points2Mat, tempImagePointMat, NORM_L2);
		total_err += err /= point_counts[i];
		std::cout << "��" << i + 1 << "��ͼ���ƽ����" << err << "����" << endl;
		fout << "��" << i + 1 << "��ͼ���ƽ����" << err << "����" << endl;
	}
	std::cout << "����ƽ����" << total_err / image_count << "����" << endl;
	fout << "����ƽ����" << total_err / image_count << "����" << endl << endl;
	std::cout << "������ɣ�" << endl;
	 
	/************************************************************************
	���涨����
	*************************************************************************/
	std::cout << "��ʼ���涨����������������" << endl;
	Mat rotation_matrix = Mat(3, 3, CV_32FC1, Scalar::all(0)); /* ����ÿ��ͼ�����ת���� */

	fout << "����ڲ�������" << endl;
	fout << intrinsic_matrix << endl << endl;
	fout << "����ϵ����\n";
	fout << distortion_coeffs << endl << endl << endl;
	for (int i = 0; i<image_count; i++)
	{
		fout << "��" << i + 1 << "��ͼ�����ת������" << endl;
		fout << rotation_vectors[i] << endl;

		/* ����ת����ת��Ϊ���Ӧ����ת���� */
		Rodrigues(rotation_vectors[i], rotation_matrix);
		fout << "��" << i + 1 << "��ͼ�����ת����" << endl;
		fout << rotation_matrix << endl;
		fout << "��" << i + 1 << "��ͼ���ƽ��������" << endl;
		fout << translation_vectors[i] << endl << endl;
	}
	std::cout << "��ɱ���" << endl;
	fout << endl;

	/************************************************************************
	��ʾ������
	*************************************************************************/
	 /*	Mat mapx = Mat(image_size,CV_32FC1);
	 	Mat mapy = Mat(image_size,CV_32FC1);
	 	Mat R = Mat::eye(3,3,CV_32F);
	 	std::cout<<"�������ͼ��"<<endl;
	 	string imageFileName;
	 	std::stringstream StrStm;
	 	for (int i = 0 ; i != image_count ; i++)
	 	{
	 		std::cout<<"Frame #"<<i+1<<"..."<<endl;
	 		Mat newCameraMatrix = Mat(3,3,CV_32FC1,Scalar::all(0));
	 		initUndistortRectifyMap(intrinsic_matrix,distortion_coeffs,R,intrinsic_matrix,image_size,CV_32FC1,mapx,mapy);
	 		StrStm.clear();
	 		imageFileName.clear();
	 		StrStm<<i+1;
	 		StrStm>>imageFileName;
	 		imageFileName += ".jpg";
	 		Mat t = imread(imageFileName);
	 		Mat newimage = t.clone();
	 		cv::remap(t,newimage,mapx, mapy, INTER_LINEAR);
	 		StrStm.clear();
	 		imageFileName.clear();
	 		StrStm<<i+1;
	 		StrStm>>imageFileName;
	 		imageFileName += "_d.jpg";
	 		imwrite(imageFileName,newimage);
	 	}
	 	std::cout<<"�������"<<endl;*/
	system("pause");
	return 0;
}