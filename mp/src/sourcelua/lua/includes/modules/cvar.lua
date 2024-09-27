--========== Copyleft © 2010, Team Sandbox, Some rights reserved. ===========--
--
-- Purpose: Implements global change callbacks for ConVars.
--
--===========================================================================--

local pairs = pairs
local Warning = Warning
local tostring = tostring
local pcall = pcall

module( "cvar" )

local bError, strError
local tCallbacks = {}

-------------------------------------------------------------------------------
-- Purpose: Adds a change callback
-- Input  : strConVarName - Name of the ConVar
--          strCallbackName - Name of the callback
--          pFn - pointer to function
-- Output :
-------------------------------------------------------------------------------
function AddChangeCallback( strConVarName, strCallbackName, pFn )
  tCallbacks[ strConVarName ] = tCallbacks[ strConVarName ] or {}
  tCallbacks[ strConVarName ][ strCallbackName ] = pFn
end

-------------------------------------------------------------------------------
-- Purpose: Called by the engine to call global change callbacks
-- Input  : var - ConVar that has changed
--          pOldString - String value before var changed
--          flOldValue - Float value before var changed
-- Output :
-------------------------------------------------------------------------------
function CallGlobalChangeCallbacks( var, pOldString, flOldValue )
  local tCallbacks = tCallbacks[ var:GetName() ]
  if ( tCallbacks ~= nil ) then
    for k, v in pairs( tCallbacks ) do
      if ( v == nil ) then
        Warning( "Callback '" .. tostring( k ) .. "' (" .. tostring( var:GetName() ) .. ") tried to call a nil function!\n" )
        tCallbacks[ k ] = nil
        break
      else
        bError, strError = pcall( v, var, pOldString, flOldValue )
        if ( bError == false ) then
          Warning( "Callback '" .. tostring( k ) .. "' (" .. tostring( var:GetName() ) .. ") Failed: " .. tostring( strError ) .. "\n" )
          tCallbacks[ k ] = nil
        end
      end
    end
  end
end

-------------------------------------------------------------------------------
-- Purpose: Returns all of the registered callbacks or only callbacks
--          pertaining to a specific ConVar
-- Input  : strConVarName - Name of the ConVar
-- Output : table
-------------------------------------------------------------------------------
function GetChangeCallbacks( strConVarName )
  if ( strConVarName ) then
    return tCallbacks[ strConVarName ]
  end
  return tCallbacks
end

-------------------------------------------------------------------------------
-- Purpose: Removes a callback from the list of registered callbacks
-- Input  : strConVarName - Name of the ConVar
--          strCallbackName - Name of the callback
-- Output :
-------------------------------------------------------------------------------
function RemoveChangeCallback( strConVarName, strCallbackName )
  if ( tHooks[ strConVarName ][ strCallbackName ] ) then
    tHooks[ strConVarName ][ strCallbackName ] = nil
  end
end
