# Governance

Endless Sky is a volunteer-run project centerd on community-driven development. Governance does not follow a strict hierarchy; each role is expected to handle only what falls within its scope. 
While the list of roles is presented in a rough hierarchy based on permission level, this should not be taken to mean that each role includes the responsibilities of the roles below it.

## Roles

### Admin
GitHub Permissions: Owner

Description: An admin is someone with the highest permission level on GitHub. The purpose of an admin is to help facilitate the development of the game by taking actions on GitHub that cannot be done by lower permission levels.

**Responsibilities/Duties:**
- Control creation and assignment of roles and teams within the GitHub organization and for each repository.
- Modify visibility of projects when necessary.
- Modify repository settings when necessary to facilitate development.
- Otherwise do that which can not be done by anyone else when necessary.
  
**Notes:**
- GitHub does not provide us with granular permissions. We have only a linear permission hierarchy that goes read -> triage -> write -> maintain -> admin. But on an organizational level, having permissions higher up GitHub's hierarchy should not be seen as an invitation to fulfill the roles that have lower GitHub permissions. For example, although a GitHub admin has higher permissions than a developer, an admin should not be seen as or fulfill the role of a developer, unless that admin is also organizationally considered to be a developer.
- Given the power that the permissions of an admin carry, most admin actions should only be taken after a discussion between the admin(s) and other individuals relevant to the action being taken.

### Developer
GitHub Permissions: Write or Maintain

Description: Developers (also known as collaborators on GitHub) are the primary driving force behind the game's development. They are the ones with the permissions to merge pull requests into the game, and thus ultimately decide what should and should not be merged.

**Responsibilities/Duties:**
- Review pull requests.
- Merge pull requests after they have reached an acceptable level of quality.
- Decide which pull requests to reject.
- Assign labels to issues and pull requests.
- Assign milestones to issues and pull requests.
- Close resolved issues and rejected pull requests.

**Notes:**
- Some developers have a specific "focus" for which area(s) of the game that they develop for. For example, some developers may focus on code, while others focus on content, or even just specific areas of content. But, a "focus" should under no circumstances be seen as a limit on the power of a developer. Just because a developer may focus on code changes does not mean that their feedback should be ignored when it comes to questions of content on the basis of their focus not being in content.
- When reviewing a pull request, developers should use their own discretion to determine which of their review comments are absolutely necessary to be addressed in order for the pull request to be merged. Not every comment that a developer may have may block the merging of a pull request.
- Developers should not simply support the review comments of other developers without first reviewing the pull request themselves. In cases where a pull request may be primarily outside of a developer's focus, understand that the ramifications of a pull request may be wide reaching. For example, while a code pull request may on its surface seem outside of the scope of a content focused developer, the impact of that code may still be felt on the content side of the game, and so a content focused developer could still review the pull request from that perspective.
- The approval of a pull request from a developer should only ever mean that the developer believes that the pull request is in a good state and should be merged into the game in the state that it is in. Developers should not approve pull requests for any other reasons (e.g. approving to appease someone even if the developer themself does not believe that the pull request should be merged).

### Reviewer
GitHub Permissions: Triage

Description: Reviewers are the helpers of the developers. They are people who have shown themselves to have a good sense of how pull requests should be changed to make them better fit with the game, be that in correcting grammar and spelling, adjusting data syntax, or speaking on the game's themes, story, and lore.

**Responsibilities/Duties:**
- Review pull requests.
- Assign labels to issues and pull requests.

**Notes:**
- Similar to developers, reviewers may have a specific "focus" for which area(s) of the game that they review for. But as with developers, the "focus" of a reviewer should not be seen as pigeon-holing them into only being able to review areas within their focus.
- Just as not all review comments from developers are binding, the same is true of reviewers. Just because a reviewer suggests a change does not mean that a contributor must accept that change, or that a developer must see to it that the change is made before merging the pull request.
- Unlike with developers, where an approval effectively states whether a pull request should be merged, approvals from reviewers can have multiple meanings. One meaning is that the reviewer believes that the pull request is good and should be merged. Another meaning is that the pull request is good in that reviewer's area of expertise. For example, if a grammar and spelling reviewer reviews a mission pull request after seeing that it has no grammar or spelling issues, they can approve that pull request on the grounds that its grammar and spelling are good. But, a reviewer could approve the same pull request to mean that they like the missions and would like to see them merged. The approval message of a reviewer should describe which of these two situations they are approving the pull request for.
- Although reviewer is a title that we give to people, one need not be a reviewer in order to review pull requests. The title of reviewer is simply an acknowledgement from the existing team that one has shown themself as capable of providing good, constructive reviews. Any power or weight that a reviewer may have does not flow from their title, but from their merit.

### Community Organizer
GitHub Permissions: Triage

Description: Community organizers should be the people's people. While developers focus on the development side of the game, community organizers focus on the game's community aspects.

**Responsibilities/Duties:**
- Assess community sentiment on the state of development, such as via surveys.
- Act as a communication bridge between the wider community and the developers.
- Be available for communication from all community members.
- Suggest granting of roles to community members.
- Encourage positive community interactions and growth.

**Notes**
- A community organizer is not the same thing as a moderator. A moderator's role is in enforcement, while a community organizer's role is in analysis and interaction.

### Oathkeeper (Moderator)
GitHub Permissions: Moderator

Description: Moderators help to keep discussions civil, ensure disagreements remain productive, de-escalate conflict, resolve communication mishaps, and prioritize creating a friendly and welcoming environment that assures contributors their time will be appreciated and valued.

**Responsibilities/Duties:**
- Show a genuine desire to foster a welcoming environment.
- Stay cool, calm, and collected even in tense situations.
- Display good communication and conflict-resolution skills.
- Show that they can be trusted to moderate fairly and unbiasedly.
- Show a willingness to resolve conflicts even with uncooperative parties.

**Notes:**
- While our moderation team is equipped with the tools to take moderation actions when necessary, we believe that this should be considered a last resort option. Our moderation team prefers to de-escalate and resolve conflicts and only utilizes moderation actions such as verbal warnings, time-outs, or more if de-escalation is impossible.

### Organization Member
GitHub Permissions: Read

Description: Members are individuals who have been added to the Endless Sky organization without being granted a role. This is typically so they can be requested for reviews or given repository-specific permissions for specific projects.

**Responsibilities/Duties:**
Address the reviews of reviewers and developers when requested.

**Notes**
- While members do not have any organization-level permissions they may have repository-level permissions if necessary for their role.
- Members are typically individuals who have contributed a great deal to a specific faction and are often requested for reviews on that faction.

### Contributor
GitHub Permissions: Read

Description: Contributors are those who create the majority of the pull requests for the game. This is not a role that is given to individuals, but one that someone steps into in deciding to open a pull request.

**Responsibilities/Duties:**
Address the reviews of reviewers and developers on their pull requests.

**Notes:**
- Includes "content creators," which is a more specific term for contributors who open content pull requests.
- Contributors need not blindly accept every suggestion from a reviewer. If a contributor disagrees with a review comment, then they should explain why they disagree, particularly if the changes being requested are substantial. See the "On Review Relationships" and "On Reviews and Approvals" sections for more detail.

A full member list of who occupies each role is available at [Members](MEMBERS.md).
For additional information, see the GitHub [documentation](https://docs.github.com/en/organizations/managing-user-access-to-your-organizations-repositories/managing-repository-roles/repository-roles-for-an-organization) available for organization roles.

## Role Criteria
Interested in obtaining one of the roles listed above? Here's how members of each role are determined.

### Contributor
Contribute Something!
Contributions vary quite a bit, and there are numerous ways you can contribute to the project, including but not limited to:
- Playtest and provide feedback
- Review existing PRs
- Make your own PR
- Showcase the game
- Add to the wiki
- Make your own tool or service
- and many more

Additionally, if you open a PR which gets successfully merged, you will be listed in the [credits](/credits.txt) as a contributor and your GitHub name will be displayed on the main menu of the game.

### Moderator
Commit to making the environment of the Endless Sky community safe, welcoming and friendly. Make sure to follow all of the outlines in the [community guide](COMMUNITY.md) and help others follow them as well. 
Show that you can stay cool, calm and collected, resolve conflicts and foster a healthy community. 

If our existing moderation team feels that you meet these requirements, you may be invited to join the team and help our community grow.
- Help to resolve conflict if you see it
- Help to keep discussions productive and on-topic
- Keep your contributions to the Endless Sky community positive, helpful and inclusive
- Report breaches of our [community guidelines](COMMUNITY.md) to the existing moderator team
- Always follow our [Code of Conduct](CONDUCT.md)

### Reviewer
The most important job of a reviewer is to provide consistently high-quality reviews on existing PRs. This does not mean to do a lot of reviews, but rather ensure that every review you provide is high quality, clear, concise, and productive. 
New reviewers are typically selected by our existing reviewers, developers and maintainers from those who have a proven track record of providing good reviews. 
- Check the list of [Pull Requests](https://github.com/endless-sky/endless-sky/pulls) for any open PRs and provide a review.
- Playtest PRs and post the results of your playtesting.
- Help identify and fix grammar/spelling errors on existing PRs.
- Assist new contributors with getting their PRs to a merge-ready state.
- Identify (or fix) logic and syntax errors.
- Provide feedback on design decisions.

### Developer
New developers are typically picked from our existing reviewer team. If reviewers are expected to provide good, high-quality reviews, then developers are expected to provide good reviews _and_ merge PRs once no further review is necessary. 
As such, if you are interested in becoming a developer, aim for a reviewer role first, following all of the advice provided in the previous section.

_[return to README](/README.md)_
