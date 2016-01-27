/******************************************************************************
�ļ����ơ�IFCommConv.c
ϵͳ���ơ��ڻ���֤����ϵͳ
ģ�����ơýӿ�ת����������ӿڣ�
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
          V3.0.0.0  ��  ��  20131211-01-��������SecuConvXmlToFix ȯ�̺�Ԥ��ȡֵ
          V3.0.0.1  ������  201402130121-01��20004��������CheckXMLMacУ��
          V3.0.0.2  ������  20150915-01-�����������͵�ǰ���ȡֵ
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
�������ơ�BankConvFixToXml
�������ܡ����з���FIX->XML����ת��
���������*pfix     -- ������FIX��Ϣ          
���������
          sAnsBuf  -- Ӧ����          
          lAnsLen  -- Ӧ���ĳ���
�� �� ֵ��
          = 0  ת���ɹ�
          < 0  ת��ʧ��  
******************************************************************************/
long BankConvFixToXml(FIX *pfix, char *sCommBuf, long *lCommLen)
{   
    char sData[256],sData1[256],sSecuNo[DB_SECUNO_LEN + 1] = {0},sSecuDir[256] = {0};    
    char sTransCode[DB_TRSCDE_LEN + 1] = {0},sFileName[256] = {0};
    struct stat f_stat;
    long lRet;
    HXMLTREE  xml;
    
    PrintLog( LEVEL_DEBUG, LOG_DEBUG_INFO, "%s() Entry", __FUNCTION__);
    /* ����XML��� */
    xml = xml_Create("ROOT");
    if ( xml == FAIL ) 
    {        
        FIXputs(pfix, "ErrorNo", "1063");
        FIXputs(pfix, "ErrorInfo", "��ʼ��XML���ʧ��");        
        PrintLog( LEVEL_ERR, LOG_NOTIC_INFO, "1063|��ʼ��XML���ʧ��");        
        return -1;
    }
    
    /* ���״��� */
    FIXgets(pfix, "FunctionId", sTransCode, sizeof(sTransCode));
       
    /* ������ڵ� */
    xml_SetElement(xml,"/MsgText","");         
    /* ��Ϣͷ */
    xml_SetElement(xml,"/MsgText/GrpHdr","");
    {
        /* �汾�� */
        xml_SetElement(xml,"/MsgText/GrpHdr/Version","1.0.0");
        
        /* Ӧ��ϵͳ����:8-����ת�� */
        xml_SetElement(xml,"/MsgText/GrpHdr/SysType","8");
        
        /* ���ܺ� */
        memset(sData, 0x00, sizeof(sData));        
        LookUp(204, sData, sTransCode);         
        xml_SetElement(xml,"/MsgText/GrpHdr/BusCd",sData);
        
        /* ���׷���:B-���з��� */
        xml_SetElement(xml,"/MsgText/GrpHdr/TradSrc","B");        
        
        /* ���ͻ��� */
        /* ���ͻ���(������ʶ) */
        memset(sData, 0x00, sizeof(sData));
        memset(sData1, 0x00, sizeof(sData1));
        FIXgets(pfix, "bOrganId", sData1, sizeof(sData1));
        if ( LookUp(202, sData, sData1) < 0 )
        {
        	xml_Destroy(xml);
            FIXputs(pfix, "ErrorNo", "3054");
            FIXputs(pfix, "ErrorInfo", "��������δ��ͨ(���ܲ������ô���)");        
            PrintLog( LEVEL_ERR, LOG_NOTIC_INFO, "3054|��������δ��ͨ(���ܲ������ô��������ֵ�[202])");        
            return -1;
        }
        xml_SetElement(xml,"/MsgText/GrpHdr/Sender/InstId",sData);
                                      
        /* ���ͻ���(��֧����) */
        memset(sData, 0x00, sizeof(sData));
        FIXgets(pfix, "bBranchId", sData, sizeof(sData));
        xml_SetElement(xml,"/MsgText/GrpHdr/Sender/BrchId",sData);
        
        /* ���ͻ���(������) */
        memset(sData, 0x00, sizeof(sData));
        FIXgets(pfix, "bDeptId", sData, sizeof(sData));                
        xml_SetElement(xml,"/MsgText/GrpHdr/Sender/SubBrchId",sData);
       
                
        /* ���ջ��� */
        /* ���ջ���(������ʶ) */
        memset(sData, 0x00, sizeof(sData));
        FIXgets(pfix, "sOrganId", sData, sizeof(sData));
        xml_SetElement(xml,"/MsgText/GrpHdr/Recver/InstId", sData);
        strncpy(sSecuNo, sData, sizeof(sSecuNo)-1);
                    
        /* ���ջ���(��֧����) */
        memset(sData, 0x00, sizeof(sData));
        FIXgets(pfix, "sBranchId", sData, sizeof(sData));
        xml_SetElement(xml,"/MsgText/GrpHdr/Recver/BrchId",sData);
                
        /* ���ջ���(��֧����) */
        FIXgets(pfix, "sBranchId", sData, sizeof(sData));
        xml_SetElement(xml,"/MsgText/GrpHdr/Recver/SubBrchId",sData);
                                                
        /* �������� */
        memset(sData, 0x00, sizeof(sData));
        FIXgets(pfix, "Date", sData, 9);
        sprintf(sData,"%08s",sData);
        xml_SetElement(xml,"/MsgText/GrpHdr/Date",sData);                      
    
        /* ����ʱ�� */
        memset(sData, 0x00, sizeof(sData));
        FIXgets(pfix, "Time", sData, 7);
        sprintf(sData,"%06s",GetSysTime());        
        xml_SetElement(xml,"/MsgText/GrpHdr/Time",sData);               
    }

    /* ������ˮ�� */
    memset(sData, 0x00, sizeof(sData));
    FIXgets(pfix, "ExSerial", sData, sizeof(sData));
    xml_SetElement(xml,"/MsgText/BkSeq",sData);          
    
    /* ժҪ */
    memset(sData, 0x00, sizeof(sData));
    FIXgets(pfix, "Summary", sData, sizeof(sData));
    xml_SetElement(xml,"/MsgText/Dgst",sData); 
                            
                           
    switch(atoi(sTransCode))
    {
    case 25714: /* ǩԼ */
    {     
        /* �ͻ���Ϣ */
        xml_SetElement(xml,"/MsgText/Cust","");
        {
            /* �ͻ����� */
            memset(sData, 0x00, sizeof(sData));
            FIXgets(pfix, "Name", sData, sizeof(sData));
            xml_SetElement(xml,"/MsgText/Cust/Name",sData);
                                   
            /* ֤������ */                  
            /*
             10-���֤ 11-���� 12-����֤ 13-ʿ��֤ 14-����֤ 15-���ڱ� 16-Ӫҵִ�� 17-��ҵ���˴���֤����� 20-����
            */ 
            memset(sData, 0x00, sizeof(sData));
            FIXgets(pfix, "CardType", sData, sizeof(sData));
            LookUp(201, sData, sData);
            xml_SetElement(xml,"/MsgText/Cust/CertType",sData);            
                           
          
            /* ֤������ */
            memset(sData, 0x00, sizeof(sData));
            FIXgets(pfix, "Card", sData, sizeof(sData));
            xml_SetElement(xml,"/MsgText/Cust/CertId",sData);
                           
        
            /* �ͻ�����:0-���� 1-���� */
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
                                            
            /* �Ա� */
            xml_SetElement(xml,"/MsgText/Cust/Sex","");
            /* ���� */
            memset(sData, 0x00, sizeof(sData));
            FIXgets(pfix, "Nationality", sData, sizeof(sData));
            xml_SetElement(xml,"/MsgText/Cust/Ntnl",sData);
        
            /* ͨѶ��ַ  */
            memset(sData, 0x00, sizeof(sData));
            FIXgets(pfix, "Address", sData, sizeof(sData));
            xml_SetElement(xml,"/MsgText/Cust/Addr",sData);
        
            /* ��������  */
            memset(sData, 0x00, sizeof(sData));
            FIXgets(pfix, "PostCode", sData, sizeof(sData));
            xml_SetElement(xml,"/MsgText/Cust/PstCd",sData);
        
            /* �����ʼ�  */
            memset(sData, 0x00, sizeof(sData));
            FIXgets(pfix, "Email", sData, sizeof(sData));
            xml_SetElement(xml,"/MsgText/Cust/Email",sData);
        
            /* ����  */
            memset(sData, 0x00, sizeof(sData));
            FIXgets(pfix, "FaxNo", sData, sizeof(sData));
            xml_SetElement(xml,"/MsgText/Cust/Fax",sData);
        
            /* �ֻ�  */
            memset(sData, 0x00, sizeof(sData));
            FIXgets(pfix, "MobiNo", sData, sizeof(sData));
            xml_SetElement(xml,"/MsgText/Cust/Mobile",sData);
        
            /* �绰  */
            memset(sData, 0x00, sizeof(sData));
            FIXgets(pfix, "TelNo", sData, sizeof(sData));
            xml_SetElement(xml,"/MsgText/Cust/Tel",sData);
            
            /* BP */
            xml_SetElement(xml,"/MsgText/Cust/Bp","");
        }
        
        /* �����˻� */
        xml_SetElement(xml,"/MsgText/BkAcct","");
        {
            /* �����ʺ� */
            memset(sData, 0x00, sizeof(sData));
            FIXgets(pfix, "bCustAcct", sData, sizeof(sData));
            xml_SetElement(xml,"/MsgText/BkAcct/Id",sData);                                                                
        }
        /* �ڻ��˻� */
        xml_SetElement(xml,"/MsgText/FtAcct","");
        {
            /* �ʽ��ʺ� */
            memset(sData, 0x00, sizeof(sData));
            FIXgets(pfix, "sCustAcct", sData, sizeof(sData));
            xml_SetElement(xml,"/MsgText/FtAcct/Id",sData);
                                
            /* �ʽ��ʺ����� */
            xml_SetElement(xml,"/MsgText/FtAcct/Pwd","");
            {
            	/*��ʱ��Ҫ�����Ƿ���ܷ�ʽ���е���*/
            	xml_SetElement(xml,"/MsgText/FtAcct/Pwd/Type","0");
            	xml_SetElement(xml,"/MsgText/FtAcct/Pwd/Enc","00");
            	memset(sData, 0x00, sizeof(sData));
            	FIXgets(pfix, "sCustPwd2", sData, sizeof(sData));
            	xml_SetElement(xml,"/MsgText/FtAcct/Pwd/Pwd",sData); 
            }                         
        }
        
        /* ���� */    
        memset(sData, 0x00, sizeof(sData));
        FIXgets(pfix, "MoneyType", sData, sizeof(sData));
        LookUp(203, sData, sData);
        xml_SetElement(xml,"/MsgText/Ccy",sData); 
         
        /* �����־:0-�� 1-�� */
        xml_SetElement(xml,"/MsgText/CashExCd","");
        
        /* ԤԼ��ˮ�� */
/*        xml_SetElement(xml,"/MsgText/Bookseq",""); */                       
    }
    break;
    case 25715: /* ��Լ */
    case 23603: /* �鱣֤����� */        
    {        
        /* �ͻ���Ϣ */
        xml_SetElement(xml,"/MsgText/Cust","");
        {
            /* �ͻ����� */
            memset(sData, 0x00, sizeof(sData));
            FIXgets(pfix, "Name", sData, sizeof(sData));
            xml_SetElement(xml,"/MsgText/Cust/Name",sData);
                                   
            /* ֤������ */                  
            /*
             10-���֤ 11-���� 12-����֤ 13-ʿ��֤ 14-����֤ 15-���ڱ� 16-Ӫҵִ�� 17-��ҵ���˴���֤����� 20-����
            */ 
            memset(sData, 0x00, sizeof(sData));
            FIXgets(pfix, "CardType", sData, sizeof(sData));
            LookUp(201, sData, sData);
            xml_SetElement(xml,"/MsgText/Cust/CertType",sData);            
                           
          
            /* ֤������ */
            memset(sData, 0x00, sizeof(sData));
            FIXgets(pfix, "Card", sData, sizeof(sData));
            xml_SetElement(xml,"/MsgText/Cust/CertId",sData);                                                                  
        }
        
        /* �����˻� */
        xml_SetElement(xml,"/MsgText/BkAcct","");
        {
            /* �����ʺ� */
            memset(sData, 0x00, sizeof(sData));
            FIXgets(pfix, "bCustAcct", sData, sizeof(sData));
            xml_SetElement(xml,"/MsgText/BkAcct/Id",sData);                                                                       
        }  

        xml_SetElement(xml,"/MsgText/FtAcct","");
        {
            /* �ʽ��ʺ� */
            memset(sData, 0x00, sizeof(sData));
            FIXgets(pfix, "sCustAcct", sData, sizeof(sData));
            xml_SetElement(xml,"/MsgText/FtAcct/Id",sData);
                                
            /* �ʽ��ʺ����� */
            xml_SetElement(xml,"/MsgText/FtAcct/Pwd","");
            {
            	/*��ʱ��Ҫ�����Ƿ���ܷ�ʽ���е���*/
            	xml_SetElement(xml,"/MsgText/FtAcct/Pwd/Type","0");
            	xml_SetElement(xml,"/MsgText/FtAcct/Pwd/Enc","00");
            	memset(sData, 0x00, sizeof(sData));
            	FIXgets(pfix, "sCustPwd2", sData, sizeof(sData));
            	xml_SetElement(xml,"/MsgText/FtAcct/Pwd/Pwd",sData); 
            }                         
        }
                
        /* ���� */    
        memset(sData, 0x00, sizeof(sData));
        FIXgets(pfix, "MoneyType", sData, sizeof(sData));
        LookUp(203, sData, sData);
        xml_SetElement(xml,"/MsgText/Ccy",sData);                            
    }
    break;
    case 25710: /* ��������ʺ� */
    {       
        /* �ͻ���Ϣ */
        xml_SetElement(xml,"/MsgText/Cust","");
        {
            /* �ͻ����� */
            memset(sData, 0x00, sizeof(sData));
            FIXgets(pfix, "Name", sData, sizeof(sData));
            xml_SetElement(xml,"/MsgText/Cust/Name",sData);
                                   
            /* ֤������ */                  
            /*
             10-���֤ 11-���� 12-����֤ 13-ʿ��֤ 14-����֤ 15-���ڱ� 16-Ӫҵִ�� 17-��ҵ���˴���֤����� 20-����
            */ 
            memset(sData, 0x00, sizeof(sData));
            FIXgets(pfix, "CardType", sData, sizeof(sData));
            LookUp(201, sData, sData);
            xml_SetElement(xml,"/MsgText/Cust/CertType",sData);            
                           
          
            /* ֤������ */
            memset(sData, 0x00, sizeof(sData));
            FIXgets(pfix, "Card", sData, sizeof(sData));
            xml_SetElement(xml,"/MsgText/Cust/CertId",sData);                                                                  
        }
        
        /* �����˻� */
        xml_SetElement(xml,"/MsgText/BkAcct","");
        {
            /* �����ʺ� */
            memset(sData, 0x00, sizeof(sData));
            FIXgets(pfix, "bCustAcct", sData, sizeof(sData));
            xml_SetElement(xml,"/MsgText/BkAcct/Id",sData);                                                                                                            
        }
        
        /* �������˻� */
        xml_SetElement(xml,"/MsgText/NewBkAcct","");
        {
            /* �����ʺ� */
            memset(sData, 0x00, sizeof(sData));
            FIXgets(pfix, "bNewAcct", sData, sizeof(sData));
            xml_SetElement(xml,"/MsgText/NewBkAcct/Id",sData);                                                                                                        
        }
        /* �ڻ��˻� */
        xml_SetElement(xml,"/MsgText/FtAcct","");
        {
            /* �ʽ��ʺ� */
            memset(sData, 0x00, sizeof(sData));
            FIXgets(pfix, "sCustAcct", sData, sizeof(sData));
            xml_SetElement(xml,"/MsgText/FtAcct/Id",sData);
                                                                                   
            /* �ʽ��ʺ����� */
            xml_SetElement(xml,"/MsgText/FtAcct/Pwd","");
            {
            	/*��ʱ��Ҫ�����Ƿ���ܷ�ʽ���е���*/
            	xml_SetElement(xml,"/MsgText/FtAcct/Pwd/Type","0");
            	xml_SetElement(xml,"/MsgText/FtAcct/Pwd/Enc","00");
            	memset(sData, 0x00, sizeof(sData));
            	FIXgets(pfix, "sCustPwd2", sData, sizeof(sData));
            	xml_SetElement(xml,"/MsgText/FtAcct/Pwd/Pwd",sData); 
            } 
        }
        
        /* ���� */    
        memset(sData, 0x00, sizeof(sData));
        FIXgets(pfix, "MoneyType", sData, sizeof(sData));
        LookUp(203, sData, sData);
        xml_SetElement(xml,"/MsgText/Ccy",sData); 
                      
        /* �����־:0-�� 1-�� */
        xml_SetElement(xml,"/MsgText/CashExCd","");                
        
    }
    break;
    
    case 23701: /* ����ת�� */
    case 23702: /* ����ת�� */  
    case 23703: /* ������ת�� */ 
    case 23704: /* ������ת�� */
    {   
        /* �Ƿ��ط���־(�Ƿ��ط�Y-��,N-��) */
        memset(sData, 0x00, sizeof(sData));        
        xml_SetElement(xml,"/MsgText/Resend", "N"); 
        
        /* ����������Ҫ��дԭ������ˮ */
        if ( strcmp(sTransCode, "23703") == 0 ||  strcmp(sTransCode, "23704") == 0 )
        {
            /* ԭ������ˮ�� */
            memset(sData, 0x00, sizeof(sData));
            FIXgets(pfix, "CorrectSerial", sData, sizeof(sData));
            xml_SetElement(xml,"/MsgText/OBkSeq",sData);
        }
        
        /* �ͻ���Ϣ */
        xml_SetElement(xml,"/MsgText/Cust","");
        {
            /* �ͻ����� */
            memset(sData, 0x00, sizeof(sData));
            FIXgets(pfix, "Name", sData, sizeof(sData));
            xml_SetElement(xml,"/MsgText/Cust/Name",sData);
                                   
            /* ֤������ */                  
            /*
             10-���֤ 11-���� 12-����֤ 13-ʿ��֤ 14-����֤ 15-���ڱ� 16-Ӫҵִ�� 17-��ҵ���˴���֤����� 20-����
            */ 
            memset(sData, 0x00, sizeof(sData));
            FIXgets(pfix, "CardType", sData, sizeof(sData));
            LookUp(201, sData, sData);
            xml_SetElement(xml,"/MsgText/Cust/CertType",sData);            
                           
          
            /* ֤������ */
            memset(sData, 0x00, sizeof(sData));
            FIXgets(pfix, "Card", sData, sizeof(sData));
            xml_SetElement(xml,"/MsgText/Cust/CertId",sData);                                                                  
        }
        
        /* �����˻� */
        xml_SetElement(xml,"/MsgText/BkAcct","");
        {
            /* �����ʺ� */
            memset(sData, 0x00, sizeof(sData));
            FIXgets(pfix, "bCustAcct", sData, sizeof(sData));
            xml_SetElement(xml,"/MsgText/BkAcct/Id",sData);                                                                                                            
        }
                        
        /* �ڻ��˻� */
        xml_SetElement(xml,"/MsgText/FtAcct","");
        {
            /* �ʽ��ʺ� */
            memset(sData, 0x00, sizeof(sData));
            FIXgets(pfix, "sCustAcct", sData, sizeof(sData));
            xml_SetElement(xml,"/MsgText/FtAcct/Id",sData);   
            
            /* �ʽ��ʺ����� */
            xml_SetElement(xml,"/MsgText/FtAcct/Pwd","");
            {
            	/*��ʱ��Ҫ�����Ƿ���ܷ�ʽ���е���*/
            	xml_SetElement(xml,"/MsgText/FtAcct/Pwd/Type","0");
            	xml_SetElement(xml,"/MsgText/FtAcct/Pwd/Enc","00");
            	memset(sData, 0x00, sizeof(sData));
            	FIXgets(pfix, "sCustPwd2", sData, sizeof(sData));
            	xml_SetElement(xml,"/MsgText/FtAcct/Pwd/Pwd",sData); 
            }                                                                    
        }
        
        /* ת�˽�� */
        xml_SetElement(xml,"/MsgText/TrfAmt","");
        {
            /* ��� */
            memset(sData, 0x00, sizeof(sData));
            FIXgets(pfix, "OccurBala", sData, sizeof(sData));
            snprintf(sData, sizeof(sData), "%.0lf", atof(sData) * 100.00); /* �����С�� */
            xml_SetElement(xml,"/MsgText/TrfAmt",sData);

            xml_SetElement(xml,"/MsgText/CustCharge","0");
            xml_SetElement(xml,"/MsgText/FutureCharge","0");
                           
        
            /* ���֣�ת�˱������⣩ */
            memset(sData, 0x00, sizeof(sData));
            FIXgets(pfix, "MoneyType", sData, sizeof(sData));
            LookUp(203, sData, sData);
            xml_SetElementAttr(xml,"/MsgText/TrfAmt","ccy", sData);            
            xml_SetElementAttr(xml,"/MsgText/CustCharge","ccy", sData);            
            xml_SetElementAttr(xml,"/MsgText/FutureCharge","ccy", sData);            
        }                                           
    }
    break;
    
    case 23904: /* �ļ�֪ͨ(�����ļ�) */
    case 23903: /* �ļ�֪ͨ */
    {
    	/*�����ļ���Ϣ1-������ϸ�ļ�*/
        xml_AddElement(xml,"/MsgText/FileInfo","");
        {
            /* �ļ�ҵ���� */
            memset(sData, 0x00, sizeof(sData));
            xml_AddElement(xml,"/MsgText/FileInfo/BusCode","0001"); 
            
            /* �ļ�ҵ������ */  
            memset(sData, 0x00, sizeof(sData));
            FIXgets(pfix, "Date", sData, 9);
        	sprintf(sData,"%08s",sData);
            xml_AddElement(xml,"/MsgText/FileInfo/BusDate",sData);  

            /* �ļ�������� */  
            memset(sData, 0x00, sizeof(sData));
            INIscanf(sSecuNo, "FILEIP", "%s", sData);
            xml_AddElement(xml,"/MsgText/FileInfo/Host",sData);
            
            /* �ļ����� */ 
        	memset(sData1, 0x00, sizeof(sData1));
        	FIXgets(pfix, "Date", sData1, 9);
        	sprintf(sData1,"%08s",sData1);             
            memset(sData, 0x00, sizeof(sData));            
            sprintf(sData, "B_DTL01_%08s.gz", sData1);            
            xml_AddElement(xml,"/MsgText/FileInfo/FileName",sData);
            memset(sFileName, 0x00, sizeof(sFileName));
            strncpy(sFileName, sData, sizeof(sFileName)-1);    
            
            /* �ļ����� */ 
    		SetIniName("rzcl.ini");
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
    		SetIniName("future.ini"); 
            memset(sData, 0x00, sizeof(sData)); 
			sprintf(sData,"%s/%s/%s%s", sSecuDir, sSecuNo, SENDFILE_DIR, sFileName);
			stat(sData, &f_stat);
			memset(sData, 0x00, sizeof(sData)); 
			sprintf(sData, "%ld", f_stat.st_size);
            xml_AddElement(xml,"/MsgText/FileInfo/FileLen",sData);
            
            /*�ļ�ʱ��*/
			memset(sData, 0x00, sizeof(sData)); 
            xml_AddElement(xml,"/MsgText/FileInfo/FileTime","");            

            /*�ļ�У����*/
/*			memset(sData, 0x00, sizeof(sData)); 
            xml_AddElement(xml,"/MsgText/FileInfo/FileMac","");
*/            			    		                   
    	} 
    	/*�����ļ���Ϣ2-���˻����ļ�*/
        xml_AddElement(xml,"/MsgText/FileInfo","");
        {
            /* �ļ�ҵ���� */
            memset(sData, 0x00, sizeof(sData));
            xml_AddElement(xml,"/MsgText/FileInfo/BusCode","0002"); 
            
            /* �ļ�ҵ������ */  
            memset(sData, 0x00, sizeof(sData));
            FIXgets(pfix, "Date", sData, 9);
        	sprintf(sData,"%08s",sData);
            xml_AddElement(xml,"/MsgText/FileInfo/BusDate",sData);  

            /* �ļ�������� */  
            memset(sData, 0x00, sizeof(sData));
            INIscanf(sSecuNo, "FILEIP", "%s", sData);
            xml_AddElement(xml,"/MsgText/FileInfo/Host",sData);
            
            /* �ļ����� */ 
        	memset(sData1, 0x00, sizeof(sData1));
        	FIXgets(pfix, "Date", sData1, 9);
        	sprintf(sData1,"%08s",sData1);             
            memset(sData, 0x00, sizeof(sData));            
            sprintf(sData, "B_DTL02_%08s.gz", sData1);            
            xml_AddElement(xml,"/MsgText/FileInfo/FileName",sData);
            memset(sFileName, 0x00, sizeof(sFileName));
            strncpy(sFileName, sData, sizeof(sFileName)-1);    
            
            /* �ļ����� */ 
    		SetIniName("rzcl.ini");
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
    		SetIniName("future.ini"); 
            memset(sData, 0x00, sizeof(sData)); 
			sprintf(sData,"%s/%s/%s%s", sSecuDir, sSecuNo, SENDFILE_DIR, sFileName);
			stat(sData, &f_stat);
			memset(sData, 0x00, sizeof(sData)); 
			sprintf(sData, "%ld", f_stat.st_size);
            xml_AddElement(xml,"/MsgText/FileInfo/FileLen",sData);
            
            /*�ļ�ʱ��*/
			memset(sData, 0x00, sizeof(sData)); 
            xml_AddElement(xml,"/MsgText/FileInfo/FileTime","");            

            /*�ļ�У����*/
/*			memset(sData, 0x00, sizeof(sData)); 
            xml_AddElement(xml,"/MsgText/FileInfo/FileMac","");
*/            			    		                   
    	}    		
    }
    break;
    default:
        FIXputs(pfix, "ErrorNo", "2033");
        FIXputs(pfix, "ErrorInfo", "�ý��ײ�֧��");        
        PrintLog( LEVEL_ERR, LOG_NOTIC_INFO, "2033|�ý��ײ�֧��(ת��),[%s]", sTransCode);        
        return -2;        
    }
    
    /* ����ͨѶ�� */
    lRet = UpdateXMLMacAndExport(xml, 'R', sCommBuf, lCommLen);
    if ( lRet < 0 )
    {
        FIXputs(pfix, "ErrorNo", "2041");
        FIXputs(pfix, "ErrorInfo", "����ͨѶ�����ʧ��");        
        PrintLog( LEVEL_ERR, LOG_NOTIC_INFO, "2041|����ͨѶ�����ʧ��(ת��)");
        return -3;
    }
    xml_Destroy(xml);
    PrintLog( LEVEL_DEBUG, LOG_DEBUG_INFO, "%s() Exit", __FUNCTION__);
    return 0;
}


/******************************************************************************
�������ơ�BankConvXmlToFix
�������ܡ����з���XML->FIX����ת��
���������
        sCommBuf  -- ͨѶXML����
        lCommLen  -- ͨѶ���ĳ���
���������
         *pfix     -- FIX
�� �� ֵ��
          = 0  �ɹ�
          < 0  ʧ��
******************************************************************************/
long BankConvXmlToFix(FIX *pfix, char *sCommBuf, long *lCommLen)
{
    char sData[256];    
    char sTransCode[DB_TRSCDE_LEN + 1] = {0};
    char sErrCode[12] = {0};
    long lRet;
    HXMLTREE  xml;
    
    /* ���״��� */
    FIXgets(pfix, "FunctionId", sTransCode, sizeof(sTransCode));
    
    /* ����XML��� */
    xml = xml_Create("ROOT");
    if ( xml ==  FAIL )  
    {        
        PrintLog(LEVEL_ERR, LOG_NOTIC_INFO, "תXMLӦ���ʱ����XML���ʧ��[%s]",(char *)xml_LastStringError());
        ERRLOGF("תXMLӦ���ʱ����XML���ʧ��[%s]", (char *)xml_LastStringError());
        FIXputs(pfix, "ErrorNo", "1063");
        FIXputs(pfix, "ErrorInfo", "��ʼ��XML���ʧ��");         
        return -1;
    }
            
    /* ����XML */
    lRet = xml_ImportXMLString(xml,sCommBuf, "", 0);
    if ( lRet == FAIL ) 
    {              
        PrintLog(LEVEL_ERR, LOG_NOTIC_INFO, "תXMLӦ���ʱ����XMLʧ��[%s]",(char *)xml_LastStringError());
        ERRLOGF("תXMLӦ���ʱ����XMLʧ��[%s]", (char *)xml_LastStringError());
        FIXputs(pfix, "ErrorNo", "1063");
        FIXputs(pfix, "ErrorInfo", "תXMLӦ���ʱ����XMLʧ��");         
        xml_Destroy(xml);
        return -2;
    }

    /*������Ϣ*/
    memset(sData, 0x00, sizeof(sData));
    xml_GetElement(xml,"/MsgText/Rst/Info",sData,sizeof(sData));
    FIXputs(pfix,"ErrorInfo",sData);
           
    /*���ɹ��Ļ���ֱ�ӷ��أ���ȥУ��MAC*/    
    /*������*/
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
        
    /* MAC��� */
    if ( CheckXMLMac(xml, 'A') < 0 )
    {
        FIXputs(pfix, "ErrorNo", "1063");
        FIXputs(pfix, "ErrorInfo", "MACУ�����");      	
    	xml_Destroy(xml);
        return -3;
    }
        
    /* ��ˮ��  */
    memset(sData, 0x00, sizeof(sData));
    xml_GetElement(xml,"/MsgText/BkSeq",sData,sizeof(sData));
    FIXputs(pfix,"ExSerial", sData);
    
    /*�ڻ���ˮ��*/ 
    memset(sData, 0x00, sizeof(sData));
    xml_GetElement(xml,"/MsgText/FtSeq",sData,sizeof(sData));
    FIXputs(pfix,"sSerial",sData);         
                 
    
    /* ���׳ɹ��������ȡӦ����Ϣ */
    if ( memcmp(sErrCode,"0000", 4) == 0) 
    {
        /* �ʽ��ʺ� */
        memset(sData, 0x00,sizeof(sData));
        xml_GetElement(xml,"/MsgText/FtAcct/Id",sData, sizeof(sData));
        FIXputs(pfix,"sCustAcct", sData);
        
        /* Ӫҵ������ */
        memset(sData, 0x00,sizeof(sData));
        xml_GetElement(xml,"/MsgText/GrpHdr/Recver/BrchId",sData, sizeof(sData));
        FIXputs(pfix,"sBranchId", sData);
                                                                                               
        /* ������򷵻������Ϣ */
        if ( strcmp(sTransCode, "23603") == 0 )
        {
            /* ��ǰ��� */
            memset(sData, 0x00, sizeof(sData));
            xml_GetElement(xml,"/MsgText/FtBal/CurBal",sData, sizeof(sData));
            FIXprintf(pfix, "OccurBala", "%.2lf", (atof(sData) /100.00) );
            
            /* ������� */
            memset(sData, 0x00, sizeof(sData));
            xml_GetElement(xml,"/MsgText/FtBal/UseBal",sData, sizeof(sData));
            FIXprintf(pfix, "sEnableBala", "%.2lf", (atof(sData) /100.00) );
            
            /* ��ȡ��� */
            memset(sData, 0x00, sizeof(sData));
            xml_GetElement(xml,"/MsgText/FtBal/FtcBal",sData, sizeof(sData));
            FIXprintf(pfix, "sFetchBala", "%.2lf", (atof(sData) /100.00) );                        
        }
    }
    xml_Destroy(xml);
    return 0;
}


/******************************************************************************
�������ơ�SecuConvXmlToFix
�������ܡ÷����̷���XML->FIX����ת��
���������
         XML -- XML���
���������
         sCommBuf -- FIX����
         lCommLen -- FIX���ĳ���
         sSecuNo  -- �����̱��
�� �� ֵ��
          = 0  �ɹ�
          < 0  ʧ��  
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
    
    /*20131211-01-��������SecuConvXmlToFix ȯ�̺�Ԥ��ȡֵ*/
    /* �����̱�� */
    memset(sData, 0x00, sizeof(sData));    
    if ( xml_GetElement(xml, "/MsgText/GrpHdr/Sender/InstId",sSecuNo, DB_SECUNO_LEN + 1) < 0 )
    {
        PrintLog(LEVEL_ERR, LOG_NOTIC_INFO, "��ȡXML�ڵ�[/MsgText/GrpHdr/Sender/InstId]ʧ�ܣ�[%s]",  (char *)xml_LastStringError());        
        strncpy(g_errcode, "1063", sizeof(g_errcode)-1);
        strncpy(g_errmsg, "��ȡXML�ڵ�[/MsgText/GrpHdr/Sender/InstId]ʧ��", sizeof(g_errmsg)-1);    	        
        return -1;
    }
            
    /* ���״��� */
    memset(sData, 0x00, sizeof(sData));
    if ( xml_GetElement(xml, "/MsgText/GrpHdr/BusCd",sTransCode, sizeof(sTransCode)) < 0 )
    {
        PrintLog(LEVEL_ERR, LOG_NOTIC_INFO, "��ȡXML�ڵ�[/MsgText/GrpHdr/BusCd]ʧ�ܣ�[%s]",  (char *)xml_LastStringError());        
        strncpy(g_errcode, "1063", sizeof(g_errcode)-1);
        strncpy(g_errmsg, "��ȡXML�ڵ�[/MsgText/GrpHdr/BusCd]ʧ��", sizeof(g_errmsg)-1);    	        
        return -1;
    }
            
            
    /* MACУ�� */
    /* 201402130121-������-��20004��������CheckXMLMacУ�� */
    /*  ���ݽ��״��������⴦�� */
    if ((atoi(sTransCode)) != 20004)
    {
        if ( CheckXMLMac(xml,  'R') < 0 )
        {
            strncpy(g_errcode, "1042", sizeof(g_errcode)-1);
            strncpy(g_errmsg, "MACУ�����", sizeof(g_errmsg)-1);
            PrintLog(LEVEL_ERR, LOG_NOTIC_INFO, "MACУ�����");         
            ERRLOGF("MACУ�����");              
            return -1; 
        }
    }     
    
    pfix = FIXopen(sCommBuf, 0, 2048, sErrMsg);
    if ( pfix == NULL) 
    {
        strncpy(g_errcode, "1041", sizeof(g_errcode)-1);
        strncpy(g_errmsg, "��������ϵͳ����(����FIXʧ��)", sizeof(g_errmsg)-1);    	        
        PrintLog(LEVEL_ERR, LOG_NOTIC_INFO, "����FIXʧ��[%s]", sErrMsg);        
        return -1;
    }
    
    LookUp(104, sData, sTransCode);
    FIXputs(pfix, "FunctionId", sData);
    PrintLog(LEVEL_ERR, LOG_NOTIC_INFO, "������[%s],[%s]", sData,sTransCode);
    /* ���� */
    memset(sData, 0x00, sizeof(sData));
    xml_GetElement(xml, "/MsgText/GrpHdr/TradSrc",sData, sizeof(sData));
    if ( strcmp(sData, "F") != 0 )
    {
        FIXclose(pfix);
        strncpy(g_errcode, "1044", sizeof(g_errcode)-1);
        strncpy(g_errmsg, "ͨѶ��Ϣ���ʽ[TradSrc]��д����", sizeof(g_errmsg)-1);    	        
        PrintLog(LEVEL_ERR, LOG_NOTIC_INFO, "ͨѶ��Ϣ���ʽ[/MsgText/GrpHdr/TradSrc]��д����[%s]", sData); 
        return -1;
    }
/*    FIXprintf(pfix, "bOrganId", "%.1s", sData);*/
    
    /* ��������ˮ�� */
    memset(sData, 0x00, sizeof(sData));
    xml_GetElement(xml, "/MsgText/FtSeq",sData, sizeof(sData));
    FIXputs(pfix, "ExSerial", sData);
    
    /* �������� */
    memset(sData, 0x00, sizeof(sData));
    memset(sData1, 0x00, sizeof(sData1));
    xml_GetElement(xml, "/MsgText/GrpHdr/Recver/InstId",sData1, sizeof(sData1));
    if ( LookUp(102, sData, sData1) < 0 )
    {
        FIXclose(pfix);
        strncpy(g_errcode, "1038", sizeof(g_errcode)-1);
        strncpy(g_errmsg, "��Ч���б��", sizeof(g_errmsg)-1);    	        
        PrintLog(LEVEL_ERR, LOG_NOTIC_INFO, "��Ч���б��[%s],[%s]", sData1,sData); 
        return -1;
    }
/*    FIXprintf(pfix, "bOrganId", "%.1s", sData);*/
	strncpy(bOrganId, sData, FLD_BANKNO_LEN);
         
/*    FIXputs(pfix, "sOrganId", sSecuNo);*/
    
    /* ������Ӫҵ����� */
    memset(sData, 0x00, sizeof(sData));
    xml_GetElement(xml, "/MsgText/GrpHdr/Sender/BrchId",sData, sizeof(sData));
    strncpy(sSecuBranch, sData, DB_SBRNCH_LEN);
/*    FIXputs(pfix, "sBranchId", sData);*/
    
    /* �������� */
    memset(sData, 0x00, sizeof(sData));
    xml_GetElement(xml, "/MsgText/GrpHdr/Date",sData, sizeof(sData));
/*    FIXputs(pfix, "Date", sData);*/
	strncpy(Date, sData, 8);
     
    /* ����ʱ�� */
    memset(sData, 0x00, sizeof(sData));
    xml_GetElement(xml, "/MsgText/GrpHdr/Time",sData, sizeof(sData));
/*    FIXputs(pfix, "Time", sData);*/
	strncpy(Time, sData, 6);
         
    /* ժҪ */
    memset(sData, 0x00, sizeof(sData));
    xml_GetElement(xml, "/MsgText/Dgst",sData, sizeof(sData));
/*    FIXputs(pfix, "Dgst", sData);*/
    
    /* ҵ������(ֱ����д0) */    
/*    FIXputs(pfix, "BusinType", "0");*/
        
    /*  ���ݽ��״��������⴦�� */
    switch (atoi(sTransCode) )
    {
    case 20001: /* ǩ�� */
    case 20002: /* ǩ�� */
        /* �������������ֶ� */
        /*  ����  */
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
    case 20004: /* ��Կͬ�� */
        {
            /* ���ش���ֱ�ӹر�FIX��� */
            FIXclose(pfix);
            memset(sData, 0x00, sizeof(sData));
            if ( xml_GetElement(xml, "/MsgText/MacKey",sData, sizeof(sData)) < 0 )
            {
                PrintLog(LEVEL_ERR, LOG_NOTIC_INFO, "��ȡXML�ڵ�[MacKey]����[%s]", (char *)xml_LastStringError());      
        		strncpy(g_errcode, "1062", sizeof(g_errcode)-1);
        		strncpy(g_errmsg, "��ȡXML�ڵ�[MacKey]����", sizeof(g_errmsg)-1);    	        
                return -1;
            }
            else
            {
            	memset(g_InitMacKey, 0x00, sizeof(g_InitMacKey));
    			if ( INIscanf(sSecuNo, "MAC_KEY", "%s", g_InitMacKey) < 0 )
    			{
    			    PrintLog(LEVEL_ERR, LOG_NOTIC_INFO, "ȡMACKEY����[%s][MAC_KEY],[%s]", sSecuNo, (char *)strerror(errno));
    			    ERRLOGF("ȡ������Կ����[%s][MAC_KEY],[%s]", sSecuNo, (char *)strerror(errno));
    			    return -1;   
    			}            	
                /* ����MAC��Կ */
                if ( ModifyCfgItem(sSecuNo, "MAC_KEY", sData) < 0 )
                {
                    PrintLog(LEVEL_ERR, LOG_NOTIC_INFO, "������Կ[MacKey]����[%s]", (char *)strerror(errno));
        			strncpy(g_errcode, "1062", sizeof(g_errcode)-1);
        			strncpy(g_errmsg, "����XML��Կ[MacKey]����", sizeof(g_errmsg)-1);    	        
                }
                else
                {
        			strncpy(g_errcode, "0000", sizeof(g_errcode)-1);
        			strncpy(g_errmsg, "������Կ�ɹ�", sizeof(g_errmsg)-1);    	        
                }
                return -1;
            }
        }
        break;
    case 22003: /* ������ת�� */
    case 22004: /* ������ת�� */        
    case 22001: /* ����ת�� */
    case 22002: /* ����ת�� */             
    case 21005: /* ��������� */
        {
        	/*���б��*/
        	FIXputs(pfix, "bOrganId", bOrganId);
        	/*���̱��*/
        	FIXputs(pfix, "sOrganId", sSecuNo);
        	/*����Ӫҵ�����*/
        	FIXputs(pfix, "sBranchId", sSecuBranch); 
            /* �������� */
            memset(sData, 0x00, sizeof(sData));
            xml_GetElement(xml, "/MsgText/BkAcct/Pwd/Pwd",sData, sizeof(sData));
            FIXputs(pfix, "bCustPwd", sData);  
            /*ҵ��ժҪ*/ 
    		memset(sData, 0x00, sizeof(sData));
    		xml_GetElement(xml, "/MsgText/Dgst",sData, sizeof(sData));            
            FIXputs(pfix, "Summary", sData); 
            /*ҵ������*/
            FIXputs(pfix, "BusinType", "0");
            /*����*/
			FIXputs(pfix, "Date", Date); 
			/*ʱ��*/       
			FIXputs(pfix, "Time", Time);                 	 
            /* �ͻ����� */
            memset(sData, 0x00, sizeof(sData));
            xml_GetElement(xml, "/MsgText/Cust/Name",sData, sizeof(sData));
            FIXputs(pfix, "Name", sData);
            
            /* ֤������ */
            memset(sData, 0x00, sizeof(sData));
            xml_GetElement(xml, "/MsgText/Cust/CertType",sData, sizeof(sData));
            /* �������֤������ */
            if ( LookUp(101, sData, sData) < 0  && strcmp(sTransCode, "21005") != 0 )
            {
                FIXclose(pfix);
        		strncpy(g_errcode, "1058", sizeof(g_errcode)-1);
        		strncpy(g_errmsg, "֤�����Ͳ�֧��", sizeof(g_errmsg)-1);    	        
                PrintLog(LEVEL_ERR, LOG_NOTIC_INFO, "֤�����Ͳ�֧��[%s]", sData); 
                return -1;
            }
            FIXputs(pfix, "CardType", sData);
            
            /* ֤������ */
            memset(sData, 0x00, sizeof(sData));
            xml_GetElement(xml, "/MsgText/Cust/CertId",sData, sizeof(sData));           
            FIXputs(pfix, "Card", sData);
            
            /* �����ʺ� */
            memset(sData, 0x00, sizeof(sData));
            xml_GetElement(xml, "/MsgText/BkAcct/Id",sData, sizeof(sData));
            FIXputs(pfix, "bCustAcct", sData);
            
             /* �ʽ��ʺ� */
            memset(sData, 0x00, sizeof(sData));
            xml_GetElement(xml, "/MsgText/FtAcct/Id",sData, sizeof(sData));
            FIXputs(pfix, "sCustAcct", sData);
            
            /* ����(�������ת�˱���λ�ò�ͬ) */
            memset(sData, 0x00, sizeof(sData));
            memset(sData1, 0x00, sizeof(sData1));
            if ( strcmp(sTransCode, "21005") == 0 )
            {
                xml_GetElement(xml, "/MsgText/Ccy",sData, sizeof(sData));
                /*�����ϰ汾û�б����ֶε����*/
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
        		strncpy(g_errmsg, "���ֲ�֧��", sizeof(g_errmsg)-1);    	        
                PrintLog(LEVEL_ERR, LOG_NOTIC_INFO, "���ֲ�֧��[%s]", sData); 
                return -1;                
            }            
            FIXprintf(pfix, "MoneyType", "%.1s", sData1);
            
            /* ת�˽�����Ҫ����ֶ� */
            if ( strcmp(sTransCode, "21005") != 0 )
            {
                /* ת�˽�� */
                memset(sData, 0x00, sizeof(sData));
                xml_GetElement(xml, "/MsgText/TrfAmt",sData, sizeof(sData));                
                FIXprintf(pfix, "OccurBala", "%.2lf", (atof(sData) /100.00) );
            }
            
            /* ����������Ҫ�ύԭ������ˮ�� */
            if ( strcmp(sTransCode, "22003") == 0 || strcmp(sTransCode, "22004") == 0 )
            {
                /* ԭ��ˮ�� */
                memset(sData, 0x00, sizeof(sData));
                xml_GetElement(xml, "/MsgText/OFtSeq",sData, sizeof(sData));
                FIXputs(pfix, "CorrectSerial", sData);
            }                         
        }
        break;
    default:
        FIXclose(pfix);
        strncpy(g_errcode, "1033", sizeof(g_errcode)-1);
        strncpy(g_errmsg, "�ý��ײ�֧��", sizeof(g_errmsg)-1);    	        
        PrintLog(LEVEL_ERR, LOG_NOTIC_INFO, "�ý��ײ�֧��[%s]", sTransCode); 
        return -1;         
    }         
    
    PrintFIXLog( LEVEL_NORMAL, "�����ܿ�����", pfix);
    *lCommLen = FIXlen(pfix);
    FIXclose(pfix);
    return 0;    
}


/******************************************************************************
�������ơ�SecuConvFixToXml
�������ܡ÷����̷���FIX->XML����ת��
���������
         XML -- XML���
         sCommBuf -- FIX����
         lCommLen -- FIX���ĳ���
���������
         
         sSecuNo  -- �����̱��
�� �� ֵ��
          = 0  �ɹ�
          < 0  ʧ��  
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
        xml_SetElement(xml,"/MsgText/Rst/Info","��������ϵͳ����(����FIXʧ��)");
        PrintLog(LEVEL_ERR, LOG_NOTIC_INFO, "����FIXʧ��[%s]", sErrMsg);        
        return -1;
    }
    
    PrintFIXLog( LEVEL_NORMAL, "�յ��ܿ�Ӧ��", pfix);
    
    xml_GetElement(xml,"/MsgText/GrpHdr/BusCd",sTransCode, sizeof(sTransCode));
           
    /* ������Ϣ */
    memset(sData, 0x00, sizeof(sData));
    FIXgets(pfix, "ErrorInfo", sData, sizeof(sData));   
    xml_SetElement(xml,"/MsgText/Rst/Info", sData);
    
    /* ������ˮ�� */
    memset(sData, 0x00, sizeof(sData));
    FIXgets(pfix, "bSerial", sData, sizeof(sData));   
    xml_SetElement(xml,"/MsgText/BkSeq", sData);
    
    /* ������� */
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
    
    /* ���׳ɹ��Ҳ��������������˻������Ϣ */
    if ( strcmp( sData, "0000" ) == 0  && strcmp(sTransCode, "21005") == 0)
    {
        /* ��ȡ��� */
        memset(sData, 0x00, sizeof(sData));
        FIXgets(pfix, "bFetchBala", sData, sizeof(sData));   
        snprintf(sData, sizeof(sData), "%.0lf", atof(sData) * 100.00); /* �����С�� */        
        xml_SetElement(xml,"/MsgText/BkBal/FtcBal", sData);
        /* 20150915-01-�����������͵�ǰ���ȡֵ */
        /* ������� */
        xml_SetElement(xml,"/MsgText/BkBal/UseBal", sData);
        
        memset(sData, 0x00, sizeof(sData));
        FIXgets(pfix, "bEnableBala", sData, sizeof(sData));   
        snprintf(sData, sizeof(sData), "%.0lf", atof(sData) * 100.00); /* �����С�� */        

        
        /* ��ǰ��� */         
        xml_SetElement(xml,"/MsgText/BkBal/CurBal", sData);
        
        /* ���� */
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
�������ơ�SecuConvXmlResHead
�������ܡ����÷����̷����Ӧ��XML����ͷ��Ϣ
���������
         XML -- ����XML���
		 HXML_RES -- Ӧ��XML���
�� �� ֵ��
          = 0  �ɹ�
          < 0  ʧ��  
******************************************************************************/
long SecuConvXmlResHead(HXMLTREE xml,HXMLTREE xml_Res)
{
	char sData[256];
    char sTransCode[DB_TRSCDE_LEN + 1] = {0};
    /* �汾�� */
    memset(sData, 0x00, sizeof(sData));
    xml_GetElement(xml, "/MsgText/GrpHdr/Version",sData, sizeof(sData));
    xml_SetElement(xml_Res, "/MsgText/GrpHdr/Version",sData);  	

    /* Ӧ��ϵͳ���� */
    memset(sData, 0x00, sizeof(sData));
    xml_GetElement(xml, "/MsgText/GrpHdr/SysType",sData, sizeof(sData));
    xml_SetElement(xml_Res, "/MsgText/GrpHdr/SysType",sData);

    /* ҵ���ܺ� */
    memset(sData, 0x00, sizeof(sData));
    xml_GetElement(xml, "/MsgText/GrpHdr/BusCd",sData, sizeof(sData));
    xml_SetElement(xml_Res, "/MsgText/GrpHdr/BusCd",sData);
	memset(sTransCode, 0x00, sizeof(sTransCode));
	strncpy(sTransCode, sData, DB_TRSCDE_LEN);
    /* ���׷��� */
    memset(sData, 0x00, sizeof(sData));
    xml_GetElement(xml, "/MsgText/GrpHdr/TradSrc",sData, sizeof(sData));
    xml_SetElement(xml_Res, "/MsgText/GrpHdr/TradSrc",sData);

    /* ���ͻ��� */
    memset(sData, 0x00, sizeof(sData));
    xml_GetElement(xml, "/MsgText/GrpHdr/Sender/InstId",sData, sizeof(sData));
    xml_SetElement(xml_Res, "/MsgText/GrpHdr/Sender/InstId",sData);

    /* ���ջ��� */
    memset(sData, 0x00, sizeof(sData));
    xml_GetElement(xml, "/MsgText/GrpHdr/Recver/InstId",sData, sizeof(sData));
    xml_SetElement(xml_Res, "/MsgText/GrpHdr/Recver/InstId",sData);

    /* �������� */
    memset(sData, 0x00, sizeof(sData));
    xml_GetElement(xml, "/MsgText/GrpHdr/Date",sData, sizeof(sData));
    xml_SetElement(xml_Res, "/MsgText/GrpHdr/Date",sData);

    /* ����ʱ�� */
    memset(sData, 0x00, sizeof(sData));
    xml_GetElement(xml, "/MsgText/GrpHdr/Time",sData, sizeof(sData));
    xml_SetElement(xml_Res, "/MsgText/GrpHdr/Time",sData);

    xml_SetElement(xml_Res, "/MsgText/Rst/Code","");
    xml_SetElement(xml_Res, "/MsgText/Rst/Info","");
    	    
    switch(atoi(sTransCode))
    {
    case 20001: /* ǩ�� */
    case 20002: /* ǩ�� */
    case 20004: /* ��Կͬ�� */
    case 23001: /* �ļ�����֪ͨ */
    	memset(sData, 0x00, sizeof(sData));
    	xml_GetElement(xml, "/MsgText/FtSeq",sData, sizeof(sData));
    	xml_SetElement(xml_Res, "/MsgText/FtSeq",sData);
    	xml_SetElement(xml_Res, "/MsgText/BkSeq","");
    	break;
    case 21005: /*��ѯ�������*/    	
    case 22003: /* ������ת�� */
    case 22004: /* ������ת�� */        
    case 22001: /* ����ת�� */
    case 22002: /* ����ת�� */     	
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
�������ơ�GenerateXMLMac
�������ܡü���MACֵ
���������
         XML     -- XML���   
         cFlag   -- 'R'-������,A--Ӧ��     
���������
         sMac    -- ����MACֵ
�� �� ֵ��
          = 0  �ɹ�
          < 0  ʧ��  
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
        
    /* ���״��� */
    xml_GetElement(xml,"/MsgText/GrpHdr/BusCd",sTransCode, sizeof(sTransCode));    
    /* ���� */
    xml_GetElement(xml,"/MsgText/GrpHdr/TradSrc",sTradeSrc, sizeof(sTradeSrc)); 
    
    /* �����̱�� */
    if ( strcmp(sTradeSrc, "B" ) == 0 ) /* ���з��� */
    {
        xml_GetElement(xml,"/MsgText/GrpHdr/Recver/InstId",sSecuNo, sizeof(sSecuNo));
    }
    else
    {
        xml_GetElement(xml,"/MsgText/GrpHdr/Sender/InstId",sSecuNo, sizeof(sSecuNo));
    }
            
    /* ���� */
    if ( cFlag == 'R' )
    {
        /* �������� */
        /* ���:ҵ���ܺ�|���׷���|���ͻ���������ʶ|���ջ���������ʶ|�������� */
        /* ���ܺ� */
        strcpy(sMacBuf, sTransCode);
        
        /* ���� */
        sprintf(sMacBuf,"%s|%s", sMacBuf, sTradeSrc);
        
        /* ���ͻ��� */
        memset(sData, 0x00, sizeof(sData));
        xml_GetElement(xml,"/MsgText/GrpHdr/Sender/InstId",sData, sizeof(sData));
        sprintf(sMacBuf,"%s|%s", sMacBuf, sData);
        
        /* ���ջ��� */
        memset(sData, 0x00, sizeof(sData));
        xml_GetElement(xml,"/MsgText/GrpHdr/Recver/InstId",sData, sizeof(sData));
        sprintf(sMacBuf,"%s|%s", sMacBuf, sData);
        
        /* �������� */
        memset(sData, 0x00, sizeof(sData));
        xml_GetElement(xml,"/MsgText/GrpHdr/Date",sData, sizeof(sData));
        sprintf(sMacBuf,"%s|%s", sMacBuf, sData);
        
        /* ��ˮ��(���з���Ϊ������ˮ��)/�ڻ�����Ϊ�ڻ���ˮ��) */
        if ( strcmp(sTradeSrc, "B") == 0 )
        {
            /* ������ˮ�� */
            memset(sData, 0x00, sizeof(sData));
            xml_GetElement(xml,"/MsgText/BkSeq",sData, sizeof(sData));
            sprintf(sMacBuf,"%s|%s", sMacBuf, sData);
        }
        else
        {
            /* �ڻ���ˮ�� */
            memset(sData, 0x00, sizeof(sData));
            xml_GetElement(xml,"/MsgText/FtSeq",sData, sizeof(sData));
            sprintf(sMacBuf,"%s|%s", sMacBuf, sData);
        }
                            
        switch(atoi(sTransCode))
        {
        case 20001: /* ǩ�� */
        case 20002: /* ǩ�� */
        case 23001: /* �ļ����� */
            {
            /*��ɣ��������� */            
            }
            break;
        case 20004: /* ��Կͬ�� */
            {
            /*��ɣ���������|MacKey */                        
            /* MacKey */
            memset(sData, 0x00, sizeof(sData));
            xml_GetElement(xml,"/MsgText/MacKey",sData, sizeof(sData));
            sprintf(sMacBuf,"%s|%s", sMacBuf, sData);            
            }
            break;
        case 21001: /* ǩԼ */
            {
            /* ��ɣ���������|�ͻ�����|֤������|֤������|�����˻��˺�|�ڻ���˾��֤���˺�|�������� */                               
            /* �ͻ����� */
            memset(sData, 0x00, sizeof(sData));
            xml_GetElement(xml,"/MsgText/Cust/Name",sData, sizeof(sData));
            sprintf(sMacBuf,"%s|%s", sMacBuf, sData);
            
            /* ֤������ */
            memset(sData, 0x00, sizeof(sData));
            xml_GetElement(xml,"/MsgText/Cust/CertType",sData, sizeof(sData));
            sprintf(sMacBuf,"%s|%s", sMacBuf, sData);
            
            /* ֤������ */
            memset(sData, 0x00, sizeof(sData));
            xml_GetElement(xml,"/MsgText/Cust/CertId",sData, sizeof(sData));
            sprintf(sMacBuf,"%s|%s", sMacBuf, sData);
            
            /* ���п��� */
            memset(sData, 0x00, sizeof(sData));
            xml_GetElement(xml,"/MsgText/BkAcct/Id",sData, sizeof(sData));
            sprintf(sMacBuf,"%s|%s", sMacBuf, sData);
            
            /* ��֤���ʺ� */
            memset(sData, 0x00, sizeof(sData));
            xml_GetElement(xml,"/MsgText/FtAcct/Id",sData, sizeof(sData));
            sprintf(sMacBuf,"%s|%s", sMacBuf, sData);
            
            /* ���� */
            memset(sData, 0x00, sizeof(sData));
            xml_GetElement(xml,"/MsgText/Ccy",sData, sizeof(sData));
            sprintf(sMacBuf,"%s|%s", sMacBuf, sData);
            
            }
            break;
        case 21002: /* ��Լ */
        case 21004: /* �鱣֤����� */
        case 21005: /* ��������� */
            {
            /* ��ɣ���������|�ͻ�����|�����˻��˺�|�ڻ���˾��֤���˺� */                                   
            /* �ͻ����� */
            memset(sData, 0x00, sizeof(sData));
            xml_GetElement(xml,"/MsgText/Cust/Name",sData, sizeof(sData));
            sprintf(sMacBuf,"%s|%s", sMacBuf, sData);
                        
            /* ���п��� */
            memset(sData, 0x00, sizeof(sData));
            xml_GetElement(xml,"/MsgText/BkAcct/Id",sData, sizeof(sData));
            sprintf(sMacBuf,"%s|%s", sMacBuf, sData);
            
            /* ��֤���ʺ� */
            memset(sData, 0x00, sizeof(sData));
            xml_GetElement(xml,"/MsgText/FtAcct/Id",sData, sizeof(sData));
            sprintf(sMacBuf,"%s|%s", sMacBuf, sData);                                    
            }
            break;
        case 21003: /* ������п��� */
            {
            /* ��ɣ���������|�ͻ�����|֤������|֤������|�����˻��˺�|�������˻��˺�|�ڻ���˾��֤���˺�|�������� */                      
            /* �ͻ����� */
            memset(sData, 0x00, sizeof(sData));
            xml_GetElement(xml,"/MsgText/Cust/Name",sData, sizeof(sData));
            sprintf(sMacBuf,"%s|%s", sMacBuf, sData);
            
            /* ֤������ */
            memset(sData, 0x00, sizeof(sData));
            xml_GetElement(xml,"/MsgText/Cust/CertType",sData, sizeof(sData));
            sprintf(sMacBuf,"%s|%s", sMacBuf, sData);
            
            /* ֤������ */
            memset(sData, 0x00, sizeof(sData));
            xml_GetElement(xml,"/MsgText/Cust/CertId",sData, sizeof(sData));
            sprintf(sMacBuf,"%s|%s", sMacBuf, sData);
            
            /* ���п��� */
            memset(sData, 0x00, sizeof(sData));
            xml_GetElement(xml,"/MsgText/BkAcct/Id",sData, sizeof(sData));
            sprintf(sMacBuf,"%s|%s", sMacBuf, sData);
            
            /* �����п��� */
            memset(sData, 0x00, sizeof(sData));
            xml_GetElement(xml,"/MsgText/NewBkAcct/Id",sData, sizeof(sData));
            sprintf(sMacBuf,"%s|%s", sMacBuf, sData);
            
            /* ��֤���ʺ� */
            memset(sData, 0x00, sizeof(sData));
            xml_GetElement(xml,"/MsgText/FtAcct/Id",sData, sizeof(sData));
            sprintf(sMacBuf,"%s|%s", sMacBuf, sData);
            
            /* ���� */
            memset(sData, 0x00, sizeof(sData));
            xml_GetElement(xml,"/MsgText/Ccy",sData, sizeof(sData));
            sprintf(sMacBuf,"%s|%s", sMacBuf, sData);
            
            }
            break;
        case 22001: /* ����ת�� */
        case 22002: /* ����ת�� */
        case 22003: /* ������ת�� */
        case 22004: /* ������ת�� */            
            {
            /* ��ɣ���������|{����������ʹ�á�ԭ������ˮ��|ԭ�ڻ���ˮ��}|�Ƿ��ط���־|�ͻ�����|�����˻��˺�|�ڻ���˾��֤���˺�|ת�ʽ��|Ӧ�տͻ�������|Ӧ���ڻ���˾������ */                            
            
            if ( strcmp(sTransCode, "22003") == 0 || strcmp(sTransCode, "22004") == 0 )
            {
		                
                /* ԭ������ˮ�� */
                memset(sData, 0x00, sizeof(sData));
                xml_GetElement(xml,"/MsgText/OBkSeq",sData, sizeof(sData));
                sprintf(sMacBuf,"%s|%s", sMacBuf, sData);

                /* ԭ��������ˮ�� */
                memset(sData, 0x00, sizeof(sData));
                xml_GetElement(xml,"/MsgText/OFtSeq",sData, sizeof(sData));
                sprintf(sMacBuf,"%s|%s", sMacBuf, sData);
                
            }
            
            /* �Ƿ��ط���־ */
            memset(sData, 0x00, sizeof(sData));
            xml_GetElement(xml,"/MsgText/Resend",sData, sizeof(sData));
            sprintf(sMacBuf,"%s|%s", sMacBuf, sData);
            
            /* �ͻ����� */
            memset(sData, 0x00, sizeof(sData));
            xml_GetElement(xml,"/MsgText/Cust/Name",sData, sizeof(sData));
            sprintf(sMacBuf,"%s|%s", sMacBuf, sData);
                                    
            /* ���п��� */
            memset(sData, 0x00, sizeof(sData));
            xml_GetElement(xml,"/MsgText/BkAcct/Id",sData, sizeof(sData));
            sprintf(sMacBuf,"%s|%s", sMacBuf, sData);
            
            /* ��֤���ʺ� */
            memset(sData, 0x00, sizeof(sData));
            xml_GetElement(xml,"/MsgText/FtAcct/Id",sData, sizeof(sData));
            sprintf(sMacBuf,"%s|%s", sMacBuf, sData);
            
            /* ת�˽�� */
            memset(sData, 0x00, sizeof(sData));
            xml_GetElement(xml,"/MsgText/TrfAmt",sData, sizeof(sData));
            sprintf(sMacBuf,"%s|%s", sMacBuf, sData);
            
            /* �ͻ������� */
            memset(sData, 0x00, sizeof(sData));
            xml_GetElement(xml,"/MsgText/CustCharge",sData, sizeof(sData));
            sprintf(sMacBuf,"%s|%s", sMacBuf, sData);
            
            /* ������������ */
            memset(sData, 0x00, sizeof(sData));
            xml_GetElement(xml,"/MsgText/FutureCharge",sData, sizeof(sData));
            sprintf(sMacBuf,"%s|%s", sMacBuf, sData);            
            }
            break;
        }        
    }
    else /* Ӧ�� */
    {
        /* �������� */
        /* ���:ҵ���ܺ�|���׷���|���ͻ���������ʶ|���ջ���������ʶ|��������|���ش���|������ˮ�� */
        /* ���ܺ� */
        strcpy(sMacBuf, sTransCode);
        
        /* ���� */
        sprintf(sMacBuf,"%s|%s", sMacBuf, sTradeSrc);
        
        /* ���ͻ��� */
        memset(sData, 0x00, sizeof(sData));
        xml_GetElement(xml,"/MsgText/GrpHdr/Sender/InstId",sData, sizeof(sData));
        sprintf(sMacBuf,"%s|%s", sMacBuf, sData);
        
        /* ���ջ��� */
        memset(sData, 0x00, sizeof(sData));
        xml_GetElement(xml,"/MsgText/GrpHdr/Recver/InstId",sData, sizeof(sData));
        sprintf(sMacBuf,"%s|%s", sMacBuf, sData);
        
        /* �������� */
        memset(sData, 0x00, sizeof(sData));
        xml_GetElement(xml,"/MsgText/GrpHdr/Date",sData, sizeof(sData));
        sprintf(sMacBuf,"%s|%s", sMacBuf, sData);
        
        /* ���ش��� */
        memset(sData, 0x00, sizeof(sData));
        xml_GetElement(xml,"/MsgText/Rst/Code",sData, sizeof(sData));
        sprintf(sMacBuf,"%s|%s", sMacBuf, sData);
        
        /* ������ˮ�� */
        memset(sData, 0x00, sizeof(sData));
        xml_GetElement(xml,"/MsgText/BkSeq",sData, sizeof(sData));
        sprintf(sMacBuf,"%s|%s", sMacBuf, sData);
        
        switch(atoi(sTransCode))
        {
        case 20001: /* ǩ�� */
        case 20002: /* ǩ�� */
        case 20004: /* ��Կͬ�� */
        case 23001: /* �ļ�����֪ͨ */
            {
            /*��ɣ��������� + �ڻ���ˮ�� */
            /* �ڻ���ˮ�� */
            memset(sData, 0x00, sizeof(sData));
            xml_GetElement(xml,"/MsgText/FtSeq",sData, sizeof(sData));
            sprintf(sMacBuf,"%s|%s", sMacBuf, sData);
            }
            break;
        
        case 21001: /* ǩԼ */
        case 21002: /* ��Լ */
        case 21003: /* ��������ʺ� */
            {
            /* ��ɣ���������|�����˻��˺�|�ڻ���˾��֤���˺�|�ڻ���˾��ˮ��| */
                        
            /* ���п��� */
            memset(sData, 0x00, sizeof(sData));
            xml_GetElement(xml,"/MsgText/BkAcct/Id",sData, sizeof(sData));
            sprintf(sMacBuf,"%s|%s", sMacBuf, sData);
            
            /* ��֤���ʺ� */
            memset(sData, 0x00, sizeof(sData));
            xml_GetElement(xml,"/MsgText/FtAcct/Id",sData, sizeof(sData));
            sprintf(sMacBuf,"%s|%s", sMacBuf, sData);
                        
            /* �ڻ���ˮ�� */
            memset(sData, 0x00, sizeof(sData));
            xml_GetElement(xml,"/MsgText/FtSeq",sData, sizeof(sData));
            sprintf(sMacBuf,"%s|%s", sMacBuf, sData);
            }
            break; 
        case 21004: /* �鱣֤����� */        
            {
            /* 
              ��ɣ���������|�ڻ���˾��ˮ��|�ڻ���˾��֤���˺�|
              �ڻ���˾��֤���˻���ǰ���|�ڻ���˾��֤���˻��������|�ڻ���˾��֤���˻���ȡ���
            */
            /* �ڻ���ˮ�� */
            memset(sData, 0x00, sizeof(sData));
            xml_GetElement(xml,"/MsgText/FtSeq",sData, sizeof(sData));
            sprintf(sMacBuf,"%s|%s", sMacBuf, sData);
                                               
            /* ��֤���ʺ� */
            memset(sData, 0x00, sizeof(sData));
            xml_GetElement(xml,"/MsgText/FtAcct/Id",sData, sizeof(sData));
            sprintf(sMacBuf,"%s|%s", sMacBuf, sData);
            
            /* ��ǰ��� */
            memset(sData, 0x00, sizeof(sData));
            xml_GetElement(xml,"/MsgText/FtBal/CurBal",sData, sizeof(sData));
            sprintf(sMacBuf,"%s|%s", sMacBuf, sData);
            
            /* ������� */
            memset(sData, 0x00, sizeof(sData));
            xml_GetElement(xml,"/MsgText/FtBal/UseBal",sData, sizeof(sData));
            sprintf(sMacBuf,"%s|%s", sMacBuf, sData); 
            
            /* ��ȡ��� */
            memset(sData, 0x00, sizeof(sData));
            xml_GetElement(xml,"/MsgText/FtBal/FtcBal",sData, sizeof(sData));
            sprintf(sMacBuf,"%s|%s", sMacBuf, sData);                                                           
            }
            break;
            
        case 21005: /* ��������� */
            {
            /* 
              ��ɣ���������|�ڻ���˾��ˮ��|�����˺�|�˻���ǰ���
            */
            /* �ڻ���ˮ�� */
            memset(sData, 0x00, sizeof(sData));
            xml_GetElement(xml,"/MsgText/FtSeq",sData, sizeof(sData));
            sprintf(sMacBuf,"%s|%s", sMacBuf, sData);
                        
            /* ���п��� */
            memset(sData, 0x00, sizeof(sData));
            xml_GetElement(xml,"/MsgText/BkAcct/Id",sData, sizeof(sData));
            sprintf(sMacBuf,"%s|%s", sMacBuf, sData);
                                                
            /* ���е�ǰ�˻���� */
            memset(sData, 0x00, sizeof(sData));
            xml_GetElement(xml,"/MsgText/BkBal/CurBal",sData, sizeof(sData));
            sprintf(sMacBuf,"%s|%s", sMacBuf, sData);
            }
            break; 
        case 22001: /* ����ת�� */
        case 22002: /* ����ת�� */
        case 22003: /* ������ת�� */
        case 22004: /* ������ת�� */
            {
            /* ��ɣ���������|�ڻ���˾��ˮ��|�����˻��˺�|�ڻ���˾��֤���˺�|ת�ʽ��|Ӧ�տͻ�������|Ӧ���ڻ���˾������ */
            /* �ڻ���ˮ�� */
            memset(sData, 0x00, sizeof(sData));
            xml_GetElement(xml,"/MsgText/FtSeq",sData, sizeof(sData));
            sprintf(sMacBuf,"%s|%s", sMacBuf, sData);
                        
            /* ���п��� */
            memset(sData, 0x00, sizeof(sData));
            xml_GetElement(xml,"/MsgText/BkAcct/Id",sData, sizeof(sData));
            sprintf(sMacBuf,"%s|%s", sMacBuf, sData);
            
            /* ��֤���ʺ� */
            memset(sData, 0x00, sizeof(sData));
            xml_GetElement(xml,"/MsgText/FtAcct/Id",sData, sizeof(sData));
            sprintf(sMacBuf,"%s|%s", sMacBuf, sData);
            
            /* ת�˽�� */
            memset(sData, 0x00, sizeof(sData));
            xml_GetElement(xml,"/MsgText/TrfAmt",sData, sizeof(sData));
            sprintf(sMacBuf,"%s|%s", sMacBuf, sData);
            
            /* �ͻ������� */
            memset(sData, 0x00, sizeof(sData));
            xml_GetElement(xml,"/MsgText/CustCharge",sData, sizeof(sData));
            sprintf(sMacBuf,"%s|%s", sMacBuf, sData);
            
            /* ������������ */
            memset(sData, 0x00, sizeof(sData));
            xml_GetElement(xml,"/MsgText/FutureCharge",sData, sizeof(sData));
            sprintf(sMacBuf,"%s|%s", sMacBuf, sData);             
            
            }
            break;             
        }
    }
    
    PrintLog( LEVEL_NORMAL, LOG_NOTIC_INFO, "���״���[%s]����[%s],����/Ӧ��[%c]MACBUF[%s]", sTransCode, sTradeSrc, cFlag, sMacBuf); 
/*
	if(memcmp(sTransCode, "20004", 5) == 0 && cFlag == 'A')
	{
		memset(sMacKey, 0x00, sizeof(sMacKey));
		strncpy(sMacKey, g_InitMacKey, sizeof(sMacKey)-1);
	}
	else
*/	{     
    	/* ȡMACKEY */
    	lRet = GetXmlMacKey(sSecuNo, sMacKey);
    	if ( lRet < 0 )
    	{
        	PrintLog( LEVEL_ERR, LOG_NOTIC_INFO, "ȡMACKEYʧ��[%ld]", lRet);
        	return -1;
    	}
    }
    
    /* ����MAC */
    GenMAC(sMacBuf, sMac, sMacKey);
                 
    return 0;
}


/******************************************************************************
�������ơ�UpdateXMLMacAndExport
�������ܡ����¼���MAC������XML���� 
���������
         XML     -- XML���   
         cFlag   -- 'R'-������,A--Ӧ��     
���������
         sCommBuf -- XMLͨѶ���� 
         lCommLen -- XML���ĳ���
�� �� ֵ��
          = 0  �ɹ�
          < 0  ʧ��  
******************************************************************************/
long UpdateXMLMacAndExport(HXMLTREE xml, char cFlag, char *sCommBuf, long *lCommLen)
{
    char sMac[16 + 1] = {0};
    long lRet;
    
    PrintLog(LEVEL_DEBUG, LOG_DEBUG_INFO, "%s() Entry", __FUNCTION__);
    
    /* ����MAC */
    lRet = GenerateXMLMac(xml, cFlag, sMac);
    if ( lRet < 0 )
    {
        return -1;   
    }
    PrintLog(LEVEL_DEBUG, LOG_DEBUG_INFO, "sMac[%s]", sMac); 
    /* ����MAC */
    if ( xml_SetElement(xml,"/MsgText/Mac", sMac) < 0 )
    {
        PrintLog(LEVEL_ERR, LOG_NOTIC_INFO, "����MACֵʧ��[%s]", (char *)xml_LastStringError());
        ERRLOGF("����MACֵʧ��[%s]", (char *)xml_LastStringError());
        return -2;
    } 
    
    *lCommLen = xml_ExportXMLStringEx(xml,sCommBuf, 2048,"/MsgText",1,0x00000001,NULL); 
    if ( *lCommLen < 0) 
    {
        PrintLog(LEVEL_ERR, LOG_NOTIC_INFO, "����XMLBUFʧ��[%s]", (char *)xml_LastStringError());
        ERRLOGF("����XMLBUFʧ��[%s]", (char *)xml_LastStringError());
        return -1;
    }
    PrintLog(LEVEL_DEBUG, LOG_DEBUG_INFO, "%s() Exit", __FUNCTION__);
    return 0;
}

/******************************************************************************
�������ơ�SecuConvXmlExport
�������ܡ÷����̷���Ľ��״���ʧ��ʱ������XML���� 
���������
         XML     -- XML���       
���������
         sCommBuf -- XMLͨѶ���� 
         lCommLen -- XML���ĳ���
�� �� ֵ��
          = 0  �ɹ�
          < 0  ʧ��  
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
	/*��Կͬ���ɹ��Ļ���Ҫ����MAC*/
/*	if(memcmp(sTransCode, "20004", 5) == 0 && memcmp(sErrCode, "0000", 4) == 0)
	{    
*/    	/* ����MAC */
    	lRet = GenerateXMLMac(xml, cFlag, sMac);
    	if ( lRet < 0 )
    	{
    	    return -1;   
    	}
    	PrintLog(LEVEL_DEBUG, LOG_DEBUG_INFO, "sMac[%s]", sMac); 
    	/* ����MAC */
    	if ( xml_SetElement(xml,"/MsgText/Mac", sMac) < 0 )
    	{
    	    PrintLog(LEVEL_ERR, LOG_NOTIC_INFO, "����MACֵʧ��[%s]", (char *)xml_LastStringError());
    	    ERRLOGF("����MACֵʧ��[%s]", (char *)xml_LastStringError());
    	    return -2;
    	}
/*    } */
    
    *lCommLen = xml_ExportXMLStringEx(xml,sCommBuf, 2048,"/MsgText",1,0x00000001,NULL); 
    if ( *lCommLen < 0) 
    {
        PrintLog(LEVEL_ERR, LOG_NOTIC_INFO, "����XMLBUFʧ��[%s]", (char *)xml_LastStringError());
        ERRLOGF("����XMLBUFʧ��[%s]", (char *)xml_LastStringError());
        return -1;
    }
    PrintLog(LEVEL_DEBUG, LOG_DEBUG_INFO, "%s() Exit", __FUNCTION__);
    return 0;	
}


/******************************************************************************
�������ơ�GetXmlMacKey
�������ܡû�ȡMACKEY��Կ
���������
        sSecuNo:�����̱��     
���������
        ȡ��Կ         
�� �� ֵ��
          = 0  �ɹ�
          < 0  ʧ��  
******************************************************************************/
long GetXmlMacKey(char *sSecuNo, char *sMacKey)
{            
    if ( strlen(sSecuNo) == 0 )
    {
        PrintLog(LEVEL_ERR, LOG_NOTIC_INFO, "ȡMACKEY����������������̱��Ϊ��[%s]", sSecuNo);
        ERRLOGF("ȡMACKEY����������������̱��Ϊ��[%s]", sSecuNo);
        return -1;  
    }
    
    if ( INIscanf(sSecuNo, "MAC_KEY", "%s", sMacKey) < 0 )
    {
        PrintLog(LEVEL_ERR, LOG_NOTIC_INFO, "ȡMACKEY����[%s][MAC_KEY],[%s]", sSecuNo, (char *)strerror(errno));
        ERRLOGF("ȡ������Կ����[%s][MAC_KEY],[%s]", sSecuNo, (char *)strerror(errno));
        return -1;   
    }
       
    PrintLog(LEVEL_DEBUG, LOG_NOTIC_INFO, "MACKEY[%s]",  sMacKey);
    
    return 0;   
}


/******************************************************************************
�������ơ�CheckXMLMac
�������ܡ�У��MAC
���������
         XML     -- XML���   
         cFlag   -- 'R'-������,A--Ӧ��     
���������
         ��         
�� �� ֵ��
          = 0  �ɹ�
          < 0  ʧ��  
******************************************************************************/
long CheckXMLMac(HXMLTREE xml, char cFlag)
{
    char sMac[16 + 1] = {0};     /* ������ */
    char sMacTmp[16 + 1] = {0};  /* ��ʱ���� */
    int  iCheckFlag = 1;         /* �Ƿ���֤MAC */
    long lRet;    
    char sTradeSrc[2] = {0};
    char sSecuNo[DB_SECUNO_LEN + 1] = {0};
    char sTransCode[DB_TRSCDE_LEN + 1] = {0};
    
    
	/*������*/
    xml_GetElement(xml,"/MsgText/GrpHdr/BusCd",sTransCode, sizeof(sTransCode)); 
	    
    /* ���� */
    xml_GetElement(xml,"/MsgText/GrpHdr/TradSrc",sTradeSrc, sizeof(sTradeSrc)); 
    
    /* �����̱�� */
    if ( strcmp(sTradeSrc, "B" ) == 0 ) /* ���з��� */
    {
        xml_GetElement(xml,"/MsgText/GrpHdr/Recver/InstId",sSecuNo, sizeof(sSecuNo));
    }
    else
    {
        xml_GetElement(xml,"/MsgText/GrpHdr/Sender/InstId",sSecuNo, sizeof(sSecuNo));
    }
    
    if ( strlen (sSecuNo) == 0 )
    {
        PrintLog( LEVEL_NORMAL, LOG_NOTIC_INFO, "�����̱��Ϊ��"); 
        return -1;
    }
         
    /* Ӧ������ݲ���У��MAC */
    if ( cFlag == 'A' )
    {
        INIscanf( sSecuNo, "CHECK_ANS_MAC", "%d", &iCheckFlag);
        if ( iCheckFlag == 0 )
        {
            PrintLog( LEVEL_NORMAL, LOG_NOTIC_INFO, "����[%s][CHECK_ANS_MAC]����Ϊ��У�飬Ӧ�����У��MAC", sSecuNo);
            return 0;   
        }
    }
    else
    {
        INIscanf( sSecuNo, "CHECK_REQ_MAC", "%d", &iCheckFlag);
        if ( iCheckFlag == 0 )
        {
            PrintLog( LEVEL_NORMAL, LOG_NOTIC_INFO, "����[%s][CHECK_REQ_MAC]����Ϊ��У�飬�������У��MAC", sSecuNo);
            return 0;   
        }         
    }
        
    /* ��MAC */
    /*��Կͬ����Ҫȡ�����ļ���ĳ�ʼ��Կ*/

    if ( xml_GetElement(xml,"/MsgText/Mac",sMac, sizeof(sMac)) < 0 )
    {
        PrintLog( LEVEL_NORMAL, LOG_NOTIC_INFO, "ȡ�ڵ�[/MsgText/Mac]ʧ�ܣ�[%s]",  (char *)xml_LastStringError());
        return -1;
    }
    
    /* ����MAC */
    lRet = GenerateXMLMac(xml, cFlag, sMacTmp);
    if ( lRet < 0 )
    {
        return -2;   
    }
    
    /*  �Ƚ�MAC */
    if ( strcmp(sMacTmp, sMac) != 0 )
    {
        PrintLog( LEVEL_NORMAL, LOG_NOTIC_INFO, "MACУ�鲻ͨ��,��MAC[%s]����MAC[%s]", sMac, sMacTmp);
        return -3;
    }
    return 0;
}
    
