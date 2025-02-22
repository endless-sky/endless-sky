# Contributing to Endless Sky

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

### Posting pull requests

If you are posting a pull request, please:

* Check if your pull request solves an existing issue. Note that just because an issue is open does not guarantee that it is something we will accept, as the number of issues we receive means that not all of them have been thoroughly discussed, and there may be open issues for features that we decide we do not want.
* Do not combine multiple unrelated changes.
* Check the diff and make sure the pull request does not contain unintended changes.
* If changing the C++ code, follow the [coding standard](https://endless-sky.github.io/styleguide/styleguide.xml).
* Fill out the pull request template to the best of your ability. GitHub's pull request template features are not as thorough as issue templates. You are able to delete the entire pull request template and put in whatever you want; please do not do this. The purpose of the pull request template is to help facilitate the review, approval, and merging of pull requests.
* If adding new content to the game (e.g. new ships, outfits, artwork, storylines, etc.) please take a look at the [Style Goals](https://github.com/endless-sky/endless-sky/wiki/StyleGoals) and [Quality Checklist](https://github.com/endless-sky/endless-sky/wiki/QualityChecklist) sections of our wiki to make sure that your content fits with Endless Sky's writing and art style.

If proposing major changes to the game through your pull request, start by posting an issue or discussion and discussing the best way to implement it. Often the first strategy that occurs to you will not be the cleanest or most effective way to implement a new feature.

Please keep in mind that **AI-generated/assisted content is forbidden** in Endless Sky development.

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

Reviewing pull requests is an important part of maintaining Endless Sky and the main way that new content is added to the game. Your feedback helps contributors improve their work and ensures that new content is the best it can be.
