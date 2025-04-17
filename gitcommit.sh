#!/bin/bash

# Prompt for GitHub username
read -p "Enter your GitHub username: " username

# Prompt for repository name
read -p "Enter the repository name: " reponame

# Initialize git repository
git init

# Add all files
git add .

# Commit changes
git commit -m "Initial commit"

# Create a new private repository on GitHub using GitHub CLI (gh)
gh repo create "$reponame" --private --source=. --remote=origin

# Set remote origin
git remote add origin "https://github.com/$username/$reponame.git"

# Push to GitHub
git branch -M main
git push -u origin main

echo "Private repository successfully created and pushed to GitHub!"
