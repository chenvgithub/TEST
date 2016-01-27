/* ����һ������. ����TCP/IPΪ������, �����޷��������ݷֽ�, �����ǲ���4������
 * �ַ�, �Է���Ӧ�ó�����.
 * byte0-1=ͬ���ַ�, byte2-3=��������(������4�������ַ�)
 *      buffer ....... �����͵Ļ�����ָ��;
 *      len .......... ���峤��;
 *      maxtime ...... ��ʱֵ(��), 0-��Զ�ȴ�,ֱ��������ϻ�socket���ر�.
 * ����: 0-�ɹ�, -1-socket���ر�, -3-��ʱ -4-�����ڴ�ʧ��.
 */
int itcp_send(char *buffer, int len, int maxtime)
{
    int rc, sock, flen, sendlen;
    char flagbuf[MAX_SYNCCHAR + sizeof(unsigned short)], *p, *fp;
#ifndef M_XENIX
    long t0;
#endif
        
    sock = *o_newsock;
    if (sock == -1)
    return -1;
    if (len <= 0)
    return 0;

#ifdef M_XENIX
    /* �������� */
    my_setalarm(maxtime);
#else
    /* ��ʼ��ʱ */
    if (maxtime != 0)
    t0 = time(NULL);
#endif

    if ((fp = (char *)malloc(sync_buflen+sizeof(unsigned short)+len)) == NULL)
    {
       return -4;
    }
    
    p = fp;
    if (_nohead == 0) /*�����ݰ�ͷ(ͬ���ַ������ݰ�����)*/
    {
        /* ���ȷ��Ϳ���ͷ(sync_buflen+2���ֽ�) */
        memcpy(flagbuf, sync_buffer, sync_buflen);
        if (_use_ns == 1)
            *(unsigned short *)(flagbuf + sync_buflen) = htons(len);
        else
            *(unsigned short *)(flagbuf + sync_buflen) = len;
        flen = sync_buflen + sizeof(unsigned short);
        memcpy(p, flagbuf, flen);
        memcpy(p+flen, buffer, len);
        sendlen = flen + len;
    }
    else
    {
        memcpy(p, buffer, len);
        sendlen = len;
    }
    
    /* ������������, ͬ�� */
    do
    {
        rc = write(sock, p, sendlen);   
        if (rc <= 0)
        {
        #ifdef M_XENIX
            if (maxtime > 0)
            {
               alarm(0);
            }            
            if (rc == -1 && o_alarm == 1)
            {
                free(fp);
                return -3;
            }
        #else
            if (rc == -1 && errno == EWOULDBLOCK)
            {
                if (s2hooking != NULL)
                {
                    rc = s2hooking(11);
                    if (rc != 0)
                    {
                        free(fp);
                        return rc;
                    }
                }
                if (maxtime == 0 || time(NULL) - t0 <= maxtime)
                {
                    continue;
                    free(fp);
                }
                return -3;
            }
        #endif
            itcp_clear();
            free(fp);
            return -1;
        }
        
        p += rc;
        sendlen -= rc;
    }
    while (sendlen > 0);

#ifdef M_XENIX
    if (maxtime > 0)
    {
        alarm(0);
    }
#endif
    free(fp);
    return 0;
}
