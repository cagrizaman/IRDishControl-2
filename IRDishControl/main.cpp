#include <iostream>
#include <iomanip>
#include <opencv2/opencv.hpp>
#include <cvblob.h>

using namespace cvb;
using namespace cv;
using namespace std;

//properties for background-subtraction mog2
#define varThreshold 50
#define bShadowDetection false
#define history 1000

//properties for capture
#define capWidth 480
#define capHeight 270

//break condition for delay-loops
#define inputWait 1000000

//calibration-values
int AHor, AVer;
int BHor, BVer;
int CHor, CVer;
int DHor, DVer;
int EHor, EVer;
int FHor, FVer;

//current angles
int PHor, PVer;

//setup variables for blobs and capture
CvTracks tracks;
VideoCapture cap(0);
BackgroundSubtractorMOG2 bg (history,varThreshold,bShadowDetection);
Mat frame;
Mat fogrMask;
Mat fogr;
Mat bagr;
IplImage frame2;
IplImage fogrMask2;

//function to map value from source-range to new range
int mapToRange(int input, int srcRangeMin, int srcRangeMax, int dstRangeMin, int dstRangeMax)
{
    return (int) ((input-srcRangeMax)/(srcRangeMax-srcRangeMin))*(dstRangeMax-dstRangeMin)+dstRangeMin;
}

//function to determine angles to rotate dish to. determined through pixel-coordinates and calibration-Values
void calcAngles(int PX, int PY)
{
    //calculate PHor
    
    int PHorMin = mapToRange(PY, 0, capHeight, AHor, DHor);
    int PHorMax = mapToRange(PX, 0, capWidth, BHor, CHor);
    PHor = mapToRange(PX, 0, capWidth, PHorMin, PHorMax);
    
    //calculate PVer
    
    int PVerMin;
    int PVerMax;
    //calculate PVer-extremes depending on what horizontal half of the picture P is in
    if (PX > capWidth/2)
    {
        PVerMin = mapToRange(PX, capWidth/2, capWidth, FVer, BVer);
        PVerMax = mapToRange(PX, 0, capWidth/2, EVer, CVer);
    }
    else
    {
        PVerMin = mapToRange(PX, 0, capWidth/2, AVer, FVer);
        PVerMax = mapToRange(PX, 0, capWidth/2, DVer, EVer);
    }
    PVer = mapToRange(PY, 0, capHeight, PVerMin, PVerMax);
}

void updateDish()
{
    
    //send P angles to arduino via serial
    
}

void displayImages(IplImage* img1, IplImage* img2)
{
    
    IplImage* image = cvCreateImage(cvSize(capWidth*2, capHeight), IPL_DEPTH_8U, 3);
    IplImage* img22 = cvCreateImage(cvSize(capWidth, capHeight), IPL_DEPTH_8U, 3);
    
    cvMerge(img2, img2, img2, NULL, img22);
    
    
    cvSetImageROI(image, Rect(0,0,capWidth, capHeight));
    cvResize(img1, image);
    cvResetImageROI(image);
    
    cvSetImageROI(image, Rect(capWidth ,0 ,capWidth , capHeight));
    cvResize(img22, image);
    cvResetImageROI(image);
    
    cvShowImage("Display", image);
    
}

void loadSavedConfig()
{
    AHor = 0; AVer = 0;
    BHor = 0; BVer = 0;
    CHor = 0; CVer = 0;
    DHor = 0; DVer = 0;
    EHor = 0; EVer = 0;
    FHor = 0; FVer = 0;
}

void calibrateDish()
{
    char key;
    int stage = 1;
    int radius = 30;
    
    while (1)
    {
        
        cap >> frame;
        switch (stage)
        {
            case 1:
                circle(frame, Point2d(0,0), radius, Scalar(255,255,255), 3); break;
            case 2:
                circle(frame, Point2d(capWidth,0), radius, Scalar(255,255,255), 3); break;
            case 3:
                circle(frame, Point2d(capWidth,capHeight), radius, Scalar(255,255,255), 3); break;
            case 4:
                circle(frame, Point2d(0,capHeight), radius, Scalar(255,255,255), 3); break;
            case 5:
                circle(frame, Point2d(capWidth/2,0), radius, Scalar(255,255,255), 3); break;
            case 6:
                circle(frame, Point2d(capWidth/2,capHeight), radius, Scalar(255,255,255), 3); break;
        }
        
        imshow("Display", frame);
        key = '0';
        key = cvWaitKey(5);
        switch (key)
        {
            case 'w': case 'W': PVer ++; cout << 'W'; break;
            
            case 's': case 'S': PVer --; cout << 'S'; break;
            
            case 'D': case 'd': PHor ++; cout << 'D'; break;
            
            case 'A': case 'a': PHor --; cout << 'A'; break;
            
            case 'N': case 'n': cout << 'N';
                switch (stage)
                {
                    case 1: AHor = PHor; AVer = PVer; break;
                    case 2: BHor = PHor; BVer = PVer; break;
                    case 3: CHor = PHor; CVer = PVer; break;
                    case 4: DHor = PHor; DVer = PVer; break;
                    case 5: EHor = PHor; EVer = PVer; break;
                    case 6: FHor = PHor; FVer = PVer; return;
                }
                for (int i = 0; i<inputWait; i++) {}
                stage++; cout << "Stage: " << stage << endl; break;
            
            case 'L': case 'l':
                loadSavedConfig(); stage = 7;
                cout << 'load config';
                return;
        }
        updateDish();
    }
    
}

void detectAndTrace()
{
    //loop:
    //  update frame
    //  detect blobs
    //  draw tracks
    //  determine track to be aimed at
    //  draw aimed at track and aimed at point in different color
    //  convert blob-center to angles
    //  update dish
    //  show image
    
    for(;;)
    {
        cap >> frame;
        
        bg.operator() (frame, fogrMask);
        
        erode(fogrMask, fogrMask, Mat(), Point(-1,-1), 2);
        dilate(fogrMask, fogrMask,Mat(), Point(-1,-1), 5);
        
        fogrMask2 = fogrMask;
        
        frame2 = frame;
        
        IplImage* labelImg = cvCreateImage(cvSize(capWidth, capHeight), IPL_DEPTH_LABEL, 1);
        
        CvBlobs blobs;
        
        unsigned int result = cvLabel(&fogrMask2, labelImg, blobs);
        
        //cvFilterByArea(blobs, 200, 1000000);
        //cvRenderBlobs(labelImg, blobs, &frame2, &frame2, CV_BLOB_RENDER_BOUNDING_BOX);
        cvUpdateTracks(blobs, tracks, 200., 5);
        cvRenderTracks(tracks, &frame2, &frame2, CV_TRACK_RENDER_ID|CV_TRACK_RENDER_BOUNDING_BOX);
        
        
        
        int abc = 0, PX, PY;
        for (CvBlobs::const_iterator it=blobs.begin(); it!=blobs.end(); ++it)
        {
            if (it->second->label == 1)
            {
                PX = (int)it->second->centroid.x;
                PY = (int)it->second->centroid.y;
            }
            //cout << "Blob #" << it->second->label << ": Area=" << it->second->area << ", Centroid=(" << it->second->centroid.x << ", " << it->second->centroid.y << ")" << endl;
            abc++;
        }

        circle(frame, Point2d(PX,PY), 10, Scalar(255,255,255), -1 );
        displayImages(&frame2, &fogrMask2);
        calcAngles(PX, PY);
        updateDish();
        
        if(waitKey(30) >= 0) break;
    }
    
    
}

int main()
{
    //settings for capture size
    cap.set(CV_CAP_PROP_FRAME_HEIGHT,capHeight);
    cap.set(CV_CAP_PROP_FRAME_WIDTH,capWidth);
    
    namedWindow("Display");

    
    calibrateDish();
    
    detectAndTrace();
    
    return 0;
    
}
