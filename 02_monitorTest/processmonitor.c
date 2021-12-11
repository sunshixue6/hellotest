#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <dirent.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <ctype.h>
#include <sys/time.h>

/****************************************************
 *  Define
 * **************************************************/
//#define MAIN_PROCESS_NAME          "mainprocess"
#define MAIN_PROCESS_NAME          "service_manager"
#define MAIN_VEHICLE_NAEM          "vfsc_DaemonProc"
#define MAIN_NODEJS_NAME           "node"

#define LOG_FILE_PATH              "./140_1.log"

//#define DEBUG_ON
#define NAME_MAX_SUFFIX            "Name:"
#define PROC_PATH                  "/proc"
#define PPID_SUFFIX                "PPid:"
#define VMRSS_SUFFIX               "VmRSS:"
#define PID_SUFFIX                 "Pid:"
#define MAX_FILE_LINE_STR_BUFSIZE  100
#define MAX_FILENAME_BUFSIZE       50
#define MAX_PPID_LEN               30
#define MAX_VMRSS_LEN              20
#define MAX_NAME_LEN               50
#define MAX_MONITOR_NODES          50
#define MAX_RETRY                  3

/****************************************************
 *  Struct Define
 * **************************************************/
typedef struct _monitor_node {
    int  activeFlag;
    int  pidNum;
    int  vmRSSInfo;
    int  retryCnt;
    int  mainCmdFlag;
    int  useCmdNameFlag;
    int  maxVmRSSInfo;
    char processName[MAX_NAME_LEN];
    char cmdName[MAX_NAME_LEN];
} Monitor_node;

typedef struct _manager_monitor
{
    int totalMemSize;
    //int activeMonitorNum;
    Monitor_node monitorNodes[MAX_MONITOR_NODES];
}Manager_monitor;

typedef struct _cpustat
{
    int user    ;
    int nice    ;
    int system  ;
    int idle    ;
    int iowait  ;
    int irq     ;
    int softirq ;
    unsigned long long totalCnt;
}CPU_Stat;


/****************************************************
 *  Groble Member
 * **************************************************/
Manager_monitor gNodesManger;
Manager_monitor gFristSaveNodesManger;
int   gMainPid   = 0;
int   gVfscPid   = 0;
int   gNodePid   = 0;

/****************************************************
 *  Method declare
 * **************************************************/
int getTotalMemInfo(int *pTotalMem, int *pFreeMem);

/****************************************
 * @func:   通过传入待解析的字符获取数字进程号
 * @param:  *str 待解析字符串指针
 * @return:  >0:正常解析结果pid
 *           -1:非正常解析出的pid
 * **************************************/
int getdigitalPID(char *str)
{
    int len = 0;
    int i   = 0;

    if (str != NULL)
    {
        len = strlen(str);
        for (i = 0; i < len; i++)
        {
            if (!isdigit(str[i]))
            {
                return -1;
            }
        }
        return atoi(str);
    }
    else
    {
        return -1;
    }
}

/****************************************
 * @func:   根据pid将监测需要的进程名，
 *          内存信息等添加到管理gNodesManger结构中
 * @param:  pid 预备添加的pid
 *          pMonitorNode 添加的节点
 *          flag node节点父进程 1
 *               node节点子进程 0
 * @return:  1:成功添加
 *          -1：添加失败
 * **************************************/
int addMonitorCmdLineInfo(int pid, Monitor_node* pMonitorNode, int flag)
{
    FILE *fp                                   = NULL;
    int  ret                                   = -1;
    int  i                                     =  0;
    int  markCnt                               =  0;
    char fileCmdBuf[MAX_FILENAME_BUFSIZE]      = {0};
    char lineStrBuf[MAX_FILE_LINE_STR_BUFSIZE] = {0};
    char ch                                    = ' ';


    memset(fileCmdBuf, 0x00, sizeof(fileCmdBuf));
    sprintf(fileCmdBuf, "/proc/%d/cmdline", pid);

    fp = fopen(fileCmdBuf, "r");

    if (fp == NULL)
    {
        printf("open file %s fail!\n", fileCmdBuf);
        return ret;
    }

    memset(lineStrBuf, 0, MAX_FILE_LINE_STR_BUFSIZE);

    while(!feof(fp))
    {
        ch = fgetc(fp);
        if (ch == '/')
        {
            markCnt++;
        }
        else
        {
            if (flag)
            {
                if (markCnt == 6)
                {
                    lineStrBuf[i++] = ch;
                }
                else if (markCnt == 7)
                {
                    lineStrBuf[i] = '\0';
                    break;
                }
            }
            else
            {
                if (markCnt == 9)
                {
                    lineStrBuf[i++] = ch;
                }
                else if (markCnt == 10)
                {
                    lineStrBuf[i] = '\0';
                    break;
                }
            }
        }
    }

    if (flag)
    {
        if (markCnt == 7)
        {
            strncpy(pMonitorNode->cmdName, lineStrBuf, strlen(lineStrBuf));
            pMonitorNode->useCmdNameFlag = 1;
            pMonitorNode->mainCmdFlag    = 1;
            printf("[Debug]==> %s markCnt = %d, %s CmdName = %s\n", __func__, markCnt, lineStrBuf, pMonitorNode->cmdName);
        }
    }
    else
    {
        if (markCnt == 10)
        {
            strncpy(pMonitorNode->cmdName, lineStrBuf, strlen(lineStrBuf));
            pMonitorNode->useCmdNameFlag = 1;
            printf("[Debug]==> %s markCnt = %d, %s CmdName = %s\n", __func__, markCnt, lineStrBuf, pMonitorNode->cmdName);
        }
    }

    if (fp)
    {
        fclose(fp);
    }

    return  ret;
}

/****************************************
 * @func:   根据pid将监测需要的进程名，
 *          内存信息等添加到管理gNodesManger结构中
 * @param:  pid 预备添加的pid
 * @return:  1:成功添加
 *          -1：添加失败
 * **************************************/
int addMonitorNodeInfo(int pid)
{
    int ret                     = -1;
    Monitor_node* pMonitorNode  = NULL;
    FILE   *fp                  = NULL;
    int    sIdx                 =  0;
    int    eIdx                 =  0;
    int    i                    =  0;
    int    len                  =  0;

    char fileNameBuf[MAX_FILENAME_BUFSIZE]     = {0};
    char lineStrBuf[MAX_FILE_LINE_STR_BUFSIZE] = {0};
    char vmRSSBuf[MAX_VMRSS_LEN]               = {0};
    char tmpNameBuf[MAX_NAME_LEN]              = {0};


    //判断PID参数合法性
    if (pid > 0)
    {
        memset(fileNameBuf, 0x00, sizeof(fileNameBuf));
        sprintf(fileNameBuf, "/proc/%d/status", pid);

        fp = fopen(fileNameBuf, "r");
        if (fp == NULL)
        {
            printf("open file %s fail!\n", fileNameBuf);
            return ret;
        }

        //找到空闲监测节点的指针
        for (i= 0; i < MAX_MONITOR_NODES; i++)
        {
            pMonitorNode = &gNodesManger.monitorNodes[i];
            if (!pMonitorNode->activeFlag)
            {
                break;
            }
        }

        if (MAX_MONITOR_NODES == i)
        {
            printf("[Err] %s:i = %d\n", __func__, i);
            return -1;
        }

        //pMonitorNode = &gNodesManger.monitorNodes[gNodesManger.activeMonitorNum];
        //gNodesManger.activeMonitorNum++;
        //pMonitorNode->activeFlag = 1;
        pMonitorNode->pidNum     = pid;

        while(!feof(fp))
        {
            memset(lineStrBuf, 0, MAX_FILE_LINE_STR_BUFSIZE);
            if (fgets(lineStrBuf, MAX_FILE_LINE_STR_BUFSIZE, fp) != NULL)
            {
                sIdx = 0;
                eIdx = 0;

                //读取name信息
                if ((strncmp(lineStrBuf, NAME_MAX_SUFFIX, 5) == 0))
                {
                    memset(tmpNameBuf, 0x0, MAX_NAME_LEN);
                    len = strlen(lineStrBuf);
                    //len = strlen(lineStrBuf) - 5;
                    for (i = 5; i < len; i++)
                    {
                        if ((lineStrBuf[i] != ' ') && (lineStrBuf[i] != '\t') && (sIdx == 0))
                        {
                            sIdx = i;
                            break;
                        }
                    }

                    if (sIdx > 0)
                    {
                        //eIdx = len + 5;
                        eIdx = len;
                        if ((eIdx > sIdx) && ((eIdx - sIdx + 2) < MAX_NAME_LEN)  )
                        //if ((eIdx > sIdx) && ((eIdx - sIdx + 1) < MAX_NAME_LEN)  )
                        {
                            strncpy(pMonitorNode->processName, &lineStrBuf[sIdx], eIdx - sIdx -1);
                            pMonitorNode->processName[eIdx - sIdx] = '\0';
                            if (strncmp(MAIN_VEHICLE_NAEM, pMonitorNode->processName, strlen(pMonitorNode->processName)) == 0)
                            {
                                //printf("Hit %s pid = %d\n", pMonitorNode->processName, pid);
                                gVfscPid = pid;
                            }
                            else if (strncmp(MAIN_NODEJS_NAME, pMonitorNode->processName, strlen(pMonitorNode->processName)) == 0)
                            {
                                //printf("Hit %s pid = %d\n", pMonitorNode->processName, pid);
                                if (0 == gNodePid)
                                {
                                    gNodePid = pid;
                                    addMonitorCmdLineInfo(pid, pMonitorNode, 1);
                                }
                                else
                                {
                                    addMonitorCmdLineInfo(pid, pMonitorNode, 0);
                                }

                            }
                            else
                            {
                                //MAIN_NODEJS_NAME
                            }
                        }
                        else
                        {
                            printf("%s Error sIdx=%d, eIdx = %d\n", __func__, sIdx, eIdx);
                        }
                    }
                }
                else if (strncmp(lineStrBuf, VMRSS_SUFFIX, 6) == 0)
                {
                    //读取vmRSS信息
                    len = strlen(lineStrBuf);
                    //printf("lineStrBuf = %s", lineStrBuf);

                    for (i = 6; i < len; i++)
                    {
                        if (sIdx == 0)
                        {
                            if (isdigit(lineStrBuf[i]))
                            {
                                sIdx = i;
                            }
                        }
                        else
                        {
                            if (!isdigit(lineStrBuf[i]))
                            {
                                eIdx = i - 1;
                                break;
                            }
                        }
                    }
                    //printf("sIdx = %d eIdx =%d\n", sIdx, eIdx);

                    if ((sIdx != 0) && (eIdx > sIdx))
                    {
                        memset(vmRSSBuf, 0, MAX_VMRSS_LEN);
                        strncpy(vmRSSBuf, &lineStrBuf[sIdx],  eIdx-sIdx+1);

                        pMonitorNode->vmRSSInfo = atoi(vmRSSBuf);
                        //gNodesManger.totalMemSize += pMonitorNode->vmRSSInfo;
                        if (pMonitorNode->vmRSSInfo > pMonitorNode->maxVmRSSInfo)
                        {
                            pMonitorNode->maxVmRSSInfo = pMonitorNode->vmRSSInfo;
                        }
                        pMonitorNode->activeFlag = 1;
                        ret = 1;
                        break;
                    }
                    else
                    {
                        printf("[Err] %s sIdx = %d eIdx =%d\n", __func__, sIdx, eIdx);
                    }
                }
                else
                {

                }
            }
        }
        if (fp)
        {
            fclose(fp);
        }
    }
#ifdef DEBUG_ON
    printf("[Debug]MonitorNode Name:%s\tPid:%d\tVmRSS:%d\n", pMonitorNode->processName, pMonitorNode->pidNum, pMonitorNode->vmRSSInfo);
#endif

    if (ret != 1)
    {
        pMonitorNode->activeFlag = 0;
    }
    return ret;
}

/****************************************
 * @func:   根据传入的父进程号找出由此父进程启动的子pid、
 *          并将子进程相关信息加入到gNodesManger数组当中
 * @param:  父进程pid
 * @return:   1:处理正常
 *           -1:处理中出现异常错误
 * **************************************/
int scan_all_childID( int fpid)
{
    FILE   *fp             = NULL;
    DIR    *pDirptr        = NULL;
    struct dirent *pEntry  = NULL;
    int    pidNum          = -1;
    int    tmpPPid         = 0;
    int    copylen         = 0;
    char   bAttachFlag     = 0;
    int    totalvmRSSMem   = 0;
    int    sIdx            = 0;
    int    eIdx            = 0;
    int    i               = 0;
    int    len             = 0;

    char ppidBuf[MAX_PPID_LEN]                 = {0};
    char fileNameBuf[MAX_FILENAME_BUFSIZE]     = {0};
    char lineStrBuf[MAX_FILE_LINE_STR_BUFSIZE] = {0};
    char vmRSSBuf[MAX_VMRSS_LEN]               = {0};

    //1. 检索 /proc目录
    if ((pDirptr = opendir(PROC_PATH)) == NULL)
    {
        printf("[Fail] open dir %s fail!\n", PROC_PATH);
        return -1;
    }
    else
    {
        while ((pEntry = readdir(pDirptr)) != NULL)
        {
            if ((strncmp(pEntry->d_name, ".", 1) == 0)
            || (strncmp(pEntry->d_name, "..", 2) == 0)
            || (pEntry->d_type != DT_DIR))
            {
                continue;
            }
            else
            {
                //get the dir folder
                //printf("pase Name =%s\n", pEntry->d_name);

                //2. 识别处是数字pid组成的文件夹
                pidNum = getdigitalPID(pEntry->d_name);
                if (pidNum != -1)
                {
                    //printf("get pid folder number = %d fileNameBuf= %d\n", pidNum, sizeof(fileNameBuf));
                    memset(fileNameBuf, 0x00, sizeof(fileNameBuf));

                    //3. 取出子pid 相关的内存信息
                    snprintf(fileNameBuf, 14+strlen(pEntry->d_name), "/proc/%s/status", pEntry->d_name);
                    //printf("fileNameBuf = %s\n", fileNameBuf);

                    //3.1 解析/proc/子pid/status 文件
                    fp = fopen(fileNameBuf, "r");
                    if (fp == NULL)
                    {
                        printf("open file %s fail!\n", fileNameBuf);
                        return -1;
                    }

                    bAttachFlag = 0;
                    while(!feof(fp))
                    {
                        memset(lineStrBuf, 0, MAX_FILE_LINE_STR_BUFSIZE);
                        if (fgets(lineStrBuf, MAX_FILE_LINE_STR_BUFSIZE, fp) != NULL)
                        {
                            //3.2 比较 PPid 与 fpid 是否一致
                            if ((strncmp(lineStrBuf, PPID_SUFFIX, 5) == 0))
                            {
                                copylen = strlen(lineStrBuf)-5;
                                if (copylen < MAX_PPID_LEN)
                                {
                                    strncpy(ppidBuf, &lineStrBuf[5], copylen);
                                    tmpPPid = atoi(ppidBuf);
                                    if (tmpPPid == fpid)
                                    {
                                        bAttachFlag = 1;
                                        break;
                                    }
                                }
                            }
                        }
                    }
                    if (fp != NULL)
                    {
                        fclose(fp);
                    }

                    if (bAttachFlag)
                    {
                        if (pEntry->d_name != NULL)
                        {
                            if (addMonitorNodeInfo(atoi(pEntry->d_name)) != 1)
                            {
                                printf("[Err] %s: add pid %s\n", __func__, pEntry->d_name);
                            }
                        }
                    }
                }
                else
                {
                    continue;
                }
            }
        }
        closedir(pDirptr);
    }

    return 1;
}

/****************************************
 * @func:   根据指定的进程名检索/proc 目录取回对应的pid
 *          目标取回父进程pid即名字相同最小的一个
 * @param:  procname 查找的进程名
 * @return: 返回找到的pid
 * **************************************/
int getProcessIdbyName(char *procname)
{
    FILE   *fp             = NULL;
    DIR    *pDirptr        = NULL;
    struct dirent *pEntry  = NULL;
    int    pidNum          = -1;
    int    mainPid         = -1;
    int    ret             = -1;
    int    i               =  0;
    int    len             =  0;
    int    sIdx            =  0;
    int    eIdx            =  0;
    int    attachFlag      =  0;
    int    tmpPid          =  0;
    int    copylen         =  0;
    char   fileNameBuf[MAX_FILENAME_BUFSIZE]     = {0};
    char   lineStrBuf[MAX_FILE_LINE_STR_BUFSIZE] = {0};
    char   tmpNameBuf[MAX_NAME_LEN]              = {0};
    char   pidBuf[MAX_PPID_LEN]                  = {0};

    //1. 检索 /proc目录
    if ((pDirptr = opendir(PROC_PATH)) == NULL)
    {
        printf("[Fail] open dir %s fail!\n", PROC_PATH);
        return -1;
    }
    else
    {
        while ((pEntry = readdir(pDirptr)) != NULL)
        {
            if ((strncmp(pEntry->d_name, ".", 1) == 0)
            || (strncmp(pEntry->d_name, "..", 2) == 0)
            || (pEntry->d_type != DT_DIR))
            {
                continue;
            }

            //2. 识别处是数字pid组成的文件夹
            pidNum = getdigitalPID(pEntry->d_name);
            if (pidNum != -1)
            {
                //printf("get pid folder number = %d fileNameBuf= %d\n", pidNum, sizeof(fileNameBuf));
                memset(fileNameBuf, 0x0, sizeof(fileNameBuf));

                //3. 取出子pid 相关的内存信息
                snprintf(fileNameBuf, 14+strlen(pEntry->d_name), "/proc/%s/status", pEntry->d_name);

                //3.1 解析/proc/子pid/status 文件
                fp = fopen(fileNameBuf, "r");
                if (fp == NULL)
                {
                    printf("open file %s fail!\n", fileNameBuf);
                    return -1;
                }

                sIdx       = 0;
                eIdx       = 0;
                attachFlag = 0;
                while (!feof(fp))
                {
                    memset(lineStrBuf, 0x0, MAX_FILE_LINE_STR_BUFSIZE);
                    if (fgets(lineStrBuf, MAX_FILE_LINE_STR_BUFSIZE, fp) != NULL)
                    {
                        //3.2 取出name属性并与监控主程序对象procname进行对比
                        if ((strncmp(lineStrBuf, NAME_MAX_SUFFIX, 5) == 0))
                        {
                            
                            memset(tmpNameBuf, 0x0, MAX_NAME_LEN);
                            len = strlen(lineStrBuf);
                            for (i = 5; i < len; i++)
                            {
                                if ((lineStrBuf[i] != ' ') && (lineStrBuf[i] != '\t') && (sIdx == 0))
                                {
                                    sIdx = i;
                                    break;
                                }
                            }
                            //printf("fileNameBuf = %s lineStrBuf = %s sIdx = %d len = %d\n", fileNameBuf, lineStrBuf, sIdx, len);
                            if (sIdx > 0)
                            {
                                eIdx = len;
                                if ((eIdx > sIdx) && ((eIdx - sIdx + 2) < MAX_NAME_LEN)  )
                                {
                                    strncpy(tmpNameBuf, &lineStrBuf[sIdx], eIdx - sIdx + 1);
                                    tmpNameBuf[eIdx - sIdx + 2] = '\0';
                                    if (strncmp(tmpNameBuf, procname, strlen(procname)) == 0)
                                    {
                                        printf("Hint it %s\n", tmpNameBuf);
                                        attachFlag = 1;
                                        continue;
                                    }
                                }
                                else
                                {
                                    printf("%s Error lineStrBuf= %ssIdx=%d, eIdx = %d\n", __func__, lineStrBuf, sIdx, eIdx);
                                }
                            }
                        }

                        //取出pid记录，取出最小的一个
                        if (attachFlag)
                        {
                            if ((strncmp(lineStrBuf, PID_SUFFIX, 4) == 0))
                            {
                                copylen = strlen(lineStrBuf) - 4;
                                if (copylen < MAX_PPID_LEN)
                                {
                                    memset(pidBuf, 0x0, MAX_PPID_LEN);
                                    strncpy(pidBuf, &lineStrBuf[4], copylen);
                                    tmpPid = atoi(pidBuf);
                                    if ((mainPid == -1) || (tmpPid < mainPid))
                                    {
                                        mainPid = tmpPid;
                                        break;
                                    }
                                }
                            }
                        }
                        else
                        {
                            break;
                        }
                    }
                }
                fclose(fp);
            }
        }
    }
    closedir(pDirptr);
    return mainPid;
}

/****************************************
 * @func:   根据指定进程的Cmd名检索/proc/pid/cmdline目录
 *          目标取回cmd名字一样的进程id
 * @param:  cmdname 查找的进程名
 *          Moni
 * @return: 返回找到的pid
 * **************************************/
int updateInfobyCmdName(char* cmdname, Monitor_node* pMonitorNode)
{
    FILE   *fp             = NULL;
    DIR    *pDirptr        = NULL;
    struct dirent *pEntry  = NULL;
    int    pidNum          = -1;
    int    ret             = -1;
    int    i               =  0;
    int    markCnt         =  0;
    char   ch              = ' ';
    char   fileNameBuf[MAX_FILENAME_BUFSIZE]     = {0};
    char   lineStrBuf[MAX_FILE_LINE_STR_BUFSIZE] = {0};
    char   tmpNameBuf[MAX_NAME_LEN]              = {0};
    char   pidBuf[MAX_PPID_LEN]                  = {0};

    //1. 检索 /proc目录
    if ((pDirptr = opendir(PROC_PATH)) == NULL)
    {
        printf("[Fail] open dir %s fail!\n", PROC_PATH);
        return -1;
    }
    else
    {
        while ((pEntry = readdir(pDirptr)) != NULL)
        {
            if ((strncmp(pEntry->d_name, ".", 1) == 0)
            || (strncmp(pEntry->d_name, "..", 2) == 0)
            || (pEntry->d_type != DT_DIR))
            {
                continue;
            }

            //2. 识别处是数字pid组成的文件夹
            pidNum = getdigitalPID(pEntry->d_name);
            if (pidNum != -1)
            {
                memset(fileNameBuf, 0x0, sizeof(fileNameBuf));

                //3. 取出子pid 相关的内存信息
                snprintf(fileNameBuf, 15+strlen(pEntry->d_name), "/proc/%s/cmdline", pEntry->d_name);
                //printf("fileNameBuf = %s\n", fileNameBuf);

                //3.1 解析/proc/子pid/status 文件
                fp = fopen(fileNameBuf, "r");
                if (fp == NULL)
                {
                    printf("open file %s fail!\n", fileNameBuf);
                    return -1;
                }

                memset(lineStrBuf, 0, MAX_FILE_LINE_STR_BUFSIZE);
                markCnt = 0;
                i = 0;
                while(!feof(fp))
                {
                    ch = fgetc(fp);
                    if (ch == '/')
                    {
                        markCnt++;
                    }
                    else
                    {
                        if (pMonitorNode->mainCmdFlag)
                        {
                            if ((markCnt == 6) && (i < (MAX_FILE_LINE_STR_BUFSIZE -1)))
                            {
                                lineStrBuf[i++] = ch;
                            }
                            else if (markCnt == 7)
                            {
                                lineStrBuf[i] = '\0';
                                break;
                            }
                            else
                            {
                                continue;
                            }
                        }
                        else
                        {
                            if ((markCnt == 9) && (i < (MAX_FILE_LINE_STR_BUFSIZE -1)))
                            {
                                lineStrBuf[i++] = ch;
                            }
                            else if (markCnt == 10)
                            {
                                lineStrBuf[i] = '\0';
                                break;
                            }
                            else
                            {
                                continue;
                            }
                        }
                    }
                }

                if (pMonitorNode->mainCmdFlag)
                {
                    if (markCnt == 7)
                    {
                        //if (strcmp(cmdname, lineStrBuf) == 0)
                        //printf("[Debug] strlen(cmdname)=%d lineStrBuf = %s\n", strlen(cmdname), lineStrBuf);
                        if (strncmp(cmdname, lineStrBuf, strlen(cmdname)) == 0)
                        {
                            //update pid and VmRSS Info
                            pMonitorNode->pidNum = pidNum;
                            printf("[Debug]==> %s CmdName:%s updatePid = %d \n", __func__, pMonitorNode->cmdName, pidNum);
                            fclose(fp);
                            return pidNum;
                        }
                    }
                }
                else
                {
                    if (markCnt == 10)
                    {
                        //if (strcmp(cmdname, lineStrBuf) == 0)
                        if (strncmp(cmdname, lineStrBuf, strlen(cmdname)) == 0)
                        {
                            //update pid and VmRSS Info
                            pMonitorNode->pidNum = pidNum;
                            printf("[Debug]==> %s CmdName:%s updatePid = %d \n", __func__, pMonitorNode->cmdName, pidNum);
                            fclose(fp);
                            return pidNum;
                        }
                    }
                }

                if (fp)
                {
                    fclose(fp);
                }
            }
        }
    }

    return -1;
}


/****************************************
 * @func:   展示监控Node的信息
 * @param:  无
 * @return: 无
 * **************************************/
void dumpMonitorNodeInfo()
{
    int i = 0;
    Monitor_node* pMonitorNode  = NULL;
    FILE *fpLog;

    int  totalSysMemSize        = 0;
    int  totalSysMemFreeSize    = 0;

    fpLog = fopen(LOG_FILE_PATH,"a");

    gNodesManger.totalMemSize = 0;
    //for (i = 0; i < gNodesManger.activeMonitorNum; i++)
    for (i = 0; i < MAX_MONITOR_NODES; i++)
    {

        pMonitorNode               = &gNodesManger.monitorNodes[i];
        if (pMonitorNode->activeFlag)
        {
            gNodesManger.totalMemSize += pMonitorNode->vmRSSInfo;
            if(pMonitorNode->useCmdNameFlag)
            {
                printf ("Name:%-20sPid:%-5d\tVmRss:%8.1f MB\n", pMonitorNode->cmdName, pMonitorNode->pidNum, (pMonitorNode->vmRSSInfo/1024.0));
            }
            else
            {
                printf ("Name:%-20sPid:%-5d\tVmRss:%8.1f MB\n", pMonitorNode->processName, pMonitorNode->pidNum, (pMonitorNode->vmRSSInfo/1024.0));
            }
            if (fpLog)
            {
                if(pMonitorNode->useCmdNameFlag)
                {
                    fprintf(fpLog, "Name:%-20sPid:%-5d\tVmRss:%8.1f MB\n", pMonitorNode->cmdName, pMonitorNode->pidNum, (pMonitorNode->vmRSSInfo/1024.0));
                }
                else
                {
                    fprintf(fpLog, "Name:%-20sPid:%-5d\tVmRss:%8.1f MB\n", pMonitorNode->processName, pMonitorNode->pidNum, (pMonitorNode->vmRSSInfo/1024.0));
                }
            }
        }
    }

    printf("Total Mem = %0.1f MB\n", (gNodesManger.totalMemSize/1024.0));
    if (fpLog)
    {
        fprintf(fpLog, "Total Mem = %0.1f MB\n", (gNodesManger.totalMemSize/1024.0));
        //fclose(fpLog);
    }

    getTotalMemInfo(&totalSysMemSize, &totalSysMemFreeSize);
    printf("Total System MemSize = %0.1f MB,  System FreeSize: %0.1f MB\n", (totalSysMemSize/1024.0), (totalSysMemFreeSize/1024.0));
    if (fpLog)
    {
        fprintf(fpLog, "Total System MemSize = %0.1f MB,  System FreeSize: %0.1f MB\n", (totalSysMemSize/1024.0), (totalSysMemFreeSize/1024.0));
        fclose(fpLog);
    }
}

/****************************************
 * @func:   更新监控Node的信息
 * @param:  无
 * @return: 无
 * **************************************/
int updateMonitorInfo()
{
    int i        = 0;
    int sIdx     = 0;
    int eIdx     = 0;
    int idx      = 0;
    int len      = 0;
    int tmpPid   = 0;
    int cnt      = 0;
    static int   s_monitorCnt   = 0;
    FILE         *fp            = NULL;
    Monitor_node *pMonitorNode  = NULL;

    char fileNameBuf[MAX_FILENAME_BUFSIZE]     = {0};
    char lineStrBuf[MAX_FILE_LINE_STR_BUFSIZE] = {0};
    char vmRSSBuf[MAX_VMRSS_LEN]               = {0};


    //遍历 gNodesManger的所有Node节点更新VmRSS情报
    //for (i = 0; i < gNodesManger.activeMonitorNum; i++)
    for (i = 0; i < MAX_MONITOR_NODES; i++)
    {
        pMonitorNode = &gNodesManger.monitorNodes[i];

        if (!pMonitorNode->activeFlag)
        {
            //printf("%s. gNodesManger.activeMonitorNum = %d\n", __func__, gNodesManger.activeMonitorNum);
            //gNodesManger.activeMonitorNum--;
            continue;
        }
        else
        {
            cnt++; 
        }

        memset(fileNameBuf, 0x00, sizeof(fileNameBuf));
        sprintf(fileNameBuf, "/proc/%d/status", pMonitorNode->pidNum);

        fp = fopen(fileNameBuf, "r");
        if (fp == NULL)
        {
            printf("%s open file %s fail! process = %s\n", __func__, fileNameBuf, pMonitorNode->processName);
            //判断是否是主Pid
            if (pMonitorNode->pidNum == gMainPid)
            {
                //更新所有节点
                memset(&gNodesManger, 0x0, sizeof(gNodesManger));
                //gNodesManger.activeMonitorNum = 0;

                //根据进程名获得父进程号
                gMainPid = getProcessIdbyName(MAIN_PROCESS_NAME);

                //printf("[Debug] monitor main pid =%d\n", gMainPid);
                if (gMainPid > 0)
                {
                    addMonitorNodeInfo(gMainPid);
                    scan_all_childID(gMainPid);
                }
                return 0;
            }
            else
            {
                //更新单个节点
                if (pMonitorNode->activeFlag )
                {
                    if (pMonitorNode->useCmdNameFlag)
                    {
                        tmpPid = updateInfobyCmdName(pMonitorNode->cmdName, pMonitorNode);
                        printf("[Debug] %s change pid %d to %d\n", pMonitorNode->processName,pMonitorNode->pidNum, tmpPid);
                        if (tmpPid > 0)
                        {
                            memset(fileNameBuf, 0x00, sizeof(fileNameBuf));
                            sprintf(fileNameBuf, "/proc/%d/status", tmpPid);
                            fp = fopen(fileNameBuf, "r");
                            if (fp == NULL)
                            {
                                printf("%s Retry open file %s fail!\n", __func__, fileNameBuf);
                                continue;
                            }
                            printf("[Debug] change pid %d to %d\n", pMonitorNode->pidNum, tmpPid);
                            pMonitorNode->pidNum = tmpPid;
                        }
                        else
                        {
                            if (pMonitorNode->retryCnt < MAX_RETRY)
                            {
                                pMonitorNode->retryCnt++;
                            }
                            else
                            {
                                //pMonitorNode->activeFlag = 0;
                                //pMonitorNode->retryCnt   = 0;
                                //pMonitorNode->pidNum     = 0;
                                //pMonitorNode->vmRSSInfo  = 0;
                                printf("[Info] %s process restart faild at 3 times\n", pMonitorNode->processName);
                                memset(pMonitorNode, 0x0, sizeof(Monitor_node));
                                //gNodesManger.activeMonitorNum--;
                            }
                            continue;
                        }
                    }
                    else
                    {
                        //根据名字查找进程对应的pid
                        tmpPid= getProcessIdbyName(pMonitorNode->processName);
                        if (tmpPid > 0)
                        {
                            //pMonitorNode->activeFlag = 0;
                            //gNodesManger.activeMonitorNum--;
                            //memset(pMonitorNode, 0x0, sizeof(Monitor_node));
                            //addMonitorNodeInfo(tmpPid);

                            memset(fileNameBuf, 0x00, sizeof(fileNameBuf));
                            sprintf(fileNameBuf, "/proc/%d/status", tmpPid);
                            fp = fopen(fileNameBuf, "r");
                            if (fp == NULL)
                            {
                                printf("%s Retry open file %s fail!\n", __func__, fileNameBuf);
                                continue;
                            }

                            printf("[Debug] change pid %d to %d\n", pMonitorNode->pidNum, tmpPid);
                            pMonitorNode->pidNum = tmpPid;
                        }
                        else
                        {
                            if (pMonitorNode->retryCnt < MAX_RETRY)
                            {
                                pMonitorNode->retryCnt++;
                            }
                            else
                            {
                                //pMonitorNode->activeFlag = 0;
                                //pMonitorNode->retryCnt   = 0;
                                //pMonitorNode->pidNum     = 0;
                                //pMonitorNode->vmRSSInfo  = 0;
                                printf("[Info] %s process restart faild at 3 times\n", pMonitorNode->processName);
                                memset(pMonitorNode, 0x0, sizeof(Monitor_node));
                                //gNodesManger.activeMonitorNum--;
                            }
                            continue;
                        }
                    }
                }
                
            }
            continue;
        }

        while(!feof(fp))
        {
            memset(lineStrBuf, 0, MAX_FILE_LINE_STR_BUFSIZE);
            if (fgets(lineStrBuf, MAX_FILE_LINE_STR_BUFSIZE, fp) != NULL)
            {
                if (strncmp(lineStrBuf, VMRSS_SUFFIX, 6) == 0)
                {
                    idx  = 0;
                    sIdx = 0;
                    eIdx = 0;

                    //读取vmRSS信息
                    len = strlen(lineStrBuf);
                    //printf("%s, lineStrBuf = %s", __func__, lineStrBuf);

                    for (idx = 6; idx < len; idx++)
                    {
                        if (sIdx == 0)
                        {
                            if (isdigit(lineStrBuf[idx]))
                            {
                                sIdx = idx;
                            }
                        }
                        else
                        {
                            if (!isdigit(lineStrBuf[idx]))
                            {
                                eIdx = idx - 1;
                                break;
                            }
                        }
                    }

                    if ((sIdx != 0) && (eIdx > sIdx))
                    {
                        memset(vmRSSBuf, 0, MAX_VMRSS_LEN);
                        strncpy(vmRSSBuf, &lineStrBuf[sIdx],  eIdx-sIdx+1);
                        pMonitorNode->vmRSSInfo = atoi(vmRSSBuf);
                        if (pMonitorNode->maxVmRSSInfo < (pMonitorNode->vmRSSInfo))
                        {
                            pMonitorNode->maxVmRSSInfo = pMonitorNode->vmRSSInfo;
                        }
                    }
                    break;
                }
            }
        }
        //printf ("%s: i = %d\n pMonitorNode->vmRSSInfo = %d\n", __func__, i, pMonitorNode->vmRSSInfo);
        if (fp != NULL)
        {
            fclose(fp);
        }
    }

    if (cnt != s_monitorCnt)
    {
        printf("Monitor Cnt has been changed s_monitorCnt = %d, cnt = %d\n", s_monitorCnt, cnt);
    }

    return 0;
}

/****************************************
 * @func:   遍历gNodesManger的节点，按照降序排列
 *
 * @param:  无
 * @return:   1:处理正常
 *           -1:处理中出现异常错误
 * **************************************/
int sortMonitorNodebyMemSize()
{
    int ret      = -1;
    int i        = 0;
    int j        = 0;
    int sortFlag = 0;
    Monitor_node tmpMonitorNode;

    //for (i = 0; (i < (gNodesManger.activeMonitorNum - 1)) && (i < (MAX_MONITOR_NODES - 1)); i++)
    for (i = 0; (i < (MAX_MONITOR_NODES - 1)); i++)
    {
        if (gNodesManger.monitorNodes[i].activeFlag)
        {
            if (gNodesManger.monitorNodes[i].vmRSSInfo < gNodesManger.monitorNodes[i+1].vmRSSInfo)
            {
                sortFlag = 1;
                break;
            }
        }
    }
    //printf("[Debug]: sortFlag = %d\n",  sortFlag);

    if (sortFlag)
    {
        //for (i = 0; (i < gNodesManger.activeMonitorNum) && (i < (MAX_MONITOR_NODES - 1)); i++)
        for (i = 0; (i < (MAX_MONITOR_NODES - 1)); i++)
        {
            for (j = i+1; (j < (MAX_MONITOR_NODES - 1)); j++)
            {
                if (gNodesManger.monitorNodes[j].activeFlag)
                {
                    if (gNodesManger.monitorNodes[i].vmRSSInfo < gNodesManger.monitorNodes[j].vmRSSInfo)
                    {
                        tmpMonitorNode               = gNodesManger.monitorNodes[i];
                        gNodesManger.monitorNodes[i] = gNodesManger.monitorNodes[j];
                        gNodesManger.monitorNodes[j] = tmpMonitorNode;
                    }
                }
            }
        }
    }

    ret = 1;
    return ret;
}

/****************************************
 * @func:   存储第一次检索信息
 *
 * @param:  无
 * @return:   1:处理正常
 *           -1:处理中出现异常错误
 * **************************************/
void saveFristTimeMonitorInfo()
{
    memset(&gFristSaveNodesManger, 0x0, sizeof(gFristSaveNodesManger));
    memcpy(&gFristSaveNodesManger, &gNodesManger, sizeof(gFristSaveNodesManger));
}

/****************************************
 * @func:   检索/proc/meminfo 信息
 *          取回 Total , Free,  Buffers, Caches信息
 *
 * @param:  无
 * @return:   >0 : 当前系统total FreeMemory的值
 *           -1:处理中出现异常错误
 * **************************************/
int getTotalMemInfo(int *pTotalMem, int *pFreeMem)
{
    FILE   *fp             = NULL;
    int    i               = 0;
    int    sIdx            = 0;
    int    eIdx            = 0;
    char   lineStrBuf[MAX_FILE_LINE_STR_BUFSIZE] = {0};
    char   tmpBuf[MAX_NAME_LEN]                  = {0};


    fp = fopen("/proc/meminfo", "r");

    while(!feof(fp))
    {
        memset(lineStrBuf, 0, MAX_FILE_LINE_STR_BUFSIZE);
        memset(tmpBuf,     0, MAX_NAME_LEN);
        i    = 0;
        sIdx = 0;
        eIdx = 0;
        if (fgets(lineStrBuf, MAX_FILE_LINE_STR_BUFSIZE, fp) != NULL)
        {
            if (strncmp(lineStrBuf, "MemTotal:", 9) == 0)
            {
                for (i = 9; i < strlen(lineStrBuf); i++)
                {
                    if ((lineStrBuf[i] == ' ') && (sIdx == 0))
                    {
                        continue;
                    }
                    else
                    {
                        if (isdigit(lineStrBuf[i]))
                        {
                            if (sIdx == 0)
                            {
                                sIdx = i;
                                eIdx++;
                            }
                            else
                            {
                                eIdx++;
                            }
                        }
                        else
                        {
                            if (eIdx > 0)
                            {
                                break;
                            }
                        }
                    }
                }

                if (eIdx > 0)
                {
                    if (eIdx < (MAX_NAME_LEN - 1))
                    {
                        strncpy(tmpBuf, &lineStrBuf[sIdx], eIdx);
                        tmpBuf[eIdx] = '\0';
                        if (pTotalMem != NULL)
                        {
                            *pTotalMem = atoi(tmpBuf);
                        }
                    }
                    else
                    {
                        printf("[Err] %s tmpBuf is overflow eIdx = %d\n", __func__, eIdx);
                    }
                }
            }
            else if (strncmp(lineStrBuf, "MemFree:", 8) == 0)
            {
                for (i = 8; i < strlen(lineStrBuf); i++)
                {
                    if ((lineStrBuf[i] == ' ') && (sIdx == 0))
                    {
                        continue;
                    }
                    else
                    {
                        if (isdigit(lineStrBuf[i]))
                        {
                            if (sIdx == 0)
                            {
                                sIdx = i;
                                eIdx++;
                            }
                            else
                            {
                                eIdx++;
                            }
                        }
                        else
                        {
                            if (eIdx > 0)
                            {
                                break;
                            }
                        }
                    }
                }

                if (eIdx > 0)
                {
                    if (eIdx < (MAX_NAME_LEN - 1))
                    {
                        strncpy(tmpBuf, &lineStrBuf[sIdx], eIdx);
                        tmpBuf[eIdx] = '\0';
                        if (pFreeMem != NULL)
                        {
                            *pFreeMem = atoi(tmpBuf);
                        }
                    }
                    else
                    {
                        printf("[Err] %s tmpBuf is overflow eIdx = %d\n", __func__, eIdx);
                    }
                }
                break;
            }
            else
            {

            }
        }
    }

    if (fp)
    {
        fclose(fp);
    }
    return 0;
}

/****************************************
 * @func:   解析/proc/stat文件
 *
 * @param:  无
 * @return:   1:处理正常
 *           -1:处理中出现异常错误
 * **************************************/
int getCPUInfo(CPU_Stat* pCpuStat)
{
    char   lineStrBuf[MAX_FILE_LINE_STR_BUFSIZE] = {0};
    char   tmpBuf[MAX_NAME_LEN]                  = {0};
    char   delim[2]                              = " ";
    char   *strv                                 = lineStrBuf;
    FILE   *fp                                   = NULL;
    char   *token                                = NULL;
    int    cnt                                   = 0;

    if (pCpuStat == NULL)
    {
        return -1;
    }

    fp = fopen("/proc/stat", "r");

    while(!feof(fp))
    {
        memset(lineStrBuf, 0, MAX_FILE_LINE_STR_BUFSIZE);
        cnt = 0;
        if (fgets(lineStrBuf, MAX_FILE_LINE_STR_BUFSIZE, fp) != NULL)
        {
            token = strsep(&strv, delim);
            while (token != NULL)
            {
                if (*token == '\0')
                {
                    //printf("token is null string\n");
                    token = strsep(&strv, delim);
                    continue;

                }
                else if (*token == ' ')
                {
                    printf("token is space\n");
                    token = strsep(&strv, delim);
                    continue;
                }
                else
                {

                    switch (cnt)
                    {
                    case 1:
                        pCpuStat->user = atoi(token);
                        break;
                    case 2:
                        pCpuStat->nice = atoi(token);
                        break;
                    case 3:
                        pCpuStat->system = atoi(token);
                        break;
                    case 4:
                        pCpuStat->idle = atoi(token);
                        break;
                    case 5:
                        pCpuStat->iowait = atoi(token);
                        break;
                    case 6:
                        pCpuStat->irq = atoi(token);
                        break;
                    case 7:
                        pCpuStat->softirq = atoi(token);
                        break;
                    default:
                        break;
                    }
                    if (cnt >= 7)
                    {
                        break;
                    }

                    token = strsep(&strv, delim);
                    cnt++;
                }
            }

            pCpuStat->totalCnt = pCpuStat->user + pCpuStat->nice + pCpuStat->system
                    + pCpuStat->idle + pCpuStat->iowait + pCpuStat->irq + pCpuStat->softirq;
            //printf("pCpuStat->user =%d, pCpuStat->nice =%d, pCpuStat->system =%d, pCpuStat->idle = %d, pCpuStat->iowait=%d, pCpuStat->totalCnt=%lld\n", pCpuStat->user,  pCpuStat->nice, pCpuStat->system, pCpuStat->idle, pCpuStat->iowait, pCpuStat->totalCnt);
            break;
        }
    }

    if (fp)
    {
        fclose(fp);
    }
    return 1;
}

/****************************************
 * @func:   解析/proc/stat文件计算CPU使用率
 *
 * @param:  无
 * @return:   1:处理正常
 *           -1:处理中出现异常错误
 * **************************************/
int dumpCPUInfo()
{
    CPU_Stat cpuStat_1;
    CPU_Stat cpuStat_2;
    int ret         = -1;
    FILE *fpLog     = NULL;
    double cpuValue = 0.0;

    ret = getCPUInfo(&cpuStat_1);
    if (ret == -1)
    {
        printf("[Err] %s %d is fail\n", __func__, __LINE__);
        return ret;
    }

    sleep(3);

    ret = getCPUInfo(&cpuStat_2);
    if (ret == -1)
    {
        printf("[Err] %s %d is fail\n", __func__, __LINE__);
        return ret;
    }

    cpuValue = 100.0 - (cpuStat_2.idle - cpuStat_1.idle) * 100.0 / (cpuStat_2.totalCnt - cpuStat_1.totalCnt);
    printf("cpu: %0.1f%%\n", cpuValue);

    fpLog = fopen(LOG_FILE_PATH,"a");
    if (fpLog != NULL)
    {
        fprintf(fpLog, "cpu: %0.1f%%\n", cpuValue);
        if (fpLog)
        {
            fclose(fpLog);
        }
        ret = 1;
    }
    else
    {
        return -1;
    }
    return ret;
}

/****************************************
 * @func:   存储第一次检索信息
 *
 * @param:  无
 * @return:   1:处理正常
 *           -1:处理中出现异常错误
 * **************************************/
void compareMonitorInfo()
{
    Monitor_node* pMonitorNode  = NULL;
    FILE *fpLog                 = NULL;
    int  i = 0;
    int  j = 0;
    int  attachFlag = 0;

    printf("Total Mem increase = %0.1f MB\n", ((gNodesManger.totalMemSize - gFristSaveNodesManger.totalMemSize)/1024.0));

    fpLog = fopen(LOG_FILE_PATH,"a");
    fprintf(fpLog, "Total Mem increase = %0.1f MB\n", ((gNodesManger.totalMemSize - gFristSaveNodesManger.totalMemSize)/1024.0));


    //for (i = 0; i < gNodesManger.activeMonitorNum; i++)
    for (i = 0; i < MAX_MONITOR_NODES; i++)
    {
        //通过遍历gNodesManger节点
        pMonitorNode = &gNodesManger.monitorNodes[i];
        attachFlag   = 0;

        if (pMonitorNode->activeFlag)
        {
            //遍历gFristSaveNodesManger 查看pid相同的节点
            for (j = 0; j < MAX_MONITOR_NODES; j++)
            {
                if (gFristSaveNodesManger.monitorNodes[j].pidNum == pMonitorNode->pidNum)
                {
                    attachFlag = 1;
                    printf ("Name:%-25sPid:%-5d\tfirst:VmRSS:%6.1f MB  current:VmRSS:%8.1f MB  VmRssIncrease:%8.1f MB MaxVmRSS:%8.1f MB\n",
                        pMonitorNode->processName, pMonitorNode->pidNum,
                        (gFristSaveNodesManger.monitorNodes[j].vmRSSInfo/1024.0), (pMonitorNode->vmRSSInfo/1024.0),
                        ((pMonitorNode->vmRSSInfo - gFristSaveNodesManger.monitorNodes[j].vmRSSInfo)/1024.0), (pMonitorNode->maxVmRSSInfo/1024.0));

                    fprintf (fpLog, "Name:%-25sPid:%-5d\tfirst:VmRSS:%6.1f MB  current:VmRSS:%8.1f MB  VmRssIncrease:%8.1f MB MaxVmRSS:%8.1f MB\n",
                        pMonitorNode->processName, pMonitorNode->pidNum,
                        (gFristSaveNodesManger.monitorNodes[j].vmRSSInfo/1024.0), (pMonitorNode->vmRSSInfo/1024.0),
                        ((pMonitorNode->vmRSSInfo - gFristSaveNodesManger.monitorNodes[j].vmRSSInfo)/1024.0), (pMonitorNode->maxVmRSSInfo/1024.0));
                    break;
                }
            }

            if (!attachFlag)
            {
                //printf("[Debug] attachFlag = %d\n", attachFlag);

                //监控的pid有变化，通过名字找到对应的监控节点
                for (j = 0; j < MAX_MONITOR_NODES; j++)
                {
                    if (gFristSaveNodesManger.monitorNodes[j].activeFlag)
                    {
                        if (strncmp(pMonitorNode->processName, gFristSaveNodesManger.monitorNodes[j].processName, strlen(pMonitorNode->processName)) == 0)
                        {
                            //通过名字找到节点时
                            //更新gFristSaveNodesManger 的节点信息的pid
                            gFristSaveNodesManger.monitorNodes[j].pidNum = pMonitorNode->pidNum;
                            printf("[Warning] Name: %s change the pid %d\n", pMonitorNode->processName, pMonitorNode->pidNum);
                            break;
                        }
                    }
                }

                if (j >= MAX_MONITOR_NODES)
                {
                    //通过名字未找到节点时
                    //新增次节点
                    printf("[Warning] Add Name: %s change the pid %d\n", pMonitorNode->processName, pMonitorNode->pidNum);
                    for (j = 0; j < MAX_MONITOR_NODES; j++)
                    {
                        if (!gFristSaveNodesManger.monitorNodes[j].activeFlag)
                        {
                            gFristSaveNodesManger.monitorNodes[j].pidNum     = pMonitorNode->pidNum;
                            gFristSaveNodesManger.monitorNodes[j].retryCnt   = 0;
                            gFristSaveNodesManger.monitorNodes[j].vmRSSInfo  = pMonitorNode->vmRSSInfo;
                            strncpy(gFristSaveNodesManger.monitorNodes[j].processName, pMonitorNode->processName, strlen(pMonitorNode->processName));

                            gFristSaveNodesManger.monitorNodes[j].activeFlag = 1;
                            break;

                        }
                    }
                }
            }
        }
    }

    if (fpLog)
    {
        fclose(fpLog);
    }

}


int main()
{
    pid_t  pid;
    int i = 0;
    unsigned int count = 0;
    FILE *fpLog = NULL;

    struct timeval start_time, end_time;
    double usetime = 0.0;

    memset(&gNodesManger, 0x0, sizeof(gNodesManger));
    printf("sizeof(gNodesManger) = %ld\n", sizeof(gNodesManger));

    //根据进程名获得父进程号
    gMainPid = getProcessIdbyName(MAIN_PROCESS_NAME);

    printf("[Debug] monitor main pid =%d\n", gMainPid);
    if (gMainPid > 0)
    {
        addMonitorNodeInfo(gMainPid);
        scan_all_childID( gMainPid );
    }
    //dumpMonitorNodeInfo();

    if (gVfscPid > 0)
    {
        scan_all_childID( gVfscPid );
    }

    if (gNodePid > 0)
    {
        scan_all_childID( gNodePid );
    }

#if 1
    while(1)
    {
        count++;
        fpLog = fopen(LOG_FILE_PATH,"a");
        printf("===============Monitor time %d===============\n", count);
        fprintf(fpLog, "===============Monitor time %d===============\n", count);
        if (fpLog)
        {
            fclose(fpLog);
        }

        gettimeofday(&start_time, NULL);
        updateMonitorInfo();
        sortMonitorNodebyMemSize();
        gettimeofday(&end_time, NULL);
        usetime = (end_time.tv_sec - start_time.tv_sec)*1000000.0 + (double)(end_time.tv_usec - start_time.tv_usec);
        printf("[use time] = %.1lf\n", usetime);

        dumpMonitorNodeInfo();
        dumpCPUInfo();

        if (count == 1)
        {
            saveFristTimeMonitorInfo();
        }
        else
        {
            //Save first monitor result
            if (count % 5 == 0)
            {
                compareMonitorInfo();
            }
        }

        sleep(55);
    }
#endif
}
