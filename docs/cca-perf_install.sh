#!/bin/bash
## Install GitHub CLI
# https://github.com/cli/cli/blob/trunk/docs/install_linux.md

(type -p wget >/dev/null || (sudo apt update && sudo apt-get install wget -y)) \
	&& sudo mkdir -p -m 755 /etc/apt/keyrings \
	&& wget -qO- https://cli.github.com/packages/githubcli-archive-keyring.gpg | sudo tee /etc/apt/keyrings/githubcli-archive-keyring.gpg > /dev/null \
	&& sudo chmod go+r /etc/apt/keyrings/githubcli-archive-keyring.gpg \
	&& echo "deb [arch=$(dpkg --print-architecture) signed-by=/etc/apt/keyrings/githubcli-archive-keyring.gpg] https://cli.github.com/packages stable main" | sudo tee /etc/apt/sources.list.d/github-cli.list > /dev/null \
	&& sudo apt update \
	&& sudo apt install gh -y
	
gh auth login
## gh auth login steps (recommended)
# 1. GitHub.com
# 2. HTTPS
# 3. Y
# 4. Login with a web browser
# 5. Copy the one-time code and Press Enter to open github.com in the default browser
# 6. Sign in with your github account and follow the instructions showed in the browser
# 7. If the console doesn't show the message "Authentication complete.", try these whole process again until it works, starting from the "gh auth login" command.

## Install cca-perf repository
cd ~/ns-3-dev/scratch
git clone https://github.com/sandjorge/cca-perf.git
cd ~/ns-3-dev/scratch/cca-perf
git checkout ns-3.41-main
mkdir out
bash main-lib-install.sh
ns3 build


