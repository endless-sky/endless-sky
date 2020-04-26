### Posting issues

The [issues page](https://github.com/endless-sky/endless-sky/issues) on GitHub is for tracking bugs and feature requests. When posting a new issue, please:

* Check to make sure it's not a duplicate of an existing issue.
* Create a separate "issue" for each bug you are reporting and each feature you are requesting.
* Do not use the issues page for things other than bug reports and feature requests.

If requesting a new feature, first ask yourself: will this make the game more fun or interesting? Remember that this is a game, not a simulator. Changes will not be made purely for the sake of realism, especially if they introduce needless complexity or aggravation.

### Posting pull requestssdfsdf

If you are posting a pull request, please:

* Do not combine multiple unrelated changes into a single pull.
* Check the diff and make sure the pull request does not contain unintended changes.
* If changing the C++ code, follow the [coding standard](http://endless-sky.github.io/styleguide/styleguide.xml).

If proposing a major pull request, start by posting an issue and discussing the best way to implement it. Often the first strategy that occurs to you will not be the cleanest or most effective way to implement a new feature. I will not merge pull requests that are too large for me to read through the diff and check that the change will not introduce bugs.

### Closing issues

If you believe your issue has been resolved, you can close the issue yourself. I won't close an issue unless it has been idle for a few weeks, to avoid having me mark something as fixed when the original poster does not think their request has been fully addressed.

If an issue is a bug and it has been fixed in the code, it may be helpful to leave it "open" until an official release that fixes the bug has been made, so that other people encountering the same bug will see that it has already been reported.

### Issue labels

The labels I assign to issues are:

* bug: Anything where the game is not behaving as intended.
* documentation: Something missing or incorrect in the game documentation.
* balance: A ship or weapon that seems too powerful or useless, or a mission that seems too easy or hard.
* mechanics: A question of whether the game mechanics should be altered.
* enhancement: A request for new functionality in the game engine itself.
* content: A suggestion for new content that could be created without changing the game code.
* question: A question of how something works, or a support question.
* unlikely: An enhancement or other change that I consider lowest priority or too large or difficult.
* unconfirmed: More information is needed to be sure this bug is really a bug.
* wontfix: A change that I definitely do not think should be made.
