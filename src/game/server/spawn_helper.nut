//=========================================================
// An spawner on the server is getting ready to
// prespawn an entity. It calls this function, sending us
// the entity that it's preparing to spawn. 
//=========================================================

function __ExecutePreSpawn( entity ) 
{
	__EntityMakerResult <- {}
	if ( "PreSpawnInstance" in this )
	{
		local overrides = PreSpawnInstance( entity.GetClassname(), entity.GetName() );
		local type = typeof( overrides );
		if ( type == "table" )
		{
			foreach( key, value in overrides )
			{
				switch ( typeof( value ) )
				{
				case "string":
					{
						entity.__KeyValueFromString( key, value );
						break;
					}
				case "integer":
					{
						entity.__KeyValueFromInt( key, value );
						break;
					}
				case "float":
				case "bool":
					{
						entity.__KeyValueFromFloat( key, value.tofloat() );
						break;
					}
					
				case "Vector":
					{
						entity.__KeyValueFromVector( key, value );
						break
					}
					
				default:
					{
						printl( "Cannot use " + typeof( value ) + " as a key" );
					}
				}
			}
		}
		
		if ( type == "bool" )
		{
			return overrides;
		}
	}
};

function __FinishSpawn()
{
	__EntityMakerResult <- null;
}
