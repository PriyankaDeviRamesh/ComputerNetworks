#ifndef CONFUTILS_H
#define CONFUTILS_H

int startProxyServer();
int hookToServer(char *servhost, unsigned short servport);

int readBytes(int, char*, int);
#endif
