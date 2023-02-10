#include "serial.h"
#include "main.h"

struct com_device *COM[MAX_PATH];

//获取串口设备的USB描述符 
static int Get_COM_USB_Info(struct com_device *COMx, char *buf, size_t buf_len)
{
    unsigned int i, j;

    if((buf_len <= 8) || (COMx == NULL))
    {
        return 0;
    }
    memset(COMx, 0, sizeof(struct com_device));
    for(i = 0; i < buf_len - 8; i ++)
    {
        
        if((buf[i] == 'v') && (buf[i + 1] == 'i') && (buf[i + 2] == 'd') && (buf[i + 3] == '_'))
        {
            for(j = 0; j < 4; j++)
            {
                COMx->USB_VID[j] = buf[i + j + 4];
            }
        }
        if((buf[i] == 'p') && (buf[i + 1] == 'i') && (buf[i + 2] == 'd') && (buf[i + 3] == '_'))
        {
            for(j = 0; j < 4; j++)
                COMx->USB_PID[j] = buf[i + j + 4];
            break;
        }
    }
    return 1;
}

//查找串口
int search_com_device(void)
{
    HKEY hkey;
    TCHAR Key_Name[MAX_PATH];//注册表的名称
    BYTE Key_Val[MAX_PATH];//注册表的值
    DWORD Key_Index = 0;//注册表索引
    DWORD Name_Size = MAX_PATH, Val_Size = MAX_PATH;
    DWORD type = 0;
    long status;

    //获取key值的数据，从函数中出来的格式不是utf-8，需要转换才能正常打印
    char com_name[MAX_PATH];
    char val_data[MAX_PATH];
    int length = MAX_PATH;

    //key在注册表的地址
    LPCTSTR KEY_PATH = TEXT("SYSTEM\\CurrentControlSet\\Control\\COM Name Arbiter\\Devices");

    //打开注册表
    status = RegOpenKeyEx(HKEY_LOCAL_MACHINE, KEY_PATH, 0, KEY_WOW64_64KEY | KEY_READ, &hkey);
    if(status != ERROR_SUCCESS)//如果失败
    {
        printf("open key error\n");
        RegCloseKey(hkey); //关闭注册表
        return ERROR;
    }

    while(1)
    {
        //枚举注册表，提取名称和值
        status = RegEnumValue(hkey, Key_Index++, Key_Name, &Name_Size, NULL, &type, Key_Val, &Val_Size);
        if(status == ERROR_SUCCESS)
        {
            memset(com_name, 0, sizeof(com_name));
            memset(val_data, 0, sizeof(val_data));
            //取名称，查看现在有几个串口在工作
            WideCharToMultiByte(CP_ACP, 0, (LPCWCH)Key_Name, -1, com_name, length, NULL, NULL);
            //取值，查看串口对应的USB设备
            WideCharToMultiByte(CP_ACP, 0, (LPCWCH)Key_Val, -1, val_data, length, NULL, NULL);
            //申请空间
            COM[Key_Index - 1] = (struct com_device*)malloc(sizeof(struct com_device) + 1);
            if(COM[Key_Index - 1] == NULL)
            {
                printf("内存申请失败\n");
                return 0;
            }
            Get_COM_USB_Info(COM[Key_Index - 1], val_data, strlen(val_data));
            memcpy(COM[Key_Index - 1]->COM_NAME, com_name, 6);
        }
        else
            break;
        Name_Size = MAX_PATH;
        Val_Size = MAX_PATH;
    }
    RegCloseKey(hkey); //关闭注册表
    return Key_Index - 1;
}


//串口初始化
HANDLE Ser_Init(char *com)
{
    HANDLE  ser1;
    TCHAR com_num[7] = {0};
    
    if(com == NULL)
        return NULL;

    for(int i = 0; i < strlen(com); i++)
    {
        com_num[i] = com[i];
    }

    //成功返回一个句柄，失败返回-1
    ser1 = CreateFile(com_num,//指向文件名的指针，这里是COM9
        GENERIC_READ | GENERIC_WRITE,//访问模式，允许读写
        0,//共享模式，独占方式
        NULL,//指向安全属性的指针，这里不需要
        OPEN_EXISTING,//打开，非创建
        0,//同步方式
        NULL);//用于复制文件句柄，这里不需要
    if(ser1 == INVALID_HANDLE_VALUE)
    {
        printf("open com9 error\n");
        return ser1;
    }
    else
    {
        printf("open com9 success\n");
    }
    SetupComm(ser1, 20480, 20480);//输入输出缓冲

    COMMTIMEOUTS TimeOuts;
    TimeOuts.ReadIntervalTimeout = 100;
    TimeOuts.ReadTotalTimeoutMultiplier = 500;
    TimeOuts.ReadTotalTimeoutConstant = 5000;
    TimeOuts.WriteTotalTimeoutMultiplier = 500;//写超时
    TimeOuts.WriteTotalTimeoutConstant = 2000;
    SetCommTimeouts(ser1, &TimeOuts);

    DCB dcb1;
    GetCommState(ser1, &dcb1);
    dcb1.BaudRate = 115200;//波特率
    dcb1.ByteSize = 8;//8 bit
    dcb1.Parity = NOPARITY;//无奇偶校验
    dcb1.StopBits = TWOSTOPBITS;//两个停止位
    dcb1.fParity = FALSE;
    dcb1.fNull = FALSE;
    SetCommState(ser1, &dcb1);
    PurgeComm(ser1, PURGE_TXCLEAR | PURGE_RXCLEAR);//清空缓冲
    return ser1;
}







int ser_read(HANDLE ser)
{
    char buf[512] = {0};
    unsigned int cnt = 0;
    DWORD Rlen = 256, Olen = 0;

    if((ser == INVALID_HANDLE_VALUE) || (ser == NULL))
    {
        printf("open ser error\n");
        return 0;
    }
    while(1)
    {
        if(!ReadFile(ser, buf, Rlen, &Olen, NULL))
        {
            printf("read com9 error\n");
            return 1;
        }
        printf("cnt:%02d  %s", cnt, buf);
        memset(buf, 0, sizeof(buf));
        cnt++;
        if(cnt > 99)
            break;
    }
    printf("%s\n", buf);
    if((ser != INVALID_HANDLE_VALUE) && (ser != NULL))
        CloseHandle(ser);
    return 1;
}




