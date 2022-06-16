#include "core.hpp"

const char    SPACE             =  ' ';
const char    TAB               =  '\t';
const char    BACK_SLANT        =  '\\';
const char    NEW_LINE          =   '\n';
int g_Quit = 0;
void EDB::start(void)
{
    std::cout << "Welcome to EmeraldDB Shell!" << std::endl;
   std::cout << "edb help for help, Ctrl+c or quit to exit" << std::endl;
   while(0==gQuit)
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

}

int main()
{
    std::cout << "test edb " << std::endl;
    return 0;
}