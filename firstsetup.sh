#!/bin/bash

GITHUBUSERNAME=`git config user.github`
GITHUBUSERREMOTE=`git remote -v | grep upstream | awk '{print $2}' | head -n 1 | cut -d / -f 2`
git remote add origin git@github.com:${GITHUBUSERNAME}/${GITHUBUSERREMOTE}

# Add the remaining forks
git remote add IPNL-CMS https://github.com/IPNL-CMS/plotIt.git
git remote add blinkseb https://github.com/blinkseb/plotIt.git
git remote add cp3-llbb https://github.com/cp3-llbb/plotIt.git
git remote add OlivierBondu https://github.com/OlivierBondu/plotIt.git

