#include <stdio.h>
#include <sys/types.h>
#include <dirent.h>
#include <string.h>

/* Define declare */
#define SDF_SUFFIX                ".sdf"
#define SDF_PATH                  "./sdf/"
#define MAX_FILENAME_BUFSIZE      30
#define MAX_FILE_LINE_STR_BUFSIZE 128

/* Globle member declare */
enum SDF_META_E {
  META_MODE = 0,
  META_RESTART,
  META_EXEC,
  META_AFTER,
  META_REQUIRES,
  META_WORKDIR,
  META_ENV,
  META_PWRCONDITION,
  META_CPULIMIT,
  META_MEMLIMIT,
  META_HEARTBEATINTERVAL,
  META_MAX,
};

const char *sdf_metakey[] = {
    [META_MODE]              = "Mode",
    [META_RESTART]           = "Restart",
    [META_EXEC]              = "Exec",
    [META_AFTER]             = "After",
    [META_REQUIRES]          = "Requires",
    [META_WORKDIR]           = "WorkDir",
    [META_ENV]               = "Environment",
    [META_PWRCONDITION]      = "PwrCondition",
    [META_CPULIMIT]          = "CpuLimit",
    [META_MEMLIMIT]          = "MemLimit",
    [META_HEARTBEATINTERVAL] = "HeartBeatInterval"
};

/* Func declare */
int paseSDFFile(const char *pName);

/*****************
 * @func:
 * @param:
 * @return:  0  success
 *           -1 fail
 * **************/
 int paseSDFDir()
 {
     DIR *pDirptr          = NULL;
     struct dirent *pEntry = NULL;
     int i                 = 0;
     char *suffix          = NULL;

     if ((pDirptr = opendir(SDF_PATH)) == NULL)
     {
         printf("[Fail] open dir %s fail!\n", SDF_PATH);
         return -1;
     }
     else
     {
         while (pEntry = readdir(pDirptr))
         {
             if (strncmp(pEntry->d_name, ".", 1) == 0)
             {
                continue;
             }
             else if (strncmp(pEntry->d_name, "..", 2) == 0)
             {
                continue;
             }
             else
             {
                /*
                 enum
                 {
                     DT_UNKNOWN = 0,         //未知类型
                 # define DT_UNKNOWN DT_UNKNOWN
                     DT_FIFO = 1,            //管道
                 # define DT_FIFO DT_FIFO
                     DT_CHR = 2,             //字符设备
                 # define DT_CHR DT_CHR
                     DT_DIR = 4,             //目录
                 # define DT_DIR DT_DIR
                     DT_BLK = 6,             //块设备
                 # define DT_BLK DT_BLK
                     DT_REG = 8,             //常规文件
                 # define DT_REG DT_REG
                     DT_LNK = 10,            //符号链接
                 # define DT_LNK DT_LNK
                     DT_SOCK = 12,           //套接字
                 # define DT_SOCK DT_SOCK
                     DT_WHT = 14             //链接
                 # define DT_WHT DT_WHT
                 };
                 */
                //if the file type is not DT_BLK skip it
                if (pEntry->d_type != DT_REG)
                {
                    continue;
                }

                //if the file suffix is not ".sdf" skip it
                suffix = strrchr(pEntry->d_name, '.');
                if (suffix == NULL)
                {
                    continue;
                }

                if (strncmp(suffix, SDF_SUFFIX, 4) != 0)
                {
                    continue;
                }

                i++;
                printf("pase file%d Name =%s\n", i, pEntry->d_name);
                paseSDFFile(pEntry->d_name);
             }
         }
         closedir(pDirptr);
     }

     return 0;
 }



/*****************
 * @func:
 * @param:
 * @return:  0  success
 *           -1 fail
 * **************/
int paseSDFFile(const char *pName)
{
    FILE *fp = NULL;
    int  cnt = 0;
    char fileNameBuf[MAX_FILENAME_BUFSIZE]     = {0};
    char lineStrBuf[MAX_FILE_LINE_STR_BUFSIZE] = {0};

    cnt = snprintf(fileNameBuf, MAX_FILENAME_BUFSIZE, "%s%s", SDF_PATH, pName);
    printf("open file name = %s, cnt = %d\n", fileNameBuf, cnt);
    fp = fopen(fileNameBuf, "r");
    if (fp == NULL)
    {
        printf("open file %s fail!\n", fileNameBuf);
        return -1;
    }

    while(!feof(fp))
    {
        memset(lineStrBuf, 0, MAX_FILE_LINE_STR_BUFSIZE);
        if (fgets(lineStrBuf, MAX_FILE_LINE_STR_BUFSIZE, fp) == NULL)
        {
            printf("fget result is NULL！\n");
            break;
        }

        printf("line = %s",lineStrBuf);

        if (strncmp(lineStrBuf, sdf_metakey[META_EXEC], strlen(sdf_metakey[META_EXEC])) == 0)
        {
            printf("match Exce prefix = %s\n", sdf_metakey[META_EXEC]);
        }
    }

    fclose(fp);
}


int main()
{
    int ret = -1;

    ret = paseSDFDir();
    if (ret != 0)
    {
        printf("paseSDFDir fail!\n");
    }

    return 0;
}
