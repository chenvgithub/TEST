/* 发送一包数据. 由于TCP/IP为数据流, 本身无法进行数据分界, 故我们插入4个定界
 * 字符, 以方便应用程序处理.
 * byte0-1=同步字符, byte2-3=本包包长(不包括4个定界字符)
 *      buffer ....... 欲发送的缓冲首指针;
 *      len .......... 缓冲长度;
 *      maxtime ...... 超时值(秒), 0-永远等待,直到发送完毕或socket被关闭.
 * 返回: 0-成功, -1-socket被关闭, -3-超时 -4-分配内存失败.
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
    /* 设置闹钟 */
    my_setalarm(maxtime);
#else
    /* 开始记时 */
    if (maxtime != 0)
    t0 = time(NULL);
#endif

    if ((fp = (char *)malloc(sync_buflen+sizeof(unsigned short)+len)) == NULL)
    {
       return -4;
    }
    
    p = fp;
    if (_nohead == 0) /*有数据包头(同步字符和数据包长度)*/
    {
        /* 首先发送控制头(sync_buflen+2个字节) */
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
    
    /* 发送正文数据, 同上 */
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
