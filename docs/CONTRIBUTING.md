# Contributing to Endless Sky

## Issues

### Posting issues

The [issues page](https://github.com/endless-sky/endless-sky/issues) on GitHub is for tracking bugs and feature requests. When posting a new issue, please:

* Check to make sure it's not a duplicate of an existing issue.
* Create a separate "issue" for each bug you are reporting and each feature you are requesting.
* Choose the correct issue template and fill it out to the best of your ability.
* Do not use the issues page for things other than bug reports and feature requests. Use the [discussions page](https://github.com/endless-sky/endless-sky/discussions) instead.

If requesting a new feature, first ask yourself: will this make the game more fun or interesting? Remember that this is a game, not a simulator. Changes will not be made purely for the sake of realism, especially if they introduce needless complexity or aggravation.

### Closing issues

If you believe your issue has been resolved, you can close the issue yourself. If your issue gets closed because a PR was merged, and you are not satisfied, please open a new issue.

## Pull requests

### Posting pull requests

If you are posting a pull request, please:

* Find if your pull request solves an existing issue. Note that just because an issue is open does not guarantee that it is something we will accept, as the number of issues we receive means that not all of them have been thoroughly discussed, and there may be open issues for features that we decide we do not want.
* Do not combine multiple unrelated changes.
* Check the diff and make sure the pull request does not contain unintended changes.
* If changing the C++ code, follow the [coding standard](https://endless-sky.github.io/styleguide/styleguide.xml).
* Fill out the pull request template to the best of your ability. GitHub's pull request template features are not as thorough as issue templates. You're able to delete the entire pull request template and put in whatever you want; please do not do this. The purpose of the pull request template is to help facilitate the review, approval, and merging of pull requests.

If proposing a major pull request, start by posting an issue and discussing the best way to implement it. Often the first strategy that occurs to you will not be the cleanest or most effective way to implement a new feature.

### What to expect when opening a pull request

Endless Sky is developed by a community of volunteers. We're a collection of people with various different interests and skill sets, so the type of pull request you open will influence who will be looking at it.

All pull requests go through a review process before being merged. We have a group of dedicated Reviewers who help provide reviews to pull requests as they come in. We also have a group of Developers who make determinations on what does and does not get merged, and are the ones who have the access to merge pull requests.
* For smaller pull requests, such as typo fixes, it may only require a Developer seeing your pull request in order for it to be merged.
* Larger pull requests typically receive reviews from multiple Reviewers and Developers before they are merged. That, in addition to the current backlog of open pull requests, can result in it taking months before a larger pull request is merged. We ask for your patience when waiting for your pull request to be reviewed.

The type (and size) of pull request you open will also influence when it is eligable to be merged. We release updates alternating between "stable" and "unstable" versions.
* During the development of an unstable release, any type of pull request is up for merging. Unstable releases are typically developed over the course of three to four months and with odd version numbers (e.g. v0.10.1).
* During the development of a stable release, we typically only accept bug fixes and minor content tweaks. Stable releases are typically developed over the course of two to six weeks and with even version numbers (e.g. v0.10.2).

If you've opened a new feature pull request during a stable release development cycle, it is unlikely to be merged until the stable release is finished. Or if you open a large pull request toward the end of an unstable release, it will likely have to wait until the next unstable release development cycle has begun before it has a chance to be merged.
