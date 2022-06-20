#pragma once
#include  "linuxinclude.h"
#include  "commontype.h"
#include "command.h"
#include "commandFactory.h"
#include "ossSocket.h"
#include <iostream>
using namespace std;
const int CMD_BUFFER_SIZE = 512;


class EDB
{
public:
    EDB()
    {

    }
    ~EDB()
    {

    }
    void start(void);
protected:
    void prompt(void);
private:
   void     split(const std::string &text, char delim, std::vector<std::string> &result);
   char*    readLine(char *p, int length);
   int      readInput(const char *pPrompt, int numIndent);
private:
    ossSocket m_socket;
    CommandFactory m_cmdFactory;
    char m_cmdBuffer[CMD_BUFFER_SIZE];
};