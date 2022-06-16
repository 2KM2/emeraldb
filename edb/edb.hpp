#pragma once
#include "core.hpp"
#include "command.hpp"
#include "ossSocket.hpp"
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
   void    split(const std::string &text, char delim, std::vector<std::string> &result);
   char*    readLine(char *p, int length);
   int      readInput(const char *pPrompt, int numIndent);
private:
    ossSocket m_socket;
    char m_cmdBuffer[CMD_BUFFER_SIZE];
}