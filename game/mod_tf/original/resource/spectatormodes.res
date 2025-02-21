// Command Menu Definition
// 
// "filename.res"
// {
//    "menuitem1"
//    {
//      "label"		"#GoToB"          // lable name shown in game, # = localized string
//      "toggle"	"sv_cheats"       // a 0/1 toggle cvar
//      "command"	"say_team Go 2 B" // your type specific data, a client command or cvar etc.
//      "rule"		"map"             // visibility rules : "none", "team", "map","class"	
//      "ruledata"	"de_dust"	  // rule data, eg map name or team number
//    }
//   
//   "menuitem2"
//   {
//	...
//   }
//
//   ...
//
// }
//
//--------------------------------------------------------
// Everything below here is editable

"spectatormodes.res"
{
	"type"		"menu"

	"menuitem1"
	{
		"label"		"#Spec_Mode3"	// name shown in game
		"command"	"spec_mode 3"	// type data
	}
			
	"menuitem2"
	{
		"label"		"#Spec_Mode4"
		"command"	"spec_mode 4"
	}
	
	"menuitem3"
	{
		"label"		"#Spec_Mode5"
		"command"	"spec_mode 5"
	}
}






