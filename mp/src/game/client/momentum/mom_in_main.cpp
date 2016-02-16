#include "cbase.h"
#include "kbutton.h"
#include "input.h"
#include "momentum/mom_shareddefs.h"
#include <game/client/iviewport.h>

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define IN_TIMES (1<<26)

static int s_ClearInputState = 0;
static	kbutton_t	in_times;

//-----------------------------------------------------------------------------
// Purpose: HL Input interface
//-----------------------------------------------------------------------------
class CMOMInput : public CInput
{
    typedef CInput BaseClass;
public:
    
    int GetButtonBits(int bResetState)
    {
        int bits = BaseClass::GetButtonBits(bResetState);
        CalcButtonBits(bits, IN_TIMES, s_ClearInputState, &in_times, bResetState);
        
        bits &= ~s_ClearInputState;

        if (bResetState)
        {
            s_ClearInputState = 0;
        }

        return bits;
    }

    void ClearInputButton(int bits)
    {
        s_ClearInputState |= bits;
        BaseClass::ClearInputButton(bits);
    }

private:

    void CalcButtonBits(int& bits, int in_button, int in_ignore, kbutton_t *button, bool reset)
    {
        // Down or still down?
        if (button->state & 3)
        {
            bits |= in_button;
        }

        int clearmask = ~2;
        if (in_ignore & in_button)
        {
            // This gets taken care of below in the GetButtonBits code
            //bits &= ~in_button;
            // Remove "still down" as well as "just down"
            clearmask = ~3;
        }

        if (reset)
        {
            button->state &= clearmask;
        }
    }
}; 

void IN_TimesDown(const CCommand &args)
{
    KeyDown(&in_times, args[1]);
    if (gViewPortInterface)
    {
        gViewPortInterface->ShowPanel(PANEL_TIMES, true);
    }
}

void IN_TimesUp(const CCommand &args)
{
    KeyUp(&in_times, args[1]);
    if (gViewPortInterface)
    {
        gViewPortInterface->ShowPanel(PANEL_TIMES, false);
        //GetClientVoiceMgr()->StopSquelchMode();
    }
}

static ConCommand startshowtimes("+showtimes", IN_TimesDown);
static ConCommand endshowtimes("-showtimes", IN_TimesUp);

// Expose this interface
static CMOMInput g_Input;
IInput *input = (IInput *) &g_Input;