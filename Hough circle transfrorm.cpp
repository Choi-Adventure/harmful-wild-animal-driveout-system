#include <csignal>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>       // Used for UART
#include <sys/fcntl.h>    // Used for UART
#include <termios.h>      // Used for UART+
#include <sys/stat.h>
#include <string>
#include <iostream>
#include <fcntl.h>
#include <JetsonGPIO.h>
#include <pthread.h>
#include <fstream>
#include <opencv2/opencv.hpp>
#include <errno.h>

using namespace cv;
using namespace GPIO;
using namespace std;
using namespace dnn;

Mat image, gray_image;

int hough_flag = 0;

vector<Vec3f> circles;

VideoCapture cap(0);

void signalingHandler(int signo);

extern "C" void *predater(void*)
{
    system("canberra-gtk-play -f tiger.wav");
    return NULL;
}

int main(void)
{
    signal(SIGINT, signalingHandler);

    pthread_t p_thread[2];      // 쓰레드 ID
    int thread_id = 0;

    if(!cap.isOpened()){
        cout << "Camera open failed!" << endl;
        return -1;
    }

    while(1)
    {
        cap >> image;

        cvtColor(image, gray_image, COLOR_BGR2GRAY); //영상을 그레이스케일로 변경

        Mat blurred;
        blur(gray_image, blurred, Size(3,3));

        //threshold(blurred, dst1, 200, 255, THRESH_BINARY);

        //Canny(dst1, dst2, 150, 200);
        
        vector<Vec3f> circles;
        HoughCircles(blurred, circles, HOUGH_GRADIENT, 1, 50, 350, 20);
        //350은 내부의 캐니 에지 검출 max값임 350이고 min값은 175
        //20은 원 검출을 위한 정보로, accumulator의 threshold 값 너무 작으면 거짓 원이 검출 됨(아마 20픽셀 그레디언트 이상은 쌓여야 원 인정?) 가장 큰 accumulator의 값이 가장 먼저 원으로 반환됨

        Mat color;
        cvtColor(blurred, color ,COLOR_GRAY2BGR);

        for(Vec3f c : circles)
        {
            Point center(cvRound(c[0]), cvRound(c[1]));

            int radius = cvRound(c[2]); //원 검출 된 반지름 길이
            circle(color, center, radius, Scalar(0,0,255), 2, LINE_AA);

            //cout << c[2] << endl;

            if(radius <= 15) //검출된 원 반지름이 작은 것이 동물의 눈
            {
                thread_id = pthread_create(&p_thread[1], NULL, predater, NULL);
                if(thread_id < 0)
                {
                    perror("thread create error : predater");
                    exit(1);
                }
                usleep(50000);
                cout << "boar's eyes detected!" << endl;
                //c[2] = 20;
            }

            else if(radius > 15)
            {
                hough_flag = 0;
            }
        }
        imshow("dst", color);

        waitKey(10);
    }
    return 0;
}

void signalingHandler(int signo) 
{
    printf("\n\nGoodbye World\n\n");
    cap.release();
    cleanup();
    destroyAllWindows();
    exit(signo);
}