/******************************************************************************
文件名称∶IFB2Fmain.c
系统名称∶期货保证金存管系统
模块名称∶银行发起接口转换（工行类接口）
软件版权∶恒生电子股份有限公司
功能说明∶
系统版本∶3.0
开发人员∶
开发时间∶2013.3.14
审核人员∶
审核时间∶
相关文档∶
修改记录∶版本号    修改人  修改标签    修改原因
          V3.0.0.0          20130314-01 初定版本

******************************************************************************/
#include <dlfcn.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <errno.h>
#include <sys/types.h>
#include "bss.inc"
#include "log.h"
#include "IFCommFunc.c"

/*********************************常量定义************************************/

/**********************************宏定义*************************************/
#define MAX_PKG_LEN     2048     /*收发包结构最大长度*/

/*********************************类型定义************************************/

/*******************************结构/联合定义*********************************/

/*******************************外部变量声明**********************************/

/*******************************全局变量定义**********************************/

int     g_iServerPort;                   /* 进程侦听端口 */
int     g_iServerSock=-1;                /* 服务器端句柄 */
char    g_sSecuNo[DB_SECUNO_LEN+1];      /* 期货公司编号 */
char    g_sTransCode[DB_TRSCDE_LEN+1];   /* 交易代码 */
char    g_sExSerial[32];                 /* 外部流水编号 */
char    g_sPkgHead[PKG_HEAD_LEN + 1];    /* 存放报文头 */
char	g_errcode[12];                   /* 错误码       */
char    g_errmsg[156];                   /* 错误信息     */ 
/*******************************函数原型声明**********************************/

void OUT(int);                                        /* 终止服务进程 */
void CLD_OUT(int);                                    /* 终止子服务进程 */
void cld_out(int a);
void SEGV_OUT(int a);
void showversion(const char *filename);               /* 显示版本号 */
int  AnsProcessXml2Fix(char *ansbuf, long *panslen, char *phsdata, long *hslen);
int  ReqProcessFix2Xml(char *xmlReq, long *preqlen, FIX pfix);
long rcvFromClient(char *sReqBuf, char *sAnsBuf,  long *lAnsLen, FIX **pfix);
long SendToClient(char *sAnsBuf, long lAnsLen);
long RetError(char *errCode, char *errMsg, char *sAnsBuf);
long requestProcess(FIX  *pfix, char *sAnsBuf, long *lAnsLen);

int warn(char *fmt, ...);

int main(int argc, char *argv[])
{
    struct sigaction new_action_cld, new_action_term,new_action_pipe;
    long lPrepProcNum;                   /* 缓冲进程数量 */
    long lSaveProcNum;                   /* 保存进程数量 */
    long lMaxProcNum;                    /* 最大进程数量 */
    long lHoldProcNum;                   /* 保留进程数量 */
    long lDelay;                         /* 进程监控时间间隔 */
    long lProcLife;                      /* 进程生命 */
    long lCurrentRec;                    /* 进程在进程池的位置 */
    long lChildPid;                      /* 子进程进程号       */
    long lRet;                           /* 返回值             */

    long lLastProc;                      /* 最近最大进程数 */
    long lLastBusy;                      /* 最近最忙进程数 */
    long lCurProc;                       /* 当前最大进程数 */
    long lCurBusy;                       /* 当前最忙进程数 */
    long lMaxConn;                       /* 最大侦听队列 */
    int  iLogLevel;                      /* 日志级别 */

    long lReqLen,lAnsLen;                /* 请求,应答包长度 */    
    char sReqBuf[MAX_PKG_LEN], sAnsBuf[MAX_PKG_LEN];     /*请求,应答包(总控)*/
    
    char sTmp[200];
    char sBuffer[128];                   /* 缓存               */
    FIX  *Reqfix = NULL;                 /* 请求FIX */
    
    if (getopt(argc, argv, "v") == 'v')
    {
        showversion((const char *)argv[0]);
        return 0;
    }

    /* 主服务进程个数只能启动一个 */
    if (testprg(argv[0]) > 1)
    {
        printf("与期货公司通信进程已经启动!\n");
        ERRLOGF( "与期货公司通信进程已经启动!\n");
        exit(1);
    }

    /* 交易日志初始化 */
    SetTransLogName(argv[0]);

    /* 记录交易日志: 进程开启时间 */
    warn("进程开启时间：%ld\n", GetSysTime());
    TransLog("", 0, "进程开启时间");

    /*期货公司接口配置文件*/
    SetIniName("future.ini");
    
    /*获取本进程的服务端口*/     
    g_iServerPort = 5001;        
    INIscanf(argv[0], "PORT", "%d", &g_iServerPort);    
    TransLog("", 0, "服务进程端口PORT：%d", g_iServerPort);
    warn("服务进程端口[g_iServerPort=%ld]\n", g_iServerPort);
    
    
    /*取缓冲进程数量*/
    lPrepProcNum = 1;
    INIscanf(argv[0], "PREP_PROC", "%ld", &lPrepProcNum);
    TransLog("", 0, "缓冲进程数量PREP_PROC：%ld", lPrepProcNum);
    warn("缓冲进程数量[PREP_PROC=%ld]\n", lPrepProcNum);

    /*取保存进程数量*/
    lSaveProcNum = 8;
    INIscanf(argv[0], "SAVE_PROC", "%ld", &lSaveProcNum);
    TransLog("", 0, "保存进程数量SAVE_PROC：%ld", lSaveProcNum);
    warn("保存进程数量[SAVE_PROC=%ld]\n", lSaveProcNum);

    /*取最大进程数量*/
    lMaxProcNum = 10;
    INIscanf(argv[0], "MAX_PROC", "%ld", &lMaxProcNum);
    TransLog("", 0, "最大进程数量MAX_PROC：%ld", lMaxProcNum);
    warn("最大进程数量[MAX_PROC=%ld]\n", lMaxProcNum);

    /*进程监控时间间隔*/
    lDelay = 3;
    INIscanf(argv[0], "MON_POOL_SPACE", "%ld", &lDelay);
    TransLog("", 0, "进程监控时间间隔MON_POOL_SPACE：%ld", lDelay);
    warn("进程监控时间间隔[MON_POOL_SPACE=%ld]\n", lDelay);

    /*进程生命*/
    lProcLife = 30;
    INIscanf(argv[0], "PROC_LIFE", "%ld", &lProcLife);
    TransLog("", 0, "进程生命PROC_LIFE：%ld", lProcLife);
    warn("进程生命[PROC_LIFE=%ld]\n", lProcLife);
    
    /*日志级别*/
    iLogLevel = 1;
    INIscanf(argv[0], "LOG_LEVEL", "%d", &iLogLevel);
    SetLogThresHold(iLogLevel);
    TransLog("", 0, "日志级别:%d", iLogLevel);
    warn("日志级别[LOG_LEVEL=%d]\n", iLogLevel);
    
    /* 设置字典信息 */
    memset(sTmp, 0x00, sizeof(sTmp));
    sprintf(sTmp, "%s/etc/IFDict.dat", getenv("HOME"));
    SetDictEnv(sTmp);
    
    TransLog("", 0, "字典信息对照表文件[%s]", sTmp);
    warn("字典信息对照表文件[%s]\n", sTmp);
     
    /*没有保留进程*/
    lHoldProcNum = 0;
    
    daemon_start(1);

    new_action_cld.sa_handler = cld_out;/* cld_out为SIGCHLD信号处理函数名*/
    sigemptyset(&new_action_cld.sa_mask);
    new_action_cld.sa_flags = 0;
    sigaction(SIGCHLD, &new_action_cld, NULL);

    new_action_term.sa_handler = CLD_OUT;/* OUT为SIG_TERM信号处理函数名*/
    sigemptyset(&new_action_term.sa_mask);
    new_action_term.sa_flags = 0;
    sigaction(SIGTERM, &new_action_term, NULL);

    new_action_pipe.sa_handler = SIG_IGN;/* SIG_PIPE忽略信号处理函数*/
    sigemptyset(&new_action_pipe.sa_mask);
    new_action_pipe.sa_flags = 0;
    sigaction(SIGPIPE, &new_action_pipe, NULL);
   
    /* 防止进程coredump而导致日志记录丢失 */
    signal(SIGSEGV,SEGV_OUT);
    signal(SIGBUS, SEGV_OUT); 
   
    /*初始化进程池*/
    sprintf(sBuffer, "%s/tmp/KEY_%s", getenv("HOME"), argv[0]);
    if( (lRet = pool_init(lPrepProcNum, lSaveProcNum, lMaxProcNum, lHoldProcNum, sBuffer)) < 0 )
    {
        ERRLOGF("初始化进程池[%s]失败", sBuffer);
        TransLog("", 0, "初始化进程池[%s]失败[%ld][%ld]", sBuffer, lRet,errno );        
        fprintf(stderr, "初始化进程池[%s]失败\n", sBuffer);
        warn("初始化进程池[%s]失败\n", sBuffer);
        exit(1);
    }
    TransLog("", 0, "初始化进程池[%s]成功", sBuffer);
    warn("初始化进程池[%s]成功\n", sBuffer);

    /* 初始化监听端口*/
    itcp_setenv(g_iServerPort, &g_iServerSock);
    
    /* 最大侦听队列 */
    lMaxConn = 20;
    INIscanf(argv[0], "MAXCONN", "%ld", &lMaxConn);
    itcp_setmaxconn(lMaxConn);
    TransLog("", 0, "最大侦听队列MAXCONN：%ld", lMaxConn);
    
    /*判断端口侦听*/
    if (itcp_listen() == -1)
    {
        TransLog("", 0, "侦听端口[%ld]失败", g_iServerPort);        
        warn("侦听端口[%ld]失败",g_iServerPort);
        exit(1);
    } 
    
    lLastProc = 0;
    lLastBusy = 0;

    warn("系统初始化结束\n");
    WriteLog("");

    for (;;)
    {
        if ((lCurrentRec = pool_monitor(lDelay, lProcLife, &lCurProc, &lCurBusy)) < 0)
        {         
            if ( lCurProc > lLastProc || lCurBusy > lLastBusy )
            {
                if ( lCurProc > lLastProc )
                   lLastProc = lCurProc;
                
                if ( lCurBusy > lLastBusy  )
                   lLastBusy = lCurBusy ;
                
                memset(sTmp, 0x00, sizeof(sTmp));
                sprintf(sTmp,"%04ld@%04ld@",lLastProc, lLastBusy);
                
                /* 更新进程信息 */
                ModifyCfgItem(argv[0],"PROC_INFO",sTmp);                
            }
            continue;
        }

        switch ((lChildPid = fork()))
        {
            case -1:
                sleep(1);
                continue;
            case 0:
                /* 设置子进程退出的处理函数 */
                SetTransLogName(argv[0]);                       

                /*一般银行处理进程*/
                while(1)
                {
                    lRet = itcp_wait();
                    if (lRet < 0)
                    {
                         sleep(1);
                         continue;
                    }             
                    SetLogSerial((long )GetSerial());
                    pool_setstatus(1); 
                    itcp_setpkghead(1);                    
                    memset(sReqBuf,0x00,sizeof(sReqBuf));
                    memset(sAnsBuf,0x00,sizeof(sAnsBuf));                    
                    memset(sTmp,0x00,sizeof(sTmp));
                    memset(g_sTransCode, 0x0, sizeof(g_sTransCode));
                    memset(g_sSecuNo, 0x00, sizeof(g_sSecuNo));
                    memset(g_sExSerial, 0x00, sizeof(g_sExSerial));
                    memset(g_sPkgHead, 0x00, sizeof(g_sPkgHead));
                    Reqfix = NULL;
                    
                    lReqLen = 0;
                    lAnsLen = 0;
                                                                                             
                    lRet = rcvFromClient(sReqBuf, sAnsBuf, &lAnsLen, &Reqfix);
                    if (lRet < 0) /* 通信异常 */
                    {
                        WriteLog("");
                        itcp_setenv(g_iServerPort,&g_iServerSock);
                        itcp_clear();                        
                        pool_setstatus(0);                        
                        continue;
                    }
                    else if ( lRet > 0 ) /* 接收报文成功 */
                    {                                                                                              
                        lRet = requestProcess( Reqfix, sAnsBuf, &lAnsLen );                        
                        FIXclose(Reqfix); 
                        if ( lRet < 0)  /* 处理异常不给应答 */                         
                        {     
                            WriteLog("");                      
                            itcp_setenv(g_iServerPort, &g_iServerSock);
                            itcp_clear();                                                      
                            pool_setstatus(0);                            
                            continue;
                        }                                                                                            
                    }
                       
                    /*发送应答数据*/
                    SendToClient( sAnsBuf, lAnsLen );
                    WriteLog("");                   
                    pool_setstatus(0);
                }
                break;
            default:
                pool_setpid(lChildPid);
        }
    }
}


/******************************************************************************
函数名称∶ rcvFromClient
函数功能∶ 从OLTP接收交易请求
输入参数∶ 无
输出参数∶ 
          sReqBuf    -- 请求包文
          pfix       -- 正常返回 
          sAnsBuf    -- 应答报文(异常时返回)
          lAnsLen    -- 应答报文长度(异常时返回)
          
返 回 值∶> 0 -- 接收请求成功, 返回实际接收的正文长度
          = 0 -- 一般性错误
          < 0 -- 报文异常           
******************************************************************************/
long rcvFromClient(char *sReqBuf, char *sAnsBuf,  long *lAnsLen, FIX **pfix)
{        
    char   sErrMsg[255];
    char   sTmpLen[6];    
    char   sFixErrMsg[255];
    long   lPkgOffSet  = 0;           /* 报文偏移量 */
    long   lPkgHeadLen;               /* 报文头长度 */
    long   lPkgBodyLen;               /* 报文体长度 */
    long   lRet;
         
    itcp_setpkghead(1);
    itcp_setenv(g_iServerPort, &g_iServerSock);
    
    /*接收交易请求包头*/
    if ((lRet = itcp_recv(sReqBuf, PKG_HEAD_LEN, 10)) <= 0)
    {
        itcp_clear();
        ERRLOGF( "接收交易请求包头错误,lRet=[%ld]\n",lRet);
        return -1;
    }

    /* 收到的日志打印 */
    PrintLog(LEVEL_DEBUG, LOG_DEBUG_INFO, "接收交易包头[%s]", sReqBuf);            
   
    /* 检查请求报文类型 */
    if ( sReqBuf[0] != 'F' )
    {
        ERRLOGF( "交易请求包类型错误,pkgHead=[%s]\n",sReqBuf);
        return -2;
    }
    
    memset(sTmpLen, 0x00, sizeof(sTmpLen));
    memcpy(sTmpLen, sReqBuf + 1, 3);
    lPkgHeadLen = atol(sTmpLen);
    memset(sTmpLen, 0x00, sizeof(sTmpLen));
    memcpy(sTmpLen, sReqBuf + 4, 5);
    lPkgBodyLen = atol(sTmpLen);
    
    if (lPkgHeadLen != PKG_HEAD_LEN )
    {
        itcp_clear();
        ERRLOGF( "交易请求包头长度错误!\n","");
        return -2; 
    }
                 
    memcpy(g_sPkgHead, sReqBuf, lPkgHeadLen);
    g_sPkgHead[lPkgHeadLen] = 0x00;
    
    memset(g_sSecuNo, 0x00, sizeof(g_sSecuNo));
    memcpy(g_sSecuNo, g_sPkgHead + 9, DB_SECUNO_LEN);     
    
    /* 包长错控制 */
    if (lPkgBodyLen == 0 ||  lPkgBodyLen > 4096)
    {                    
        *lAnsLen = RetError(ERR_FIX_OPEN, "包体长度为0或超长", sAnsBuf);        
        return -6;
    }
    
    /*接收请求*/
    lPkgBodyLen = itcp_recv(sReqBuf, lPkgBodyLen, 10);
    if (lPkgBodyLen <= 0)
    {
        itcp_clear();
        *lAnsLen = RetError(ERR_FIX_OPEN, "接收包体失败", sAnsBuf);
        ERRLOGF( "接收请求交易包错误! lRecvLen = [%ld]\n", lPkgBodyLen);
        return -4;
    }
               
    PrintLog(LEVEL_DEBUG, LOG_DEBUG_INFO, sReqBuf);
                     
    /* 包文为FIX结构  */
    if ((*pfix = FIXopen(sReqBuf, lPkgBodyLen, 2048, sFixErrMsg)) == NULL)
    {
        sprintf(sErrMsg, "FIX包打开错误2[%s]",sFixErrMsg);
        *lAnsLen = RetError(ERR_FIX_OPEN, sErrMsg, sAnsBuf);        
        return 0;
    }
    FIXgets(*pfix, "FunctionId", g_sTransCode, DB_TRSCDE_LEN + 1);
    FIXgets(*pfix, "sOrganId", g_sSecuNo, DB_SECUNO_LEN + 1);   
    FIXgets(*pfix, "ExSerial", g_sExSerial, sizeof(g_sExSerial) );   
        
    PrintFIXLog(LEVEL_NORMAL, "收到总控请求", *pfix);   
    
    return lPkgBodyLen;
}

/******************************************************************************
函数名称∶requestProcess
函数功能∶转换处理
输入参数∶pfix     -- 请求报文FIX信息          
输出参数∶
          sAnsBuf  -- 应答报文          
          lAnsLen  -- 应答报文长度
返 回 值∶
          = 0  需要应答
          < 0  无须应答
******************************************************************************/
long requestProcess(FIX  *pfix, char *sAnsBuf, long *lAnsLen)
{
    char sCommBuf[MAX_PKG_LEN]; 
    long lCommLen;
    long lRet;      
    
    /* FIX->XML【函数内会主动关闭FIX】*/
    memset(sCommBuf,0,sizeof(sCommBuf));
    lRet = BankConvFixToXml(pfix, sCommBuf, &lCommLen);
    if(lRet < 0)
    {
        ERRLOGF("[%d][%s][%d]\n", __LINE__, "将恒生报文格式转换为期货公司数据格式失败!",lRet);
        PrintLog(LEVEL_ERR,LOG_NOTIC_INFO,"将恒生报文格式转换为期货公司数据格式失败[%ld]\n", lRet);
        *lAnsLen = FIXlen(pfix); 
        memcpy(sAnsBuf, FIXBuffer(pfix), *lAnsLen);   
        PrintFIXLog(LEVEL_NORMAL, "发送总控应答", pfix); 
        return 0;
    }
        
    PrintLog(LEVEL_NORMAL, "发送服务商请求", sCommBuf);
                            
    /* 发送请求到期货公司并接收应答*/                       
    lRet = SendToFutureAndRecv(g_sSecuNo, sCommBuf, &lCommLen);                      
    if( lRet < 0)                           
    {   
        /* OLTP通讯超时  */
        if ( lRet == -99 )
        {           
            return -1;   
        } 
        else
        {                                                        
            FIXputs(pfix, "ErrorNo",    "5901");
            FIXputs(pfix, "ErrorInfo",  "与服务商通讯失败");
            *lAnsLen = FIXlen(pfix); 
            memcpy(sAnsBuf, FIXBuffer(pfix), *lAnsLen); 
            PrintFIXLog(LEVEL_NORMAL, "发送总控应答", pfix); 
            return 0;
        }
    }                        
    
    PrintLog(LEVEL_NORMAL, "接收服务商应答", sCommBuf);
                                          
    /* XML->FIX */                                         
    lRet = BankConvXmlToFix(pfix, sCommBuf, lCommLen);
    if (lRet < 0)
    {
        ERRLOGF("[%d][%s][%d]\n", __LINE__, "将期货公司应答数据报转换为恒生格式失败!",lRet);
        PrintLog(LEVEL_ERR,LOG_NOTIC_INFO,"将期货公司应答数据报转换为恒生格式失败[%ld]\n", lRet); 
        *lAnsLen = FIXlen(pfix); 
        memcpy(sAnsBuf, FIXBuffer(pfix), *lAnsLen);   
        PrintFIXLog(LEVEL_NORMAL, "发送总控应答", pfix);                                
        return -2;        
    } 
       
    /* 获取FIX报文 */
    *lAnsLen = FIXlen(pfix); 
    memcpy(sAnsBuf, FIXBuffer(pfix), *lAnsLen); 
    PrintFIXLog(LEVEL_NORMAL, "发送总控应答", pfix); 
            
    return 0;
}

/******************************************************************************
函数名称∶ SendToClient
函数功能∶ 发送应答给总控
输入参数∶ 
            sAnsBuf -- 应答报文
            lAnsLen -- 应答报文长度
输出参数∶ 
          无
返 回 值∶ = 0 发送成功
******************************************************************************/
long SendToClient(char *sAnsBuf, long lAnsLen)
{                   
    char  sTmpBuf[MAX_PKG_LEN + PKG_HEAD_LEN + 1];
    long  lRet;
    
    itcp_setpkghead(1);                                        
    itcp_setenv(g_iServerPort,&g_iServerSock);
    
    memset( sTmpBuf, 0x00, sizeof(sTmpBuf));
    snprintf(sTmpBuf, sizeof(sTmpBuf), "F028%05.5ld%-19.19s%s", lAnsLen, g_sPkgHead + 9, sAnsBuf);
    
    PrintLog(LEVEL_DEBUG,"发送总控应答",sTmpBuf);
                                                      
    /* 包体报文 */                             
    lRet = itcp_send(sTmpBuf, lAnsLen + PKG_HEAD_LEN, 10);
    itcp_clear();
    if (lRet != 0)
    {
        ERRLOGF( "发送应答包错误! ret = [%ld]\n", lRet);
        TransLog("", 0, "发送应答失败");
        return -1;
    }      
           
    return 0;                  
}

/******************************************************************************
函数名称∶RetError
函数功能∶把错误信息写入应答缓冲区并且返回其正文长度
输入参数∶g_sTransCode -- 交易码
          errCode   -- 错误代码
          errMsg    -- 错误信息
          fixFlag   -- 是否使用fix协议的标志
输出参数∶sAnsBuf    -- 应答缓冲区指针
返 回 值∶正文长度
******************************************************************************/
long RetError(char *errCode, char *errMsg, char *sAnsBuf)
{
    FIX  *pfix;
    long lBuflen;    
    
    pfix = FIXopen(sAnsBuf, 0, 10240, NULL);
    FIXputs(pfix, "FunctionId", g_sTransCode);
    FIXputs(pfix, "ExSerial",    g_sExSerial);
    FIXputs(pfix, "ErrorNo",    errCode);
    FIXputs(pfix, "ErrorInfo",  errMsg);
    lBuflen = FIXlen(pfix);
    PrintFIXLog(LEVEL_NORMAL,"发送总控应答", pfix);
    FIXclose(pfix);
    return lBuflen;    
}


/******************************************************************************
函数名称∶CLD_OUT
函数功能∶信号SIGTERM捕捉函数（子进程）
输入参数∶无
输出参数∶无
返 回 值∶无
******************************************************************************/
void CLD_OUT(int a)
{
    char sBuffer[64];

    sprintf(sBuffer, "子进程关闭时间：%s", GetSysTime());
    TransLog("", 0, sBuffer);
    WriteLog("");
    
    itcp_setenv(g_iServerPort, &g_iServerSock);
    itcp_clear();

    pool_poolclear();
    exit(0);
}

/******************************************************************************
函数名称∶SEGV_OUT
函数功能∶信号SIGSEGV捕捉函数
输入参数∶无
输出参数∶无
返 回 值∶无
******************************************************************************/
void SEGV_OUT(int a)
{        
    TransLog("", 0, "子进程coredump");    
    WriteLog("");
    
    itcp_setenv(g_iServerPort, &g_iServerSock);
    itcp_clear();

    pool_poolclear();
    exit(-1);
}

void cld_out(int a)
{
    int istatus, isig;
    pid_t pid;
    FILE *fp;
    
    while ((pid = waitpid(0, &istatus, WNOHANG)) > 0 )
    { 
        if(WIFEXITED(istatus)) /*正常终止的子进程*/
        {        
            continue;
        }
        
        if(WIFSIGNALED(istatus)) /*由于异常而终止的子进程 */
        {
            isig=WTERMSIG(istatus);        
            fprintf(stderr,"abnormal termination,signal istatus=%d,isig=%d,pid=%d,%s:\n",istatus,isig,pid,
        
            #ifdef WCOREDUMP
                WCOREDUMP(istatus)?")core file generated)":"");
            #else
                "");
            #endif
            ERRLOGF( "abnormal termination,signal istatus=%d,isig=%d,pid=%d\n",istatus,isig,pid);
            continue;
        }
        ERRLOGF( "子进程未知终止 istatus=%d,pid=%d,errno=%d\n",istatus,pid,errno);
    }
    return;
}


/*******************************************
 *功能说明：输出警告信息
 *输入参数：
 *输出参数：
 *返回值：  
 *创建时间：
 *创建者：
 *修改记录： 
*********************************************/
int warn(char *fmt, ...) 
{ 
    long r;
    va_list ap;
    va_start(ap, fmt);
    r = vfprintf(stderr, fmt, ap);
    va_end(ap);
    return r;
}

/*
** 函数名称∶showversion
** 函数功能∶显示主控程序的版本号
** 输入参数∶const char *filename -- 文件名
** 输出参数∶无
** 返 回 值∶无
*/
void showversion(const char *filename)
{
    #define SYSTEM_VER "V3.0.0.0"       /* V后面的字符，若带数据库的，加O-Oracle;I-Informix;S-Sybae:D-DB2 版本号3.0 + 兼容序号（从1开始编号）+发行序号（从1开始编号）*/
    fprintf(stderr, "【期货保证金存管系统】%-17s版本号 %-17s日期 %s\n", filename, SYSTEM_VER, __DATE__);
    return;
}

/*********************************文件结束************************************/

