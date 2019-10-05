#include "sort.h"
//#include "DAI_push.h"
#include "socket_client.h"

#include <cstdlib>
#include <cstdio>
#include <cmath>
#include <iostream>
#include <vector>
#include <algorithm>
#include <Python.h>

#define PERSON_ONLY 1
#define SORT_FREQ 1 // sort update frequency
#define MAX_SORT_NUM 500
#define ENABLE_SORT_TRACKER 1

#define CHECK_PYTHON_NULL(p) \
    if (NULL == (p)) {\
        PyErr_Print();\
        exit(EXIT_FAILURE);\
    }


// update by sort_update()
static std::vector<person_det> person_dets; // store new dets
static std::vector<person_sort_det> person_sort_dets; // store new dets
static std::vector<person_sort_compare_det> person_sort_compare_dets; // store old dets to compare

static int first_called = 1;
//static int frame_stamp = 0;
static double time_stamp = 0;

static PyObject *pName, *pModule, *pDict, *pClass, *pTracker, *pTrackerRet;

#ifdef __cplusplus
extern "C" {
#endif

int get_time_stamp(double &time_stamp)
{
    struct timeval time;
    if (gettimeofday(&time, NULL))
    {
        return 0;
    }
    time_stamp = (double)time.tv_sec + (double)time.tv_usec * .000001;

    return 1;
}

//void draw_detections_with_sort_id(image im, box *boxes, float **probs, int num, float thresh, char **names, image **alphabet, int classes)
void draw_detections_with_sort_id(image im, detection *dets, int num, float thresh, char **names, image **alphabet, int classes)
{
    //frame_stamp++;
    get_time_stamp(time_stamp);
    //std::cout<<"time_stamp: "<<time_stamp<<std::endl;

    // ------------------------------------------
    // ---------- begin to get sort id ----------
    // ------------------------------------------

    //int person_num = sort_update(im, num, thresh, names, alphabet, classes);
    int person_num = sort_update(im, dets, num, thresh, names, alphabet, classes);
    printf("person_sort_dets.size(): %d\n", person_num);

    // send frame
    //send_frame(SERVER_IP, 8091, 95, im, time_stamp);

    // send box information to iot talk
    //iot_talk_send(person_sort_dets, time_stamp);

    float red = 0;
    float green = 255;
    float blue = 0;

    float rgb[3];
    rgb[0] = red;
    rgb[1] = green;
    rgb[2] = blue;

    // draw frame stamp on the image
    //char frame_stamp_label_str[4096] = {0};
    //sprintf(frame_stamp_label_str, "%d", frame_stamp);
    //image frame_stamp_label = get_label(alphabet, frame_stamp_label_str, (im.h*.03));
    //draw_label(im, 0, 0, frame_stamp_label, rgb);

    char timer_label_str[4096] = {0};
    sprintf(timer_label_str, "%lf", time_stamp);
    image timer_label = get_label(alphabet, timer_label_str, (im.h*.03));
    draw_label(im, 0, 0, timer_label, rgb);

    for(int i = 0; i < person_num; ++i)
    {
        char labelstr[4096] = {0};

        int id = person_sort_dets[i].id;
        int left = person_sort_dets[i].x1;
        int top = person_sort_dets[i].y1;
        int right = person_sort_dets[i].x2;
        int bot = person_sort_dets[i].y2;

        sprintf(labelstr, "%d", id);
        //printf("person: %d\n", person_sort_dets[i].id);
        printf("person id : %d\nx1: %d, y1: %d\nx2: %d, y2: %d\n", id, left, top, right, bot);

        // store box center for comparing
        person_sort_compare_det tmp_pscd = { id, left + (right - left) / 2, top + (bot - top) / 2 };
        person_sort_compare_dets.push_back(tmp_pscd);

        int width = im.h * .006;
        draw_box_width(im, left, top, right, bot, width, red, green, blue);
        if (alphabet)
        {
            image label = get_label(alphabet, labelstr, (im.h*.03));
            draw_label(im, top + width, left, label, rgb);
            free_image(label);
        }
    }
}

void sort_init()
{
    person_sort_dets.clear();
    person_sort_compare_dets.clear();

    if(!Py_IsInitialized())
    {
        Py_Initialize();
    }

    pModule = PyImport_ImportModule("sort");
    CHECK_PYTHON_NULL(pModule)

    pDict = PyModule_GetDict(pModule);
    pClass = PyDict_GetItemString(pDict, "Sort"); // get Sort class from sort.py

    if (PyCallable_Check(pClass))
    {
        pTracker = PyObject_CallObject(pClass, NULL);
    }
}

//int sort_update(image im, int num, float thresh, char **names, image **alphabet, int classes)
int sort_update(image im, detection *dets, int num, float thresh, char **names, image **alphabet, int classes)
{
    if(first_called)
    {
        first_called = 0;
        sort_init();
    }

    printf("classes: %d\n", classes);

    person_dets.clear();
    person_sort_dets.clear();

    // get all boxes(detections) information and sort
    for(int i = 0; i < num; ++i)
    {
        int class_index = -1;
        //printf("%d\n", classes);
        for(int j = 0; j < classes; ++j)
        {
            if (dets[i].prob[j] > thresh)
            {
                if (class_index < 0)
                {
                    class_index = j;
                }
                printf("%s: %.0f%%\n", names[j], dets[i].prob[j]*100);
            }
        }

        //printf("%d\n", class_index);

        if(class_index == 0)
        {
            /*
               if(0){
               width = pow(prob, 1./2.)*10+1;
               alphabet = 0;
               }
            */

            box b = dets[i].bbox;
            int left  = (b.x-b.w/2.)*im.w;
            int right = (b.x+b.w/2.)*im.w;
            int top   = (b.y-b.h/2.)*im.h;
            int bot   = (b.y+b.h/2.)*im.h;
            float prob = dets[i].prob[class_index];

            if(left < 0) left = 0;
            if(right > im.w-1) right = im.w-1;
            if(top < 0) top = 0;
            if(bot > im.h-1) bot = im.h-1;

            // skip too small box
            /*float filter_small_scale = 20;
            if(filter_small_scale!=0 && ((right-left)<im.h/filter_small_scale || (bot-top)<im.h/filter_small_scale))
                continue;*/

            // store person detection information
            printf("person: %d\nx1: %d, y1: %d\nx2: %d, y2: %d\n", i, left, top, right, bot);
            person_det tmp_pd = {left, top, right, bot, prob};
            person_dets.push_back(tmp_pd);
        }
    }

    int person_num = 0;

    // store all boxes(detections) information
    PyObject *pDetections = PyList_New(0);

    for(int i = 0; i < person_dets.size(); ++i)
    {
        person_det tmp_pd = person_dets[i];
        
        int left = tmp_pd.x1;
        int top = tmp_pd.y1;
        int right = tmp_pd.x2;
        int bot = tmp_pd.y2;

        float prob = tmp_pd.prob;

        // store left, top, right, bot, prob
        PyObject *pDetection = PyList_New(5);
        PyList_SetItem(pDetection, 0, PyInt_FromLong(left));
        PyList_SetItem(pDetection, 1, PyInt_FromLong(top));
        PyList_SetItem(pDetection, 2, PyInt_FromLong(right));
        PyList_SetItem(pDetection, 3, PyInt_FromLong(bot));
        PyList_SetItem(pDetection, 4, PyFloat_FromDouble(prob)); // ((rand() % 100) / 100)

        if(PyList_Append(pDetections, pDetection))
        {
            printf("ERROR: failed to append pDetection into pDetections.\n");
        }

        person_num++;

        // free pDetection
        Py_DECREF(pDetection);
    }

    // no any person
    if(person_num == 0)
    {
        printf("Do not detect any person!\n");
        Py_DECREF(pDetections);
        return 0;
    }

    printf("person_num: %d\n", person_num);
    printf("pDetections: %d\n", PyList_Size(pDetections));


    // get sort.update return
    pTrackerRet = PyObject_CallMethod(pTracker, "update", "O", pDetections);

    int pTrackerRetSize = PyList_Size(pTrackerRet);

    // no any person
    if(pTrackerRetSize == 0)
    {
        printf("Do not sort any person!\n");
        Py_DECREF(pDetections);
        return 0;
    }

    PyObject *pSortDets = NULL;

    int x1, y1, x2, y2, score;
    
    printf("pTrackerRetSize: %d\n", pTrackerRetSize);

    for(int i = 0; i < pTrackerRetSize; i++)
    {
        pSortDets = PyList_GetItem(pTrackerRet, i);
        x1 = PyInt_AsLong(PyList_GetItem(pSortDets, 0));
        y1 = PyInt_AsLong(PyList_GetItem(pSortDets, 1));
        x2 = PyInt_AsLong(PyList_GetItem(pSortDets, 2));
        y2 = PyInt_AsLong(PyList_GetItem(pSortDets, 3));
        score = PyInt_AsLong(PyList_GetItem(pSortDets, 4)) % MAX_SORT_NUM;

        if(i < person_num)
        {
            person_sort_det tmp_psd = {score, x1, y1, x2, y2};

            person_sort_dets.push_back(tmp_psd);
        }
    }

    // free pDetections
    Py_DECREF(pSortDets);
    Py_DECREF(pTrackerRet);
   	Py_DECREF(pDetections);

    return pTrackerRetSize;
    //return person_num;
}

void sort_cleanUp()
{
	Py_DECREF(pModule);
    Py_DECREF(pClass);
    Py_DECREF(pDict);
    Py_DECREF(pTracker);
    Py_DECREF(pName);
    Py_Finalize();
}

#ifdef __cplusplus
}
#endif