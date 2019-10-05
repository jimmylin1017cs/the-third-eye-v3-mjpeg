// ==============================
// By Jimmy
//
// 2018/12/14
//
// 1. sort out code
// ==============================

#ifdef __cplusplus
extern "C" {
#endif

//#include "image.h"

#ifdef __cplusplus
}
#endif

#include "image.h"
#include "socket_header.h"
#include <iostream>
#include <vector>

int send_frame(std::string ip, int port, int quality, std::vector<unsigned char> &frame);
int send_frame(std::string ip, int port, int quality, image im, double time_stamp);
int send_frame(std::string ip, int port, int quality, image im);

