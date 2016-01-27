/*****************************************************************************
文件名称∶99901c
系统名称∶存管系统
模块名称∶日终清算
软件版权∶恒生电子股份有限公司
功能说明∶生成券商对帐文件
系统版本∶2.0
开发人员∶存管系统项目组
开发时间∶2007-9-1
审核人员∶
审核时间∶
相关文档∶
修改记录∶版本号    修改人  修改标签    修改原因
          V2.0.0.17 王思艺  20141014-01 过滤批量换卡交易流水
          V2.0.0.16 刘俊阳  20130916-01 重新赋值整数型宏定义
          V2.0.0.15 刘俊阳  20130326-01 增加证件类型转换处理
          V2.0.0.14 王晨波  20120817-01 调整业务类别判断
                            20120929-01 销户日期未变需要参数控制
          V2.0.0.13 黄慧龄  20110819-01 调整未初始化bug
          V2.0.0.12 黄慧龄  20110506-02 销户区分预指定销户和正常销户
          V2.0.0.11 华玉磊  20091123-01 初始化sTransCode变量
          V2.0.0.11 黄慧龄  20090720-01 增加客户帐户类型变动文件
                            20090804-01 初始化参数和增加业务类型
                    朱武林  20090810-01 根据账户类型的不同设置调整账户类型变动文件的生成
                    黄慧龄  20090819-01 增加国泰格式预约开户和联名卡文件
                    王光宽  20090901-01 BUG修改        
          V2.0.0.10 黄慧龄  20090624-01 增加预约开户客户信息对账文件
          V2.0.0.9  胡元方  20081010-01 融资融券业务中使用同一券商编号时对账文件(恒生模式)增加业务类型
          V2.0.0.8  胡元方  20081006-01 增加融资融券，修改获取券商业务表模式
          V2.0.0.7  朱武林  20080708-01 保证账户状态文件记录唯一性增加distinct
          V2.0.0.6  朱武林  20080605-01 优化账户状态文件的SQL语句
                    朱武林  20080605-02 获取包含缺省的券商营业部信息
          V2.0.0.5  朱武林  20080530-01 标志版本中转账明细结构体初始化
          V2.0.0.4  朱武林  20080529-01 日终清算功能统一使用初始化日期
          V2.0.0.3  朱武林  20080523-01 账户状态文件中避免生成当日签约的多条记录
          V2.0.0.2  朱武林  20080522-01 带卡预指定客户在证券方为正常客户
                    朱武林  20080522-02 调整标准接口中的钞汇标志
          V2.0.0.1  朱武林  20080514-01 记录银行后台核对状态
******************************************************************************/

#include "bss.inc"
#include "99901I.h"

/*********************************常量定义************************************/


/**********************************宏定义*************************************/
#define    PAGELINES        50
#define    SELECT_NUM       400000
/*********************************类型定义************************************/

/*******************************结构/联合定义*********************************/
typedef struct tagTrans99901
{
    TBSYSARG      *sysarg;                           /* 系统参数结构 */
    TBTRANS       *trans;                            /* 交易参数 */
    REQ99901      req;
    ANS99901      ans;
    long          lSerialNo;                         /* 流水号 */
    long          lTransDate;                        /* 交易日=sysarg.prev_date */
    char          sErrCode[DB_ERRNO_LEN + 1];        /* 错误代码 */
    char          sErrMsg[DB_ERRMSG_LEN + 1];        /* 错误信息 */
    char          sBankErrMsg[1024];
    char          sSecuNo[DB_SECUNO_LEN + 1];
    char          cNeedModify;                       /* 20120711-01-用于判断是否修改销户日期 */
    
}TRANS99901;

/*20090810-01-根据账户类型的不同设置调整账户类型变动文件的生成*/
struct CARDSTR
{
    char      cFlag;                   /*匹配标志 '='定位匹配 '<'范围匹配 */
    char      sString_b[32 + 1];       /*匹配起始串 */  
    char      sString_e[32 + 1];       /*匹配结束串 */ 
};
struct CARDPARSE
{
    long      lNum;                   /*匹配串解析个数*/
    struct CARDSTR  cardstr[32+1];
};

/*******************************外部变量声明**********************************/

/*******************************全局变量定义**********************************/
char sSecuDir[128];
char sLogName[256];

/*******************************函数原型声明**********************************/
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
函数名称: main99901
函数功能: 生成券商对帐文件
输入参数∶request - 请求包
输出参数∶answer  - 应答包
返 回 值∶无
说    明∶注意Flag的应用
******************************************************************************/
void main99901(const char *request, long lReqLen, char *answer, long *lAnsLen, INITPARAMENT *initPara)
{
    TRANS99901 trans99901;
    TRANS99901 *pub;

    pub = &trans99901;
    memset(pub, 0x00, sizeof(TRANS99901));

    /* 解包 */
    if (UnPackRequest(pub, (char *)request, lReqLen, initPara) != 0)
    {
        goto _RETURN;
    }

    SET_ERROR_CODE(ERR_DEFAULT);    /* 防止遗漏设置错误码 */
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
    /* 发送证券端银行对账文件生成通知 */
    if (ToSecu(pub) != 0)
    {
        goto _RETURN;
    } 
    
    SET_ERROR_CODE(ERR_SUCCESS);

_RETURN:
    /* 生成应答包 */
    PackAnswer(pub, answer, lAnsLen);

    TransToMonitor(pub, &(initPara->monitor));

    return;
}

/******************************************************************************
函数名称∶UnPackRequest
函数功能∶对输入缓冲区数据解包到交易的总结构中
输入参数∶request  -- 输入缓冲区指针
          initPara -- 系统参数结构指针
输出参数∶pub     -- 交易的总结构
返 回 值∶无
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
        PrintLog(LEVEL_ERR, LOG_ERROR_INFO, "解包错%s", sBuff);
        SET_ERROR_CODE(ERR_PACKAGE);
        return -1;
    }

    return 0;
}

/******************************************************************************
函数名称∶PackAnswer
函数功能∶生成应答包
输入参数∶pub      -- 交易结构
输出参数∶answer   -- 应答缓冲区
          lAnsLen   -- 返回包正文长度
返 回 值∶
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
            sprintf(sBuff, "生成文件成功!自动发送生成下列券商文件通知失败:%s", pub->sBankErrMsg);
            memcpy(pub->sErrMsg, sBuff, sizeof(pub->sErrMsg) -1);
        }
        
        if (strlen(alltrim(pub->sErrMsg)) == 0)
        {
            strcpy(ans->ErrorInfo, "交易成功");
        }
        else
        {
            memcpy(ans->ErrorInfo, pub->sErrMsg, sizeof(ans->ErrorInfo)-1);
        }

        if (Struct2MFS(answer, (char *)ans, _ANS99901, sBuff) != 0)
        {
            SET_ERROR_CODE(ERR_PACKAGE);
            PrintLog(LEVEL_ERR, LOG_ERROR_INFO, "打包错%s", sBuff);
        }
    }
    else
    {
        GetErrorMsg(pub->sErrCode, pub->sErrMsg);
        sprintf(answer, "%s|%s|%s|%s|", pub->req.FunctionId, pub->req.ExSerial, pub->sErrCode, pub->sErrMsg);
    }
    
    *lAnsLen = strlen(answer);

    /* 记日志 */
    PrintLog(LEVEL_NORMAL, LOG_SEND_BANK_ANS, answer);

    return;
}

/******************************************************************************
函数名称∶Gen_SecuFile
函数功能∶按券商生成券商需要的文件
输入参数∶无
输出参数∶无
返 回 值∶ 0 -- 成功
          <0 -- 失败
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
    
    /*取券商目录*/
    memset(sSecuDir, 0x00, sizeof(sSecuDir));
    if (GetCfgItem("PUBLIC", "SECUDIR", sSecuDir) != 0)
    {
        sprintf(sSecuDir, "%s", getenv("HOME"));
    }
    else
    {
        /*取消最后一位'/'*/
        if (sSecuDir[strlen(sSecuDir)-1] == '/')
            sSecuDir[strlen(sSecuDir)-1] = '\0';
    }
        
    /*赋日志名称*/
    memset(sLogName, 0x00, sizeof(sLogName));
    if ( strlen(alltrim((pub->req).sOrganId)) > 0)
        sprintf(sLogName, "%s/%s/%spr%s.%ld", sSecuDir, (pub->req).sOrganId, SENDFILE_DIR, pub->req.FunctionId, pub->lTransDate);
    else
        sprintf(sLogName, "%s/log/pr%s.%ld", getenv("HOME"), pub->req.FunctionId, pub->lTransDate);
    LogTail(sLogName, "※生成券商对帐文件开始...", "");
      
    LogTail(sLogName, "  ※合法性检查开始...", "");
    /* 控制系统日切标志:0－正常，1－准备日切，8－日切成功，9－日切失败 */
    memset(sFlag, 0x00, sizeof(sFlag));
    if (GetCfgItem("HSRZCL", "SYSDATE_FLAG", sFlag) != 0)
    {
        SET_ERROR_CODE(ERR_GETTURNUPFLAG);
        LogTail(sLogName, "  ★系统取日期翻牌标志错！", "");
        return -1;
    }
    if (sFlag[0] != '8')
    {
        SET_ERROR_CODE(ERR_NOTURNUP);
        LogTail(sLogName, "  ★系统还未日期翻牌错！", "");
        return -2;
    }

    /* 控制银行核对标准(可选):0－正常，1－准备核对，8－核对成功，9－核对失败*/
    memset(sFlag, 0x00, sizeof(sFlag));
    if (GetCfgItem("HSRZCL", "BZZCHCK_FLAG", sFlag) == 0)
    {
        if (sFlag[0] != '8')
        {
            SET_ERROR_CODE(ERR_CHECKDETAIL);
            SET_ERROR_MSG("系统还未完成银行明细核对");
            LogTail(sLogName, "  ★系统还未完成银行明细核对！", "");
            return -2;
        }
    }
    LogTail(sLogName, "  ※合法性检查结束！", "");

    /* 20120929-01-调整开销户日期 */
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
        LogTail(sLogName, "  ★取券商列表错！", "");
        return -3;
    }

    if ( lCount == 0 && strlen((pub->req).sOrganId) > 0)
    {
       SET_ERROR_CODE(ERR_SECUSTATUS);
       LogTail(sLogName, "  ★券商状态不正常！", "");
       return -3;
    }
    
    LogTail(sLogName, "  ※生成文件开始...", "");
    for(lCnt = 0; lCnt < lCount; lCnt ++)
    {
        memset(sSecuNo, 0x00, sizeof(sSecuNo));
        memcpy(sSecuNo, (char *)GetSubStr(psResult[lCnt], 0), DB_SECUNO_LEN);
        
        /*日终文件格式标志：0-恒生格式(默认) 1-标准格式 2-国泰格式*/
        cFileFlag = '0';
        if (DBGetParam(sSecuNo, "FILE_FLAG", &param) == 0)
        {
            cFileFlag = param.param_value[0];
        }
        if (cFileFlag == '0')  /*0-恒生格式(默认)*/
        {
            ERRLOGF("开始生成银证转帐明细文件%s","");
            LogTail(sLogName, "    ※恒生格式【%s】银证转帐明细文件开始...", sSecuNo);
            /* 生成银证转帐明细文件 */
            if (Gen_DetailFile(pub,initPara, sSecuNo, sBusinType, 0) != 0)
            {
               DBFreePArray2(&psResult, lCount);
               LogTail(sLogName, "    ★生成[%s]的银证转帐明细文件失败！", sSecuNo);
               return -4;
            }
            ERRLOGF("结束生成银证转帐明细文件%s","");
            LogTail(sLogName, "    ※恒生格式【%s】银证转帐明细文件结束！", sSecuNo);
           
            LogTail(sLogName, "    ※恒生格式【%s】存管帐户变动明细文件开始...", sSecuNo);
            /* 生成存管帐户变动明细 */
            if ( Gen_DetailFile(pub, initPara, sSecuNo, sBusinType, 1) != 0)
            {
               DBFreePArray2(&psResult, lCount);
               LogTail(sLogName, "    ★生成[%s]的存管帐户变动明细文件失败！", sSecuNo);
               return -5;
            }
            ERRLOGF("结束生成存管帐户变动明细%s","");
            LogTail(sLogName, "    ※恒生格式【%s】存管帐户变动明细文件结束！", sSecuNo);
            
            LogTail(sLogName, "    ※恒生格式【%s】客户帐户状态文件开始...", sSecuNo);
            /* 生成客户信息状态文件 */
            if ( Gen_DetailFile(pub, initPara, sSecuNo, sBusinType, 2) != 0)
            {
               DBFreePArray2(&psResult, lCount);
               LogTail(sLogName, "    ★生成[%s]的帐户状态文件失败！", sSecuNo);
               return -6;
            }
            ERRLOGF("结束生成客户信息状态文件%s","");
            LogTail(sLogName, "    ※恒生格式【%s】客户帐户状态文件结束！", sSecuNo);
            
            /* LABEL : 20090624-01-增加预约开户客户信息对账文件
             * MODIFY: huanghl@hundsun.com
             * REASON: 新增预约开户和激活交易
             * HANDLE: 增加预约开户客户信息文件bprecMMDD.dat
             */   
            LogTail(sLogName, "    ※恒生格式【%s】预约开户客户信息文件开始...", sSecuNo);
            /* 生成预约开户客户信息文件 */
            if ( Gen_DetailFile(pub, initPara, sSecuNo, sBusinType, 3) != 0)
            {
               DBFreePArray2(&psResult, lCount);
               LogTail(sLogName, "    ★生成[%s]的预约开户客户信息文件失败！", sSecuNo);
               return -7;
            }
            ERRLOGF("结束生成预约开户客户信息文件%s","");
            LogTail(sLogName, "    ※恒生格式【%s】预约开户客户信息文件结束！", sSecuNo);
            /* LABEL : 20090720-01-增加客户帐户类型变动文件
             * MODIFY: huanghl@hundsun.com
             * REASON: 增加客户帐户类型变动文件
             * HANDLE: 增加客户帐户类型变动文件bacctMMDD.dat
             */   
            LogTail(sLogName, "    ※恒生格式【%s】客户帐户类型变动文件开始...", sSecuNo);
            /* 生成客户帐户类型变动文件 */
            if ( Gen_DetailFile(pub, initPara, sSecuNo, sBusinType, 4) != 0)
            {
               DBFreePArray2(&psResult, lCount);
               LogTail(sLogName, "    ★生成[%s]的客户帐户类型变动文件失败！", sSecuNo);
               return -7;
            }
            ERRLOGF("结束生成客户帐户类型变动文件%s","");
            LogTail(sLogName, "    ※恒生格式【%s】客户帐户类型变动文件结束！", sSecuNo);
        }
        else if (cFileFlag == '1')  /*1-标准格式*/
        {
            LogTail(sLogName, "    ※标准格式【%s】银证转帐明细文件开始...", sSecuNo);
            /* 生成银证转帐明细文件 */
            if (Gen_DetailFile_1(pub,initPara, sSecuNo, sBusinType, 0) != 0)
            {
               DBFreePArray2(&psResult, lCount);
               LogTail(sLogName, "    ★生成[%s]的银证转帐明细文件失败！", sSecuNo);
               return -4;
            }
            LogTail(sLogName, "    ※标准格式【%s】银证转帐明细文件结束！", sSecuNo);
           
            LogTail(sLogName, "    ※标准格式【%s】存管帐户变动明细文件开始...", sSecuNo);
            /* 生成帐户变动明细 */
            if (Gen_DetailFile_1(pub, initPara, sSecuNo, sBusinType, 1) != 0)
            {
               DBFreePArray2(&psResult, lCount);
               LogTail(sLogName, "    ★生成[%s]的存管帐户变动明细文件失败！", sSecuNo);
               return -5;
            }
            LogTail(sLogName, "    ※标准格式【%s】存管帐户变动明细文件结束！", sSecuNo);
            
            LogTail(sLogName, "    ※标准格式【%s】客户帐户状态文件开始...", sSecuNo);
            /* 生成帐户状态明细 */
            if (Gen_DetailFile_1(pub, initPara, sSecuNo, sBusinType, 2) != 0)
            {
               DBFreePArray2(&psResult, lCount);
               LogTail(sLogName, "    ★生成[%s]的帐户状态文件失败！", sSecuNo);
               return -6;
            }
            LogTail(sLogName, "    ※恒生格式【%s】客户帐户状态文件结束！", sSecuNo);
        }
        else if (cFileFlag == '2')  /*2-国泰格式*/
        {
            LogTail(sLogName, "    ※国泰格式【%s】银证转帐明细文件开始...", sSecuNo);          
            /* 生成银证转帐明细文件 */
            if (Gen_DetailFile_2(pub,initPara, sSecuNo, sBusinType, 0) != 0)
            {
               DBFreePArray2(&psResult, lCount);
               LogTail(sLogName, "    ★生成[%s]的银证转帐明细文件失败！", sSecuNo);
               return -4;
            }
            LogTail(sLogName, "    ※国泰格式【%s】银证转帐明细文件结束！", sSecuNo);
           
            LogTail(sLogName, "    ※国泰格式【%s】存管帐户变动明细文件开始...", sSecuNo); 
            /* 生成帐户变动明细 */
            if (Gen_DetailFile_2(pub, initPara, sSecuNo, sBusinType, 1) != 0)
            {
               DBFreePArray2(&psResult, lCount);
               LogTail(sLogName, "    ★生成[%s]的存管帐户变动明细文件失败！", sSecuNo);
               return -5;
            }
            LogTail(sLogName, "    ※国泰格式【%s】存管帐户变动明细文件结束！", sSecuNo);

            LogTail(sLogName, "    ※国泰格式【%s】客户帐户状态文件开始...", sSecuNo);
            /* 生成帐户状态明细 */
            if (Gen_DetailFile_2(pub, initPara, sSecuNo, sBusinType, 2) != 0)
            {
               DBFreePArray2(&psResult, lCount);
               LogTail(sLogName, "    ★生成[%s]的帐户状态文件失败！", sSecuNo);
               return -6;
            }
            LogTail(sLogName, "    ※国泰格式【%s】客户帐户状态文件结束！", sSecuNo);
            
            /* LABEL : 20090819-01-增加国泰格式预约开户和联名卡文件
             * MODIFY: huanghl@hundsun.com
             * REASON: 新增预约开户和联名卡文件生成
             * HANDLE: 增加国泰格式预约开户和联名卡文件生成
             */
            LogTail(sLogName, "    ※国泰格式【%s】预约开户客户信息对账文件开始...", sSecuNo);
            /* 生成预约开户明细 */
            if (Gen_DetailFile_2(pub, initPara, sSecuNo, sBusinType, 5) != 0)
            {
               DBFreePArray2(&psResult, lCount);
               LogTail(sLogName, "    ★生成[%s]的预约开户客户信息对账文件失败！", sSecuNo);
               return -7;
            }
            LogTail(sLogName, "    ※国泰格式【%s】预约开户客户信息对账文件结束！", sSecuNo);
            
            LogTail(sLogName, "    ※国泰格式【%s】客户帐户类型变动文件开始...", sSecuNo);
            /* 生成联名卡客户明细 */
            if (Gen_DetailFile_2(pub, initPara, sSecuNo, sBusinType, 6) != 0)
            {
               DBFreePArray2(&psResult, lCount);
               LogTail(sLogName, "    ★生成[%s]的客户帐户类型变动文件失败！", sSecuNo);
               return -8;
            }
            LogTail(sLogName, "    ※国泰格式【%s】客户帐户类型变动文件结束！", sSecuNo);
        }
        else
        {
            LogTail(sLogName, "    ★错误的文件格式[%c]", cFileFlag);   
        }
    }
    LogTail(sLogName, "  ※生成文件结束！", "");
    DBFreePArray2(&psResult, lCount);
    
    /*20080514-01-记录银行后台核对状态*/
    if ( strlen(alltrim(pub->req.sOrganId)) > 0)
    {
        memset(&secuzjcg, 0x00, sizeof(TBSECUZJCG));
        
        /* LABEL : 20081006-01-增加融资融券，修改获取券商业务表模式 
         * MODIFY: 胡元方
         * REASON: 新增了“融资融券”业务类别
         * HANDLE: 原来在取券商业务表信息的时候限定了业务类别用“存管”，现改成取回全部后再判断
         */        
        /*
        DBGetSecuzjcg(pub->sSecuNo, BUSIN_YZCG, &secuzjcg);
        */        
        /* 为融资融券新增 */
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
        
    /*20120817-01-增加业务类别比较-begin*/
    /* 取转帐签约交易允许处理的业务类别 */
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
    
        /* 业务类别非法 */    
    if (strchr(param.param_value, secuzjcg.busin_type[0]) == NULL)
    {
        SET_ERROR_CODE(ERR_INVAILD_BUSINTYPE);
        return -1;
    }
    /*20120817-01-增加业务类别比较-end*/
        
        /* 增加结束 */
        if ((secuzjcg.clear_status[0] == SECU_CLRSTATUS_WCLEAR) ||
            (secuzjcg.clear_status[0] == SECU_CLRSTATUS_EBZZCHK))
        {
            SetSecuClearStatus(pub->req.sOrganId, SECU_CLRSTATUS_ESFILE);
        }
    }
    
    LogTail(sLogName, "※生成券商对帐文件结束！", "");
    
    return 0;
}

/******************************************************************************
函数名称∶ToShell
函数功能∶SHELL脚本调用，20070322-01-SHELL脚本调用
输入参数∶pub -- 交易参数
输出参数∶无
返 回 值∶==0 -- 成功
          -1  -- 需冲正证券端的失败
          -2  -- 需冲正银行端的失败
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
        SET_ERROR_MSG("生成文件交易成功!自动发送生成券商文件通知失败");
        return -1;
    }

    if ( lCount == 0 && strlen((pub->req).sOrganId) > 0)
    {
        SET_ERROR_CODE(ERR_SUCCESS);
        SET_ERROR_MSG("生成文件交易成功!券商状态不正常");
        return -2;
    }

    for(lCnt = 0; lCnt < lCount; lCnt ++)
    {
        memset(pub->sSecuNo, 0x00, sizeof(pub->sSecuNo));
        memcpy(pub->sSecuNo, (char *)GetSubStr(psResult[lCnt], 0), DB_SECUNO_LEN);
        
        /*获取深证通文件交换模式的券商*/
        /*日终文件交换方式:0-恒生通讯机传送(默认) 1-FTP方式 2-深证通模式(格式:2-8位交易日期-处理结果(0-未处理 1-完毕))*/
        memset(&param, 0x00, sizeof(TBPARAM));
        if (DBGetParam(pub->sSecuNo, "SWAP_FILE", &param) < 0)
            continue;
        /*1-FTP方式和2-深证通模式是用FTP上传文件*/
        else if (param.param_value[0] == '0') 
            continue;
        
        memset(sBuff, 0x00, sizeof(sBuff));
        if (GetCfgItem("HSRZCL", "UPFILE_BIN", sBuff) != 0)
            continue;
            
        memset(sCmd, 0x00, sizeof(sCmd));
        sprintf(sCmd, "%s %s DZMX %04ld %ld 1>/dev/null 2>/dev/null", sBuff, pub->sSecuNo, pub->lTransDate%10000, pub->lTransDate);
        lRet = system(sCmd);
        PrintLog(LEVEL_NORMAL,LOG_DEBUG_INFO, "文件传输FTP脚本[%s]", sCmd);
    }
    DBFreePArray2(&psResult, lCount);

    return 0;
}

/******************************************************************************
函数名称∶ToSecu
函数功能∶和证券通信
输入参数∶pub -- 交易参数
输出参数∶无
返 回 值∶==0 -- 成功
          -1  -- 需冲正证券端的失败
          -2  -- 需冲正银行端的失败
******************************************************************************/
static long ToSecu(TRANS99901 *pub)
{
    char    sBankReq[REQUEST_LEN];           /* 银行请求缓冲区 */
    char    sSecuAns[REQUEST_LEN];           /* 证券应答缓冲区 */
    char    sSql[SQL_BUF_LEN];
    char    sBuffer[256],sBuff[32];
    char    **psResult;
    long    lReqLen, lAnsLen, lCount, lRet, lCnt;
    FIX     *pfix;  
    TBTRANS    secutrans;
    TBSECUZJCG secuzjcg;
    TBSECUINFO secuinfo;
    YZPTKEYS   keys;                             /* 密钥结构 */    
/*
    sprintf(sSql,"select distinct secu_no from tbsecuzjcg where (status='%c' or status='%c')", SECU_STATUS_WCLEAR, SECU_STATUS_END);
    if ( strlen((pub->req).sOrganId) > 0)
        sprintf(sSql,"%s and secu_no ='%s'", sSql,(pub->req).sOrganId);
    lCount = DBExecSelect(1, 0, sSql,&psResult);
    if ( lCount < 0)
    {
        SET_ERROR_CODE(ERR_SUCCESS);
        SET_ERROR_MSG("生成文件交易成功!自动发送生成券商文件通知失败");

        return -1;
    }

    if ( lCount == 0 && strlen((pub->req).sOrganId) > 0)
    {
        SET_ERROR_CODE(ERR_SUCCESS);
        SET_ERROR_MSG("生成文件交易成功!券商状态不正常");

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
        /* 无需到证券则退出 */
        if (secutrans.secu_online[0] == TRANS_TOSECU_NO )
        {
            continue;
        }
        
        /*检查券商信息*/
        lRet = DBGetSecuinfo(pub->sSecuNo, &secuinfo); 
        if (lRet < 0)
        {
            continue;
        }

        /* LABEL : 20081006-01-增加融资融券，修改获取券商业务表模式 
         * MODIFY: 胡元方
         * REASON: 新增了“融资融券”业务类别
         * HANDLE: 在取不到业务类别“存管”券商后再取业务类别“融资融券”券商
         */        
        memset(&secuzjcg, 0x00, sizeof(TBSECUZJCG));
        if (DBGetSecuzjcg(pub->sSecuNo, BUSIN_YZCG, &secuzjcg) < 0)
        {
            if (DBGetSecuzjcg(pub->sSecuNo, BUSIN_RZRQ, &secuzjcg) < 0)     /*LABEL : 20081006-01-增加*/
            {               
                if (DBGetSecuzjcg(pub->sSecuNo, BUSIN_YZZZ, &secuzjcg) < 0)
                {
                    memset(sBuff, 0x00, sizeof(sBuff));
                    sprintf(sBuff, "%s-", pub->sSecuNo);
                    /*防止pub->sBankErrMsg溢出*/
                    if (strlen(pub->sBankErrMsg) + strlen(sBuff) < sizeof(pub->sBankErrMsg))
                        strcat(pub->sBankErrMsg, sBuff);
                
                    continue;
                }
            }
        }
        
        /* 获得券商密钥明文结构 */
        GetSecuZjcgKeys(&secuinfo, &keys);
        
        /* 转为证券FIX结构 */
        TransToSecu(pub, sBankReq, &lReqLen);

        /* 与证券端通讯 ,返回  -2ERR_TIMEOUT, -6ERR_RECVPKG 表示接收证券应答失败,此时需要冲证券*/
        lRet = TransWithSecu(&secuinfo, &keys, &secuzjcg, sBankReq, &lReqLen, sSecuAns, &lAnsLen);
        if (lRet != 0)
        {
            PrintLog(LEVEL_ERR, LOG_ERROR_INFO,  "触发券商[%s]取文件功能通讯失败!", pub->sSecuNo);
            memset(sBuff, 0x00, sizeof(sBuff));
            sprintf(sBuff, "%s-", pub->sSecuNo);
            /*防止pub->sBankErrMsg溢出*/
            if (strlen(pub->sBankErrMsg) + strlen(sBuff) < sizeof(pub->sBankErrMsg))
                strcat(pub->sBankErrMsg, sBuff);
            
            continue;
        }
        
        /* 处理证券应答 */
        if ((pfix = FIXopen(sSecuAns, lAnsLen, REQUEST_LEN, NULL)) == NULL)
        {
            PrintLog(LEVEL_ERR, LOG_ERROR_INFO,  "触发券商[%s]取文件功能应答失败!", pub->sSecuNo);
            memset(sBuff, 0x00, sizeof(sBuff));
            sprintf(sBuff, "%s-", pub->sSecuNo);
            /*防止pub->sBankErrMsg溢出*/
            if (strlen(pub->sBankErrMsg) + strlen(sBuff) < sizeof(pub->sBankErrMsg))
                strcat(pub->sBankErrMsg, sBuff);
             
            continue;
        }
    
        /* 记日志 */
        PrintFIXLog(LEVEL_NORMAL, LOG_RECV_SECU_ANS, pfix);
    
        /* 返回码错 置错误码和错误信息 */
        memset(sBuff, 0x00, sizeof(sBuff));
        FIXgets(pfix, "ErrorNo", sBuff, sizeof(sBuff));
        FIXgets(pfix, "ErrorInfo", sBuffer, sizeof(sBuffer));
        /*memcpy(pub->sErrMsg, sBankErrMsg, sizeof(pub->sErrMsg) - 1);*/
        FIXclose(pfix);
    
        if (memcmp(sBuff, ERR_SUCCESS, 4) != 0)
        {
            PrintLog(LEVEL_ERR, LOG_ERROR_INFO,  "触发券商[%s]取文件功能应答失败[%s-%s]!", pub->sSecuNo,sBuff,sBuffer);

            memset(sBuffer, 0x00, sizeof(sBuffer));
            sprintf(sBuffer, "%s-", pub->sSecuNo);
            /*防止pub->sBankErrMsg溢出*/
            if (strlen(pub->sBankErrMsg) + strlen(sBuff) < sizeof(pub->sBankErrMsg))
                strcat(pub->sBankErrMsg, sBuffer);

            continue;
        }
    }
    DBFreePArray2(&psResult, lCount);
    
    return 0;
}


/******************************************************************************
函数名称∶TransToSecu
函数功能∶生产向证券银转证的结构
输入参数∶pub     -- 交易参数
输出参数∶sBankReq -- 证券请求缓冲区
          lReqLen  -- 缓冲区长度
返 回 值∶无
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
    /* 记日志 */
    PrintFIXLog(LEVEL_NORMAL, LOG_SEND_SECU_REQ, pfix);

    FIXclose(pfix);
    
    return;
}

/******************************************************************************
函数名称∶Gen_DetailFile
函数功能∶生成单个券商的转帐明细文件
输入参数∶
         iFalg  :
                0  ----银证转帐明细
                1  ----存管帐户余额明细
输出参数∶无
返 回 值∶ 0 -- 成功
          <0 -- 失败
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
    struct CARDPARSE cardparse;  /*20090810-01-根据账户类型的不同设置调整账户类型变动文件的生成*/
    FILE  *fp, *fp1;
    TRDETAIL99901  trdetail99901;
    BALDETAIL99901 baldetail99901;
    ACCDETAIL99901 accdetail99901;
    PRECDETAIL99901 precdetail99901;
    TYPEDETAIL99901 typedetail99901;
    TBPARAM param;
    TBTRANS trans;
    
    /*存在转账流水异常情况不允许生成对帐明细文件*/
    if (iFlag == 0)
    {
        LogTail(sLogName, "      ※检查银证转帐冲正异常的情况...");
        /*1 检查冲正异常的情况*/
        memset(sSql, 0x00, sizeof(sSql));
        sprintf(sSql, "select secu_no from tbtransfer where secu_no='%s' and status not in ('%s', '%s') and repeal_status in ('1', '2', '3', '4')", psSecuNo, FLOW_SUCCESS, FLOW_FAILED);
        /*sprintf(sSql, "select secu_no from tbtransfer where secu_no='%s' and (repeal_status='2' or repeal_status='4') and (((trans_code='23701' or trans_code='23731') and status not in ('%s','%s','%s','%s')) or (trans_code='23702' and status not in ('%s','%s')))", 
                       psSecuNo, FLOW_SUCCESS, FLOW_FAILED, FLOW_SECU_SEND_F, FLOW_SECU_ERR, FLOW_SUCCESS, FLOW_FAILED);*/
        lCount = DBExecSelect(1, 0, sSql,&psResult);
        DBFreePArray2(&psResult, lCount);
        if ( lCount > 0)
        {
            SET_ERROR_CODE(ERR_FILEBYREPEAL);
            SET_ERROR_MSG("存在冲正异常的流水");
            PrintLog(LEVEL_ERR,LOG_ERROR_INFO,"存在冲正异常的流水");
            ERRLOGF("存在冲正异常的流水[%s]", sSql);
            LogTail(sLogName, "      ★存在银证转帐冲正异常的流水！");
            LogTail(sLogName, "        执行语句[%s]", sSql);            
            return -1;
        }
        LogTail(sLogName, "      ※检查银证转帐冲正异常的情况结束");
        
        /*2 检查资金处理和处理状态不一致的情况*/
        LogTail(sLogName, "      ※检查银证转帐资金处理和处理状态不一致的情况...");
        memset(sSql, 0x00, sizeof(sSql));
        sprintf(sSql, "select secu_no from tbtransfer where secu_no='%s' and ((balance_status='0' and status='%s') or (balance_status='1' and status<>'%s'))", psSecuNo, FLOW_SUCCESS, FLOW_SUCCESS);
        lCount = DBExecSelect(1, 0, sSql,&psResult);
        DBFreePArray2(&psResult, lCount);
        if ( lCount > 0)
        {
            SET_ERROR_CODE(ERR_FILEBYREPEAL);
            SET_ERROR_MSG("存在资金处理和处理状态不一致的流水");
            PrintLog(LEVEL_ERR,LOG_ERROR_INFO,"存在资金处理和处理状态不一致的流水");
            ERRLOGF("存在资金处理和处理状态不一致的流水[%s]", sSql);
            LogTail(sLogName, "      ★存在银证转帐资金处理和处理状态不一致的流水！");
            LogTail(sLogName, "        执行语句[%s]", sSql);            
            return -2;
        }
        LogTail(sLogName, "      ※检查银证转帐资金处理和处理状态不一致的情况结束");
    }
    else
    {
        /*生成存管帐户变动明细和生成客户信息状态文件*/
        /*获取券商内部营业部编号*/
        memset(&param, 0x00, sizeof(TBPARAM));
        if ((DBGetParam(psSecuNo, "RZ_REALBRANCH_99901", &param) == 0) && (param.param_value[0] == '0'))
        {
            lNum = 0;
        }
        else
        {
            memset(sSql, 0x00, sizeof(sSql));
            /* 20080605-02-获取包含缺省的券商营业部信息*/
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
        
        /*binfo文件中的状态是否使用新接口*/
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
        /*20090819-01删除bprecMMDD.dat,bacctMMDD.dat*/   
    if (iFlag == 0)
    {
        /* 生成银证转帐明细文件 */
        strcpy(sFileType, alltrim(GetSubStrViaFlag(sFTypes, 0, ',')));
    }
    else if (iFlag == 1)
    {
        /*生成存管帐户变动明细*/
        strcpy(sFileType, alltrim(GetSubStrViaFlag(sFTypes, 3, ',')));
    }
    else if (iFlag == 2)
    {
        /* 生成客户信息状态文件 */
        strcpy(sFileType, alltrim(GetSubStrViaFlag(sFTypes, 2, ',')));
    }
   /* LABEL : 20090624-01-增加预约开户客户信息对账文件
    * MODIFY: huanghl@hundsun.com
    * REASON: 新增预约开户和激活交易
    * HANDLE: 增加预约开户客户信息文件bprecMMDD.dat
    */
    else if (iFlag == 3)
    {
        /* 生成预约开户客户信息文件 */
        strcpy(sFileType, alltrim(GetSubStrViaFlag(sFTypes, 4, ',')));
    } 
   /* LABEL : 20090720-01-增加客户帐户类型变动文件
    * MODIFY: huanghl@hundsun.com
    * REASON: 增加客户帐户类型变动文件
    * HANDLE: 增加客户帐户类型变动文件bacctMMDD.dat
    */
    else if (iFlag == 4)
    {
        /* 生成预约开户客户信息文件 */
        strcpy(sFileType, alltrim(GetSubStrViaFlag(sFTypes, 5, ',')));
    }         
    /*如设置文件名为空，则不处理*/
    if (strlen(alltrim(sFileType)) == 0)
    {
        return 0;
    }

    /* 创建文件 */
    memset(sFileName, 0x00, sizeof(sFileName));
    sprintf(sFileName, "%s/%s/%s%s",sSecuDir,psSecuNo,SENDFILE_DIR,ReplaceYYMMDD(sFileType, pub->lTransDate));
    fp = fopen(sFileName, "w+");
    if ( fp == NULL)
    {
        SET_ERROR_CODE(ERR_CREATFILE);
        PrintLog(LEVEL_ERR, LOG_ERROR_INFO, "创建文件失败[%s]!", sFileName);
        LogTail(sLogName, "      ★创建文件失败[%s]!", sFileName);
        return -1;
    }

    /*
     *  生成SQL语句
     */
    /* LABEL : 20081010-01 融资融券业务中使用同一券商编号时对账文件(恒生模式)增加业务类型
     * MODIFY: 胡元方20081010
     * REASON: 新增了“融资融券”业务类别、需要修改生成的文件
     * HANDLE: 在文件记录的最后追加“业务类型”字段
     */      

    /* 20081010-01取对账文件中是否增加业务类别标志 */
    cDzfileBusintypeFlag='0';   /* 默认文件不需要增加业务类别字段 */
    memset(&param, 0x00, sizeof(param));
    if (DBGetParam(psSecuNo, "RZ_DZFILE_BUSINTYPE", &param) == 0)
    {
        cDzfileBusintypeFlag = param.param_value[0];
    }                

    memset(sSql, 0x00, sizeof(sSql));
    if (iFlag == 0)
    {
        LogTail(sLogName, "      ※查询成功转帐流水记录数...");
        /*查询成功转帐流水*/
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
        LogTail(sLogName, "      ※查询成功转帐流水记录数结束，总记录数[%ld]", lTransNum);

        LogTail(sLogName, "      ※生成转帐明细文件...");

        /*查询成功转帐流水*/        
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
            LogTail(sLogName, "        ※查询成功转帐流水数据[%ld-%ld]...", i+1, lTransNum<(i+SELECT_NUM)?lTransNum:(i+SELECT_NUM));
            lCount = DBExecSelect(i+1, SELECT_NUM, sSql,&psResult);
            if ( lCount < 0)
            {
                SET_ERROR_CODE(ERR_DATABASE);
                return -2;
            }
            LogTail(sLogName, "        ※查询成功转帐流水数据结束");

            LogTail(sLogName, "        ※生成成功转帐明细文件...");
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
                /* 20081010-01 业务类别 */
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
    
                /*写文件*/
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

            LogTail(sLogName, "        ※生成成功转帐明细文件结束![%ld-%ld]", i+1, lTransNum<(i+SELECT_NUM)?lTransNum:(i+SELECT_NUM));
        }
        LogTail(sLogName, "      ※生成转帐明细文件结束");
    }
    else if (iFlag == 1)
    {
        LogTail(sLogName, "      ※查询存管帐户变动明细数据...");

        /* LABEL : 20110506-02-销户区分预指定销户和正常销户
         * MODIFY: huanghl@hundsun.com
         * REASON: 北京银行需求销户区分预指定销户和正常销户
         * HANDLE：增加1032-预指定销户
         */
        /*恒生接口根据换卡券商联机标志在券商账户对账文件中记录换卡流水信息*/
        memset(sQuery, 0x00, sizeof(sQuery));
        memset(&trans, 0x00, sizeof(TBTRANS));
        DBGetTrans(psSecuNo, "25710", &trans);
        if (trans.secu_online[0] == '0')
            strcpy(sQuery, "(1001,1002,1015,1016,1017,1018,1019,1022,1032)");
        else
            strcpy(sQuery, "(1001,1002,1015,1016,1017,1018,1019,1022,1004,1032)");
        
        /*生成存管帐户变动明细*/
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
        LogTail(sLogName, "      ※查询存管帐户变动明细数据结束");

        LogTail(sLogName, "      ※生成存管帐户变动明细文件...");
        for(lCnt = 0; lCnt < lCount; lCnt ++)    
        {
            /*生成存管帐户变动明细*/
            memset(&accdetail99901, 0x00, sizeof(accdetail99901));
            accdetail99901serial_no = atol((char *)GetSubStr(psResult[lCnt],0)); 
            memcpy(accdetail99901bank_id         , (char *)GetSubStr(psResult[lCnt],1 ), sizeof(accdetail99901bank_id));
            memcpy(accdetail99901secu_use_account, (char *)GetSubStr(psResult[lCnt],2 ), sizeof(accdetail99901secu_use_account));
            memcpy(accdetail99901secu_no         , (char *)GetSubStr(psResult[lCnt],3 ), sizeof(accdetail99901secu_no));
            /*在bedit文件中填写券商内部营业部编号*/
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
            /* 20081010-01 业务类别 */ 
            memcpy(accdetail99901busin_type      , (char *)GetSubStr(psResult[lCnt],14 ), 1);

            /*调整客户账户类明细和客户状态文件格式*/
            /* 20081010-01 业务类别 
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

            /* 20081010-01 业务类别 */
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
        
        LogTail(sLogName, "      ※生成存管帐户变动明细文件结束");
    }
    else if (iFlag == 2)
    {
        LogTail(sLogName, "      ※查询客户信息状态数据...");

        /*20080522-01-带卡预指定客户在证券方为正常客户*/
        memset(&param, 0x00, sizeof(TBPARAM));
        DBGetParam(psSecuNo,"FORCE_MIGRATE_STATUS", &param);
        /*预指定交易有卡时是否强制客户状态为预指定状态:0-否(默认),1-是*/
        if ( param.param_value[0] == '1')
            cForceMigrateFlag = '1';
        else
            cForceMigrateFlag = '0';
        /* LABEL : 20110506-02-销户区分预指定销户和正常销户
         * MODIFY: huanghl@hundsun.com
         * REASON: 北京银行需求销户区分预指定销户和正常销户
         * HANDLE：增加FLAG_XYZD-预指定销户
         */
        /* 生成客户信息状态文件 */
        /* 20080708-01-保证账户状态文件记录唯一性增加distinct*/
        memset(sSql, 0x00, sizeof(sSql));        
        /* 20081010-01
        sprintf(sSql,"select distinct a.bank_id,a.secu_use_account,a.secu_no,a.secu_branch,a.secu_account,a.client_name,a.id_kind,a.id_code,a.money_kind,a.status,b.occur_balance,a.client_account from tbclientyzzz a,tbaccountedit b"); 
        */
        sprintf(sSql,"select distinct a.bank_id,a.secu_use_account,a.secu_no,a.secu_branch,a.secu_account,a.client_name,a.id_kind,a.id_code,a.money_kind,a.status,b.occur_balance,a.client_account,a.busin_type from tbclientyzzz a,tbaccountedit b"); 
        sprintf(sSql,"%s where a.secu_no='%s'", sSql, psSecuNo);
        sprintf(sSql,"%s and a.secu_no=b.secu_no and a.secu_account=b.secu_account and a.busin_type=b.busin_type and a.money_kind=b.money_kind", sSql);
        /* 20080523-01-账户状态文件中避免生成当日签约的多条记录 */
        /* 20120929-01-调整销户日期，通过参数控制 */
        if (pub->cNeedModify != '0')
        {
            /*20130916-01-重新赋值宏定义为整数型的business_flag字段 %ld->%d */
            sprintf(sSql,"%s and ((a.open_date=%ld and b.business_flag in (%d,%d,%d,%d,%d,%d)) or (a.open_date<%ld and a.status='0' and b.business_flag=%d))", 
                      sSql, pub->lTransDate, FLAG_KHKH, FLAG_KHXH, FLAG_ZZKH, FLAG_ZZXH, FLAG_YYZD,FLAG_XYZD,pub->lTransDate, FLAG_YZKH);
        }
        else
        {
            /*20130916-01-重新赋值宏定义为整数型的business_flag字段 %ld->%d */
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
        LogTail(sLogName, "      ※查询客户信息状态数据结束");

        LogTail(sLogName, "      ※生成存管帐户状态文件...");
        for(lCnt = 0; lCnt < lCount; lCnt ++)    
        {
            memset(&accdetail99901, 0x00, sizeof(accdetail99901));
            accdetail99901serial_no = ++lSerialNo; 
            memcpy(accdetail99901bank_id         , (char *)GetSubStr(psResult[lCnt],0 ), sizeof(accdetail99901bank_id));
            memcpy(accdetail99901secu_use_account, (char *)GetSubStr(psResult[lCnt],1 ), sizeof(accdetail99901secu_use_account));
            memcpy(accdetail99901secu_no         , (char *)GetSubStr(psResult[lCnt],2 ), sizeof(accdetail99901secu_no));
            /*在binfo文件中填写券商内部营业部编号*/
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
            /* 20081010-01 业务类别 */
            memcpy(accdetail99901busin_type      , (char *)GetSubStr(psResult[lCnt],12 ), 1);
            
            /*解约状态*/
            if (accdetail99901status[0] == '3')
            {    
                accdetail99901status[0] = '1';
            }
            /*预指定状态*/
            else if (accdetail99901status[0] == '5')
            {
                /*20071013-01-带卡预指定客户在证券方为正常客户*/
                memset(sBuff, 0x00, sizeof(sBuff));
                memcpy(sBuff, (char *)GetSubStr(psResult[lCnt],11), DB_BNKACC_LEN);
                /*带卡号并且强制预指定状态，则送券商为正常状态*/
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
            /*正常状态*/
            else
                accdetail99901status[0] = '0';
            
            /* 20081010-01 业务类别
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

            /* 20081010-01 业务类别 */
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

        LogTail(sLogName, "      ※生成存管帐户状态文件结束");
    }
   /* LABEL : 20090624-01-增加预约开户客户信息对账文件
    * MODIFY: huanghl@hundsun.com
    * REASON: 新增预约开户和激活交易
    * HANDLE: 增加预约开户客户信息文件bprecMMDD.dat
    */
    else if (iFlag == 3)
    {
        LogTail(sLogName, "      ※查询预约开户客户信息数据...");

        /* 生成预约开户客户信息状态文件 */
        memset(sSql, 0x00, sizeof(sSql));        
        sprintf(sSql,"select distinct a.bank_id,a.secu_use_account,a.secu_no,a.secu_branch_b,a.secu_branch_s,a.secu_account,a.client_name,a.id_kind,a.id_code,a.money_kind,a.status,a.busin_type from tbpreacc a,tbaccountedit b"); 
        sprintf(sSql,"%s where a.secu_no='%s'", sSql, psSecuNo);
        sprintf(sSql,"%s and a.secu_no=b.secu_no and a.secu_account=b.secu_account and a.busin_type=b.busin_type and a.money_kind=b.money_kind and a.fund_account=b.fund_account", sSql);
        /*20130916-01-重新赋值宏定义为整数型的business_flag字段 %ld->%d */
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
        LogTail(sLogName, "      ※查询预约开户客户信息数据结束");

        LogTail(sLogName, "      ※生成预约开户客户信息文件...");
        for(lCnt = 0; lCnt < lCount; lCnt ++)    
        {
           /* LABEL : 20090804-01-初始化参数和增加业务类型
            * MODIFY: huanghl@hundsun.com
            * REASON: lZDBranch和lJHBranch未初始化
            * HANDLE: 初始化lZDBranch和lJHBranch
            */
            lZDBranch = 0;/*预约开户时指定的券商营业部*/
            lJHBranch = 0;/*券商发激活时使用的券商营业部*/
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
                    /*当开户指定的营业部和激活使用的营业部均与券商内部营业部编号匹配时退出*/
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
            
            /*正常状态*/
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

        LogTail(sLogName, "      ※生成预约开户客户信息文件结束");
    }
    
   /* LABEL : 20090720-01-增加客户帐户类型变动文件
    * MODIFY: huanghl@hundsun.com
    * REASON: 增加客户帐户类型变动文件
    * HANDLE: 增加客户帐户类型变动文件bacctMMDD.dat
    */
    else if (iFlag == 4)
    {
        memset(&param, 0x00, sizeof(TBPARAM));
        memset(sRuleFlag, 0x00, sizeof(sRuleFlag));
        /*CHKGET_ACCTYPE_RUAL联名卡规则参数：若指定券商未配此参数以系统联名卡规则为准,规则为空则不生成文件*/
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
        
        /* LABEL : 20090810-01-根据账户类型的不同设置调整账户类型变动文件的生成
         * MODIFY: zhuwl@hundsun.com
         * REASON: 银行联名卡的账户标志比较复杂
         * HANDLE: 增加对定位匹配和范围匹配的账户类型参数配置的支持
         */
        /*解析账户类型的设置*/
        /* LABEL : 20090901-01-增加变量cardparse初始化
         * MODIFY: wanggk@hundsun.com
         * REASON: 变量cardparse未初始化
         * HANDLE: 增加变量cardparse初始化
         */ 
        memset(&cardparse, 0x00, sizeof(cardparse)); 
        ParseParam(sRuleFlag, &cardparse);
        
         /* LABEL : 20090804-01-初始化参数和增加业务类型
          * MODIFY: huanghl@hundsun.com
          * REASON: FLAG_QZHK和FLAG_YZKH业务类型也涉及到银行卡类型变动
          * HANDLE: 增加1024-FLAG_QZHK和1019-FLAG_YZKH类型
          */
        LogTail(sLogName, "      ※查询客户帐户类型数据...");
        memset(sSql, 0x00, sizeof(sSql));
        sprintf(sSql,"select distinct c.secu_no,c.secu_account,c.money_kind,c.busin_type,c.bank_id,c.secu_use_account,c.client_account,c.secu_branch,c.client_name,c.id_kind,c.id_code from tbaccountedit a ,tbclientyzzz c");
        /* LABEL : 20090819-01-增加国泰格式预约开户和联名卡文件
         * MODIFY: huanghl@hundsun.com
         * REASON: 25710换卡后币种可能为空
         * HANDLE: 两条sql查询语句中匹配条件删除币种
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
        LogTail(sLogName, "      ※查询客户帐户类型数据结束");
        LogTail(sLogName, "      ※生成客户帐户类型变动文件...");
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

            /*使用当日第一条tbaccountedit记录的卡号为当日旧卡号*/
            memset(sSql,0x00,sizeof(sSql));
            /* LABEL : 20090804-01-初始化参数和增加业务类型
             * MODIFY: huanghl@hundsun.com
             * REASON: FLAG_QZHK和FLAG_YZKH业务类型也涉及到银行卡类型变动
             * HANDLE: 增加1024-FLAG_QZHK和1019-FLAG_YZKH类型
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
            /*当日开户则默认旧卡类型为普通卡0*/
            /* LABEL : 20090804-01-初始化参数和增加业务类型
             * MODIFY: huanghl@hundsun.com
             * REASON: FLAG_QZHK和FLAG_YZKH业务类型也涉及到银行卡类型变动
             * HANDLE: 增加1024-FLAG_QZHK和1019-FLAG_YZKH类型
             */
            if ((strcmp(business_flag,"1001") == 0) ||(strcmp(business_flag,"1019") == 0))
            {
                lOld = 0;
            }
            else
            {
                /*检查旧卡类型,0-普通卡,1-联名卡,-1-查询失败*/
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
            /*lType表示新旧卡号的类型差*/
            lType=lNew-lOld;
            DBFreePArray2(&psResult2, lnum);
            /*0-类型未发生改变,不记录*/
            if (lType == 0)
            {
                continue;
            }
            /*1-普通卡换为联名卡或新开联名卡*/
            else if (lType == 1)
            {
                typedetail99901acc_type[0]='1';/*1-联名卡*/
                typedetail99901serial_no = ++lSerialNo;
            }
            /*-1-联名卡换为普通卡*/
            else if (lType == -1)
            {
                typedetail99901acc_type[0]='0';/*0-普通卡*/
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
        LogTail(sLogName, "      ※生成客户帐户类型变动文件...");
    }
    
    fclose(fp);

    return 0;
}           

/******************************************************************************
函数名称∶Gen_DetailFile_1
函数功能∶生成单个券商的文件(标准接口)
输入参数∶
         iFalg  :
                0  ----银证转帐明细
                1  ----存管帐户余额明细
输出参数∶无
返 回 值∶ 0 -- 成功
          <0 -- 失败
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

    /*20130326-01-增加证件类型转换处理*/
    memset(sBuffer, 0x00, sizeof(sBuffer));
    sprintf(sBuffer, "%s/etc/secuxml.dat", getenv("HOME"));
    SetDictEnv(sBuffer);

    /*存在转账流水冲正异常不允许生成对帐明细文件*/
    if (iFlag == 0)
    {
        /*1 检查冲正异常的情况*/
        memset(sSql, 0x00, sizeof(sSql));
        sprintf(sSql, "select secu_no from tbtransfer where secu_no='%s' and status not in ('%s', '%s') and repeal_status in ('1', '2', '3', '4')", psSecuNo, FLOW_SUCCESS, FLOW_FAILED);
        /*sprintf(sSql, "select secu_no from tbtransfer where secu_no='%s' and (repeal_status='2' or repeal_status='4') and (((trans_code='23701' or trans_code='23731') and status not in ('%s','%s','%s','%s')) or (trans_code='23702' and status not in ('%s','%s')))", 
                       psSecuNo, FLOW_SUCCESS, FLOW_FAILED, FLOW_SECU_SEND_F, FLOW_SECU_ERR, FLOW_SUCCESS, FLOW_FAILED);*/
        lCount = DBExecSelect(1, 0, sSql,&psResult);
        DBFreePArray2(&psResult, lCount);
        if ( lCount > 0)
        {
            SET_ERROR_CODE(ERR_FILEBYREPEAL);
            SET_ERROR_MSG("存在冲正异常的流水");
            PrintLog(LEVEL_ERR,LOG_ERROR_INFO,"存在冲正异常的流水");
            ERRLOGF("存在冲正异常的流水[%s]", sSql);
            return -1;
        }
        
        /*2 检查资金处理和处理状态不一致的情况*/
        memset(sSql, 0x00, sizeof(sSql));
        sprintf(sSql, "select secu_no from tbtransfer where secu_no='%s' and ((balance_status='0' and status='%s') or (balance_status='1' and status<>'%s'))", psSecuNo, FLOW_SUCCESS, FLOW_SUCCESS);
        lCount = DBExecSelect(1, 0, sSql,&psResult);
        DBFreePArray2(&psResult, lCount);
        if ( lCount > 0)
        {
            SET_ERROR_CODE(ERR_FILEBYREPEAL);
            SET_ERROR_MSG("存在资金处理和处理状态不一致的流水");
            PrintLog(LEVEL_ERR,LOG_ERROR_INFO,"存在资金处理和处理状态不一致的流水");
            ERRLOGF("存在资金处理和处理状态不一致的流水[%s]", sSql);
            return -2;
        }
    }    

    memset(&param, 0x00, sizeof(param));
    memset(sFTypes, 0x00, sizeof(sFTypes));
    if (DBGetParam(psSecuNo, "SECUFILE", &param) == 0)
        sprintf(sFTypes, "%s", param.param_value);
    else
        sprintf(sFTypes, "B_CHK01_YYYYMMDD,B_CHK02_YYYYMMDD,B_CHK03_YYYYMMDD");

    if (iFlag == 0)  /*B_CHK01_YYYYMMDD:转账交易明细对账文件*/
    {
        strcpy(sFileType, alltrim(GetSubStrViaFlag(sFTypes, 0, ',')));
    }
    else if (iFlag == 1) /*B_CHK02_YYYYMMDD:客户账户状态对账文件*/
    {
         strcpy(sFileType, alltrim(GetSubStrViaFlag(sFTypes, 1, ',')));
    }
    else if (iFlag == 2) /*B_CHK03_YYYYMMDD:账户类交易明细对账文件*/
    {
         strcpy(sFileType, alltrim(GetSubStrViaFlag(sFTypes, 2, ',')));
    }

    /* 创建文件 */
    memset(sFileName, 0x00, sizeof(sFileName));
    sprintf(sFileName, "%s/%s/qsjg/%s",sSecuDir,psSecuNo,ReplaceYYMMDD(sFileType, pub->lTransDate));
    fp = fopen(sFileName, "w+");
    if ( fp == NULL)
    {
        SET_ERROR_CODE(ERR_CREATFILE);
        PrintLog(LEVEL_ERR, LOG_ERROR_INFO, "创建文件失败%s", "");
        return -1;
    }

    /*获取券商内部营业部编号*/
    memset(sSql, 0x00, sizeof(sSql));
    /* 20080605-02-获取包含缺省的券商营业部信息*/
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
     *  生成SQL语句
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
        /* 20080605-01-优化账户状态文件的SQL语句*/
        /****
        sprintf(sSql,"select a.bank_id, a.secu_no, a.secu_branch, a.open_date, a.secu_use_account, a.secu_account, a.client_name, a.money_kind, a.status from tbclientyzzz a,tbaccountedit b"); 
        sprintf(sSql,"%s where a.secu_no='%s'", sSql, psSecuNo);
        sprintf(sSql,"%s and a.secu_no=b.secu_no and a.secu_account=b.secu_account and a.busin_type=b.busin_type and a.money_kind=b.money_kind", sSql);
        /--* 20080523-01-账户状态文件中避免生成当日签约的多条记录 *--/
        /--*sprintf(sSql,"%s and (a.open_date=%ld or (a.open_date<%ld and a.status='0' and b.business_flag=%ld))", sSql, pub->lTransDate, pub->lTransDate, FLAG_YZKH);*--/
        sprintf(sSql,"%s and ((a.open_date=%ld and b.business_flag in (%ld,%ld,%ld,%ld,%ld)) or (a.open_date<%ld and a.status='0' and b.business_flag=%ld))", 
                      sSql, pub->lTransDate, FLAG_KHKH, FLAG_KHXH, FLAG_ZZKH, FLAG_ZZXH, FLAG_YYZD,pub->lTransDate, FLAG_YZKH);
        ****/
        sprintf(sSql,"select distinct a.bank_id, a.secu_no, a.secu_branch, a.open_date, a.secu_use_account, a.secu_account, a.client_name, a.money_kind, a.status from tbclientyzzz a,tbaccountedit b"); 
        sprintf(sSql,"%s where a.secu_no='%s'", sSql, psSecuNo);
        sprintf(sSql,"%s and a.secu_no=b.secu_no and a.secu_account=b.secu_account and a.busin_type=b.busin_type and a.money_kind=b.money_kind", sSql);
        if (strlen(alltrim(psBusinType)) > 0)
            sprintf(sSql,"%s and a.busin_type='%c'", sSql, psBusinType[0]);

        /*20080522-01-带卡预指定客户在证券方为正常客户*/
        memset(&param, 0x00, sizeof(TBPARAM));
        DBGetParam(psSecuNo,"FORCE_MIGRATE_STATUS", &param);
        /*预指定交易有卡时是否强制客户状态为预指定状态:0-否(默认),1-是*/
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
            /*20080530-01-标志版本中转账明细结构体初始化*/
            memset(&trdetail99901, 0x00, sizeof(trdetail99901));
            memset(sGTBankKind, 0x00, sizeof(sGTBankKind));
            memset(sGBMoneyKind, 0x00, sizeof(sGBMoneyKind));
            /* LABEL : 20091123-01-初始化sTransCode变量
             * MODIFY: huayl@hundsun.com
             * REASON: 未初始化变量，生成文件有乱码
             * HANDLE: 初始化变量
             */ 
            memset(sTransCode, 0x00, sizeof(sTransCode));
            /*20110819-01-初始化sSourceSide*/
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

            /*写文件*/
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
                /*20080522-01-带卡预指定客户在证券方为正常客户*/
                /*带卡号并且强制预指定状态，则送券商为正常状态*/
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
                     "2",  /*20080522-02-调整标准接口中的钞汇标志*/
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
            
            /* 20130326-01-增加证件类型转换处理 */
            LookUp(104, sIdKind, accdetail99901id_kind);
            if ( strlen(alltrim(sIdKind)) == 0 )
            {
                Get1ViaIdKind(accdetail99901id_kind, sIdKind);
            }
            
            memcpy(accdetail99901id_code         , (char *)GetSubStr(psResult[lCnt],11), sizeof(accdetail99901id_code) - 1);
            memcpy(sBusinessFlag, (char *)GetSubStr(psResult[lCnt],13), 4);
            strcpy(sTransCode, (char *)GetSubStr(psResult[lCnt],12 ));
            /*过滤浦发行的存管激活25730交易*/
            if (memcmp(sTransCode, "25730", 5) == 0)
                continue;
            
            /* 20141014-wangsy-过滤批量换卡交易流水 */
            if (memcmp(sTransCode, "20922", 5) == 0)
                continue;
            
            Get1TransViaTCode(sTransCode, sIsoTransSource, sIsoTransCode);
            /*标准版账户交易明细文件中预指定交易码的特殊处理*/
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
                        "2", /*20080522-02-调整标准接口中的钞汇标志*/
                        accdetail99901occur_bala*100
                    );
        }
    }

    fclose(fp);
    DBFreePArray2(&psResult, lCount);
    
    return 0;
}
            
/******************************************************************************
函数名称∶Gen_DetailFile_2
函数功能∶生成单个券商的转帐明细文件(国泰接口)
输入参数∶
         iFalg  :
                0  ----银证转帐明细
                1  ----存管帐户余额明细
输出参数∶无
返 回 值∶ 0 -- 成功
          <0 -- 失败
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

    /*存在转账流水冲正异常不允许生成对帐明细文件*/
    if (iFlag == 0)
    {
        /*1 检查冲正异常的情况*/
        memset(sSql, 0x00, sizeof(sSql));
        sprintf(sSql, "select secu_no from tbtransfer where secu_no='%s' and status not in ('%s', '%s') and repeal_status in ('1', '2', '3', '4')", psSecuNo, FLOW_SUCCESS, FLOW_FAILED);
        /*sprintf(sSql, "select secu_no from tbtransfer where secu_no='%s' and (repeal_status='2' or repeal_status='4') and (((trans_code='23701' or trans_code='23731') and status not in ('%s','%s','%s','%s')) or (trans_code='23702' and status not in ('%s','%s')))", 
                       psSecuNo, FLOW_SUCCESS, FLOW_FAILED, FLOW_SECU_SEND_F, FLOW_SECU_ERR, FLOW_SUCCESS, FLOW_FAILED);*/
        lCount = DBExecSelect(1, 0, sSql,&psResult);
        DBFreePArray2(&psResult, lCount);
        if ( lCount > 0)
        {
            SET_ERROR_CODE(ERR_FILEBYREPEAL);
            SET_ERROR_MSG("存在冲正异常的流水");
            PrintLog(LEVEL_ERR,LOG_ERROR_INFO,"存在冲正异常的流水");
            ERRLOGF("存在冲正异常的流水[%s]", sSql);
            return -1;
        }
        
        /*2 检查资金处理和处理状态不一致的情况*/
        memset(sSql, 0x00, sizeof(sSql));
        sprintf(sSql, "select secu_no from tbtransfer where secu_no='%s' and ((balance_status='0' and status='%s') or (balance_status='1' and status<>'%s'))", psSecuNo, FLOW_SUCCESS, FLOW_SUCCESS);
        lCount = DBExecSelect(1, 0, sSql,&psResult);
        DBFreePArray2(&psResult, lCount);
        if ( lCount > 0)
        {
            SET_ERROR_CODE(ERR_FILEBYREPEAL);
            SET_ERROR_MSG("存在资金处理和处理状态不一致的流水");
            PrintLog(LEVEL_ERR,LOG_ERROR_INFO,"存在资金处理和处理状态不一致的流水");
            ERRLOGF("存在资金处理和处理状态不一致的流水[%s]", sSql);
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
    /* LABEL : 20090819-01-增加国泰格式预约开户和联名卡文件
     * MODIFY: huanghl@hundsun.com
     * REASON: 新增预约开户和联名卡文件生成
     * HANDLE: 取预约开户和联名卡文件名
     */
    else if (iFlag == 5)
    {
         strcpy(sFileType, alltrim(GetSubStrViaFlag(sFTypes, 5, ',')));
    }
    else if (iFlag == 6)
    {
         strcpy(sFileType, alltrim(GetSubStrViaFlag(sFTypes, 6, ',')));
    }
    
    /*如设置文件名为空，则不处理*/
    if (strlen(alltrim(sFileType)) == 0)
    {
        return 0;
    }
    /* 创建文件 */
    memset(sFileName, 0x00, sizeof(sFileName));
    sprintf(sFileName, "%s/%s/qsjg/%s",sSecuDir,psSecuNo,ReplaceYYMMDD(sFileType, pub->lTransDate));
    fp = fopen(sFileName, "w+");
    if ( fp == NULL)
    {
        SET_ERROR_CODE(ERR_CREATFILE);
        PrintLog(LEVEL_ERR, LOG_ERROR_INFO, "创建文件失败%s", "");
        return -1;
    }
    
    /* LABEL : 20090819-01-增加国泰格式预约开户和联名卡文件
     * MODIFY: huanghl@hundsun.com
     * REASON: 新增预约开户和联名卡文件生成
     * HANDLE: 根据参数判断是否需要加业务类别字段
     */
    cDzfileBusintypeFlag='0';   /* 默认文件不需要增加业务类别字段 */
    memset(&param, 0x00, sizeof(param));
    if (DBGetParam(psSecuNo, "RZ_DZFILE_BUSINTYPE", &param) == 0)
    {
        cDzfileBusintypeFlag = param.param_value[0];
    }
    
    /*获取券商内部营业部编号*/
    memset(sSql, 0x00, sizeof(sSql));
    /* 20080605-02-获取包含缺省的券商营业部信息*/
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
     *  生成SQL语句
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
        /* LABEL : 20110506-02-销户区分预指定销户和正常销户
         * MODIFY: huanghl@hundsun.com
         * REASON: 北京银行需求销户区分预指定销户和正常销户
         * HANDLE：增加FLAG_XYZD-预指定销户
         */
        /* 20080708-01-保证账户状态文件记录唯一性增加distinct*/
        sprintf(sSql,"select distinct a.bank_id, a.secu_no, a.secu_branch, a.open_date, a.secu_use_account, a.secu_account, a.client_name, a.money_kind, a.status from tbclientyzzz a,tbaccountedit b"); 
        sprintf(sSql,"%s where a.secu_no='%s'", sSql, psSecuNo);
        sprintf(sSql,"%s and a.secu_no=b.secu_no and a.secu_account=b.secu_account and a.busin_type=b.busin_type and a.money_kind=b.money_kind", sSql);
        /* 20080523-01-账户状态文件中避免生成当日签约的多条记录 */
        /*sprintf(sSql,"%s and (a.open_date=%ld or (a.open_date<%ld and a.status='0' and b.business_flag=%ld))", sSql, pub->lTransDate, pub->lTransDate, FLAG_YZKH);*/
        /* 20120929-01-调整销户日期，通过参数控制 */
        if (pub->cNeedModify != '0')
        {
            /*20130916-01-重新赋值宏定义为整数型的business_flag字段 %ld->%d */
            sprintf(sSql,"%s and ((a.open_date=%ld and b.business_flag in (%d,%d,%d,%d,%d,%d)) or (a.open_date<%ld and a.status='0' and b.business_flag=%d))", 
                      sSql, pub->lTransDate, FLAG_KHKH, FLAG_KHXH, FLAG_ZZKH, FLAG_ZZXH, FLAG_YYZD, FLAG_XYZD, pub->lTransDate, FLAG_YZKH);
        }
        else
        {
            /*20130916-01-重新赋值宏定义为整数型的business_flag字段 %ld->%d */
            sprintf(sSql,"%s and ((a.open_date=%ld and b.business_flag in (%d,%d,%d)) or (a.open_date<%ld and a.status='0' and b.business_flag=%d) or (a.open_date<%ld and a.status='3' and b.business_flag in (%d,%d,%d)) )", 
                      sSql, pub->lTransDate, FLAG_KHKH, FLAG_ZZKH, FLAG_YYZD, pub->lTransDate, FLAG_YZKH, pub->lTransDate, FLAG_KHXH, FLAG_XYZD, FLAG_ZZXH);
        }
        
        if (strlen(alltrim(psBusinType)) > 0)
            sprintf(sSql,"%s and a.busin_type='%c'", sSql, psBusinType[0]);

        /*20080522-01-带卡预指定客户在证券方为正常客户*/
        memset(&param, 0x00, sizeof(TBPARAM));
        DBGetParam(psSecuNo,"FORCE_MIGRATE_STATUS", &param);
        /*预指定交易有卡时是否强制客户状态为预指定状态:0-否(默认),1-是*/
        if ( param.param_value[0] == '1')
            cForceMigrateFlag = '1';
        else
            cForceMigrateFlag = '0';

    }
    else if (iFlag == 2)
    {
        /* LABEL : 20110506-02-销户区分预指定销户和正常销户
         * MODIFY: huanghl@hundsun.com
         * REASON: 北京银行需求销户区分预指定销户和正常销户
         * HANDLE：增加1032-预指定销户
         */
        sprintf(sSql,"select bank_id,secu_no,secu_branch,hs_date,hs_time,serial_no,secu_serial,client_account,secu_account,client_name,trans_code,business_flag,money_kind,occur_balance,secu_use_account from tbaccountedit"); 
        sprintf(sSql,"%s where secu_no='%s'", sSql, psSecuNo);
        if (strlen(alltrim(psBusinType)) > 0)
            sprintf(sSql,"%s and busin_type='%c'", sSql, psBusinType[0]);
        sprintf(sSql,"%s and business_flag in (1001,1002,1004,1019,1022,1032)", sSql);
    }
    /* LABEL : 20090819-01-增加国泰格式预约开户和联名卡文件
     * MODIFY: huanghl@hundsun.com
     * REASON: 新增预约开户和联名卡文件生成
     * HANDLE: 增加国泰格式预约开户和联名卡文件生成
     */
    else if (iFlag == 5)
    {
        sprintf(sSql,"select distinct a.bank_id,a.secu_use_account,a.secu_no,a.secu_branch_b,a.secu_branch_s,a.secu_account,a.client_name,a.id_kind,a.id_code,a.money_kind,a.status,a.busin_type from tbpreacc a,tbaccountedit b"); 
        sprintf(sSql,"%s where a.secu_no='%s'", sSql, psSecuNo);
        sprintf(sSql,"%s and a.secu_no=b.secu_no and a.secu_account=b.secu_account and a.busin_type=b.busin_type and a.money_kind=b.money_kind and a.fund_account=b.fund_account", sSql);
        /*20130916-01-重新赋值宏定义为整数型的business_flag字段 %ld->%d */
        sprintf(sSql,"%s and ((a.active_date=%ld and b.business_flag in (%d,%d)) or (a.pre_date=%ld and b.business_flag=%d))", 
                      sSql, pub->lTransDate, FLAG_KHKH, FLAG_YZKH, pub->lTransDate, FLAG_YDJH);
        if (strlen(alltrim(psBusinType)) > 0)
            sprintf(sSql,"%s and a.busin_type='%c'", sSql, psBusinType[0]);
    }
    else if (iFlag == 6)
    {
        memset(&param, 0x00, sizeof(TBPARAM));
        memset(sRuleFlag, 0x00, sizeof(sRuleFlag));
        /*CHKGET_ACCTYPE_RUAL联名卡规则参数：若指定券商未配此参数以系统联名卡规则为准,规则为空则不生成文件*/
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
        
        /*解析账户类型的设置*/
        /* LABEL : 20090901-01-增加变量cardparse初始化
         * MODIFY: wanggk@hundsun.com
         * REASON: 变量cardparse未初始化
         * HANDLE: 增加变量cardparse初始化
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

            /*写文件*/
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
                /*20080522-01-带卡预指定客户在证券方为正常客户*/
                /*带卡号并且强制预指定状态，则送券商为正常状态*/
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
            /* LABEL : 20091123-01-初始化sTransCode变量
             * MODIFY: huayl@hundsun.com
             * REASON: 未初始化变量，生成文件有乱码
             * HANDLE: 初始化变量
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
    /* LABEL : 20090819-01-增加国泰格式预约开户和联名卡文件
     * MODIFY: huanghl@hundsun.com
     * REASON: 新增预约开户和联名卡文件生成
     * HANDLE: 增加国泰格式预约开户和联名卡文件生成
     */
    else if (iFlag == 5)
    {
        lSerialNo = 0;
        for(lCnt = 0; lCnt < lCount; lCnt ++)    
        {
            lZDBranch = 0;/*预约开户时指定的券商营业部*/
            lJHBranch = 0;/*券商发激活时使用的券商营业部*/
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
                    /*当开户指定的营业部和激活使用的营业部均与券商内部营业部编号匹配时退出*/
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
            
            /*正常状态*/
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

            /*使用当日第一条tbaccountedit记录的卡号为当日旧卡号*/
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
            /*当日开户则默认旧卡类型为普通卡0*/
            if ((strcmp(business_flag,"1001") == 0) ||(strcmp(business_flag,"1019") == 0))
            {
                lOld = 0;
            }
            else
            {
                /*检查旧卡类型,0-普通卡,1-联名卡,-1-查询失败*/
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
            /*lType表示新旧卡号的类型差*/
            lType=lNew-lOld;
            DBFreePArray2(&psResult2, lnum);
            /*0-类型未发生改变,不记录*/
            if (lType == 0)
            {
                continue;
            }
            /*1-普通卡换为联名卡或新开联名卡*/
            else if (lType == 1)
            {
                typedetail99901acc_type[0]='1';/*1-联名卡*/
                typedetail99901serial_no = ++lSerialNo;
            }
            /*-1-联名卡换为普通卡*/
            else if (lType == -1)
            {
                typedetail99901acc_type[0]='0';/*0-普通卡*/
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
 函数名称:  Get1ViaBKind
 函数编号:  
 单元文件:  99901c
 函数功能:  根据恒生定义的银行编号转换成标准接口定义的银行编号
 函数参数:  psBType  -- ‘0’中行，‘1’工行，‘2’建行，‘3’农行，
                        ‘4’交行，‘5’招商，‘6’浦发，‘7’合行，
                        ‘8’光大，‘9’华夏，‘a’兴业，‘b’民生，
                        ‘c’深发，‘d’中信
 函数返回:  =0:成功  !0:失败
            gtBType -- 标准接口银行编号 
 函数说明:
 修改说明:
*****************************************************/
long Get1ViaBKind(char *psBType, char *gtBType)
{
    switch(psBType[0])
    {
        case '3':   /*中国农业银行*/
            strcpy(gtBType, "1031000");
            break;
        case '4':   /*交通银行*/
            strcpy(gtBType, "3012900");
            break;
        case '6':   /*上海浦东发展银行*/
            strcpy(gtBType, "3102900");
            break;
        case '7':   /*上海银行*/
            strcpy(gtBType, "3132900");
            break;
        case '8':   /*中国光大银行*/
            strcpy(gtBType, "3031000");
            break;
        case 'a':   /*兴业银行*/
            strcpy(gtBType, "3093910");
            break;
        case 'b':   /*中国民生银行*/
            strcpy(gtBType, "3051000");
            break;
        case 'c':   /*徽商银行*/
            strcpy(gtBType, "");
            break;
        case 'h':   /*北京银行*//*20110819-01-增加北京银行3131000*/
            strcpy(gtBType, "3131000");
            break;
        case 'j':   /*宁波银行*/
            strcpy(gtBType, "");
            break;
        case 'k':   /*绍兴银行*/
            strcpy(gtBType, "");
            break;
        case 'l':   /*青岛银行*/
            strcpy(gtBType, "");
            break;
        case 'n':   /*南京银行*/
            strcpy(gtBType, "");
            break;
        default:
            strcpy(gtBType, psBType);
            break;
    }        
    
    return 0;
}


/*****************************************************
 函数名称:  Get2ViaBKind
 函数编号:  
 单元文件:  99901c
 函数功能:  根据恒生定义的银行编号转换成国泰定义的银行编号
 函数参数:  psBType  -- ‘0’中行，‘1’工行，‘2’建行，‘3’农行，
                        ‘4’交行，‘5’招商，‘6’浦发，‘7’合行，
                        ‘8’光大，‘9’华夏，‘a’兴业，‘b’民生，
                        ‘c’深发，‘d’中信
 函数返回:  =0:成功  !0:失败
            gtBType -- ‘6005’交行 
 函数说明:
 修改说明:  
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
 函数名称:  Get1TransViaTCode
 函数编号:  
 单元文件:  99901c
 函数功能:  根据恒生定义的银行编号转换成国泰定义的银行编号
 函数参数:  psTCode  -- 
 函数返回:  =0:成功  !0:失败
            gtTSource -- 0：证券端,1：银行端
            gtTCode   -- 101：银行发起指定存管银行交易；
                         103：银行发起变更银行账户交易；
                         104：银行受理存管银行确认业务；
                         201：券商发起指定存管银行交易；
                         202：券商发起撤销存管银行交易；
                         203：券商发起变更银行账户交易；
                         204：存管银行预指定。
 函数说明:
 修改说明:
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
 函数名称:  Get2TransViaTCode
 函数编号:  
 单元文件:  99901c
 函数功能:  根据恒生定义的银行编号转换成国泰定义的银行编号
 函数参数:  psTCode  -- 
 函数返回:  =0:成功  !0:失败
            gtTSource -- 0：证券端,1：银行端
            gtTCode   -- 101：银行发起指定存管银行交易；
                         103：银行发起变更银行账户交易；
                         104：银行受理存管银行确认业务；
                         201：券商发起指定存管银行交易；
                         202：券商发起撤销存管银行交易；
                         203：券商发起变更银行账户交易；
                         204：存管银行预指定。
 函数说明:
 修改说明:
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
 函数名称:  Get1ViaIdKind
 函数编号:  
 单元文件:  99901c
 函数功能:  根据恒生定义的证件类型转换成标准接口定义的证件类型
 函数参数:  psIdType  -- 1身份证,2军官证,3国内护照,4户口本,
                         5学员证,6退休证,7临时身份证,8组织机构代码,
                         9营业执照,A警官证,B解放军士兵证,C回乡证,
                         D外国护照, E港澳台居民身份证,
                         F台湾通行证及其他有效旅行证,G海外客户编号,
                         H解放军文职干部证,I武警文职干部证,
                         J武警士兵证,X其他有效证件,Z重号身份证
 函数返回:  =0:成功  !0:失败
            gtIdType -- 标准接口证件类型
                        10  身份证
                        11  护照
                        12  军官证
                        13  士兵证
                        14  回乡证
                        15  户口本
                        16  营业执照
                        17  组织机构代码证
                        20  其他
 函数说明:
 修改说明:
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
函数名称: SetSecuClearStatus
函数功能: 修改券商清算状态
输入参数∶psSecuNo - 券商号 
          cStatus  －状态
输出参数∶
返 回 值∶=0:成功  <0:失败
说    明∶  
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
函数名称∶TransToMonitor
函数功能∶生成监控结构
输入参数∶pub     -- 交易结构
输出参数∶
返 回 值∶无
******************************************************************************/
static void TransToMonitor(TRANS99901 *pub, TBMONITOR *monitor)
{
    /*20080529-01-日终清算功能统一使用初始化日期*/
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
函数名称∶ParseParam
函数功能∶解析联名卡标志的参数值
          value--联名卡标志的参数值
          cardparse--联名卡规则(格式:622827???00-622827???05,622845???99,)
输出参数∶无
返 回 值∶0-成功
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
            cardparse->cardstr[i].cFlag = '='; /*定位匹配*/
            memcpy(cardparse->cardstr[i].sString_b, sBuffer, sizeof(cardparse->cardstr[i].sString_b)-1);
        }
        else
        {
            cardparse->cardstr[i].cFlag = '<'; /*范围匹配*/
            memcpy(cardparse->cardstr[i].sString_b, GetSubStrViaFlag(sBuffer, 0, '-'), sizeof(cardparse->cardstr[i].sString_b)-1);
            /* LABEL : 20090901-01-笔误修改
             * MODIFY: wanggk@hundsun.com
             * REASON: 
             * HANDLE: sString_b改为sString_e
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
函数名称∶CheckAcctType
函数功能∶检查银行账号是否为联名卡输入参数
          CardNo--待查银行账号
          cardparse--联名卡规则(格式:622827???00-622827???05,622845???99,)
输出参数∶无
返 回 值∶0-普通卡,1-联名卡
******************************************************************************/
static long CheckAcctType(char *sCardNo, struct CARDPARSE *cardparse)
{
    long i, j, k, num, lFlag, lLen;
    char sString_b[32+1][32+1], sString_e[32+1][32+1], sTmp[32+1][32+1];
    
    /*对每一组进行匹配处理*/
    for(i=0; i<cardparse->lNum; i++)
    {
        /*初始值为匹配*/
        lFlag = 1;  

        /*匹配串长度，如空则不匹配*/
        lLen = strlen(alltrim(cardparse->cardstr[i].sString_b));
        if (lLen == 0)
        {
            lFlag = 0;
            break;
        }
        /*如是'='，则相等匹配*/
        if (cardparse->cardstr[i].cFlag == '=')
        {
            for(j=0; j<lLen; j++)
            {
                /*'?'为通配符，不参与匹配*/
                if (cardparse->cardstr[i].sString_b[j] == '?')
                    continue;
                else
                {
                    /*不匹配，则继续下一组*/
                    if(cardparse->cardstr[i].sString_b[j] != sCardNo[j])
                    {
                        lFlag = 0;
                        break;
                    }
                }
            }
        }
        /*如是'<'，则范围匹配*/
        else if(cardparse->cardstr[i].cFlag == '<')
        {
            memset(sString_b, 0x00, sizeof(sString_b));
            memset(sString_e, 0x00, sizeof(sString_e));
            memset(sTmp, 0x00, sizeof(sTmp));
            for(j=0,k=0,num=0; j<lLen; j++)
            {
                /*'?'为通配符，不参与匹配*/
                if (cardparse->cardstr[i].sString_b[j] == '?')
                    continue;
                else
                {
                    /*确认匹配段个数*/
                    if ((j>0) && (cardparse->cardstr[i].sString_b[j-1] == '?'))
                    {
                        /*????*/
                        num++;
                        k=0;
                    }
                    /*获取匹配端字符串*/
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
                    /*转成整数型进行范围匹配*/
                    /* LABEL : 20090901-01-BUG修改
         　　　　　　* MODIFY: wanggk@hundsun.com
         　　　　　　* REASON: atol有长度溢出危险
         　　　　　　* HANDLE: atol改用strcmp
         　　　　　　*/ 
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
        /*如匹配，则停止继续匹配操作*/
        if(lFlag == 1)
            break;
    }
    
    return lFlag;
}

/******************************************************************************
函数名称∶GetNumViaFlag
函数功能∶检查字符串中指定字符的个数
输入参数∶
输出参数∶
返 回 值∶
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
    #define SYSTEM_VER "V2.0.0.17"       /* V后面的字符，若带数据库的，加O-Oracle;I-Informix;S-Sybae
                                           版本号1.1 + 兼容序号（从1开始编号）+发行序号（从1开始编号）*/
    fprintf(stderr, "【存管系统】%-17s版本号 %-17s日期 %s\n", filename, SYSTEM_VER, __DATE__);
    return;
}

/**********************************************文件结束********************************************/
