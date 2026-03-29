# Contributing to Endless Sky

As a free and open source game, Endless Sky is the product of many people's work. Those who wish to contribute new content are encouraged to review the [wiki](https://github.com/endless-sky/endless-sky/wiki), and to post in the [community-run Discord](https://discord.gg/ZeuASSx) beforehand. There are also [discussion rooms](https://steamcommunity.com/app/404410/discussions/) for those who prefer to use Steam, or GitHub's [discussion zone](https://github.com/endless-sky/endless-sky/discussions). Our forums (especially the GitHub and Discord) have projects that could use your help developing artwork, storylines, and other writing. We also have a loosely defined [roadmap](https://github.com/endless-sky/endless-sky/wiki/DevelopmentRoadmap) that outlines what our current goals for game development are.

We are always excited to welcome new contributors! Below are some guidelines and resources to help get involved.

## Issues

### Posting issues

The [issues page](https://github.com/endless-sky/endless-sky/issues) on GitHub is for tracking bugs and feature requests. When posting a new issue, please:

* Check to make sure it is not a duplicate of an existing issue.
* Create a separate "issue" for each bug you are reporting and each feature you are requesting.
* Choose the correct issue template and fill it out to the best of your ability.
* Do not use the issues page for things other than what match the provided issue templates. Use the [discussions page](https://github.com/endless-sky/endless-sky/discussions) instead. Examples of what the discussion pages would be used for include topics that are not looking for a resolution (as the point of an issue is to have it be resolved) or topics that are extremely broad and not befitting of an issue because there is no single resolution to the topic.

If requesting a new feature, first ask yourself: will this make the game more fun or interesting within the existing scope of the game? While features in other games may be fun or interesting, they may not fit Endless Sky. Be sure to check the game's [overall vision](https://github.com/endless-sky/endless-sky/wiki/Endless-Sky's-Vision) to get an idea of what our design values are.

### Closing issues

If you believe your issue has been resolved, you can close the issue yourself. If your issue gets closed because a PR was merged, and you are not satisfied, please do not comment on the old issue. Open a new issue and explain the gap that you feel still needs to be addressed.

## Pull requests

### Acknowledgement

**By contributing to the game, you acknowledge that your work may be added to, modified, or removed as a result of future contributions by others. While we prefer to work with the original authors of contributions where possible when it comes to changing their contributions, particularly with respect to story content, you do not have strict ownership over your work after it has been contributed to the game.**

If you are not comfortable with losing complete creative freedom over your work, please consider publishing it as a plugin instead. Plugins can be advertised on our [Discord server](https://discord.gg/ZeuASSx) or the [official plugins repository](https://github.com/endless-sky/endless-sky-plugins). For more information on when to consider publishing your content as a plugin or opening a pull request to add it to the game, please read the [Creation Guide - Plugin or Pull Request?](https://github.com/endless-sky/endless-sky/wiki/Creation-Guide---Plug-in-or-Pull-Request) article.

### Acceptable licenses

By contributing code or written content to the game, you accept that your works are being published under the [GPLv3.0 license](https://www.gnu.org/licenses/).

All artwork, sounds, and other non-written assets contributed to the game must fall under a copyright license that follows the [Debian Free Software Guidelines](https://www.debian.org/social_contract#guidelines) and the work must be listed in the [copyright](https://github.com/endless-sky/endless-sky/blob/master/copyright) file along with the license it is published under.

Common artwork licenses that are allowed include:
* Anything in the public domain. (e.g. works published by NASA or other government agencies, or licensed under the CC0 public domain license.)
* Any attribution (BY) Creative Commons license version 3.0 and later, which may also be share-alike (SA). (e.g. CC-BY-3.0, CC-BY-SA-4.0)

Common artwork licenses that are disallowed include:
* Creative Commons licenses older than version 3.0. (e.g. CC-BY-2.0)
* Any non-commerical (NC) or no derivative works (ND) creative commons licenses. (e.g. CC-BY-NC-3.0)
* Most proprietary licenses, including licenses used by many image hosting websites such as Pexels, Pixabay, and Unsplash.
	* Note that many image hosting websites used to license images under the CC0 Public Domain license, but have since moved to using non-free proprietary licenses which limit derivative and/or commerical uses. For such websites, using older images may be acceptable. Confirm that the page that you sourced the image from clearly lists the license that the image was published under.

For more reading on free licenses, see the following pages:
* [The DFSG and Software Licenses](https://wiki.debian.org/DFSGLicenses)
* [Debian License Information](https://www.debian.org/legal/licenses/)

### Posting pull requests

If you are posting a pull request, please:

* Check if your pull request solves an existing issue. Note that just because an issue is open does not guarantee that it is something we will accept, as the number of issues we receive means that not all of them have been thoroughly discussed, and there may be open issues for features that we decide we do not want.
* Do not combine multiple unrelated changes.
* Check the diff and make sure the pull request does not contain unintended changes.
* If changing the C++ code, follow the [coding standard](https://github.com/endless-sky/endless-sky/wiki/C++-Style-Guide).
* Fill out the pull request template to the best of your ability. GitHub's pull request template features are not as thorough as issue templates. You are able to delete the entire pull request template and put in whatever you want; please do not do this. The purpose of the pull request template is to help facilitate the review, approval, and merging of pull requests.
* If adding new content to the game (e.g. new ships, outfits, artwork, storylines, etc.) please take a look at the [Style Goals](https://github.com/endless-sky/endless-sky/wiki/StyleGoals) and [Quality Checklist](https://github.com/endless-sky/endless-sky/wiki/QualityChecklist) sections of our wiki to make sure that your content fits with Endless Sky's writing and art style.

If proposing major changes to the game through your pull request, start by posting an issue or discussion and discussing the best way to implement it. Often the first strategy that occurs to you will not be the cleanest or most effective way to implement a new feature.

### On AI-generated/assisted content

**AI-generated/assisted content is forbidden** in Endless Sky development. This is for several reasons, including:

* Ethical issues. LLMs rely on the unauthorized usage of copyrighted material in order to generate text. These 'tools' by their nature require mass copyright infringement to operate.

* Philosophical issues. The reason why we're here and contributing to this project in the first place is because we like to write/model/edit/code for the game. We do those things because we enjoy it. Using AI to do these processes for us completely misses the point of this project. 
There are many volunteers willing to help review, polish, and create new content. If you would like help with art, writing tone or style (particularly for aliens with unique speech mannerisms), please feel free to post in the content-creating channel in Discord, or feel free to state in your PR where you would particularly like our reviewers' help.

* Legal issues. AI-generated material is currently in a legal gray area and most likely does not conform with the requirements of GPLv3, which Endless Sky is licensed under. This is the ultimate reason why we have a hard no on any AI content in the game.

### What to expect when opening a pull request

Endless Sky is developed by a community of volunteers. We are a collection of people with various different interests and skill sets, so the type of pull request you open will influence who will be looking at it.

All pull requests go through a review process before being merged. We have a group of dedicated Reviewers who help provide reviews to pull requests as they come in. We also have a group of Developers who make determinations on what does and does not get merged, and are the ones with the permissions to merge pull requests.

* For smaller pull requests, such as typo fixes, it may only require a Developer seeing your pull request in order for it to be merged.
* Larger pull requests typically receive reviews from multiple Reviewers and Developers before they are merged. That, in addition to the current backlog of open pull requests, can result in the full review process taking a number of months before a larger pull request may be merged. We ask for your patience while your pull request undergoes the review process. We also ask that large content pull requests (for example, pull requests that introduce a new faction or large storyline/campaign) be discussed in a [discussion post](https://github.com/endless-sky/endless-sky/discussions) or on the game's [Discord server](https://discord.gg/ZeuASSx). This is so other contributors get a chance to review your writing when it is still in the planning stages, and prevent conflicts with other planned storylines or with existing lore. We would like to avoid situations where someone puts in a lot of effort into writing a new campaign or faction, only to be told that it requires fundamental changes once they've submitted a pull request. It is much easier to make conceptual changes to a story when it is not yet finalized and written.

The type of pull request you open will also influence when it is eligible to be merged. We release updates alternating between "stable" and "unstable" versions.

* During the development of an unstable release, any type of pull request is up for merging. Unstable releases are typically developed over the course of three to four months and with odd version numbers (e.g. v0.10.1).
* During the development of a stable release, we typically only accept bug fixes and minor content tweaks. Stable releases are typically developed over the course of two to six weeks and with even version numbers (e.g. v0.10.2).

If you have opened a new feature pull request during a stable release development cycle, it is unlikely to be merged until the stable release is finished. Similarly, if you open a large pull request toward the end of an unstable release, it will likely have to wait until the next unstable release development cycle has begun before people will begin reviewing it and it has a chance to be merged. This is because the time it would take to appropriately review the pull request is longer than the time until the next release, and Developers and Reviewers near the end of a release cycle are often focused on a priority list determined in advance closer to the release date.

### Reviewing a pull request

Reviewing pull requests is an important part of maintaining Endless Sky and the main way that new content is added to the game. Your feedback helps contributors improve their work and ensures that new content is the best it can be. For more information, see the [wiki page](https://github.com/endless-sky/endless-sky/wiki/ReviewingPRs).
