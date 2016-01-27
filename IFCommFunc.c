/******************************************************************************
�ļ����ơ�IFCommFunc.c
ϵͳ���ơ��ڻ���֤����ϵͳ
ģ�����ơù�������
�����Ȩ�ú������ӹɷ����޹�˾
����˵����
ϵͳ�汾��3.0
������Ա��
����ʱ���2013.3.14
�����Ա��
���ʱ���
����ĵ���
�޸ļ�¼�ð汾��    �޸���  �޸ı�ǩ    �޸�ԭ��
          V3.0.0.0          20130314-01 �����汾
                            20131203-01-����һ��ͨ����
                            20131211-02-������ֵ
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

#define MAX_PKG_LEN     2048     /*�շ����ṹ��󳤶�*/
#define PKG_HEAD_LEN_G  17       /*20131203-01-����һ��ͨ����*/

/******************************************************************************
�������ơ�rcvFromIFComm
�������ܡôӷ����̽��ձ���
���������
        iSocketId:Socket���
���������
        sCommBuf��ͨѶ����           
�� �� ֵ��
          > 0  ���յ��ı���
          < 0  ͨѶ�쳣  
******************************************************************************/
long rcvFromIFComm(int iSocketId, char *sCommBuf, int iTimeOut)
{
    char cFlag = '0'; /* �����Ƿ������� */
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
        ERRLOGF("���ձ��ĳ�ʱ,[%s]\n", strerror(errno));
        PrintLog(LEVEL_ERR, LOG_NOTIC_INFO, "���ձ��ĳ�ʱ,[%s]\n", strerror(errno));
        /*��ʱ*/
        return -3;   
    }
    else if( retval == -1 )
    {
        /*��ʱ*/
        return -2;  
    }  
    
    signal(SIGPIPE, SIG_IGN);
        
    while( (cFlag == '0') )
    {
        lRet = recv(iSocketId, sTmp, 1024, 0);  
        if ( lRet < 0 )
        {
            ERRLOGF("���ձ���ʧ��,[%s]\n", strerror(errno));
            PrintLog(LEVEL_ERR, LOG_NOTIC_INFO, "���ձ���ʧ��,[%s]\n", strerror(errno));            
            return -1;   
        }
        else
        {   
            if ( lRet > 0 )
            {                                                                                                 
                memcpy(sCommBuf + lOffset, sTmp, lRet);
                lOffset += lRet; 
                
                /* ����</MsgText>��ʾ�����Ѿ����� */
                if ( strstr(sCommBuf, "</MsgText>") != NULL )
                {
                   cFlag = '1'; 
                } 
                
                PrintLog(LEVEL_DEBUG, LOG_DEBUG_INFO, "����[%s][%c]", sTmp, cFlag );                
            }
            
            iCount++;
            /* ���������⴦�����г�ʱ���ƣ���������(ÿ10��ѭ����2��) */
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
                        ERRLOGF("���ձ��ĳ�ʱ,[%s]\n", strerror(errno));
                        PrintLog(LEVEL_ERR, LOG_NOTIC_INFO, "���ձ��ĳ�ʱ,[%s]\n", strerror(errno));
                        /*��ʱ*/
                        return -3;   
                    }  
                    iCount  = 0;
                }
                else
                {
                    ERRLOGF("���ձ��ĳ�ʱ,��ָ��ʱ����δ���걨��,����[%ld]\n", lOffset);
                    PrintLog(LEVEL_ERR, LOG_NOTIC_INFO, "���ձ��ĳ�ʱ,��ָ��ʱ����δ���걨��,����[%ld]\n", lOffset);
                    /*��ʱ*/
                    return -4;   
                }
            }                      
        }
    }
    PrintLog(LEVEL_DEBUG, LOG_DEBUG_INFO, "%s() Exit", __FUNCTION__);
    return  lOffset;      
}

/******************************************************************************
�������ơ�SendToFutureAndRecv
�������ܡ÷��ͱ��ĸ������̲�����Ӧ��
���������
        sSecuNo  :�����̱��
        sCommBuf :������          
        lCommLen :���ĳ���
���������
        sCommBuf: Ӧ����
        lCommLen: Ӧ���ĳ���
�� �� ֵ��
          = 0  �ɹ�
          < 0  ͨѶ�쳣 
          -99:ͨѶ��ʱ 
******************************************************************************/
long SendToFutureAndRecv(char *sSecuNo, char *sCommBuf, long *lCommLen)
{
    char sCommIP[128] = {0};
    int  iCommPort = 5000;
    int  iTimeOut = 30;
    int  iSocketId;
    long lRet;
    char sTransMode[128] = {0};                              /*20131203-01-����һ��ͨ����*/
    char sBuffer[MAX_PKG_LEN + PKG_HEAD_LEN_G + 1] = {0};    /*20131211-02-������ֵ*/
    
    PrintLog(LEVEL_DEBUG, LOG_DEBUG_INFO, "%s() Entry", __FUNCTION__);
    /* �����̱�� */
    if ( strlen(sSecuNo) == 0 )
    {
         ERRLOGF("ȡ����ʧ��,ԭ��[�����̱��Ϊ��]");
         PrintLog(LEVEL_ERR, LOG_NOTIC_INFO, "ȡ����ʧ��,ԭ��[�����̱��Ϊ��]");
         return -1;
    }
    
    /* IP��ַ */
    if ( INIscanf(sSecuNo, "COMMIP", "%s", sCommIP) < 0 )
    {
         ERRLOGF("ȡ[%s]_[COMMIP]����ʧ��,ԭ��[%s]", sSecuNo, strerror(errno));
         PrintLog(LEVEL_ERR, LOG_NOTIC_INFO, "ȡ[%s]_[COMMIP]����ʧ��,ԭ��[%s]", sSecuNo, strerror(errno));
         return -1;   
    }
            
    /* �˿� */
    if ( INIscanf(sSecuNo, "COMMPORT", "%d", &iCommPort) < 0 )
    {
         ERRLOGF("ȡ[%s]_[COMMIP]����ʧ��,ԭ��[%s]", sSecuNo, strerror(errno));
         PrintLog(LEVEL_ERR, LOG_NOTIC_INFO, "ȡ[%s]_[COMMPORT]����ʧ��,ԭ��[%s]", sSecuNo, strerror(errno));
         return -1;   
    }      
    
    /*  ��ʱʱ�� */
    if ( INIscanf(sSecuNo, "TIMEOUT", "%d", &iTimeOut) < 0 )
    {
         PrintLog(LEVEL_ERR, LOG_NOTIC_INFO, "ȡ[%s]_[TIMEOUT]����ʧ�ܣ�ȡĬ��ֵ:30", sSecuNo);         
    }
    
    /*20131203-01-����һ��ͨ����*/
    if ( INIscanf(sSecuNo, "TRANSMODE", "%s", sTransMode) < 0 )
    {       
         PrintLog(LEVEL_ERR, LOG_NOTIC_INFO, "ȡ[%s]_[TRANSMODE]����ʧ�ܣ�ȡĬ��ֵ:0", sSecuNo);
         sTransMode[0] = '0';
    }
    
    PrintLog(LEVEL_NORMAL, LOG_NOTIC_INFO, "SECUNO[%s],COMMIP[%s],COMMPORT[%d],TIMEOUT[%d]", sSecuNo, sCommIP, iCommPort, iTimeOut);
    
    itcp_setenv(iCommPort, &iSocketId);
    lRet = itcp_calln(sCommIP, 2);
    if ( lRet < 0 )
    {
        ERRLOGF("���ӷ�����{IP[%s],PORT[%d]}ʧ��", sCommIP, iCommPort); 
        PrintLog(LEVEL_ERR, LOG_NOTIC_INFO, "���ӷ�����{IP[%s],PORT[%d]}ʧ��", sCommIP, iCommPort); 
        return -2;
    }
    itcp_setpkghead(1);
    
    if (sTransMode[0] == '1')
    {
        /*20131211-02-������ֵ*/
        sprintf(sBuffer, "G%03d%05ld%8s%s", PKG_HEAD_LEN_G, *lCommLen, sSecuNo, sCommBuf);
        lRet = itcp_send(sBuffer, *lCommLen + PKG_HEAD_LEN_G , 10);
    }
    else
    {
        lRet = itcp_send(sCommBuf, *lCommLen , 10);
    }
    
    if ( lRet < 0 )
    {
        ERRLOGF("���ͱ�����������ʧ��"); 
        PrintLog(LEVEL_ERR, LOG_NOTIC_INFO, "���ͱ�����������ʧ��"); 
        itcp_clear();
        return -2;
    }
    
    /* ��ʼ�� */
    memset(sCommBuf, 0x00, *lCommLen);
    
    *lCommLen = rcvFromIFComm(iSocketId, sCommBuf, iTimeOut);
    itcp_clear();
    if ( *lCommLen < 0 )
    {
        ERRLOGF("���շ�����Ӧ����ʧ��"); 
        PrintLog(LEVEL_ERR, LOG_NOTIC_INFO, "���շ�����Ӧ����ʧ��");       
        return -99;              
    }   
    PrintLog(LEVEL_DEBUG, LOG_DEBUG_INFO, "%s() Exit", __FUNCTION__); 
    return 0;
}

/******************************************************************************
�������ơ�SendToOltpAndRecv
�������ܡ÷��ͱ��ĸ������ܿز�����Ӧ��
���������
        sSecuNo  :�����̱��
        sCommBuf :������          
        lCommLen :���ĳ���
���������
        sCommBuf :������          
        lCommLen :���ĳ���
        sErrMsg  :������Ϣ(�쳣ʱ����)        
�� �� ֵ��
          = 0  �ɹ�
          < 0  ͨѶ�쳣  
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
    /* IP��ַ */
    if ( INIscanf("COMMINFO", "OLTPIP", "%s", sCommIP) < 0 )
    {
         ERRLOGF("ȡ[COMMINFO]_[OLTPIP]����ʧ��,ԭ��[%s]", strerror(errno));
         PrintLog(LEVEL_ERR, LOG_NOTIC_INFO, "ȡ[COMMINFO]_[OLTPIP]����ʧ��,ԭ��[%s]", strerror(errno));
         strcpy(sErrMsg, "1041|ȡͨѶ����OLTPIPʧ��|");
         return -1;   
    }
    
    /* �˿� */
    if ( INIscanf("COMMINFO", "OLTPPORT", "%d", &iCommPort) < 0 )
    {
         ERRLOGF("ȡ[COMMINFO]_[COMMIP]����ʧ��,ԭ��[%s]", strerror(errno));
         PrintLog(LEVEL_ERR, LOG_NOTIC_INFO, "ȡ[COMMINFO]_[OLTPPORT]����ʧ��,ԭ��[%s]", strerror(errno));
         strcpy(sErrMsg, "1041|ȡͨѶ����OLTPPORTʧ��|");
         return -1;   
    }
    
    /*  ��ʱʱ�� */
    if ( INIscanf("COMMINFO", "OLTPTIMEOUT", "%d", &iTimeOut) < 0 )
    {       
         PrintLog(LEVEL_ERR, LOG_NOTIC_INFO, "ȡ[COMMINFO]_[OLTPTIMEOUT]����ʧ�ܣ�ȡĬ��ֵ:30");         
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
        ERRLOGF("���ӽ����ܿ�{IP[%s],PORT[%d]}ʧ��", sCommIP, iCommPort); 
        PrintLog(LEVEL_ERR, LOG_NOTIC_INFO, "���ӽ����ܿ�{IP[%s],PORT[%d]}ʧ��", sCommIP, iCommPort); 
        strcpy(sErrMsg, "9001|��������ǰ�û�ʧ��|");
        return -2;
    }
    
    itcp_setpkghead(1);    
    lRet = itcp_send(sTmpBuf, *lCommLen + 28 , iTimeOut);   
    if ( lRet < 0 )
    {
        ERRLOGF("���ͱ������ܿ�ʧ��"); 
        PrintLog(LEVEL_ERR, LOG_NOTIC_INFO, "���ͱ������ܿ�ʧ��");
        strcpy(sErrMsg, "9002|������ǰ�û����ͱ���ʧ��|"); 
        itcp_clear();
        return -2;
    }
    memset(sTmpBuf, 0x00, sizeof(sTmpBuf));
    lRet = itcp_recv(sTmpBuf, 28 , iTimeOut);   
    if ( lRet < 0 )
    {
        ERRLOGF("�����ܿ�Ӧ���ͷʧ��"); 
        PrintLog(LEVEL_ERR, LOG_NOTIC_INFO, "�����ܿ�Ӧ���ͷʧ��"); 
        itcp_clear();
        return -99;
    }
    
    memcpy(sTmpLen, sTmpBuf + 4, 5);
    iDataLen = atoi(sTmpLen);
    PrintLog(LEVEL_NORMAL, LOG_NOTIC_INFO, "���峤��[%d]", iDataLen );
    
    if ( iDataLen < 0 || iDataLen >  2048 )
    {
        PrintLog(LEVEL_ERR, LOG_NOTIC_INFO, "�������쳣,DataLen[%d],[%s]", iDataLen, sTmpLen); 
        itcp_clear();
        return -99;        
    }
    memset(sCommBuf, 0x00, MAX_PKG_LEN);
    *lCommLen = itcp_recv(sCommBuf, iDataLen , iTimeOut); 
    itcp_clear();  
    if ( *lCommLen < 0 )
    {
        ERRLOGF("�����ܿ�Ӧ�����ʧ��"); 
        PrintLog(LEVEL_ERR, LOG_NOTIC_INFO, "�����ܿ�Ӧ�����ʧ��");         
        return -99;
    }  
    PrintLog(LEVEL_DEBUG, LOG_DEBUG_INFO, "%s() Exit", __FUNCTION__);  
    return 0;      
}
