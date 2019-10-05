// ==============================
// By Jimmy
//
// 2018/12/14
//
// 1. sort out code
// ==============================

#include "socket_client.h"

#include "opencv2/opencv.hpp"

class ClientSocket
{
    SOCKET client_sock; // client fd

    PORT server_port; // server port number
    IP_ADDR server_ip; // server ip address

    int quality; // jpeg compression [1..100], not use in here

    // connect to server
    //
    // @port: port number
    //
    // @return: connect sueecssful or not
    //
    bool _connect(int server_port)
    {
        //std::cout<<"_connect"<<std::endl;

        // initial client fd
        client_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

        SOCKADDR_IN address; // server address information
        address.sin_addr.s_addr = inet_addr(SERVER_IP); // server address for connecting
        address.sin_family = AF_INET; // tcp
        address.sin_port = htons(server_port); // convert server port number to network format

        // connect to server
        if (connect(client_sock, (struct sockaddr *)&address, sizeof(address)) == SOCKET_ERROR)
        {
            std::cerr << "error : couldn't connect on client sock " << client_sock << " on port " << server_port << " !" << std::endl;
            return _release();
        }

        return true;
    }

    // shutdown client socket
    bool _release()
    {
        if (client_sock != INVALID_SOCKET)
            shutdown(client_sock, 2); // disable receive or send data, like close()
        client_sock = (INVALID_SOCKET);
        return false;
    }


    // ===============================================================================================================================
    // ===================================================== Write Function ==========================================================
    // ===============================================================================================================================

    // send message to server
    //
    // @send_sock: server fd
    // @message: string of message
    // @message_len: message length
    // 
    // @return: message length client already send
    //
    int _write(int send_sock, std::string message, int message_len)
    {
        int send_len = 0;

        send_len = send(send_sock, message.c_str(), message_len, 0);

        return send_len;
    }

    // send frame to server
    //
    // @send_sock: server fd
    // @frame: frame data
    // @frame_len: frame data length
    // 
    // @return: frame data length client already send
    //
    int _write_frame(int &send_sock, std::vector<uchar> frame, int frame_len)
    {
        std::cout<<"_write_frame"<<std::endl;

        int send_len = 0;

        send_len = send(send_sock, frame.data(), frame_len, 0);

        return send_len;
    }

    // send data length to server, then server known how many data it need to receive
    //
    // @send_sock: server fd
    // @data_len: data length client need to send after
    // 
    // @return: data_len size (int is 4 bytes)
    //
    int _write_len(int send_sock, int data_len)
    {
        //std::cout<<"_write_len"<<std::endl;

        int send_len = 0;  

        send_len = send(send_sock, &data_len, sizeof(int), 0);

        return send_len;
    }

    int _write_time_stamp(int send_sock, double time_stamp)
    {
        std::cout<<"_write_time_stamp"<<std::endl;

        int send_len = 0;  

        send_len = send(send_sock, &time_stamp, sizeof(double), 0);

        return send_len;
    }

    // ===============================================================================================================================
    // ===================================================== Read Function ===========================================================
    // ===============================================================================================================================

    // receive message from server
    //
    // @receive_sock: server fd
    // @message_len: message length
    //
    // @return: message
    //
    std::string _read(int receive_sock, int message_len)
    {
        std::string message; // declare message string
        message.clear(); // initial message string

        int receive_len = 0; // message length already receive
        int rest_len = message_len; // message client need to receive

        // receive message buffer, "......\0"
        // +1 is for big data (4096), because it needs '\0' to decide string end
        char buffer[BUFFER_MAX+1];
        memset(buffer, '\0', sizeof(buffer)); // initial message buffer with '\0'

        //std::cout<<"should receive: "<<message_len<<std::endl;

        // BUFFER_MAX is define in the socket_header.h
        if(message_len <= BUFFER_MAX)
        {
            // if message length is not larger than buffer size, it can just recevie one time
            
            // receive message into message buffer
            receive_len = recv(receive_sock, buffer, message_len, 0);
            // add message
            message += buffer;
        }
        else
        {
            // if message length is larger than buffer size, it need to recevie many time until less than buffer size

            // if message length client need to receive larger than zero, client need to receive again and again
            while(rest_len > 0)
            {
                if(rest_len <= BUFFER_MAX)
                {
                    // if message length is not larger than buffer size

                    // receive message into message buffer
                    receive_len += recv(receive_sock, buffer, rest_len, 0);
                    // update message length client need to receive
                    rest_len = message_len - receive_len;
                }
                else
                {
                    // if message length is larger than buffer size

                    // receive message into message buffer
                    receive_len += recv(receive_sock, buffer, BUFFER_MAX, 0);
                    // update message length client need to receive
                    rest_len = message_len - receive_len;
                }

                // add message
                message += buffer;
                // clear message buffer for receive new message
                memset(buffer, '\0', sizeof(buffer));

                //std::cout<<rest_len<<std::endl;
            }
        }

        //std::cout<<message<<" ("<<message.size()<<")"<<std::endl;

        return message;
    }

    // receive frame data form server
    //
    // @receive_sock: server fd
    // @frame_len: frame data length
    //
    // @return: frame data
    //
    std::vector<unsigned char> _read_frame(int receive_sock, int frame_len)
    {
        std::vector<unsigned char> frame; // declare frame data vector
        frame.clear(); // initial frame data vector

        int receive_len = 0; // frame data length already receive
        int rest_len = frame_len; // frame data length client need to receive

        // receive frame data buffer, "......\0"
        // +1 is for big data (4096), because it needs '\0' to decide string end
        unsigned char buffer[BUFFER_MAX+1]; 
        memset(buffer, '\0', sizeof(buffer)); // initial frame data buffer with '\0'

        //std::cout<<"should receive: "<<frame_len<<std::endl;

        // BUFFER_MAX is define in the socket_header.h
        if(frame_len <= BUFFER_MAX)
        {
            // if frame data length is not larger than buffer size, it can just recevie one time
            
            // receive frame in to frame data buffer
            receive_len = recv(receive_sock, buffer, frame_len, 0);
            // put all frame data in to frame data vector
            frame.insert(frame.end(), buffer, buffer + frame_len);
        }
        else
        {
            // if frame data length is larger than buffer size, it need to recevie many time until less than buffer size

            // if frame data length client need to receive larger than zero, client need to receive again and again
            while(rest_len > 0)
            {
                if(rest_len <= BUFFER_MAX)
                {
                    // if frame data length is not larger than buffer size

                    // receive frame into frame data buffer
                    receive_len = recv(receive_sock, buffer, rest_len, 0);
                    // update frame data length client need to receive
                    rest_len -= receive_len;
                }
                else
                {
                    // if frame data length is larger than buffer size

                    // receive frame into frame data buffer
                    receive_len = recv(receive_sock, buffer, BUFFER_MAX, 0);
                    // update frame data length client need to receive
                    rest_len -= receive_len;
                }

                // put all frame data in to frame data vector
                frame.insert(frame.end(), buffer, buffer + receive_len);
                // clear frame data  buffer for receive new frame data
                memset(buffer, '\0', sizeof(buffer));

                //std::cout<<frame.size()<<std::endl;
                //std::cout<<rest_len<<std::endl;
                //std::cout<<receive_len<<std::endl;
            }
        }

        //std::cout<<message<<" ("<<message.size()<<")"<<std::endl;

        // return 
        return frame;
    }

    // receive data length from server, then server known how many data it need to receive 
    // 
    // @receive_sock: server fd
    //
    // @return: data length
    //
    int _read_len(int receive_sock)
    {
        int data_len = 0;
        recv(receive_sock, &data_len, sizeof(int), 0);

        return data_len;
    }

public:

    // constructor
    // _port: port number
    // _timeout: timeout for select() timeval
    // _quality: jpeg compression [1..100] for cv::imencode (the higher is the better)
    ClientSocket(std::string _ip = "", int _port = 0, int _quality = 30)
        : server_ip(_ip)
        , server_port(_port)
        , client_sock(INVALID_SOCKET)
        , quality(_quality)
    {
        // signal(SIGPIPE, SIG_IGN); // ignore ISGPIP to avoid client crash and server is forcde to stop

        std::cout<<"constructor"<<std::endl;

        if(server_ip.size() && server_port) _connect(server_port); // if port > 0, then create a server with port
    }

    // destructor
    ~ClientSocket()
    {
        _release();
    }

    // check sock is opened or not
    bool isOpened()
    {
        return client_sock != INVALID_SOCKET;
    }

    // ===============================================================================================================================
    // ===================================================== Start Function ==========================================================
    // ===============================================================================================================================

    // @s: constant pointer to a constant char for message
    /*bool start()
    {
        std::string message = "test";
        
        int send_len = 0;
        send_len = _write_len(client_sock, message.size());
        printf("send: %d\n", send_len);

        send_len = _write(client_sock, message.c_str(), message.size());
        printf("send: %d\n", send_len);

        return true;
    }*/

    // send frame data to server
    //
    // @frame: frame data
    //
    bool start_frame(std::vector<uchar> &frame)
    {
        // frame data length client need to send
        int send_len = 0;
        send_len = _write_len(client_sock, frame.size());

        printf("frame size: %d\n", frame.size());
        printf("send: %d\n", send_len);

        // if send_len is equal to -1 (less than zero), it is mean server is closed
        if(send_len < 0) 
        {
            return false;
        }

        // send frame data to server
        send_len = _write_frame(client_sock, frame, frame.size());
        printf("send: %d\n", send_len);

        // if send_len is equal to -1 (less than zero), it is mean server is closed
        if(send_len < 0)
        {
            return false;
        }

        return true;
    }

    // send frame data and frame stamp to server
    //
    // @frame: frame data
    // @frame_stamp: frame stamp
    //
    bool start_frame_with_time_stamp(std::vector<unsigned char> frame, double time_stamp)
    {
        // send frame stamp to server
        _write_time_stamp(client_sock, time_stamp);

        // frame data length client need to send
        int send_len = 0;
        send_len = _write_len(client_sock, frame.size());

        printf("frame size: %d\n", frame.size());
        printf("send: %d\n", send_len);

        // if send_len is equal to -1 (less than zero), it is mean server is closed
        if(send_len < 0)
        {
            return false;
        }

        // send frame data to server
        send_len = _write_frame(client_sock, frame, frame.size());
        printf("send: %d\n", send_len);

        // if send_len is equal to -1 (less than zero), it is mean server is closed
        if(send_len < 0)
        {
            return false;
        }

        return true;
    }
};


// ===============================================================================================================================
// =============================================== Function Call for Client ======================================================
// ===============================================================================================================================

/*int send_message(std::string ip, int port, int quality)
{
    static ClientSocket client_socket(ip, port, quality);

    client_socket.start();

    return 0;
}*/

static IplImage *image_to_ipl(image im)
{
    int x,y,c;
    IplImage *disp = cvCreateImage(cvSize(im.w,im.h), IPL_DEPTH_8U, im.c);
    int step = disp->widthStep;
    for(y = 0; y < im.h; ++y){
        for(x = 0; x < im.w; ++x){
            for(c= 0; c < im.c; ++c){
                float val = im.data[c*im.h*im.w + y*im.w + x];
                disp->imageData[y*step + x*im.c + c] = (unsigned char)(val*255);
            }
        }
    }
    return disp;
}

static cv::Mat image_to_mat(image im)
{
    image copy = copy_image(im);
    constrain_image(copy);
    if(im.c == 3) rgbgr_image(copy);

    IplImage *ipl = image_to_ipl(copy);
    cv::Mat m = cv::cvarrToMat(ipl, true);
    cvReleaseImage(&ipl);
    free_image(copy);
    return m;
}


int send_frame(std::string ip, int port, int quality, std::vector<uchar> &frame)
{
    static ClientSocket client_socket(ip, port, quality);

    if(!client_socket.start_frame(frame)) // check whether server is cracked
    {
        exit(EXIT_FAILURE);
    }

    return 0;
}

int send_frame(std::string ip, int port, int quality, image im, double time_stamp)
{
    static ClientSocket client_socket(ip, port, quality);

    std::vector<unsigned char> frame;
    std::vector<int> compression_params;
    compression_params.push_back(cv::IMWRITE_JPEG_QUALITY); // default quality value is 95
    compression_params.push_back(quality);
    cv::imencode(".jpg", image_to_mat(im), frame, compression_params); // encodes an image into a memory buffer

    if(!client_socket.start_frame_with_time_stamp(frame, time_stamp)) // check whether server is cracked
    {
        exit(EXIT_FAILURE);
    }

    return 0;
}

int send_frame(std::string ip, int port, int quality, image im)
{
    static ClientSocket client_socket(ip, port, quality);

    std::vector<unsigned char> frame;
    std::vector<int> compression_params;
    compression_params.push_back(cv::IMWRITE_JPEG_QUALITY); // default quality value is 95
    compression_params.push_back(quality);
    cv::imencode(".jpg", image_to_mat(im), frame, compression_params); // encodes an image into a memory buffer

    if(!client_socket.start_frame(frame)) // check whether server is cracked
    {
        exit(EXIT_FAILURE);
    }

    return 0;
}