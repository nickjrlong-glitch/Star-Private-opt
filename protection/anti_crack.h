#ifndef ANTI_CRACK_H
#define ANTI_CRACK_H

#include <string>
#include <windows.h>

namespace AntiCrack
{
    int getProcID(const std::string& p_name);
    bool isProcRunning(const std::string& process);

    namespace BSOD
    {
        void bsod();
    }

    void Tick();
    void HwidBan();
    bool CheckHwidBan();
    void InitializeAntiDebug();
}

#endif // ANTI_CRACK_H 