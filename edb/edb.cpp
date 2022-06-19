#include "core.hpp"
#include "edb.hpp"
const char    SPACE             =  ' ';
const char    TAB               =  '\t';
const char    BACK_SLANT        =  '\\';
const char    NEW_LINE          =   '\n';
int g_Quit = 0;
void EDB::start(void)
{
    std::cout << "Welcome to EmeraldDB Shell!" << std::endl;
   std::cout << "edb help for help, Ctrl+c or quit to exit" << std::endl;
   while(0==g_Quit)
   {
      prompt();
   }
}

void EDB::prompt(void)
{
    int ret = EDB_OK;
    ret = readInput("edb", 0);

    if(ret)
    {
        return;
    }

    //Input string
    std::string textInput = m_cmdBuffer;
    std::vector<std::string> textVec;
    split(textInput,SPACE,textVec);

    int count = 0;
    std::string cmd = "";
    std::vector<std::string> optionVec;
    std::vector<std::string>::iterator iter = textVec.begin();

    //handle different command here

    ICommand * pCmd = NULL;
    for(;iter!=textVec.end();++iter)
    {
        std::string  str = *iter;
        if(0==count)
        {
            cmd = str;
            count++;
        }else {
            optionVec.push_back(str);
        }
    }

    pCmd = m_cmdFactory.getCommandProcesser(cmd.c_str());
    if(NULL!=pCmd)
    {
        pCmd->execute(m_socket,optionVec);
    }
}

void EDB::split(const std::string &text, char delim, std::vector<std::string> &result)
{
    size_t strlen = text.length();
    size_t first = 0;
    size_t pos = 0;
    for(first = 0;first <strlen;first = pos+1)
    {
        pos =  first;
        while(text[pos]!=delim && pos<strlen)
        {
            pos++;
        }
        std::string str = text.substr(first,pos-first);
        result.push_back(str);
    }
}

int EDB::readInput(const char *pPrompt,int numIndent)
{
    memset(m_cmdBuffer,0,CMD_BUFFER_SIZE);

    //printf tab
    for(int i = 0;i<numIndent; i++)
    {
        std::cout<< TAB;
    }
    //printf "edb>"
    std::cout<<pPrompt<<"> ";

    //read a line from cmd
    readLine(m_cmdBuffer,CMD_BUFFER_SIZE-1);
    int curBufLen = strlen(m_cmdBuffer);
    
    // '\' means continue
    while(m_cmdBuffer[curBufLen-1] == BACK_SLANT &&(CMD_BUFFER_SIZE-curBufLen)>0)
    {
        for(int i= 0;i<numIndent;i++)
        {
            std::cout << TAB;
        }
        std::cout << "> ";

        readLine(&m_cmdBuffer[curBufLen-1],CMD_BUFFER_SIZE-curBufLen);
    }

    curBufLen = strlen(m_cmdBuffer);

    for(int i = 0;i<numIndent; i++)
    {
        if(m_cmdBuffer[i]==TAB)
        {
            m_cmdBuffer[i] = SPACE;
        }
    }

    return EDB_OK;
}

char *EDB::readLine(char *p,int length)
{
    int len = 0;
    int ch;
    while((ch = getchar())!=NEW_LINE)
    {
        switch (ch)
        {
         case BACK_SLANT:
            break;
         default:
            p[len] = ch;
            len++;
            break;
        }
        continue;
    }
    len = strlen(p);
    p[len] = 0;
    return p;
}







int main()
{
    std::cout << "test edb " << std::endl;

    EDB edb;
    edb.start();
    return 0;
}