--========== Copyleft © 2010, Team Sandbox, Some rights reserved. ===========--
--
-- Purpose: Hook implementation.
--
--===========================================================================--

local pairs = pairs
local Warning = Warning
local tostring = tostring
local pcall = pcall

module( "hook" )

local bError, r1, r2, r3, r4, r5, r6, r7, r8
local tHooks = {}

-------------------------------------------------------------------------------
-- Purpose: Adds a hook to the given GameRules function
-- Input  : strEventName - Name of the internal GameRules method
--          strHookName - Name of the hook
--          pFn - pointer to function
-- Output :
-------------------------------------------------------------------------------
function Add( strEventName, strHookName, pFn )
  tHooks[ strEventName ] = tHooks[ strEventName ] or {}
  tHooks[ strEventName ][ strHookName ] = pFn
end

-------------------------------------------------------------------------------
-- Purpose: Called by the engine to call a GameRules hook
-- Input  : strEventName - Name of the internal GameRules method
-- Output :
-------------------------------------------------------------------------------
function Call( strEventName, ... )
  local tHooks = tHooks[ strEventName ]
  if ( tHooks ~= nil ) then
    for k, v in pairs( tHooks ) do
      if ( v == nil ) then
        Warning( "Hook '" .. tostring( k ) .. "' (" .. tostring( strEventName ) .. ") tried to call a nil function!\n" )
        tHooks[ k ] = nil
        break
      else
        bError, r1, r2, r3, r4, r5, r6, r7, r8 = pcall( v, ... )
        if ( bError == false ) then
          Warning( "Hook '" .. tostring( k ) .. "' (" .. tostring( strEventName ) .. ") Failed: " .. tostring( r1 ) .. "\n" )
          tHooks[ k ] = nil
        else
          if ( r1 ~= nil ) then
            return r1, r2, r3, r4, r5, r6, r7, r8
          end
        end
      end
    end
  end
  return r1, r2, r3, r4, r5, r6, r7, r8
end

-------------------------------------------------------------------------------
-- Purpose: Returns all of the registered hooks or only hooks pertaining to a
--          specific event
-- Input  : strEventName - Name of the internal GameRules method
-- Output : table
-------------------------------------------------------------------------------
function GetHooks( strEventName )
  if ( strEventName ) then
    return tHooks[ strEventName ]
  end
  return tHooks
end

-------------------------------------------------------------------------------
-- Purpose: Removes a hook from the list of registered hooks
-- Input  : strEventName - Name of the internal GameRules method
--          strHookName - Name of the hook
-- Output :
-------------------------------------------------------------------------------
function Remove( strEventName, strHookName )
  if ( tHooks[ strEventName ][ strHookName ] ) then
    tHooks[ strEventName ][ strHookName ] = nil
  end
end
