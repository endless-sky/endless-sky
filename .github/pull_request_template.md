### How To Title Your PR

A good Pull Request title will use the following structure:
`<type>(<scope>): [optional faction] Title`

<details>
 <summary>Suggested Types</summary>

- `build`: Changes that affect the build system or external dependencies.
- `chore`: Changes to the build process or additional tools and libraries such as documentation generation.
- `ci`: Changes to our CI configuration files and scripts.
- `docs`: Documentation only changes
- `feat`: A new feature
- `fix`: A bug fix
- `perf`: A code change that improves performance
- `refactor`: A code change that neither fixes a bug nor adds a feature
- `style`: Changes that do not affect the meaning of the code (white space, formatting, etc.)
- `test`: Adding missing tests or correcting existing tests

</details>

<details>
 <summary>Suggested Scopes</summary>

-   `balance`: Addition or modification of existing ship/outfit stats
-   `mechanics`: Modifications to the game engine which affect gameplay
-   `UI`: Modifications of existing User Interface elements
-   `enhancement`: Additional features added to the game engine
-   `AI`: Modification to the game engine which affect NPC/Escort Behavior
-   `mission`: Addition or Modification of missions
-   `ship`: Addition or Modification of ships
-   `outfit`: Addition or Modification of outfits
-   `content`: Additional content that is not ships, outfits or missions

</details>

<details>
 <summary>Suggested Factions</summary>

-   `Human`
-   `Hai`
-   `Wanderer`
-   `Remnant`
-   `Builder`
-   `Korath`
-   `Coalition`
-   `Quarg`
-   `Drak`
-   `Pug`
-   `Sheragi`
-   Others not listed are allowed; use your best judgment when necessary.

</details>

<details>
 <summary>Title</summary>

- Provide a clear, concise, and descriptive title of your PR's function without repeating information already provided with the previous tags.

 </details>

### PR Template

NOTICE: Delete the sections that do not apply to your PR, and fill out the section that does.
(You can open a PR to add or improve a section, if you find them lacking!)

----------------------
**Content (Artwork / Missions / Jobs)**

## Summary
{{summarize your content! Include links to related issues, in-game screenshots, etc.}}

## Save File
This save file can be used to play through the new mission content:
{{attach a save file that allows people to easily test your added mission content or see your new in-game art}}

## Artwork Checklist
 - [ ] I updated the copyright attributions, or decline to claim copyright of any assets produced or modified
 - [ ] I created a PR to the [endless-sky-assets repo](https://github.com/endless-sky/endless-sky-assets) with the necessary image, blend, and texture assets: {{insert PR link}}
 - [ ] I created a PR to the [endless-sky-high-dpi repo](https://github.com/endless-sky/endless-sky-high-dpi) with the `@2x` versions of these art assets: {{insert PR link}}


-----------------------
**Bugfix:** This PR addresses issue #{{insert number}}

## Fix Details
{{add details}}

## Testing Done
{{describe how you tested that the fix doesn't introduce other issues}}

## Save File
This save file can be used to verify the bugfix. The bug will occur when using {{insert commit hash / version}}, and will not occur when using this branch's build.
{{attach a save file that can be used to verify your bugfix. It MUST have no plugin requirements}}


-----------------------
**Feature:** This PR implements the feature request detailed and discussed in issue #{{insert number}}

## Feature Details
{{add details about the feature you implemented}}

## UI Screenshots
{{attach before + after screenshots of any changes to UI, or replace this line with "N/A"}}

## Usage Examples
{{if this feature is used in the data files, provide examples!}}

## Testing Done
{{describe how you tested the new feature}}

### Automated Tests Added
{{describe the automated tests you added (using the unit-test framework, integration-test framework or otherwise) to reduce the risk of regressions in the future. "N/A" if this PR is not for an in-game feature that needs to remain functional in the future. }}

## Performance Impact
{{describe any performance impact (positive or negative). "N/A" if no performance-critical code is changed. }}
