/******************************************************************************
文件名称∶IFCommFunc.c
系统名称∶期货保证金存管系统
模块名称∶公共函数
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
                            20131203-01-增加一线通处理
                            20131211-02-调整赋值
******************************************************************************/
#include <unistd.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/signal.h>
#include <string.h>
#include "bss.inc"

#define MAX_PKG_LEN     2048     /*收发包结构最大长度*/
#define PKG_HEAD_LEN_G  17       /*20131203-01-增加一线通处理*/

/******************************************************************************
函数名称∶rcvFromIFComm
函数功能∶从服务商接收报文
输入参数∶
        iSocketId:Socket句柄
输出参数∶
        sCommBuf：通讯报文           
返 回 值∶
          > 0  接收到的报文
          < 0  通讯异常  
******************************************************************************/
long rcvFromIFComm(int iSocketId, char *sCommBuf, int iTimeOut)
{
    char cFlag = '0'; /* 报文是否接收完毕 */
    long lRet;
    long lOffset = 0;    
    char sTmp[1024 + 1] = {0};
    int  iCount = 0;
    struct timeval tv; 
    fd_set readfds;
    int retval; 
    int  iTimeOutTmp = iTimeOut;
    
    
    PrintLog(LEVEL_DEBUG, LOG_DEBUG_INFO, "%s() Entry", __FUNCTION__);
    tv.tv_sec   = iTimeOutTmp; 
    tv.tv_usec  = 0;        
    FD_ZERO(&readfds); 
    FD_SET(iSocketId, &readfds);
    retval=select(iSocketId + 1 ,&readfds,NULL,NULL,&tv);
    if( retval == 0)
    {
        ERRLOGF("接收报文超时,[%s]\n", strerror(errno));
        PrintLog(LEVEL_ERR, LOG_NOTIC_INFO, "接收报文超时,[%s]\n", strerror(errno));
        /*超时*/
        return -3;   
    }
    else if( retval == -1 )
    {
        /*超时*/
        return -2;  
    }  
    
    signal(SIGPIPE, SIG_IGN);
        
    while( (cFlag == '0') )
    {
        lRet = recv(iSocketId, sTmp, 1024, 0);  
        if ( lRet < 0 )
        {
            ERRLOGF("接收报文失败,[%s]\n", strerror(errno));
            PrintLog(LEVEL_ERR, LOG_NOTIC_INFO, "接收报文失败,[%s]\n", strerror(errno));            
            return -1;   
        }
        else
        {   
            if ( lRet > 0 )
            {                                                                                                 
                memcpy(sCommBuf + lOffset, sTmp, lRet);
                lOffset += lRet; 
                
                /* 发现</MsgText>表示报文已经结束 */
                if ( strstr(sCommBuf, "</MsgText>") != NULL )
                {
                   cFlag = '1'; 
                } 
                
                PrintLog(LEVEL_DEBUG, LOG_DEBUG_INFO, "报文[%s][%c]", sTmp, cFlag );                
            }
            
            iCount++;
            /* 这里作特殊处理，进行超时控制，避免死等(每10次循环减2秒) */
            if ( iCount > 10 && cFlag == '0' )
            {
                iTimeOutTmp -= 2;
                if ( iTimeOutTmp > 0 )
                {                                    
                    tv.tv_sec   = iTimeOutTmp; 
                    tv.tv_usec  = 0;        
                    FD_ZERO(&readfds); 
                    FD_SET(iSocketId, &readfds);
                    retval=select(iSocketId + 1 ,&readfds,NULL,NULL,&tv);
                    if( retval == 0)
                    {
                        ERRLOGF("接收报文超时,[%s]\n", strerror(errno));
                        PrintLog(LEVEL_ERR, LOG_NOTIC_INFO, "接收报文超时,[%s]\n", strerror(errno));
                        /*超时*/
                        return -3;   
                    }  
                    iCount  = 0;
                }
                else
                {
                    ERRLOGF("接收报文超时,在指定时间内未收完报文,已收[%ld]\n", lOffset);
                    PrintLog(LEVEL_ERR, LOG_NOTIC_INFO, "接收报文超时,在指定时间内未收完报文,已收[%ld]\n", lOffset);
                    /*超时*/
                    return -4;   
                }
            }                      
        }
    }
    PrintLog(LEVEL_DEBUG, LOG_DEBUG_INFO, "%s() Exit", __FUNCTION__);
    return  lOffset;      
}

/******************************************************************************
函数名称∶SendToFutureAndRecv
函数功能∶发送报文给服务商并接收应答
输入参数∶
        sSecuNo  :服务商编号
        sCommBuf :请求报文          
        lCommLen :报文长度
输出参数∶
        sCommBuf: 应答报文
        lCommLen: 应答报文长度
返 回 值∶
          = 0  成功
          < 0  通讯异常 
          -99:通讯超时 
******************************************************************************/
long SendToFutureAndRecv(char *sSecuNo, char *sCommBuf, long *lCommLen)
{
    char sCommIP[128] = {0};
    int  iCommPort = 5000;
    int  iTimeOut = 30;
    int  iSocketId;
    long lRet;
    char sTransMode[128] = {0};                              /*20131203-01-增加一线通处理*/
    char sBuffer[MAX_PKG_LEN + PKG_HEAD_LEN_G + 1] = {0};    /*20131211-02-调整赋值*/
    
    PrintLog(LEVEL_DEBUG, LOG_DEBUG_INFO, "%s() Entry", __FUNCTION__);
    /* 服务商编号 */
    if ( strlen(sSecuNo) == 0 )
    {
         ERRLOGF("取参数失败,原因[服务商编号为空]");
         PrintLog(LEVEL_ERR, LOG_NOTIC_INFO, "取参数失败,原因[服务商编号为空]");
         return -1;
    }
    
    /* IP地址 */
    if ( INIscanf(sSecuNo, "COMMIP", "%s", sCommIP) < 0 )
    {
         ERRLOGF("取[%s]_[COMMIP]参数失败,原因[%s]", sSecuNo, strerror(errno));
         PrintLog(LEVEL_ERR, LOG_NOTIC_INFO, "取[%s]_[COMMIP]参数失败,原因[%s]", sSecuNo, strerror(errno));
         return -1;   
    }
            
    /* 端口 */
    if ( INIscanf(sSecuNo, "COMMPORT", "%d", &iCommPort) < 0 )
    {
         ERRLOGF("取[%s]_[COMMIP]参数失败,原因[%s]", sSecuNo, strerror(errno));
         PrintLog(LEVEL_ERR, LOG_NOTIC_INFO, "取[%s]_[COMMPORT]参数失败,原因[%s]", sSecuNo, strerror(errno));
         return -1;   
    }      
    
    /*  超时时间 */
    if ( INIscanf(sSecuNo, "TIMEOUT", "%d", &iTimeOut) < 0 )
    {
         PrintLog(LEVEL_ERR, LOG_NOTIC_INFO, "取[%s]_[TIMEOUT]参数失败，取默认值:30", sSecuNo);         
    }
    
    /*20131203-01-增加一线通处理*/
    if ( INIscanf(sSecuNo, "TRANSMODE", "%s", sTransMode) < 0 )
    {       
         PrintLog(LEVEL_ERR, LOG_NOTIC_INFO, "取[%s]_[TRANSMODE]参数失败，取默认值:0", sSecuNo);
         sTransMode[0] = '0';
    }
    
    PrintLog(LEVEL_NORMAL, LOG_NOTIC_INFO, "SECUNO[%s],COMMIP[%s],COMMPORT[%d],TIMEOUT[%d]", sSecuNo, sCommIP, iCommPort, iTimeOut);
    
    itcp_setenv(iCommPort, &iSocketId);
    lRet = itcp_calln(sCommIP, 2);
    if ( lRet < 0 )
    {
        ERRLOGF("连接服务商{IP[%s],PORT[%d]}失败", sCommIP, iCommPort); 
        PrintLog(LEVEL_ERR, LOG_NOTIC_INFO, "连接服务商{IP[%s],PORT[%d]}失败", sCommIP, iCommPort); 
        return -2;
    }
    itcp_setpkghead(1);
    
    if (sTransMode[0] == '1')
    {
        /*20131211-02-调整赋值*/
        sprintf(sBuffer, "G%03d%05ld%8s%s", PKG_HEAD_LEN_G, *lCommLen, sSecuNo, sCommBuf);
        lRet = itcp_send(sBuffer, *lCommLen + PKG_HEAD_LEN_G , 10);
    }
    else
    {
        lRet = itcp_send(sCommBuf, *lCommLen , 10);
    }
    
    if ( lRet < 0 )
    {
        ERRLOGF("发送报文至服务商失败"); 
        PrintLog(LEVEL_ERR, LOG_NOTIC_INFO, "发送报文至服务商失败"); 
        itcp_clear();
        return -2;
    }
    
    /* 初始化 */
    memset(sCommBuf, 0x00, *lCommLen);
    
    *lCommLen = rcvFromIFComm(iSocketId, sCommBuf, iTimeOut);
    itcp_clear();
    if ( *lCommLen < 0 )
    {
        ERRLOGF("接收服务商应答报文失败"); 
        PrintLog(LEVEL_ERR, LOG_NOTIC_INFO, "接收服务商应答报文失败");       
        return -99;              
    }   
    PrintLog(LEVEL_DEBUG, LOG_DEBUG_INFO, "%s() Exit", __FUNCTION__); 
    return 0;
}

/******************************************************************************
函数名称∶SendToOltpAndRecv
函数功能∶发送报文给交易总控并接收应答
输入参数∶
        sSecuNo  :服务商编号
        sCommBuf :请求报文          
        lCommLen :报文长度
输出参数∶
        sCommBuf :请求报文          
        lCommLen :报文长度
        sErrMsg  :错误信息(异常时返回)        
返 回 值∶
          = 0  成功
          < 0  通讯异常  
******************************************************************************/
long SendToOltpAndRecv(char *sSecuNo, char *sCommBuf, long *lCommLen, char *sErrMsg)
{
    char sTmpBuf[2048] = {0};    
    char sCommIP[128] = {0};
    int  iCommPort = 5000;
    int  iTimeOut = 30;
    int  iSocketId;
    long lRet;
    int  iDataLen;
    char sTmpLen[5 + 1] = {0};
    char sKeyFlag[256 + 1] = "200";
        
    PrintLog(LEVEL_DEBUG, LOG_DEBUG_INFO, "%s() Entry", __FUNCTION__);
    /* IP地址 */
    if ( INIscanf("COMMINFO", "OLTPIP", "%s", sCommIP) < 0 )
    {
         ERRLOGF("取[COMMINFO]_[OLTPIP]参数失败,原因[%s]", strerror(errno));
         PrintLog(LEVEL_ERR, LOG_NOTIC_INFO, "取[COMMINFO]_[OLTPIP]参数失败,原因[%s]", strerror(errno));
         strcpy(sErrMsg, "1041|取通讯参数OLTPIP失败|");
         return -1;   
    }
    
    /* 端口 */
    if ( INIscanf("COMMINFO", "OLTPPORT", "%d", &iCommPort) < 0 )
    {
         ERRLOGF("取[COMMINFO]_[COMMIP]参数失败,原因[%s]", strerror(errno));
         PrintLog(LEVEL_ERR, LOG_NOTIC_INFO, "取[COMMINFO]_[OLTPPORT]参数失败,原因[%s]", strerror(errno));
         strcpy(sErrMsg, "1041|取通讯参数OLTPPORT失败|");
         return -1;   
    }
    
    /*  超时时间 */
    if ( INIscanf("COMMINFO", "OLTPTIMEOUT", "%d", &iTimeOut) < 0 )
    {       
         PrintLog(LEVEL_ERR, LOG_NOTIC_INFO, "取[COMMINFO]_[OLTPTIMEOUT]参数失败，取默认值:30");         
    }
    
    /* KEY_FLAG */
    INIscanf(sSecuNo, "KEY_FLAG", "%s", sKeyFlag);
            
    PrintLog(LEVEL_DEBUG, LOG_DEBUG_INFO, "OLTPIP[%s],PORT[%d],TIMEOUT[%d][%s]", sCommIP, iCommPort, iTimeOut, sKeyFlag);
    
    
    snprintf(sTmpBuf, sizeof(sTmpBuf), "F028%05.5ld%-8.8s%-3.3s%-8.8s", *lCommLen, sSecuNo, sKeyFlag, " ");
    memcpy(sTmpBuf + 28, sCommBuf, *lCommLen); 
    
    itcp_setenv(iCommPort, &iSocketId);
    lRet = itcp_calln(sCommIP, 3);
    if ( lRet < 0 )
    {
        ERRLOGF("连接交易总控{IP[%s],PORT[%d]}失败", sCommIP, iCommPort); 
        PrintLog(LEVEL_ERR, LOG_NOTIC_INFO, "连接交易总控{IP[%s],PORT[%d]}失败", sCommIP, iCommPort); 
        strcpy(sErrMsg, "9001|连接银行前置机失败|");
        return -2;
    }
    
    itcp_setpkghead(1);    
    lRet = itcp_send(sTmpBuf, *lCommLen + 28 , iTimeOut);   
    if ( lRet < 0 )
    {
        ERRLOGF("发送报文至总控失败"); 
        PrintLog(LEVEL_ERR, LOG_NOTIC_INFO, "发送报文至总控失败");
        strcpy(sErrMsg, "9002|向银行前置机发送报文失败|"); 
        itcp_clear();
        return -2;
    }
    memset(sTmpBuf, 0x00, sizeof(sTmpBuf));
    lRet = itcp_recv(sTmpBuf, 28 , iTimeOut);   
    if ( lRet < 0 )
    {
        ERRLOGF("接收总控应答包头失败"); 
        PrintLog(LEVEL_ERR, LOG_NOTIC_INFO, "接收总控应答包头失败"); 
        itcp_clear();
        return -99;
    }
    
    memcpy(sTmpLen, sTmpBuf + 4, 5);
    iDataLen = atoi(sTmpLen);
    PrintLog(LEVEL_NORMAL, LOG_NOTIC_INFO, "包体长度[%d]", iDataLen );
    
    if ( iDataLen < 0 || iDataLen >  2048 )
    {
        PrintLog(LEVEL_ERR, LOG_NOTIC_INFO, "包长度异常,DataLen[%d],[%s]", iDataLen, sTmpLen); 
        itcp_clear();
        return -99;        
    }
    memset(sCommBuf, 0x00, MAX_PKG_LEN);
    *lCommLen = itcp_recv(sCommBuf, iDataLen , iTimeOut); 
    itcp_clear();  
    if ( *lCommLen < 0 )
    {
        ERRLOGF("接收总控应答包体失败"); 
        PrintLog(LEVEL_ERR, LOG_NOTIC_INFO, "接收总控应答包体失败");         
        return -99;
    }  
    PrintLog(LEVEL_DEBUG, LOG_DEBUG_INFO, "%s() Exit", __FUNCTION__);  
    return 0;      
}
