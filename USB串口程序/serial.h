#ifndef __SERIAL_H
#define __SERIAL_H

HANDLE Ser_Init(char *com);


struct com_device
{
    char COM_NAME[7];
    char USB_PID[5];
    char USB_VID[5];
};

extern struct com_device *COM[MAX_PATH];

//在注册表查找串口设备
int search_com_device(void);

//初始化串口设备
HANDLE Ser_Init(char *com);

//查看串口设备发送的数据
int ser_read(HANDLE ser);



#endif


