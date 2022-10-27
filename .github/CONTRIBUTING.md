This file details how to contribute to the Mapbase project on GitHub:
	https://github.com/mapbase-source/source-sdk-2013

For the original Source SDK 2013 contribution guidelines, click here:
	https://github.com/ValveSoftware/source-sdk-2013/blob/master/CONTRIBUTING

---

Mapbase is a project which many Source modders draw from, so it has its own unique standards
for contributions which differ from other projects, but it is still an open-source repository
that is always open to contributions.

Whenever you contribute to the Mapbase repository, you must keep in mind that any contributions
made could be deployed to all mods utilizing Mapbase, which can include everything from high-profile
Steam mods to amateur HL2 maps. Many contributions can also end up being available in both SP and MP
if the contributions are not obviously exclusive to one of the two.

All contributions must follow the following rules:

 * A contribution must be aligned with Mapbase's goals and priorities and should not be "subjective"
   or related to a specific mod or type of mod. For example, fixing an existing issue or adding a
   new tool for mappers to use is usually fine, but adding a new custom weapon with its own assets
   is usually not fit for Mapbase.
   
 * All content in a contribution must be either already legally open-source or done with the
   full permission of the content's original creator(s). If a license is involved, the contributor
   should ensure Mapbase conforms to its terms.
    * **NOTE:** Due to concerns with mods which do not wish to be open-source, content using GPL licenses (or any
	  license with similar open-source requirements) are currently not allowed to be distributed with Mapbase.
	  Contributions which can draw from them without actually distributing the licensed content may be excepted
	  from this rule.
   
 * Contributions must not break existing maps/content or interfere with them in a negative or non-objective way.
   
 * Code contributions are not obliged to follow Mapbase's preprocessor conventions (e.g. #ifdef MAPBASE),
   although following them is usually acceptable.
   
 * Code contributions which modify or add onto existing code should generally match its syntax and shouldn't
   change the spacing unless necessary.
   
 * If you are contributing a file you created yourself specifically for Mapbase, you are required to
   use the custom "Mapbase - Source 2013" header used in other Mapbase files as of Mapbase v5.0.
   You are encouraged to append an "Author(s)" part to that header in your file in order to clarify who wrote it.
   
 * Do not modify the README to add attribution for your contribution. That is handled by Mapbase's maintainers.
   
Contributions which do not follow these guidelines cannot be accepted into Mapbase. Attempting to contribute content
which seriously violates the rules above can lead to being blocked from contributing, especially if done repeatedly.

---

Mapbase uses GitHub Actions to help manage issues and pull requests. Some of these workflows build the code of incoming
contributions to make sure they compile properly. The code is compiled separately for Visual Studio 2022 and GCC/G++ 9 (Linux)
and on both Debug and Release configurations.

If these workflows fail, don't freak out! Accidents can happen frequently due to compiler syntax differences and conflicts
from other contributions. You can look at a failed workflow's log by clicking "Details", which will include the build's output
in the "Build" step(s). Any errors must be resolved by you and/or by code reviewers before a pull request can be merged.

If your contribution is accepted, you may be listed in Mapbase's credits and the README's external content list:
	https://github.com/mapbase-source/source-sdk-2013/wiki/Mapbase-Credits#Contributors
	https://github.com/mapbase-source/source-sdk-2013/blob/master/README
	
You may also receive the "Contributor" or "Major Contributor" role on Mapbase's Discord server if you are
a member of it.

