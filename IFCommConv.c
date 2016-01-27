/******************************************************************************
文件名称∶IFCommConv.c
系统名称∶期货保证金存管系统
模块名称∶接口转换（工行类接口）
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
          V3.0.0.0  徐  磊  20131211-01-调整程序，SecuConvXmlToFix 券商号预先取值
          V3.0.0.1  王锴亮  201402130121-01对20004交易屏蔽CheckXMLMac校验
          V3.0.0.2  王晨波  20150915-01-调整可用余额和当前余额取值
******************************************************************************/
#include <unistd.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <errno.h>
#include <sys/types.h>
#include <stdlib.h>
#include "bss.inc"
#include "HsMacAPI.h"

extern char g_errcode[12];
extern char g_errmsg[256];

static char g_InitMacKey[33];

long UpdateXMLMacAndExport(HXMLTREE xml, char cFlag, char *sCommBuf, long *lCommLen);
long GetXmlMacKey(char *sSecuNo, char *sMacKey);
long CheckXMLMac(HXMLTREE xml, char cFlag);
long BankConvFixToXml(FIX *pfix, char *sCommBuf, long *lCommLen);
long BankConvXmlToFix(FIX *pfix, char *sCommBuf, long *lCommLen);
long SecuConvFixToXml(HXMLTREE xml, char *sCommBuf, long lCommLen);
long SecuConvXmlToFix(HXMLTREE xml, char *sCommBuf, long *lCommLen, char *sSecuNo);
long SecuConvXmlResHead(HXMLTREE xml,HXMLTREE xml_Res);

/******************************************************************************
函数名称∶BankConvFixToXml
函数功能∶银行发起FIX->XML报文转换
输入参数∶*pfix     -- 请求报文FIX信息          
输出参数∶
          sAnsBuf  -- 应答报文          
          lAnsLen  -- 应答报文长度
返 回 值∶
          = 0  转换成功
          < 0  转换失败  
******************************************************************************/
long BankConvFixToXml(FIX *pfix, char *sCommBuf, long *lCommLen)
{   
    char sData[256],sData1[256],sSecuNo[DB_SECUNO_LEN + 1] = {0},sSecuDir[256] = {0};    
    char sTransCode[DB_TRSCDE_LEN + 1] = {0},sFileName[256] = {0};
    struct stat f_stat;
    long lRet;
    HXMLTREE  xml;
    
    PrintLog( LEVEL_DEBUG, LOG_DEBUG_INFO, "%s() Entry", __FUNCTION__);
    /* 创建XML句柄 */
    xml = xml_Create("ROOT");
    if ( xml == FAIL ) 
    {        
        FIXputs(pfix, "ErrorNo", "1063");
        FIXputs(pfix, "ErrorInfo", "初始化XML句柄失败");        
        PrintLog( LEVEL_ERR, LOG_NOTIC_INFO, "1063|初始化XML句柄失败");        
        return -1;
    }
    
    /* 交易代码 */
    FIXgets(pfix, "FunctionId", sTransCode, sizeof(sTransCode));
       
    /* 加入根节点 */
    xml_SetElement(xml,"/MsgText","");         
    /* 消息头 */
    xml_SetElement(xml,"/MsgText/GrpHdr","");
    {
        /* 版本号 */
        xml_SetElement(xml,"/MsgText/GrpHdr/Version","1.0.0");
        
        /* 应用系统类型:8-银期转账 */
        xml_SetElement(xml,"/MsgText/GrpHdr/SysType","8");
        
        /* 功能号 */
        memset(sData, 0x00, sizeof(sData));        
        LookUp(204, sData, sTransCode);         
        xml_SetElement(xml,"/MsgText/GrpHdr/BusCd",sData);
        
        /* 交易发起方:B-银行发起 */
        xml_SetElement(xml,"/MsgText/GrpHdr/TradSrc","B");        
        
        /* 发送机构 */
        /* 发送机构(机构标识) */
        memset(sData, 0x00, sizeof(sData));
        memset(sData1, 0x00, sizeof(sData1));
        FIXgets(pfix, "bOrganId", sData1, sizeof(sData1));
        if ( LookUp(202, sData, sData1) < 0 )
        {
        	xml_Destroy(xml);
            FIXputs(pfix, "ErrorNo", "3054");
            FIXputs(pfix, "ErrorInfo", "此银行暂未开通(可能参数设置错误)");        
            PrintLog( LEVEL_ERR, LOG_NOTIC_INFO, "3054|此银行暂未开通(可能参数设置错误具体见字典[202])");        
            return -1;
        }
        xml_SetElement(xml,"/MsgText/GrpHdr/Sender/InstId",sData);
                                      
        /* 发送机构(分支机构) */
        memset(sData, 0x00, sizeof(sData));
        FIXgets(pfix, "bBranchId", sData, sizeof(sData));
        xml_SetElement(xml,"/MsgText/GrpHdr/Sender/BrchId",sData);
        
        /* 发送机构(网点编号) */
        memset(sData, 0x00, sizeof(sData));
        FIXgets(pfix, "bDeptId", sData, sizeof(sData));                
        xml_SetElement(xml,"/MsgText/GrpHdr/Sender/SubBrchId",sData);
       
                
        /* 接收机构 */
        /* 接收机构(机构标识) */
        memset(sData, 0x00, sizeof(sData));
        FIXgets(pfix, "sOrganId", sData, sizeof(sData));
        xml_SetElement(xml,"/MsgText/GrpHdr/Recver/InstId", sData);
        strncpy(sSecuNo, sData, sizeof(sSecuNo)-1);
                    
        /* 接收机构(分支机构) */
        memset(sData, 0x00, sizeof(sData));
        FIXgets(pfix, "sBranchId", sData, sizeof(sData));
        xml_SetElement(xml,"/MsgText/GrpHdr/Recver/BrchId",sData);
                
        /* 接收机构(分支机构) */
        FIXgets(pfix, "sBranchId", sData, sizeof(sData));
        xml_SetElement(xml,"/MsgText/GrpHdr/Recver/SubBrchId",sData);
                                                
        /* 交易日期 */
        memset(sData, 0x00, sizeof(sData));
        FIXgets(pfix, "Date", sData, 9);
        sprintf(sData,"%08s",sData);
        xml_SetElement(xml,"/MsgText/GrpHdr/Date",sData);                      
    
        /* 交易时间 */
        memset(sData, 0x00, sizeof(sData));
        FIXgets(pfix, "Time", sData, 7);
        sprintf(sData,"%06s",GetSysTime());        
        xml_SetElement(xml,"/MsgText/GrpHdr/Time",sData);               
    }

    /* 银行流水号 */
    memset(sData, 0x00, sizeof(sData));
    FIXgets(pfix, "ExSerial", sData, sizeof(sData));
    xml_SetElement(xml,"/MsgText/BkSeq",sData);          
    
    /* 摘要 */
    memset(sData, 0x00, sizeof(sData));
    FIXgets(pfix, "Summary", sData, sizeof(sData));
    xml_SetElement(xml,"/MsgText/Dgst",sData); 
                            
                           
    switch(atoi(sTransCode))
    {
    case 25714: /* 签约 */
    {     
        /* 客户信息 */
        xml_SetElement(xml,"/MsgText/Cust","");
        {
            /* 客户姓名 */
            memset(sData, 0x00, sizeof(sData));
            FIXgets(pfix, "Name", sData, sizeof(sData));
            xml_SetElement(xml,"/MsgText/Cust/Name",sData);
                                   
            /* 证件类型 */                  
            /*
             10-身份证 11-护照 12-军官证 13-士兵证 14-回乡证 15-户口本 16-营业执照 17-企业法人代码证书代码 20-其他
            */ 
            memset(sData, 0x00, sizeof(sData));
            FIXgets(pfix, "CardType", sData, sizeof(sData));
            LookUp(201, sData, sData);
            xml_SetElement(xml,"/MsgText/Cust/CertType",sData);            
                           
          
            /* 证件号码 */
            memset(sData, 0x00, sizeof(sData));
            FIXgets(pfix, "Card", sData, sizeof(sData));
            xml_SetElement(xml,"/MsgText/Cust/CertId",sData);
                           
        
            /* 客户类型:0-个人 1-机构 */
            memset(sData, 0x00, sizeof(sData));
            FIXgets(pfix, "CardType", sData, sizeof(sData));
            if ( strcmp(sData, "8") == 0 || strcmp(sData, "9") == 0 )
            {
                xml_SetElement(xml,"/MsgText/Cust/Type","INVI");
            }
            else
            {
                xml_SetElement(xml,"/MsgText/Cust/Type","INVE");
            }
                                            
            /* 性别 */
            xml_SetElement(xml,"/MsgText/Cust/Sex","");
            /* 籍贯 */
            memset(sData, 0x00, sizeof(sData));
            FIXgets(pfix, "Nationality", sData, sizeof(sData));
            xml_SetElement(xml,"/MsgText/Cust/Ntnl",sData);
        
            /* 通讯地址  */
            memset(sData, 0x00, sizeof(sData));
            FIXgets(pfix, "Address", sData, sizeof(sData));
            xml_SetElement(xml,"/MsgText/Cust/Addr",sData);
        
            /* 邮政编码  */
            memset(sData, 0x00, sizeof(sData));
            FIXgets(pfix, "PostCode", sData, sizeof(sData));
            xml_SetElement(xml,"/MsgText/Cust/PstCd",sData);
        
            /* 电子邮件  */
            memset(sData, 0x00, sizeof(sData));
            FIXgets(pfix, "Email", sData, sizeof(sData));
            xml_SetElement(xml,"/MsgText/Cust/Email",sData);
        
            /* 传真  */
            memset(sData, 0x00, sizeof(sData));
            FIXgets(pfix, "FaxNo", sData, sizeof(sData));
            xml_SetElement(xml,"/MsgText/Cust/Fax",sData);
        
            /* 手机  */
            memset(sData, 0x00, sizeof(sData));
            FIXgets(pfix, "MobiNo", sData, sizeof(sData));
            xml_SetElement(xml,"/MsgText/Cust/Mobile",sData);
        
            /* 电话  */
            memset(sData, 0x00, sizeof(sData));
            FIXgets(pfix, "TelNo", sData, sizeof(sData));
            xml_SetElement(xml,"/MsgText/Cust/Tel",sData);
            
            /* BP */
            xml_SetElement(xml,"/MsgText/Cust/Bp","");
        }
        
        /* 银行账户 */
        xml_SetElement(xml,"/MsgText/BkAcct","");
        {
            /* 银行帐号 */
            memset(sData, 0x00, sizeof(sData));
            FIXgets(pfix, "bCustAcct", sData, sizeof(sData));
            xml_SetElement(xml,"/MsgText/BkAcct/Id",sData);                                                                
        }
        /* 期货账户 */
        xml_SetElement(xml,"/MsgText/FtAcct","");
        {
            /* 资金帐号 */
            memset(sData, 0x00, sizeof(sData));
            FIXgets(pfix, "sCustAcct", sData, sizeof(sData));
            xml_SetElement(xml,"/MsgText/FtAcct/Id",sData);
                                
            /* 资金帐号密码 */
            xml_SetElement(xml,"/MsgText/FtAcct/Pwd","");
            {
            	/*到时需要根据是否加密方式进行调整*/
            	xml_SetElement(xml,"/MsgText/FtAcct/Pwd/Type","0");
            	xml_SetElement(xml,"/MsgText/FtAcct/Pwd/Enc","00");
            	memset(sData, 0x00, sizeof(sData));
            	FIXgets(pfix, "sCustPwd2", sData, sizeof(sData));
            	xml_SetElement(xml,"/MsgText/FtAcct/Pwd/Pwd",sData); 
            }                         
        }
        
        /* 币种 */    
        memset(sData, 0x00, sizeof(sData));
        FIXgets(pfix, "MoneyType", sData, sizeof(sData));
        LookUp(203, sData, sData);
        xml_SetElement(xml,"/MsgText/Ccy",sData); 
         
        /* 炒汇标志:0-钞 1-汇 */
        xml_SetElement(xml,"/MsgText/CashExCd","");
        
        /* 预约流水号 */
/*        xml_SetElement(xml,"/MsgText/Bookseq",""); */                       
    }
    break;
    case 25715: /* 解约 */
    case 23603: /* 查保证金余额 */        
    {        
        /* 客户信息 */
        xml_SetElement(xml,"/MsgText/Cust","");
        {
            /* 客户姓名 */
            memset(sData, 0x00, sizeof(sData));
            FIXgets(pfix, "Name", sData, sizeof(sData));
            xml_SetElement(xml,"/MsgText/Cust/Name",sData);
                                   
            /* 证件类型 */                  
            /*
             10-身份证 11-护照 12-军官证 13-士兵证 14-回乡证 15-户口本 16-营业执照 17-企业法人代码证书代码 20-其他
            */ 
            memset(sData, 0x00, sizeof(sData));
            FIXgets(pfix, "CardType", sData, sizeof(sData));
            LookUp(201, sData, sData);
            xml_SetElement(xml,"/MsgText/Cust/CertType",sData);            
                           
          
            /* 证件号码 */
            memset(sData, 0x00, sizeof(sData));
            FIXgets(pfix, "Card", sData, sizeof(sData));
            xml_SetElement(xml,"/MsgText/Cust/CertId",sData);                                                                  
        }
        
        /* 银行账户 */
        xml_SetElement(xml,"/MsgText/BkAcct","");
        {
            /* 银行帐号 */
            memset(sData, 0x00, sizeof(sData));
            FIXgets(pfix, "bCustAcct", sData, sizeof(sData));
            xml_SetElement(xml,"/MsgText/BkAcct/Id",sData);                                                                       
        }  

        xml_SetElement(xml,"/MsgText/FtAcct","");
        {
            /* 资金帐号 */
            memset(sData, 0x00, sizeof(sData));
            FIXgets(pfix, "sCustAcct", sData, sizeof(sData));
            xml_SetElement(xml,"/MsgText/FtAcct/Id",sData);
                                
            /* 资金帐号密码 */
            xml_SetElement(xml,"/MsgText/FtAcct/Pwd","");
            {
            	/*到时需要根据是否加密方式进行调整*/
            	xml_SetElement(xml,"/MsgText/FtAcct/Pwd/Type","0");
            	xml_SetElement(xml,"/MsgText/FtAcct/Pwd/Enc","00");
            	memset(sData, 0x00, sizeof(sData));
            	FIXgets(pfix, "sCustPwd2", sData, sizeof(sData));
            	xml_SetElement(xml,"/MsgText/FtAcct/Pwd/Pwd",sData); 
            }                         
        }
                
        /* 币种 */    
        memset(sData, 0x00, sizeof(sData));
        FIXgets(pfix, "MoneyType", sData, sizeof(sData));
        LookUp(203, sData, sData);
        xml_SetElement(xml,"/MsgText/Ccy",sData);                            
    }
    break;
    case 25710: /* 变更银行帐号 */
    {       
        /* 客户信息 */
        xml_SetElement(xml,"/MsgText/Cust","");
        {
            /* 客户姓名 */
            memset(sData, 0x00, sizeof(sData));
            FIXgets(pfix, "Name", sData, sizeof(sData));
            xml_SetElement(xml,"/MsgText/Cust/Name",sData);
                                   
            /* 证件类型 */                  
            /*
             10-身份证 11-护照 12-军官证 13-士兵证 14-回乡证 15-户口本 16-营业执照 17-企业法人代码证书代码 20-其他
            */ 
            memset(sData, 0x00, sizeof(sData));
            FIXgets(pfix, "CardType", sData, sizeof(sData));
            LookUp(201, sData, sData);
            xml_SetElement(xml,"/MsgText/Cust/CertType",sData);            
                           
          
            /* 证件号码 */
            memset(sData, 0x00, sizeof(sData));
            FIXgets(pfix, "Card", sData, sizeof(sData));
            xml_SetElement(xml,"/MsgText/Cust/CertId",sData);                                                                  
        }
        
        /* 银行账户 */
        xml_SetElement(xml,"/MsgText/BkAcct","");
        {
            /* 银行帐号 */
            memset(sData, 0x00, sizeof(sData));
            FIXgets(pfix, "bCustAcct", sData, sizeof(sData));
            xml_SetElement(xml,"/MsgText/BkAcct/Id",sData);                                                                                                            
        }
        
        /* 新银行账户 */
        xml_SetElement(xml,"/MsgText/NewBkAcct","");
        {
            /* 银行帐号 */
            memset(sData, 0x00, sizeof(sData));
            FIXgets(pfix, "bNewAcct", sData, sizeof(sData));
            xml_SetElement(xml,"/MsgText/NewBkAcct/Id",sData);                                                                                                        
        }
        /* 期货账户 */
        xml_SetElement(xml,"/MsgText/FtAcct","");
        {
            /* 资金帐号 */
            memset(sData, 0x00, sizeof(sData));
            FIXgets(pfix, "sCustAcct", sData, sizeof(sData));
            xml_SetElement(xml,"/MsgText/FtAcct/Id",sData);
                                                                                   
            /* 资金帐号密码 */
            xml_SetElement(xml,"/MsgText/FtAcct/Pwd","");
            {
            	/*到时需要根据是否加密方式进行调整*/
            	xml_SetElement(xml,"/MsgText/FtAcct/Pwd/Type","0");
            	xml_SetElement(xml,"/MsgText/FtAcct/Pwd/Enc","00");
            	memset(sData, 0x00, sizeof(sData));
            	FIXgets(pfix, "sCustPwd2", sData, sizeof(sData));
            	xml_SetElement(xml,"/MsgText/FtAcct/Pwd/Pwd",sData); 
            } 
        }
        
        /* 币种 */    
        memset(sData, 0x00, sizeof(sData));
        FIXgets(pfix, "MoneyType", sData, sizeof(sData));
        LookUp(203, sData, sData);
        xml_SetElement(xml,"/MsgText/Ccy",sData); 
                      
        /* 炒汇标志:0-钞 1-汇 */
        xml_SetElement(xml,"/MsgText/CashExCd","");                
        
    }
    break;
    
    case 23701: /* 银行转出 */
    case 23702: /* 银行转入 */  
    case 23703: /* 冲银行转出 */ 
    case 23704: /* 冲银行转入 */
    {   
        /* 是否重发标志(是否重发Y-是,N-否) */
        memset(sData, 0x00, sizeof(sData));        
        xml_SetElement(xml,"/MsgText/Resend", "N"); 
        
        /* 冲正交易需要填写原交易流水 */
        if ( strcmp(sTransCode, "23703") == 0 ||  strcmp(sTransCode, "23704") == 0 )
        {
            /* 原交易流水号 */
            memset(sData, 0x00, sizeof(sData));
            FIXgets(pfix, "CorrectSerial", sData, sizeof(sData));
            xml_SetElement(xml,"/MsgText/OBkSeq",sData);
        }
        
        /* 客户信息 */
        xml_SetElement(xml,"/MsgText/Cust","");
        {
            /* 客户姓名 */
            memset(sData, 0x00, sizeof(sData));
            FIXgets(pfix, "Name", sData, sizeof(sData));
            xml_SetElement(xml,"/MsgText/Cust/Name",sData);
                                   
            /* 证件类型 */                  
            /*
             10-身份证 11-护照 12-军官证 13-士兵证 14-回乡证 15-户口本 16-营业执照 17-企业法人代码证书代码 20-其他
            */ 
            memset(sData, 0x00, sizeof(sData));
            FIXgets(pfix, "CardType", sData, sizeof(sData));
            LookUp(201, sData, sData);
            xml_SetElement(xml,"/MsgText/Cust/CertType",sData);            
                           
          
            /* 证件号码 */
            memset(sData, 0x00, sizeof(sData));
            FIXgets(pfix, "Card", sData, sizeof(sData));
            xml_SetElement(xml,"/MsgText/Cust/CertId",sData);                                                                  
        }
        
        /* 银行账户 */
        xml_SetElement(xml,"/MsgText/BkAcct","");
        {
            /* 银行帐号 */
            memset(sData, 0x00, sizeof(sData));
            FIXgets(pfix, "bCustAcct", sData, sizeof(sData));
            xml_SetElement(xml,"/MsgText/BkAcct/Id",sData);                                                                                                            
        }
                        
        /* 期货账户 */
        xml_SetElement(xml,"/MsgText/FtAcct","");
        {
            /* 资金帐号 */
            memset(sData, 0x00, sizeof(sData));
            FIXgets(pfix, "sCustAcct", sData, sizeof(sData));
            xml_SetElement(xml,"/MsgText/FtAcct/Id",sData);   
            
            /* 资金帐号密码 */
            xml_SetElement(xml,"/MsgText/FtAcct/Pwd","");
            {
            	/*到时需要根据是否加密方式进行调整*/
            	xml_SetElement(xml,"/MsgText/FtAcct/Pwd/Type","0");
            	xml_SetElement(xml,"/MsgText/FtAcct/Pwd/Enc","00");
            	memset(sData, 0x00, sizeof(sData));
            	FIXgets(pfix, "sCustPwd2", sData, sizeof(sData));
            	xml_SetElement(xml,"/MsgText/FtAcct/Pwd/Pwd",sData); 
            }                                                                    
        }
        
        /* 转账金额 */
        xml_SetElement(xml,"/MsgText/TrfAmt","");
        {
            /* 金额 */
            memset(sData, 0x00, sizeof(sData));
            FIXgets(pfix, "OccurBala", sData, sizeof(sData));
            snprintf(sData, sizeof(sData), "%.0lf", atof(sData) * 100.00); /* 金额无小数 */
            xml_SetElement(xml,"/MsgText/TrfAmt",sData);

            xml_SetElement(xml,"/MsgText/CustCharge","0");
            xml_SetElement(xml,"/MsgText/FutureCharge","0");
                           
        
            /* 币种（转账币种特殊） */
            memset(sData, 0x00, sizeof(sData));
            FIXgets(pfix, "MoneyType", sData, sizeof(sData));
            LookUp(203, sData, sData);
            xml_SetElementAttr(xml,"/MsgText/TrfAmt","ccy", sData);            
            xml_SetElementAttr(xml,"/MsgText/CustCharge","ccy", sData);            
            xml_SetElementAttr(xml,"/MsgText/FutureCharge","ccy", sData);            
        }                                           
    }
    break;
    
    case 23904: /* 文件通知(单个文件) */
    case 23903: /* 文件通知 */
    {
    	/*结算文件信息1-对账明细文件*/
        xml_AddElement(xml,"/MsgText/FileInfo","");
        {
            /* 文件业务功能 */
            memset(sData, 0x00, sizeof(sData));
            xml_AddElement(xml,"/MsgText/FileInfo/BusCode","0001"); 
            
            /* 文件业务日期 */  
            memset(sData, 0x00, sizeof(sData));
            FIXgets(pfix, "Date", sData, 9);
        	sprintf(sData,"%08s",sData);
            xml_AddElement(xml,"/MsgText/FileInfo/BusDate",sData);  

            /* 文件存放主机 */  
            memset(sData, 0x00, sizeof(sData));
            INIscanf(sSecuNo, "FILEIP", "%s", sData);
            xml_AddElement(xml,"/MsgText/FileInfo/Host",sData);
            
            /* 文件名称 */ 
        	memset(sData1, 0x00, sizeof(sData1));
        	FIXgets(pfix, "Date", sData1, 9);
        	sprintf(sData1,"%08s",sData1);             
            memset(sData, 0x00, sizeof(sData));            
            sprintf(sData, "B_DTL01_%08s.gz", sData1);            
            xml_AddElement(xml,"/MsgText/FileInfo/FileName",sData);
            memset(sFileName, 0x00, sizeof(sFileName));
            strncpy(sFileName, sData, sizeof(sFileName)-1);    
            
            /* 文件长度 */ 
    		SetIniName("rzcl.ini");
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
    		SetIniName("future.ini"); 
            memset(sData, 0x00, sizeof(sData)); 
			sprintf(sData,"%s/%s/%s%s", sSecuDir, sSecuNo, SENDFILE_DIR, sFileName);
			stat(sData, &f_stat);
			memset(sData, 0x00, sizeof(sData)); 
			sprintf(sData, "%ld", f_stat.st_size);
            xml_AddElement(xml,"/MsgText/FileInfo/FileLen",sData);
            
            /*文件时间*/
			memset(sData, 0x00, sizeof(sData)); 
            xml_AddElement(xml,"/MsgText/FileInfo/FileTime","");            

            /*文件校验码*/
/*			memset(sData, 0x00, sizeof(sData)); 
            xml_AddElement(xml,"/MsgText/FileInfo/FileMac","");
*/            			    		                   
    	} 
    	/*结算文件信息2-对账汇总文件*/
        xml_AddElement(xml,"/MsgText/FileInfo","");
        {
            /* 文件业务功能 */
            memset(sData, 0x00, sizeof(sData));
            xml_AddElement(xml,"/MsgText/FileInfo/BusCode","0002"); 
            
            /* 文件业务日期 */  
            memset(sData, 0x00, sizeof(sData));
            FIXgets(pfix, "Date", sData, 9);
        	sprintf(sData,"%08s",sData);
            xml_AddElement(xml,"/MsgText/FileInfo/BusDate",sData);  

            /* 文件存放主机 */  
            memset(sData, 0x00, sizeof(sData));
            INIscanf(sSecuNo, "FILEIP", "%s", sData);
            xml_AddElement(xml,"/MsgText/FileInfo/Host",sData);
            
            /* 文件名称 */ 
        	memset(sData1, 0x00, sizeof(sData1));
        	FIXgets(pfix, "Date", sData1, 9);
        	sprintf(sData1,"%08s",sData1);             
            memset(sData, 0x00, sizeof(sData));            
            sprintf(sData, "B_DTL02_%08s.gz", sData1);            
            xml_AddElement(xml,"/MsgText/FileInfo/FileName",sData);
            memset(sFileName, 0x00, sizeof(sFileName));
            strncpy(sFileName, sData, sizeof(sFileName)-1);    
            
            /* 文件长度 */ 
    		SetIniName("rzcl.ini");
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
    		SetIniName("future.ini"); 
            memset(sData, 0x00, sizeof(sData)); 
			sprintf(sData,"%s/%s/%s%s", sSecuDir, sSecuNo, SENDFILE_DIR, sFileName);
			stat(sData, &f_stat);
			memset(sData, 0x00, sizeof(sData)); 
			sprintf(sData, "%ld", f_stat.st_size);
            xml_AddElement(xml,"/MsgText/FileInfo/FileLen",sData);
            
            /*文件时间*/
			memset(sData, 0x00, sizeof(sData)); 
            xml_AddElement(xml,"/MsgText/FileInfo/FileTime","");            

            /*文件校验码*/
/*			memset(sData, 0x00, sizeof(sData)); 
            xml_AddElement(xml,"/MsgText/FileInfo/FileMac","");
*/            			    		                   
    	}    		
    }
    break;
    default:
        FIXputs(pfix, "ErrorNo", "2033");
        FIXputs(pfix, "ErrorInfo", "该交易不支持");        
        PrintLog( LEVEL_ERR, LOG_NOTIC_INFO, "2033|该交易不支持(转换),[%s]", sTransCode);        
        return -2;        
    }
    
    /* 导出通讯包 */
    lRet = UpdateXMLMacAndExport(xml, 'R', sCommBuf, lCommLen);
    if ( lRet < 0 )
    {
        FIXputs(pfix, "ErrorNo", "2041");
        FIXputs(pfix, "ErrorInfo", "导出通讯请求包失败");        
        PrintLog( LEVEL_ERR, LOG_NOTIC_INFO, "2041|导出通讯请求包失败(转换)");
        return -3;
    }
    xml_Destroy(xml);
    PrintLog( LEVEL_DEBUG, LOG_DEBUG_INFO, "%s() Exit", __FUNCTION__);
    return 0;
}


/******************************************************************************
函数名称∶BankConvXmlToFix
函数功能∶银行发起XML->FIX报文转换
输入参数∶
        sCommBuf  -- 通讯XML报文
        lCommLen  -- 通讯报文长度
输出参数∶
         *pfix     -- FIX
返 回 值∶
          = 0  成功
          < 0  失败
******************************************************************************/
long BankConvXmlToFix(FIX *pfix, char *sCommBuf, long *lCommLen)
{
    char sData[256];    
    char sTransCode[DB_TRSCDE_LEN + 1] = {0};
    char sErrCode[12] = {0};
    long lRet;
    HXMLTREE  xml;
    
    /* 交易代码 */
    FIXgets(pfix, "FunctionId", sTransCode, sizeof(sTransCode));
    
    /* 创建XML句柄 */
    xml = xml_Create("ROOT");
    if ( xml ==  FAIL )  
    {        
        PrintLog(LEVEL_ERR, LOG_NOTIC_INFO, "转XML应答包时创建XML句柄失败[%s]",(char *)xml_LastStringError());
        ERRLOGF("转XML应答包时创建XML句柄失败[%s]", (char *)xml_LastStringError());
        FIXputs(pfix, "ErrorNo", "1063");
        FIXputs(pfix, "ErrorInfo", "初始化XML句柄失败");         
        return -1;
    }
            
    /* 导入XML */
    lRet = xml_ImportXMLString(xml,sCommBuf, "", 0);
    if ( lRet == FAIL ) 
    {              
        PrintLog(LEVEL_ERR, LOG_NOTIC_INFO, "转XML应答包时导入XML失败[%s]",(char *)xml_LastStringError());
        ERRLOGF("转XML应答包时导入XML失败[%s]", (char *)xml_LastStringError());
        FIXputs(pfix, "ErrorNo", "1063");
        FIXputs(pfix, "ErrorInfo", "转XML应答包时导入XML失败");         
        xml_Destroy(xml);
        return -2;
    }

    /*返回信息*/
    memset(sData, 0x00, sizeof(sData));
    xml_GetElement(xml,"/MsgText/Rst/Info",sData,sizeof(sData));
    FIXputs(pfix,"ErrorInfo",sData);
           
    /*不成功的话，直接返回，不去校验MAC*/    
    /*返回码*/
    memset(sErrCode, 0x00, sizeof(sErrCode));
    xml_GetElement(xml,"/MsgText/Rst/Code",sErrCode,sizeof(sErrCode));    
    if ( strcmp(sErrCode, "0000") != 0 )
    {
        LookUp(105, sErrCode, sErrCode);
        FIXputs(pfix,"ErrorNo",sErrCode);
        xml_Destroy(xml);
        return 0;        
    } 
    else
    {
        FIXputs(pfix,"ErrorNo","0000");
    }                          
        
    /* MAC检查 */
    if ( CheckXMLMac(xml, 'A') < 0 )
    {
        FIXputs(pfix, "ErrorNo", "1063");
        FIXputs(pfix, "ErrorInfo", "MAC校验错误");      	
    	xml_Destroy(xml);
        return -3;
    }
        
    /* 流水号  */
    memset(sData, 0x00, sizeof(sData));
    xml_GetElement(xml,"/MsgText/BkSeq",sData,sizeof(sData));
    FIXputs(pfix,"ExSerial", sData);
    
    /*期货流水号*/ 
    memset(sData, 0x00, sizeof(sData));
    xml_GetElement(xml,"/MsgText/FtSeq",sData,sizeof(sData));
    FIXputs(pfix,"sSerial",sData);         
                 
    
    /* 交易成功，则继续取应答信息 */
    if ( memcmp(sErrCode,"0000", 4) == 0) 
    {
        /* 资金帐号 */
        memset(sData, 0x00,sizeof(sData));
        xml_GetElement(xml,"/MsgText/FtAcct/Id",sData, sizeof(sData));
        FIXputs(pfix,"sCustAcct", sData);
        
        /* 营业部编码 */
        memset(sData, 0x00,sizeof(sData));
        xml_GetElement(xml,"/MsgText/GrpHdr/Recver/BrchId",sData, sizeof(sData));
        FIXputs(pfix,"sBranchId", sData);
                                                                                               
        /* 查余额则返回余额信息 */
        if ( strcmp(sTransCode, "23603") == 0 )
        {
            /* 当前余额 */
            memset(sData, 0x00, sizeof(sData));
            xml_GetElement(xml,"/MsgText/FtBal/CurBal",sData, sizeof(sData));
            FIXprintf(pfix, "OccurBala", "%.2lf", (atof(sData) /100.00) );
            
            /* 可用余额 */
            memset(sData, 0x00, sizeof(sData));
            xml_GetElement(xml,"/MsgText/FtBal/UseBal",sData, sizeof(sData));
            FIXprintf(pfix, "sEnableBala", "%.2lf", (atof(sData) /100.00) );
            
            /* 可取余额 */
            memset(sData, 0x00, sizeof(sData));
            xml_GetElement(xml,"/MsgText/FtBal/FtcBal",sData, sizeof(sData));
            FIXprintf(pfix, "sFetchBala", "%.2lf", (atof(sData) /100.00) );                        
        }
    }
    xml_Destroy(xml);
    return 0;
}


/******************************************************************************
函数名称∶SecuConvXmlToFix
函数功能∶服务商发起XML->FIX报文转换
输入参数∶
         XML -- XML句柄
输出参数∶
         sCommBuf -- FIX报文
         lCommLen -- FIX报文长度
         sSecuNo  -- 服务商编号
返 回 值∶
          = 0  成功
          < 0  失败  
******************************************************************************/
long SecuConvXmlToFix(HXMLTREE xml,  char *sCommBuf, long *lCommLen, char *sSecuNo)
{
    FIX *pfix;    
    char sData[256],sData1[256];    
    char sTransCode[DB_TRSCDE_LEN + 1] = {0};
    long lRet;
    char sErrMsg[256] = {0};
    char bOrganId[FLD_BANKNO_LEN + 1] = {0};
    char sSecuBranch[DB_SBRNCH_LEN + 1] = {0};
    char Date[9] = {0};
    char Time[7] = {0};
    
    /*20131211-01-调整程序，SecuConvXmlToFix 券商号预先取值*/
    /* 服务商编号 */
    memset(sData, 0x00, sizeof(sData));    
    if ( xml_GetElement(xml, "/MsgText/GrpHdr/Sender/InstId",sSecuNo, DB_SECUNO_LEN + 1) < 0 )
    {
        PrintLog(LEVEL_ERR, LOG_NOTIC_INFO, "获取XML节点[/MsgText/GrpHdr/Sender/InstId]失败，[%s]",  (char *)xml_LastStringError());        
        strncpy(g_errcode, "1063", sizeof(g_errcode)-1);
        strncpy(g_errmsg, "获取XML节点[/MsgText/GrpHdr/Sender/InstId]失败", sizeof(g_errmsg)-1);    	        
        return -1;
    }
            
    /* 交易代码 */
    memset(sData, 0x00, sizeof(sData));
    if ( xml_GetElement(xml, "/MsgText/GrpHdr/BusCd",sTransCode, sizeof(sTransCode)) < 0 )
    {
        PrintLog(LEVEL_ERR, LOG_NOTIC_INFO, "获取XML节点[/MsgText/GrpHdr/BusCd]失败，[%s]",  (char *)xml_LastStringError());        
        strncpy(g_errcode, "1063", sizeof(g_errcode)-1);
        strncpy(g_errmsg, "获取XML节点[/MsgText/GrpHdr/BusCd]失败", sizeof(g_errmsg)-1);    	        
        return -1;
    }
            
            
    /* MAC校验 */
    /* 201402130121-王锴亮-对20004交易屏蔽CheckXMLMac校验 */
    /*  根据交易代码来特殊处理 */
    if ((atoi(sTransCode)) != 20004)
    {
        if ( CheckXMLMac(xml,  'R') < 0 )
        {
            strncpy(g_errcode, "1042", sizeof(g_errcode)-1);
            strncpy(g_errmsg, "MAC校验错误", sizeof(g_errmsg)-1);
            PrintLog(LEVEL_ERR, LOG_NOTIC_INFO, "MAC校验错误");         
            ERRLOGF("MAC校验错误");              
            return -1; 
        }
    }     
    
    pfix = FIXopen(sCommBuf, 0, 2048, sErrMsg);
    if ( pfix == NULL) 
    {
        strncpy(g_errcode, "1041", sizeof(g_errcode)-1);
        strncpy(g_errmsg, "银行主机系统错误(创建FIX失败)", sizeof(g_errmsg)-1);    	        
        PrintLog(LEVEL_ERR, LOG_NOTIC_INFO, "创建FIX失败[%s]", sErrMsg);        
        return -1;
    }
    
    LookUp(104, sData, sTransCode);
    FIXputs(pfix, "FunctionId", sData);
    PrintLog(LEVEL_ERR, LOG_NOTIC_INFO, "交易码[%s],[%s]", sData,sTransCode);
    /* 发起方 */
    memset(sData, 0x00, sizeof(sData));
    xml_GetElement(xml, "/MsgText/GrpHdr/TradSrc",sData, sizeof(sData));
    if ( strcmp(sData, "F") != 0 )
    {
        FIXclose(pfix);
        strncpy(g_errcode, "1044", sizeof(g_errcode)-1);
        strncpy(g_errmsg, "通讯消息体格式[TradSrc]填写错误", sizeof(g_errmsg)-1);    	        
        PrintLog(LEVEL_ERR, LOG_NOTIC_INFO, "通讯消息体格式[/MsgText/GrpHdr/TradSrc]填写错误[%s]", sData); 
        return -1;
    }
/*    FIXprintf(pfix, "bOrganId", "%.1s", sData);*/
    
    /* 服务商流水号 */
    memset(sData, 0x00, sizeof(sData));
    xml_GetElement(xml, "/MsgText/FtSeq",sData, sizeof(sData));
    FIXputs(pfix, "ExSerial", sData);
    
    /* 银行类型 */
    memset(sData, 0x00, sizeof(sData));
    memset(sData1, 0x00, sizeof(sData1));
    xml_GetElement(xml, "/MsgText/GrpHdr/Recver/InstId",sData1, sizeof(sData1));
    if ( LookUp(102, sData, sData1) < 0 )
    {
        FIXclose(pfix);
        strncpy(g_errcode, "1038", sizeof(g_errcode)-1);
        strncpy(g_errmsg, "无效银行编号", sizeof(g_errmsg)-1);    	        
        PrintLog(LEVEL_ERR, LOG_NOTIC_INFO, "无效银行编号[%s],[%s]", sData1,sData); 
        return -1;
    }
/*    FIXprintf(pfix, "bOrganId", "%.1s", sData);*/
	strncpy(bOrganId, sData, FLD_BANKNO_LEN);
         
/*    FIXputs(pfix, "sOrganId", sSecuNo);*/
    
    /* 服务商营业部编号 */
    memset(sData, 0x00, sizeof(sData));
    xml_GetElement(xml, "/MsgText/GrpHdr/Sender/BrchId",sData, sizeof(sData));
    strncpy(sSecuBranch, sData, DB_SBRNCH_LEN);
/*    FIXputs(pfix, "sBranchId", sData);*/
    
    /* 交易日期 */
    memset(sData, 0x00, sizeof(sData));
    xml_GetElement(xml, "/MsgText/GrpHdr/Date",sData, sizeof(sData));
/*    FIXputs(pfix, "Date", sData);*/
	strncpy(Date, sData, 8);
     
    /* 交易时间 */
    memset(sData, 0x00, sizeof(sData));
    xml_GetElement(xml, "/MsgText/GrpHdr/Time",sData, sizeof(sData));
/*    FIXputs(pfix, "Time", sData);*/
	strncpy(Time, sData, 6);
         
    /* 摘要 */
    memset(sData, 0x00, sizeof(sData));
    xml_GetElement(xml, "/MsgText/Dgst",sData, sizeof(sData));
/*    FIXputs(pfix, "Dgst", sData);*/
    
    /* 业务类型(直接填写0) */    
/*    FIXputs(pfix, "BusinType", "0");*/
        
    /*  根据交易代码来特殊处理 */
    switch (atoi(sTransCode) )
    {
    case 20001: /* 签到 */
    case 20002: /* 签退 */
        /* 无须新增其它字段 */
        /*  币种  */
/*        memset(sData, 0x00, sizeof(sData));
        xml_GetElement(xml, "/MsgText/Ccy",sData, sizeof(sData));
        LookUp(103, sData, sData);
        FIXprintf(pfix, "MoneyType", "%.1s", sData);
*/
		FIXputs(pfix, "bOrganId", bOrganId);        
		FIXputs(pfix, "sOrganId", sSecuNo);        
		FIXputs(pfix, "sBranchId", sSecuBranch);        
		FIXputs(pfix, "Date", Date);        
		FIXputs(pfix, "Time", Time);        
        break;
    case 20004: /* 密钥同步 */
        {
            /* 本地处理，直接关闭FIX句柄 */
            FIXclose(pfix);
            memset(sData, 0x00, sizeof(sData));
            if ( xml_GetElement(xml, "/MsgText/MacKey",sData, sizeof(sData)) < 0 )
            {
                PrintLog(LEVEL_ERR, LOG_NOTIC_INFO, "获取XML节点[MacKey]错误[%s]", (char *)xml_LastStringError());      
        		strncpy(g_errcode, "1062", sizeof(g_errcode)-1);
        		strncpy(g_errmsg, "获取XML节点[MacKey]错误", sizeof(g_errmsg)-1);    	        
                return -1;
            }
            else
            {
            	memset(g_InitMacKey, 0x00, sizeof(g_InitMacKey));
    			if ( INIscanf(sSecuNo, "MAC_KEY", "%s", g_InitMacKey) < 0 )
    			{
    			    PrintLog(LEVEL_ERR, LOG_NOTIC_INFO, "取MACKEY错误，[%s][MAC_KEY],[%s]", sSecuNo, (char *)strerror(errno));
    			    ERRLOGF("取交换密钥错误，[%s][MAC_KEY],[%s]", sSecuNo, (char *)strerror(errno));
    			    return -1;   
    			}            	
                /* 保存MAC密钥 */
                if ( ModifyCfgItem(sSecuNo, "MAC_KEY", sData) < 0 )
                {
                    PrintLog(LEVEL_ERR, LOG_NOTIC_INFO, "保存密钥[MacKey]错误[%s]", (char *)strerror(errno));
        			strncpy(g_errcode, "1062", sizeof(g_errcode)-1);
        			strncpy(g_errmsg, "保存XML密钥[MacKey]错误", sizeof(g_errmsg)-1);    	        
                }
                else
                {
        			strncpy(g_errcode, "0000", sizeof(g_errcode)-1);
        			strncpy(g_errmsg, "重置密钥成功", sizeof(g_errmsg)-1);    	        
                }
                return -1;
            }
        }
        break;
    case 22003: /* 冲银行转出 */
    case 22004: /* 冲银行转入 */        
    case 22001: /* 银行转出 */
    case 22002: /* 银行转入 */             
    case 21005: /* 查银行余额 */
        {
        	/*银行编号*/
        	FIXputs(pfix, "bOrganId", bOrganId);
        	/*期商编号*/
        	FIXputs(pfix, "sOrganId", sSecuNo);
        	/*期商营业部编号*/
        	FIXputs(pfix, "sBranchId", sSecuBranch); 
            /* 银行密码 */
            memset(sData, 0x00, sizeof(sData));
            xml_GetElement(xml, "/MsgText/BkAcct/Pwd/Pwd",sData, sizeof(sData));
            FIXputs(pfix, "bCustPwd", sData);  
            /*业务摘要*/ 
    		memset(sData, 0x00, sizeof(sData));
    		xml_GetElement(xml, "/MsgText/Dgst",sData, sizeof(sData));            
            FIXputs(pfix, "Summary", sData); 
            /*业务类型*/
            FIXputs(pfix, "BusinType", "0");
            /*日期*/
			FIXputs(pfix, "Date", Date); 
			/*时间*/       
			FIXputs(pfix, "Time", Time);                 	 
            /* 客户姓名 */
            memset(sData, 0x00, sizeof(sData));
            xml_GetElement(xml, "/MsgText/Cust/Name",sData, sizeof(sData));
            FIXputs(pfix, "Name", sData);
            
            /* 证件类型 */
            memset(sData, 0x00, sizeof(sData));
            xml_GetElement(xml, "/MsgText/Cust/CertType",sData, sizeof(sData));
            /* 查余额不检查证件类型 */
            if ( LookUp(101, sData, sData) < 0  && strcmp(sTransCode, "21005") != 0 )
            {
                FIXclose(pfix);
        		strncpy(g_errcode, "1058", sizeof(g_errcode)-1);
        		strncpy(g_errmsg, "证件类型不支持", sizeof(g_errmsg)-1);    	        
                PrintLog(LEVEL_ERR, LOG_NOTIC_INFO, "证件类型不支持[%s]", sData); 
                return -1;
            }
            FIXputs(pfix, "CardType", sData);
            
            /* 证件号码 */
            memset(sData, 0x00, sizeof(sData));
            xml_GetElement(xml, "/MsgText/Cust/CertId",sData, sizeof(sData));           
            FIXputs(pfix, "Card", sData);
            
            /* 银行帐号 */
            memset(sData, 0x00, sizeof(sData));
            xml_GetElement(xml, "/MsgText/BkAcct/Id",sData, sizeof(sData));
            FIXputs(pfix, "bCustAcct", sData);
            
             /* 资金帐号 */
            memset(sData, 0x00, sizeof(sData));
            xml_GetElement(xml, "/MsgText/FtAcct/Id",sData, sizeof(sData));
            FIXputs(pfix, "sCustAcct", sData);
            
            /* 币种(查余额与转账币种位置不同) */
            memset(sData, 0x00, sizeof(sData));
            memset(sData1, 0x00, sizeof(sData1));
            if ( strcmp(sTransCode, "21005") == 0 )
            {
                xml_GetElement(xml, "/MsgText/Ccy",sData, sizeof(sData));
                /*兼容老版本没有币种字段的情况*/
                Alltrim(sData);
                if (strlen(sData) == 0)
                {
                	strcpy(sData, "RMB");
                }                
            }
            else
            {                
                xml_GetElementAttr(xml, "/MsgText/TrfAmt","ccy", sData, sizeof(sData));                 
            }
            if ( LookUp(103, sData1, sData) < 0 )
            {
                FIXclose(pfix);
        		strncpy(g_errcode, "1059", sizeof(g_errcode)-1);
        		strncpy(g_errmsg, "币种不支持", sizeof(g_errmsg)-1);    	        
                PrintLog(LEVEL_ERR, LOG_NOTIC_INFO, "币种不支持[%s]", sData); 
                return -1;                
            }            
            FIXprintf(pfix, "MoneyType", "%.1s", sData1);
            
            /* 转账交易需要金额字段 */
            if ( strcmp(sTransCode, "21005") != 0 )
            {
                /* 转账金额 */
                memset(sData, 0x00, sizeof(sData));
                xml_GetElement(xml, "/MsgText/TrfAmt",sData, sizeof(sData));                
                FIXprintf(pfix, "OccurBala", "%.2lf", (atof(sData) /100.00) );
            }
            
            /* 冲正交易需要提交原交易流水号 */
            if ( strcmp(sTransCode, "22003") == 0 || strcmp(sTransCode, "22004") == 0 )
            {
                /* 原流水号 */
                memset(sData, 0x00, sizeof(sData));
                xml_GetElement(xml, "/MsgText/OFtSeq",sData, sizeof(sData));
                FIXputs(pfix, "CorrectSerial", sData);
            }                         
        }
        break;
    default:
        FIXclose(pfix);
        strncpy(g_errcode, "1033", sizeof(g_errcode)-1);
        strncpy(g_errmsg, "该交易不支持", sizeof(g_errmsg)-1);    	        
        PrintLog(LEVEL_ERR, LOG_NOTIC_INFO, "该交易不支持[%s]", sTransCode); 
        return -1;         
    }         
    
    PrintFIXLog( LEVEL_NORMAL, "发送总控请求", pfix);
    *lCommLen = FIXlen(pfix);
    FIXclose(pfix);
    return 0;    
}


/******************************************************************************
函数名称∶SecuConvFixToXml
函数功能∶服务商发起FIX->XML报文转换
输入参数∶
         XML -- XML句柄
         sCommBuf -- FIX报文
         lCommLen -- FIX报文长度
输出参数∶
         
         sSecuNo  -- 服务商编号
返 回 值∶
          = 0  成功
          < 0  失败  
******************************************************************************/
long SecuConvFixToXml(HXMLTREE xml, char *sCommBuf, long lCommLen)
{
    FIX *pfix;    
    char sData[256],sData1[256];    
    char sTransCode[DB_TRSCDE_LEN + 1] = {0};
    long lRet;
    char sErrMsg[256] = {0};
    
    pfix = FIXopen(sCommBuf, lCommLen, 2048, sErrMsg);
    if ( pfix == NULL) 
    {        
        xml_SetElement(xml,"/MsgText/Rst/Code","1041");
        xml_SetElement(xml,"/MsgText/Rst/Info","银行主机系统错误(创建FIX失败)");
        PrintLog(LEVEL_ERR, LOG_NOTIC_INFO, "创建FIX失败[%s]", sErrMsg);        
        return -1;
    }
    
    PrintFIXLog( LEVEL_NORMAL, "收到总控应答", pfix);
    
    xml_GetElement(xml,"/MsgText/GrpHdr/BusCd",sTransCode, sizeof(sTransCode));
           
    /* 错误信息 */
    memset(sData, 0x00, sizeof(sData));
    FIXgets(pfix, "ErrorInfo", sData, sizeof(sData));   
    xml_SetElement(xml,"/MsgText/Rst/Info", sData);
    
    /* 银行流水号 */
    memset(sData, 0x00, sizeof(sData));
    FIXgets(pfix, "bSerial", sData, sizeof(sData));   
    xml_SetElement(xml,"/MsgText/BkSeq", sData);
    
    /* 错误代码 */
    memset(sData, 0x00, sizeof(sData));
    memset(sData1, 0x00, sizeof(sData1));
    FIXgets(pfix, "ErrorNo", sData, sizeof(sData));
    if ( strcmp(sData, "0000") != 0 )
    {
    	if(strlen(sData) == 4)
    	{
    		strncpy(sData1,sData,4);
    	}
    	else
    	{
        	LookUp(205, sData1, sData);
        	if(strlen(sData1) == 0)
        	{
        		strcpy(sData1,"9999");
        	}
        }
        xml_SetElement(xml,"/MsgText/Rst/Code", sData1);
    }
    else
    {
    	xml_SetElement(xml,"/MsgText/Rst/Code", sData);
    }
    
    /* 交易成功且查银行余额交易则处理账户余额信息 */
    if ( strcmp( sData, "0000" ) == 0  && strcmp(sTransCode, "21005") == 0)
    {
        /* 可取余额 */
        memset(sData, 0x00, sizeof(sData));
        FIXgets(pfix, "bFetchBala", sData, sizeof(sData));   
        snprintf(sData, sizeof(sData), "%.0lf", atof(sData) * 100.00); /* 金额无小数 */        
        xml_SetElement(xml,"/MsgText/BkBal/FtcBal", sData);
        /* 20150915-01-调整可用余额和当前余额取值 */
        /* 可用余额 */
        xml_SetElement(xml,"/MsgText/BkBal/UseBal", sData);
        
        memset(sData, 0x00, sizeof(sData));
        FIXgets(pfix, "bEnableBala", sData, sizeof(sData));   
        snprintf(sData, sizeof(sData), "%.0lf", atof(sData) * 100.00); /* 金额无小数 */        

        
        /* 当前余额 */         
        xml_SetElement(xml,"/MsgText/BkBal/CurBal", sData);
        
        /* 币种 */
/*        memset(sData, 0x00, sizeof(sData));
        xml_GetElement(xml,"/MsgText/Ccy", sData, sizeof(sData));           
        xml_SetElementAttr(xml,"/MsgText/BkBal/FtcBal","ccy", sData);
        xml_SetElementAttr(xml,"/MsgText/BkBal/CurBal","ccy", sData);
        xml_SetElementAttr(xml,"/MsgText/BkBal/UseBal","ccy", sData);                                 
*/    } 
    
    FIXclose(pfix);
    return 0;        
}

/******************************************************************************
函数名称∶SecuConvXmlResHead
函数功能∶设置服务商发起的应答XML报文头信息
输入参数∶
         XML -- 请求XML句柄
		 HXML_RES -- 应答XML句柄
返 回 值∶
          = 0  成功
          < 0  失败  
******************************************************************************/
long SecuConvXmlResHead(HXMLTREE xml,HXMLTREE xml_Res)
{
	char sData[256];
    char sTransCode[DB_TRSCDE_LEN + 1] = {0};
    /* 版本号 */
    memset(sData, 0x00, sizeof(sData));
    xml_GetElement(xml, "/MsgText/GrpHdr/Version",sData, sizeof(sData));
    xml_SetElement(xml_Res, "/MsgText/GrpHdr/Version",sData);  	

    /* 应用系统类型 */
    memset(sData, 0x00, sizeof(sData));
    xml_GetElement(xml, "/MsgText/GrpHdr/SysType",sData, sizeof(sData));
    xml_SetElement(xml_Res, "/MsgText/GrpHdr/SysType",sData);

    /* 业务功能号 */
    memset(sData, 0x00, sizeof(sData));
    xml_GetElement(xml, "/MsgText/GrpHdr/BusCd",sData, sizeof(sData));
    xml_SetElement(xml_Res, "/MsgText/GrpHdr/BusCd",sData);
	memset(sTransCode, 0x00, sizeof(sTransCode));
	strncpy(sTransCode, sData, DB_TRSCDE_LEN);
    /* 交易发起方 */
    memset(sData, 0x00, sizeof(sData));
    xml_GetElement(xml, "/MsgText/GrpHdr/TradSrc",sData, sizeof(sData));
    xml_SetElement(xml_Res, "/MsgText/GrpHdr/TradSrc",sData);

    /* 发送机构 */
    memset(sData, 0x00, sizeof(sData));
    xml_GetElement(xml, "/MsgText/GrpHdr/Sender/InstId",sData, sizeof(sData));
    xml_SetElement(xml_Res, "/MsgText/GrpHdr/Sender/InstId",sData);

    /* 接收机构 */
    memset(sData, 0x00, sizeof(sData));
    xml_GetElement(xml, "/MsgText/GrpHdr/Recver/InstId",sData, sizeof(sData));
    xml_SetElement(xml_Res, "/MsgText/GrpHdr/Recver/InstId",sData);

    /* 交易日期 */
    memset(sData, 0x00, sizeof(sData));
    xml_GetElement(xml, "/MsgText/GrpHdr/Date",sData, sizeof(sData));
    xml_SetElement(xml_Res, "/MsgText/GrpHdr/Date",sData);

    /* 交易时间 */
    memset(sData, 0x00, sizeof(sData));
    xml_GetElement(xml, "/MsgText/GrpHdr/Time",sData, sizeof(sData));
    xml_SetElement(xml_Res, "/MsgText/GrpHdr/Time",sData);

    xml_SetElement(xml_Res, "/MsgText/Rst/Code","");
    xml_SetElement(xml_Res, "/MsgText/Rst/Info","");
    	    
    switch(atoi(sTransCode))
    {
    case 20001: /* 签到 */
    case 20002: /* 签退 */
    case 20004: /* 密钥同步 */
    case 23001: /* 文件生成通知 */
    	memset(sData, 0x00, sizeof(sData));
    	xml_GetElement(xml, "/MsgText/FtSeq",sData, sizeof(sData));
    	xml_SetElement(xml_Res, "/MsgText/FtSeq",sData);
    	xml_SetElement(xml_Res, "/MsgText/BkSeq","");
    	break;
    case 21005: /*查询银行余额*/    	
    case 22003: /* 冲银行转出 */
    case 22004: /* 冲银行转入 */        
    case 22001: /* 银行转出 */
    case 22002: /* 银行转入 */     	
    	memset(sData, 0x00, sizeof(sData));
    	xml_GetElement(xml, "/MsgText/FtSeq",sData, sizeof(sData));
    	xml_SetElement(xml_Res, "/MsgText/FtSeq",sData);
    	xml_SetElement(xml_Res, "/MsgText/BkSeq","");
/*    	memset(sData, 0x00, sizeof(sData));
    	xml_GetElement(xml, "/MsgText/Ccy",sData, sizeof(sData));
    	xml_SetElement(xml_Res, "/MsgText/Ccy",sData);    	
*/    	memset(sData, 0x00, sizeof(sData));
    	xml_GetElement(xml, "/MsgText/BkAcct/Id",sData, sizeof(sData));
    	xml_SetElement(xml_Res, "/MsgText/BkAcct/Id",sData);
    	memset(sData, 0x00, sizeof(sData));
    	xml_GetElement(xml, "/MsgText/FtAcct/Id",sData, sizeof(sData));
    	xml_SetElement(xml_Res, "/MsgText/FtAcct/Id",sData); 
		if(memcmp(sTransCode, "21005", 5) == 0)
		{
        	xml_SetElement(xml_Res,"/MsgText/BkBal/FtcBal", "");
        	xml_SetElement(xml_Res,"/MsgText/BkBal/UseBal", "");
        	xml_SetElement(xml_Res,"/MsgText/BkBal/CurBal", "");			
	        memset(sData, 0x00, sizeof(sData));
	        xml_GetElement(xml,"/MsgText/Ccy", sData, sizeof(sData)); 
	        Alltrim(sData);
            if (strlen(sData) == 0)
            {
                strcpy(sData, "RMB");
            }          
	        xml_SetElementAttr(xml_Res,"/MsgText/BkBal/FtcBal","ccy", sData);
	        xml_SetElementAttr(xml_Res,"/MsgText/BkBal/CurBal","ccy", sData);
	        xml_SetElementAttr(xml_Res,"/MsgText/BkBal/UseBal","ccy", sData); 			
		}
		else
		{			
    		memset(sData, 0x00, sizeof(sData));
    		xml_GetElement(xml, "/MsgText/TrfAmt",sData, sizeof(sData));
    		xml_SetElement(xml_Res, "/MsgText/TrfAmt",sData);
    		memset(sData, 0x00, sizeof(sData));    	
			xml_GetElementAttr(xml, "/MsgText/TrfAmt","ccy", sData, sizeof(sData));
			xml_SetElementAttr(xml_Res,"/MsgText/TrfAmt","ccy", sData); 		
			
    		memset(sData, 0x00, sizeof(sData));
    		xml_GetElement(xml, "/MsgText/CustCharge",sData, sizeof(sData));
    		xml_SetElement(xml_Res, "/MsgText/CustCharge",sData);
    		memset(sData, 0x00, sizeof(sData));    	
			xml_GetElementAttr(xml, "/MsgText/CustCharge","ccy", sData, sizeof(sData));
			xml_SetElementAttr(xml_Res,"/MsgText/CustCharge","ccy", sData); 		
        	
    		memset(sData, 0x00, sizeof(sData));
    		xml_GetElement(xml, "/MsgText/FutureCharge",sData, sizeof(sData));
    		xml_SetElement(xml_Res, "/MsgText/FutureCharge",sData);
    		memset(sData, 0x00, sizeof(sData));    	
			xml_GetElementAttr(xml, "/MsgText/FutureCharge","ccy", sData, sizeof(sData));
			xml_SetElementAttr(xml_Res,"/MsgText/FutureCharge","ccy", sData);			
		}
    	memset(sData, 0x00, sizeof(sData));
    	xml_GetElement(xml, "/MsgText/Dgst",sData, sizeof(sData));
    	xml_SetElement(xml_Res, "/MsgText/Dgst",sData);		 		    	    	    	
    	break;    
    default:
    	return -1;
    }
    return 0;                        	
}


/******************************************************************************
函数名称∶GenerateXMLMac
函数功能∶计算MAC值
输入参数∶
         XML     -- XML句柄   
         cFlag   -- 'R'-请求报文,A--应答     
输出参数∶
         sMac    -- 生成MAC值
返 回 值∶
          = 0  成功
          < 0  失败  
******************************************************************************/
long GenerateXMLMac(HXMLTREE xml, char cFlag, char *sMac)
{
    char sMacBuf[1024] = {0};
    char sTransCode[DB_TRSCDE_LEN + 1] = {0};
    char sTradeSrc[2] = {0};
    char sData[256]; 
    char sMacKey[16 + 1] = {0};
    long lRet;
    char sSecuNo[DB_SECUNO_LEN + 1] = {0};
        
    /* 交易代码 */
    xml_GetElement(xml,"/MsgText/GrpHdr/BusCd",sTransCode, sizeof(sTransCode));    
    /* 发起方 */
    xml_GetElement(xml,"/MsgText/GrpHdr/TradSrc",sTradeSrc, sizeof(sTradeSrc)); 
    
    /* 服务商编号 */
    if ( strcmp(sTradeSrc, "B" ) == 0 ) /* 银行发起 */
    {
        xml_GetElement(xml,"/MsgText/GrpHdr/Recver/InstId",sSecuNo, sizeof(sSecuNo));
    }
    else
    {
        xml_GetElement(xml,"/MsgText/GrpHdr/Sender/InstId",sSecuNo, sizeof(sSecuNo));
    }
            
    /* 请求 */
    if ( cFlag == 'R' )
    {
        /* 公共部分 */
        /* 组成:业务功能号|交易发起方|发送机构机构标识|接收机构机构标识|交易日期 */
        /* 功能号 */
        strcpy(sMacBuf, sTransCode);
        
        /* 发起方 */
        sprintf(sMacBuf,"%s|%s", sMacBuf, sTradeSrc);
        
        /* 发送机构 */
        memset(sData, 0x00, sizeof(sData));
        xml_GetElement(xml,"/MsgText/GrpHdr/Sender/InstId",sData, sizeof(sData));
        sprintf(sMacBuf,"%s|%s", sMacBuf, sData);
        
        /* 接收机构 */
        memset(sData, 0x00, sizeof(sData));
        xml_GetElement(xml,"/MsgText/GrpHdr/Recver/InstId",sData, sizeof(sData));
        sprintf(sMacBuf,"%s|%s", sMacBuf, sData);
        
        /* 交易日期 */
        memset(sData, 0x00, sizeof(sData));
        xml_GetElement(xml,"/MsgText/GrpHdr/Date",sData, sizeof(sData));
        sprintf(sMacBuf,"%s|%s", sMacBuf, sData);
        
        /* 流水号(银行发起为银行流水号)/期货发起为期货流水号) */
        if ( strcmp(sTradeSrc, "B") == 0 )
        {
            /* 银行流水号 */
            memset(sData, 0x00, sizeof(sData));
            xml_GetElement(xml,"/MsgText/BkSeq",sData, sizeof(sData));
            sprintf(sMacBuf,"%s|%s", sMacBuf, sData);
        }
        else
        {
            /* 期货流水号 */
            memset(sData, 0x00, sizeof(sData));
            xml_GetElement(xml,"/MsgText/FtSeq",sData, sizeof(sData));
            sprintf(sMacBuf,"%s|%s", sMacBuf, sData);
        }
                            
        switch(atoi(sTransCode))
        {
        case 20001: /* 签到 */
        case 20002: /* 签退 */
        case 23001: /* 文件生成 */
            {
            /*组成：公共部分 */            
            }
            break;
        case 20004: /* 密钥同步 */
            {
            /*组成：公共部分|MacKey */                        
            /* MacKey */
            memset(sData, 0x00, sizeof(sData));
            xml_GetElement(xml,"/MsgText/MacKey",sData, sizeof(sData));
            sprintf(sMacBuf,"%s|%s", sMacBuf, sData);            
            }
            break;
        case 21001: /* 签约 */
            {
            /* 组成：公共部分|客户姓名|证件类型|证件号码|银行账户账号|期货公司保证金账号|开户币种 */                               
            /* 客户姓名 */
            memset(sData, 0x00, sizeof(sData));
            xml_GetElement(xml,"/MsgText/Cust/Name",sData, sizeof(sData));
            sprintf(sMacBuf,"%s|%s", sMacBuf, sData);
            
            /* 证件类型 */
            memset(sData, 0x00, sizeof(sData));
            xml_GetElement(xml,"/MsgText/Cust/CertType",sData, sizeof(sData));
            sprintf(sMacBuf,"%s|%s", sMacBuf, sData);
            
            /* 证件号码 */
            memset(sData, 0x00, sizeof(sData));
            xml_GetElement(xml,"/MsgText/Cust/CertId",sData, sizeof(sData));
            sprintf(sMacBuf,"%s|%s", sMacBuf, sData);
            
            /* 银行卡号 */
            memset(sData, 0x00, sizeof(sData));
            xml_GetElement(xml,"/MsgText/BkAcct/Id",sData, sizeof(sData));
            sprintf(sMacBuf,"%s|%s", sMacBuf, sData);
            
            /* 保证金帐号 */
            memset(sData, 0x00, sizeof(sData));
            xml_GetElement(xml,"/MsgText/FtAcct/Id",sData, sizeof(sData));
            sprintf(sMacBuf,"%s|%s", sMacBuf, sData);
            
            /* 币种 */
            memset(sData, 0x00, sizeof(sData));
            xml_GetElement(xml,"/MsgText/Ccy",sData, sizeof(sData));
            sprintf(sMacBuf,"%s|%s", sMacBuf, sData);
            
            }
            break;
        case 21002: /* 解约 */
        case 21004: /* 查保证金余额 */
        case 21005: /* 查银行余额 */
            {
            /* 组成：公共部分|客户姓名|银行账户账号|期货公司保证金账号 */                                   
            /* 客户姓名 */
            memset(sData, 0x00, sizeof(sData));
            xml_GetElement(xml,"/MsgText/Cust/Name",sData, sizeof(sData));
            sprintf(sMacBuf,"%s|%s", sMacBuf, sData);
                        
            /* 银行卡号 */
            memset(sData, 0x00, sizeof(sData));
            xml_GetElement(xml,"/MsgText/BkAcct/Id",sData, sizeof(sData));
            sprintf(sMacBuf,"%s|%s", sMacBuf, sData);
            
            /* 保证金帐号 */
            memset(sData, 0x00, sizeof(sData));
            xml_GetElement(xml,"/MsgText/FtAcct/Id",sData, sizeof(sData));
            sprintf(sMacBuf,"%s|%s", sMacBuf, sData);                                    
            }
            break;
        case 21003: /* 变更银行卡号 */
            {
            /* 组成：公共部分|客户姓名|证件类型|证件号码|银行账户账号|新银行账户账号|期货公司保证金账号|开户币种 */                      
            /* 客户姓名 */
            memset(sData, 0x00, sizeof(sData));
            xml_GetElement(xml,"/MsgText/Cust/Name",sData, sizeof(sData));
            sprintf(sMacBuf,"%s|%s", sMacBuf, sData);
            
            /* 证件类型 */
            memset(sData, 0x00, sizeof(sData));
            xml_GetElement(xml,"/MsgText/Cust/CertType",sData, sizeof(sData));
            sprintf(sMacBuf,"%s|%s", sMacBuf, sData);
            
            /* 证件号码 */
            memset(sData, 0x00, sizeof(sData));
            xml_GetElement(xml,"/MsgText/Cust/CertId",sData, sizeof(sData));
            sprintf(sMacBuf,"%s|%s", sMacBuf, sData);
            
            /* 银行卡号 */
            memset(sData, 0x00, sizeof(sData));
            xml_GetElement(xml,"/MsgText/BkAcct/Id",sData, sizeof(sData));
            sprintf(sMacBuf,"%s|%s", sMacBuf, sData);
            
            /* 新银行卡号 */
            memset(sData, 0x00, sizeof(sData));
            xml_GetElement(xml,"/MsgText/NewBkAcct/Id",sData, sizeof(sData));
            sprintf(sMacBuf,"%s|%s", sMacBuf, sData);
            
            /* 保证金帐号 */
            memset(sData, 0x00, sizeof(sData));
            xml_GetElement(xml,"/MsgText/FtAcct/Id",sData, sizeof(sData));
            sprintf(sMacBuf,"%s|%s", sMacBuf, sData);
            
            /* 币种 */
            memset(sData, 0x00, sizeof(sData));
            xml_GetElement(xml,"/MsgText/Ccy",sData, sizeof(sData));
            sprintf(sMacBuf,"%s|%s", sMacBuf, sData);
            
            }
            break;
        case 22001: /* 银行转出 */
        case 22002: /* 银行转入 */
        case 22003: /* 冲银行转出 */
        case 22004: /* 冲银行转入 */            
            {
            /* 组成：公共部分|{【冲正交易使用】原银行流水号|原期货流水号}|是否重发标志|客户姓名|银行账户账号|期货公司保证金账号|转帐金额|应收客户手续费|应收期货公司手续费 */                            
            
            if ( strcmp(sTransCode, "22003") == 0 || strcmp(sTransCode, "22004") == 0 )
            {
		                
                /* 原银行流水号 */
                memset(sData, 0x00, sizeof(sData));
                xml_GetElement(xml,"/MsgText/OBkSeq",sData, sizeof(sData));
                sprintf(sMacBuf,"%s|%s", sMacBuf, sData);

                /* 原服务商流水号 */
                memset(sData, 0x00, sizeof(sData));
                xml_GetElement(xml,"/MsgText/OFtSeq",sData, sizeof(sData));
                sprintf(sMacBuf,"%s|%s", sMacBuf, sData);
                
            }
            
            /* 是否重发标志 */
            memset(sData, 0x00, sizeof(sData));
            xml_GetElement(xml,"/MsgText/Resend",sData, sizeof(sData));
            sprintf(sMacBuf,"%s|%s", sMacBuf, sData);
            
            /* 客户姓名 */
            memset(sData, 0x00, sizeof(sData));
            xml_GetElement(xml,"/MsgText/Cust/Name",sData, sizeof(sData));
            sprintf(sMacBuf,"%s|%s", sMacBuf, sData);
                                    
            /* 银行卡号 */
            memset(sData, 0x00, sizeof(sData));
            xml_GetElement(xml,"/MsgText/BkAcct/Id",sData, sizeof(sData));
            sprintf(sMacBuf,"%s|%s", sMacBuf, sData);
            
            /* 保证金帐号 */
            memset(sData, 0x00, sizeof(sData));
            xml_GetElement(xml,"/MsgText/FtAcct/Id",sData, sizeof(sData));
            sprintf(sMacBuf,"%s|%s", sMacBuf, sData);
            
            /* 转账金额 */
            memset(sData, 0x00, sizeof(sData));
            xml_GetElement(xml,"/MsgText/TrfAmt",sData, sizeof(sData));
            sprintf(sMacBuf,"%s|%s", sMacBuf, sData);
            
            /* 客户手续费 */
            memset(sData, 0x00, sizeof(sData));
            xml_GetElement(xml,"/MsgText/CustCharge",sData, sizeof(sData));
            sprintf(sMacBuf,"%s|%s", sMacBuf, sData);
            
            /* 服务商手续费 */
            memset(sData, 0x00, sizeof(sData));
            xml_GetElement(xml,"/MsgText/FutureCharge",sData, sizeof(sData));
            sprintf(sMacBuf,"%s|%s", sMacBuf, sData);            
            }
            break;
        }        
    }
    else /* 应答 */
    {
        /* 公共部分 */
        /* 组成:业务功能号|交易发起方|发送机构机构标识|接收机构机构标识|交易日期|返回代码|银行流水号 */
        /* 功能号 */
        strcpy(sMacBuf, sTransCode);
        
        /* 发起方 */
        sprintf(sMacBuf,"%s|%s", sMacBuf, sTradeSrc);
        
        /* 发送机构 */
        memset(sData, 0x00, sizeof(sData));
        xml_GetElement(xml,"/MsgText/GrpHdr/Sender/InstId",sData, sizeof(sData));
        sprintf(sMacBuf,"%s|%s", sMacBuf, sData);
        
        /* 接收机构 */
        memset(sData, 0x00, sizeof(sData));
        xml_GetElement(xml,"/MsgText/GrpHdr/Recver/InstId",sData, sizeof(sData));
        sprintf(sMacBuf,"%s|%s", sMacBuf, sData);
        
        /* 交易日期 */
        memset(sData, 0x00, sizeof(sData));
        xml_GetElement(xml,"/MsgText/GrpHdr/Date",sData, sizeof(sData));
        sprintf(sMacBuf,"%s|%s", sMacBuf, sData);
        
        /* 返回代码 */
        memset(sData, 0x00, sizeof(sData));
        xml_GetElement(xml,"/MsgText/Rst/Code",sData, sizeof(sData));
        sprintf(sMacBuf,"%s|%s", sMacBuf, sData);
        
        /* 银行流水号 */
        memset(sData, 0x00, sizeof(sData));
        xml_GetElement(xml,"/MsgText/BkSeq",sData, sizeof(sData));
        sprintf(sMacBuf,"%s|%s", sMacBuf, sData);
        
        switch(atoi(sTransCode))
        {
        case 20001: /* 签到 */
        case 20002: /* 签退 */
        case 20004: /* 密钥同步 */
        case 23001: /* 文件生成通知 */
            {
            /*组成：公共部分 + 期货流水号 */
            /* 期货流水号 */
            memset(sData, 0x00, sizeof(sData));
            xml_GetElement(xml,"/MsgText/FtSeq",sData, sizeof(sData));
            sprintf(sMacBuf,"%s|%s", sMacBuf, sData);
            }
            break;
        
        case 21001: /* 签约 */
        case 21002: /* 解约 */
        case 21003: /* 变更银行帐号 */
            {
            /* 组成：公共部分|银行账户账号|期货公司保证金账号|期货公司流水号| */
                        
            /* 银行卡号 */
            memset(sData, 0x00, sizeof(sData));
            xml_GetElement(xml,"/MsgText/BkAcct/Id",sData, sizeof(sData));
            sprintf(sMacBuf,"%s|%s", sMacBuf, sData);
            
            /* 保证金帐号 */
            memset(sData, 0x00, sizeof(sData));
            xml_GetElement(xml,"/MsgText/FtAcct/Id",sData, sizeof(sData));
            sprintf(sMacBuf,"%s|%s", sMacBuf, sData);
                        
            /* 期货流水号 */
            memset(sData, 0x00, sizeof(sData));
            xml_GetElement(xml,"/MsgText/FtSeq",sData, sizeof(sData));
            sprintf(sMacBuf,"%s|%s", sMacBuf, sData);
            }
            break; 
        case 21004: /* 查保证金余额 */        
            {
            /* 
              组成：公共部分|期货公司流水号|期货公司保证金账号|
              期货公司保证金账户当前余额|期货公司保证金账户可用余额|期货公司保证金账户可取余额
            */
            /* 期货流水号 */
            memset(sData, 0x00, sizeof(sData));
            xml_GetElement(xml,"/MsgText/FtSeq",sData, sizeof(sData));
            sprintf(sMacBuf,"%s|%s", sMacBuf, sData);
                                               
            /* 保证金帐号 */
            memset(sData, 0x00, sizeof(sData));
            xml_GetElement(xml,"/MsgText/FtAcct/Id",sData, sizeof(sData));
            sprintf(sMacBuf,"%s|%s", sMacBuf, sData);
            
            /* 当前余额 */
            memset(sData, 0x00, sizeof(sData));
            xml_GetElement(xml,"/MsgText/FtBal/CurBal",sData, sizeof(sData));
            sprintf(sMacBuf,"%s|%s", sMacBuf, sData);
            
            /* 可用余额 */
            memset(sData, 0x00, sizeof(sData));
            xml_GetElement(xml,"/MsgText/FtBal/UseBal",sData, sizeof(sData));
            sprintf(sMacBuf,"%s|%s", sMacBuf, sData); 
            
            /* 可取余额 */
            memset(sData, 0x00, sizeof(sData));
            xml_GetElement(xml,"/MsgText/FtBal/FtcBal",sData, sizeof(sData));
            sprintf(sMacBuf,"%s|%s", sMacBuf, sData);                                                           
            }
            break;
            
        case 21005: /* 查银行余额 */
            {
            /* 
              组成：公共部分|期货公司流水号|银行账号|账户当前余额
            */
            /* 期货流水号 */
            memset(sData, 0x00, sizeof(sData));
            xml_GetElement(xml,"/MsgText/FtSeq",sData, sizeof(sData));
            sprintf(sMacBuf,"%s|%s", sMacBuf, sData);
                        
            /* 银行卡号 */
            memset(sData, 0x00, sizeof(sData));
            xml_GetElement(xml,"/MsgText/BkAcct/Id",sData, sizeof(sData));
            sprintf(sMacBuf,"%s|%s", sMacBuf, sData);
                                                
            /* 银行当前账户余额 */
            memset(sData, 0x00, sizeof(sData));
            xml_GetElement(xml,"/MsgText/BkBal/CurBal",sData, sizeof(sData));
            sprintf(sMacBuf,"%s|%s", sMacBuf, sData);
            }
            break; 
        case 22001: /* 银行转出 */
        case 22002: /* 银行转入 */
        case 22003: /* 冲银行转出 */
        case 22004: /* 冲银行转入 */
            {
            /* 组成：公共部分|期货公司流水号|银行账户账号|期货公司保证金账号|转帐金额|应收客户手续费|应收期货公司手续费 */
            /* 期货流水号 */
            memset(sData, 0x00, sizeof(sData));
            xml_GetElement(xml,"/MsgText/FtSeq",sData, sizeof(sData));
            sprintf(sMacBuf,"%s|%s", sMacBuf, sData);
                        
            /* 银行卡号 */
            memset(sData, 0x00, sizeof(sData));
            xml_GetElement(xml,"/MsgText/BkAcct/Id",sData, sizeof(sData));
            sprintf(sMacBuf,"%s|%s", sMacBuf, sData);
            
            /* 保证金帐号 */
            memset(sData, 0x00, sizeof(sData));
            xml_GetElement(xml,"/MsgText/FtAcct/Id",sData, sizeof(sData));
            sprintf(sMacBuf,"%s|%s", sMacBuf, sData);
            
            /* 转账金额 */
            memset(sData, 0x00, sizeof(sData));
            xml_GetElement(xml,"/MsgText/TrfAmt",sData, sizeof(sData));
            sprintf(sMacBuf,"%s|%s", sMacBuf, sData);
            
            /* 客户手续费 */
            memset(sData, 0x00, sizeof(sData));
            xml_GetElement(xml,"/MsgText/CustCharge",sData, sizeof(sData));
            sprintf(sMacBuf,"%s|%s", sMacBuf, sData);
            
            /* 服务商手续费 */
            memset(sData, 0x00, sizeof(sData));
            xml_GetElement(xml,"/MsgText/FutureCharge",sData, sizeof(sData));
            sprintf(sMacBuf,"%s|%s", sMacBuf, sData);             
            
            }
            break;             
        }
    }
    
    PrintLog( LEVEL_NORMAL, LOG_NOTIC_INFO, "交易代码[%s]发起方[%s],请求/应答[%c]MACBUF[%s]", sTransCode, sTradeSrc, cFlag, sMacBuf); 
/*
	if(memcmp(sTransCode, "20004", 5) == 0 && cFlag == 'A')
	{
		memset(sMacKey, 0x00, sizeof(sMacKey));
		strncpy(sMacKey, g_InitMacKey, sizeof(sMacKey)-1);
	}
	else
*/	{     
    	/* 取MACKEY */
    	lRet = GetXmlMacKey(sSecuNo, sMacKey);
    	if ( lRet < 0 )
    	{
        	PrintLog( LEVEL_ERR, LOG_NOTIC_INFO, "取MACKEY失败[%ld]", lRet);
        	return -1;
    	}
    }
    
    /* 生成MAC */
    GenMAC(sMacBuf, sMac, sMacKey);
                 
    return 0;
}


/******************************************************************************
函数名称∶UpdateXMLMacAndExport
函数功能∶重新计算MAC并导出XML报文 
输入参数∶
         XML     -- XML句柄   
         cFlag   -- 'R'-请求报文,A--应答     
输出参数∶
         sCommBuf -- XML通讯报文 
         lCommLen -- XML报文长度
返 回 值∶
          = 0  成功
          < 0  失败  
******************************************************************************/
long UpdateXMLMacAndExport(HXMLTREE xml, char cFlag, char *sCommBuf, long *lCommLen)
{
    char sMac[16 + 1] = {0};
    long lRet;
    
    PrintLog(LEVEL_DEBUG, LOG_DEBUG_INFO, "%s() Entry", __FUNCTION__);
    
    /* 生成MAC */
    lRet = GenerateXMLMac(xml, cFlag, sMac);
    if ( lRet < 0 )
    {
        return -1;   
    }
    PrintLog(LEVEL_DEBUG, LOG_DEBUG_INFO, "sMac[%s]", sMac); 
    /* 设置MAC */
    if ( xml_SetElement(xml,"/MsgText/Mac", sMac) < 0 )
    {
        PrintLog(LEVEL_ERR, LOG_NOTIC_INFO, "设置MAC值失败[%s]", (char *)xml_LastStringError());
        ERRLOGF("设置MAC值失败[%s]", (char *)xml_LastStringError());
        return -2;
    } 
    
    *lCommLen = xml_ExportXMLStringEx(xml,sCommBuf, 2048,"/MsgText",1,0x00000001,NULL); 
    if ( *lCommLen < 0) 
    {
        PrintLog(LEVEL_ERR, LOG_NOTIC_INFO, "导出XMLBUF失败[%s]", (char *)xml_LastStringError());
        ERRLOGF("导出XMLBUF失败[%s]", (char *)xml_LastStringError());
        return -1;
    }
    PrintLog(LEVEL_DEBUG, LOG_DEBUG_INFO, "%s() Exit", __FUNCTION__);
    return 0;
}

/******************************************************************************
函数名称∶SecuConvXmlExport
函数功能∶服务商发起的交易处理失败时，导出XML报文 
输入参数∶
         XML     -- XML句柄       
输出参数∶
         sCommBuf -- XML通讯报文 
         lCommLen -- XML报文长度
返 回 值∶
          = 0  成功
          < 0  失败  
******************************************************************************/
long SecuConvXmlExport(HXMLTREE xml, char cFlag, char *sCommBuf, long *lCommLen)
{
    char sMac[16 + 1] = {0};
    char sTransCode[DB_TRSCDE_LEN + 1] = {0};
    char sErrCode[12] = {0};
    long lRet;
    
    PrintLog(LEVEL_DEBUG, LOG_DEBUG_INFO, "%s() Entry", __FUNCTION__);
    xml_SetElement(xml, "/MsgText/Rst/Code",g_errcode);
    xml_SetElement(xml, "/MsgText/Rst/Info",g_errmsg);
	memcpy(sErrCode, g_errcode, sizeof(sErrCode));
	xml_GetElement(xml, "/MsgText/GrpHdr/BusCd",sTransCode, sizeof(sTransCode));
	/*密钥同步成功的话，要返回MAC*/
/*	if(memcmp(sTransCode, "20004", 5) == 0 && memcmp(sErrCode, "0000", 4) == 0)
	{    
*/    	/* 生成MAC */
    	lRet = GenerateXMLMac(xml, cFlag, sMac);
    	if ( lRet < 0 )
    	{
    	    return -1;   
    	}
    	PrintLog(LEVEL_DEBUG, LOG_DEBUG_INFO, "sMac[%s]", sMac); 
    	/* 设置MAC */
    	if ( xml_SetElement(xml,"/MsgText/Mac", sMac) < 0 )
    	{
    	    PrintLog(LEVEL_ERR, LOG_NOTIC_INFO, "设置MAC值失败[%s]", (char *)xml_LastStringError());
    	    ERRLOGF("设置MAC值失败[%s]", (char *)xml_LastStringError());
    	    return -2;
    	}
/*    } */
    
    *lCommLen = xml_ExportXMLStringEx(xml,sCommBuf, 2048,"/MsgText",1,0x00000001,NULL); 
    if ( *lCommLen < 0) 
    {
        PrintLog(LEVEL_ERR, LOG_NOTIC_INFO, "导出XMLBUF失败[%s]", (char *)xml_LastStringError());
        ERRLOGF("导出XMLBUF失败[%s]", (char *)xml_LastStringError());
        return -1;
    }
    PrintLog(LEVEL_DEBUG, LOG_DEBUG_INFO, "%s() Exit", __FUNCTION__);
    return 0;	
}


/******************************************************************************
函数名称∶GetXmlMacKey
函数功能∶获取MACKEY密钥
输入参数∶
        sSecuNo:服务商编号     
输出参数∶
        取密钥         
返 回 值∶
          = 0  成功
          < 0  失败  
******************************************************************************/
long GetXmlMacKey(char *sSecuNo, char *sMacKey)
{            
    if ( strlen(sSecuNo) == 0 )
    {
        PrintLog(LEVEL_ERR, LOG_NOTIC_INFO, "取MACKEY错误，输入参数服务商编号为空[%s]", sSecuNo);
        ERRLOGF("取MACKEY错误，输入参数服务商编号为空[%s]", sSecuNo);
        return -1;  
    }
    
    if ( INIscanf(sSecuNo, "MAC_KEY", "%s", sMacKey) < 0 )
    {
        PrintLog(LEVEL_ERR, LOG_NOTIC_INFO, "取MACKEY错误，[%s][MAC_KEY],[%s]", sSecuNo, (char *)strerror(errno));
        ERRLOGF("取交换密钥错误，[%s][MAC_KEY],[%s]", sSecuNo, (char *)strerror(errno));
        return -1;   
    }
       
    PrintLog(LEVEL_DEBUG, LOG_NOTIC_INFO, "MACKEY[%s]",  sMacKey);
    
    return 0;   
}


/******************************************************************************
函数名称∶CheckXMLMac
函数功能∶校验MAC
输入参数∶
         XML     -- XML句柄   
         cFlag   -- 'R'-请求报文,A--应答     
输出参数∶
         无         
返 回 值∶
          = 0  成功
          < 0  失败  
******************************************************************************/
long CheckXMLMac(HXMLTREE xml, char cFlag)
{
    char sMac[16 + 1] = {0};     /* 包里面 */
    char sMacTmp[16 + 1] = {0};  /* 临时生成 */
    int  iCheckFlag = 1;         /* 是否验证MAC */
    long lRet;    
    char sTradeSrc[2] = {0};
    char sSecuNo[DB_SECUNO_LEN + 1] = {0};
    char sTransCode[DB_TRSCDE_LEN + 1] = {0};
    
    
	/*交易码*/
    xml_GetElement(xml,"/MsgText/GrpHdr/BusCd",sTransCode, sizeof(sTransCode)); 
	    
    /* 发起方 */
    xml_GetElement(xml,"/MsgText/GrpHdr/TradSrc",sTradeSrc, sizeof(sTradeSrc)); 
    
    /* 服务商编号 */
    if ( strcmp(sTradeSrc, "B" ) == 0 ) /* 银行发起 */
    {
        xml_GetElement(xml,"/MsgText/GrpHdr/Recver/InstId",sSecuNo, sizeof(sSecuNo));
    }
    else
    {
        xml_GetElement(xml,"/MsgText/GrpHdr/Sender/InstId",sSecuNo, sizeof(sSecuNo));
    }
    
    if ( strlen (sSecuNo) == 0 )
    {
        PrintLog( LEVEL_NORMAL, LOG_NOTIC_INFO, "服务商编号为空"); 
        return -1;
    }
         
    /* 应答包根据参数校验MAC */
    if ( cFlag == 'A' )
    {
        INIscanf( sSecuNo, "CHECK_ANS_MAC", "%d", &iCheckFlag);
        if ( iCheckFlag == 0 )
        {
            PrintLog( LEVEL_NORMAL, LOG_NOTIC_INFO, "参数[%s][CHECK_ANS_MAC]设置为不校验，应答包则不校验MAC", sSecuNo);
            return 0;   
        }
    }
    else
    {
        INIscanf( sSecuNo, "CHECK_REQ_MAC", "%d", &iCheckFlag);
        if ( iCheckFlag == 0 )
        {
            PrintLog( LEVEL_NORMAL, LOG_NOTIC_INFO, "参数[%s][CHECK_REQ_MAC]设置为不校验，请求包不校验MAC", sSecuNo);
            return 0;   
        }         
    }
        
    /* 包MAC */
    /*密钥同步需要取配置文件里的初始密钥*/

    if ( xml_GetElement(xml,"/MsgText/Mac",sMac, sizeof(sMac)) < 0 )
    {
        PrintLog( LEVEL_NORMAL, LOG_NOTIC_INFO, "取节点[/MsgText/Mac]失败，[%s]",  (char *)xml_LastStringError());
        return -1;
    }
    
    /* 生成MAC */
    lRet = GenerateXMLMac(xml, cFlag, sMacTmp);
    if ( lRet < 0 )
    {
        return -2;   
    }
    
    /*  比较MAC */
    if ( strcmp(sMacTmp, sMac) != 0 )
    {
        PrintLog( LEVEL_NORMAL, LOG_NOTIC_INFO, "MAC校验不通过,包MAC[%s]计算MAC[%s]", sMac, sMacTmp);
        return -3;
    }
    return 0;
}
    
