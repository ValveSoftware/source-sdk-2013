--========== Copyleft © 2010, Team Sandbox, Some rights reserved. ===========--
--
-- Purpose: ConCommand implementation.
--
--===========================================================================--

local ConCommand = ConCommand
local Warning = Warning
local tostring = tostring
local pcall = pcall

module( "concommand" )

local bError, strError
local tFnCommandCallbacks = {}

-------------------------------------------------------------------------------
-- Purpose: Creates a ConCommand
-- Input  : pName - Name of the ConCommand
--          callback - Callback function for the ConCommand
--          pHelpString - Help string to be displayed
--          flags - Flags of the ConCommand
-- Output :
-------------------------------------------------------------------------------
function Create( pName, callback, pHelpString, flags )
  tFnCommandCallbacks[ pName ] = callback
  ConCommand( pName, pHelpString, flags )
end

-------------------------------------------------------------------------------
-- Purpose: Called by the engine to dispatch a ConCommand
-- Input  : pPlayer - Player who ran the ConCommand
--          pCmd - Name of the ConCommand
--          ArgS - All args that occur after the 0th arg, in string form
-- Output : boolean
-------------------------------------------------------------------------------
function Dispatch( pPlayer, pCmd, ArgS )
  local fnCommandCallback = tFnCommandCallbacks[ pCmd ]
  if ( not fnCommandCallback ) then
    return false
  else
    bError, strError = pcall( fnCommandCallback, pPlayer, pCmd, ArgS )
    if ( bError == false ) then
      Warning( "ConCommand '" .. tostring( pCmd ) .. "' Failed: " .. tostring( strError ) .. "\n" )
    end
    return true
  end
end

-------------------------------------------------------------------------------
-- Purpose: Removes a ConCommand callback
-- Input  : pName - Name of the ConCommand
-- Output :
-------------------------------------------------------------------------------
function Remove( pName )
  if ( tFnCommandCallbacks[ pName ] ) then
    tFnCommandCallbacks[ pName ] = nil
  end
end
