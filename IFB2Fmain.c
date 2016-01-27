/******************************************************************************
�ļ����ơ�IFB2Fmain.c
ϵͳ���ơ��ڻ���֤����ϵͳ
ģ�����ơ����з���ӿ�ת����������ӿڣ�
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

/*********************************��������************************************/

/**********************************�궨��*************************************/
#define MAX_PKG_LEN     2048     /*�շ����ṹ��󳤶�*/

/*********************************���Ͷ���************************************/

/*******************************�ṹ/���϶���*********************************/

/*******************************�ⲿ��������**********************************/

/*******************************ȫ�ֱ�������**********************************/

int     g_iServerPort;                   /* ���������˿� */
int     g_iServerSock=-1;                /* �������˾�� */
char    g_sSecuNo[DB_SECUNO_LEN+1];      /* �ڻ���˾��� */
char    g_sTransCode[DB_TRSCDE_LEN+1];   /* ���״��� */
char    g_sExSerial[32];                 /* �ⲿ��ˮ��� */
char    g_sPkgHead[PKG_HEAD_LEN + 1];    /* ��ű���ͷ */
char	g_errcode[12];                   /* ������       */
char    g_errmsg[156];                   /* ������Ϣ     */ 
/*******************************����ԭ������**********************************/

void OUT(int);                                        /* ��ֹ������� */
void CLD_OUT(int);                                    /* ��ֹ�ӷ������ */
void cld_out(int a);
void SEGV_OUT(int a);
void showversion(const char *filename);               /* ��ʾ�汾�� */
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
    long lPrepProcNum;                   /* ����������� */
    long lSaveProcNum;                   /* ����������� */
    long lMaxProcNum;                    /* ���������� */
    long lHoldProcNum;                   /* ������������ */
    long lDelay;                         /* ���̼��ʱ���� */
    long lProcLife;                      /* �������� */
    long lCurrentRec;                    /* �����ڽ��̳ص�λ�� */
    long lChildPid;                      /* �ӽ��̽��̺�       */
    long lRet;                           /* ����ֵ             */

    long lLastProc;                      /* ����������� */
    long lLastBusy;                      /* �����æ������ */
    long lCurProc;                       /* ��ǰ�������� */
    long lCurBusy;                       /* ��ǰ��æ������ */
    long lMaxConn;                       /* ����������� */
    int  iLogLevel;                      /* ��־���� */

    long lReqLen,lAnsLen;                /* ����,Ӧ������� */    
    char sReqBuf[MAX_PKG_LEN], sAnsBuf[MAX_PKG_LEN];     /*����,Ӧ���(�ܿ�)*/
    
    char sTmp[200];
    char sBuffer[128];                   /* ����               */
    FIX  *Reqfix = NULL;                 /* ����FIX */
    
    if (getopt(argc, argv, "v") == 'v')
    {
        showversion((const char *)argv[0]);
        return 0;
    }

    /* ��������̸���ֻ������һ�� */
    if (testprg(argv[0]) > 1)
    {
        printf("���ڻ���˾ͨ�Ž����Ѿ�����!\n");
        ERRLOGF( "���ڻ���˾ͨ�Ž����Ѿ�����!\n");
        exit(1);
    }

    /* ������־��ʼ�� */
    SetTransLogName(argv[0]);

    /* ��¼������־: ���̿���ʱ�� */
    warn("���̿���ʱ�䣺%ld\n", GetSysTime());
    TransLog("", 0, "���̿���ʱ��");

    /*�ڻ���˾�ӿ������ļ�*/
    SetIniName("future.ini");
    
    /*��ȡ�����̵ķ���˿�*/     
    g_iServerPort = 5001;        
    INIscanf(argv[0], "PORT", "%d", &g_iServerPort);    
    TransLog("", 0, "������̶˿�PORT��%d", g_iServerPort);
    warn("������̶˿�[g_iServerPort=%ld]\n", g_iServerPort);
    
    
    /*ȡ�����������*/
    lPrepProcNum = 1;
    INIscanf(argv[0], "PREP_PROC", "%ld", &lPrepProcNum);
    TransLog("", 0, "�����������PREP_PROC��%ld", lPrepProcNum);
    warn("�����������[PREP_PROC=%ld]\n", lPrepProcNum);

    /*ȡ�����������*/
    lSaveProcNum = 8;
    INIscanf(argv[0], "SAVE_PROC", "%ld", &lSaveProcNum);
    TransLog("", 0, "�����������SAVE_PROC��%ld", lSaveProcNum);
    warn("�����������[SAVE_PROC=%ld]\n", lSaveProcNum);

    /*ȡ����������*/
    lMaxProcNum = 10;
    INIscanf(argv[0], "MAX_PROC", "%ld", &lMaxProcNum);
    TransLog("", 0, "����������MAX_PROC��%ld", lMaxProcNum);
    warn("����������[MAX_PROC=%ld]\n", lMaxProcNum);

    /*���̼��ʱ����*/
    lDelay = 3;
    INIscanf(argv[0], "MON_POOL_SPACE", "%ld", &lDelay);
    TransLog("", 0, "���̼��ʱ����MON_POOL_SPACE��%ld", lDelay);
    warn("���̼��ʱ����[MON_POOL_SPACE=%ld]\n", lDelay);

    /*��������*/
    lProcLife = 30;
    INIscanf(argv[0], "PROC_LIFE", "%ld", &lProcLife);
    TransLog("", 0, "��������PROC_LIFE��%ld", lProcLife);
    warn("��������[PROC_LIFE=%ld]\n", lProcLife);
    
    /*��־����*/
    iLogLevel = 1;
    INIscanf(argv[0], "LOG_LEVEL", "%d", &iLogLevel);
    SetLogThresHold(iLogLevel);
    TransLog("", 0, "��־����:%d", iLogLevel);
    warn("��־����[LOG_LEVEL=%d]\n", iLogLevel);
    
    /* �����ֵ���Ϣ */
    memset(sTmp, 0x00, sizeof(sTmp));
    sprintf(sTmp, "%s/etc/IFDict.dat", getenv("HOME"));
    SetDictEnv(sTmp);
    
    TransLog("", 0, "�ֵ���Ϣ���ձ��ļ�[%s]", sTmp);
    warn("�ֵ���Ϣ���ձ��ļ�[%s]\n", sTmp);
     
    /*û�б�������*/
    lHoldProcNum = 0;
    
    daemon_start(1);

    new_action_cld.sa_handler = cld_out;/* cld_outΪSIGCHLD�źŴ�������*/
    sigemptyset(&new_action_cld.sa_mask);
    new_action_cld.sa_flags = 0;
    sigaction(SIGCHLD, &new_action_cld, NULL);

    new_action_term.sa_handler = CLD_OUT;/* OUTΪSIG_TERM�źŴ�������*/
    sigemptyset(&new_action_term.sa_mask);
    new_action_term.sa_flags = 0;
    sigaction(SIGTERM, &new_action_term, NULL);

    new_action_pipe.sa_handler = SIG_IGN;/* SIG_PIPE�����źŴ�����*/
    sigemptyset(&new_action_pipe.sa_mask);
    new_action_pipe.sa_flags = 0;
    sigaction(SIGPIPE, &new_action_pipe, NULL);
   
    /* ��ֹ����coredump��������־��¼��ʧ */
    signal(SIGSEGV,SEGV_OUT);
    signal(SIGBUS, SEGV_OUT); 
   
    /*��ʼ�����̳�*/
    sprintf(sBuffer, "%s/tmp/KEY_%s", getenv("HOME"), argv[0]);
    if( (lRet = pool_init(lPrepProcNum, lSaveProcNum, lMaxProcNum, lHoldProcNum, sBuffer)) < 0 )
    {
        ERRLOGF("��ʼ�����̳�[%s]ʧ��", sBuffer);
        TransLog("", 0, "��ʼ�����̳�[%s]ʧ��[%ld][%ld]", sBuffer, lRet,errno );        
        fprintf(stderr, "��ʼ�����̳�[%s]ʧ��\n", sBuffer);
        warn("��ʼ�����̳�[%s]ʧ��\n", sBuffer);
        exit(1);
    }
    TransLog("", 0, "��ʼ�����̳�[%s]�ɹ�", sBuffer);
    warn("��ʼ�����̳�[%s]�ɹ�\n", sBuffer);

    /* ��ʼ�������˿�*/
    itcp_setenv(g_iServerPort, &g_iServerSock);
    
    /* ����������� */
    lMaxConn = 20;
    INIscanf(argv[0], "MAXCONN", "%ld", &lMaxConn);
    itcp_setmaxconn(lMaxConn);
    TransLog("", 0, "�����������MAXCONN��%ld", lMaxConn);
    
    /*�ж϶˿�����*/
    if (itcp_listen() == -1)
    {
        TransLog("", 0, "�����˿�[%ld]ʧ��", g_iServerPort);        
        warn("�����˿�[%ld]ʧ��",g_iServerPort);
        exit(1);
    } 
    
    lLastProc = 0;
    lLastBusy = 0;

    warn("ϵͳ��ʼ������\n");
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
                
                /* ���½�����Ϣ */
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
                /* �����ӽ����˳��Ĵ����� */
                SetTransLogName(argv[0]);                       

                /*һ�����д������*/
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
                    if (lRet < 0) /* ͨ���쳣 */
                    {
                        WriteLog("");
                        itcp_setenv(g_iServerPort,&g_iServerSock);
                        itcp_clear();                        
                        pool_setstatus(0);                        
                        continue;
                    }
                    else if ( lRet > 0 ) /* ���ձ��ĳɹ� */
                    {                                                                                              
                        lRet = requestProcess( Reqfix, sAnsBuf, &lAnsLen );                        
                        FIXclose(Reqfix); 
                        if ( lRet < 0)  /* �����쳣����Ӧ�� */                         
                        {     
                            WriteLog("");                      
                            itcp_setenv(g_iServerPort, &g_iServerSock);
                            itcp_clear();                                                      
                            pool_setstatus(0);                            
                            continue;
                        }                                                                                            
                    }
                       
                    /*����Ӧ������*/
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
�������ơ� rcvFromClient
�������ܡ� ��OLTP���ս�������
��������� ��
��������� 
          sReqBuf    -- �������
          pfix       -- �������� 
          sAnsBuf    -- Ӧ����(�쳣ʱ����)
          lAnsLen    -- Ӧ���ĳ���(�쳣ʱ����)
          
�� �� ֵ��> 0 -- ��������ɹ�, ����ʵ�ʽ��յ����ĳ���
          = 0 -- һ���Դ���
          < 0 -- �����쳣           
******************************************************************************/
long rcvFromClient(char *sReqBuf, char *sAnsBuf,  long *lAnsLen, FIX **pfix)
{        
    char   sErrMsg[255];
    char   sTmpLen[6];    
    char   sFixErrMsg[255];
    long   lPkgOffSet  = 0;           /* ����ƫ���� */
    long   lPkgHeadLen;               /* ����ͷ���� */
    long   lPkgBodyLen;               /* �����峤�� */
    long   lRet;
         
    itcp_setpkghead(1);
    itcp_setenv(g_iServerPort, &g_iServerSock);
    
    /*���ս��������ͷ*/
    if ((lRet = itcp_recv(sReqBuf, PKG_HEAD_LEN, 10)) <= 0)
    {
        itcp_clear();
        ERRLOGF( "���ս��������ͷ����,lRet=[%ld]\n",lRet);
        return -1;
    }

    /* �յ�����־��ӡ */
    PrintLog(LEVEL_DEBUG, LOG_DEBUG_INFO, "���ս��װ�ͷ[%s]", sReqBuf);            
   
    /* ������������� */
    if ( sReqBuf[0] != 'F' )
    {
        ERRLOGF( "������������ʹ���,pkgHead=[%s]\n",sReqBuf);
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
        ERRLOGF( "���������ͷ���ȴ���!\n","");
        return -2; 
    }
                 
    memcpy(g_sPkgHead, sReqBuf, lPkgHeadLen);
    g_sPkgHead[lPkgHeadLen] = 0x00;
    
    memset(g_sSecuNo, 0x00, sizeof(g_sSecuNo));
    memcpy(g_sSecuNo, g_sPkgHead + 9, DB_SECUNO_LEN);     
    
    /* ��������� */
    if (lPkgBodyLen == 0 ||  lPkgBodyLen > 4096)
    {                    
        *lAnsLen = RetError(ERR_FIX_OPEN, "���峤��Ϊ0�򳬳�", sAnsBuf);        
        return -6;
    }
    
    /*��������*/
    lPkgBodyLen = itcp_recv(sReqBuf, lPkgBodyLen, 10);
    if (lPkgBodyLen <= 0)
    {
        itcp_clear();
        *lAnsLen = RetError(ERR_FIX_OPEN, "���հ���ʧ��", sAnsBuf);
        ERRLOGF( "���������װ�����! lRecvLen = [%ld]\n", lPkgBodyLen);
        return -4;
    }
               
    PrintLog(LEVEL_DEBUG, LOG_DEBUG_INFO, sReqBuf);
                     
    /* ����ΪFIX�ṹ  */
    if ((*pfix = FIXopen(sReqBuf, lPkgBodyLen, 2048, sFixErrMsg)) == NULL)
    {
        sprintf(sErrMsg, "FIX���򿪴���2[%s]",sFixErrMsg);
        *lAnsLen = RetError(ERR_FIX_OPEN, sErrMsg, sAnsBuf);        
        return 0;
    }
    FIXgets(*pfix, "FunctionId", g_sTransCode, DB_TRSCDE_LEN + 1);
    FIXgets(*pfix, "sOrganId", g_sSecuNo, DB_SECUNO_LEN + 1);   
    FIXgets(*pfix, "ExSerial", g_sExSerial, sizeof(g_sExSerial) );   
        
    PrintFIXLog(LEVEL_NORMAL, "�յ��ܿ�����", *pfix);   
    
    return lPkgBodyLen;
}

/******************************************************************************
�������ơ�requestProcess
�������ܡ�ת������
���������pfix     -- ������FIX��Ϣ          
���������
          sAnsBuf  -- Ӧ����          
          lAnsLen  -- Ӧ���ĳ���
�� �� ֵ��
          = 0  ��ҪӦ��
          < 0  ����Ӧ��
******************************************************************************/
long requestProcess(FIX  *pfix, char *sAnsBuf, long *lAnsLen)
{
    char sCommBuf[MAX_PKG_LEN]; 
    long lCommLen;
    long lRet;      
    
    /* FIX->XML�������ڻ������ر�FIX��*/
    memset(sCommBuf,0,sizeof(sCommBuf));
    lRet = BankConvFixToXml(pfix, sCommBuf, &lCommLen);
    if(lRet < 0)
    {
        ERRLOGF("[%d][%s][%d]\n", __LINE__, "���������ĸ�ʽת��Ϊ�ڻ���˾���ݸ�ʽʧ��!",lRet);
        PrintLog(LEVEL_ERR,LOG_NOTIC_INFO,"���������ĸ�ʽת��Ϊ�ڻ���˾���ݸ�ʽʧ��[%ld]\n", lRet);
        *lAnsLen = FIXlen(pfix); 
        memcpy(sAnsBuf, FIXBuffer(pfix), *lAnsLen);   
        PrintFIXLog(LEVEL_NORMAL, "�����ܿ�Ӧ��", pfix); 
        return 0;
    }
        
    PrintLog(LEVEL_NORMAL, "���ͷ���������", sCommBuf);
                            
    /* ���������ڻ���˾������Ӧ��*/                       
    lRet = SendToFutureAndRecv(g_sSecuNo, sCommBuf, &lCommLen);                      
    if( lRet < 0)                           
    {   
        /* OLTPͨѶ��ʱ  */
        if ( lRet == -99 )
        {           
            return -1;   
        } 
        else
        {                                                        
            FIXputs(pfix, "ErrorNo",    "5901");
            FIXputs(pfix, "ErrorInfo",  "�������ͨѶʧ��");
            *lAnsLen = FIXlen(pfix); 
            memcpy(sAnsBuf, FIXBuffer(pfix), *lAnsLen); 
            PrintFIXLog(LEVEL_NORMAL, "�����ܿ�Ӧ��", pfix); 
            return 0;
        }
    }                        
    
    PrintLog(LEVEL_NORMAL, "���շ�����Ӧ��", sCommBuf);
                                          
    /* XML->FIX */                                         
    lRet = BankConvXmlToFix(pfix, sCommBuf, lCommLen);
    if (lRet < 0)
    {
        ERRLOGF("[%d][%s][%d]\n", __LINE__, "���ڻ���˾Ӧ�����ݱ�ת��Ϊ������ʽʧ��!",lRet);
        PrintLog(LEVEL_ERR,LOG_NOTIC_INFO,"���ڻ���˾Ӧ�����ݱ�ת��Ϊ������ʽʧ��[%ld]\n", lRet); 
        *lAnsLen = FIXlen(pfix); 
        memcpy(sAnsBuf, FIXBuffer(pfix), *lAnsLen);   
        PrintFIXLog(LEVEL_NORMAL, "�����ܿ�Ӧ��", pfix);                                
        return -2;        
    } 
       
    /* ��ȡFIX���� */
    *lAnsLen = FIXlen(pfix); 
    memcpy(sAnsBuf, FIXBuffer(pfix), *lAnsLen); 
    PrintFIXLog(LEVEL_NORMAL, "�����ܿ�Ӧ��", pfix); 
            
    return 0;
}

/******************************************************************************
�������ơ� SendToClient
�������ܡ� ����Ӧ����ܿ�
��������� 
            sAnsBuf -- Ӧ����
            lAnsLen -- Ӧ���ĳ���
��������� 
          ��
�� �� ֵ�� = 0 ���ͳɹ�
******************************************************************************/
long SendToClient(char *sAnsBuf, long lAnsLen)
{                   
    char  sTmpBuf[MAX_PKG_LEN + PKG_HEAD_LEN + 1];
    long  lRet;
    
    itcp_setpkghead(1);                                        
    itcp_setenv(g_iServerPort,&g_iServerSock);
    
    memset( sTmpBuf, 0x00, sizeof(sTmpBuf));
    snprintf(sTmpBuf, sizeof(sTmpBuf), "F028%05.5ld%-19.19s%s", lAnsLen, g_sPkgHead + 9, sAnsBuf);
    
    PrintLog(LEVEL_DEBUG,"�����ܿ�Ӧ��",sTmpBuf);
                                                      
    /* ���屨�� */                             
    lRet = itcp_send(sTmpBuf, lAnsLen + PKG_HEAD_LEN, 10);
    itcp_clear();
    if (lRet != 0)
    {
        ERRLOGF( "����Ӧ�������! ret = [%ld]\n", lRet);
        TransLog("", 0, "����Ӧ��ʧ��");
        return -1;
    }      
           
    return 0;                  
}

/******************************************************************************
�������ơ�RetError
�������ܡðѴ�����Ϣд��Ӧ�𻺳������ҷ��������ĳ���
���������g_sTransCode -- ������
          errCode   -- �������
          errMsg    -- ������Ϣ
          fixFlag   -- �Ƿ�ʹ��fixЭ��ı�־
���������sAnsBuf    -- Ӧ�𻺳���ָ��
�� �� ֵ�����ĳ���
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
    PrintFIXLog(LEVEL_NORMAL,"�����ܿ�Ӧ��", pfix);
    FIXclose(pfix);
    return lBuflen;    
}


/******************************************************************************
�������ơ�CLD_OUT
�������ܡ��ź�SIGTERM��׽�������ӽ��̣�
�����������
�����������
�� �� ֵ����
******************************************************************************/
void CLD_OUT(int a)
{
    char sBuffer[64];

    sprintf(sBuffer, "�ӽ��̹ر�ʱ�䣺%s", GetSysTime());
    TransLog("", 0, sBuffer);
    WriteLog("");
    
    itcp_setenv(g_iServerPort, &g_iServerSock);
    itcp_clear();

    pool_poolclear();
    exit(0);
}

/******************************************************************************
�������ơ�SEGV_OUT
�������ܡ��ź�SIGSEGV��׽����
�����������
�����������
�� �� ֵ����
******************************************************************************/
void SEGV_OUT(int a)
{        
    TransLog("", 0, "�ӽ���coredump");    
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
        if(WIFEXITED(istatus)) /*������ֹ���ӽ���*/
        {        
            continue;
        }
        
        if(WIFSIGNALED(istatus)) /*�����쳣����ֹ���ӽ��� */
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
        ERRLOGF( "�ӽ���δ֪��ֹ istatus=%d,pid=%d,errno=%d\n",istatus,pid,errno);
    }
    return;
}


/*******************************************
 *����˵�������������Ϣ
 *���������
 *���������
 *����ֵ��  
 *����ʱ�䣺
 *�����ߣ�
 *�޸ļ�¼�� 
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
** �������ơ�showversion
** �������ܡ���ʾ���س���İ汾��
** ���������const char *filename -- �ļ���
** �����������
** �� �� ֵ����
*/
void showversion(const char *filename)
{
    #define SYSTEM_VER "V3.0.0.0"       /* V������ַ����������ݿ�ģ���O-Oracle;I-Informix;S-Sybae:D-DB2 �汾��3.0 + ������ţ���1��ʼ��ţ�+������ţ���1��ʼ��ţ�*/
    fprintf(stderr, "���ڻ���֤����ϵͳ��%-17s�汾�� %-17s���� %s\n", filename, SYSTEM_VER, __DATE__);
    return;
}

/*********************************�ļ�����************************************/

