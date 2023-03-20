#!/bin/bash -xe

repo_name=upstream
branch_name=master
old_branch=$(git branch --show-current)

git remote add ${repo_name} git@github.com:endless-sky/endless-sky.git
git switch ${branch_name}
git fetch ${repo_name}
git rebase ${repo_name}/${branch_name}
git remote remove ${repo_name}
git push origin ${branch_name}
git switch ${old_branch}
