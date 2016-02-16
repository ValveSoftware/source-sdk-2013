#include "cbase.h"
#include "../EventLog.h"
#include "KeyValues.h"

class CMOMEventLog : public CEventLog
{
private:
    typedef CEventLog BaseClass;

public:
    virtual ~CMOMEventLog() {};

public:
    bool PrintEvent(IGameEvent * event)	// override virtual function
    {
        if (BaseClass::PrintEvent(event))
        {
            return true;
        }

        if (Q_strcmp(event->GetName(), "mom_") == 0)
        {
            return PrintMOMEvent(event);
        }

        return false;
    }

protected:

    bool PrintMOMEvent(IGameEvent * event)	// print Mod specific logs
    {
        //const char * name = event->GetName() + Q_strlen("sdk_"); // remove prefix
        return false;
    }

};

CMOMEventLog g_MOMEventLog;

//-----------------------------------------------------------------------------
// Singleton access
//-----------------------------------------------------------------------------
IGameSystem* GameLogSystem()
{
    return &g_MOMEventLog;
}