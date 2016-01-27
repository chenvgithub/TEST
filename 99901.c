/*****************************************************************************
�ļ����ơ�99901c
ϵͳ���ơô��ϵͳ
ģ�����ơ���������
�����Ȩ�ú������ӹɷ����޹�˾
����˵��������ȯ�̶����ļ�
ϵͳ�汾��2.0
������Ա�ô��ϵͳ��Ŀ��
����ʱ���2007-9-1
�����Ա��
���ʱ���
����ĵ���
�޸ļ�¼�ð汾��    �޸���  �޸ı�ǩ    �޸�ԭ��
          V2.0.0.17 ��˼��  20141014-01 ������������������ˮ
          V2.0.0.16 ������  20130916-01 ���¸�ֵ�����ͺ궨��
          V2.0.0.15 ������  20130326-01 ����֤������ת������
          V2.0.0.14 ������  20120817-01 ����ҵ������ж�
                            20120929-01 ��������δ����Ҫ��������
          V2.0.0.13 �ƻ���  20110819-01 ����δ��ʼ��bug
          V2.0.0.12 �ƻ���  20110506-02 ��������Ԥָ����������������
          V2.0.0.11 ������  20091123-01 ��ʼ��sTransCode����
          V2.0.0.11 �ƻ���  20090720-01 ���ӿͻ��ʻ����ͱ䶯�ļ�
                            20090804-01 ��ʼ������������ҵ������
                    ������  20090810-01 �����˻����͵Ĳ�ͬ���õ����˻����ͱ䶯�ļ�������
                    �ƻ���  20090819-01 ���ӹ�̩��ʽԤԼ�������������ļ�
                    �����  20090901-01 BUG�޸�        
          V2.0.0.10 �ƻ���  20090624-01 ����ԤԼ�����ͻ���Ϣ�����ļ�
          V2.0.0.9  ��Ԫ��  20081010-01 ������ȯҵ����ʹ��ͬһȯ�̱��ʱ�����ļ�(����ģʽ)����ҵ������
          V2.0.0.8  ��Ԫ��  20081006-01 ����������ȯ���޸Ļ�ȡȯ��ҵ���ģʽ
          V2.0.0.7  ������  20080708-01 ��֤�˻�״̬�ļ���¼Ψһ������distinct
          V2.0.0.6  ������  20080605-01 �Ż��˻�״̬�ļ���SQL���
                    ������  20080605-02 ��ȡ����ȱʡ��ȯ��Ӫҵ����Ϣ
          V2.0.0.5  ������  20080530-01 ��־�汾��ת����ϸ�ṹ���ʼ��
          V2.0.0.4  ������  20080529-01 �������㹦��ͳһʹ�ó�ʼ������
          V2.0.0.3  ������  20080523-01 �˻�״̬�ļ��б������ɵ���ǩԼ�Ķ�����¼
          V2.0.0.2  ������  20080522-01 ����Ԥָ���ͻ���֤ȯ��Ϊ�����ͻ�
                    ������  20080522-02 ������׼�ӿ��еĳ����־
          V2.0.0.1  ������  20080514-01 ��¼���к�̨�˶�״̬
******************************************************************************/

#include "bss.inc"
#include "99901I.h"

/*********************************��������************************************/


/**********************************�궨��*************************************/
#define    PAGELINES        50
#define    SELECT_NUM       400000
/*********************************���Ͷ���************************************/

/*******************************�ṹ/���϶���*********************************/
typedef struct tagTrans99901
{
    TBSYSARG      *sysarg;                           /* ϵͳ�����ṹ */
    TBTRANS       *trans;                            /* ���ײ��� */
    REQ99901      req;
    ANS99901      ans;
    long          lSerialNo;                         /* ��ˮ�� */
    long          lTransDate;                        /* ������=sysarg.prev_date */
    char          sErrCode[DB_ERRNO_LEN + 1];        /* ������� */
    char          sErrMsg[DB_ERRMSG_LEN + 1];        /* ������Ϣ */
    char          sBankErrMsg[1024];
    char          sSecuNo[DB_SECUNO_LEN + 1];
    char          cNeedModify;                       /* 20120711-01-�����ж��Ƿ��޸��������� */
    
}TRANS99901;

/*20090810-01-�����˻����͵Ĳ�ͬ���õ����˻����ͱ䶯�ļ�������*/
struct CARDSTR
{
    char      cFlag;                   /*ƥ���־ '='��λƥ�� '<'��Χƥ�� */
    char      sString_b[32 + 1];       /*ƥ����ʼ�� */  
    char      sString_e[32 + 1];       /*ƥ������� */ 
};
struct CARDPARSE
{
    long      lNum;                   /*ƥ�䴮��������*/
    struct CARDSTR  cardstr[32+1];
};

/*******************************�ⲿ��������**********************************/

/*******************************ȫ�ֱ�������**********************************/
char sSecuDir[128];
char sLogName[256];

/*******************************����ԭ������**********************************/
static void PackAnswer(TRANS99901 *pub, char *answer, long *lAnsLen);
static long UnPackRequest(TRANS99901 *pub, char *request, long lReqLen, INITPARAMENT *initPara);
static long Get1ViaBKind(char *psBType, char *gtBType);
static long Get2ViaBKind(char *psBType, char *gtBType);
static long Get1TransViaTCode(char *psTCode, char *gtTSource, char *gtTCode);
static long Get2TransViaTCode(char *psTCode, char *gtTSource, char *gtTCode);
static long Get1ViaIdKind(char *psIdType, char *gtIdType);
static long ToLocal(TRANS99901 *pub, INITPARAMENT *initPara);
static long ToSecu(TRANS99901 *pub);
static long ToShell(TRANS99901 *pub);
static void TransToSecu(TRANS99901 *pub, char *sBankReq, long *lReqLen);
static void TransToMonitor(TRANS99901 *pub, TBMONITOR *monitor);
static long SetSecuClearStatus(char *psSecuNo, char cStatus);
static long ParseParam(char *value, struct CARDPARSE *cardparse);
static long CheckAcctType(char *sCardNo, struct CARDPARSE *cardparse);
static long GetNumViaFlag(char *sString, char cFlag);

/******************************************************************************
��������: main99901
��������: ����ȯ�̶����ļ�
���������request - �����
���������answer  - Ӧ���
�� �� ֵ����
˵    ����ע��Flag��Ӧ��
******************************************************************************/
void main99901(const char *request, long lReqLen, char *answer, long *lAnsLen, INITPARAMENT *initPara)
{
    TRANS99901 trans99901;
    TRANS99901 *pub;

    pub = &trans99901;
    memset(pub, 0x00, sizeof(TRANS99901));

    /* ��� */
    if (UnPackRequest(pub, (char *)request, lReqLen, initPara) != 0)
    {
        goto _RETURN;
    }

    SET_ERROR_CODE(ERR_DEFAULT);    /* ��ֹ��©���ô����� */
/*
    if (ToLocal(pub,initPara) != 0)
    {
        goto _RETURN;
    }
    

    if (ToShell(pub) != 0)
    {
        goto _RETURN;
    } 
*/
    /* ����֤ȯ�����ж����ļ�����֪ͨ */
    if (ToSecu(pub) != 0)
    {
        goto _RETURN;
    } 
    
    SET_ERROR_CODE(ERR_SUCCESS);

_RETURN:
    /* ����Ӧ��� */
    PackAnswer(pub, answer, lAnsLen);

    TransToMonitor(pub, &(initPara->monitor));

    return;
}

/******************************************************************************
�������ơ�UnPackRequest
�������ܡö����뻺�������ݽ�������׵��ܽṹ��
���������request  -- ���뻺����ָ��
          initPara -- ϵͳ�����ṹָ��
���������pub     -- ���׵��ܽṹ
�� �� ֵ����
******************************************************************************/
static long UnPackRequest(TRANS99901 *pub, char *request, long lReqLen, INITPARAMENT *initPara)
{
    char sBuff[255];

    memset((char *)pub, 0x00, sizeof(TRANS99901));

    PrintLog(LEVEL_NORMAL, LOG_RECV_BANK_REQ, request);

    pub->sysarg = &(initPara->sysarg);
    pub->trans  = &(initPara->trans);
    pub->lSerialNo = initPara->lSerialNo;
    pub->lTransDate = pub->sysarg->prev_date;

    if (MFS2Struct(&(pub->req), _REQ99901, request, sBuff) != 0)
    {
        PrintLog(LEVEL_ERR, LOG_ERROR_INFO, "�����%s", sBuff);
        SET_ERROR_CODE(ERR_PACKAGE);
        return -1;
    }

    return 0;
}

/******************************************************************************
�������ơ�PackAnswer
�������ܡ�����Ӧ���
���������pub      -- ���׽ṹ
���������answer   -- Ӧ�𻺳���
          lAnsLen   -- ���ذ����ĳ���
�� �� ֵ��
******************************************************************************/
static void PackAnswer(TRANS99901 *pub, char *answer, long *lAnsLen)
{
    char sBuff[1088];
    long ret;
    ANS99901  *ans;

    ans = &(pub->ans);

    if (strcmp(pub->sErrCode, ERR_SUCCESS) == 0)
    {
        memcpy(ans->FunctionId, (pub->req).FunctionId, sizeof(ans->FunctionId)-1);
        memcpy(ans->ExSerial, (pub->req).ExSerial, sizeof(ans->ExSerial)-1);
        memcpy(ans->ErrorNo, pub->sErrCode, sizeof(ans->ErrorNo)-1);
        
        if (strlen(alltrim(pub->sBankErrMsg)) > 0)
        {
            memset(sBuff, 0x00, sizeof(sBuff));
            sprintf(sBuff, "�����ļ��ɹ�!�Զ�������������ȯ���ļ�֪ͨʧ��:%s", pub->sBankErrMsg);
            memcpy(pub->sErrMsg, sBuff, sizeof(pub->sErrMsg) -1);
        }
        
        if (strlen(alltrim(pub->sErrMsg)) == 0)
        {
            strcpy(ans->ErrorInfo, "���׳ɹ�");
        }
        else
        {
            memcpy(ans->ErrorInfo, pub->sErrMsg, sizeof(ans->ErrorInfo)-1);
        }

        if (Struct2MFS(answer, (char *)ans, _ANS99901, sBuff) != 0)
        {
            SET_ERROR_CODE(ERR_PACKAGE);
            PrintLog(LEVEL_ERR, LOG_ERROR_INFO, "�����%s", sBuff);
        }
    }
    else
    {
        GetErrorMsg(pub->sErrCode, pub->sErrMsg);
        sprintf(answer, "%s|%s|%s|%s|", pub->req.FunctionId, pub->req.ExSerial, pub->sErrCode, pub->sErrMsg);
    }
    
    *lAnsLen = strlen(answer);

    /* ����־ */
    PrintLog(LEVEL_NORMAL, LOG_SEND_BANK_ANS, answer);

    return;
}

/******************************************************************************
�������ơ�Gen_SecuFile
�������ܡð�ȯ������ȯ����Ҫ���ļ�
�����������
�����������
�� �� ֵ�� 0 -- �ɹ�
          <0 -- ʧ��
******************************************************************************/
static long ToLocal(TRANS99901 *pub, INITPARAMENT *initPara)
{
    char    **psResult;
    char    sSql[SQL_BUF_LEN], sFlag[2], cFileFlag;
    long    lCount = 0, lCnt = 0;
    char    sSecuNo[DB_SECUNO_LEN+1], sBusinType[2];
    long    i, lRet;
    TBPARAM     param;
    TBSECUZJCG  secuzjcg;
    TBSECUZJCG  *szjcg;

    SetIniName("rzcl.ini");
    
    /*ȡȯ��Ŀ¼*/
    memset(sSecuDir, 0x00, sizeof(sSecuDir));
    if (GetCfgItem("PUBLIC", "SECUDIR", sSecuDir) != 0)
    {
        sprintf(sSecuDir, "%s", getenv("HOME"));
    }
    else
    {
        /*ȡ�����һλ'/'*/
        if (sSecuDir[strlen(sSecuDir)-1] == '/')
            sSecuDir[strlen(sSecuDir)-1] = '\0';
    }
        
    /*����־����*/
    memset(sLogName, 0x00, sizeof(sLogName));
    if ( strlen(alltrim((pub->req).sOrganId)) > 0)
        sprintf(sLogName, "%s/%s/%spr%s.%ld", sSecuDir, (pub->req).sOrganId, SENDFILE_DIR, pub->req.FunctionId, pub->lTransDate);
    else
        sprintf(sLogName, "%s/log/pr%s.%ld", getenv("HOME"), pub->req.FunctionId, pub->lTransDate);
    LogTail(sLogName, "������ȯ�̶����ļ���ʼ...", "");
      
    LogTail(sLogName, "  ���Ϸ��Լ�鿪ʼ...", "");
    /* ����ϵͳ���б�־:0��������1��׼�����У�8�����гɹ���9������ʧ�� */
    memset(sFlag, 0x00, sizeof(sFlag));
    if (GetCfgItem("HSRZCL", "SYSDATE_FLAG", sFlag) != 0)
    {
        SET_ERROR_CODE(ERR_GETTURNUPFLAG);
        LogTail(sLogName, "  ��ϵͳȡ���ڷ��Ʊ�־��", "");
        return -1;
    }
    if (sFlag[0] != '8')
    {
        SET_ERROR_CODE(ERR_NOTURNUP);
        LogTail(sLogName, "  ��ϵͳ��δ���ڷ��ƴ�", "");
        return -2;
    }

    /* �������к˶Ա�׼(��ѡ):0��������1��׼���˶ԣ�8���˶Գɹ���9���˶�ʧ��*/
    memset(sFlag, 0x00, sizeof(sFlag));
    if (GetCfgItem("HSRZCL", "BZZCHCK_FLAG", sFlag) == 0)
    {
        if (sFlag[0] != '8')
        {
            SET_ERROR_CODE(ERR_CHECKDETAIL);
            SET_ERROR_MSG("ϵͳ��δ���������ϸ�˶�");
            LogTail(sLogName, "  ��ϵͳ��δ���������ϸ�˶ԣ�", "");
            return -2;
        }
    }
    LogTail(sLogName, "  ���Ϸ��Լ�������", "");

    /* 20120929-01-�������������� */
    memset(&param, 0x00, sizeof(TBPARAM));
    if (DBGetParam(DEFAULTSECUNO, "CUSTOM_MOPEN_DATE", &param) < 0)
    {
        strcpy(param.param_value,"1");
    }
    pub->cNeedModify = param.param_value[0];

    memset(sBusinType, 0x00, sizeof(sBusinType));
    memcpy(sBusinType, (pub->req).BusinType, 1);
    sprintf(sSql,"select distinct secu_no from tbsecuzjcg where (status='%c' or status='%c')", SECU_STATUS_WCLEAR, SECU_STATUS_END);
    if ( strlen((pub->req).sOrganId) > 0)
        sprintf(sSql,"%s and secu_no ='%s'", sSql,(pub->req).sOrganId);
    if ( strlen(sBusinType) > 0)
        sprintf(sSql,"%s and busin_type='%s'", sSql, sBusinType);
    lCount = DBExecSelect(1, 0, sSql,&psResult);
    if ( lCount < 0)
    {
        SET_ERROR_CODE(ERR_DATABASE);
        LogTail(sLogName, "  ��ȡȯ���б��", "");
        return -3;
    }

    if ( lCount == 0 && strlen((pub->req).sOrganId) > 0)
    {
       SET_ERROR_CODE(ERR_SECUSTATUS);
       LogTail(sLogName, "  ��ȯ��״̬��������", "");
       return -3;
    }
    
    LogTail(sLogName, "  �������ļ���ʼ...", "");
    for(lCnt = 0; lCnt < lCount; lCnt ++)
    {
        memset(sSecuNo, 0x00, sizeof(sSecuNo));
        memcpy(sSecuNo, (char *)GetSubStr(psResult[lCnt], 0), DB_SECUNO_LEN);
        
        /*�����ļ���ʽ��־��0-������ʽ(Ĭ��) 1-��׼��ʽ 2-��̩��ʽ*/
        cFileFlag = '0';
        if (DBGetParam(sSecuNo, "FILE_FLAG", &param) == 0)
        {
            cFileFlag = param.param_value[0];
        }
        if (cFileFlag == '0')  /*0-������ʽ(Ĭ��)*/
        {
            ERRLOGF("��ʼ������֤ת����ϸ�ļ�%s","");
            LogTail(sLogName, "    ��������ʽ��%s����֤ת����ϸ�ļ���ʼ...", sSecuNo);
            /* ������֤ת����ϸ�ļ� */
            if (Gen_DetailFile(pub,initPara, sSecuNo, sBusinType, 0) != 0)
            {
               DBFreePArray2(&psResult, lCount);
               LogTail(sLogName, "    ������[%s]����֤ת����ϸ�ļ�ʧ�ܣ�", sSecuNo);
               return -4;
            }
            ERRLOGF("����������֤ת����ϸ�ļ�%s","");
            LogTail(sLogName, "    ��������ʽ��%s����֤ת����ϸ�ļ�������", sSecuNo);
           
            LogTail(sLogName, "    ��������ʽ��%s������ʻ��䶯��ϸ�ļ���ʼ...", sSecuNo);
            /* ���ɴ���ʻ��䶯��ϸ */
            if ( Gen_DetailFile(pub, initPara, sSecuNo, sBusinType, 1) != 0)
            {
               DBFreePArray2(&psResult, lCount);
               LogTail(sLogName, "    ������[%s]�Ĵ���ʻ��䶯��ϸ�ļ�ʧ�ܣ�", sSecuNo);
               return -5;
            }
            ERRLOGF("�������ɴ���ʻ��䶯��ϸ%s","");
            LogTail(sLogName, "    ��������ʽ��%s������ʻ��䶯��ϸ�ļ�������", sSecuNo);
            
            LogTail(sLogName, "    ��������ʽ��%s���ͻ��ʻ�״̬�ļ���ʼ...", sSecuNo);
            /* ���ɿͻ���Ϣ״̬�ļ� */
            if ( Gen_DetailFile(pub, initPara, sSecuNo, sBusinType, 2) != 0)
            {
               DBFreePArray2(&psResult, lCount);
               LogTail(sLogName, "    ������[%s]���ʻ�״̬�ļ�ʧ�ܣ�", sSecuNo);
               return -6;
            }
            ERRLOGF("�������ɿͻ���Ϣ״̬�ļ�%s","");
            LogTail(sLogName, "    ��������ʽ��%s���ͻ��ʻ�״̬�ļ�������", sSecuNo);
            
            /* LABEL : 20090624-01-����ԤԼ�����ͻ���Ϣ�����ļ�
             * MODIFY: huanghl@hundsun.com
             * REASON: ����ԤԼ�����ͼ����
             * HANDLE: ����ԤԼ�����ͻ���Ϣ�ļ�bprecMMDD.dat
             */   
            LogTail(sLogName, "    ��������ʽ��%s��ԤԼ�����ͻ���Ϣ�ļ���ʼ...", sSecuNo);
            /* ����ԤԼ�����ͻ���Ϣ�ļ� */
            if ( Gen_DetailFile(pub, initPara, sSecuNo, sBusinType, 3) != 0)
            {
               DBFreePArray2(&psResult, lCount);
               LogTail(sLogName, "    ������[%s]��ԤԼ�����ͻ���Ϣ�ļ�ʧ�ܣ�", sSecuNo);
               return -7;
            }
            ERRLOGF("��������ԤԼ�����ͻ���Ϣ�ļ�%s","");
            LogTail(sLogName, "    ��������ʽ��%s��ԤԼ�����ͻ���Ϣ�ļ�������", sSecuNo);
            /* LABEL : 20090720-01-���ӿͻ��ʻ����ͱ䶯�ļ�
             * MODIFY: huanghl@hundsun.com
             * REASON: ���ӿͻ��ʻ����ͱ䶯�ļ�
             * HANDLE: ���ӿͻ��ʻ����ͱ䶯�ļ�bacctMMDD.dat
             */   
            LogTail(sLogName, "    ��������ʽ��%s���ͻ��ʻ����ͱ䶯�ļ���ʼ...", sSecuNo);
            /* ���ɿͻ��ʻ����ͱ䶯�ļ� */
            if ( Gen_DetailFile(pub, initPara, sSecuNo, sBusinType, 4) != 0)
            {
               DBFreePArray2(&psResult, lCount);
               LogTail(sLogName, "    ������[%s]�Ŀͻ��ʻ����ͱ䶯�ļ�ʧ�ܣ�", sSecuNo);
               return -7;
            }
            ERRLOGF("�������ɿͻ��ʻ����ͱ䶯�ļ�%s","");
            LogTail(sLogName, "    ��������ʽ��%s���ͻ��ʻ����ͱ䶯�ļ�������", sSecuNo);
        }
        else if (cFileFlag == '1')  /*1-��׼��ʽ*/
        {
            LogTail(sLogName, "    ����׼��ʽ��%s����֤ת����ϸ�ļ���ʼ...", sSecuNo);
            /* ������֤ת����ϸ�ļ� */
            if (Gen_DetailFile_1(pub,initPara, sSecuNo, sBusinType, 0) != 0)
            {
               DBFreePArray2(&psResult, lCount);
               LogTail(sLogName, "    ������[%s]����֤ת����ϸ�ļ�ʧ�ܣ�", sSecuNo);
               return -4;
            }
            LogTail(sLogName, "    ����׼��ʽ��%s����֤ת����ϸ�ļ�������", sSecuNo);
           
            LogTail(sLogName, "    ����׼��ʽ��%s������ʻ��䶯��ϸ�ļ���ʼ...", sSecuNo);
            /* �����ʻ��䶯��ϸ */
            if (Gen_DetailFile_1(pub, initPara, sSecuNo, sBusinType, 1) != 0)
            {
               DBFreePArray2(&psResult, lCount);
               LogTail(sLogName, "    ������[%s]�Ĵ���ʻ��䶯��ϸ�ļ�ʧ�ܣ�", sSecuNo);
               return -5;
            }
            LogTail(sLogName, "    ����׼��ʽ��%s������ʻ��䶯��ϸ�ļ�������", sSecuNo);
            
            LogTail(sLogName, "    ����׼��ʽ��%s���ͻ��ʻ�״̬�ļ���ʼ...", sSecuNo);
            /* �����ʻ�״̬��ϸ */
            if (Gen_DetailFile_1(pub, initPara, sSecuNo, sBusinType, 2) != 0)
            {
               DBFreePArray2(&psResult, lCount);
               LogTail(sLogName, "    ������[%s]���ʻ�״̬�ļ�ʧ�ܣ�", sSecuNo);
               return -6;
            }
            LogTail(sLogName, "    ��������ʽ��%s���ͻ��ʻ�״̬�ļ�������", sSecuNo);
        }
        else if (cFileFlag == '2')  /*2-��̩��ʽ*/
        {
            LogTail(sLogName, "    ����̩��ʽ��%s����֤ת����ϸ�ļ���ʼ...", sSecuNo);          
            /* ������֤ת����ϸ�ļ� */
            if (Gen_DetailFile_2(pub,initPara, sSecuNo, sBusinType, 0) != 0)
            {
               DBFreePArray2(&psResult, lCount);
               LogTail(sLogName, "    ������[%s]����֤ת����ϸ�ļ�ʧ�ܣ�", sSecuNo);
               return -4;
            }
            LogTail(sLogName, "    ����̩��ʽ��%s����֤ת����ϸ�ļ�������", sSecuNo);
           
            LogTail(sLogName, "    ����̩��ʽ��%s������ʻ��䶯��ϸ�ļ���ʼ...", sSecuNo); 
            /* �����ʻ��䶯��ϸ */
            if (Gen_DetailFile_2(pub, initPara, sSecuNo, sBusinType, 1) != 0)
            {
               DBFreePArray2(&psResult, lCount);
               LogTail(sLogName, "    ������[%s]�Ĵ���ʻ��䶯��ϸ�ļ�ʧ�ܣ�", sSecuNo);
               return -5;
            }
            LogTail(sLogName, "    ����̩��ʽ��%s������ʻ��䶯��ϸ�ļ�������", sSecuNo);

            LogTail(sLogName, "    ����̩��ʽ��%s���ͻ��ʻ�״̬�ļ���ʼ...", sSecuNo);
            /* �����ʻ�״̬��ϸ */
            if (Gen_DetailFile_2(pub, initPara, sSecuNo, sBusinType, 2) != 0)
            {
               DBFreePArray2(&psResult, lCount);
               LogTail(sLogName, "    ������[%s]���ʻ�״̬�ļ�ʧ�ܣ�", sSecuNo);
               return -6;
            }
            LogTail(sLogName, "    ����̩��ʽ��%s���ͻ��ʻ�״̬�ļ�������", sSecuNo);
            
            /* LABEL : 20090819-01-���ӹ�̩��ʽԤԼ�������������ļ�
             * MODIFY: huanghl@hundsun.com
             * REASON: ����ԤԼ�������������ļ�����
             * HANDLE: ���ӹ�̩��ʽԤԼ�������������ļ�����
             */
            LogTail(sLogName, "    ����̩��ʽ��%s��ԤԼ�����ͻ���Ϣ�����ļ���ʼ...", sSecuNo);
            /* ����ԤԼ������ϸ */
            if (Gen_DetailFile_2(pub, initPara, sSecuNo, sBusinType, 5) != 0)
            {
               DBFreePArray2(&psResult, lCount);
               LogTail(sLogName, "    ������[%s]��ԤԼ�����ͻ���Ϣ�����ļ�ʧ�ܣ�", sSecuNo);
               return -7;
            }
            LogTail(sLogName, "    ����̩��ʽ��%s��ԤԼ�����ͻ���Ϣ�����ļ�������", sSecuNo);
            
            LogTail(sLogName, "    ����̩��ʽ��%s���ͻ��ʻ����ͱ䶯�ļ���ʼ...", sSecuNo);
            /* �����������ͻ���ϸ */
            if (Gen_DetailFile_2(pub, initPara, sSecuNo, sBusinType, 6) != 0)
            {
               DBFreePArray2(&psResult, lCount);
               LogTail(sLogName, "    ������[%s]�Ŀͻ��ʻ����ͱ䶯�ļ�ʧ�ܣ�", sSecuNo);
               return -8;
            }
            LogTail(sLogName, "    ����̩��ʽ��%s���ͻ��ʻ����ͱ䶯�ļ�������", sSecuNo);
        }
        else
        {
            LogTail(sLogName, "    �������ļ���ʽ[%c]", cFileFlag);   
        }
    }
    LogTail(sLogName, "  �������ļ�������", "");
    DBFreePArray2(&psResult, lCount);
    
    /*20080514-01-��¼���к�̨�˶�״̬*/
    if ( strlen(alltrim(pub->req.sOrganId)) > 0)
    {
        memset(&secuzjcg, 0x00, sizeof(TBSECUZJCG));
        
        /* LABEL : 20081006-01-����������ȯ���޸Ļ�ȡȯ��ҵ���ģʽ 
         * MODIFY: ��Ԫ��
         * REASON: �����ˡ�������ȯ��ҵ�����
         * HANDLE: ԭ����ȡȯ��ҵ�����Ϣ��ʱ���޶���ҵ������á���ܡ����ָĳ�ȡ��ȫ�������ж�
         */        
        /*
        DBGetSecuzjcg(pub->sSecuNo, BUSIN_YZCG, &secuzjcg);
        */        
        /* Ϊ������ȯ���� */
        /*lRet = DBGetSecuzjcgAll(pub->req.sOrganId, &szjcg);
        if(lRet <= 0)
        {
            SET_ERROR_CODE(ERR_GETSECUINFO);
            return -1;              
        }   
        for(i=0; i<lRet; i++)
        {
            if((szjcg[i].busin_type[0] == BUSIN_YZCG) || (szjcg[i].busin_type[0] == BUSIN_RZRQ))
            {
                memcpy(&secuzjcg, &szjcg[i], sizeof(TBSECUZJCG));
                break;      
            }       
        }
        
        if(lRet > 0)
        {
            free(szjcg);
        }*/
        
    /*20120817-01-����ҵ�����Ƚ�-begin*/
    /* ȡת��ǩԼ�����������ҵ����� */
    memset(&param, 0x00, sizeof(param));
    DBGetParam(DEFAULTSECUNO,"TRUSTALLOW_BUSINTYPE", &param);
        
    if ( strlen(alltrim(param.param_value)) == 0)
        strcpy(param.param_value,"12");
        
    lRet = DBGetSecuzjcgFirst(pub->req.sOrganId, &secuzjcg);
    if(lRet < 0)
    {
        SET_ERROR_CODE(ERR_GETSECUINFO);
        return -1;              
    }
    
        /* ҵ�����Ƿ� */    
    if (strchr(param.param_value, secuzjcg.busin_type[0]) == NULL)
    {
        SET_ERROR_CODE(ERR_INVAILD_BUSINTYPE);
        return -1;
    }
    /*20120817-01-����ҵ�����Ƚ�-end*/
        
        /* ���ӽ��� */
        if ((secuzjcg.clear_status[0] == SECU_CLRSTATUS_WCLEAR) ||
            (secuzjcg.clear_status[0] == SECU_CLRSTATUS_EBZZCHK))
        {
            SetSecuClearStatus(pub->req.sOrganId, SECU_CLRSTATUS_ESFILE);
        }
    }
    
    LogTail(sLogName, "������ȯ�̶����ļ�������", "");
    
    return 0;
}

/******************************************************************************
�������ơ�ToShell
�������ܡ�SHELL�ű����ã�20070322-01-SHELL�ű�����
���������pub -- ���ײ���
�����������
�� �� ֵ��==0 -- �ɹ�
          -1  -- �����֤ȯ�˵�ʧ��
          -2  -- ��������ж˵�ʧ��
******************************************************************************/
static long ToShell(TRANS99901 *pub)
{
    char    sSql[SQL_BUF_LEN],sBuff[128], sCmd[128];
    char    **psResult;
    long    lCount, lRet, lCnt;
    TBPARAM param;

    sprintf(sSql,"select distinct secu_no from tbsecuzjcg where (status='%c' or status='%c')", SECU_STATUS_WCLEAR, SECU_STATUS_END);
    if ( strlen((pub->req).sOrganId) > 0)
        sprintf(sSql,"%s and secu_no ='%s'", sSql,(pub->req).sOrganId);
    lCount = DBExecSelect(1, 0, sSql,&psResult);
    if ( lCount < 0)
    {
        SET_ERROR_CODE(ERR_SUCCESS);
        SET_ERROR_MSG("�����ļ����׳ɹ�!�Զ���������ȯ���ļ�֪ͨʧ��");
        return -1;
    }

    if ( lCount == 0 && strlen((pub->req).sOrganId) > 0)
    {
        SET_ERROR_CODE(ERR_SUCCESS);
        SET_ERROR_MSG("�����ļ����׳ɹ�!ȯ��״̬������");
        return -2;
    }

    for(lCnt = 0; lCnt < lCount; lCnt ++)
    {
        memset(pub->sSecuNo, 0x00, sizeof(pub->sSecuNo));
        memcpy(pub->sSecuNo, (char *)GetSubStr(psResult[lCnt], 0), DB_SECUNO_LEN);
        
        /*��ȡ��֤ͨ�ļ�����ģʽ��ȯ��*/
        /*�����ļ�������ʽ:0-����ͨѶ������(Ĭ��) 1-FTP��ʽ 2-��֤ͨģʽ(��ʽ:2-8λ��������-������(0-δ���� 1-���))*/
        memset(&param, 0x00, sizeof(TBPARAM));
        if (DBGetParam(pub->sSecuNo, "SWAP_FILE", &param) < 0)
            continue;
        /*1-FTP��ʽ��2-��֤ͨģʽ����FTP�ϴ��ļ�*/
        else if (param.param_value[0] == '0') 
            continue;
        
        memset(sBuff, 0x00, sizeof(sBuff));
        if (GetCfgItem("HSRZCL", "UPFILE_BIN", sBuff) != 0)
            continue;
            
        memset(sCmd, 0x00, sizeof(sCmd));
        sprintf(sCmd, "%s %s DZMX %04ld %ld 1>/dev/null 2>/dev/null", sBuff, pub->sSecuNo, pub->lTransDate%10000, pub->lTransDate);
        lRet = system(sCmd);
        PrintLog(LEVEL_NORMAL,LOG_DEBUG_INFO, "�ļ�����FTP�ű�[%s]", sCmd);
    }
    DBFreePArray2(&psResult, lCount);

    return 0;
}

/******************************************************************************
�������ơ�ToSecu
�������ܡú�֤ȯͨ��
���������pub -- ���ײ���
�����������
�� �� ֵ��==0 -- �ɹ�
          -1  -- �����֤ȯ�˵�ʧ��
          -2  -- ��������ж˵�ʧ��
******************************************************************************/
static long ToSecu(TRANS99901 *pub)
{
    char    sBankReq[REQUEST_LEN];           /* �������󻺳��� */
    char    sSecuAns[REQUEST_LEN];           /* ֤ȯӦ�𻺳��� */
    char    sSql[SQL_BUF_LEN];
    char    sBuffer[256],sBuff[32];
    char    **psResult;
    long    lReqLen, lAnsLen, lCount, lRet, lCnt;
    FIX     *pfix;  
    TBTRANS    secutrans;
    TBSECUZJCG secuzjcg;
    TBSECUINFO secuinfo;
    YZPTKEYS   keys;                             /* ��Կ�ṹ */    
/*
    sprintf(sSql,"select distinct secu_no from tbsecuzjcg where (status='%c' or status='%c')", SECU_STATUS_WCLEAR, SECU_STATUS_END);
    if ( strlen((pub->req).sOrganId) > 0)
        sprintf(sSql,"%s and secu_no ='%s'", sSql,(pub->req).sOrganId);
    lCount = DBExecSelect(1, 0, sSql,&psResult);
    if ( lCount < 0)
    {
        SET_ERROR_CODE(ERR_SUCCESS);
        SET_ERROR_MSG("�����ļ����׳ɹ�!�Զ���������ȯ���ļ�֪ͨʧ��");

        return -1;
    }

    if ( lCount == 0 && strlen((pub->req).sOrganId) > 0)
    {
        SET_ERROR_CODE(ERR_SUCCESS);
        SET_ERROR_MSG("�����ļ����׳ɹ�!ȯ��״̬������");

       return -2;
    }
*/
 lCount = 1;
    memset(pub->sBankErrMsg, 0x00, sizeof(pub->sBankErrMsg));
    for(lCnt = 0; lCnt < lCount; lCnt ++)
    {
        memset(pub->sSecuNo, 0x00, sizeof(pub->sSecuNo));
        memcpy(pub->sSecuNo, (char *)GetSubStr(psResult[lCnt], 0), DB_SECUNO_LEN);
        
        if (DBGetTrans(pub->sSecuNo, "23903", &(secutrans)) != 0)
        {
            continue;
        }
        /* ���赽֤ȯ���˳� */
        if (secutrans.secu_online[0] == TRANS_TOSECU_NO )
        {
            continue;
        }
        
        /*���ȯ����Ϣ*/
        lRet = DBGetSecuinfo(pub->sSecuNo, &secuinfo); 
        if (lRet < 0)
        {
            continue;
        }

        /* LABEL : 20081006-01-����������ȯ���޸Ļ�ȡȯ��ҵ���ģʽ 
         * MODIFY: ��Ԫ��
         * REASON: �����ˡ�������ȯ��ҵ�����
         * HANDLE: ��ȡ����ҵ����𡰴�ܡ�ȯ�̺���ȡҵ�����������ȯ��ȯ��
         */        
        memset(&secuzjcg, 0x00, sizeof(TBSECUZJCG));
        if (DBGetSecuzjcg(pub->sSecuNo, BUSIN_YZCG, &secuzjcg) < 0)
        {
            if (DBGetSecuzjcg(pub->sSecuNo, BUSIN_RZRQ, &secuzjcg) < 0)     /*LABEL : 20081006-01-����*/
            {               
                if (DBGetSecuzjcg(pub->sSecuNo, BUSIN_YZZZ, &secuzjcg) < 0)
                {
                    memset(sBuff, 0x00, sizeof(sBuff));
                    sprintf(sBuff, "%s-", pub->sSecuNo);
                    /*��ֹpub->sBankErrMsg���*/
                    if (strlen(pub->sBankErrMsg) + strlen(sBuff) < sizeof(pub->sBankErrMsg))
                        strcat(pub->sBankErrMsg, sBuff);
                
                    continue;
                }
            }
        }
        
        /* ���ȯ����Կ���Ľṹ */
        GetSecuZjcgKeys(&secuinfo, &keys);
        
        /* תΪ֤ȯFIX�ṹ */
        TransToSecu(pub, sBankReq, &lReqLen);

        /* ��֤ȯ��ͨѶ ,����  -2ERR_TIMEOUT, -6ERR_RECVPKG ��ʾ����֤ȯӦ��ʧ��,��ʱ��Ҫ��֤ȯ*/
        lRet = TransWithSecu(&secuinfo, &keys, &secuzjcg, sBankReq, &lReqLen, sSecuAns, &lAnsLen);
        if (lRet != 0)
        {
            PrintLog(LEVEL_ERR, LOG_ERROR_INFO,  "����ȯ��[%s]ȡ�ļ�����ͨѶʧ��!", pub->sSecuNo);
            memset(sBuff, 0x00, sizeof(sBuff));
            sprintf(sBuff, "%s-", pub->sSecuNo);
            /*��ֹpub->sBankErrMsg���*/
            if (strlen(pub->sBankErrMsg) + strlen(sBuff) < sizeof(pub->sBankErrMsg))
                strcat(pub->sBankErrMsg, sBuff);
            
            continue;
        }
        
        /* ����֤ȯӦ�� */
        if ((pfix = FIXopen(sSecuAns, lAnsLen, REQUEST_LEN, NULL)) == NULL)
        {
            PrintLog(LEVEL_ERR, LOG_ERROR_INFO,  "����ȯ��[%s]ȡ�ļ�����Ӧ��ʧ��!", pub->sSecuNo);
            memset(sBuff, 0x00, sizeof(sBuff));
            sprintf(sBuff, "%s-", pub->sSecuNo);
            /*��ֹpub->sBankErrMsg���*/
            if (strlen(pub->sBankErrMsg) + strlen(sBuff) < sizeof(pub->sBankErrMsg))
                strcat(pub->sBankErrMsg, sBuff);
             
            continue;
        }
    
        /* ����־ */
        PrintFIXLog(LEVEL_NORMAL, LOG_RECV_SECU_ANS, pfix);
    
        /* ������� �ô�����ʹ�����Ϣ */
        memset(sBuff, 0x00, sizeof(sBuff));
        FIXgets(pfix, "ErrorNo", sBuff, sizeof(sBuff));
        FIXgets(pfix, "ErrorInfo", sBuffer, sizeof(sBuffer));
        /*memcpy(pub->sErrMsg, sBankErrMsg, sizeof(pub->sErrMsg) - 1);*/
        FIXclose(pfix);
    
        if (memcmp(sBuff, ERR_SUCCESS, 4) != 0)
        {
            PrintLog(LEVEL_ERR, LOG_ERROR_INFO,  "����ȯ��[%s]ȡ�ļ�����Ӧ��ʧ��[%s-%s]!", pub->sSecuNo,sBuff,sBuffer);

            memset(sBuffer, 0x00, sizeof(sBuffer));
            sprintf(sBuffer, "%s-", pub->sSecuNo);
            /*��ֹpub->sBankErrMsg���*/
            if (strlen(pub->sBankErrMsg) + strlen(sBuff) < sizeof(pub->sBankErrMsg))
                strcat(pub->sBankErrMsg, sBuffer);

            continue;
        }
    }
    DBFreePArray2(&psResult, lCount);
    
    return 0;
}


/******************************************************************************
�������ơ�TransToSecu
�������ܡ�������֤ȯ��ת֤�Ľṹ
���������pub     -- ���ײ���
���������sBankReq -- ֤ȯ���󻺳���
          lReqLen  -- ����������
�� �� ֵ����
******************************************************************************/
static void TransToSecu(TRANS99901 *pub, char *sBankReq, long *lReqLen)
{
    REQ99901 *req;
    long lSerialNo;
    FIX *pfix;

    BeginTrans();
    lSerialNo = GetNewSequenceNoByMax("fundserial_no");
    CommitTrans();

    req = &(pub->req);
    pfix = FIXopen(sBankReq, 0, REQUEST_LEN, NULL);
    
    FIXprintf(pfix,  "FunctionId"   , "%s", "23903");
    FIXprintf(pfix,  "ExSerial"     , "%ld", lSerialNo);
    FIXputs  (pfix,  "bOrganId"     , pub->sysarg->bank_id);
    FIXputs  (pfix,  "sOrganId"     , pub->sSecuNo);
    FIXprintf(pfix,  "Date"         , "%ld" , pub->lTransDate);
    FIXprintf(pfix,  "Time"         , "%ld" , req->Time);
   
    *lReqLen = FIXlen(pfix);
    /* ����־ */
    PrintFIXLog(LEVEL_NORMAL, LOG_SEND_SECU_REQ, pfix);

    FIXclose(pfix);
    
    return;
}

/******************************************************************************
�������ơ�Gen_DetailFile
�������ܡ����ɵ���ȯ�̵�ת����ϸ�ļ�
���������
         iFalg  :
                0  ----��֤ת����ϸ
                1  ----����ʻ������ϸ
�����������
�� �� ֵ�� 0 -- �ɹ�
          <0 -- ʧ��
******************************************************************************/
int Gen_DetailFile(TRANS99901 *pub, INITPARAMENT *initPara, char *psSecuNo, char *psBusinType, int iFlag)
{
    long  lCount = 0, lCnt = 0,i,lType=0,lNew,lOld;
    long  lnum = 0;
    long  lSerialNo, lNum, lTransNum;
    long  lZDBranch = 0,lJHBranch = 0;
    char  sRuleFlag[250], business_flag[4+1],OldCard[32+1],NewCard[32+1];
    char  cForceMigrateFlag;
    char  cDzfileBusintypeFlag;    /* 20081010-01 */
    char  sCustStatus[2], sQuery[128], sBuff[32+1];    
    char  sFTypes[251], sFileName[512], sFileType[32];
    char  sSecuAccount[DB_SCUACC_LEN+1];
    char  sSecuBranch[1000][DB_SBRNCH_LEN+1], sRealBranch[1000][DB_SBRNCH_LEN+1];
    char  sSql[SQL_BUF_LEN];
    char  **psResult, **psResult1, **psResult2;
    struct CARDPARSE cardparse;  /*20090810-01-�����˻����͵Ĳ�ͬ���õ����˻����ͱ䶯�ļ�������*/
    FILE  *fp, *fp1;
    TRDETAIL99901  trdetail99901;
    BALDETAIL99901 baldetail99901;
    ACCDETAIL99901 accdetail99901;
    PRECDETAIL99901 precdetail99901;
    TYPEDETAIL99901 typedetail99901;
    TBPARAM param;
    TBTRANS trans;
    
    /*����ת����ˮ�쳣������������ɶ�����ϸ�ļ�*/
    if (iFlag == 0)
    {
        LogTail(sLogName, "      �������֤ת�ʳ����쳣�����...");
        /*1 �������쳣�����*/
        memset(sSql, 0x00, sizeof(sSql));
        sprintf(sSql, "select secu_no from tbtransfer where secu_no='%s' and status not in ('%s', '%s') and repeal_status in ('1', '2', '3', '4')", psSecuNo, FLOW_SUCCESS, FLOW_FAILED);
        /*sprintf(sSql, "select secu_no from tbtransfer where secu_no='%s' and (repeal_status='2' or repeal_status='4') and (((trans_code='23701' or trans_code='23731') and status not in ('%s','%s','%s','%s')) or (trans_code='23702' and status not in ('%s','%s')))", 
                       psSecuNo, FLOW_SUCCESS, FLOW_FAILED, FLOW_SECU_SEND_F, FLOW_SECU_ERR, FLOW_SUCCESS, FLOW_FAILED);*/
        lCount = DBExecSelect(1, 0, sSql,&psResult);
        DBFreePArray2(&psResult, lCount);
        if ( lCount > 0)
        {
            SET_ERROR_CODE(ERR_FILEBYREPEAL);
            SET_ERROR_MSG("���ڳ����쳣����ˮ");
            PrintLog(LEVEL_ERR,LOG_ERROR_INFO,"���ڳ����쳣����ˮ");
            ERRLOGF("���ڳ����쳣����ˮ[%s]", sSql);
            LogTail(sLogName, "      �������֤ת�ʳ����쳣����ˮ��");
            LogTail(sLogName, "        ִ�����[%s]", sSql);            
            return -1;
        }
        LogTail(sLogName, "      �������֤ת�ʳ����쳣���������");
        
        /*2 ����ʽ���ʹ���״̬��һ�µ����*/
        LogTail(sLogName, "      �������֤ת���ʽ���ʹ���״̬��һ�µ����...");
        memset(sSql, 0x00, sizeof(sSql));
        sprintf(sSql, "select secu_no from tbtransfer where secu_no='%s' and ((balance_status='0' and status='%s') or (balance_status='1' and status<>'%s'))", psSecuNo, FLOW_SUCCESS, FLOW_SUCCESS);
        lCount = DBExecSelect(1, 0, sSql,&psResult);
        DBFreePArray2(&psResult, lCount);
        if ( lCount > 0)
        {
            SET_ERROR_CODE(ERR_FILEBYREPEAL);
            SET_ERROR_MSG("�����ʽ���ʹ���״̬��һ�µ���ˮ");
            PrintLog(LEVEL_ERR,LOG_ERROR_INFO,"�����ʽ���ʹ���״̬��һ�µ���ˮ");
            ERRLOGF("�����ʽ���ʹ���״̬��һ�µ���ˮ[%s]", sSql);
            LogTail(sLogName, "      �������֤ת���ʽ���ʹ���״̬��һ�µ���ˮ��");
            LogTail(sLogName, "        ִ�����[%s]", sSql);            
            return -2;
        }
        LogTail(sLogName, "      �������֤ת���ʽ���ʹ���״̬��һ�µ��������");
    }
    else
    {
        /*���ɴ���ʻ��䶯��ϸ�����ɿͻ���Ϣ״̬�ļ�*/
        /*��ȡȯ���ڲ�Ӫҵ�����*/
        memset(&param, 0x00, sizeof(TBPARAM));
        if ((DBGetParam(psSecuNo, "RZ_REALBRANCH_99901", &param) == 0) && (param.param_value[0] == '0'))
        {
            lNum = 0;
        }
        else
        {
            memset(sSql, 0x00, sizeof(sSql));
            /* 20080605-02-��ȡ����ȱʡ��ȯ��Ӫҵ����Ϣ*/
            /*sprintf(sSql, "select secu_branch,real_branch from tbsecubranch where secu_no='%s' and secu_branch<>'%s'", psSecuNo, DEFAULTSBRANCHNO);*/
            sprintf(sSql, "select secu_branch,real_branch from tbsecubranch where secu_no='%s'", psSecuNo);
            lNum = DBExecSelect(1, 0, sSql, &psResult1);
            if ( lNum <= 0)
            {
                lNum = 0;
            }
            else 
            {
                for(i=0; i<lNum&&i<1000; i++)
                {
                    memset(sSecuBranch[i], 0x00, sizeof(sSecuBranch[i])-1);
                    memset(sRealBranch[i], 0x00, sizeof(sRealBranch[i])-1);
                    memcpy(sSecuBranch[i], GetSubStr(psResult1[i],0 ), DB_SBRNCH_LEN);
                    memcpy(sRealBranch[i], GetSubStr(psResult1[i],1 ), DB_SBRNCH_LEN);
                }
                DBFreePArray2(&psResult1, lNum);
            }
        }
        
        /*binfo�ļ��е�״̬�Ƿ�ʹ���½ӿ�*/
        memset(sCustStatus, 0x00, sizeof(sCustStatus));
        memset(&param, 0x00, sizeof(TBPARAM));
        if ((DBGetParam(psSecuNo, "RZ_BINFO_99901", &param) == 0) && (param.param_value[0] == '1'))
        {
            sCustStatus[0] = '1';
        }
        else
        {
            sCustStatus[0] = '0';
        }
    }
    
    memset(&param, 0x00, sizeof(param));
    memset(sFTypes, 0x00, sizeof(sFTypes));
    if (DBGetParam(psSecuNo, "SECUFILE", &param) == 0)
        sprintf(sFTypes, "%s", param.param_value);
    else
        sprintf(sFTypes, "btranMMDD.dat,bbalaMMDD.dat,binfoMMDD.dat,beditMMDD.dat,");
        /*20090819-01ɾ��bprecMMDD.dat,bacctMMDD.dat*/   
    if (iFlag == 0)
    {
        /* ������֤ת����ϸ�ļ� */
        strcpy(sFileType, alltrim(GetSubStrViaFlag(sFTypes, 0, ',')));
    }
    else if (iFlag == 1)
    {
        /*���ɴ���ʻ��䶯��ϸ*/
        strcpy(sFileType, alltrim(GetSubStrViaFlag(sFTypes, 3, ',')));
    }
    else if (iFlag == 2)
    {
        /* ���ɿͻ���Ϣ״̬�ļ� */
        strcpy(sFileType, alltrim(GetSubStrViaFlag(sFTypes, 2, ',')));
    }
   /* LABEL : 20090624-01-����ԤԼ�����ͻ���Ϣ�����ļ�
    * MODIFY: huanghl@hundsun.com
    * REASON: ����ԤԼ�����ͼ����
    * HANDLE: ����ԤԼ�����ͻ���Ϣ�ļ�bprecMMDD.dat
    */
    else if (iFlag == 3)
    {
        /* ����ԤԼ�����ͻ���Ϣ�ļ� */
        strcpy(sFileType, alltrim(GetSubStrViaFlag(sFTypes, 4, ',')));
    } 
   /* LABEL : 20090720-01-���ӿͻ��ʻ����ͱ䶯�ļ�
    * MODIFY: huanghl@hundsun.com
    * REASON: ���ӿͻ��ʻ����ͱ䶯�ļ�
    * HANDLE: ���ӿͻ��ʻ����ͱ䶯�ļ�bacctMMDD.dat
    */
    else if (iFlag == 4)
    {
        /* ����ԤԼ�����ͻ���Ϣ�ļ� */
        strcpy(sFileType, alltrim(GetSubStrViaFlag(sFTypes, 5, ',')));
    }         
    /*�������ļ���Ϊ�գ��򲻴���*/
    if (strlen(alltrim(sFileType)) == 0)
    {
        return 0;
    }

    /* �����ļ� */
    memset(sFileName, 0x00, sizeof(sFileName));
    sprintf(sFileName, "%s/%s/%s%s",sSecuDir,psSecuNo,SENDFILE_DIR,ReplaceYYMMDD(sFileType, pub->lTransDate));
    fp = fopen(sFileName, "w+");
    if ( fp == NULL)
    {
        SET_ERROR_CODE(ERR_CREATFILE);
        PrintLog(LEVEL_ERR, LOG_ERROR_INFO, "�����ļ�ʧ��[%s]!", sFileName);
        LogTail(sLogName, "      �ﴴ���ļ�ʧ��[%s]!", sFileName);
        return -1;
    }

    /*
     *  ����SQL���
     */
    /* LABEL : 20081010-01 ������ȯҵ����ʹ��ͬһȯ�̱��ʱ�����ļ�(����ģʽ)����ҵ������
     * MODIFY: ��Ԫ��20081010
     * REASON: �����ˡ�������ȯ��ҵ�������Ҫ�޸����ɵ��ļ�
     * HANDLE: ���ļ���¼�����׷�ӡ�ҵ�����͡��ֶ�
     */      

    /* 20081010-01ȡ�����ļ����Ƿ�����ҵ������־ */
    cDzfileBusintypeFlag='0';   /* Ĭ���ļ�����Ҫ����ҵ������ֶ� */
    memset(&param, 0x00, sizeof(param));
    if (DBGetParam(psSecuNo, "RZ_DZFILE_BUSINTYPE", &param) == 0)
    {
        cDzfileBusintypeFlag = param.param_value[0];
    }                

    memset(sSql, 0x00, sizeof(sSql));
    if (iFlag == 0)
    {
        LogTail(sLogName, "      ����ѯ�ɹ�ת����ˮ��¼��...");
        /*��ѯ�ɹ�ת����ˮ*/
        sprintf(sSql,"select count(secu_no) from tbtransfer");
        sprintf(sSql,"%s where secu_no='%s' and hs_date=%ld and status='S'", sSql, psSecuNo, pub->lTransDate);
        if (strlen(alltrim(psBusinType)) > 0)
            sprintf(sSql,"%s and busin_type='%c'", sSql, psBusinType[0]);
        lCount = DBExecSelect(1, 0, sSql,&psResult);
        if ( lCount <= 0)
        {
            SET_ERROR_CODE(ERR_DATABASE);
            return -2;
        }
        else
        {
            lTransNum = atol((char *)GetSubStr(psResult[0],0 ));
        }
        DBFreePArray2(&psResult, lCount);
        LogTail(sLogName, "      ����ѯ�ɹ�ת����ˮ��¼���������ܼ�¼��[%ld]", lTransNum);

        LogTail(sLogName, "      ������ת����ϸ�ļ�...");

        /*��ѯ�ɹ�ת����ˮ*/        
        memset(sSql, 0x00, sizeof(sSql));
        /* 20081010-01
        sprintf(sSql,"select source_side,trans_code, secu_use_account, secu_account, occur_balance, money_kind, serial_no, secu_serial, hs_date, hs_time, secu_no from tbtransfer");
        */
        sprintf(sSql,"select source_side,trans_code, secu_use_account, secu_account, occur_balance, money_kind, serial_no, secu_serial, hs_date, hs_time, secu_no, busin_type from tbtransfer");
        sprintf(sSql,"%s where secu_no='%s' and hs_date=%ld and status='S'", sSql, psSecuNo, pub->lTransDate);
        
        if (strlen(alltrim(psBusinType)) > 0)
            sprintf(sSql,"%s and busin_type='%c'", sSql, psBusinType[0]);
        
        for(i=0; i<lTransNum; i=i+SELECT_NUM)
        {
            LogTail(sLogName, "        ����ѯ�ɹ�ת����ˮ����[%ld-%ld]...", i+1, lTransNum<(i+SELECT_NUM)?lTransNum:(i+SELECT_NUM));
            lCount = DBExecSelect(i+1, SELECT_NUM, sSql,&psResult);
            if ( lCount < 0)
            {
                SET_ERROR_CODE(ERR_DATABASE);
                return -2;
            }
            LogTail(sLogName, "        ����ѯ�ɹ�ת����ˮ���ݽ���");

            LogTail(sLogName, "        �����ɳɹ�ת����ϸ�ļ�...");
            for(lCnt = 0; lCnt < lCount; lCnt ++)    
            {
                memset(&trdetail99901, 0x00, sizeof(trdetail99901));
                memcpy(trdetail99901source_side     , (char *)GetSubStr(psResult[lCnt],0 ) , sizeof(trdetail99901source_side   ) - 1);
                memcpy(trdetail99901trans_code      , (char *)GetSubStr(psResult[lCnt],1 ) , sizeof(trdetail99901trans_code    ) - 1);
                memcpy(trdetail99901client_account  , (char *)GetSubStr(psResult[lCnt],2 ) , sizeof(trdetail99901client_account) - 1);
                memcpy(trdetail99901secu_account    , (char *)GetSubStr(psResult[lCnt],3 ) , sizeof(trdetail99901secu_account  ) - 1);
                trdetail99901occur_balance    = atof( (char *)GetSubStr(psResult[lCnt],4 ) );
                memcpy(trdetail99901money_kind      , (char *)GetSubStr(psResult[lCnt],5 ) , sizeof(trdetail99901money_kind    ) - 1);
                memcpy(trdetail99901serial_no       , (char *)GetSubStr(psResult[lCnt],6 ) , sizeof(trdetail99901serial_no     ) - 1);
                memcpy(trdetail99901secu_serial     , (char *)GetSubStr(psResult[lCnt],7 ) , sizeof(trdetail99901secu_serial   ) - 1);
                trdetail99901hs_date                = atol((char *)GetSubStr(psResult[lCnt],8 ) );
                trdetail99901hs_time                = atol((char *)GetSubStr(psResult[lCnt],9 ) );
                memcpy(trdetail99901secu_no         , (char *)GetSubStr(psResult[lCnt],10) , sizeof(trdetail99901secu_no       ) - 1);
                /* 20081010-01 ҵ����� */
                memcpy(trdetail99901reserved1       , (char *)GetSubStr(psResult[lCnt],11) , 1);   
                if ((memcmp(trdetail99901trans_code, "24701", 5) == 0) ||
                    (memcmp(trdetail99901trans_code, "23701", 5) == 0)||
                    (memcmp(trdetail99901trans_code, "23731", 5) == 0 ))
                {
                     memcpy(trdetail99901trans_type, "01", 2);
                }
                else
                if ((memcmp(trdetail99901trans_code, "24702", 5) == 0) ||
                      (memcmp(trdetail99901trans_code, "23702", 5) == 0))
                {
                       memcpy(trdetail99901trans_type, "02", 2);
                }
                else
                   continue;
    
                /*д�ļ�*/
                fprintf(fp,"%1s|%2s|%32s|%20s|%13.2lf|%s|%20s|%20s|%8ld|%06ld|%s|%6s|%1s|\n",
                           trdetail99901source_side      ,
                           trdetail99901trans_type      ,
                           trdetail99901client_account  ,
                           trdetail99901secu_account    ,
                           trdetail99901occur_balance   ,
                           trdetail99901money_kind      ,
                           trdetail99901serial_no       ,
                           trdetail99901secu_serial     ,
                           trdetail99901hs_date         ,
                           trdetail99901hs_time         ,
                           pub->sysarg->bank_id          ,  
                           trdetail99901secu_no         ,
                           trdetail99901reserved1       );
            }
            DBFreePArray2(&psResult, lCount);

            LogTail(sLogName, "        �����ɳɹ�ת����ϸ�ļ�����![%ld-%ld]", i+1, lTransNum<(i+SELECT_NUM)?lTransNum:(i+SELECT_NUM));
        }
        LogTail(sLogName, "      ������ת����ϸ�ļ�����");
    }
    else if (iFlag == 1)
    {
        LogTail(sLogName, "      ����ѯ����ʻ��䶯��ϸ����...");

        /* LABEL : 20110506-02-��������Ԥָ����������������
         * MODIFY: huanghl@hundsun.com
         * REASON: ��������������������Ԥָ����������������
         * HANDLE������1032-Ԥָ������
         */
        /*�����ӿڸ��ݻ���ȯ��������־��ȯ���˻������ļ��м�¼������ˮ��Ϣ*/
        memset(sQuery, 0x00, sizeof(sQuery));
        memset(&trans, 0x00, sizeof(TBTRANS));
        DBGetTrans(psSecuNo, "25710", &trans);
        if (trans.secu_online[0] == '0')
            strcpy(sQuery, "(1001,1002,1015,1016,1017,1018,1019,1022,1032)");
        else
            strcpy(sQuery, "(1001,1002,1015,1016,1017,1018,1019,1022,1004,1032)");
        
        /*���ɴ���ʻ��䶯��ϸ*/
        /*
        sprintf(sSql,"select serial_no,bank_id,secu_use_account,secu_no,secu_branch,secu_account,client_name,id_kind,id_code,money_kind,occur_balance,trans_code,secu_serial,business_flag from tbaccountedit a"); 
        */
        memset(sSql, 0x00, sizeof(sSql));
        sprintf(sSql,"select serial_no,bank_id,secu_use_account,secu_no,secu_branch,secu_account,client_name,id_kind,id_code,money_kind,occur_balance,trans_code,secu_serial,business_flag,busin_type from tbaccountedit a"); 
        sprintf(sSql,"%s where secu_no='%s'", sSql, psSecuNo);
        sprintf(sSql,"%s and business_flag in %s", sSql, sQuery);
        if (strlen(alltrim(psBusinType)) > 0)
            sprintf(sSql,"%s and busin_type='%c'", sSql, psBusinType[0]);
        lCount = DBExecSelect(1, 0, sSql,&psResult);
        if ( lCount < 0)
        {
            SET_ERROR_CODE(ERR_DATABASE);
            return -2;
        }
        LogTail(sLogName, "      ����ѯ����ʻ��䶯��ϸ���ݽ���");

        LogTail(sLogName, "      �����ɴ���ʻ��䶯��ϸ�ļ�...");
        for(lCnt = 0; lCnt < lCount; lCnt ++)    
        {
            /*���ɴ���ʻ��䶯��ϸ*/
            memset(&accdetail99901, 0x00, sizeof(accdetail99901));
            accdetail99901serial_no = atol((char *)GetSubStr(psResult[lCnt],0)); 
            memcpy(accdetail99901bank_id         , (char *)GetSubStr(psResult[lCnt],1 ), sizeof(accdetail99901bank_id));
            memcpy(accdetail99901secu_use_account, (char *)GetSubStr(psResult[lCnt],2 ), sizeof(accdetail99901secu_use_account));
            memcpy(accdetail99901secu_no         , (char *)GetSubStr(psResult[lCnt],3 ), sizeof(accdetail99901secu_no));
            /*��bedit�ļ�����дȯ���ڲ�Ӫҵ�����*/
            memcpy(sBuff, (char *)GetSubStr(psResult[lCnt],4 ), DB_SBRNCH_LEN);
            memcpy(accdetail99901secu_branch, sBuff, DB_SBRNCH_LEN);
            if (lNum > 0)
            {
                for(i=0; i<lNum&&i<1000; i++)
                {
                    if (memcmp(sBuff, sSecuBranch[i], DB_SBRNCH_LEN) == 0)
                    {
                        memset(accdetail99901secu_branch, 0x00, sizeof(accdetail99901secu_branch));
                        memcpy(accdetail99901secu_branch, sRealBranch[i], DB_SBRNCH_LEN);
                        break;
                    }
                    else
                        continue;
                }
            }
            memcpy(accdetail99901secu_account    , (char *)GetSubStr(psResult[lCnt],5 ), sizeof(accdetail99901secu_account));                                          
            memcpy(accdetail99901client_name     , (char *)GetSubStr(psResult[lCnt],6 ), sizeof(accdetail99901client_name));
            memcpy(accdetail99901id_kind         , (char *)GetSubStr(psResult[lCnt],7 ), sizeof(accdetail99901id_kind));
            memcpy(accdetail99901id_code         , (char *)GetSubStr(psResult[lCnt],8 ), sizeof(accdetail99901id_code));
            memcpy(accdetail99901money_kind      , (char *)GetSubStr(psResult[lCnt],9 ), sizeof(accdetail99901money_kind));
            accdetail99901occur_bala = atol((char *)GetSubStr(psResult[lCnt],10));
            memcpy(accdetail99901trans_code      , (char *)GetSubStr(psResult[lCnt],11 ), sizeof(accdetail99901trans_code));
            memcpy(accdetail99901secu_serial     , (char *)GetSubStr(psResult[lCnt],12 ), sizeof(accdetail99901secu_serial));
            /* 20081010-01 ҵ����� */ 
            memcpy(accdetail99901busin_type      , (char *)GetSubStr(psResult[lCnt],14 ), 1);

            /*�����ͻ��˻�����ϸ�Ϳͻ�״̬�ļ���ʽ*/
            /* 20081010-01 ҵ����� 
            fprintf(fp,"%8ld|%s|%32s|%8s|%8s|%20s|%60s|%s|%18s|%s|%15.2lf|%5s|%20s|\n",
            */
            fprintf(fp,"%8ld|%s|%32s|%8s|%8s|%20s|%60s|%s|%18s|%s|%15.2lf|%5s|%20s|",
                        accdetail99901serial_no,
                        accdetail99901bank_id,
                        accdetail99901secu_use_account,
                        accdetail99901secu_no, 
                        accdetail99901secu_branch,
                        accdetail99901secu_account,
                        accdetail99901client_name,
                        accdetail99901id_kind,
                        accdetail99901id_code,
                        accdetail99901money_kind,
                        accdetail99901occur_bala,
                        accdetail99901trans_code,
                        accdetail99901secu_serial
                    );

            /* 20081010-01 ҵ����� */
            if(cDzfileBusintypeFlag == '1')
            {
                fprintf(fp, "%1s|\n", accdetail99901busin_type);
            }
            else
            {
                fprintf(fp, "\n");
            }   

        }
        DBFreePArray2(&psResult, lCount);
        
        LogTail(sLogName, "      �����ɴ���ʻ��䶯��ϸ�ļ�����");
    }
    else if (iFlag == 2)
    {
        LogTail(sLogName, "      ����ѯ�ͻ���Ϣ״̬����...");

        /*20080522-01-����Ԥָ���ͻ���֤ȯ��Ϊ�����ͻ�*/
        memset(&param, 0x00, sizeof(TBPARAM));
        DBGetParam(psSecuNo,"FORCE_MIGRATE_STATUS", &param);
        /*Ԥָ�������п�ʱ�Ƿ�ǿ�ƿͻ�״̬ΪԤָ��״̬:0-��(Ĭ��),1-��*/
        if ( param.param_value[0] == '1')
            cForceMigrateFlag = '1';
        else
            cForceMigrateFlag = '0';
        /* LABEL : 20110506-02-��������Ԥָ����������������
         * MODIFY: huanghl@hundsun.com
         * REASON: ��������������������Ԥָ����������������
         * HANDLE������FLAG_XYZD-Ԥָ������
         */
        /* ���ɿͻ���Ϣ״̬�ļ� */
        /* 20080708-01-��֤�˻�״̬�ļ���¼Ψһ������distinct*/
        memset(sSql, 0x00, sizeof(sSql));        
        /* 20081010-01
        sprintf(sSql,"select distinct a.bank_id,a.secu_use_account,a.secu_no,a.secu_branch,a.secu_account,a.client_name,a.id_kind,a.id_code,a.money_kind,a.status,b.occur_balance,a.client_account from tbclientyzzz a,tbaccountedit b"); 
        */
        sprintf(sSql,"select distinct a.bank_id,a.secu_use_account,a.secu_no,a.secu_branch,a.secu_account,a.client_name,a.id_kind,a.id_code,a.money_kind,a.status,b.occur_balance,a.client_account,a.busin_type from tbclientyzzz a,tbaccountedit b"); 
        sprintf(sSql,"%s where a.secu_no='%s'", sSql, psSecuNo);
        sprintf(sSql,"%s and a.secu_no=b.secu_no and a.secu_account=b.secu_account and a.busin_type=b.busin_type and a.money_kind=b.money_kind", sSql);
        /* 20080523-01-�˻�״̬�ļ��б������ɵ���ǩԼ�Ķ�����¼ */
        /* 20120929-01-�����������ڣ�ͨ���������� */
        if (pub->cNeedModify != '0')
        {
            /*20130916-01-���¸�ֵ�궨��Ϊ�����͵�business_flag�ֶ� %ld->%d */
            sprintf(sSql,"%s and ((a.open_date=%ld and b.business_flag in (%d,%d,%d,%d,%d,%d)) or (a.open_date<%ld and a.status='0' and b.business_flag=%d))", 
                      sSql, pub->lTransDate, FLAG_KHKH, FLAG_KHXH, FLAG_ZZKH, FLAG_ZZXH, FLAG_YYZD,FLAG_XYZD,pub->lTransDate, FLAG_YZKH);
        }
        else
        {
            /*20130916-01-���¸�ֵ�궨��Ϊ�����͵�business_flag�ֶ� %ld->%d */
            sprintf(sSql,"%s and ((a.open_date=%ld and b.business_flag in (%d,%d,%d)) or (a.open_date<%ld and a.status='0' and b.business_flag=%d) or (a.open_date<%ld and a.status='3' and b.business_flag in (%d,%d,%d)) )", 
                      sSql, pub->lTransDate, FLAG_KHKH, FLAG_ZZKH, FLAG_YYZD, pub->lTransDate, FLAG_YZKH, pub->lTransDate, FLAG_KHXH, FLAG_XYZD, FLAG_ZZXH);
        }
        
        if (strlen(alltrim(psBusinType)) > 0)
            sprintf(sSql,"%s and a.busin_type='%c'", sSql, psBusinType[0]);

        lSerialNo = 0;

        lCount = DBExecSelect(1, 0, sSql,&psResult);
        if ( lCount < 0)
        {
            SET_ERROR_CODE(ERR_DATABASE);
            return -2;
        }
        LogTail(sLogName, "      ����ѯ�ͻ���Ϣ״̬���ݽ���");

        LogTail(sLogName, "      �����ɴ���ʻ�״̬�ļ�...");
        for(lCnt = 0; lCnt < lCount; lCnt ++)    
        {
            memset(&accdetail99901, 0x00, sizeof(accdetail99901));
            accdetail99901serial_no = ++lSerialNo; 
            memcpy(accdetail99901bank_id         , (char *)GetSubStr(psResult[lCnt],0 ), sizeof(accdetail99901bank_id));
            memcpy(accdetail99901secu_use_account, (char *)GetSubStr(psResult[lCnt],1 ), sizeof(accdetail99901secu_use_account));
            memcpy(accdetail99901secu_no         , (char *)GetSubStr(psResult[lCnt],2 ), sizeof(accdetail99901secu_no));
            /*��binfo�ļ�����дȯ���ڲ�Ӫҵ�����*/
            memset(sBuff, 0x00, sizeof(sBuff));
            memcpy(sBuff, (char *)GetSubStr(psResult[lCnt],3 ), DB_SBRNCH_LEN);
            memcpy(accdetail99901secu_branch, sBuff, DB_SBRNCH_LEN);
            if (lNum > 0)
            {
                for(i=0; i<lNum&&i<1000; i++)
                {
                    if (memcmp(sBuff, sSecuBranch[i], DB_SBRNCH_LEN) == 0)
                    {
                        memcpy(accdetail99901secu_branch, sRealBranch[i], DB_SBRNCH_LEN);
                        break;
                    }
                    else
                        continue;
                }
            }
            memcpy(accdetail99901secu_account    , (char *)GetSubStr(psResult[lCnt],4 ), sizeof(accdetail99901secu_account));                                          
            memcpy(accdetail99901client_name     , (char *)GetSubStr(psResult[lCnt],5 ), sizeof(accdetail99901client_name));
            memcpy(accdetail99901id_kind         , (char *)GetSubStr(psResult[lCnt],6 ), sizeof(accdetail99901id_kind));
            memcpy(accdetail99901id_code         , (char *)GetSubStr(psResult[lCnt],7 ), sizeof(accdetail99901id_code));
            memcpy(accdetail99901money_kind      , (char *)GetSubStr(psResult[lCnt],8 ), sizeof(accdetail99901money_kind));
            memset(sBuff, 0x00, sizeof(sBuff));
            memcpy(sBuff, (char *)GetSubStr(psResult[lCnt],10), 16);
            accdetail99901occur_bala = atof(sBuff);  
            memcpy(accdetail99901status          , (char *)GetSubStr(psResult[lCnt],9 ), sizeof(accdetail99901status));
            /* 20081010-01 ҵ����� */
            memcpy(accdetail99901busin_type      , (char *)GetSubStr(psResult[lCnt],12 ), 1);
            
            /*��Լ״̬*/
            if (accdetail99901status[0] == '3')
            {    
                accdetail99901status[0] = '1';
            }
            /*Ԥָ��״̬*/
            else if (accdetail99901status[0] == '5')
            {
                /*20071013-01-����Ԥָ���ͻ���֤ȯ��Ϊ�����ͻ�*/
                memset(sBuff, 0x00, sizeof(sBuff));
                memcpy(sBuff, (char *)GetSubStr(psResult[lCnt],11), DB_BNKACC_LEN);
                /*�����Ų���ǿ��Ԥָ��״̬������ȯ��Ϊ����״̬*/
                if ((strlen(alltrim(sBuff)) > 0) && (cForceMigrateFlag == '1'))
                    accdetail99901status[0] = '0';
                else
                {
                    if (sCustStatus[0] == '1')
                        accdetail99901status[0] = '2';
                    else
                        accdetail99901status[0] = '0';
                }

            }
            /*����״̬*/
            else
                accdetail99901status[0] = '0';
            
            /* 20081010-01 ҵ�����
            fprintf(fp,"%8ld|%s|%32s|%8s|%8s|%20s|%60s|%s|%18s|%s|%15.2lf|%s|\n",
            */
            fprintf(fp,"%8ld|%s|%32s|%8s|%8s|%20s|%60s|%s|%18s|%s|%15.2lf|%s|",
                        accdetail99901serial_no,
                        accdetail99901bank_id,
                        accdetail99901secu_use_account,
                        accdetail99901secu_no, 
                        accdetail99901secu_branch,
                        accdetail99901secu_account,
                        accdetail99901client_name,
                        accdetail99901id_kind,
                        accdetail99901id_code,
                        accdetail99901money_kind,
                        accdetail99901occur_bala,
                        accdetail99901status
                    );

            /* 20081010-01 ҵ����� */
            if(cDzfileBusintypeFlag == '1')
            {
                fprintf(fp, "%1s|\n", accdetail99901busin_type);
            }
            else
            {
                fprintf(fp, "\n");
            }   

        }   
        DBFreePArray2(&psResult, lCount);

        LogTail(sLogName, "      �����ɴ���ʻ�״̬�ļ�����");
    }
   /* LABEL : 20090624-01-����ԤԼ�����ͻ���Ϣ�����ļ�
    * MODIFY: huanghl@hundsun.com
    * REASON: ����ԤԼ�����ͼ����
    * HANDLE: ����ԤԼ�����ͻ���Ϣ�ļ�bprecMMDD.dat
    */
    else if (iFlag == 3)
    {
        LogTail(sLogName, "      ����ѯԤԼ�����ͻ���Ϣ����...");

        /* ����ԤԼ�����ͻ���Ϣ״̬�ļ� */
        memset(sSql, 0x00, sizeof(sSql));        
        sprintf(sSql,"select distinct a.bank_id,a.secu_use_account,a.secu_no,a.secu_branch_b,a.secu_branch_s,a.secu_account,a.client_name,a.id_kind,a.id_code,a.money_kind,a.status,a.busin_type from tbpreacc a,tbaccountedit b"); 
        sprintf(sSql,"%s where a.secu_no='%s'", sSql, psSecuNo);
        sprintf(sSql,"%s and a.secu_no=b.secu_no and a.secu_account=b.secu_account and a.busin_type=b.busin_type and a.money_kind=b.money_kind and a.fund_account=b.fund_account", sSql);
        /*20130916-01-���¸�ֵ�궨��Ϊ�����͵�business_flag�ֶ� %ld->%d */
        sprintf(sSql,"%s and ((a.active_date=%ld and b.business_flag in (%d,%d)) or (a.pre_date=%ld and b.business_flag=%d))", 
                      sSql, pub->lTransDate, FLAG_KHKH, FLAG_YZKH, pub->lTransDate, FLAG_YDJH);
        if (strlen(alltrim(psBusinType)) > 0)
            sprintf(sSql,"%s and a.busin_type='%c'", sSql, psBusinType[0]);

        lSerialNo = 0;

        lCount = DBExecSelect(1, 0, sSql,&psResult);
        if ( lCount < 0)
        {
            SET_ERROR_CODE(ERR_DATABASE);
            return -2;
        }
        LogTail(sLogName, "      ����ѯԤԼ�����ͻ���Ϣ���ݽ���");

        LogTail(sLogName, "      ������ԤԼ�����ͻ���Ϣ�ļ�...");
        for(lCnt = 0; lCnt < lCount; lCnt ++)    
        {
           /* LABEL : 20090804-01-��ʼ������������ҵ������
            * MODIFY: huanghl@hundsun.com
            * REASON: lZDBranch��lJHBranchδ��ʼ��
            * HANDLE: ��ʼ��lZDBranch��lJHBranch
            */
            lZDBranch = 0;/*ԤԼ����ʱָ����ȯ��Ӫҵ��*/
            lJHBranch = 0;/*ȯ�̷�����ʱʹ�õ�ȯ��Ӫҵ��*/
            memset(&precdetail99901, 0x00, sizeof(precdetail99901));
            precdetail99901serial_no = ++lSerialNo; 
            memcpy(precdetail99901bank_id         , (char *)GetSubStr(psResult[lCnt],0 ), sizeof(precdetail99901bank_id));
            memcpy(precdetail99901secu_use_account, (char *)GetSubStr(psResult[lCnt],1 ), sizeof(precdetail99901secu_use_account));
            memcpy(precdetail99901secu_no         , (char *)GetSubStr(psResult[lCnt],2 ), sizeof(precdetail99901secu_no));
            memcpy(precdetail99901secu_branch_b   , (char *)GetSubStr(psResult[lCnt],3 ), DB_SBRNCH_LEN);
            memcpy(precdetail99901secu_branch_s   , (char *)GetSubStr(psResult[lCnt],4 ), DB_SBRNCH_LEN);
            if (lNum > 0)
            {
                for(i=0; i<lNum&&i<1000; i++)
                {
                    if (memcmp(precdetail99901secu_branch_b, sSecuBranch[i], DB_SBRNCH_LEN) == 0)
                    {
                        memcpy(precdetail99901secu_branch_b, sRealBranch[i], DB_SBRNCH_LEN);
                        lZDBranch = 1;
                        if (strlen(alltrim(precdetail99901secu_branch_s)) == 0)
                        {
                            break;
                        }
                    }
                    if (memcmp(precdetail99901secu_branch_s, sSecuBranch[i], DB_SBRNCH_LEN) == 0)
                    {
                        memcpy(precdetail99901secu_branch_s, sRealBranch[i], DB_SBRNCH_LEN);
                        lJHBranch = 1;
                    }
                    /*������ָ����Ӫҵ���ͼ���ʹ�õ�Ӫҵ������ȯ���ڲ�Ӫҵ�����ƥ��ʱ�˳�*/
                    if (lZDBranch == 1 && lJHBranch == 1)
                        break;
                    else
                        continue;
                }
                
            }
            
            memcpy(precdetail99901secu_account    , (char *)GetSubStr(psResult[lCnt],5 ), sizeof(precdetail99901secu_account));                                          
            memcpy(precdetail99901client_name     , (char *)GetSubStr(psResult[lCnt],6 ), sizeof(precdetail99901client_name));
            memcpy(precdetail99901id_kind         , (char *)GetSubStr(psResult[lCnt],7 ), sizeof(precdetail99901id_kind));
            memcpy(precdetail99901id_code         , (char *)GetSubStr(psResult[lCnt],8 ), sizeof(precdetail99901id_code));
            memcpy(precdetail99901money_kind      , (char *)GetSubStr(psResult[lCnt],9 ), sizeof(precdetail99901money_kind));
            memcpy(precdetail99901status          , (char *)GetSubStr(psResult[lCnt],10 ), sizeof(precdetail99901status));
            memcpy(precdetail99901busin_type      , (char *)GetSubStr(psResult[lCnt],11 ), 1);
            
            /*����״̬*/
            if (precdetail99901status[0] == '2')
            {    
                precdetail99901status[0] = '0';
            }
            
            fprintf(fp,"%8ld|%s|%32s|%8s|%8s|%8s|%20s|%60s|%s|%18s|%s|%s|",
                        precdetail99901serial_no,
                        precdetail99901bank_id,
                        precdetail99901secu_use_account,
                        precdetail99901secu_no, 
                        precdetail99901secu_branch_b,
                        precdetail99901secu_branch_s,
                        precdetail99901secu_account,
                        precdetail99901client_name,
                        precdetail99901id_kind,
                        precdetail99901id_code,
                        precdetail99901money_kind,
                        precdetail99901status
                    );

            if(cDzfileBusintypeFlag == '1')
            {
                fprintf(fp, "%1s|\n", precdetail99901busin_type);
            }
            else
            {
                fprintf(fp, "\n");
            }   

        }   
        DBFreePArray2(&psResult, lCount);

        LogTail(sLogName, "      ������ԤԼ�����ͻ���Ϣ�ļ�����");
    }
    
   /* LABEL : 20090720-01-���ӿͻ��ʻ����ͱ䶯�ļ�
    * MODIFY: huanghl@hundsun.com
    * REASON: ���ӿͻ��ʻ����ͱ䶯�ļ�
    * HANDLE: ���ӿͻ��ʻ����ͱ䶯�ļ�bacctMMDD.dat
    */
    else if (iFlag == 4)
    {
        memset(&param, 0x00, sizeof(TBPARAM));
        memset(sRuleFlag, 0x00, sizeof(sRuleFlag));
        /*CHKGET_ACCTYPE_RUAL�����������������ָ��ȯ��δ��˲�����ϵͳ����������Ϊ׼,����Ϊ���������ļ�*/
        if (DBGetParam(psSecuNo, "CHKGET_ACCTYPE_RUAL", &param) == 0)
        {
            strcpy(sRuleFlag,param.param_value);
        }
        else
        {
            memset(&param, 0x00, sizeof(TBPARAM));
            if (DBGetParam(DEFAULTSECUNO, "CHKGET_ACCTYPE_RUAL", &param) == 0)
            {
                strcpy(sRuleFlag,param.param_value);
            }
        }
        if (strlen(alltrim(sRuleFlag)) == 0)
        {
            return 0;
        }
        
        /* LABEL : 20090810-01-�����˻����͵Ĳ�ͬ���õ����˻����ͱ䶯�ļ�������
         * MODIFY: zhuwl@hundsun.com
         * REASON: �������������˻���־�Ƚϸ���
         * HANDLE: ���ӶԶ�λƥ��ͷ�Χƥ����˻����Ͳ������õ�֧��
         */
        /*�����˻����͵�����*/
        /* LABEL : 20090901-01-���ӱ���cardparse��ʼ��
         * MODIFY: wanggk@hundsun.com
         * REASON: ����cardparseδ��ʼ��
         * HANDLE: ���ӱ���cardparse��ʼ��
         */ 
        memset(&cardparse, 0x00, sizeof(cardparse)); 
        ParseParam(sRuleFlag, &cardparse);
        
         /* LABEL : 20090804-01-��ʼ������������ҵ������
          * MODIFY: huanghl@hundsun.com
          * REASON: FLAG_QZHK��FLAG_YZKHҵ������Ҳ�漰�����п����ͱ䶯
          * HANDLE: ����1024-FLAG_QZHK��1019-FLAG_YZKH����
          */
        LogTail(sLogName, "      ����ѯ�ͻ��ʻ���������...");
        memset(sSql, 0x00, sizeof(sSql));
        sprintf(sSql,"select distinct c.secu_no,c.secu_account,c.money_kind,c.busin_type,c.bank_id,c.secu_use_account,c.client_account,c.secu_branch,c.client_name,c.id_kind,c.id_code from tbaccountedit a ,tbclientyzzz c");
        /* LABEL : 20090819-01-���ӹ�̩��ʽԤԼ�������������ļ�
         * MODIFY: huanghl@hundsun.com
         * REASON: 25710��������ֿ���Ϊ��
         * HANDLE: ����sql��ѯ�����ƥ������ɾ������
         */
        sprintf(sSql,"%s where a.secu_no=c.secu_no and a.secu_account=c.secu_account and a.busin_type=c.busin_type",sSql);
        sprintf(sSql,"%s and a.business_flag in(1001,1004,1024,1019) and c.status='0' and a.secu_no='%s' and a.hs_date =%ld", sSql, psSecuNo,pub->lTransDate);
        if (strlen(alltrim(psBusinType)) > 0)
            sprintf(sSql,"%s and a.busin_type='%c'", sSql, psBusinType[0]);
        lSerialNo = 0;
        lCount = DBExecSelect(1, 0, sSql,&psResult);
        if ( lCount < 0)
        {
            SET_ERROR_CODE(ERR_DATABASE);
            return -2;
        }
        LogTail(sLogName, "      ����ѯ�ͻ��ʻ��������ݽ���");
        LogTail(sLogName, "      �����ɿͻ��ʻ����ͱ䶯�ļ�...");
        for(lCnt = 0; lCnt < lCount; lCnt ++)
        {
            memset(&typedetail99901, 0x00, sizeof(typedetail99901));
            memcpy(typedetail99901secu_no, (char *)GetSubStr(psResult[lCnt],0 ), sizeof(typedetail99901secu_no));
            memcpy(typedetail99901secu_account, (char *)GetSubStr(psResult[lCnt],1 ), sizeof(typedetail99901secu_account));
            memcpy(typedetail99901money_kind, (char *)GetSubStr(psResult[lCnt],2 ), sizeof(typedetail99901money_kind));
            memcpy(typedetail99901busin_type, (char *)GetSubStr(psResult[lCnt],3 ), sizeof(typedetail99901busin_type));
            memcpy(typedetail99901bank_id, (char *)GetSubStr(psResult[lCnt],4 ), sizeof(typedetail99901bank_id));
            memcpy(typedetail99901secu_use_account, (char *)GetSubStr(psResult[lCnt],5 ), sizeof(typedetail99901secu_use_account));
            memcpy(typedetail99901client_account, alltrim(GetSubStr(psResult[lCnt],6 )), sizeof(typedetail99901client_account));
            memcpy(typedetail99901secu_branch, (char *)GetSubStr(psResult[lCnt],7 ), sizeof(typedetail99901secu_branch));
            memcpy(typedetail99901client_name, (char *)GetSubStr(psResult[lCnt],8 ), sizeof(typedetail99901client_name));
            memcpy(typedetail99901id_kind, (char *)GetSubStr(psResult[lCnt],9 ), sizeof(typedetail99901id_kind));
            memcpy(typedetail99901id_code, (char *)GetSubStr(psResult[lCnt],10 ), sizeof(typedetail99901id_code));

            /*ʹ�õ��յ�һ��tbaccountedit��¼�Ŀ���Ϊ���վɿ���*/
            memset(sSql,0x00,sizeof(sSql));
            /* LABEL : 20090804-01-��ʼ������������ҵ������
             * MODIFY: huanghl@hundsun.com
             * REASON: FLAG_QZHK��FLAG_YZKHҵ������Ҳ�漰�����п����ͱ䶯
             * HANDLE: ����1024-FLAG_QZHK��1019-FLAG_YZKH����
             */
            sprintf(sSql,"select client_account,business_flag from tbaccountedit where hs_date=%ld and serial_no=(select min(serial_no) from tbaccountedit where secu_no='%s' and secu_account='%s' and busin_type='%s' and hs_date=%ld and business_flag in(1001,1004,1024,1019))",
                     pub->lTransDate,
                     typedetail99901secu_no,
                     typedetail99901secu_account,
                     typedetail99901busin_type,
                     pub->lTransDate);
            lnum = DBExecSelect(1, 0, sSql,&psResult2);
            if (lnum < 0)
            {
                DBFreePArray2(&psResult, lCount);
                SET_ERROR_CODE(ERR_DATABASE);
                return -2;
            }
            memset(OldCard,0x00,sizeof(OldCard));
            memset(NewCard,0x00,sizeof(NewCard));
            memset(business_flag,0x00,sizeof(business_flag));
            memcpy(OldCard,alltrim(GetSubStr(psResult2[0],0 )),sizeof(OldCard)-1);
            memcpy(business_flag,alltrim(GetSubStr(psResult2[0],1 )),sizeof(business_flag)-1);
            memcpy(NewCard,typedetail99901client_account,sizeof(NewCard)-1);
            /*���տ�����Ĭ�Ͼɿ�����Ϊ��ͨ��0*/
            /* LABEL : 20090804-01-��ʼ������������ҵ������
             * MODIFY: huanghl@hundsun.com
             * REASON: FLAG_QZHK��FLAG_YZKHҵ������Ҳ�漰�����п����ͱ䶯
             * HANDLE: ����1024-FLAG_QZHK��1019-FLAG_YZKH����
             */
            if ((strcmp(business_flag,"1001") == 0) ||(strcmp(business_flag,"1019") == 0))
            {
                lOld = 0;
            }
            else
            {
                /*���ɿ�����,0-��ͨ��,1-������,-1-��ѯʧ��*/
                lOld=CheckAcctType(OldCard,&cardparse);
                if (lOld < 0)
                {
                    DBFreePArray2(&psResult2, lnum);
                    DBFreePArray2(&psResult, lCount);
                    SET_ERROR_CODE(ERR_PARAMETER);
                    return -3;
                }
            }
            lNew=CheckAcctType(NewCard,&cardparse);
            if (lNew < 0)
            {
                DBFreePArray2(&psResult2, lnum);
                DBFreePArray2(&psResult, lCount);
                SET_ERROR_CODE(ERR_PARAMETER);
                return -3;
            }
            /*lType��ʾ�¾ɿ��ŵ����Ͳ�*/
            lType=lNew-lOld;
            DBFreePArray2(&psResult2, lnum);
            /*0-����δ�����ı�,����¼*/
            if (lType == 0)
            {
                continue;
            }
            /*1-��ͨ����Ϊ���������¿�������*/
            else if (lType == 1)
            {
                typedetail99901acc_type[0]='1';/*1-������*/
                typedetail99901serial_no = ++lSerialNo;
            }
            /*-1-��������Ϊ��ͨ��*/
            else if (lType == -1)
            {
                typedetail99901acc_type[0]='0';/*0-��ͨ��*/
                typedetail99901serial_no = ++lSerialNo;
            }
                
            if (lNum > 0)
            {
                for(i=0; i<lNum&&i<1000; i++)
                {
                    if (memcmp(typedetail99901secu_branch, sSecuBranch[i], DB_SBRNCH_LEN) == 0)
                    {
                        memcpy(typedetail99901secu_branch, sRealBranch[i], DB_SBRNCH_LEN);
                        break;
                    }
                    else
                        continue;
                }
            }
            
            fprintf(fp,"%8ld|%s|%32s|%8s|%8s|%20s|%60s|%s|%18s|%s|%s|",
                        typedetail99901serial_no,
                        typedetail99901bank_id,
                        typedetail99901secu_use_account,
                        typedetail99901secu_no, 
                        typedetail99901secu_branch,
                        typedetail99901secu_account,
                        typedetail99901client_name,
                        typedetail99901id_kind,
                        typedetail99901id_code,
                        typedetail99901money_kind,
                        typedetail99901acc_type
                    );
            if(cDzfileBusintypeFlag == '1')
            {
                fprintf(fp, "%1s|\n", typedetail99901busin_type);
            }
            else
            {
                fprintf(fp, "\n");
            }
        }
        DBFreePArray2(&psResult, lCount);
        LogTail(sLogName, "      �����ɿͻ��ʻ����ͱ䶯�ļ�...");
    }
    
    fclose(fp);

    return 0;
}           

/******************************************************************************
�������ơ�Gen_DetailFile_1
�������ܡ����ɵ���ȯ�̵��ļ�(��׼�ӿ�)
���������
         iFalg  :
                0  ----��֤ת����ϸ
                1  ----����ʻ������ϸ
�����������
�� �� ֵ�� 0 -- �ɹ�
          <0 -- ʧ��
******************************************************************************/
int Gen_DetailFile_1(TRANS99901 *pub, INITPARAMENT *initPara, char *psSecuNo, char *psBusinType, int iFlag)
{
    long  lCount=0, lCnt=0, i, lRet, lNum;
    char  cForceMigrateFlag;
    char  **psResult, **psResult1;
    char  sSql[SQL_BUF_LEN];
    char  sFileName[512], sFileType[32], sFTypes[251], sBuffer[1024];
    char  sGBMoneyKind[4], sGTBankKind[9], sBusinessFlag[5];
    char  sTransCode[6], sIsoTransSource[2], sIsoTransCode[6];
    char  sSourceSide[1+1], sIdKind[2+1], sBuff[32];
    char  sSecuBranch[1000][DB_SBRNCH_LEN+1], sRealBranch[1000][DB_SBRNCH_LEN+1];
    FILE  *fp;
    TRDETAIL99901  trdetail99901;
    BALDETAIL99901 baldetail99901;
    ACCDETAIL99901 accdetail99901;
    TBPARAM param;

    /*20130326-01-����֤������ת������*/
    memset(sBuffer, 0x00, sizeof(sBuffer));
    sprintf(sBuffer, "%s/etc/secuxml.dat", getenv("HOME"));
    SetDictEnv(sBuffer);

    /*����ת����ˮ�����쳣���������ɶ�����ϸ�ļ�*/
    if (iFlag == 0)
    {
        /*1 �������쳣�����*/
        memset(sSql, 0x00, sizeof(sSql));
        sprintf(sSql, "select secu_no from tbtransfer where secu_no='%s' and status not in ('%s', '%s') and repeal_status in ('1', '2', '3', '4')", psSecuNo, FLOW_SUCCESS, FLOW_FAILED);
        /*sprintf(sSql, "select secu_no from tbtransfer where secu_no='%s' and (repeal_status='2' or repeal_status='4') and (((trans_code='23701' or trans_code='23731') and status not in ('%s','%s','%s','%s')) or (trans_code='23702' and status not in ('%s','%s')))", 
                       psSecuNo, FLOW_SUCCESS, FLOW_FAILED, FLOW_SECU_SEND_F, FLOW_SECU_ERR, FLOW_SUCCESS, FLOW_FAILED);*/
        lCount = DBExecSelect(1, 0, sSql,&psResult);
        DBFreePArray2(&psResult, lCount);
        if ( lCount > 0)
        {
            SET_ERROR_CODE(ERR_FILEBYREPEAL);
            SET_ERROR_MSG("���ڳ����쳣����ˮ");
            PrintLog(LEVEL_ERR,LOG_ERROR_INFO,"���ڳ����쳣����ˮ");
            ERRLOGF("���ڳ����쳣����ˮ[%s]", sSql);
            return -1;
        }
        
        /*2 ����ʽ���ʹ���״̬��һ�µ����*/
        memset(sSql, 0x00, sizeof(sSql));
        sprintf(sSql, "select secu_no from tbtransfer where secu_no='%s' and ((balance_status='0' and status='%s') or (balance_status='1' and status<>'%s'))", psSecuNo, FLOW_SUCCESS, FLOW_SUCCESS);
        lCount = DBExecSelect(1, 0, sSql,&psResult);
        DBFreePArray2(&psResult, lCount);
        if ( lCount > 0)
        {
            SET_ERROR_CODE(ERR_FILEBYREPEAL);
            SET_ERROR_MSG("�����ʽ���ʹ���״̬��һ�µ���ˮ");
            PrintLog(LEVEL_ERR,LOG_ERROR_INFO,"�����ʽ���ʹ���״̬��һ�µ���ˮ");
            ERRLOGF("�����ʽ���ʹ���״̬��һ�µ���ˮ[%s]", sSql);
            return -2;
        }
    }    

    memset(&param, 0x00, sizeof(param));
    memset(sFTypes, 0x00, sizeof(sFTypes));
    if (DBGetParam(psSecuNo, "SECUFILE", &param) == 0)
        sprintf(sFTypes, "%s", param.param_value);
    else
        sprintf(sFTypes, "B_CHK01_YYYYMMDD,B_CHK02_YYYYMMDD,B_CHK03_YYYYMMDD");

    if (iFlag == 0)  /*B_CHK01_YYYYMMDD:ת�˽�����ϸ�����ļ�*/
    {
        strcpy(sFileType, alltrim(GetSubStrViaFlag(sFTypes, 0, ',')));
    }
    else if (iFlag == 1) /*B_CHK02_YYYYMMDD:�ͻ��˻�״̬�����ļ�*/
    {
         strcpy(sFileType, alltrim(GetSubStrViaFlag(sFTypes, 1, ',')));
    }
    else if (iFlag == 2) /*B_CHK03_YYYYMMDD:�˻��ཻ����ϸ�����ļ�*/
    {
         strcpy(sFileType, alltrim(GetSubStrViaFlag(sFTypes, 2, ',')));
    }

    /* �����ļ� */
    memset(sFileName, 0x00, sizeof(sFileName));
    sprintf(sFileName, "%s/%s/qsjg/%s",sSecuDir,psSecuNo,ReplaceYYMMDD(sFileType, pub->lTransDate));
    fp = fopen(sFileName, "w+");
    if ( fp == NULL)
    {
        SET_ERROR_CODE(ERR_CREATFILE);
        PrintLog(LEVEL_ERR, LOG_ERROR_INFO, "�����ļ�ʧ��%s", "");
        return -1;
    }

    /*��ȡȯ���ڲ�Ӫҵ�����*/
    memset(sSql, 0x00, sizeof(sSql));
    /* 20080605-02-��ȡ����ȱʡ��ȯ��Ӫҵ����Ϣ*/
    /*sprintf(sSql, "select secu_branch,real_branch from tbsecubranch where secu_no='%s' and secu_branch<>'%s'", psSecuNo, DEFAULTSBRANCHNO);*/
    sprintf(sSql, "select secu_branch,real_branch from tbsecubranch where secu_no='%s'", psSecuNo);
    lNum = DBExecSelect(1, 0, sSql, &psResult1);
    if ( lNum <= 0)
    {
        lNum = 0;
    }
    else 
    {
        for(i=0; i<lNum&&i<1000; i++)
        {
            memset(sSecuBranch[i], 0x00, sizeof(sSecuBranch[i])-1);
            memset(sRealBranch[i], 0x00, sizeof(sRealBranch[i])-1);
            memcpy(sSecuBranch[i], GetSubStr(psResult1[i],0 ), DB_SBRNCH_LEN);
            memcpy(sRealBranch[i], GetSubStr(psResult1[i],1 ), DB_SBRNCH_LEN);
        }
        DBFreePArray2(&psResult1, lNum);
    }
    
    /*
     *  ����SQL���
     */
    memset(sSql, 0x00, sizeof(sSql));
    if (iFlag == 0)
    {
         sprintf(sSql,"select %s %s %s from tbtransfer",
                       "bank_id,secu_no,secu_branch,hs_date,hs_time, ",
                       "serial_no,secu_serial,secu_use_account,secu_account, ",
                       "source_side,trans_code,money_kind,occur_balance" );
        sprintf(sSql,"%s where secu_no='%s' and hs_date=%ld and status='S'", sSql, psSecuNo, pub->lTransDate);
        if (strlen(alltrim(psBusinType)) > 0)
            sprintf(sSql,"%s and busin_type='%c'", sSql, psBusinType[0]);
        /*sprintf(sSql,"%s and ((error_code='0000' and status='S') or ((trans_code='23701' or trans_code='23731') and (status='D' or status='C' or status='I')))", sSql);*/
    }
    else if (iFlag == 1)
    {
        /* 20080605-01-�Ż��˻�״̬�ļ���SQL���*/
        /****
        sprintf(sSql,"select a.bank_id, a.secu_no, a.secu_branch, a.open_date, a.secu_use_account, a.secu_account, a.client_name, a.money_kind, a.status from tbclientyzzz a,tbaccountedit b"); 
        sprintf(sSql,"%s where a.secu_no='%s'", sSql, psSecuNo);
        sprintf(sSql,"%s and a.secu_no=b.secu_no and a.secu_account=b.secu_account and a.busin_type=b.busin_type and a.money_kind=b.money_kind", sSql);
        /--* 20080523-01-�˻�״̬�ļ��б������ɵ���ǩԼ�Ķ�����¼ *--/
        /--*sprintf(sSql,"%s and (a.open_date=%ld or (a.open_date<%ld and a.status='0' and b.business_flag=%ld))", sSql, pub->lTransDate, pub->lTransDate, FLAG_YZKH);*--/
        sprintf(sSql,"%s and ((a.open_date=%ld and b.business_flag in (%ld,%ld,%ld,%ld,%ld)) or (a.open_date<%ld and a.status='0' and b.business_flag=%ld))", 
                      sSql, pub->lTransDate, FLAG_KHKH, FLAG_KHXH, FLAG_ZZKH, FLAG_ZZXH, FLAG_YYZD,pub->lTransDate, FLAG_YZKH);
        ****/
        sprintf(sSql,"select distinct a.bank_id, a.secu_no, a.secu_branch, a.open_date, a.secu_use_account, a.secu_account, a.client_name, a.money_kind, a.status from tbclientyzzz a,tbaccountedit b"); 
        sprintf(sSql,"%s where a.secu_no='%s'", sSql, psSecuNo);
        sprintf(sSql,"%s and a.secu_no=b.secu_no and a.secu_account=b.secu_account and a.busin_type=b.busin_type and a.money_kind=b.money_kind", sSql);
        if (strlen(alltrim(psBusinType)) > 0)
            sprintf(sSql,"%s and a.busin_type='%c'", sSql, psBusinType[0]);

        /*20080522-01-����Ԥָ���ͻ���֤ȯ��Ϊ�����ͻ�*/
        memset(&param, 0x00, sizeof(TBPARAM));
        DBGetParam(psSecuNo,"FORCE_MIGRATE_STATUS", &param);
        /*Ԥָ�������п�ʱ�Ƿ�ǿ�ƿͻ�״̬ΪԤָ��״̬:0-��(Ĭ��),1-��*/
        if ( param.param_value[0] == '1')
            cForceMigrateFlag = '1';
        else
            cForceMigrateFlag = '0';

    }
    else if (iFlag == 2)
    {
        sprintf(sSql,"select bank_id,secu_no,secu_branch,hs_date,hs_time,serial_no,secu_serial,client_account,secu_account,client_name,id_kind,id_code,trans_code,business_flag,money_kind,occur_balance,secu_use_account,fund_account from tbaccountedit"); 
        sprintf(sSql,"%s where secu_no='%s'", sSql, psSecuNo);
        if (strlen(alltrim(psBusinType)) > 0)
            sprintf(sSql,"%s and busin_type='%c'", sSql, psBusinType[0]);
    }
    

    lCount = DBExecSelect(1, 0, sSql,&psResult);
    if ( lCount < 0)
    {
        SET_ERROR_CODE(ERR_DATABASE);
        return -2;
    }

    if (iFlag == 0)
    {
        for(lCnt = 0; lCnt < lCount; lCnt ++)
        {
            /*20080530-01-��־�汾��ת����ϸ�ṹ���ʼ��*/
            memset(&trdetail99901, 0x00, sizeof(trdetail99901));
            memset(sGTBankKind, 0x00, sizeof(sGTBankKind));
            memset(sGBMoneyKind, 0x00, sizeof(sGBMoneyKind));
            /* LABEL : 20091123-01-��ʼ��sTransCode����
             * MODIFY: huayl@hundsun.com
             * REASON: δ��ʼ�������������ļ�������
             * HANDLE: ��ʼ������
             */ 
            memset(sTransCode, 0x00, sizeof(sTransCode));
            /*20110819-01-��ʼ��sSourceSide*/
            memset(sSourceSide, 0x00, sizeof(sSourceSide));

            memcpy(trdetail99901bank_kind       , (char *)GetSubStr(psResult[lCnt],0 ) , sizeof(trdetail99901bank_kind   ) - 1);
            Get1ViaBKind(trdetail99901bank_kind, sGTBankKind);
            memcpy(trdetail99901secu_no         , (char *)GetSubStr(psResult[lCnt],1 ) , sizeof(trdetail99901secu_no       ) - 1);
            memcpy(sBuff, (char *)GetSubStr(psResult[lCnt],2 ), DB_SBRNCH_LEN);
            memcpy(trdetail99901secu_branch, sBuff, DB_SBRNCH_LEN);
            if (lNum > 0)
            {
                for(i=0; i<lNum&&i<1000; i++)
                {
                    if (memcmp(sBuff, sSecuBranch[i], DB_SBRNCH_LEN) == 0)
                    {
                        memset(trdetail99901secu_branch, 0x00, sizeof(trdetail99901secu_branch));
                        memcpy(trdetail99901secu_branch, sRealBranch[i], DB_SBRNCH_LEN);
                        break;
                    }
                    else
                        continue;
                }
            }
            trdetail99901hs_date                = atol((char *)GetSubStr(psResult[lCnt],3 ) );
            trdetail99901hs_time                = atol((char *)GetSubStr(psResult[lCnt],4 ) );
            memcpy(trdetail99901serial_no       , (char *)GetSubStr(psResult[lCnt],5 ) , sizeof(trdetail99901serial_no     ) - 1);
            memcpy(trdetail99901secu_serial     , (char *)GetSubStr(psResult[lCnt],6 ) , sizeof(trdetail99901secu_serial   ) - 1);
            memcpy(trdetail99901client_account  , (char *)GetSubStr(psResult[lCnt],7 ) , sizeof(trdetail99901client_account) - 1);
            memcpy(trdetail99901secu_account    , (char *)GetSubStr(psResult[lCnt],8 ) , sizeof(trdetail99901secu_account  ) - 1);
            memcpy(trdetail99901source_side     , (char *)GetSubStr(psResult[lCnt],9 ) , sizeof(trdetail99901source_side   ) - 1);
            if (trdetail99901source_side[0] == '0')
                sSourceSide[0] = 'S';
            else
                sSourceSide[0] = 'B';
            memcpy(trdetail99901trans_code      , (char *)GetSubStr(psResult[lCnt],10) , sizeof(trdetail99901trans_code    ) - 1);
            if ((memcmp(trdetail99901trans_code, "24701", 5) == 0) ||
                (memcmp(trdetail99901trans_code, "23701", 5) == 0) ||
                (memcmp(trdetail99901trans_code, "23731", 5) == 0) )
            {
                memcpy(sTransCode, "12001", 5);
            }
            else
            if ((memcmp(trdetail99901trans_code, "24702", 5) == 0) ||
                (memcmp(trdetail99901trans_code, "23702", 5) == 0))
            {
                memcpy(sTransCode, "12002", 5);
            }
            memcpy(trdetail99901money_kind      , (char *)GetSubStr(psResult[lCnt],11 ) , sizeof(trdetail99901money_kind    ) - 1);
            GetISOViaMKind(trdetail99901money_kind, sGBMoneyKind);
            trdetail99901occur_balance          = atof( (char *)GetSubStr(psResult[lCnt],12 ) );

            /*д�ļ�*/
            fprintf(fp,"%-8s|%-8s|%-4s|%-8ld|%-6ld|%-20s|%-20s|%-32s|%-14s|%-32s|%1s|%5s|%-3s|%1s|%016.0lf|\n",
                        sGTBankKind                 ,
                        trdetail99901secu_no       ,
                        trdetail99901secu_branch   ,
                        trdetail99901hs_date       , 
                        trdetail99901hs_time       ,
                        trdetail99901serial_no     ,
                        trdetail99901secu_serial   ,
                        trdetail99901client_account,
                        trdetail99901secu_account  ,
                        " "                         ,
                        sSourceSide                 ,
                        sTransCode                  ,
                        sGBMoneyKind                ,
                        "2"                         ,       
                        trdetail99901occur_balance*100
                    );

        }
    }
    else if (iFlag == 1)
    {
        for(lCnt = 0; lCnt < lCount; lCnt ++)
        {
            memset(&accdetail99901, 0x00, sizeof(accdetail99901));
            memset(sGTBankKind, 0x00, sizeof(sGTBankKind));
            memset(sGBMoneyKind, 0x00, sizeof(sGBMoneyKind));

            memcpy(accdetail99901bank_id         , (char *)GetSubStr(psResult[lCnt],0 ), sizeof(accdetail99901bank_id) - 1);
            Get1ViaBKind(accdetail99901bank_id, sGTBankKind);
            memcpy(accdetail99901secu_no         , (char *)GetSubStr(psResult[lCnt],1 ), sizeof(accdetail99901secu_no) - 1);
            memcpy(sBuff, (char *)GetSubStr(psResult[lCnt],2 ), DB_SBRNCH_LEN);
            memcpy(accdetail99901secu_branch, sBuff, DB_SBRNCH_LEN);
            if (lNum > 0)
            {
                for(i=0; i<lNum&&i<1000; i++)
                {
                    if (memcmp(sBuff, sSecuBranch[i], DB_SBRNCH_LEN) == 0)
                    {
                        memset(accdetail99901secu_branch, 0x00, sizeof(accdetail99901secu_branch));
                        memcpy(accdetail99901secu_branch, sRealBranch[i], DB_SBRNCH_LEN);
                        break;
                    }
                    else
                        continue;
                }
            }
            accdetail99901hs_date           = atol((char *)GetSubStr(psResult[lCnt],3 )) ;
            memcpy(accdetail99901client_account  , (char *)GetSubStr(psResult[lCnt],4 ), sizeof(accdetail99901client_account) - 1);                                          
            memcpy(accdetail99901secu_account    , (char *)GetSubStr(psResult[lCnt],5 ), sizeof(accdetail99901secu_account) - 1);                                          
            memcpy(accdetail99901client_name     , (char *)GetSubStr(psResult[lCnt],6 ), sizeof(accdetail99901client_name) - 1);
            memcpy(accdetail99901money_kind      , (char *)GetSubStr(psResult[lCnt],7 ), sizeof(accdetail99901money_kind) - 1);
            GetISOViaMKind(accdetail99901money_kind, sGBMoneyKind);
            strcpy(accdetail99901status          , (char *)GetSubStr(psResult[lCnt],8 ));
            if (accdetail99901status[0] == '0')
                accdetail99901status[0] = '0';
            else if (accdetail99901status[0] == '5')
            {
                /*20080522-01-����Ԥָ���ͻ���֤ȯ��Ϊ�����ͻ�*/
                /*�����Ų���ǿ��Ԥָ��״̬������ȯ��Ϊ����״̬*/
                if ((strlen(alltrim(accdetail99901client_account)) > 0) 
                 && (cForceMigrateFlag == '1'))
                    accdetail99901status[0] = '0';
                else
                    accdetail99901status[0] = '1';
            }
            else if (accdetail99901status[0] == '3')
                accdetail99901status[0] = '2';
            else
                accdetail99901status[0] = '0';
            
            fprintf(fp,"%-8s|%-8s|%-4s|%-8ld|%-32s|%-14s|%-32s|%-3s|%1s|%1s|\n",
                     sGTBankKind,
                     accdetail99901secu_no,
                     accdetail99901secu_branch,
                     accdetail99901hs_date,
                     accdetail99901client_account,
                     accdetail99901secu_account,
                     accdetail99901client_name,
                     sGBMoneyKind,
                     "2",  /*20080522-02-������׼�ӿ��еĳ����־*/
                     accdetail99901status
                      );
        }
    }
    else if (iFlag == 2)
    {
        for(lCnt = 0; lCnt < lCount; lCnt ++)
        {
            memset(&accdetail99901, 0x00, sizeof(accdetail99901));
            memset(sGTBankKind, 0x00, sizeof(sGTBankKind));
            memset(sIdKind, 0x00, sizeof(sIdKind));
            memset(sBusinessFlag, 0x00, sizeof(sBusinessFlag));
            memset(sTransCode, 0x00, sizeof(sTransCode));
            memset(sIsoTransSource, 0x00, sizeof(sIsoTransSource));
            memset(sIsoTransCode, 0x00, sizeof(sIsoTransCode));
            memset(sGBMoneyKind, 0x00, sizeof(sGBMoneyKind));
            
            memcpy(accdetail99901bank_id         , (char *)GetSubStr(psResult[lCnt],0 ), 1);
            Get1ViaBKind(accdetail99901bank_id, sGTBankKind);
            memcpy(accdetail99901secu_no         , (char *)GetSubStr(psResult[lCnt],1 ), sizeof(accdetail99901secu_no) - 1);
            memcpy(sBuff, (char *)GetSubStr(psResult[lCnt],2 ), DB_SBRNCH_LEN);
            memcpy(accdetail99901secu_branch, sBuff, DB_SBRNCH_LEN);
            if (lNum > 0)
            {
                for(i=0; i<lNum&&i<1000; i++)
                {
                    if (memcmp(sBuff, sSecuBranch[i], DB_SBRNCH_LEN) == 0)
                    {
                        memset(accdetail99901secu_branch, 0x00, sizeof(accdetail99901secu_branch));
                        memcpy(accdetail99901secu_branch, sRealBranch[i], DB_SBRNCH_LEN);
                        break;
                    }
                    else
                        continue;
                }
            }
            accdetail99901hs_date = atol((char *)GetSubStr(psResult[lCnt],3 )) ;
            accdetail99901hs_time = atol((char *)GetSubStr(psResult[lCnt],4 )) ;
            accdetail99901serial_no = atol((char *)GetSubStr(psResult[lCnt],5)); 
            memcpy(accdetail99901secu_serial     , (char *)GetSubStr(psResult[lCnt],6 ), sizeof(accdetail99901secu_serial) - 1); 
            memcpy(accdetail99901client_account  , (char *)GetSubStr(psResult[lCnt],7 ), sizeof(accdetail99901client_account) - 1);                                          
            memcpy(accdetail99901secu_account    , (char *)GetSubStr(psResult[lCnt],8 ), sizeof(accdetail99901secu_account) - 1);                                          
            memcpy(accdetail99901client_name     , (char *)GetSubStr(psResult[lCnt],9 ), sizeof(accdetail99901client_name) - 1);
            memcpy(accdetail99901id_kind         , (char *)GetSubStr(psResult[lCnt],10), 1);
            
            /* 20130326-01-����֤������ת������ */
            LookUp(104, sIdKind, accdetail99901id_kind);
            if ( strlen(alltrim(sIdKind)) == 0 )
            {
                Get1ViaIdKind(accdetail99901id_kind, sIdKind);
            }
            
            memcpy(accdetail99901id_code         , (char *)GetSubStr(psResult[lCnt],11), sizeof(accdetail99901id_code) - 1);
            memcpy(sBusinessFlag, (char *)GetSubStr(psResult[lCnt],13), 4);
            strcpy(sTransCode, (char *)GetSubStr(psResult[lCnt],12 ));
            /*�����ַ��еĴ�ܼ���25730����*/
            if (memcmp(sTransCode, "25730", 5) == 0)
                continue;
            
            /* 20141014-wangsy-������������������ˮ */
            if (memcmp(sTransCode, "20922", 5) == 0)
                continue;
            
            Get1TransViaTCode(sTransCode, sIsoTransSource, sIsoTransCode);
            /*��׼���˻�������ϸ�ļ���Ԥָ������������⴦��*/
            if ((memcmp(sTransCode, "26701", 5) == 0) && (memcmp(sBusinessFlag, "1022", 4) == 0))
                memcpy(sIsoTransCode, "11002", 5);
            else if ((memcmp(sTransCode, "25701", 5) == 0) && (memcmp(sBusinessFlag, "1019", 4) == 0))
                memcpy(sIsoTransCode, "11003", 5);
                                
            memcpy(accdetail99901money_kind      , (char *)GetSubStr(psResult[lCnt],14 ), 1);
            GetISOViaMKind(accdetail99901money_kind, sGBMoneyKind);
            accdetail99901occur_bala = atof((char *)GetSubStr(psResult[lCnt],15 ));
            memcpy(accdetail99901secu_use_account  , (char *)GetSubStr(psResult[lCnt],16 ), sizeof(accdetail99901secu_use_account) - 1);                                          
            
            fprintf(fp,"%-8s|%-8s|%-4s|%-8ld|%-6ld|%-20ld|%-20s|%-22s|%-32s|%-14s|%-32s|%-2s|%-20s|%1s|%-5s|%-3s|%1s|%16.0lf|\n",
                        sGTBankKind,
                        accdetail99901secu_no, 
                        accdetail99901secu_branch,
                        accdetail99901hs_date,
                        accdetail99901hs_time,
                        accdetail99901serial_no,         
                        accdetail99901secu_serial,         
                        accdetail99901secu_use_account,
                        accdetail99901client_account,
                        accdetail99901secu_account,
                        accdetail99901client_name,
                        sIdKind,
                        accdetail99901id_code,
                        sIsoTransSource,
                        sIsoTransCode,
                        sGBMoneyKind,
                        "2", /*20080522-02-������׼�ӿ��еĳ����־*/
                        accdetail99901occur_bala*100
                    );
        }
    }

    fclose(fp);
    DBFreePArray2(&psResult, lCount);
    
    return 0;
}
            
/******************************************************************************
�������ơ�Gen_DetailFile_2
�������ܡ����ɵ���ȯ�̵�ת����ϸ�ļ�(��̩�ӿ�)
���������
         iFalg  :
                0  ----��֤ת����ϸ
                1  ----����ʻ������ϸ
�����������
�� �� ֵ�� 0 -- �ɹ�
          <0 -- ʧ��
******************************************************************************/
int Gen_DetailFile_2(TRANS99901 *pub, INITPARAMENT *initPara, char *psSecuNo, char *psBusinType, int iFlag)
{
    long  lCount=0, lCnt=0, i, lRet, lNum,lSerialNo,lnum = 0;
    long  lType=0,lNew,lOld;
    long  lZDBranch = 0,lJHBranch = 0;
    char  cDzfileBusintypeFlag;
    char  cForceMigrateFlag;
    char  **psResult, **psResult1, **psResult2;
    char  sSql[SQL_BUF_LEN];
    char  sRuleFlag[250], business_flag[4+1],OldCard[32+1],NewCard[32+1];
    char  sFileName[512], sFileType[32], sFTypes[251];
    char  sGBMoneyKind[4], sGTBankKind[5], sBusinessFlag[5];
    char  sTransCode[6], gtTransSource[2], gtTransCode[4], sBuff[32];
    char  sSecuBranch[1000][DB_SBRNCH_LEN+1], sRealBranch[1000][DB_SBRNCH_LEN+1];
    struct CARDPARSE cardparse;
    FILE  *fp;
    TRDETAIL99901  trdetail99901;
    BALDETAIL99901 baldetail99901;
    ACCDETAIL99901 accdetail99901;
    TYPEDETAIL99901 typedetail99901;
    PRECDETAIL99901 precdetail99901;
    TBPARAM param;

    /*����ת����ˮ�����쳣���������ɶ�����ϸ�ļ�*/
    if (iFlag == 0)
    {
        /*1 �������쳣�����*/
        memset(sSql, 0x00, sizeof(sSql));
        sprintf(sSql, "select secu_no from tbtransfer where secu_no='%s' and status not in ('%s', '%s') and repeal_status in ('1', '2', '3', '4')", psSecuNo, FLOW_SUCCESS, FLOW_FAILED);
        /*sprintf(sSql, "select secu_no from tbtransfer where secu_no='%s' and (repeal_status='2' or repeal_status='4') and (((trans_code='23701' or trans_code='23731') and status not in ('%s','%s','%s','%s')) or (trans_code='23702' and status not in ('%s','%s')))", 
                       psSecuNo, FLOW_SUCCESS, FLOW_FAILED, FLOW_SECU_SEND_F, FLOW_SECU_ERR, FLOW_SUCCESS, FLOW_FAILED);*/
        lCount = DBExecSelect(1, 0, sSql,&psResult);
        DBFreePArray2(&psResult, lCount);
        if ( lCount > 0)
        {
            SET_ERROR_CODE(ERR_FILEBYREPEAL);
            SET_ERROR_MSG("���ڳ����쳣����ˮ");
            PrintLog(LEVEL_ERR,LOG_ERROR_INFO,"���ڳ����쳣����ˮ");
            ERRLOGF("���ڳ����쳣����ˮ[%s]", sSql);
            return -1;
        }
        
        /*2 ����ʽ���ʹ���״̬��һ�µ����*/
        memset(sSql, 0x00, sizeof(sSql));
        sprintf(sSql, "select secu_no from tbtransfer where secu_no='%s' and ((balance_status='0' and status='%s') or (balance_status='1' and status<>'%s'))", psSecuNo, FLOW_SUCCESS, FLOW_SUCCESS);
        lCount = DBExecSelect(1, 0, sSql,&psResult);
        DBFreePArray2(&psResult, lCount);
        if ( lCount > 0)
        {
            SET_ERROR_CODE(ERR_FILEBYREPEAL);
            SET_ERROR_MSG("�����ʽ���ʹ���״̬��һ�µ���ˮ");
            PrintLog(LEVEL_ERR,LOG_ERROR_INFO,"�����ʽ���ʹ���״̬��һ�µ���ˮ");
            ERRLOGF("�����ʽ���ʹ���״̬��һ�µ���ˮ[%s]", sSql);
            return -2;
        }
    }

    memset(&param, 0x00, sizeof(param));
    memset(sFTypes, 0x00, sizeof(sFTypes));
    if (DBGetParam(psSecuNo, "SECUFILE", &param) == 0)
        sprintf(sFTypes, "%s", param.param_value);
    else
        sprintf(sFTypes, "B01YYYYMMDD,B02YYYYMMDD,B03YYYYMMDD");

    if (iFlag == 0)
    {
        strcpy(sFileType, alltrim(GetSubStrViaFlag(sFTypes, 0, ',')));
    }
    else if (iFlag == 1)
    {
         strcpy(sFileType, alltrim(GetSubStrViaFlag(sFTypes, 1, ',')));
    }
    else if (iFlag == 2)
    {
         strcpy(sFileType, alltrim(GetSubStrViaFlag(sFTypes, 2, ',')));
    }
    /* LABEL : 20090819-01-���ӹ�̩��ʽԤԼ�������������ļ�
     * MODIFY: huanghl@hundsun.com
     * REASON: ����ԤԼ�������������ļ�����
     * HANDLE: ȡԤԼ�������������ļ���
     */
    else if (iFlag == 5)
    {
         strcpy(sFileType, alltrim(GetSubStrViaFlag(sFTypes, 5, ',')));
    }
    else if (iFlag == 6)
    {
         strcpy(sFileType, alltrim(GetSubStrViaFlag(sFTypes, 6, ',')));
    }
    
    /*�������ļ���Ϊ�գ��򲻴���*/
    if (strlen(alltrim(sFileType)) == 0)
    {
        return 0;
    }
    /* �����ļ� */
    memset(sFileName, 0x00, sizeof(sFileName));
    sprintf(sFileName, "%s/%s/qsjg/%s",sSecuDir,psSecuNo,ReplaceYYMMDD(sFileType, pub->lTransDate));
    fp = fopen(sFileName, "w+");
    if ( fp == NULL)
    {
        SET_ERROR_CODE(ERR_CREATFILE);
        PrintLog(LEVEL_ERR, LOG_ERROR_INFO, "�����ļ�ʧ��%s", "");
        return -1;
    }
    
    /* LABEL : 20090819-01-���ӹ�̩��ʽԤԼ�������������ļ�
     * MODIFY: huanghl@hundsun.com
     * REASON: ����ԤԼ�������������ļ�����
     * HANDLE: ���ݲ����ж��Ƿ���Ҫ��ҵ������ֶ�
     */
    cDzfileBusintypeFlag='0';   /* Ĭ���ļ�����Ҫ����ҵ������ֶ� */
    memset(&param, 0x00, sizeof(param));
    if (DBGetParam(psSecuNo, "RZ_DZFILE_BUSINTYPE", &param) == 0)
    {
        cDzfileBusintypeFlag = param.param_value[0];
    }
    
    /*��ȡȯ���ڲ�Ӫҵ�����*/
    memset(sSql, 0x00, sizeof(sSql));
    /* 20080605-02-��ȡ����ȱʡ��ȯ��Ӫҵ����Ϣ*/
    /*sprintf(sSql, "select secu_branch,real_branch from tbsecubranch where secu_no='%s' and secu_branch<>'%s'", psSecuNo, DEFAULTSBRANCHNO);*/
    sprintf(sSql, "select secu_branch,real_branch from tbsecubranch where secu_no='%s'", psSecuNo);
    lNum = DBExecSelect(1, 0, sSql, &psResult1);
    if ( lNum <= 0)
    {
        lNum = 0;
    }
    else 
    {
        for(i=0; i<lNum&&i<1000; i++)
        {
            memset(sSecuBranch[i], 0x00, sizeof(sSecuBranch[i])-1);
            memset(sRealBranch[i], 0x00, sizeof(sRealBranch[i])-1);
            memcpy(sSecuBranch[i], GetSubStr(psResult1[i],0 ), DB_SBRNCH_LEN);
            memcpy(sRealBranch[i], GetSubStr(psResult1[i],1 ), DB_SBRNCH_LEN);
        }
        DBFreePArray2(&psResult1, lNum);
    }

    /*
     *  ����SQL���
     */
    memset(sSql, 0x00, sizeof(sSql));
    if (iFlag == 0)
    {
         sprintf(sSql,"select %s %s %s from tbtransfer",
                       "bank_id,secu_no,secu_branch,hs_date,hs_time, ",
                       "serial_no,secu_serial,secu_use_account,secu_account, ",
                       "source_side,trans_code,money_kind,occur_balance" );
        sprintf(sSql,"%s where secu_no='%s' and hs_date=%ld and status='S'", sSql, psSecuNo, pub->lTransDate);
        if (strlen(alltrim(psBusinType)) > 0)
            sprintf(sSql,"%s and busin_type='%c'", sSql, psBusinType[0]);
        /*sprintf(sSql,"%s and ((error_code='0000' and status='S') or ((trans_code='23701' or trans_code='23731') and (status='D' or status='C' or status='I')))", sSql);*/
    }
    else if (iFlag == 1)
    {
        /* LABEL : 20110506-02-��������Ԥָ����������������
         * MODIFY: huanghl@hundsun.com
         * REASON: ��������������������Ԥָ����������������
         * HANDLE������FLAG_XYZD-Ԥָ������
         */
        /* 20080708-01-��֤�˻�״̬�ļ���¼Ψһ������distinct*/
        sprintf(sSql,"select distinct a.bank_id, a.secu_no, a.secu_branch, a.open_date, a.secu_use_account, a.secu_account, a.client_name, a.money_kind, a.status from tbclientyzzz a,tbaccountedit b"); 
        sprintf(sSql,"%s where a.secu_no='%s'", sSql, psSecuNo);
        sprintf(sSql,"%s and a.secu_no=b.secu_no and a.secu_account=b.secu_account and a.busin_type=b.busin_type and a.money_kind=b.money_kind", sSql);
        /* 20080523-01-�˻�״̬�ļ��б������ɵ���ǩԼ�Ķ�����¼ */
        /*sprintf(sSql,"%s and (a.open_date=%ld or (a.open_date<%ld and a.status='0' and b.business_flag=%ld))", sSql, pub->lTransDate, pub->lTransDate, FLAG_YZKH);*/
        /* 20120929-01-�����������ڣ�ͨ���������� */
        if (pub->cNeedModify != '0')
        {
            /*20130916-01-���¸�ֵ�궨��Ϊ�����͵�business_flag�ֶ� %ld->%d */
            sprintf(sSql,"%s and ((a.open_date=%ld and b.business_flag in (%d,%d,%d,%d,%d,%d)) or (a.open_date<%ld and a.status='0' and b.business_flag=%d))", 
                      sSql, pub->lTransDate, FLAG_KHKH, FLAG_KHXH, FLAG_ZZKH, FLAG_ZZXH, FLAG_YYZD, FLAG_XYZD, pub->lTransDate, FLAG_YZKH);
        }
        else
        {
            /*20130916-01-���¸�ֵ�궨��Ϊ�����͵�business_flag�ֶ� %ld->%d */
            sprintf(sSql,"%s and ((a.open_date=%ld and b.business_flag in (%d,%d,%d)) or (a.open_date<%ld and a.status='0' and b.business_flag=%d) or (a.open_date<%ld and a.status='3' and b.business_flag in (%d,%d,%d)) )", 
                      sSql, pub->lTransDate, FLAG_KHKH, FLAG_ZZKH, FLAG_YYZD, pub->lTransDate, FLAG_YZKH, pub->lTransDate, FLAG_KHXH, FLAG_XYZD, FLAG_ZZXH);
        }
        
        if (strlen(alltrim(psBusinType)) > 0)
            sprintf(sSql,"%s and a.busin_type='%c'", sSql, psBusinType[0]);

        /*20080522-01-����Ԥָ���ͻ���֤ȯ��Ϊ�����ͻ�*/
        memset(&param, 0x00, sizeof(TBPARAM));
        DBGetParam(psSecuNo,"FORCE_MIGRATE_STATUS", &param);
        /*Ԥָ�������п�ʱ�Ƿ�ǿ�ƿͻ�״̬ΪԤָ��״̬:0-��(Ĭ��),1-��*/
        if ( param.param_value[0] == '1')
            cForceMigrateFlag = '1';
        else
            cForceMigrateFlag = '0';

    }
    else if (iFlag == 2)
    {
        /* LABEL : 20110506-02-��������Ԥָ����������������
         * MODIFY: huanghl@hundsun.com
         * REASON: ��������������������Ԥָ����������������
         * HANDLE������1032-Ԥָ������
         */
        sprintf(sSql,"select bank_id,secu_no,secu_branch,hs_date,hs_time,serial_no,secu_serial,client_account,secu_account,client_name,trans_code,business_flag,money_kind,occur_balance,secu_use_account from tbaccountedit"); 
        sprintf(sSql,"%s where secu_no='%s'", sSql, psSecuNo);
        if (strlen(alltrim(psBusinType)) > 0)
            sprintf(sSql,"%s and busin_type='%c'", sSql, psBusinType[0]);
        sprintf(sSql,"%s and business_flag in (1001,1002,1004,1019,1022,1032)", sSql);
    }
    /* LABEL : 20090819-01-���ӹ�̩��ʽԤԼ�������������ļ�
     * MODIFY: huanghl@hundsun.com
     * REASON: ����ԤԼ�������������ļ�����
     * HANDLE: ���ӹ�̩��ʽԤԼ�������������ļ�����
     */
    else if (iFlag == 5)
    {
        sprintf(sSql,"select distinct a.bank_id,a.secu_use_account,a.secu_no,a.secu_branch_b,a.secu_branch_s,a.secu_account,a.client_name,a.id_kind,a.id_code,a.money_kind,a.status,a.busin_type from tbpreacc a,tbaccountedit b"); 
        sprintf(sSql,"%s where a.secu_no='%s'", sSql, psSecuNo);
        sprintf(sSql,"%s and a.secu_no=b.secu_no and a.secu_account=b.secu_account and a.busin_type=b.busin_type and a.money_kind=b.money_kind and a.fund_account=b.fund_account", sSql);
        /*20130916-01-���¸�ֵ�궨��Ϊ�����͵�business_flag�ֶ� %ld->%d */
        sprintf(sSql,"%s and ((a.active_date=%ld and b.business_flag in (%d,%d)) or (a.pre_date=%ld and b.business_flag=%d))", 
                      sSql, pub->lTransDate, FLAG_KHKH, FLAG_YZKH, pub->lTransDate, FLAG_YDJH);
        if (strlen(alltrim(psBusinType)) > 0)
            sprintf(sSql,"%s and a.busin_type='%c'", sSql, psBusinType[0]);
    }
    else if (iFlag == 6)
    {
        memset(&param, 0x00, sizeof(TBPARAM));
        memset(sRuleFlag, 0x00, sizeof(sRuleFlag));
        /*CHKGET_ACCTYPE_RUAL�����������������ָ��ȯ��δ��˲�����ϵͳ����������Ϊ׼,����Ϊ���������ļ�*/
        if (DBGetParam(psSecuNo, "CHKGET_ACCTYPE_RUAL", &param) == 0)
        {
            strcpy(sRuleFlag,param.param_value);
        }
        else
        {
            memset(&param, 0x00, sizeof(TBPARAM));
            if (DBGetParam(DEFAULTSECUNO, "CHKGET_ACCTYPE_RUAL", &param) == 0)
            {
                strcpy(sRuleFlag,param.param_value);
            }
        }
        if (strlen(alltrim(sRuleFlag)) == 0)
        {
            return 0;
        }
        
        /*�����˻����͵�����*/
        /* LABEL : 20090901-01-���ӱ���cardparse��ʼ��
         * MODIFY: wanggk@hundsun.com
         * REASON: ����cardparseδ��ʼ��
         * HANDLE: ���ӱ���cardparse��ʼ��
         */ 
        memset(&cardparse, 0x00, sizeof(cardparse)); 
        ParseParam(sRuleFlag, &cardparse);
        
        memset(sSql, 0x00, sizeof(sSql));
        sprintf(sSql,"select distinct c.secu_no,c.secu_account,c.money_kind,c.busin_type,c.bank_id,c.secu_use_account,c.client_account,c.secu_branch,c.client_name,c.id_kind,c.id_code from tbaccountedit a ,tbclientyzzz c");
        sprintf(sSql,"%s where a.secu_no=c.secu_no and a.secu_account=c.secu_account and a.busin_type=c.busin_type",sSql);
        sprintf(sSql,"%s and a.business_flag in(1001,1004,1024,1019) and c.status='0' and a.secu_no='%s' and a.hs_date =%ld", sSql, psSecuNo,pub->lTransDate);
        if (strlen(alltrim(psBusinType)) > 0)
            sprintf(sSql,"%s and a.busin_type='%c'", sSql, psBusinType[0]);
    }
    
    lCount = DBExecSelect(1, 0, sSql,&psResult);
    if ( lCount < 0)
    {
        SET_ERROR_CODE(ERR_DATABASE);
        return -2;
    }

    if (iFlag == 0)
    {
        for(lCnt = 0; lCnt < lCount; lCnt ++)
        {
            memset(&trdetail99901, 0x00, sizeof(trdetail99901));
            memset(sGTBankKind, 0x00, sizeof(sGTBankKind));
            memset(sGBMoneyKind, 0, sizeof(sGBMoneyKind));

            memcpy(trdetail99901bank_kind       , (char *)GetSubStr(psResult[lCnt],0 ) , sizeof(trdetail99901bank_kind   ) - 1);
            Get2ViaBKind(trdetail99901bank_kind, sGTBankKind);
            memcpy(trdetail99901secu_no         , (char *)GetSubStr(psResult[lCnt],1 ) , sizeof(trdetail99901secu_no       ) - 1);
            memcpy(sBuff, (char *)GetSubStr(psResult[lCnt],2 ), DB_SBRNCH_LEN);
            memcpy(trdetail99901secu_branch, sBuff, DB_SBRNCH_LEN);
            if (lNum > 0)
            {
                for(i=0; i<lNum&&i<1000; i++)
                {
                    if (memcmp(sBuff, sSecuBranch[i], DB_SBRNCH_LEN) == 0)
                    {
                        memset(trdetail99901secu_branch, 0x00, sizeof(trdetail99901secu_branch));
                        memcpy(trdetail99901secu_branch, sRealBranch[i], DB_SBRNCH_LEN);
                        break;
                    }
                    else
                        continue;
                }
            }
            trdetail99901hs_date                = atol((char *)GetSubStr(psResult[lCnt],3 ) );
            trdetail99901hs_time                = atol((char *)GetSubStr(psResult[lCnt],4 ) );
            memcpy(trdetail99901serial_no       , (char *)GetSubStr(psResult[lCnt],5 ) , sizeof(trdetail99901serial_no     ) - 1);
            memcpy(trdetail99901secu_serial     , (char *)GetSubStr(psResult[lCnt],6 ) , sizeof(trdetail99901secu_serial   ) - 1);
            memcpy(trdetail99901client_account  , (char *)GetSubStr(psResult[lCnt],7 ) , sizeof(trdetail99901client_account) - 1);
            memcpy(trdetail99901secu_account    , (char *)GetSubStr(psResult[lCnt],8 ) , sizeof(trdetail99901secu_account  ) - 1);
            memcpy(trdetail99901source_side     , (char *)GetSubStr(psResult[lCnt],9 ) , sizeof(trdetail99901source_side   ) - 1);
            memcpy(trdetail99901trans_code      , (char *)GetSubStr(psResult[lCnt],10) , sizeof(trdetail99901trans_code    ) - 1);
            memcpy(trdetail99901money_kind      , (char *)GetSubStr(psResult[lCnt],11 ) , sizeof(trdetail99901money_kind   ) - 1);
            GetGBViaMKind(trdetail99901money_kind, sGBMoneyKind);
            
            trdetail99901occur_balance          = atof( (char *)GetSubStr(psResult[lCnt],12 ) );

            if ((memcmp(trdetail99901trans_code, "24701", 5) == 0) ||
                (memcmp(trdetail99901trans_code, "23701", 5) == 0) ||
                (memcmp(trdetail99901trans_code, "23731", 5) == 0) )
            {
                 memcpy(trdetail99901trans_type, "1", 2);
            }
            else
            if ((memcmp(trdetail99901trans_code, "24702", 5) == 0) ||
                  (memcmp(trdetail99901trans_code, "23702", 5) == 0))
            {
                   memcpy(trdetail99901trans_type, "2", 2);
            }

            /*д�ļ�*/
            fprintf(fp,"%-4s|%-8s|%-4s|%-8ld|%-6ld|%-20s|%-20s|%-32s|%-14s|%-32s|%1s|%1s|%-3s|%1s|%014.0lf|\n",
                        sGTBankKind                 ,
                        trdetail99901secu_no       ,
                        trdetail99901secu_branch   ,
                        trdetail99901hs_date       ,       
                        trdetail99901hs_time       ,       
                        trdetail99901serial_no     ,
                        trdetail99901secu_serial   ,
                        trdetail99901client_account,
                        trdetail99901secu_account  ,
                        " "                         ,
                        trdetail99901source_side   ,
                        trdetail99901trans_type    ,
                        sGBMoneyKind                ,
                        "0"                         ,       
                        trdetail99901occur_balance*100
                    );

        }
    }
    else if (iFlag == 1)
    {
        for(lCnt = 0; lCnt < lCount; lCnt ++)
        {
            memset(&accdetail99901, 0x00, sizeof(accdetail99901));
            memset(sRealBranch, 0x00, sizeof(sRealBranch));
            memset(sGBMoneyKind, 0x00, sizeof(sGBMoneyKind));
            memset(sGTBankKind, 0x00, sizeof(sGTBankKind));

            memcpy(accdetail99901bank_id         , (char *)GetSubStr(psResult[lCnt],0 ), sizeof(accdetail99901bank_id) - 1);
            Get2ViaBKind(accdetail99901bank_id, sGTBankKind);
            memcpy(accdetail99901secu_no         , (char *)GetSubStr(psResult[lCnt],1 ), sizeof(accdetail99901secu_no) - 1);
            memcpy(sBuff, (char *)GetSubStr(psResult[lCnt],2 ), DB_SBRNCH_LEN);
            memcpy(accdetail99901secu_branch, sBuff, DB_SBRNCH_LEN);
            if (lNum > 0)
            {
                for(i=0; i<lNum&&i<1000; i++)
                {
                    if (memcmp(sBuff, sSecuBranch[i], DB_SBRNCH_LEN) == 0)
                    {
                        memset(accdetail99901secu_branch, 0x00, sizeof(accdetail99901secu_branch));
                        memcpy(accdetail99901secu_branch, sRealBranch[i], DB_SBRNCH_LEN);
                        break;
                    }
                    else
                        continue;
                }
            }
            accdetail99901hs_date      = atol((char *)GetSubStr(psResult[lCnt],3 )) ;
            memcpy(accdetail99901client_account  , (char *)GetSubStr(psResult[lCnt],4 ), sizeof(accdetail99901client_account) - 1);                                          
            memcpy(accdetail99901secu_account    , (char *)GetSubStr(psResult[lCnt],5 ), sizeof(accdetail99901secu_account) - 1);                                          
            memcpy(accdetail99901client_name     , (char *)GetSubStr(psResult[lCnt],6 ), sizeof(accdetail99901client_name) - 1);
            memcpy(accdetail99901money_kind      , (char *)GetSubStr(psResult[lCnt],7 ), sizeof(accdetail99901money_kind) - 1);
            GetGBViaMKind(accdetail99901money_kind, sGBMoneyKind);
            strcpy(accdetail99901status          , (char *)GetSubStr(psResult[lCnt],8 ));
            if (accdetail99901status[0] == '0')
                accdetail99901status[0] = '0';
            else if (accdetail99901status[0] == '5')
            {
                /*20080522-01-����Ԥָ���ͻ���֤ȯ��Ϊ�����ͻ�*/
                /*�����Ų���ǿ��Ԥָ��״̬������ȯ��Ϊ����״̬*/
                if ((strlen(alltrim(accdetail99901client_account)) > 0) 
                 && (cForceMigrateFlag == '1'))
                    accdetail99901status[0] = '0';
                else
                    accdetail99901status[0] = '1';
            }
            else if (accdetail99901status[0] == '3')
                accdetail99901status[0] = '0';
            else
                accdetail99901status[0] = '0';

             fprintf(fp,"%-4s|%-8s|%-4s|%-8ld|%-32s|%-14s|%-32s|%-3s|%1s|%1s|\n",
                     sGTBankKind,
                     accdetail99901secu_no,
                     accdetail99901secu_branch,
                     accdetail99901hs_date,
                     accdetail99901client_account,
                     accdetail99901secu_account,
                     accdetail99901client_name,
                     sGBMoneyKind,
                     "0",
                     accdetail99901status
                    );
        }
    }
    else if (iFlag == 2)
    {
        for(lCnt = 0; lCnt < lCount; lCnt ++)
        {
            memset(&accdetail99901, 0x00, sizeof(accdetail99901));
            memset(sRealBranch, 0x00, sizeof(sRealBranch));
            memset(sGBMoneyKind, 0x00, sizeof(sGBMoneyKind));
            memset(gtTransCode, 0x00, sizeof(gtTransCode));
            memset(gtTransSource, 0x00, sizeof(gtTransSource));
            memset(sGTBankKind, 0x00, sizeof(sGTBankKind));
            memset(sBusinessFlag, 0x00, sizeof(sBusinessFlag));
            /* LABEL : 20091123-01-��ʼ��sTransCode����
             * MODIFY: huayl@hundsun.com
             * REASON: δ��ʼ�������������ļ�������
             * HANDLE: ��ʼ������
             */ 
            memset(sTransCode, 0x00, sizeof(sTransCode));

            memcpy(accdetail99901bank_id         , (char *)GetSubStr(psResult[lCnt],0 ), sizeof(accdetail99901bank_id) - 1);
            Get2ViaBKind(accdetail99901bank_id, sGTBankKind);
            memcpy(accdetail99901secu_no         , (char *)GetSubStr(psResult[lCnt],1 ), sizeof(accdetail99901secu_no) - 1);
            memcpy(sBuff, (char *)GetSubStr(psResult[lCnt],2 ), DB_SBRNCH_LEN);
            memcpy(accdetail99901secu_branch, sBuff, DB_SBRNCH_LEN);
            if (lNum > 0)
            {
                for(i=0; i<lNum&&i<1000; i++)
                {
                    if (memcmp(sBuff, sSecuBranch[i], DB_SBRNCH_LEN) == 0)
                    {
                        memset(accdetail99901secu_branch, 0x00, sizeof(accdetail99901secu_branch));
                        memcpy(accdetail99901secu_branch, sRealBranch[i], DB_SBRNCH_LEN);
                        break;
                    }
                    else
                        continue;
                }
            }
            accdetail99901hs_date      = atol((char *)GetSubStr(psResult[lCnt],3 )) ;
            accdetail99901hs_time      = atol((char *)GetSubStr(psResult[lCnt],4 )) ;
            accdetail99901serial_no    = atol((char *)GetSubStr(psResult[lCnt],5)); 
            memcpy(accdetail99901secu_serial     , (char *)GetSubStr(psResult[lCnt],6 ), sizeof(accdetail99901secu_serial) - 1); 
            memcpy(accdetail99901client_account  , (char *)GetSubStr(psResult[lCnt],7 ), sizeof(accdetail99901client_account) - 1);                                          
            memcpy(accdetail99901secu_account    , (char *)GetSubStr(psResult[lCnt],8 ), sizeof(accdetail99901secu_account) - 1);                                          
            memcpy(accdetail99901client_name     , (char *)GetSubStr(psResult[lCnt],9 ), sizeof(accdetail99901client_name) - 1);
            strcpy(sTransCode, (char *)GetSubStr(psResult[lCnt],10 ));
            Get2TransViaTCode(sTransCode, gtTransSource, gtTransCode);
            memcpy(accdetail99901money_kind      , (char *)GetSubStr(psResult[lCnt],12 ), sizeof(accdetail99901money_kind) - 1);
            GetGBViaMKind(accdetail99901money_kind, sGBMoneyKind);
            accdetail99901occur_bala = atol((char *)GetSubStr(psResult[lCnt],13 )) ;
            memcpy(accdetail99901secu_use_account  , (char *)GetSubStr(psResult[lCnt],14 ), sizeof(accdetail99901client_account) - 1);                                          

            fprintf(fp,"%-4s|%-8s|%-4s|%-8ld|%-6ld|%-20ld|%-20s|%-32s|%-14s|%-32s|%1s|%-3s|%-3s|%1s|%14.0lf|\n",
                        sGTBankKind,
                        accdetail99901secu_no, 
                        accdetail99901secu_branch,
                        accdetail99901hs_date,
                        accdetail99901hs_time,
                        accdetail99901serial_no,         
                        accdetail99901secu_serial,         
                        accdetail99901secu_use_account,
                        accdetail99901secu_account,
                        accdetail99901client_name,
                        gtTransSource,
                        gtTransCode,
                        sGBMoneyKind,
                        "0",
                        accdetail99901occur_bala*100
                    );
        }
    }
    /* LABEL : 20090819-01-���ӹ�̩��ʽԤԼ�������������ļ�
     * MODIFY: huanghl@hundsun.com
     * REASON: ����ԤԼ�������������ļ�����
     * HANDLE: ���ӹ�̩��ʽԤԼ�������������ļ�����
     */
    else if (iFlag == 5)
    {
        lSerialNo = 0;
        for(lCnt = 0; lCnt < lCount; lCnt ++)    
        {
            lZDBranch = 0;/*ԤԼ����ʱָ����ȯ��Ӫҵ��*/
            lJHBranch = 0;/*ȯ�̷�����ʱʹ�õ�ȯ��Ӫҵ��*/
            memset(sGTBankKind, 0x00, sizeof(sGTBankKind));
            memset(sGBMoneyKind, 0, sizeof(sGBMoneyKind));
            memset(&precdetail99901, 0x00, sizeof(precdetail99901));
            precdetail99901serial_no = ++lSerialNo; 
            memcpy(precdetail99901bank_id         , (char *)GetSubStr(psResult[lCnt],0 ), sizeof(precdetail99901bank_id));
            Get2ViaBKind(precdetail99901bank_id, sGTBankKind);
            memcpy(precdetail99901secu_use_account, (char *)GetSubStr(psResult[lCnt],1 ), sizeof(precdetail99901secu_use_account));
            memcpy(precdetail99901secu_no         , (char *)GetSubStr(psResult[lCnt],2 ), sizeof(precdetail99901secu_no));
            memcpy(precdetail99901secu_branch_b   , (char *)GetSubStr(psResult[lCnt],3 ), DB_SBRNCH_LEN);
            memcpy(precdetail99901secu_branch_s   , (char *)GetSubStr(psResult[lCnt],4 ), DB_SBRNCH_LEN);
            if (lNum > 0)
            {
                for(i=0; i<lNum&&i<1000; i++)
                {
                    if (memcmp(precdetail99901secu_branch_b, sSecuBranch[i], DB_SBRNCH_LEN) == 0)
                    {
                        memcpy(precdetail99901secu_branch_b, sRealBranch[i], DB_SBRNCH_LEN);
                        lZDBranch = 1;
                        if (strlen(alltrim(precdetail99901secu_branch_s)) == 0)
                        {
                            break;
                        }
                    }
                    if (memcmp(precdetail99901secu_branch_s, sSecuBranch[i], DB_SBRNCH_LEN) == 0)
                    {
                        memcpy(precdetail99901secu_branch_s, sRealBranch[i], DB_SBRNCH_LEN);
                        lJHBranch = 1;
                    }
                    /*������ָ����Ӫҵ���ͼ���ʹ�õ�Ӫҵ������ȯ���ڲ�Ӫҵ�����ƥ��ʱ�˳�*/
                    if (lZDBranch == 1 && lJHBranch == 1)
                        break;
                    else
                        continue;
                }
                
            }
            
            memcpy(precdetail99901secu_account    , (char *)GetSubStr(psResult[lCnt],5 ), sizeof(precdetail99901secu_account));                                          
            memcpy(precdetail99901client_name     , (char *)GetSubStr(psResult[lCnt],6 ), sizeof(precdetail99901client_name));
            memcpy(precdetail99901id_kind         , (char *)GetSubStr(psResult[lCnt],7 ), sizeof(precdetail99901id_kind));
            memcpy(precdetail99901id_code         , (char *)GetSubStr(psResult[lCnt],8 ), sizeof(precdetail99901id_code));
            memcpy(precdetail99901money_kind      , (char *)GetSubStr(psResult[lCnt],9 ), sizeof(precdetail99901money_kind));
            GetGBViaMKind(precdetail99901money_kind, sGBMoneyKind);
            memcpy(precdetail99901status          , (char *)GetSubStr(psResult[lCnt],10 ), sizeof(precdetail99901status));
            memcpy(precdetail99901busin_type      , (char *)GetSubStr(psResult[lCnt],11 ), 1);
            
            /*����״̬*/
            if (precdetail99901status[0] == '2')
            {    
                precdetail99901status[0] = '0';
            }
            
            fprintf(fp,"%-8ld|%-4s|%-32s|%-8s|%-8s|%-8s|%-20s|%-60s|%-s|%-18s|%-3s|%1s|",
                        precdetail99901serial_no,
                        sGTBankKind,
                        precdetail99901secu_use_account,
                        precdetail99901secu_no, 
                        precdetail99901secu_branch_b,
                        precdetail99901secu_branch_s,
                        precdetail99901secu_account,
                        precdetail99901client_name,
                        precdetail99901id_kind,
                        precdetail99901id_code,
                        sGBMoneyKind,
                        precdetail99901status
                    );

            if(cDzfileBusintypeFlag == '1')
            {
                fprintf(fp, "%1s|\n", precdetail99901busin_type);
            }
            else
            {
                fprintf(fp, "\n");
            }   
        }
    }
    else if (iFlag == 6)
    {
        lSerialNo = 0;
        for(lCnt = 0; lCnt < lCount; lCnt ++)
        {
            memset(&typedetail99901, 0x00, sizeof(typedetail99901));
            memset(sGTBankKind, 0x00, sizeof(sGTBankKind));
            memset(sGBMoneyKind,0x00, sizeof(sGBMoneyKind));
            memcpy(typedetail99901secu_no, (char *)GetSubStr(psResult[lCnt],0 ), sizeof(typedetail99901secu_no));
            memcpy(typedetail99901secu_account, (char *)GetSubStr(psResult[lCnt],1 ), sizeof(typedetail99901secu_account));
            memcpy(typedetail99901money_kind, (char *)GetSubStr(psResult[lCnt],2 ), sizeof(typedetail99901money_kind));
            GetGBViaMKind(typedetail99901money_kind, sGBMoneyKind);
            memcpy(typedetail99901busin_type, (char *)GetSubStr(psResult[lCnt],3 ), sizeof(typedetail99901busin_type));
            memcpy(typedetail99901bank_id, (char *)GetSubStr(psResult[lCnt],4 ), sizeof(typedetail99901bank_id));
            Get2ViaBKind(typedetail99901bank_id, sGTBankKind);
            memcpy(typedetail99901secu_use_account, (char *)GetSubStr(psResult[lCnt],5 ), sizeof(typedetail99901secu_use_account));
            memcpy(typedetail99901client_account, alltrim(GetSubStr(psResult[lCnt],6 )), sizeof(typedetail99901client_account));
            memcpy(typedetail99901secu_branch, (char *)GetSubStr(psResult[lCnt],7 ), sizeof(typedetail99901secu_branch));
            memcpy(typedetail99901client_name, (char *)GetSubStr(psResult[lCnt],8 ), sizeof(typedetail99901client_name));
            memcpy(typedetail99901id_kind, (char *)GetSubStr(psResult[lCnt],9 ), sizeof(typedetail99901id_kind));
            memcpy(typedetail99901id_code, (char *)GetSubStr(psResult[lCnt],10 ), sizeof(typedetail99901id_code));

            /*ʹ�õ��յ�һ��tbaccountedit��¼�Ŀ���Ϊ���վɿ���*/
            memset(sSql,0x00,sizeof(sSql));
            sprintf(sSql,"select client_account,business_flag from tbaccountedit where hs_date=%ld and serial_no=(select min(serial_no) from tbaccountedit where secu_no='%s' and secu_account='%s' and busin_type='%s' and hs_date=%ld and business_flag in(1001,1004,1024,1019))",
                     pub->lTransDate,
                     typedetail99901secu_no,
                     typedetail99901secu_account,
                     typedetail99901busin_type,
                     pub->lTransDate);
            lnum = DBExecSelect(1, 0, sSql,&psResult2);
            if (lnum < 0)
            {
                DBFreePArray2(&psResult, lCount);
                SET_ERROR_CODE(ERR_DATABASE);
                return -2;
            }
            memset(OldCard,0x00,sizeof(OldCard));
            memset(NewCard,0x00,sizeof(NewCard));
            memset(business_flag,0x00,sizeof(business_flag));
            memcpy(OldCard,alltrim(GetSubStr(psResult2[0],0 )),sizeof(OldCard)-1);
            memcpy(business_flag,alltrim(GetSubStr(psResult2[0],1 )),sizeof(business_flag)-1);
            memcpy(NewCard,typedetail99901client_account,sizeof(NewCard)-1);
            /*���տ�����Ĭ�Ͼɿ�����Ϊ��ͨ��0*/
            if ((strcmp(business_flag,"1001") == 0) ||(strcmp(business_flag,"1019") == 0))
            {
                lOld = 0;
            }
            else
            {
                /*���ɿ�����,0-��ͨ��,1-������,-1-��ѯʧ��*/
                lOld=CheckAcctType(OldCard,&cardparse);
                if (lOld < 0)
                {
                    DBFreePArray2(&psResult2, lnum);
                    DBFreePArray2(&psResult, lCount);
                    SET_ERROR_CODE(ERR_PARAMETER);
                    return -3;
                }
            }
            lNew=CheckAcctType(NewCard,&cardparse);
            if (lNew < 0)
            {
                DBFreePArray2(&psResult2, lnum);
                DBFreePArray2(&psResult, lCount);
                SET_ERROR_CODE(ERR_PARAMETER);
                return -3;
            }
            /*lType��ʾ�¾ɿ��ŵ����Ͳ�*/
            lType=lNew-lOld;
            DBFreePArray2(&psResult2, lnum);
            /*0-����δ�����ı�,����¼*/
            if (lType == 0)
            {
                continue;
            }
            /*1-��ͨ����Ϊ���������¿�������*/
            else if (lType == 1)
            {
                typedetail99901acc_type[0]='1';/*1-������*/
                typedetail99901serial_no = ++lSerialNo;
            }
            /*-1-��������Ϊ��ͨ��*/
            else if (lType == -1)
            {
                typedetail99901acc_type[0]='0';/*0-��ͨ��*/
                typedetail99901serial_no = ++lSerialNo;
            }
                
            if (lNum > 0)
            {
                for(i=0; i<lNum&&i<1000; i++)
                {
                    if (memcmp(typedetail99901secu_branch, sSecuBranch[i], DB_SBRNCH_LEN) == 0)
                    {
                        memcpy(typedetail99901secu_branch, sRealBranch[i], DB_SBRNCH_LEN);
                        break;
                    }
                    else
                        continue;
                }
            }
            
            fprintf(fp,"%-8ld|%-4s|%-32s|%-8s|%-8s|%-20s|%-60s|%1s|%-18s|%-3s|%1s|",
                        typedetail99901serial_no,
                        sGTBankKind,
                        typedetail99901secu_use_account,
                        typedetail99901secu_no, 
                        typedetail99901secu_branch,
                        typedetail99901secu_account,
                        typedetail99901client_name,
                        typedetail99901id_kind,
                        typedetail99901id_code,
                        sGBMoneyKind,
                        typedetail99901acc_type
                    );
            if(cDzfileBusintypeFlag == '1')
            {
                fprintf(fp, "%1s|\n", typedetail99901busin_type);
            }
            else
            {
                fprintf(fp, "\n");
            }
        }
    }
    
    fclose(fp);
    DBFreePArray2(&psResult, lCount);
    
    return 0;
}
/*****************************************************
 ��������:  Get1ViaBKind
 �������:  
 ��Ԫ�ļ�:  99901c
 ��������:  ���ݺ�����������б��ת���ɱ�׼�ӿڶ�������б��
 ��������:  psBType  -- ��0�����У���1�����У���2�����У���3��ũ�У�
                        ��4�����У���5�����̣���6���ַ�����7�����У�
                        ��8����󣬡�9�����ģ���a����ҵ����b��������
                        ��c�������d������
 ��������:  =0:�ɹ�  !0:ʧ��
            gtBType -- ��׼�ӿ����б�� 
 ����˵��:
 �޸�˵��:
*****************************************************/
long Get1ViaBKind(char *psBType, char *gtBType)
{
    switch(psBType[0])
    {
        case '3':   /*�й�ũҵ����*/
            strcpy(gtBType, "1031000");
            break;
        case '4':   /*��ͨ����*/
            strcpy(gtBType, "3012900");
            break;
        case '6':   /*�Ϻ��ֶ���չ����*/
            strcpy(gtBType, "3102900");
            break;
        case '7':   /*�Ϻ�����*/
            strcpy(gtBType, "3132900");
            break;
        case '8':   /*�й��������*/
            strcpy(gtBType, "3031000");
            break;
        case 'a':   /*��ҵ����*/
            strcpy(gtBType, "3093910");
            break;
        case 'b':   /*�й���������*/
            strcpy(gtBType, "3051000");
            break;
        case 'c':   /*��������*/
            strcpy(gtBType, "");
            break;
        case 'h':   /*��������*//*20110819-01-���ӱ�������3131000*/
            strcpy(gtBType, "3131000");
            break;
        case 'j':   /*��������*/
            strcpy(gtBType, "");
            break;
        case 'k':   /*��������*/
            strcpy(gtBType, "");
            break;
        case 'l':   /*�ൺ����*/
            strcpy(gtBType, "");
            break;
        case 'n':   /*�Ͼ�����*/
            strcpy(gtBType, "");
            break;
        default:
            strcpy(gtBType, psBType);
            break;
    }        
    
    return 0;
}


/*****************************************************
 ��������:  Get2ViaBKind
 �������:  
 ��Ԫ�ļ�:  99901c
 ��������:  ���ݺ�����������б��ת���ɹ�̩��������б��
 ��������:  psBType  -- ��0�����У���1�����У���2�����У���3��ũ�У�
                        ��4�����У���5�����̣���6���ַ�����7�����У�
                        ��8����󣬡�9�����ģ���a����ҵ����b��������
                        ��c�������d������
 ��������:  =0:�ɹ�  !0:ʧ��
            gtBType -- ��6005������ 
 ����˵��:
 �޸�˵��:  
*****************************************************/
long Get2ViaBKind(char *psBType, char *gtBType)
{
    switch(psBType[0])
    {
        case '4':
            strcpy(gtBType, "6005");
            break;
        default:
            strcpy(gtBType, psBType);
            break;
    }

    return 0;
}


/*****************************************************
 ��������:  Get1TransViaTCode
 �������:  
 ��Ԫ�ļ�:  99901c
 ��������:  ���ݺ�����������б��ת���ɹ�̩��������б��
 ��������:  psTCode  -- 
 ��������:  =0:�ɹ�  !0:ʧ��
            gtTSource -- 0��֤ȯ��,1�����ж�
            gtTCode   -- 101�����з���ָ��������н��ף�
                         103�����з����������˻����ף�
                         104����������������ȷ��ҵ��
                         201��ȯ�̷���ָ��������н��ף�
                         202��ȯ�̷�����������н��ף�
                         203��ȯ�̷����������˻����ף�
                         204���������Ԥָ����
 ����˵��:
 �޸�˵��:
*****************************************************/
long Get1TransViaTCode(char *psTCode, char *gtTSource, char *gtTCode)
{
    switch(atol(psTCode))
    {
        case 25701:
            gtTSource[0] = 'B';
            strcpy(gtTCode, "11001");
            break;
        case 25702:
            gtTSource[0] = 'B';
            strcpy(gtTCode, "11003");
            break;
        case 25710:
            gtTSource[0] = 'B';
            strcpy(gtTCode, "11006");
            break;
        case 25704:
            gtTSource[0] = 'B';
            strcpy(gtTCode, "11005");
            break;
        case 26701:
            gtTSource[0] = 'S';
            strcpy(gtTCode, "11001");
            break;
        case 26702:
            gtTSource[0] = 'S';
            strcpy(gtTCode, "11002");
            break;
        case 26703:
            gtTSource[0] = 'S';
            strcpy(gtTCode, "11004");
            break;
        case 26704:
            gtTSource[0] = 'S';
            strcpy(gtTCode, "11005");
            break;
        case 26710:
            gtTSource[0] = 'S';
            strcpy(gtTCode, "11006");
            break;
        case 26712:
            gtTSource[0] = 'S';
            strcpy(gtTCode, "12006");
            break;
        default:
            break;
    }

    return 0;
}

/*****************************************************
 ��������:  Get2TransViaTCode
 �������:  
 ��Ԫ�ļ�:  99901c
 ��������:  ���ݺ�����������б��ת���ɹ�̩��������б��
 ��������:  psTCode  -- 
 ��������:  =0:�ɹ�  !0:ʧ��
            gtTSource -- 0��֤ȯ��,1�����ж�
            gtTCode   -- 101�����з���ָ��������н��ף�
                         103�����з����������˻����ף�
                         104����������������ȷ��ҵ��
                         201��ȯ�̷���ָ��������н��ף�
                         202��ȯ�̷�����������н��ף�
                         203��ȯ�̷����������˻����ף�
                         204���������Ԥָ����
 ����˵��:
 �޸�˵��:
*****************************************************/
long Get2TransViaTCode(char *psTCode, char *gtTSource, char *gtTCode)
{
    switch(atol(psTCode))
    {
        case 25701:
            gtTSource[0] = '1';
            strcpy(gtTCode, "101");
            break;
        case 25702:
            gtTSource[0] = '1';
            strcpy(gtTCode, "104");
            break;
        case 25710:
            gtTSource[0] = '1';
            strcpy(gtTCode, "103");
            break;
        case 26701:
            gtTSource[0] = '2';
            strcpy(gtTCode, "201");
            break;
        case 26702:
            gtTSource[0] = '2';
            strcpy(gtTCode, "201");
            break;
        case 26703:
            gtTSource[0] = '2';
            strcpy(gtTCode, "202");
            break;
        case 26710:
            gtTSource[0] = '2';
            strcpy(gtTCode, "203");
            break;
        default:
            break;
    }

    return 0;
}

/*****************************************************
 ��������:  Get1ViaIdKind
 �������:  
 ��Ԫ�ļ�:  99901c
 ��������:  ���ݺ��������֤������ת���ɱ�׼�ӿڶ����֤������
 ��������:  psIdType  -- 1���֤,2����֤,3���ڻ���,4���ڱ�,
                         5ѧԱ֤,6����֤,7��ʱ���֤,8��֯��������,
                         9Ӫҵִ��,A����֤,B��ž�ʿ��֤,C����֤,
                         D�������, E�۰�̨�������֤,
                         F̨��ͨ��֤��������Ч����֤,G����ͻ����,
                         H��ž���ְ�ɲ�֤,I�侯��ְ�ɲ�֤,
                         J�侯ʿ��֤,X������Ч֤��,Z�غ����֤
 ��������:  =0:�ɹ�  !0:ʧ��
            gtIdType -- ��׼�ӿ�֤������
                        10  ���֤
                        11  ����
                        12  ����֤
                        13  ʿ��֤
                        14  ����֤
                        15  ���ڱ�
                        16  Ӫҵִ��
                        17  ��֯��������֤
                        20  ����
 ����˵��:
 �޸�˵��:
*****************************************************/
long Get1ViaIdKind(char *psIdType, char *gtIdType)
{
    switch(psIdType[0])
    {
        case '1':
            strcpy(gtIdType, "10");
            break;
        case '2':
            strcpy(gtIdType, "12");
            break;
        case '3':
            strcpy(gtIdType, "11");
            break;
        case '4':
            strcpy(gtIdType, "15");
            break;
        case '7':
            strcpy(gtIdType, "10");
            break;
        case '8':
            strcpy(gtIdType, "17");
            break;
        case '9':
            strcpy(gtIdType, "16");
            break;
        case 'B':
            strcpy(gtIdType, "13");
            break;
        case 'C':
            strcpy(gtIdType, "14");
            break;
        default:
            strcpy(gtIdType, "20");
            break;
    }        
    
    return 0;
}

/******************************************************************************
��������: SetSecuClearStatus
��������: �޸�ȯ������״̬
���������psSecuNo - ȯ�̺� 
          cStatus  ��״̬
���������
�� �� ֵ��=0:�ɹ�  <0:ʧ��
˵    ����  
******************************************************************************/
long SetSecuClearStatus(char *psSecuNo, char cStatus)
{
    char sSql[SQL_BUF_LEN];

    memset(sSql, 0x00, sizeof(sSql));
    sprintf(sSql, "update tbsecuzjcg set clear_status = '%c' where busin_type!='%c'", cStatus, BUSIN_YZZZ);
    if (strlen(alltrim(psSecuNo)) > 0)
        sprintf(sSql, "%s and secu_no='%s'", sSql, psSecuNo);
    BeginTrans();
    if (DBExecIUD(sSql) < 1)
    {
        RollbackTrans();
        return -1;
    }
    CommitTrans();
    
    return 0;
}


/******************************************************************************
�������ơ�TransToMonitor
�������ܡ����ɼ�ؽṹ
���������pub     -- ���׽ṹ
���������
�� �� ֵ����
******************************************************************************/
static void TransToMonitor(TRANS99901 *pub, TBMONITOR *monitor)
{
    /*20080529-01-�������㹦��ͳһʹ�ó�ʼ������*/
    /*monitor->hs_date = pub->lTransDate;*/
    monitor->secu_time = 0;    
    monitor->bank_time = 0;    
    memcpy(monitor->secu_no, alltrim(pub->req.sOrganId),sizeof(monitor->secu_no) -1);
    monitor->entrust_way[0] = pub->req.EntrustWay[0];
    memcpy(monitor->error_code, alltrim(pub->sErrCode), sizeof(monitor->error_code) - 1);  
    memcpy(monitor->error_msg, alltrim(pub->sErrMsg), sizeof(monitor->error_msg) -1 );
    
    return ;
}

/******************************************************************************
�������ơ�ParseParam
�������ܡý�����������־�Ĳ���ֵ
          value--��������־�Ĳ���ֵ
          cardparse--����������(��ʽ:622827???00-622827???05,622845???99,)
�����������
�� �� ֵ��0-�ɹ�
******************************************************************************/
static long ParseParam(char *value, struct CARDPARSE *cardparse)
{
    long i, lNum;
    char sBuffer[128+1];
    
    for(i=0,lNum=0; ; i++)
    {
        memset(sBuffer, 0x00, sizeof(sBuffer));
        strcpy(sBuffer, GetSubStrViaFlag(value, i, ','));
        if (strlen(alltrim(sBuffer)) == 0)
            break;
        
        if (strchr(sBuffer, '-') == NULL)
        {
            cardparse->cardstr[i].cFlag = '='; /*��λƥ��*/
            memcpy(cardparse->cardstr[i].sString_b, sBuffer, sizeof(cardparse->cardstr[i].sString_b)-1);
        }
        else
        {
            cardparse->cardstr[i].cFlag = '<'; /*��Χƥ��*/
            memcpy(cardparse->cardstr[i].sString_b, GetSubStrViaFlag(sBuffer, 0, '-'), sizeof(cardparse->cardstr[i].sString_b)-1);
            /* LABEL : 20090901-01-�����޸�
             * MODIFY: wanggk@hundsun.com
             * REASON: 
             * HANDLE: sString_b��ΪsString_e
             */ 
            /*memcpy(cardparse->cardstr[i].sString_e, strchr(sBuffer, '-')+1, sizeof(cardparse->cardstr[i].sString_b)-1);*/
            memcpy(cardparse->cardstr[i].sString_e, strchr(sBuffer, '-')+1, sizeof(cardparse->cardstr[i].sString_e)-1);
            
        }
        lNum++;
    }
    cardparse->lNum = lNum;
    
    return 0;
}

/******************************************************************************
�������ơ�CheckAcctType
�������ܡü�������˺��Ƿ�Ϊ�������������
          CardNo--���������˺�
          cardparse--����������(��ʽ:622827???00-622827???05,622845???99,)
�����������
�� �� ֵ��0-��ͨ��,1-������
******************************************************************************/
static long CheckAcctType(char *sCardNo, struct CARDPARSE *cardparse)
{
    long i, j, k, num, lFlag, lLen;
    char sString_b[32+1][32+1], sString_e[32+1][32+1], sTmp[32+1][32+1];
    
    /*��ÿһ�����ƥ�䴦��*/
    for(i=0; i<cardparse->lNum; i++)
    {
        /*��ʼֵΪƥ��*/
        lFlag = 1;  

        /*ƥ�䴮���ȣ������ƥ��*/
        lLen = strlen(alltrim(cardparse->cardstr[i].sString_b));
        if (lLen == 0)
        {
            lFlag = 0;
            break;
        }
        /*����'='�������ƥ��*/
        if (cardparse->cardstr[i].cFlag == '=')
        {
            for(j=0; j<lLen; j++)
            {
                /*'?'Ϊͨ�����������ƥ��*/
                if (cardparse->cardstr[i].sString_b[j] == '?')
                    continue;
                else
                {
                    /*��ƥ�䣬�������һ��*/
                    if(cardparse->cardstr[i].sString_b[j] != sCardNo[j])
                    {
                        lFlag = 0;
                        break;
                    }
                }
            }
        }
        /*����'<'����Χƥ��*/
        else if(cardparse->cardstr[i].cFlag == '<')
        {
            memset(sString_b, 0x00, sizeof(sString_b));
            memset(sString_e, 0x00, sizeof(sString_e));
            memset(sTmp, 0x00, sizeof(sTmp));
            for(j=0,k=0,num=0; j<lLen; j++)
            {
                /*'?'Ϊͨ�����������ƥ��*/
                if (cardparse->cardstr[i].sString_b[j] == '?')
                    continue;
                else
                {
                    /*ȷ��ƥ��θ���*/
                    if ((j>0) && (cardparse->cardstr[i].sString_b[j-1] == '?'))
                    {
                        /*????*/
                        num++;
                        k=0;
                    }
                    /*��ȡƥ����ַ���*/
                    sString_b[num][k] = cardparse->cardstr[i].sString_b[j];
                    sString_e[num][k] = cardparse->cardstr[i].sString_e[j];
                    sTmp[num][k] = sCardNo[j];
                    k++;

                }
            }
            /*if (num == 0)
            {
                lFlag = 0;
            }
            else*/ if (num > 0)
            {
                for(j=0; j<=num; j++)
                {
                    /*ת�������ͽ��з�Χƥ��*/
                    /* LABEL : 20090901-01-BUG�޸�
         ������������* MODIFY: wanggk@hundsun.com
         ������������* REASON: atol�г������Σ��
         ������������* HANDLE: atol����strcmp
         ������������*/ 
                    /* if ((atol(sTmp[j]) < atol(sString_b[j])) || (atol(sTmp[j]) > atol(sString_e[j])) */
                    if ((strcmp(sTmp[j], sString_b[j]) < 0) || (strcmp(sTmp[j], sString_e[j]) > 0))
                    {
                        lFlag = 0;
                        break;
                    }
                }
            }
        }
        else
        {
            lFlag = 0;
            continue;
        }
        /*��ƥ�䣬��ֹͣ����ƥ�����*/
        if(lFlag == 1)
            break;
    }
    
    return lFlag;
}

/******************************************************************************
�������ơ�GetNumViaFlag
�������ܡü���ַ�����ָ���ַ��ĸ���
���������
���������
�� �� ֵ��
******************************************************************************/
static long GetNumViaFlag(char *sString, char cFlag)
{
    long i, lNum, lLen;
    
    lLen = strlen(sString);
    for (i=0,lNum=0; i<lLen; i++)
    {
        if (sString[i] == cFlag)
            lNum++;
    }

    return lNum;
}

void showversion(const char *filename)
{
    #define SYSTEM_VER "V2.0.0.17"       /* V������ַ����������ݿ�ģ���O-Oracle;I-Informix;S-Sybae
                                           �汾��1.1 + ������ţ���1��ʼ��ţ�+������ţ���1��ʼ��ţ�*/
    fprintf(stderr, "�����ϵͳ��%-17s�汾�� %-17s���� %s\n", filename, SYSTEM_VER, __DATE__);
    return;
}

/**********************************************�ļ�����********************************************/
